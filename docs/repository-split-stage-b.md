# Engine/game repository split: Stage B

Working checkout: `D:/LightYears-game-next`, branch `split-work`, original commit
`d5596aa6844eed3bae6c4804598fb5a1807db7c1`. Engine dependency: Stage A `7eef946`
in the read-only sibling `D:/Vega-engine-next`. No commits, remote operations or
network installs were performed. Only `git rm` was used; no other git operations ran.

Removed 540 tracked engine-owned files from the active game tree:

| Area | Files |
| --- | ---: |
| VegaEngine | 146 |
| VegaEditor | 153 |
| VegaScript | 18 |
| Modules | 92 |
| SpaceAbilitySystem | 68 |
| Templates | 37 |
| Engine documentation | 22 |
| Engine tools | 4 |

`split-staging/removed-from-game.txt` is the exact removal manifest. This ignored
local directory also preserves the supplied Stage A inventories/original CMake
files and the pre-edit game CMake/tool inputs for Claude to copy out.

The game runtime/content, LightYearsVisual, 178 game/shared documents (including
designs, archive and vault), galaxy tools, content-field generator, portable
check and release-config template remain. `docs/engine.md` points to the sibling
engine and its `docs/new-game.md`. README build instructions now describe this
standalone consumer. No game classes, namespace `sas`, runtime targets or CTest
names were renamed.

The game editor entry moved to `LightYearsGame/app/LightYearsEditorMain.cpp`;
the 11 original integration harnesses/scripts moved to
`LightYearsGame/editor-tests`. The original Sky capture/preservation checker is
compiled there against public editor targets, with its assertions unchanged.
The game reflection test and its byte-identical golden moved into
`LightYearsGame/tests`. The compatibility shader hash comparison also lives in
game tests; its original CTest name is retained. Engine import guards remain
engine-owned and are not registered by the game.

The supplied clone/staging did not contain `RunBootstrapFailure.cmake`, although
the original game CMake referenced it. A replacement now corrupts only a
build-tree copy of abilities.json, requires exit 1 and the bootstrap rejection
diagnostic, and rejects scene startup. Its original assertions were unavailable.
The ignore rules now allow checked-in game CMake fixtures and JSON goldens.

CMake uses `project(LightYears)`, top-level CTest, explicit GAS/Sky/editor options,
and `VEGA_ENGINE_DIR` (default `../Vega`). It adds the engine at `build/engine`,
then LightYearsVisual and LightYearsGame. Game libraries/tests link public
VegaEngine/VegaGAS/VegaScript/VegaSky targets instead of monorepo include paths
or engine directory-local target variables. LightYearsEditorModule links
VegaEditorCore/VegaSkyEditor/VegaGASEditor, and `vega_add_game_editor` receives
explicit game SOURCE_ROOT/BUILD_ROOT. The existing project JSON paths already
resolve to this game's runtime/build/export layout and did not need edits.
Warnings are applied with the exported directory helper. `vega_mute_tests` runs
after all registrations; the engine's exported audio-environment script checks
the combined CTest listing. Embedded engine tests and templates stay OFF.

Exact configure command, from a VS 2022 x64 developer environment with VSLANG=1033:

```bat
cmake -S D:/LightYears-game-next -B D:/LightYears-game-next/build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DVEGA_ENGINE_DIR=D:/Vega-engine-next -DFETCHCONTENT_FULLY_DISCONNECTED=ON
cmake --build D:/LightYears-game-next/build --parallel 4
```

Only dependency `*-src` directories were copied from the engine build's `_deps`.
Generated dependency builds and all engine binaries are built inside this one
game build tree. `tools/vega-build.cmd` forwards VEGA_ENGINE_DIR and
VEGA_CMAKE_ARGS, supports the existing --release option and build arguments, and
can seed sources via VEGA_DEPS_SEED. No Release build tree was made here.

The TSV marks **95** tests "moved to game", **including** the 11 editor
integrations (84 + 11), rather than 95 + 11 distinct tests. All 95 are registered,
plus the two unchanged original visual tests, shader compatibility check and
audio-environment check: **99 total**. No embedded engine tests are registered.
The baseline for these 95 is 71 Passed, 23 Failed (already including the
sandbox-only AbilityBalance-second-E2E failure), and RareZoneRulesE2E Timeout.
This follows the supplied TSV and engine-context-validation.md exactly.

