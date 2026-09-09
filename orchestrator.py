import argparse
import dataclasses
import difflib
import hashlib
import json
import os
import pathlib
import re
import shutil
import signal
import subprocess
import sys
import time
import uuid
from typing import Any, Callable, Dict, List, Optional, Set, Tuple

# ---------------------------------------------------------------------------
# Constants & Configuration
# ---------------------------------------------------------------------------
DEFAULT_WORKER_MODEL = "gemini-3.8-flash-medium"
ALLOWED_WORKER_MODELS = {
    "gemini-3.8-flash-high",
    "gemini-3.8-flash-medium",
    "gemini-3.8-flash-low",
}
DEFAULT_TIMEOUT_SECONDS = 600
REVIEW_TIMEOUT_SECONDS = 300
PLAN_TIMEOUT_SECONDS = 300
MAX_ATTEMPTS = 3

EXCLUDED_DIR_NAMES = {
    ".git",
    ".idea",
    ".orchestrator_snapshots",
    "__pycache__",
    ".pytest_cache",
    ".mypy_cache",
    ".vs",
    "build",
    "Build",
    "dist",
    "out",
    "node_modules",
    "Binaries",
    "Intermediate",
    "DerivedDataCache",
    "Saved",
}

ROOT_EXCLUDED_DIR_NAMES = {
    "build-asan",
    ".codebase-memory",
    "Testing",
}

EXCLUDED_FILE_NAMES = {
    ".orchestrator.lock",
    "orchestrator_log.md",
}

ROOT_EXCLUDED_FILE_NAMES = {
    "LightYears.7z",
    "LightYears.log",
}

PROTECTED_FILES = {
    "ORCHESTRATOR.md",
}

SECRET_KEY_PATTERNS = re.compile(
    r"(api[_-]?key|secret|token|password|auth|private[_-]?key|credential)",
    re.IGNORECASE,
)

GENERIC_SECRET_PATTERNS = [
    re.compile(r"AIza[0-9A-Za-z\-_]{20,}"),
    re.compile(r"Bearer\s+[a-zA-Z0-9_\-\.]+", re.IGNORECASE),
    re.compile(r"sk-[a-zA-Z0-9]{20,}"),
    re.compile(r"(ghp|gho|ghu|ghs|ghr)_[A-Za-z0-9_]{36,}"),
]

KEY_VALUE_SECRET_PATTERN = re.compile(
    r"""(?i)(api[_-]?key|secret|token|password|auth|private[_-]?key|credential)\s*[:=]\s*(['"]?)([^\s,;'"\}\]\)]+)\2"""
)

JSON_SECRET_PATTERN = re.compile(
    r"""(?i)"(api[_-]?key|secret|token|password|auth|private[_-]?key|credential)"\s*:\s*"([^"]+)"""
)

INITIAL_ORCHESTRATOR_CONTENT = """# LightYears Bounded Standalone Orchestrator

## Overview & Architecture Context
The LightYears Orchestrator is a bounded standalone Python tooling layer (`orchestrator.py`) designed to orchestrate incremental, safe software development across the LightYears repository without altering runtime code or CMake build configurations outside validated tasks.

### Architectural Context & Project References
- Core Project Rules & Contracts: [`docs/IDENTITY_AND_CONTRACT_RULES.md`](docs/IDENTITY_AND_CONTRACT_RULES.md)
- Architecture Documentation: [`docs/PROJECT_DOCUMENTATION.md`](docs/PROJECT_DOCUMENTATION.md)
- Agent Configuration & Roles: [`AGENTS.md`](AGENTS.md)
- Repository Structure: C++17 modular architecture built with CMake, strictly partitioned into Engine, Game, and SpaceAbilitySystem modules.
- Note on Git Status: The existing root `.gitignore` ignores markdown files by pattern; documentation files are tracked/managed explicitly, and `.gitignore` must remain unmodified.

---

## Project Architecture
- **Engine Module**: Core runtime framework, application loop, windowing, asset streaming, base rendering, and platform abstraction layers.
- **Game Module**: Concrete gameplay actors, game modes, player controllers, space flight mechanics, level progression, and UI integration.
- **SpaceAbilitySystem (SAS)**: Distinct ability system maintaining strict separation between gameplay logic, ability cooldowns/tags/attributes, and presentation layers without loadout duplication.
- **Presentation Profile Architecture**: Ability visuals and telemetry strictly adhere to typed presentation profiles under `presentation/ability/<family>/` resolved via `PresentationProfileRegistry<ConcreteProfile>`. Universal structs or `std::any` catch-alls are forbidden.

---

## Important Systems
- **Transaction & Snapshot Subsystem**: Filesystem snapshots taken before any mutation. Rollback guarantees that failed or interrupted tasks revert cleanly to the before-state while preserving earlier accepted tasks.
- **Exclusion Boundaries**:
  - Global Excluded Directories: `.git`, `.idea`, `.orchestrator_snapshots`, `__pycache__`, `.pytest_cache`, `.mypy_cache`, `.vs`, `build`, `Build`, `dist`, `out`, `node_modules`, `Binaries`, `Intermediate`, `DerivedDataCache`, `Saved`.
  - Root-only Excluded Directories: `build-asan`, `.codebase-memory`, `Testing`.
  - Root-only Excluded Files: `LightYears.7z`, `LightYears.log`.
  - Excluded Files: `.orchestrator.lock`, `orchestrator_log.md`.
- **Atomic Lock Subsystem**: Uses atomic `os.open(O_CREAT | O_EXCL)` on `.orchestrator_snapshots/.orchestrator.lock` to prevent concurrent orchestrator processes on the same workspace without racy check-then-create gaps or racy unlinking.
- **Unified Diff Engine**: Computes unified diffs against frozen before and after snapshots (never live workspace). Added/deleted files produce unified content diffs against `/dev/null`. Reparse points and symlinks fail closed.
- **Logging & Redaction**: Root-level append-only `orchestrator_log.md` records all worker invocations, stdout, stderr, return codes, diffs, reviewer notes, and rollback events. Sensitive secrets (API keys, env credentials, JSON fields) are strictly redacted before serialization and logging.

---

## Implementation Rules
- Priority: Correctness and invariants > scope containment > build-and-test evidence > documentation.
- Non-zero subprocess exit codes are always treated as hard failures.
- No silent fallback or swallow of exceptions during restoration or verification.
- Protected files (`ORCHESTRATOR.md`) must never be modified by workers or planners. Only append-only durable additions are permitted by the final validator.
- Clean recovery on `KeyboardInterrupt` / `SIGINT`: running child processes are killed immediately, and the workspace is rolled back to the pre-task snapshot.

---

## Worker Rules
1. **Implement Only**: Confine all modifications strictly to the assigned task or step.
2. **Inspect Direct Owners First**: Always examine existing canonical owners and headers before writing new classes or functions.
3. **Preserve Architecture**: Respect modular boundaries between Engine, Game, and SpaceAbilitySystem. Do not cross module abstractions or create circular dependencies.
4. **No Unrelated Refactor or Balancing**: Do not reformat adjacent files, balance gameplay stats, or restructure code beyond the explicit scope.
5. **No Git Commands**: Under no circumstances execute `git`, `git commit`, `git stash`, `git restore`, or repository history commands.
6. **No ORCHESTRATOR Modifications**: The worker must never alter `ORCHESTRATOR.md`.
7. **Tooling Constraint**: Use native file tools ONLY. Do NOT use directory listing (`list_dir`, `find_by_name`, `ls`, `dir`) or terminal command execution tools. All file operations must use absolute paths within the current workspace.
8. **Structured Reporting**: Always report:
   - Ambiguities encountered
   - Explicit list of changed files
   - Summary of modifications
   - Validation performed
   - Potential concerns or edge cases
9. **Invocation Specification**:
   - Executable: Antigravity CLI (`agy`). (Obsolete Gemini CLI is forbidden).
   - Mode: `--mode accept-edits`
   - Model: `--model <worker-model>` (validated against `agy models`)
   - Output format: `--output-format json`
   - Task prompt points to absolute task file in `.orchestrator_snapshots`.
   - Never use `--dangerously-skip-permissions` or bypass flags.
   - Any worker response containing `denied_actions`, non-`SUCCESS` status, invalid JSON, or empty response constitutes immediate failure.

---

## Reviewer Rules
1. **Read-Only Codex Process**:
   - Invocation: `codex exec --sandbox read-only --skip-git-repo-check --ephemeral -o <output-file> -`
   - Review prompt includes `ORCHESTRATOR.md` content, unified diff between snapshots, prior accepted task summaries, plan context, and worker report.
   - Requires final output file `-o`. If output file is missing or exit code != 0, review fails.
   - Must never mutate workspace. Before/after diff check verifies read-only integrity.
2. **Strict First Line Protocol**:
   - The very first line of the output file (without stripping leading blank lines) MUST be:
     - `ONAYLANDI` for full approval, OR
     - `DUZELTME: <nonempty instruction>` for required corrections.
   - Any missing status, malformed first line, or empty correction is rejected.
   - The full correction body (including subsequent lines) is preserved and forwarded to the worker for retry.
3. **Independent Review**: Every review runs as a fresh process. Reviewer is always invoked even if worker fails, but worker failure can never be approved.
4. **Three-Attempt Exhaustion**: Up to 3 attempts per task. If attempt 3 fails, print `MANUEL MÜDAHALE GEREKİYOR`, record failure in history, restore workspace, retain snapshot, and stop execution.

---

## Confirmed Project Decisions
1. **Tooling Environment**: Standalone Python standard library implementation (`orchestrator.py`) without third-party runtime package dependencies.
2. **Execution Environment**: Windows PowerShell / Win32 process execution with strict process-tree killing on timeout (`taskkill /PID <pid> /T /F`).
3. **Plan Format**: Ordered `## ADIM N: Title` headings with preserved step body and completion tracking (`[x]`).
4. **Final Validation**: Fresh `codex exec --sandbox workspace-write` running project build/tests. Only exact plan checkmark additions and append-only `ORCHESTRATOR.md` decisions are permitted.
5. **No Git Integration**: Orchestrator relies solely on internal atomic snapshot/diff/rollback mechanisms, completely decoupled from git working trees.
"""

# ---------------------------------------------------------------------------
# Data Classes
# ---------------------------------------------------------------------------
@dataclasses.dataclass
class FileState:
    path: str  # relative path using forward slashes
    is_dir: bool
    is_binary: bool
    size: int
    content_hash: str


