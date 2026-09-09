import io
import json
import os
import pathlib
import shutil
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from typing import List, Optional

# Add parent directory to sys.path so orchestrator can be imported directly
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))

import orchestrator
from orchestrator import (
    ALLOWED_WORKER_MODELS,
    DEFAULT_WORKER_MODEL,
    EXCLUDED_DIR_NAMES,
    EXCLUDED_FILE_NAMES,
    ROOT_EXCLUDED_DIR_NAMES,
    ROOT_EXCLUDED_FILE_NAMES,
    DiffSummary,
    FileState,
    HistoryManager,
    Orchestrator,
    PlanStep,
    ProcessRunner,
    SnapshotEngine,
    SubprocessResult,
    WorkspaceLock,
    is_inspect_query,
    is_simple_numeric_edit,
    log_event,
    main,
    parse_plan_steps,
    redact_data,
    redact_secrets,
)


class MockProcessRunner(ProcessRunner):
    """Configurable mock process runner for simulating subprocess calls with agy JSON & -o file output."""

    def __init__(self):
        super().__init__()
        self.commands_run = []
        self.default_result = SubprocessResult(
            returncode=0,
            stdout=f"Available models:\n  {DEFAULT_WORKER_MODEL}\n  gemini-3.8-flash-high\n  gemini-3.8-flash-low\n",
            stderr="",
        )
        self.responses = []

    def set_next_response(self, res):
        self.responses.append(res)

    def run(
        self,
        cmd: List[str],
        cwd: pathlib.Path,
        stdin_data: Optional[str] = None,
        timeout: Optional[float] = None,
    ) -> SubprocessResult:
        self.commands_run.append((cmd, cwd, stdin_data))

        # Auto-handle scoping planner if no explicit response was queued
        if cmd and cmd[0] == "codex" and any("scope_out" in str(a) for a in cmd):
            if not self.responses or (self.responses and not callable(self.responses[0])):
                if "-o" in cmd:
                    out_idx = cmd.index("-o") + 1
                    out_path = pathlib.Path(cmd[out_idx])
                    out_path.write_text("1. OBJECTIVE: Test\n2. ALLOWED FILES: All\n", encoding="utf-8")
                return SubprocessResult(returncode=0, stdout="Scoped", stderr="")

        if self.responses:
            resp = self.responses.pop(0)
            if callable(resp):
                resp = resp(cmd, cwd, stdin_data)
        else:
            resp = self.default_result

        # Auto-populate output file (-o) if command requested one and file not yet created
        if "-o" in cmd:
            out_idx = cmd.index("-o") + 1
            out_path = pathlib.Path(cmd[out_idx])
            if resp.returncode == 0 and not out_path.exists():
                out_content = resp.stdout if resp.stdout else "ONAYLANDI\nAuto approved."
                out_path.parent.mkdir(parents=True, exist_ok=True)
                out_path.write_text(out_content, encoding="utf-8")

        # Auto-wrap worker stdout in valid agy JSON if it is a worker invocation and not yet JSON
        if cmd and cmd[0] == "agy" and resp.returncode == 0 and resp.stdout:
            stdout_str = resp.stdout.strip()
            try:
                json.loads(stdout_str)
            except Exception:
                resp = SubprocessResult(
                    returncode=resp.returncode,
                    stdout=json.dumps({"status": "SUCCESS", "report": resp.stdout}),
                    stderr=resp.stderr,
                    timed_out=resp.timed_out,
                )

        return resp