Validation completed successfully for the split contract. Configure and the
full default build returned 0. The wrapper was then exercised with
VEGA_ENGINE_DIR=D:/Vega-engine-next and targets LightYearsGame, LightYearsVisual
and LightYearsEditor; it returned 0 and performed only asset synchronization.
The full suite ran after compilation completed:

```bat
ctest --test-dir D:/LightYears-game-next/build -j 8 --timeout 300 --output-on-failure --output-junit D:/LightYears-game-next/build/split-b-tests.xml
```

| Scope | Passed | Failed | Timeout | Status differences |
| --- | ---: | ---: | ---: | ---: |
| 95 TSV game-owned tests: baseline | 71 | 23 | 1 | — |
| 95 TSV game-owned tests: Stage B | 71 | 23 | 1 | 0 |
| Four additional compatibility/visual/audio checks | 4 | 0 | 0 | 0 |
| Entire configured game tree | 75 | 23 | 1 | 0 |

| Test whose status differs | Baseline | Stage B | Cause |
| --- | --- | --- | --- |
| None | All 95 statuses match | All 95 statuses match | No split regression |

CTest returned 8, as expected for the unchanged known failures/timeout, in
124.52 seconds. RareZoneRulesE2E timed out at 120.04 seconds. The failed-test
list exactly matches the TSV, including the sandbox GetCursorPos failure in
AbilityBalance-second-E2E; no tests were skipped, disabled or marked expected-fail.
All **11 editor integrations passed**, including Export. Export launched the
exported LightYears executable, required its smoke test to pass, and verified
the authored startup scene loaded with one entity. The recreated bootstrap
failure fixture and unchanged game reflection golden also passed.

The built game additionally launched in the existing capture mode with its
staged game assets, exited 0 after 120 frames, and wrote a 1,513,487-byte capture.
This checks startup using the same unattended mode as export/portable validation;
it does not establish Release portability. The Release-only portability script
was retained but not run because there is no Release runtime in this build.

Post-validation SHA-256 checks found zero changed or added engine source files
(535 checked), zero changes to 1,557 original game source/content/test/visual
files, and zero changes to all 178 original game/shared documents. The golden
remains byte-identical with SHA-256
`85EFAECE9519414889C8ACA8BD17672A5E33E1269775109A41A308EB6D738C53`.
The boundary scan checked 1,168 C++/header files and found no old monorepo include
imports or game CMake include roots into the removed engine directories.

Evidence under `build/`:

- `split-b-configure.log`, `split-b-build.log`, `split-b-tests.log`, `split-b-tests.xml`
- `split-b-test-inventory.json`, `split-b-test-comparison.tsv`, `split-b-test-summary.json`
- `split-b-headless-smoke.log`, `LightYearsGame/split-b-headless-smoke.png`
- `split-b-wrapper-check.log`, `split-b-boundary-scan.txt`
- `split-b-source-verification.json`, `split-b-game-preservation.tsv`, `split-b-engine-source-before.tsv`

`build/compare-split-b.ps1` recreates the per-test comparison from the supplied
TSV and saved JUnit results. The removal manifest is in ignored split-staging
and must be copied out by Claude before cleaning this checkout.

Engine requests: **none**. The exported contract satisfied this game integration. The source data used by the Lua example
fixtures, engine audio-environment check and Sky shader comparison are read via
VEGA_ENGINE_DIR; game code uses target-propagated engine includes.

Caveats: Release/portable-check validation requires a Release runtime and is not
part of this one-Debug-tree validation. The Codex account usage limit is not
visible in this session. Graph discovery/index tools were denied by the session's
never-approve policy, so file-based discovery was used.

The real `.git/index.lock` is read-only. `git rm` succeeded with
`GIT_INDEX_FILE=D:/LightYears-game-next/build/split-b.index`. This copied index
records removals only; Claude must stage the final deletions/edits/new files in
the real index before committing. The real index was not changed.