@dataclasses.dataclass
class DiffSummary:
    added_files: List[str] = dataclasses.field(default_factory=list)
    deleted_files: List[str] = dataclasses.field(default_factory=list)
    modified_text_files: Dict[str, str] = dataclasses.field(default_factory=dict)  # relpath -> unified diff
    modified_binary_files: List[str] = dataclasses.field(default_factory=list)
    type_changed_files: List[str] = dataclasses.field(default_factory=list)
    added_dirs: List[str] = dataclasses.field(default_factory=list)
    deleted_dirs: List[str] = dataclasses.field(default_factory=list)

    @property
    def has_changes(self) -> bool:
        return bool(
            self.added_files
            or self.deleted_files
            or self.modified_text_files
            or self.modified_binary_files
            or self.type_changed_files
            or self.added_dirs
            or self.deleted_dirs
        )

    def format_diff_text(self) -> str:
        lines = []
        if self.added_dirs:
            lines.append("=== ADDED DIRECTORIES ===")
            for d in sorted(self.added_dirs):
                lines.append(f"  + [DIR] {d}")
        if self.deleted_dirs:
            lines.append("=== DELETED DIRECTORIES ===")
            for d in sorted(self.deleted_dirs):
                lines.append(f"  - [DIR] {d}")
        if self.added_files:
            lines.append("=== ADDED FILES ===")
            for f in sorted(self.added_files):
                lines.append(f"  + {f}")
        if self.deleted_files:
            lines.append("=== DELETED FILES ===")
            for f in sorted(self.deleted_files):
                lines.append(f"  - {f}")
        if self.type_changed_files:
            lines.append("=== TYPE CHANGED ===")
            for f in sorted(self.type_changed_files):
                lines.append(f"  ! {f}")
        if self.modified_binary_files:
            lines.append("=== MODIFIED BINARY FILES ===")
            for f in sorted(self.modified_binary_files):
                lines.append(f"Binary file changed: {f}")
        if self.modified_text_files:
            lines.append("=== MODIFIED TEXT FILES ===")
            for f in sorted(self.modified_text_files.keys()):
                lines.append(f"--- {f}")
                lines.append(self.modified_text_files[f])
        return "\n".join(lines)


@dataclasses.dataclass
class PlanStep:
    number: int
    title: str
    completed: bool = False
    body: str = ""
    raw_heading: str = ""


@dataclasses.dataclass
class SubprocessResult:
    returncode: int
    stdout: str
    stderr: str
    timed_out: bool = False


# ---------------------------------------------------------------------------
# Redaction & Logging
# ---------------------------------------------------------------------------
def get_env_secrets() -> Set[str]:
    secrets = set()
    for k, v in os.environ.items():
        if len(v) >= 4 and SECRET_KEY_PATTERNS.search(k):
            secrets.add(v)
    return secrets


def redact_secrets(text: str, custom_secrets: Optional[Set[str]] = None) -> str:
    if not text:
        return ""
    result = text

    all_secrets = set(custom_secrets or set())
    all_secrets.update(get_env_secrets())

    for sec in sorted(all_secrets, key=len, reverse=True):
        if sec:
            result = result.replace(sec, "[REDACTED_SECRET]")
            try:
                escaped = json.dumps(sec)[1:-1]
                if escaped and escaped != sec:
                    result = result.replace(escaped, "[REDACTED_SECRET]")
            except Exception:
                pass

    result = JSON_SECRET_PATTERN.sub(r'"\1": "[REDACTED_SECRET]"', result)
    result = KEY_VALUE_SECRET_PATTERN.sub(r'\1=\2[REDACTED_SECRET]\2', result)

    for pat in GENERIC_SECRET_PATTERNS:
        result = pat.sub("[REDACTED_SECRET]", result)

    return result


def redact_data(obj: Any, custom_secrets: Optional[Set[str]] = None) -> Any:
    """Recursively redacts secrets in nested data structures before JSON serialization."""
    if isinstance(obj, str):
        return redact_secrets(obj, custom_secrets)
    elif isinstance(obj, dict):
        new_dict = {}
        for k, v in obj.items():
            redacted_k = redact_secrets(str(k), custom_secrets) if isinstance(k, str) else k
            if isinstance(k, str) and SECRET_KEY_PATTERNS.search(k):
                new_dict[redacted_k] = "[REDACTED_SECRET]"
            else:
                new_dict[redacted_k] = redact_data(v, custom_secrets)
        return new_dict
    elif isinstance(obj, list):
        return [redact_data(item, custom_secrets) for item in obj]
    elif isinstance(obj, tuple):
        return tuple(redact_data(item, custom_secrets) for item in obj)
    return obj


def log_event(log_file: pathlib.Path, event_name: str, payload: dict) -> None:
    redacted_payload = redact_data(payload)
    timestamp = time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime())
    serialized = json.dumps(redacted_payload, indent=2, ensure_ascii=False)
    entry = f"### [{timestamp}] Event: {event_name}\n```json\n{serialized}\n```\n\n"
    try:
        log_file.parent.mkdir(parents=True, exist_ok=True)
        with open(log_file, "a", encoding="utf-8") as f:
            f.write(entry)
    except Exception as e:
        print(f"[LOG ERROR] Failed to write event '{event_name}' to {log_file}: {e}", file=sys.stderr)
        raise


# ---------------------------------------------------------------------------
# Process Execution & Tree Termination
# ---------------------------------------------------------------------------
def kill_process_tree(pid: int) -> None:
    if sys.platform == "win32":
        try:
            subprocess.run(
                ["taskkill", "/PID", str(pid), "/T", "/F"],
                shell=False,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
            )
        except Exception:
            pass
    else:
        try:
            pgid = os.getpgid(pid)
            os.killpg(pgid, signal.SIGKILL)
        except Exception:
            try:
                os.kill(pid, signal.SIGKILL)
            except Exception:
                pass


class ProcessRunner:
    """Injectable subprocess runner with process tracking and tree termination."""

    def __init__(self):
        self.active_proc: Optional[subprocess.Popen] = None

    def kill_active(self) -> None:
        if self.active_proc and self.active_proc.poll() is None:
            kill_process_tree(self.active_proc.pid)

    def run(
        self,
        cmd: List[str],
        cwd: pathlib.Path,
        stdin_data: Optional[str] = None,
        timeout: Optional[float] = None,
    ) -> SubprocessResult:
        preexec = None
        creationflags = 0
        if sys.platform != "win32":
            preexec = os.setsid

        proc = None
        try:
            proc = subprocess.Popen(
                cmd,
                cwd=str(cwd),
                stdin=subprocess.PIPE if stdin_data is not None else None,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                shell=False,
                text=True,
                encoding="utf-8",
                errors="replace",
                preexec_fn=preexec,
                creationflags=creationflags,
            )
            self.active_proc = proc

            stdout, stderr = proc.communicate(input=stdin_data, timeout=timeout)
            return SubprocessResult(
                returncode=proc.returncode,
                stdout=stdout,
                stderr=stderr,
                timed_out=False,
            )
        except subprocess.TimeoutExpired:
            if proc:
                kill_process_tree(proc.pid)
                try:
                    stdout, stderr = proc.communicate(timeout=2.0)
                except Exception:
                    stdout, stderr = "", ""
            return SubprocessResult(
                returncode=-1,
                stdout=stdout or "",
                stderr=(stderr or "") + "\n[Process timed out]",
                timed_out=True,
            )
        except KeyboardInterrupt:
            if proc:
                kill_process_tree(proc.pid)
            raise
        except Exception as e:
            if proc:
                kill_process_tree(proc.pid)
            return SubprocessResult(
                returncode=-1,
                stdout="",
                stderr=f"[Execution error: {e}]",
                timed_out=False,
            )
        finally:
            self.active_proc = None