class TestOrchestrator(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.mkdtemp(prefix="lightyears_orch_test_")
        self.workspace = pathlib.Path(self.temp_dir)
        self.snapshots_dir = self.workspace / ".orchestrator_snapshots"

    def tearDown(self):
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    # -----------------------------------------------------------------------
    # 1. Plan Parsing Tests
    # -----------------------------------------------------------------------
    def test_parse_plan_steps_valid(self):
        plan = """
        ## ADIM 1: Initialize subsystem
        Details here...
        ## ADIM 2: [ ] Implement ability logic
        More details...
        ## ADIM 3: [x] Unit test setup
        """
        steps = parse_plan_steps(plan)
        self.assertEqual(len(steps), 3)
        self.assertEqual(steps[0].number, 1)
        self.assertEqual(steps[0].title, "Initialize subsystem")
        self.assertFalse(steps[0].completed)
        self.assertEqual(steps[0].body, "Details here...")

        self.assertEqual(steps[1].number, 2)
        self.assertEqual(steps[1].title, "Implement ability logic")
        self.assertFalse(steps[1].completed)
        self.assertEqual(steps[1].body, "More details...")

        self.assertEqual(steps[2].number, 3)
        self.assertEqual(steps[2].title, "Unit test setup")
        self.assertTrue(steps[2].completed)

    def test_parse_plan_steps_errors(self):
        with self.assertRaises(ValueError):
            parse_plan_steps("No steps here")

        plan_dup = "## ADIM 1: First\n## ADIM 1: Duplicate"
        with self.assertRaises(ValueError):
            parse_plan_steps(plan_dup)

        plan_unordered = "## ADIM 2: Second\n## ADIM 1: First"
        with self.assertRaises(ValueError):
            parse_plan_steps(plan_unordered)

        plan_empty_title = "## ADIM 1: "
        with self.assertRaises(ValueError):
            parse_plan_steps(plan_empty_title)

    # -----------------------------------------------------------------------
    # 2. Snapshot & Diff Engine Tests
    # -----------------------------------------------------------------------
    def test_snapshot_diff_added_deleted_text_binary(self):
        engine = SnapshotEngine(self.workspace, self.snapshots_dir)

        f_existing = self.workspace / "existing.txt"
        f_existing.write_text("line 1\nline 2\n", encoding="utf-8")
        f_del = self.workspace / "to_delete.txt"
        f_del.write_text("delete me\n", encoding="utf-8")
        f_bin = self.workspace / "binary.bin"
        f_bin.write_bytes(b"\x00\x01\x02\x03")

        snap_dir = engine.create_snapshot("snap_1")

        # Mutate
        (self.workspace / "added.txt").write_text("new file", encoding="utf-8")
        f_del.unlink()
        f_existing.write_text("line 1\nline 2 modified\n", encoding="utf-8")
        f_bin.write_bytes(b"\x00\x09\x09\x09")

        diff = engine.compute_diff(snap_dir)
        self.assertTrue(diff.has_changes)
        self.assertIn("added.txt", diff.added_files)
        self.assertIn("to_delete.txt", diff.deleted_files)
        self.assertIn("existing.txt", diff.modified_text_files)
        self.assertIn("binary.bin", diff.modified_binary_files)

        # Restore
        engine.restore_snapshot(snap_dir)
        diff_after_restore = engine.compute_diff(snap_dir)
        self.assertFalse(diff_after_restore.has_changes)
        self.assertTrue(f_del.exists())
        self.assertFalse((self.workspace / "added.txt").exists())
        self.assertEqual(f_existing.read_text(encoding="utf-8"), "line 1\nline 2\n")
        self.assertEqual(f_bin.read_bytes(), b"\x00\x01\x02\x03")

    def test_excluded_files_survive_rollback(self):
        engine = SnapshotEngine(self.workspace, self.snapshots_dir)

        build_asan = self.workspace / "build-asan"
        build_asan.mkdir()
        (build_asan / "artifact.o").write_text("object file", encoding="utf-8")

        git_dir = self.workspace / ".git"
        git_dir.mkdir()
        (git_dir / "HEAD").write_text("ref: refs/heads/main", encoding="utf-8")

        log_file = self.workspace / "LightYears.log"
        log_file.write_text("runtime log", encoding="utf-8")

        f_src = self.workspace / "main.cpp"
        f_src.write_text("int main() { return 0; }", encoding="utf-8")

        snap_dir = engine.create_snapshot("snap_excludes")

        # Mutate both source and excluded file
        f_src.write_text("modified", encoding="utf-8")
        (build_asan / "new_build_output.o").write_text("new build", encoding="utf-8")
        log_file.write_text("new runtime log entry", encoding="utf-8")

        # Rollback
        engine.restore_snapshot(snap_dir)

        self.assertEqual(f_src.read_text(encoding="utf-8"), "int main() { return 0; }")
        self.assertTrue((build_asan / "new_build_output.o").exists())
        self.assertEqual(log_file.read_text(encoding="utf-8"), "new runtime log entry")
        self.assertTrue((git_dir / "HEAD").exists())

    # -----------------------------------------------------------------------
    # 3. Rejection, Rollback, Retry, and 3-Fail Exhaustion
    # -----------------------------------------------------------------------
    def test_rejection_rollback_retry_success(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        src_file = self.workspace / "logic.py"
        src_file.write_text("x = 1\n", encoding="utf-8")

        attempt_count = 0

        def fake_worker(cmd, cwd, stdin_data):
            nonlocal attempt_count
            attempt_count += 1
            src_file.write_text(f"x = {attempt_count}\n", encoding="utf-8")
            return SubprocessResult(
                returncode=0,
                stdout=json.dumps({"status": "SUCCESS", "report": f"Attempt {attempt_count}"}),
                stderr="",
            )

        def fake_reviewer(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            out_p = pathlib.Path(cmd[out_idx])
            if attempt_count == 1:
                out_p.write_text("DUZELTME: Make x equal 2 instead\n", encoding="utf-8")
            else:
                out_p.write_text("ONAYLANDI\nLooks great.", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        mock_runner.set_next_response(fake_worker)
        mock_runner.set_next_response(fake_reviewer)
        mock_runner.set_next_response(fake_worker)
        mock_runner.set_next_response(fake_reviewer)

        success = orch.execute_single_task("set x to 2")
        self.assertTrue(success)
        self.assertEqual(attempt_count, 2)
        self.assertEqual(src_file.read_text(encoding="utf-8"), "x = 2\n")

    def test_three_failed_attempts_exhausts_and_restores(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        src_file = self.workspace / "config.py"
        src_file.write_text("initial = True\n", encoding="utf-8")

        def fake_worker(cmd, cwd, stdin_data):
            src_file.write_text("mutated = True\n", encoding="utf-8")
            return SubprocessResult(
                returncode=0,
                stdout=json.dumps({"status": "SUCCESS", "report": "worker done"}),
                stderr="",
            )

        def fake_reviewer(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("DUZELTME: Not acceptable\n", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        for _ in range(3):
            mock_runner.set_next_response(fake_worker)
            mock_runner.set_next_response(fake_reviewer)

        success = orch.execute_single_task("Fail 3 times")
        self.assertFalse(success)
        self.assertEqual(src_file.read_text(encoding="utf-8"), "initial = True\n")

    # -----------------------------------------------------------------------
    # 4. Earlier Accepted Step Survives Next Failure
    # -----------------------------------------------------------------------
    def test_earlier_accepted_survives_next_failure(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        file1 = self.workspace / "step1.txt"
        file2 = self.workspace / "step2.txt"

        plan = "## ADIM 1: Step One\n## ADIM 2: Step Two"

        def worker_step1(cmd, cwd, stdin_data):
            file1.write_text("step 1 complete\n", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout=json.dumps({"status": "SUCCESS"}), stderr="")

        def reviewer_step1(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("ONAYLANDI\n", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        def worker_step2(cmd, cwd, stdin_data):
            file2.write_text("step 2 failed attempt\n", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout=json.dumps({"status": "SUCCESS"}), stderr="")

        def reviewer_step2(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("DUZELTME: Broken\n", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        mock_runner.set_next_response(worker_step1)
        mock_runner.set_next_response(reviewer_step1)
        for _ in range(3):
            mock_runner.set_next_response(worker_step2)
            mock_runner.set_next_response(reviewer_step2)

        plan_ok = orch.run_plan(plan)
        self.assertFalse(plan_ok)
        self.assertTrue(file1.exists())
        self.assertEqual(file1.read_text(encoding="utf-8"), "step 1 complete\n")
        self.assertFalse(file2.exists())

    # -----------------------------------------------------------------------
    # 5. Dry Run Checks
    # -----------------------------------------------------------------------
    def test_dry_run_no_writes(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, dry_run=True, runner=mock_runner)

        test_file = self.workspace / "untouched.txt"
        test_file.write_text("must not change", encoding="utf-8")

        ok, msg = orch.startup_checks()
        self.assertTrue(ok)

        success = orch.execute_single_task("Make arbitrary changes")
        self.assertTrue(success)
        self.assertEqual(test_file.read_text(encoding="utf-8"), "must not change")

    # -----------------------------------------------------------------------
    # 6. Model Validation Checks
    # -----------------------------------------------------------------------
    def test_invalid_worker_models(self):
        mock_runner = MockProcessRunner()
        orch_bad = Orchestrator(self.workspace, worker_model="claude-3-5-sonnet", runner=mock_runner)
        ok, msg = orch_bad.startup_checks()
        self.assertFalse(ok)
        self.assertIn("not in allowed set", msg)

        orch_missing = Orchestrator(self.workspace, worker_model="gemini-3.8-flash-high", runner=mock_runner)
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout="help", stderr=""))
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout="help", stderr=""))
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout="gemini-3.8-flash-medium only", stderr=""))
        ok2, msg2 = orch_missing.startup_checks()
        self.assertFalse(ok2)
        self.assertIn("not found in 'agy models' output", msg2)

    # -----------------------------------------------------------------------
    # 7. Malformed Review Handling & Protected Files
    # -----------------------------------------------------------------------
    def test_malformed_review_output(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        diff = DiffSummary(added_files=["a.txt"])
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout="I think it looks good!", stderr=""))
        approved, note = orch.invoke_reviewer("task", diff, "worker done")
        self.assertFalse(approved)
        self.assertIn("Malformed", note)

    def test_protected_orchestrator_md_alteration_rejected(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        diff = DiffSummary(modified_text_files={"ORCHESTRATOR.md": "diff"})
        approved, note = orch.invoke_reviewer("task", diff, "worker modified doc")
        self.assertFalse(approved)
        self.assertIn("Protected file 'ORCHESTRATOR.md' was altered", note)

    # -----------------------------------------------------------------------
    # 8. Worker Failure & Timeout
    # -----------------------------------------------------------------------
    def test_worker_failure_and_timeout(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        mock_runner.set_next_response(SubprocessResult(returncode=1, stdout="", stderr="Crash"))
        res = orch.invoke_worker("broken task")
        self.assertEqual(res.returncode, 1)

        mock_runner.set_next_response(SubprocessResult(returncode=-1, stdout="", stderr="Timeout", timed_out=True))
        res_timeout = orch.invoke_worker("slow task")
        self.assertTrue(res_timeout.timed_out)

    # -----------------------------------------------------------------------
    # 9. Inspect Mutation Detection & Rollback
    # -----------------------------------------------------------------------
    def test_inspect_mutation_detected_and_restored(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        target = self.workspace / "file.txt"
        target.write_text("initial", encoding="utf-8")

        def rogue_inspector(cmd, cwd, stdin_data):
            target.write_text("inspector wrote something forbidden", encoding="utf-8")
            if "-o" in cmd:
                out_idx = cmd.index("-o") + 1
                pathlib.Path(cmd[out_idx]).write_text("inspected", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="inspected", stderr="")

        mock_runner.set_next_response(rogue_inspector)
        ok, msg = orch.run_inspect("what is in file.txt?")
        self.assertFalse(ok)
        self.assertIn("mutations detected and reverted", msg)
        self.assertEqual(target.read_text(encoding="utf-8"), "initial")

    # -----------------------------------------------------------------------
    # 10. Secrets Redaction & History
    # -----------------------------------------------------------------------
    def test_secret_redaction(self):
        text = "My API key is AIzaSyD9876543210123456789012345678901 and token is Bearer eyJhbGciOiJIUzI1NiJ9"
        redacted = redact_secrets(text)
        self.assertNotIn("AIzaSy", redacted)
        self.assertNotIn("eyJhbGci", redacted)
        self.assertIn("[REDACTED_SECRET]", redacted)

    def test_history_logging(self):
        hm = HistoryManager(self.snapshots_dir)
        hm.record_success("Task 1", "ADIM 1")
        hm.record_failure("Task 2", "Failed after 3 attempts")

        loaded = hm.load()
        self.assertEqual(len(loaded), 2)
        self.assertEqual(loaded[0]["status"], "ACCEPTED")
        self.assertEqual(loaded[1]["status"], "REJECTED")

    # -----------------------------------------------------------------------
    # 11. Final Validation Failure
    # -----------------------------------------------------------------------
    def test_final_validation_failure_and_rollback(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        src_file = self.workspace / "main.cpp"
        src_file.write_text("int main() { return 0; }\n", encoding="utf-8")

        def rogue_validator(cmd, cwd, stdin_data):
            src_file.write_text("int main() { return 1; }\n", encoding="utf-8")
            if "-o" in cmd:
                out_idx = cmd.index("-o") + 1
                pathlib.Path(cmd[out_idx]).write_text("ONAYLANDI\nbuild passed", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="build passed", stderr="")

        mock_runner.set_next_response(rogue_validator)
        ok, msg = orch.run_final_validation("## ADIM 1: Completed step")
        self.assertFalse(ok)
        self.assertIn("boundary", msg)
        self.assertEqual(src_file.read_text(encoding="utf-8"), "int main() { return 0; }\n")

    # -----------------------------------------------------------------------
    # 12. Workspace Lock
    # -----------------------------------------------------------------------
    def test_workspace_lock_acquisition(self):
        lock_path = self.workspace / ".orchestrator.lock"
        lock1 = WorkspaceLock(lock_path)
        lock2 = WorkspaceLock(lock_path)

        self.assertTrue(lock1.acquire())
        self.assertTrue(lock1.acquired)
        self.assertFalse(lock2.acquire())

        lock1.release()
        self.assertFalse(lock1.acquired)
        self.assertTrue(lock2.acquire())
        lock2.release()

    # -----------------------------------------------------------------------
    # 13. Regression: Main Dry Run Exact Tree Unchanged
    # -----------------------------------------------------------------------
    def test_main_dry_run_temp_tree_exact_unchanged(self):
        test_file = self.workspace / "sample.cpp"
        test_file.write_text("void test() {}\n", encoding="utf-8")

        # Snapshot exact file tree and timestamps before
        initial_tree = {}
        for r, dirs, files in os.walk(self.workspace):
            for f in files:
                p = pathlib.Path(r) / f
                initial_tree[p.relative_to(self.workspace).as_posix()] = p.stat().st_mtime

        f = io.StringIO()
        with redirect_stdout(f):
            code = main(["--dry-run", "--workspace", str(self.workspace)])
        self.assertEqual(code, 0)

        current_tree = {}
        for r, dirs, files in os.walk(self.workspace):
            for f in files:
                p = pathlib.Path(r) / f
                current_tree[p.relative_to(self.workspace).as_posix()] = p.stat().st_mtime

        self.assertEqual(initial_tree, current_tree)
        self.assertFalse((self.workspace / ".orchestrator_snapshots").exists())
        self.assertFalse((self.workspace / ".orchestrator.lock").exists())
        self.assertFalse((self.workspace / "orchestrator_log.md").exists())

    # -----------------------------------------------------------------------
    # 14. Regression: Missing Model Substring Invalid (Exact Token Required)
    # -----------------------------------------------------------------------
    def test_missing_model_substring_invalid(self):
        mock_runner = MockProcessRunner()
        # Mock agy models output containing a substring token
        mock_runner.default_result = SubprocessResult(
            returncode=0,
            stdout="Models:\n  gemini-3.8-flash-medium-preview\n  other-model\n",
            stderr="",
        )
        orch = Orchestrator(self.workspace, worker_model="gemini-3.8-flash-medium", runner=mock_runner)
        ok, msg = orch.startup_checks()
        self.assertFalse(ok)
        self.assertIn("not found in 'agy models' output", msg)

    # -----------------------------------------------------------------------
    # 15. Regression: Fake Successful agy Denied Action & Bad Status
    # -----------------------------------------------------------------------
    def test_fake_successful_agy_denied_action(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        # Worker returncode 0 but denied_actions list present
        denied_json = json.dumps({"status": "SUCCESS", "denied_actions": ["edit /root/forbidden.txt"]})
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout=denied_json, stderr=""))

        res = orch.invoke_worker("test task")
        self.assertNotEqual(res.returncode, 0)
        self.assertIn("denied actions", res.stderr)

    def test_fake_successful_agy_non_success_or_invalid_json(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        # Non-SUCCESS status
        bad_status_json = json.dumps({"status": "FAILED", "error": "Model crashed"})
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout=bad_status_json, stderr=""))
        res = orch.invoke_worker("test task")
        self.assertNotEqual(res.returncode, 0)

        # Invalid JSON
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout="not-json", stderr=""))
        res2 = orch.invoke_worker("test task")
        self.assertNotEqual(res2.returncode, 0)

    # -----------------------------------------------------------------------
    # 16. Regression: Actual After Snapshot Diff Added/Deleted Content
    # -----------------------------------------------------------------------
    def test_actual_after_snapshot_diff_added_deleted_content(self):
        engine = SnapshotEngine(self.workspace, self.snapshots_dir)

        old_file = self.workspace / "old.txt"
        old_file.write_text("line A\nline B\n", encoding="utf-8")
        bin_file = self.workspace / "data.bin"
        bin_file.write_bytes(b"\x00\x01\x02")

        before_dir = engine.create_snapshot("before_snap")

        old_file.unlink()
        new_file = self.workspace / "new.txt"
        new_file.write_text("added 1\nadded 2\n", encoding="utf-8")
        bin_file.write_bytes(b"\x00\x99\x99")

        after_dir = engine.create_snapshot("after_snap")

        diff = engine.compute_diff(before_dir, after_dir)
        diff_text = diff.format_diff_text()

        self.assertIn("Binary file changed: data.bin", diff_text)
        self.assertIn("--- /dev/null", diff_text)
        self.assertIn("+++ b/new.txt", diff_text)
        self.assertIn("--- a/old.txt", diff_text)
        self.assertIn("+++ /dev/null", diff_text)

    # -----------------------------------------------------------------------
    # 17. Regression: Manifest Filename Source Preservation
    # -----------------------------------------------------------------------
    def test_manifest_filename_source_preservation(self):
        engine = SnapshotEngine(self.workspace, self.snapshots_dir)

        # User project files named manifest.json and __manifest__.json
        user_m1 = self.workspace / "manifest.json"
        user_m1.write_text('{"project_meta": "true"}', encoding="utf-8")
        user_m2 = self.workspace / "__manifest__.json"
        user_m2.write_text('{"build_system": "ok"}', encoding="utf-8")

        snap_dir = engine.create_snapshot("manifest_test")

        # Mutate
        user_m1.write_text('{"project_meta": "altered"}', encoding="utf-8")
        user_m2.unlink()

        # Restore
        engine.restore_snapshot(snap_dir)
        self.assertEqual(user_m1.read_text(encoding="utf-8"), '{"project_meta": "true"}')
        self.assertEqual(user_m2.read_text(encoding="utf-8"), '{"build_system": "ok"}')

    # -----------------------------------------------------------------------
    # 18. Regression: File/Dir Type Change and Restore
    # -----------------------------------------------------------------------
    def test_file_dir_type_change_and_restore(self):
        engine = SnapshotEngine(self.workspace, self.snapshots_dir)

        f_file = self.workspace / "item_file"
        f_file.write_text("content", encoding="utf-8")

        d_dir = self.workspace / "item_dir"
        d_dir.mkdir()
        (d_dir / "child.txt").write_text("child", encoding="utf-8")

        snap_dir = engine.create_snapshot("type_test")

        # Swap file <-> dir
        f_file.unlink()
        f_file.mkdir()
        (f_file / "nested.txt").write_text("now a dir", encoding="utf-8")

        shutil.rmtree(d_dir)
        d_dir.write_text("now a file", encoding="utf-8")

        engine.restore_snapshot(snap_dir)

        self.assertTrue(f_file.is_file())
        self.assertEqual(f_file.read_text(encoding="utf-8"), "content")
        self.assertTrue(d_dir.is_dir())
        self.assertTrue((d_dir / "child.txt").is_file())

    # -----------------------------------------------------------------------
    # 19. Regression: Restore Preserves Excluded Descendants
    # -----------------------------------------------------------------------
    def test_restore_preserves_excluded_descendants(self):
        engine = SnapshotEngine(self.workspace, self.snapshots_dir)

        snap_dir = engine.create_snapshot("preserve_test")

        # Worker created a directory structure that happens to have excluded descendants
        new_dir = self.workspace / "new_module"
        new_dir.mkdir()
        (new_dir / "code.cpp").write_text("code", encoding="utf-8")
        ex_dir = new_dir / "build"
        ex_dir.mkdir()
        (ex_dir / "compiled.o").write_text("object file", encoding="utf-8")

        engine.restore_snapshot(snap_dir)

        # code.cpp is removed, but build and its contents survive
        self.assertFalse((new_dir / "code.cpp").exists())
        self.assertTrue((ex_dir / "compiled.o").exists())

    # -----------------------------------------------------------------------
    # 20. Regression: Symlink Fail Closed
    # -----------------------------------------------------------------------
    def test_symlink_fail_closed(self):
        engine = SnapshotEngine(self.workspace, self.snapshots_dir)
        target = self.workspace / "real_file.txt"
        target.write_text("target", encoding="utf-8")
        link = self.workspace / "link_file.txt"

        try:
            os.symlink(str(target), str(link))
        except (OSError, NotImplementedError):
            self.skipTest("Symlinks not supported in this test environment.")

        with self.assertRaises(RuntimeError) as ctx:
            engine.scan_workspace()
        self.assertIn("symlink detected", str(ctx.exception))

    # -----------------------------------------------------------------------
    # 21. Regression: Exception & KeyboardInterrupt Restores Workspace
    # -----------------------------------------------------------------------
    def test_exception_and_keyboard_interrupt_restores(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        target = self.workspace / "file_safe.txt"
        target.write_text("safe content", encoding="utf-8")

        def explosive_worker(cmd, cwd, stdin_data):
            target.write_text("tainted content", encoding="utf-8")
            raise KeyboardInterrupt("Simulated Ctrl+C")

        mock_runner.set_next_response(explosive_worker)

        with self.assertRaises(KeyboardInterrupt):
            orch.execute_single_task("set file_safe.txt to 10")

        self.assertEqual(target.read_text(encoding="utf-8"), "safe content")

    # -----------------------------------------------------------------------
    # 22. Regression: Readonly Nonzero ONAYLANDI Cannot Approve
    # -----------------------------------------------------------------------
    def test_readonly_nonzero_onaylandi_cannot_approve(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        diff = DiffSummary(added_files=["foo.txt"])

        def failing_reviewer(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("ONAYLANDI\nApproved but crashed", encoding="utf-8")
            return SubprocessResult(returncode=1, stdout="", stderr="Internal crash")

        mock_runner.set_next_response(failing_reviewer)
        approved, note = orch.invoke_reviewer("task", diff, "worker stdout")
        self.assertFalse(approved)
        self.assertIn("failed with exit code 1", note)

    # -----------------------------------------------------------------------
    # 23. Regression: Plan Body Reaching Worker & Plan File Completion
    # -----------------------------------------------------------------------
    def test_plan_body_reaching_worker_and_plan_file_completion(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        plan_file = self.workspace / "PLAN.md"
        plan_file.write_text("## ADIM 1: Feature Setup\nDetailed step specifications line 1\n", encoding="utf-8")

        received_prompts = []

        def recording_worker(cmd, cwd, stdin_data):
            prompt_arg = [a for a in cmd if "task_input" in a][0]
            # Extract task input file path
            for part in prompt_arg.split():
                if "task_input" in part:
                    received_prompts.append(pathlib.Path(part).read_text(encoding="utf-8"))
            return SubprocessResult(returncode=0, stdout=json.dumps({"status": "SUCCESS"}), stderr="")

        def approving_reviewer(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("ONAYLANDI\n", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        def validator(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("ONAYLANDI\nTests passed", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        mock_runner.set_next_response(recording_worker)
        mock_runner.set_next_response(approving_reviewer)
        mock_runner.set_next_response(validator)

        success = orch.run_plan(str(plan_file))
        self.assertTrue(success)
        self.assertTrue(len(received_prompts) > 0)
        self.assertIn("Detailed step specifications line 1", received_prompts[0])
        self.assertIn("## ADIM 1: [x] Feature Setup", plan_file.read_text(encoding="utf-8"))

    # -----------------------------------------------------------------------
    # 24. Regression: Final Validation Deletions, Binary, Failed Rejected
    # -----------------------------------------------------------------------
    def test_final_validation_deletions_binary_failed_rejected(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        f = self.workspace / "base.txt"
        f.write_text("keep me", encoding="utf-8")

        # Case 1: Validator deletes a file
        def deleting_val(cmd, cwd, stdin_data):
            f.unlink()
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("ONAYLANDI\n", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        mock_runner.set_next_response(deleting_val)
        ok1, msg1 = orch.run_final_validation("## ADIM 1: Step")
        self.assertFalse(ok1)
        self.assertTrue(f.exists())

        # Case 2: Validator returns DUZELTME
        def duzeltme_val(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("DUZELTME: Tests failed\n", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        mock_runner.set_next_response(duzeltme_val)
        ok2, msg2 = orch.run_final_validation("## ADIM 1: Step")
        self.assertFalse(ok2)

    # -----------------------------------------------------------------------
    # 25. Regression: All Exclusions Rules
    # -----------------------------------------------------------------------
    def test_all_exclusions_rules(self):
        engine = SnapshotEngine(self.workspace, self.snapshots_dir)

        for d in EXCLUDED_DIR_NAMES:
            self.assertTrue(engine.is_excluded(pathlib.Path(d)))
            self.assertTrue(engine.is_excluded(pathlib.Path("sub") / d))

        for f in EXCLUDED_FILE_NAMES:
            self.assertTrue(engine.is_excluded(pathlib.Path(f)))
            self.assertTrue(engine.is_excluded(pathlib.Path("sub") / f))

        for rd in ROOT_EXCLUDED_DIR_NAMES:
            self.assertTrue(engine.is_excluded(pathlib.Path(rd)))
            self.assertFalse(engine.is_excluded(pathlib.Path("sub") / rd))

        for rf in ROOT_EXCLUDED_FILE_NAMES:
            self.assertTrue(engine.is_excluded(pathlib.Path(rf)))
            self.assertFalse(engine.is_excluded(pathlib.Path("sub") / rf))

    # -----------------------------------------------------------------------
    # 26. Regression: Logging Required and Secrets Redaction
    # -----------------------------------------------------------------------
    def test_logging_required_and_secrets_redaction(self):
        os.environ["TEST_SECRET_ENV"] = "SUPER_SECRET_VALUE_12345"
        try:
            log_file = self.workspace / "orchestrator_log.md"
            log_event(
                log_file,
                "test_event",
                {
                    "api_key": "AIzaSyD9876543210123456789012345678901",
                    "token": "SUPER_SECRET_VALUE_12345",
                    "text": "Call with token=SUPER_SECRET_VALUE_12345 and AIzaSyD9876543210123456789012345678901",
                },
            )

            content = log_file.read_text(encoding="utf-8")
            self.assertNotIn("AIzaSy", content)
            self.assertNotIn("SUPER_SECRET_VALUE_12345", content)
            self.assertIn("[REDACTED_SECRET]", content)
        finally:
            del os.environ["TEST_SECRET_ENV"]

    # -----------------------------------------------------------------------
    # 27. Regression: Turkish Inspection Routing
    # -----------------------------------------------------------------------
    def test_turkish_inspection_routing(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        query = "incele ama kod değiştirme"
        self.assertTrue(is_inspect_query(query))

        target = self.workspace / "main.cpp"
        target.write_text("int x = 0;\n", encoding="utf-8")

        def inspect_responder(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("Code analyzed cleanly.", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        mock_runner.set_next_response(inspect_responder)

        f = io.StringIO()
        with redirect_stdout(f):
            ok = orch.execute_single_task(query)
        self.assertTrue(ok)
        self.assertEqual(target.read_text(encoding="utf-8"), "int x = 0;\n")

    # -----------------------------------------------------------------------
    # 28. Regression: Three Rejected Attempts Prints Manuel Müdahale
    # -----------------------------------------------------------------------
    def test_three_rejected_attempts_manuel_mudahele_output(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        def worker_fn(cmd, cwd, stdin_data):
            return SubprocessResult(returncode=0, stdout=json.dumps({"status": "SUCCESS", "report": "Done"}), stderr="")

        def reviewer_fn(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("DUZELTME: Still failing\n", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        for _ in range(3):
            mock_runner.set_next_response(worker_fn)
            mock_runner.set_next_response(reviewer_fn)

        f = io.StringIO()
        with redirect_stdout(f):
            ok = orch.execute_single_task("set param to 5")
        output = f.getvalue()

        self.assertFalse(ok)
        self.assertIn("MANUEL MÜDAHALE GEREKİYOR", output)
        self.assertIn("Still failing", output)

    # -----------------------------------------------------------------------
    # 29. Regression: Protected Child Survives Type Collision During Rollback
    # -----------------------------------------------------------------------
    def test_protected_child_survives_type_collision_during_rollback(self):
        engine = SnapshotEngine(self.workspace, self.snapshots_dir)
        sub_dir = self.workspace / "sub"
        sub_dir.mkdir(parents=True, exist_ok=True)
        tracked_file = sub_dir / "tracked.txt"
        tracked_file.write_text("initial content\n", encoding="utf-8")

        # Excluded file inside sub
        excluded_file = sub_dir / ".orchestrator.lock"
        excluded_file.write_text("lock\n", encoding="utf-8")

        snap_dir = engine.create_snapshot("collision_snap")

        # Mutate: delete sub and replace with a plain file
        tracked_file.unlink()
        excluded_file.unlink()
        sub_dir.rmdir()
        sub_dir.write_text("rogue file replacing dir\n", encoding="utf-8")

        # Restore
        engine.restore_snapshot(snap_dir)

        self.assertTrue(sub_dir.is_dir())
        self.assertTrue(tracked_file.is_file())
        self.assertEqual(tracked_file.read_text(encoding="utf-8"), "initial content\n")

    # -----------------------------------------------------------------------
    # 30. Regression: Lock Exclusivity and Stale Detection
    # -----------------------------------------------------------------------
    def test_lock_exclusivity_and_stale_detection(self):
        lock_path = self.snapshots_dir / ".orchestrator.lock"
        lock1 = WorkspaceLock(lock_path)
        lock2 = WorkspaceLock(lock_path)

        self.assertTrue(lock1.acquire())
        self.assertTrue(lock1.acquired)

        # Second acquisition must fail without unlinking lock
        self.assertFalse(lock2.acquire())
        self.assertFalse(lock2.acquired)
        self.assertIn("Another orchestrator process holds the lock", lock2.recovery_message)
        self.assertIn(str(lock_path), lock2.recovery_message)
        self.assertTrue(lock_path.exists())

        lock1.release()
        self.assertFalse(lock1.acquired)
        self.assertTrue(lock2.acquire())
        lock2.release()

    # -----------------------------------------------------------------------
    # 31. Regression: Scoping Planner Rollback on Error or Interrupt
    # -----------------------------------------------------------------------
    def test_scoping_planner_rollback_on_error_or_interrupt(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        target = self.workspace / "original.txt"
        target.write_text("safe state\n", encoding="utf-8")

        # Scoping planner fails with non-zero exit code
        def failing_scope(cmd, cwd, stdin_data):
            return SubprocessResult(returncode=1, stdout="", stderr="Scoping error")

        mock_runner.set_next_response(failing_scope)

        ok = orch.execute_single_task("complex task requiring scoping")
        self.assertFalse(ok)
        self.assertEqual(target.read_text(encoding="utf-8"), "safe state\n")
        # Ensure worker was never invoked
        worker_calls = [c for c in mock_runner.commands_run if c[0] and c[0][0] == "agy"]
        self.assertEqual(len(worker_calls), 0)

    # -----------------------------------------------------------------------
    # 32. Regression: Worker Prompt Guardrails and Response Validation
    # -----------------------------------------------------------------------
    def test_worker_prompt_guardrails_and_response_validation(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        # 1. Verify guardrails in prompt
        def worker_check_prompt(cmd, cwd, stdin_data):
            prompt_arg = cmd[2]
            self.assertIn("task file", prompt_arg)
            return SubprocessResult(returncode=0, stdout=json.dumps({"status": "SUCCESS", "report": "Valid"}), stderr="")

        mock_runner.set_next_response(worker_check_prompt)
        res = orch.invoke_worker("Build feature X")
        self.assertEqual(res.returncode, 0)

        # 2. Verify invalid response formats rejected
        # Non-dict response
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout="[1, 2, 3]", stderr=""))
        res_list = orch.invoke_worker("Task")
        self.assertEqual(res_list.returncode, 1)

        # Non-SUCCESS status
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout=json.dumps({"status": "FAILED", "report": "no"}), stderr=""))
        res_failed = orch.invoke_worker("Task")
        self.assertEqual(res_failed.returncode, 1)

        # Missing report
        mock_runner.set_next_response(SubprocessResult(returncode=0, stdout=json.dumps({"status": "SUCCESS"}), stderr=""))
        res_noreport = orch.invoke_worker("Task")
        self.assertEqual(res_noreport.returncode, 1)

    # -----------------------------------------------------------------------
    # 33. Regression: Reviewer Invoked on Protected File With Forced Rejection
    # -----------------------------------------------------------------------
    def test_reviewer_invoked_on_protected_file_with_forced_rejection(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        diff = DiffSummary(modified_text_files={"ORCHESTRATOR.md": "diff"})

        # Reviewer outputs ONAYLANDI
        def reviewer_fn(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("ONAYLANDI\nApproving regardless", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        mock_runner.set_next_response(reviewer_fn)
        approved, note = orch.invoke_reviewer("Task", diff, "worker done")

        # Reviewer MUST have been invoked
        self.assertTrue(any(c[0] and c[0][0] == "codex" for c in mock_runner.commands_run))
        # But approved MUST be forced to False
        self.assertFalse(approved)
        self.assertIn("Protected file 'ORCHESTRATOR.md' was altered", note)

    # -----------------------------------------------------------------------
    # 34. Regression: Full Plan Context Propagation
    # -----------------------------------------------------------------------
    def test_full_plan_context_propagation(self):
        mock_runner = MockProcessRunner()
        orch = Orchestrator(self.workspace, runner=mock_runner)

        plan_content = (
            "## ADIM 1: Step One\n"
            "Body of step one\n\n"
            "## ADIM 2: Step Two\n"
            "Body of step two\n"
        )
        plan_file = self.workspace / "test_plan.md"
        plan_file.write_text(plan_content, encoding="utf-8")

        observed_contexts = []

        def worker_fn(cmd, cwd, stdin_data):
            return SubprocessResult(returncode=0, stdout=json.dumps({"status": "SUCCESS", "report": "Done"}), stderr="")

        def reviewer_fn(cmd, cwd, stdin_data):
            if stdin_data:
                observed_contexts.append(stdin_data)
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("ONAYLANDI\nValid.", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        # Step 1
        mock_runner.set_next_response(worker_fn)
        mock_runner.set_next_response(reviewer_fn)
        # Step 2
        mock_runner.set_next_response(worker_fn)
        mock_runner.set_next_response(reviewer_fn)
        # Final validation
        def validator_fn(cmd, cwd, stdin_data):
            out_idx = cmd.index("-o") + 1
            pathlib.Path(cmd[out_idx]).write_text("ONAYLANDI\nTests executed: 2/2 passed.", encoding="utf-8")
            return SubprocessResult(returncode=0, stdout="", stderr="")

        mock_runner.set_next_response(validator_fn)

        ok = orch.run_plan(str(plan_file))
        self.assertTrue(ok)
        self.assertTrue(len(observed_contexts) >= 2)
        # Verify full plan context was passed
        for ctx in observed_contexts:
            self.assertIn("FULL PLAN CONTENT:", ctx)
            self.assertIn("Body of step one", ctx)
            self.assertIn("Body of step two", ctx)


if __name__ == "__main__":
    unittest.main()