# ---------------------------------------------------------------------------
# Atomic Lock Management
# ---------------------------------------------------------------------------
class WorkspaceLock:
    def __init__(self, lock_path: pathlib.Path):
        self.lock_path = lock_path.resolve()
        self.acquired = False

    @property
    def recovery_message(self) -> str:
        return (
            "Error: Another orchestrator process holds the lock for this workspace. "
            f"If stale, verify no process is running and remove: {self.lock_path}"
        )

    def acquire(self) -> bool:
        self.lock_path.parent.mkdir(parents=True, exist_ok=True)
        pid = os.getpid()
        try:
            fd = os.open(str(self.lock_path), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
            with os.fdopen(fd, "w", encoding="utf-8") as f:
                f.write(f"{pid}\n")
            self.acquired = True
            return True
        except (FileExistsError, OSError):
            return False

    def release(self) -> None:
        if self.acquired:
            try:
                if self.lock_path.exists():
                    content = self.lock_path.read_text(encoding="utf-8").strip()
                    if content == str(os.getpid()):
                        self.lock_path.unlink(missing_ok=True)
            except Exception:
                pass
            self.acquired = False


# ---------------------------------------------------------------------------
# Snapshot & Diff Engine
# ---------------------------------------------------------------------------
class SnapshotEngine:
    def __init__(
        self,
        workspace: pathlib.Path,
        snapshots_dir: pathlib.Path,
        excluded_dirs: Optional[Set[str]] = None,
        excluded_files: Optional[Set[str]] = None,
        root_excluded_dirs: Optional[Set[str]] = None,
        root_excluded_files: Optional[Set[str]] = None,
    ):
        self.workspace = workspace.resolve()
        self.snapshots_dir = snapshots_dir.resolve()
        self.excluded_dirs = set(excluded_dirs or EXCLUDED_DIR_NAMES)
        self.excluded_files = set(excluded_files or EXCLUDED_FILE_NAMES)
        self.root_excluded_dirs = set(root_excluded_dirs or ROOT_EXCLUDED_DIR_NAMES)
        self.root_excluded_files = set(root_excluded_files or ROOT_EXCLUDED_FILE_NAMES)

    def is_excluded(self, rel_path: pathlib.Path) -> bool:
        parts = rel_path.parts
        if not parts:
            return False
        if parts[0] in self.root_excluded_dirs or parts[0] in self.root_excluded_files:
            return True
        for part in parts:
            if part in self.excluded_dirs:
                return True
        if parts[-1] in self.excluded_files:
            return True
        # Purely lexical check for snapshots_dir confinement
        try:
            full_lexical = self.workspace / rel_path
            # Check relative_to without resolving symlinks/reparse points
            rel_to_snap = full_lexical.relative_to(self.snapshots_dir)
            return True
        except ValueError:
            pass
        return False

    def is_reparse_point_or_symlink(self, path: pathlib.Path) -> bool:
        if path.is_symlink():
            return True
        if hasattr(path, "is_junction") and path.is_junction():
            return True
        try:
            st = os.lstat(path)
            if sys.platform == "win32" and hasattr(st, "st_file_attributes"):
                if st.st_file_attributes & 0x0400:  # FILE_ATTRIBUTE_REPARSE_POINT
                    return True
        except Exception:
            pass
        return False

    def _validate_snapshot_boundary(self, snap_dir: pathlib.Path) -> None:
        try:
            # Must be strictly inside snapshots_dir
            snap_dir.relative_to(self.snapshots_dir)
            if snap_dir == self.snapshots_dir:
                raise ValueError("Cannot target snapshots_dir directly")
        except Exception as e:
            raise RuntimeError(f"Unsafe snapshot directory boundary: {snap_dir}: {e}")

    def scan_workspace(self) -> Dict[str, FileState]:
        # Refuse unsafe snapshot root junction before any write or scan
        if self.is_reparse_point_or_symlink(self.snapshots_dir):
            raise RuntimeError(f"Unsafe snapshot root junction/symlink detected: {self.snapshots_dir}")

        states: Dict[str, FileState] = {}

        def on_walk_error(err: OSError):
            raise err

        for root, dirs, files in os.walk(self.workspace, topdown=True, onerror=on_walk_error):
            root_path = pathlib.Path(root)
            rel_root = root_path.relative_to(self.workspace)

            filtered_dirs = []
            for d in sorted(dirs):
                dir_rel = rel_root / d
                full_dir = root_path / d
                if self.is_excluded(dir_rel):
                    continue
                if self.is_reparse_point_or_symlink(full_dir):
                    raise RuntimeError(f"Unsafe included reparse point or symlink detected: {dir_rel.as_posix()}")
                filtered_dirs.append(d)
            dirs[:] = filtered_dirs

            norm_rel_root = rel_root.as_posix()
            if norm_rel_root != ".":
                states[norm_rel_root] = FileState(
                    path=norm_rel_root,
                    is_dir=True,
                    is_binary=False,
                    size=0,
                    content_hash="",
                )

            for f in sorted(files):
                rel_file = rel_root / f
                if self.is_excluded(rel_file):
                    continue
                full_file = root_path / f
                if self.is_reparse_point_or_symlink(full_file):
                    raise RuntimeError(f"Unsafe included reparse point or symlink detected: {rel_file.as_posix()}")

                norm_rel = rel_file.as_posix()
                try:
                    size = full_file.stat().st_size
                    with open(full_file, "rb") as fh:
                        content = fh.read()
                except Exception as e:
                    raise RuntimeError(f"Inaccessible file detected during scan: {norm_rel}: {e}")

                h = hashlib.sha256(content).hexdigest()
                is_bin = False
                if b"\x00" in content[:8192]:
                    is_bin = True
                else:
                    try:
                        content.decode("utf-8")
                    except UnicodeDecodeError:
                        is_bin = True

                states[norm_rel] = FileState(
                    path=norm_rel,
                    is_dir=False,
                    is_binary=is_bin,
                    size=size,
                    content_hash=h,
                )
        return states

    def create_snapshot(self, snapshot_id: str) -> pathlib.Path:
        # Refuse unsafe snapshot root junction before any write
        if self.is_reparse_point_or_symlink(self.snapshots_dir):
            raise RuntimeError(f"Unsafe snapshot root junction/symlink detected: {self.snapshots_dir}")

        target_dir = (self.snapshots_dir / snapshot_id).resolve()
        self._validate_snapshot_boundary(target_dir)

        if target_dir.exists():
            raise RuntimeError(f"Snapshot '{snapshot_id}' already exists and is immutable.")
        target_dir.mkdir(parents=True, exist_ok=False)

        files_dir = target_dir / "files"
        files_dir.mkdir(parents=True, exist_ok=True)

        states = self.scan_workspace()
        manifest_data = {k: dataclasses.asdict(st) for k, st in sorted(states.items())}

        for rel, st in states.items():
            src_path = self.workspace / rel
            dest_path = files_dir / rel
            if st.is_dir:
                dest_path.mkdir(parents=True, exist_ok=True)
            else:
                dest_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src_path, dest_path)

        manifest_file = target_dir / "manifest.json"
        manifest_file.write_text(json.dumps(manifest_data, indent=2), encoding="utf-8")
        return target_dir

    def _read_manifest(self, snap_dir: pathlib.Path) -> Dict[str, FileState]:
        self._validate_snapshot_boundary(snap_dir)
        # Try manifest.json first, fall back to legacy __manifest__.json
        manifest_file = snap_dir / "manifest.json"
        if not manifest_file.exists():
            manifest_file = snap_dir / "__manifest__.json"
        if not manifest_file.exists():
            raise RuntimeError(f"Snapshot manifest missing in: {snap_dir}")
        try:
            data = json.loads(manifest_file.read_text(encoding="utf-8"))
        except Exception as e:
            raise RuntimeError(f"Failed to parse manifest in {snap_dir}: {e}")

        result: Dict[str, FileState] = {}
        for k, v in data.items():
            p = pathlib.Path(k)
            if p.is_absolute() or ".." in p.parts:
                raise RuntimeError(f"Unsafe manifest entry (absolute or traverses parent): {k}")
            try:
                confined = (self.workspace / p).resolve()
                confined.relative_to(self.workspace)
            except Exception:
                raise RuntimeError(f"Unconfined manifest entry: {k}")
            result[k] = FileState(**v)
        return result

    def _get_snapshot_files_root(self, snap_dir: pathlib.Path) -> pathlib.Path:
        files_subdir = snap_dir / "files"
        if files_subdir.is_dir():
            return files_subdir
        return snap_dir

    def compute_diff(
        self,
        before_dir: pathlib.Path,
        after_dir: Optional[pathlib.Path] = None,
    ) -> DiffSummary:
        before_states = self._read_manifest(before_dir)
        before_root = self._get_snapshot_files_root(before_dir)

        if after_dir is not None:
            after_states = self._read_manifest(after_dir)
            after_root = self._get_snapshot_files_root(after_dir)
        else:
            # Fallback to scanning live workspace
            after_states = self.scan_workspace()
            after_root = self.workspace

        diff = DiffSummary()

        # Added & modified
        for rel in sorted(after_states.keys()):
            curr = after_states[rel]
            if rel not in before_states:
                if curr.is_dir:
                    diff.added_dirs.append(rel)
                else:
                    diff.added_files.append(rel)
                    if curr.is_binary:
                        diff.modified_binary_files.append(rel)
                    else:
                        curr_path = after_root / rel
                        try:
                            curr_text = curr_path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)
                            unified = "".join(
                                difflib.unified_diff(
                                    [],
                                    curr_text,
                                    fromfile="/dev/null",
                                    tofile=f"b/{rel}",
                                )
                            )
                            diff.modified_text_files[rel] = unified
                        except Exception:
                            diff.modified_binary_files.append(rel)
            else:
                bef = before_states[rel]
                if bef.is_dir != curr.is_dir:
                    diff.type_changed_files.append(rel)
                elif not curr.is_dir:
                    if bef.content_hash != curr.content_hash:
                        if curr.is_binary or bef.is_binary:
                            diff.modified_binary_files.append(rel)
                        else:
                            bef_path = before_root / rel
                            curr_path = after_root / rel
                            try:
                                bef_text = bef_path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)
                                curr_text = curr_path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)
                                unified = "".join(
                                    difflib.unified_diff(
                                        bef_text,
                                        curr_text,
                                        fromfile=f"a/{rel}",
                                        tofile=f"b/{rel}",
                                    )
                                )
                                diff.modified_text_files[rel] = unified
                            except Exception:
                                diff.modified_binary_files.append(rel)

        # Deleted
        for rel in sorted(before_states.keys()):
            bef = before_states[rel]
            if rel not in after_states:
                if bef.is_dir:
                    diff.deleted_dirs.append(rel)
                else:
                    diff.deleted_files.append(rel)
                    if bef.is_binary:
                        diff.modified_binary_files.append(rel)
                    else:
                        bef_path = before_root / rel
                        try:
                            bef_text = bef_path.read_text(encoding="utf-8", errors="replace").splitlines(keepends=True)
                            unified = "".join(
                                difflib.unified_diff(
                                    bef_text,
                                    [],
                                    fromfile=f"a/{rel}",
                                    tofile="/dev/null",
                                )
                            )
                            diff.modified_text_files[rel] = unified
                        except Exception:
                            diff.modified_binary_files.append(rel)

        return diff

    def restore_snapshot(self, before_dir: pathlib.Path) -> None:
        before_states = self._read_manifest(before_dir)
        before_root = self._get_snapshot_files_root(before_dir)

        # 1. Unlink any newly introduced symlinks / junctions in current workspace
        for root, dirs, files in os.walk(self.workspace, topdown=True):
            root_path = pathlib.Path(root)
            rel_root = root_path.relative_to(self.workspace)

            # Prune excluded dirs before descent
            dirs[:] = [d for d in dirs if not self.is_excluded(rel_root / d)]

            for d in list(dirs):
                full_d = root_path / d
                rel_d = rel_root / d
                if self.is_reparse_point_or_symlink(full_d):
                    try:
                        if full_d.is_symlink():
                            full_d.unlink()
                        else:
                            os.rmdir(str(full_d))
                    except Exception as e:
                        raise RuntimeError(f"Failed to safely unlink newly introduced junction/link {rel_d}: {e}")
            for f in list(files):
                full_f = root_path / f
                rel_f = rel_root / f
                if self.is_excluded(rel_f):
                    continue
                if self.is_reparse_point_or_symlink(full_f):
                    try:
                        if full_f.is_symlink():
                            full_f.unlink()
                        else:
                            os.unlink(str(full_f))
                    except Exception as e:
                        raise RuntimeError(f"Failed to safely unlink newly introduced link {rel_f}: {e}")

        # 2. Rescan current workspace
        current_states = self.scan_workspace()

        # 3. Remove added files & directories not present in before_states (non-recursive)
        # Process files first
        for rel in sorted(current_states.keys()):
            curr = current_states[rel]
            if rel not in before_states or before_states[rel].is_dir != curr.is_dir:
                curr_path = self.workspace / rel
                rel_p = pathlib.Path(rel)
                if self.is_excluded(rel_p):
                    continue
                if not curr.is_dir and curr_path.exists() and not curr_path.is_dir():
                    try:
                        curr_path.unlink(missing_ok=True)
                    except Exception as e:
                        raise RuntimeError(f"Failed to remove added file {rel}: {e}")

        # Process directories bottom-up without recursive rmtree
        for rel in sorted([r for r, st in current_states.items() if st.is_dir], key=lambda p: len(p.split("/")), reverse=True):
            if rel not in before_states or not before_states[rel].is_dir:
                curr_path = self.workspace / rel
                rel_p = pathlib.Path(rel)
                if self.is_excluded(rel_p):
                    continue
                if curr_path.exists() and curr_path.is_dir():
                    try:
                        os.rmdir(str(curr_path))
                    except OSError:
                        # Non-empty directory (e.g. contains excluded files or other children) - leave protected children
                        pass

        # 4. Copy back all before files and create before directories
        # Avoid shutil.rmtree(dst) which would delete excluded contents in file->directory case!
        for rel, bef in sorted(before_states.items()):
            rel_p = pathlib.Path(rel)
            if self.is_excluded(rel_p):
                continue
            src = before_root / rel
            dst = self.workspace / rel

            if bef.is_dir:
                if dst.exists() and not dst.is_dir():
                    dst.unlink()
                dst.mkdir(parents=True, exist_ok=True)
            else:
                if dst.exists() and dst.is_dir():
                    # Attempt safe bottom-up removal of empty dirs; do NOT use shutil.rmtree
                    try:
                        os.rmdir(str(dst))
                    except OSError:
                        raise RuntimeError(
                            f"Cannot restore file {rel}: destination is a directory containing protected or excluded files that cannot be deleted."
                        )
                dst.parent.mkdir(parents=True, exist_ok=True)
                if src.exists():
                    shutil.copy2(src, dst)
                else:
                    raise RuntimeError(f"Missing source file in snapshot to restore: {rel}")

        # 5. Verify rollback matches before snapshot
        diff_verify = self.compute_diff(before_dir)
        if diff_verify.has_changes:
            raise RuntimeError(
                f"Rollback verification failed: workspace does not match before snapshot. Changes:\n{diff_verify.format_diff_text()}"
            )


# ---------------------------------------------------------------------------
# History Sidecar
# ---------------------------------------------------------------------------
class HistoryManager:
    def __init__(self, snapshots_dir: pathlib.Path):
        self.history_file = snapshots_dir / "history.json"

    def record_success(self, task: str, plan_step: Optional[str] = None) -> None:
        records = self.load()
        records.append(
            {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "status": "ACCEPTED",
                "task": redact_secrets(task),
                "plan_step": redact_secrets(plan_step or ""),
            }
        )
        try:
            self.history_file.parent.mkdir(parents=True, exist_ok=True)
            self.history_file.write_text(json.dumps(records, indent=2), encoding="utf-8")
        except Exception:
            pass

    def record_failure(self, task: str, reason: str) -> None:
        records = self.load()
        records.append(
            {
                "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                "status": "REJECTED",
                "task": redact_secrets(task),
                "reason": redact_secrets(reason),
            }
        )
        try:
            self.history_file.parent.mkdir(parents=True, exist_ok=True)
            self.history_file.write_text(json.dumps(records, indent=2), encoding="utf-8")
        except Exception:
            pass

    def load(self) -> List[dict]:
        if not self.history_file.exists():
            return []
        try:
            return json.loads(self.history_file.read_text(encoding="utf-8"))
        except Exception:
            return []

    def get_accepted_summaries(self) -> str:
        records = self.load()
        accepted = [r for r in records if r.get("status") == "ACCEPTED"]
        if not accepted:
            return "No prior accepted tasks."
        lines = []
        for r in accepted:
            step = r.get("plan_step")
            task = r.get("task", "")
            if step:
                lines.append(f"- [{step}] {task}")
            else:
                lines.append(f"- {task}")
        return "\n".join(lines)


# ---------------------------------------------------------------------------
# Plan Parser
# ---------------------------------------------------------------------------
def parse_plan_steps(plan_text: str) -> List[PlanStep]:
    lines = plan_text.strip().splitlines()
    steps: List[PlanStep] = []
    seen_numbers: Set[int] = set()

    step_regex = re.compile(r"^##\s+ADIM\s+(\d+)\s*:\s*(.+)$", re.IGNORECASE)
    in_code_fence = False

    current_step: Optional[PlanStep] = None
    current_body_lines: List[str] = []

    for line in lines:
        stripped = line.strip()
        if stripped.startswith("```"):
            in_code_fence = not in_code_fence
            if current_step:
                current_body_lines.append(line)
            continue

        if in_code_fence:
            if current_step:
                current_body_lines.append(line)
            continue

        match = step_regex.match(stripped)
        if match:
            if current_step:
                current_step.body = "\n".join(current_body_lines).strip()
                steps.append(current_step)
                current_body_lines = []
                current_step = None

            num = int(match.group(1))
            if num <= 0:
                raise ValueError(f"Step number must be positive: ADIM {num}")
            if num in seen_numbers:
                raise ValueError(f"Duplicate step number: ADIM {num}")

            title = match.group(2).strip()
            completed = False
            if title.startswith("[x]") or title.startswith("[X]"):
                completed = True
                title = title[3:].strip()
            elif title.startswith("[ ]"):
                title = title[3:].strip()

            if not title:
                raise ValueError(f"Empty title for ADIM {num}")

            seen_numbers.add(num)
            current_step = PlanStep(
                number=num,
                title=title,
                completed=completed,
                body="",
                raw_heading=stripped,
            )
        else:
            if current_step:
                current_body_lines.append(line)

    if current_step:
        current_step.body = "\n".join(current_body_lines).strip()
        steps.append(current_step)

    if not steps:
        raise ValueError("No valid '## ADIM N: Title' steps found in plan.")

    expected_order = sorted([s.number for s in steps])
    actual_order = [s.number for s in steps]
    if actual_order != expected_order:
        raise ValueError(f"Plan steps out of order: {actual_order}")

    return steps


# ---------------------------------------------------------------------------
# Planner & Inspection Routing
# ---------------------------------------------------------------------------
def is_simple_numeric_edit(task: str) -> bool:
    """Conservative routing: single simple explicit numeric value edit."""
    pattern = re.compile(
        r"^(change|set|update|modify|edit)\s+([a-zA-Z0-9_\.\-\/\\]+)\s+(to|=)\s+(-?\d+(\.\d+)?)$",
        re.IGNORECASE,
    )
    return bool(pattern.match(task.strip()))


def is_inspect_query(task: str) -> bool:
    """Conservative inspect detection supporting English and Turkish keywords."""
    t = task.strip().lower()
    keywords = [
        "inspect",
        "what is",
        "where is",
        "show me",
        "explain",
        "find ",
        "incele",
        "inceleme",
        "kod değiştirme",
        "nedir",
        "nerede",
        "göster",
        "açıkla",
        "bul ",
    ]
    for kw in keywords:
        if kw in t:
            return True
    return False


# ---------------------------------------------------------------------------
# Orchestrator Core Class
# ---------------------------------------------------------------------------
class Orchestrator:
    def __init__(
        self,
        workspace: pathlib.Path,
        worker_model: str = DEFAULT_WORKER_MODEL,
        dry_run: bool = False,
        worker_timeout: int = DEFAULT_TIMEOUT_SECONDS,
        review_timeout: int = REVIEW_TIMEOUT_SECONDS,
        runner: Optional[ProcessRunner] = None,
        timeout: Optional[int] = None,
    ):
        self.workspace = workspace.resolve()
        self.worker_model = worker_model
        self.dry_run = dry_run
        self.worker_timeout = timeout if timeout is not None else worker_timeout
        self.timeout = self.worker_timeout  # For backward compatibility
        self.review_timeout = review_timeout
        self.runner = runner or ProcessRunner()

        self.snapshots_dir = self.workspace / ".orchestrator_snapshots"
        self.lock = WorkspaceLock(self.snapshots_dir / ".orchestrator.lock")
        self.snapshot_engine = SnapshotEngine(self.workspace, self.snapshots_dir)
        self.history = HistoryManager(self.snapshots_dir)
        self.log_file = self.workspace / "orchestrator_log.md"
        self.last_attempt_info: Dict[str, str] = {}

    def ensure_orchestrator_doc(self) -> None:
        """Creates ORCHESTRATOR.md once if missing; never overwrites existing."""
        doc_path = self.workspace / "ORCHESTRATOR.md"
        if not doc_path.exists():
            doc_path.write_text(INITIAL_ORCHESTRATOR_CONTENT, encoding="utf-8")

    def get_orchestrator_doc_content(self) -> str:
        doc_path = self.workspace / "ORCHESTRATOR.md"
        if doc_path.is_file():
            try:
                return doc_path.read_text(encoding="utf-8", errors="replace")
            except Exception:
                pass
        return INITIAL_ORCHESTRATOR_CONTENT

    def startup_checks(self) -> Tuple[bool, str]:
        """Run startup diagnostics: codex exec --help, agy --help, agy models.

        Validates worker model is present as an exact token in agy models.
        """
        res_codex = self.runner.run(["codex", "exec", "--help"], cwd=self.workspace, timeout=15)
        if res_codex.returncode != 0:
            return False, f"Failed to execute 'codex exec --help' (code {res_codex.returncode}): {res_codex.stderr}"

        res_agy = self.runner.run(["agy", "--help"], cwd=self.workspace, timeout=15)
        if res_agy.returncode != 0:
            return False, f"Failed to execute 'agy --help' (code {res_agy.returncode}): {res_agy.stderr}"

        res_models = self.runner.run(["agy", "models"], cwd=self.workspace, timeout=15)
        if res_models.returncode != 0:
            return False, f"Failed to execute 'agy models' (code {res_models.returncode}): {res_models.stderr}"

        if self.worker_model not in ALLOWED_WORKER_MODELS:
            return False, f"Worker model '{self.worker_model}' not in allowed set: {sorted(ALLOWED_WORKER_MODELS)}"

        # Validate exact token in agy models output
        tokens = set(re.findall(r"[A-Za-z0-9_\-\.]+", res_models.stdout))
        if self.worker_model not in tokens:
            return False, f"Worker model '{self.worker_model}' not found in 'agy models' output"

        return True, "Startup checks passed."

    def run_inspect(self, query: str) -> Tuple[bool, str]:
        """Runs an inspect-only query using readonly codex. Asserts no mutations."""
        if self.dry_run:
            return True, f"[DRY-RUN] Inspect query '{query}' previewed successfully."

        snap_id = f"inspect_{uuid.uuid4().hex[:8]}_before"
        before_dir = self.snapshot_engine.create_snapshot(snap_id)
        out_file = self.snapshots_dir / f"inspect_out_{uuid.uuid4().hex[:8]}.txt"

        try:
            cmd = [
                "codex",
                "exec",
                "--sandbox",
                "read-only",
                "--skip-git-repo-check",
                "--ephemeral",
                "-o",
                str(out_file),
                "-",
            ]

            prompt = (
                f"You are a readonly inspector for LightYears.\n"
                f"Answer the following inquiry about the codebase:\n"
                f"{query}\n"
                f"Do not write or modify any files.\n"
            )

            res = self.runner.run(cmd, cwd=self.workspace, stdin_data=prompt, timeout=PLAN_TIMEOUT_SECONDS)

            diff = self.snapshot_engine.compute_diff(before_dir)
            if diff.has_changes:
                self.snapshot_engine.restore_snapshot(before_dir)
                return False, "Inspection violated read-only constraint: mutations detected and reverted."

            if res.returncode != 0 or res.timed_out:
                return False, f"Inspection failed with exit code {res.returncode}: {res.stderr}"

            if not out_file.exists():
                return False, "Inspection failed: no output file produced (-o)."

            inspect_output = out_file.read_text(encoding="utf-8", errors="replace").strip()
            out_file.unlink(missing_ok=True)
            shutil.rmtree(before_dir, ignore_errors=True)
            return True, redact_secrets(inspect_output)
        except (Exception, KeyboardInterrupt) as e:
            self.runner.kill_active()
            try:
                self.snapshot_engine.restore_snapshot(before_dir)
            except Exception as restore_err:
                print(f"[RESTORE ERROR] Failed during inspection rollback: {restore_err}", file=sys.stderr)
            if isinstance(e, KeyboardInterrupt):
                raise
            return False, f"Inspection error: {e}"
        finally:
            out_file.unlink(missing_ok=True)

    def generate_scoped_plan(self, task: str) -> Tuple[bool, str]:
        """Generates a short readonly Codex scope (objective, allowed files, invariants, proof)."""
        if self.dry_run:
            return True, f"[DRY-RUN] Scope for {task}"

        snap_id = f"plan_{uuid.uuid4().hex[:8]}_before"
        before_dir = self.snapshot_engine.create_snapshot(snap_id)
        out_file = self.snapshots_dir / f"scope_out_{uuid.uuid4().hex[:8]}.txt"

        try:
            cmd = [
                "codex",
                "exec",
                "--sandbox",
                "read-only",
                "--skip-git-repo-check",
                "--ephemeral",
                "-o",
                str(out_file),
                "-",
            ]

            orch_content = self.get_orchestrator_doc_content()
            prompt = (
                f"You are a conservative software scoping planner for LightYears.\n\n"
                f"ORCHESTRATOR.md:\n{orch_content}\n\n"
                f"Original Task: {task}\n\n"
                f"Produce a short, concrete execution scope containing:\n"
                f"1. OBJECTIVE\n"
                f"2. ALLOWED FILES (canonical owners only)\n"
                f"3. CANONICAL OWNER\n"
                f"4. INVARIANTS\n"
                f"5. PROOF CRITERIA\n"
                f"Do NOT write or modify any files.\n"
            )

            res = self.runner.run(cmd, cwd=self.workspace, stdin_data=prompt, timeout=PLAN_TIMEOUT_SECONDS)

            diff = self.snapshot_engine.compute_diff(before_dir)
            if diff.has_changes:
                self.snapshot_engine.restore_snapshot(before_dir)
                return False, "Planner mutated workspace. Aborting plan."

            if res.returncode != 0 or res.timed_out:
                return False, f"Scoping planner failed with exit code {res.returncode}: {res.stderr}"

            if not out_file.exists():
                return False, "Scoping planner failed: no output file (-o) produced."

            scope_text = out_file.read_text(encoding="utf-8", errors="replace").strip()
            out_file.unlink(missing_ok=True)
            if not scope_text:
                return False, "Scoping planner produced empty output."
            shutil.rmtree(before_dir, ignore_errors=True)
            return True, scope_text
        except (Exception, KeyboardInterrupt) as e:
            self.runner.kill_active()
            try:
                self.snapshot_engine.restore_snapshot(before_dir)
            except Exception as restore_err:
                print(f"[RESTORE ERROR] Failed during scoping rollback: {restore_err}", file=sys.stderr)
            if isinstance(e, KeyboardInterrupt):
                raise
            return False, f"Scoping error: {e}"
        finally:
            out_file.unlink(missing_ok=True)

    def generate_plan(self, task: str) -> Tuple[bool, str]:
        return self.generate_scoped_plan(task)

    def invoke_worker(
        self,
        task: str,
        correction: str = "",
        plan_context: str = "",
    ) -> SubprocessResult:
        """Invokes agy worker with guardrails, native file tools only, and parses JSON output."""
        if self.dry_run:
            return SubprocessResult(returncode=0, stdout='{"status": "SUCCESS", "report": "[DRY-RUN]"}', stderr="")

        task_id = uuid.uuid4().hex[:8]
        task_file = self.snapshots_dir / f"task_input_{task_id}.txt"
        self.snapshots_dir.mkdir(parents=True, exist_ok=True)

        orch_content = self.get_orchestrator_doc_content()
        full_prompt = (
            f"=== ORCHESTRATOR RULES & INVARIANTS ===\n"
            f"{orch_content}\n\n"
            f"=== IMPLEMENTATION GUARDRAILS ===\n"
            f"1. Implementation-only constraint: strictly implement the requested task.\n"
            f"2. Strictly NO git operations (do NOT run git commit, stash, reset, checkout, branch, or any git commands).\n"
            f"3. No architecture redesign or changes outside canonical owners.\n"
            f"4. No unrelated balance or refactoring edits.\n"
            f"5. Under NO circumstances modify ORCHESTRATOR.md or protected files.\n"
            f"6. If requirements are ambiguous, do NOT guess; report ambiguity in the report.\n"
            f"7. You must produce a structured report containing:\n"
            f"   - ambiguities: (list any ambiguities encountered)\n"
            f"   - changed files: (list of files modified)\n"
            f"   - modifications: (summary of changes made)\n"
            f"   - validation: (validation or test steps verified)\n"
            f"   - concerns: (any potential risks or invariants to watch)\n\n"
            f"=== TASK ===\n{task}\n"
        )
        if plan_context:
            full_prompt += f"\n=== PLAN / SCOPE CONTEXT ===\n{plan_context}\n"
        if correction:
            full_prompt += f"\n=== CORRECTION INSTRUCTION FROM PRIOR REVIEW ===\n{correction}\n"

        task_file.write_text(full_prompt, encoding="utf-8")

        prompt_arg = (
            f"Execute instructions contained in task file: {task_file.resolve().as_posix()}. "
            f"Use native file tools ONLY. Do NOT use directory listing or command execution tools. "
            f"All work must be within absolute workspace: {self.workspace.resolve().as_posix()}."
        )

        cmd = [
            "agy",
            "-p",
            prompt_arg,
            "--mode",
            "accept-edits",
            "--model",
            self.worker_model,
            "--output-format",
            "json",
            "--print-timeout",
            f"{self.worker_timeout}s",
        ]

        try:
            res = self.runner.run(cmd, cwd=self.workspace, timeout=float(self.worker_timeout))

            if res.timed_out:
                return SubprocessResult(
                    returncode=-1,
                    stdout=res.stdout,
                    stderr=res.stderr + "\n[Worker error: execution timed out]",
                    timed_out=True,
                )

            # Validate agy JSON response even if exit code is 0
            if res.returncode == 0:
                raw_out = res.stdout.strip()
                if not raw_out:
                    return SubprocessResult(
                        returncode=1,
                        stdout=res.stdout,
                        stderr=res.stderr + "\n[Worker error: blank response received from agy]",
                        timed_out=False,
                    )
                try:
                    data = json.loads(raw_out)
                except json.JSONDecodeError:
                    return SubprocessResult(
                        returncode=1,
                        stdout=res.stdout,
                        stderr=res.stderr + "\n[Worker error: invalid JSON response received from agy]",
                        timed_out=False,
                    )

                if not isinstance(data, dict) or not data:
                    return SubprocessResult(
                        returncode=1,
                        stdout=res.stdout,
                        stderr=res.stderr + "\n[Worker error: JSON response is not a non-empty dictionary]",
                        timed_out=False,
                    )

                denied = data.get("denied_actions", [])
                if denied:
                    return SubprocessResult(
                        returncode=1,
                        stdout=res.stdout,
                        stderr=res.stderr + f"\n[Worker error: denied actions: {denied}]",
                        timed_out=False,
                    )

                if data.get("status") != "SUCCESS":
                    return SubprocessResult(
                        returncode=1,
                        stdout=res.stdout,
                        stderr=res.stderr + f"\n[Worker error: non-SUCCESS status '{data.get('status')}']",
                        timed_out=False,
                    )

                if data.get("error"):
                    return SubprocessResult(
                        returncode=1,
                        stdout=res.stdout,
                        stderr=res.stderr + f"\n[Worker error: {data.get('error')}]",
                        timed_out=False,
                    )

                report = data.get("report") or data.get("response")
                if not isinstance(report, str) or not report.strip():
                    return SubprocessResult(
                        returncode=1,
                        stdout=res.stdout,
                        stderr=res.stderr + "\n[Worker error: missing or empty report/response string]",
                        timed_out=False,
                    )

            return res
        finally:
            task_file.unlink(missing_ok=True)

    def invoke_reviewer(
        self,
        task: str,
        diff: DiffSummary,
        worker_report: str,
        prior_correction: str = "",
        plan_context: str = "",
        worker_failed: bool = False,
    ) -> Tuple[bool, str]:
        """Invokes fresh readonly Codex reviewer process. Returns (approved, instruction_or_note)."""
        if self.dry_run:
            return True, "ONAYLANDI (dry-run)"

        # Check protected files without early return: still invoke reviewer, then force rejection
        protected_violation = None
        for protected in PROTECTED_FILES:
            if (
                protected in diff.added_files
                or protected in diff.deleted_files
                or protected in diff.modified_text_files
                or protected in diff.modified_binary_files
                or protected in diff.type_changed_files
            ):
                protected_violation = f"Protected file '{protected}' was altered by worker. Automatic rejection."
                break

        diff_text = diff.format_diff_text()
        snap_id = f"review_snap_{uuid.uuid4().hex[:8]}_before"
        before_review_dir = self.snapshot_engine.create_snapshot(snap_id)
        out_file = self.snapshots_dir / f"review_out_{uuid.uuid4().hex[:8]}.txt"

        try:
            cmd = [
                "codex",
                "exec",
                "--sandbox",
                "read-only",
                "--skip-git-repo-check",
                "--ephemeral",
                "-o",
                str(out_file),
                "-",
            ]

            orch_content = self.get_orchestrator_doc_content()
            accepted_summaries = self.history.get_accepted_summaries()

            status_str = "FAILED" if worker_failed else "SUCCESS"
            prompt = (
                f"You are the senior code reviewer for LightYears.\n\n"
                f"ORCHESTRATOR.md CONTENT:\n{orch_content}\n\n"
                f"PRIOR ACCEPTED TASKS:\n{accepted_summaries}\n\n"
                f"TASK REQUEST:\n{task}\n\n"
                f"PLAN CONTEXT:\n{plan_context or 'None'}\n\n"
                f"PRIOR CORRECTION:\n{prior_correction or 'None'}\n\n"
                f"WORKER EXECUTION STATUS: {status_str}\n\n"
                f"WORKER REPORT / STDOUT:\n{worker_report}\n\n"
                f"WORKSPACE DIFF:\n{diff_text}\n\n"
            )
            if protected_violation:
                prompt += f"SECURITY WARNING: {protected_violation}\n\n"

            prompt += (
                f"REVIEW CRITERIA:\n"
                f"- Architecture & invariants alignment\n"
                f"- Scope containment (no unrelated edits or refactorings)\n"
                f"- No regressions introduced\n"
                f"- User request fulfillment\n"
                f"- Strictly NO git operations or file edits permitted\n\n"
                f"STRICT REVIEW OUTPUT PROTOCOL:\n"
                f"The VERY FIRST LINE of your response MUST be EXACTLY one of:\n"
                f"ONAYLANDI\n"
                f"DUZELTME: <nonempty instruction>\n"
                f"Do not output leading blank lines. A non-empty instruction directly on the DUZELTME line is mandatory.\n"
                f"Retain detailed correction feedback on subsequent lines if needed.\n"
            )

            res = self.runner.run(cmd, cwd=self.workspace, stdin_data=prompt, timeout=self.review_timeout)

            # Log Codex role execution details
            log_event(
                self.log_file,
                "reviewer_process_execution",
                {
                    "task": task,
                    "exit_code": res.returncode,
                    "stdout": res.stdout,
                    "stderr": res.stderr,
                    "timed_out": res.timed_out,
                    "worker_failed": worker_failed,
                    "protected_violation": protected_violation,
                },
            )

            # Reviewer must NOT mutate workspace
            review_diff = self.snapshot_engine.compute_diff(before_review_dir)
            if review_diff.has_changes:
                self.snapshot_engine.restore_snapshot(before_review_dir)
                return False, "Reviewer attempted to mutate workspace. Rejected."

            if res.returncode != 0 or res.timed_out:
                fail_msg = f"Reviewer process failed with exit code {res.returncode}: {res.stderr}"
                log_event(self.log_file, "reviewer_final_reply", {"task": task, "approved": False, "reply": fail_msg})
                return False, fail_msg

            if not out_file.exists():
                fail_msg = "Reviewer failed: no output file (-o) produced."
                log_event(self.log_file, "reviewer_final_reply", {"task": task, "approved": False, "reply": fail_msg})
                return False, fail_msg

            review_content = out_file.read_text(encoding="utf-8", errors="replace")
            out_file.unlink(missing_ok=True)

            lines = review_content.splitlines()
            first_line = lines[0] if lines else ""

            approved = False
            feedback = ""

            if first_line == "ONAYLANDI":
                approved = True
                feedback = review_content.strip()
            elif first_line.startswith("DUZELTME:"):
                instruction = first_line[len("DUZELTME:") :].strip()
                if not instruction:
                    approved = False
                    feedback = "DUZELTME instruction was empty on the first line."
                else:
                    approved = False
                    if len(lines) > 1:
                        feedback = instruction + "\n" + "\n".join(lines[1:]).strip()
                    else:
                        feedback = instruction
            else:
                approved = False
                feedback = f"Malformed review response (missing ONAYLANDI/DUZELTME on first line): {first_line}"

            if protected_violation:
                approved = False
                feedback = f"{protected_violation}\nReviewer note: {feedback}"

            if worker_failed:
                approved = False

            log_event(
                self.log_file,
                "reviewer_final_reply",
                {
                    "task": task,
                    "approved": approved,
                    "first_line": first_line,
                    "final_reply": feedback,
                },
            )

            return approved, feedback
        except (Exception, KeyboardInterrupt) as exc:
            self.runner.kill_active()
            try:
                self.snapshot_engine.restore_snapshot(before_review_dir)
            except Exception as restore_err:
                print(f"[RESTORE ERROR] Failed during reviewer rollback: {restore_err}", file=sys.stderr)
            if isinstance(exc, KeyboardInterrupt):
                raise
            return False, f"Reviewer exception: {exc}"
        finally:
            out_file.unlink(missing_ok=True)
            shutil.rmtree(before_review_dir, ignore_errors=True)

    def _cleanup_attempt_snapshots(self, snapshot_dirs: List[pathlib.Path]) -> None:
        """Safely remove attempt snapshot directories upon approval, validating boundaries."""
        for sdir in snapshot_dirs:
            try:
                if sdir.exists():
                    self.snapshot_engine._validate_snapshot_boundary(sdir)
                    shutil.rmtree(sdir, ignore_errors=True)
            except Exception as e:
                print(f"[CLEANUP WARNING] Failed removing attempt snapshot {sdir}: {e}", file=sys.stderr)

    def execute_single_task(
        self,
        task: str,
        plan_step: Optional[str] = None,
        plan_context: str = "",
    ) -> bool:
        """Executes a single task with up to 3 attempts, snapshotting, reviewer check, and rollback on failure."""
        if self.dry_run:
            print(f"[DRY-RUN] Executing task: {task}")
            return True

        # Route inspection queries
        if is_inspect_query(task):
            ok, res = self.run_inspect(task)
            print(res)
            return ok

        task_uuid = uuid.uuid4().hex[:8]
        snap_before_id = f"task_{task_uuid}_before"
        before_dir = self.snapshot_engine.create_snapshot(snap_before_id)
        attempt_snapshots: List[pathlib.Path] = [before_dir]

        # Conservative routing: if not simple numeric edit, obtain scope
        scoped_context = plan_context
        if not is_simple_numeric_edit(task) and not plan_context:
            try:
                scope_ok, scope_text = self.generate_scoped_plan(task)
            except (Exception, KeyboardInterrupt) as exc:
                self.runner.kill_active()
                print(f"[ERROR] Scoping planner exception: {exc}", file=sys.stderr)
                try:
                    self.snapshot_engine.restore_snapshot(before_dir)
                except Exception as restore_err:
                    print(f"[RESTORE ERROR] Failed during scoping rollback: {restore_err}", file=sys.stderr)
                raise
            if not scope_ok or not scope_text.strip():
                print(f"[SCOPING FAILED] Scoping planner failed: {scope_text}", file=sys.stderr)
                try:
                    self.snapshot_engine.restore_snapshot(before_dir)
                except Exception as restore_err:
                    print(f"[RESTORE ERROR] Failed during scoping rollback: {restore_err}", file=sys.stderr)
                return False
            scoped_context = scope_text.strip()

        correction = ""
        success = False
        last_reviewer_note = ""

        try:
            for attempt in range(1, MAX_ATTEMPTS + 1):
                log_event(
                    self.log_file,
                    "task_attempt_start",
                    {"task": task, "attempt": attempt, "model": self.worker_model},
                )

                # 1. Run worker
                worker_res = self.invoke_worker(task, correction, scoped_context)
                worker_failed = (worker_res.returncode != 0 or worker_res.timed_out)

                # 2. Create frozen after snapshot
                snap_after_id = f"task_{task_uuid}_attempt_{attempt}_after"
                after_dir = self.snapshot_engine.create_snapshot(snap_after_id)
                attempt_snapshots.append(after_dir)

                # 3. Compute diff between before snapshot and frozen after snapshot
                diff = self.snapshot_engine.compute_diff(before_dir, after_dir)

                self.last_attempt_info = {
                    "task": redact_secrets(task),
                    "attempt": str(attempt),
                    "worker_stdout": redact_secrets(worker_res.stdout),
                    "worker_stderr": redact_secrets(worker_res.stderr),
                    "diff": redact_secrets(diff.format_diff_text()),
                }

                # 4. Review: Always invoke reviewer even if worker failed, passing worker_failed
                approved, review_note = self.invoke_reviewer(
                    task,
                    diff,
                    worker_res.stdout,
                    correction,
                    scoped_context,
                    worker_failed=worker_failed,
                )

                if worker_failed:
                    approved = False
                    worker_err_desc = f"Worker failed with code {worker_res.returncode}: {worker_res.stderr}"
                    review_note = f"{worker_err_desc}\nReviewer note: {review_note}"

                last_reviewer_note = review_note
                self.last_attempt_info["review"] = redact_secrets(review_note)
                self.last_attempt_info["approved"] = str(approved)

                log_event(
                    self.log_file,
                    "task_attempt_result",
                    {
                        "task": task,
                        "attempt": attempt,
                        "approved": approved,
                        "worker_exit_code": worker_res.returncode,
                        "worker_stdout": worker_res.stdout,
                        "worker_stderr": worker_res.stderr,
                        "diff": diff.format_diff_text(),
                        "review_note": review_note,
                    },
                )

                if approved:
                    success = True
                    self.history.record_success(task, plan_step)
                    log_event(
                        self.log_file,
                        "task_cleanup_snapshots",
                        {
                            "task": task,
                            "attempt": attempt,
                            "cleaned_snapshots": [d.name for d in attempt_snapshots],
                        },
                    )
                    self._cleanup_attempt_snapshots(attempt_snapshots)
                    print(f"[SUCCESS] Task accepted on attempt {attempt}: {task}")
                    return True
                else:
                    print(f"[REJECTED] Attempt {attempt}/{MAX_ATTEMPTS} rejected: {review_note}. Restoring workspace...")
                    log_event(
                        self.log_file,
                        "task_rollback_start",
                        {"task": task, "attempt": attempt, "reason": review_note},
                    )
                    self.snapshot_engine.restore_snapshot(before_dir)
                    log_event(
                        self.log_file,
                        "task_rollback_complete",
                        {"task": task, "attempt": attempt},
                    )
                    correction = review_note

            # All 3 attempts exhausted
            print(f"\nMANUEL MÜDAHALE GEREKİYOR: Task '{task}' failed after {MAX_ATTEMPTS} attempts.")
            print(f"Last reviewer note: {last_reviewer_note}")
            self.history.record_failure(task, f"Exhausted {MAX_ATTEMPTS} attempts. Last note: {last_reviewer_note}")
            # Retain failure snapshots on task failure for post-mortem evidence
            return False
        except (Exception, KeyboardInterrupt) as exc:
            self.runner.kill_active()
            print(f"[INTERRUPT/ERROR] Rolling back task due to exception: {exc}")
            try:
                log_event(
                    self.log_file,
                    "task_exception_rollback",
                    {"task": task, "error": str(exc)},
                )
                self.snapshot_engine.restore_snapshot(before_dir)
            except Exception as restore_err:
                print(f"[RESTORE ERROR] Failed during exception rollback: {restore_err}", file=sys.stderr)
            raise

    def run_final_validation(
        self,
        plan_file_or_text: Any,
        plan_text: Optional[str] = None,
    ) -> Tuple[bool, str]:
        """Runs fresh Codex validation process in workspace-write.

        Permits ONLY exact plan file checkmark transitions ('[x]') and
        append-only additions to ORCHESTRATOR.md.
        """
        if self.dry_run:
            return True, "[DRY-RUN] Final validation simulated successfully."

        if plan_text is None:
            plan_text = str(plan_file_or_text)
            plan_file = self.snapshots_dir / "temp_plan.md"
            self.snapshots_dir.mkdir(parents=True, exist_ok=True)
            plan_file.write_text(plan_text, encoding="utf-8")
        else:
            plan_file = pathlib.Path(plan_file_or_text).resolve()

        # Validate plan file path is inside workspace and not excluded
        try:
            rel_plan_path = plan_file.relative_to(self.workspace)
        except ValueError:
            return False, f"Plan file '{plan_file}' is outside workspace '{self.workspace}'."

        if self.snapshot_engine.is_excluded(rel_plan_path):
            return False, f"Plan file '{plan_file}' is located in an excluded directory or file."

        snap_id = f"validation_{uuid.uuid4().hex[:8]}_before"
        before_dir = self.snapshot_engine.create_snapshot(snap_id)
        out_file = self.snapshots_dir / f"validation_out_{uuid.uuid4().hex[:8]}.txt"

        try:
            cmd = [
                "codex",
                "exec",
                "--sandbox",
                "workspace-write",
                "--skip-git-repo-check",
                "--ephemeral",
                "-o",
                str(out_file),
                "-",
            ]

            prompt = (
                f"You are the final validation agent for LightYears.\n"
                f"Execute project tests and build assertions to verify all plan steps.\n"
                f"Do NOT invent nonexistent build systems. If no build system is present, run content assertions.\n"
                f"Strictly FORBIDDEN from performing any git operations (git status, commit, checkout, etc.).\n"
                f"You may ONLY modify plan checkmarks ('[x]') in the plan file and make append-only additions to ORCHESTRATOR.md.\n"
                f"You are strictly FORBIDDEN from modifying other files, adding/deleting files or directories.\n"
                f"Plan file path: {plan_file.resolve().as_posix()}\n"
                f"Plan content:\n{plan_text}\n\n"
                f"VALIDATION OUTPUT PROTOCOL:\n"
                f"First line MUST be EXACTLY 'ONAYLANDI' if and only if all tests pass.\n"
                f"Do not include any leading blank line.\n"
                f"Subsequent lines MUST contain explicit build/test commands executed and their outcome report.\n"
                f"If tests fail or are unavailable, the first line MUST be DUZELTME: <reason>.\n"
            )

            res = self.runner.run(cmd, cwd=self.workspace, stdin_data=prompt, timeout=self.worker_timeout)

            # Freeze after snapshot for diff verification
            snap_after_id = f"validation_{uuid.uuid4().hex[:8]}_after"
            after_dir = self.snapshot_engine.create_snapshot(snap_after_id)
            diff = self.snapshot_engine.compute_diff(before_dir, after_dir)

            # Strict boundaries check
            # Disallow any deletions, additions of directories/binary/types
            if (
                diff.deleted_files
                or diff.deleted_dirs
                or diff.added_dirs
                or diff.modified_binary_files
                or diff.type_changed_files
            ):
                self.snapshot_engine.restore_snapshot(before_dir)
                return False, "Final validation violated boundary: illegal deletions, binary changes, or type changes."

            rel_plan = plan_file.resolve().relative_to(self.workspace).as_posix()
            allowed_files = {rel_plan, "ORCHESTRATOR.md"}

            for f in diff.added_files:
                if f not in allowed_files:
                    self.snapshot_engine.restore_snapshot(before_dir)
                    return False, f"Final validation added unexpected file: {f}"

            for f in diff.modified_text_files.keys():
                if f not in allowed_files:
                    self.snapshot_engine.restore_snapshot(before_dir)
                    return False, f"Final validation modified forbidden source file: {f}"
                if f == "ORCHESTRATOR.md":
                    # Verify append-only
                    bef_orch = (before_dir / "files" / "ORCHESTRATOR.md").read_text(encoding="utf-8", errors="replace")
                    aft_orch = (self.workspace / "ORCHESTRATOR.md").read_text(encoding="utf-8", errors="replace")
                    if not aft_orch.startswith(bef_orch):
                        self.snapshot_engine.restore_snapshot(before_dir)
                        return False, "Final validation altered existing ORCHESTRATOR.md content (not append-only)."

            if res.returncode != 0 or res.timed_out:
                self.snapshot_engine.restore_snapshot(before_dir)
                return False, f"Final validation process failed (code {res.returncode}): {res.stderr}"

            if not out_file.exists():
                self.snapshot_engine.restore_snapshot(before_dir)
                return False, "Final validation failed: no output file (-o) produced."

            raw_val_output = out_file.read_text(encoding="utf-8", errors="replace")
            out_file.unlink(missing_ok=True)

            val_lines = raw_val_output.splitlines()
            first_line = val_lines[0] if val_lines else ""
            if first_line != "ONAYLANDI":
                self.snapshot_engine.restore_snapshot(before_dir)
                return False, f"Final validation rejected: first line must be EXACT 'ONAYLANDI' (got '{first_line}')"

            subsequent_report = "\n".join(val_lines[1:]).strip()
            if not subsequent_report:
                self.snapshot_engine.restore_snapshot(before_dir)
                return False, "Final validation rejected: missing explicit test/build command and outcome report after status."

            # Verify plan content modulo ONLY heading [x] additions
            bef_plan_file = before_dir / "files" / rel_plan
            orig_plan = bef_plan_file.read_text(encoding="utf-8", errors="replace") if bef_plan_file.exists() else plan_text
            curr_plan_text = plan_file.read_text(encoding="utf-8", errors="replace")

            def strip_headings(t: str) -> str:
                return re.sub(r"(?m)^(\s*##\s+ADIM\s+\d+\s*:\s*)(\[[ xX]\]\s*)?", r"\1", t)

            if strip_headings(orig_plan) != strip_headings(curr_plan_text):
                self.snapshot_engine.restore_snapshot(before_dir)
                return False, "Final validation altered plan body, title, or non-heading content."

            # Ensure plan file checkmarks are actually marked [x], preserving original line endings
            newline = "\r\n" if "\r\n" in curr_plan_text else "\n"
            updated_plan_lines = []
            heading_regex = re.compile(r"^(##\s+ADIM\s+\d+\s*:\s*)(.*)$", re.IGNORECASE)
            modified_plan = False
            for line in curr_plan_text.splitlines():
                m = heading_regex.match(line)
                if m:
                    prefix = m.group(1)
                    title = m.group(2).strip()
                    if title.startswith("[ ]"):
                        title = "[x] " + title[3:].strip()
                        line = prefix + title
                        modified_plan = True
                    elif not title.startswith("[x]") and not title.startswith("[X]"):
                        line = prefix + "[x] " + title
                        modified_plan = True
                updated_plan_lines.append(line)

            if modified_plan:
                plan_file.write_text(newline.join(updated_plan_lines) + (newline if curr_plan_text.endswith(newline) else ""), encoding="utf-8")

            shutil.rmtree(before_dir, ignore_errors=True)
            shutil.rmtree(after_dir, ignore_errors=True)
            return True, raw_val_output.strip()
        except (Exception, KeyboardInterrupt) as e:
            self.runner.kill_active()
            try:
                self.snapshot_engine.restore_snapshot(before_dir)
            except Exception as restore_err:
                print(f"[RESTORE ERROR] Failed during validation rollback: {restore_err}", file=sys.stderr)
            if isinstance(e, KeyboardInterrupt):
                raise
            return False, f"Final validation exception: {e}"
        finally:
            out_file.unlink(missing_ok=True)

    def run_plan(self, plan_content_or_file: str) -> bool:
        """Executes an ordered ADIM plan from file, preserving prior accepted steps on failure."""
        plan_file_path = None
        plan_text = plan_content_or_file
        potential_path = pathlib.Path(plan_content_or_file)
        if potential_path.is_file():
            plan_file_path = potential_path.resolve()
            try:
                rel = plan_file_path.relative_to(self.workspace)
            except ValueError:
                print(f"Error: Plan file '{plan_file_path}' must be inside workspace '{self.workspace}'.", file=sys.stderr)
                return False
            if self.snapshot_engine.is_excluded(rel):
                print(f"Error: Plan file '{plan_file_path}' is in an excluded path.", file=sys.stderr)
                return False
            plan_text = plan_file_path.read_text(encoding="utf-8")

        steps = parse_plan_steps(plan_text)
        print(f"Loaded plan with {len(steps)} steps.")

        if self.dry_run:
            print("[DRY-RUN] Plan parsed and verified successfully.")
            return True

        for step in steps:
            if step.completed:
                print(f"Skipping already completed ADIM {step.number}: {step.title}")
                continue

            print(f"\n--- Starting ADIM {step.number}: {step.title} ---")
            task_desc = f"ADIM {step.number}: {step.title}\n{step.body}" if step.body else f"ADIM {step.number}: {step.title}"
            ok = self.execute_single_task(
                task_desc,
                plan_step=f"ADIM {step.number}",
                plan_context=f"FULL PLAN CONTENT:\n{plan_text}\n\nCURRENT ADIM: ADIM {step.number}: {step.title}",
            )
            if not ok:
                print(f"Plan halted at ADIM {step.number}. Prior approved steps remain preserved.")
                return False
            step.completed = True

        print("\nAll plan steps accepted! Running final validation...")
        if plan_file_path is None:
            plan_file_path = self.snapshots_dir / "temp_plan.md"
            plan_file_path.write_text(plan_text, encoding="utf-8")

        val_ok, val_msg = self.run_final_validation(plan_file_path, plan_text)
        if not val_ok:
            print(f"[FINAL VALIDATION FAILED]: {val_msg}")
            return False

        print(f"[FINAL VALIDATION PASSED]:\n{val_msg}")
        return True

    def interactive_loop(self) -> None:
        print("LightYears Orchestrator Interactive Shell")
        print("Type /help for available commands or enter a task description directly.")

        while True:
            try:
                raw = input("\norchestrator> ").strip()
            except (EOFError, KeyboardInterrupt):
                print("\nExiting orchestrator.")
                break

            if not raw:
                continue

            if raw == "/quit" or raw == "/exit":
                print("Exiting.")
                break
            elif raw == "/help":
                print("Available slash commands:")
                print("  /help           - Display this help message")
                print("  /plan <file>    - Execute an ADIM plan file")
                print("  /history        - Display accepted task history")
                print("  /last           - Show diff and details of the last task attempt")
                print("  /inspect <query>- Run a readonly inspect query (no mutations)")
                print("  /quit, /exit    - Terminate the orchestrator")
            elif raw == "/history":
                hist = self.history.load()
                if not hist:
                    print("No task history recorded.")
                else:
                    for h in hist:
                        print(f"[{h.get('timestamp')}] {h.get('status')}: {h.get('task')}")
            elif raw == "/last":
                if not self.last_attempt_info:
                    print("No attempt information available.")
                else:
                    for k, v in self.last_attempt_info.items():
                        print(f"\n=== {k.upper()} ===")
                        print(v)
            elif raw.startswith("/inspect"):
                query = raw[len("/inspect") :].strip()
                if not query:
                    print("Usage: /inspect <query>")
                else:
                    ok, res = self.run_inspect(query)
                    print(res)
            elif raw.startswith("/plan"):
                arg = raw[len("/plan") :].strip()
                if not arg:
                    print("Usage: /plan <file_path>")
                else:
                    p_path = pathlib.Path(arg)
                    if not p_path.is_file():
                        print(f"Error: Plan file '{arg}' does not exist.")
                    else:
                        try:
                            self.run_plan(str(p_path))
                        except Exception as e:
                            print(f"Plan error: {e}")
            else:
                if is_inspect_query(raw):
                    ok, res = self.run_inspect(raw)
                    print(res)
                else:
                    success = self.execute_single_task(raw)
                    if not success:
                        print("Task execution failed. Exiting interactive session.", file=sys.stderr)
                        return


# ---------------------------------------------------------------------------
# CLI Entry Point
# ---------------------------------------------------------------------------
def main(argv: Optional[List[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="LightYears Bounded Standalone Orchestrator")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-t", "--task", type=str, help="Single task to execute")
    group.add_argument("--inspect", type=str, help="Execute a readonly inspection inquiry")
    group.add_argument("--plan", type=str, help="Path to plan file")

    parser.add_argument("--dry-run", action="store_true", help="Simulate without writing or invoking models")
    parser.add_argument(
        "--worker-model",
        type=str,
        default=DEFAULT_WORKER_MODEL,
        help=f"Worker model ({', '.join(sorted(ALLOWED_WORKER_MODELS))})",
    )
    parser.add_argument("--workspace", type=str, default=".", help="Target workspace path")
    parser.add_argument(
        "--worker-timeout",
        "--timeout",
        dest="worker_timeout",
        type=int,
        default=DEFAULT_TIMEOUT_SECONDS,
        help="Worker timeout in seconds (positive integer)",
    )
    parser.add_argument(
        "--review-timeout",
        type=int,
        default=REVIEW_TIMEOUT_SECONDS,
        help="Reviewer timeout in seconds (positive integer)",
    )

    args = parser.parse_args(argv)

    if args.worker_timeout <= 0:
        print(f"Error: --worker-timeout must be positive, got {args.worker_timeout}", file=sys.stderr)
        return 1
    if args.review_timeout <= 0:
        print(f"Error: --review-timeout must be positive, got {args.review_timeout}", file=sys.stderr)
        return 1

    workspace_path = pathlib.Path(args.workspace).resolve()
    if not workspace_path.exists() or not workspace_path.is_dir():
        print(f"Error: Target workspace directory does not exist: {workspace_path}", file=sys.stderr)
        return 1

    print("=" * 60)
    print(" LightYears Bounded Standalone Orchestrator")
    print(" Planner/Reviewer: Codex (sandbox read-only)")
    print(" Final Validator:  Codex (sandbox workspace-write)")
    print(f" Worker Model:     {args.worker_model}")
    print("=" * 60)

    if args.plan:
        plan_file = pathlib.Path(args.plan).resolve()
        if not plan_file.is_file():
            print(f"Error: Plan file '{args.plan}' not found or is not a file.", file=sys.stderr)
            return 1
        try:
            rel = plan_file.relative_to(workspace_path)
        except ValueError:
            print(f"Error: Plan file '{plan_file}' must be inside workspace '{workspace_path}'.", file=sys.stderr)
            return 1

    orchestrator = Orchestrator(
        workspace=workspace_path,
        worker_model=args.worker_model,
        dry_run=args.dry_run,
        worker_timeout=args.worker_timeout,
        review_timeout=args.review_timeout,
    )

    if args.plan:
        rel = pathlib.Path(args.plan).resolve().relative_to(workspace_path)
        if orchestrator.snapshot_engine.is_excluded(rel):
            print(f"Error: Plan file '{args.plan}' is in an excluded path.", file=sys.stderr)
            return 1

    # Startup preflight checks run on all executions including dry-run
    ok, msg = orchestrator.startup_checks()
    if not ok:
        print(f"Startup check failed: {msg}", file=sys.stderr)
        return 1

    # Early dry-run branch: no lock acquired, no files/snapshots/history created, no models invoked
    if args.dry_run:
        print("[DRY-RUN PREVIEW]")
        print(f"  Workspace: {workspace_path}")
        print(f"  Worker Model: {args.worker_model}")
        print(f"  Worker Timeout: {args.worker_timeout}s | Review Timeout: {args.review_timeout}s")
        print(f"  Exclusions: {sorted(EXCLUDED_DIR_NAMES | ROOT_EXCLUDED_DIR_NAMES)}")
        print(f"  Excluded Files: {sorted(EXCLUDED_FILE_NAMES | ROOT_EXCLUDED_FILE_NAMES)}")
        print("  Planned Process ARGV:")
        print("    - Planner:   ['codex', 'exec', '--sandbox', 'read-only', '--skip-git-repo-check', '--ephemeral', '-o', '<snapshots_dir>/scope_out_<id>.txt', '-']")
        print(f"    - Worker:    ['agy', '-p', '<task_prompt>', '--mode', 'accept-edits', '--model', '{args.worker_model}', '--output-format', 'json', '--print-timeout', '{args.worker_timeout}s']")
        print("    - Reviewer:  ['codex', 'exec', '--sandbox', 'read-only', '--skip-git-repo-check', '--ephemeral', '-o', '<snapshots_dir>/review_out_<id>.txt', '-']")
        print("    - Validator: ['codex', 'exec', '--sandbox', 'workspace-write', '--skip-git-repo-check', '--ephemeral', '-o', '<snapshots_dir>/validation_out_<id>.txt', '-']")
        if args.task:
            print(f"  Mode: Single Task -> '{args.task}'")
        elif args.inspect:
            print(f"  Mode: Inspect -> '{args.inspect}'")
        elif args.plan:
            print(f"  Mode: Plan File -> '{args.plan}'")
            try:
                steps = parse_plan_steps(pathlib.Path(args.plan).read_text(encoding="utf-8"))
                for s in steps:
                    print(f"    - ADIM {s.number}: {s.title} ({'completed' if s.completed else 'pending'})")
            except Exception as e:
                print(f"    - Plan parse preview error: {e}", file=sys.stderr)
                return 1
        else:
            print("  Mode: Interactive REPL (Dry-run preview complete. Exiting.)")
        return 0

    # Acquire lock for live execution
    if not orchestrator.lock.acquire():
        print(orchestrator.lock.recovery_message, file=sys.stderr)
        return 1

    try:
        # Initialize ORCHESTRATOR.md once if missing
        orchestrator.ensure_orchestrator_doc()

        if args.inspect:
            success, output = orchestrator.run_inspect(args.inspect)
            print(output)
            return 0 if success else 1

        if args.plan:
            try:
                success = orchestrator.run_plan(args.plan)
                return 0 if success else 1
            except Exception as e:
                print(f"Plan error: {e}", file=sys.stderr)
                return 1

        if args.task:
            success = orchestrator.execute_single_task(args.task)
            return 0 if success else 1

        orchestrator.interactive_loop()
        return 0
    finally:
        orchestrator.lock.release()


if __name__ == "__main__":
    sys.exit(main())
