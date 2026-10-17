# Vega Engine rename validation

Date: 2026-10-17. Worktree: `D:\Vega`; branch supplied as `vega-rename`.

## Changes

- Migrated the engine, tests, editor, scripting, modules, game integration,
  visual library and Blank template to namespace `vega`, retaining subnamespaces.
- Migrated diagnostic macros and build options to the `VEGA_` prefix, including
  `VEGA_WITH_PROFILING`, `VEGA_OPTIMIZE_RENDER_IN_DEBUG` and `VEGA_DEPS_SEED`.
  Existing reflection macros did not collide with the renamed macros.
- Renamed the engine directory (146 files) and library target to `VegaEngine`.
  The public `framework/...` include layout is unchanged. The root CMake project
  is `Vega`; its engine target variable is `VEGA_ENGINE_TARGET_NAME`.
- Renamed the context/lifetime test executable targets to `VegaEngineContextTests`
  and `VegaEngineLifetimeTests`. Every CTest name is unchanged.
- Updated CMake helpers, templates, scripts, comments, README and documentation,
  including archived namespace/path examples and the context-accessor guard.

No editor-side symbol collision required a rename. Actor, World and EngineContext
forward declarations now refer to the engine types in the shared namespace.
No stored engine-qualified type name required an alias. Lua API names and data
files were preserved. LightYearsGame, LightYearsVisual, SpaceAbilitySystem, GAS
and namespace `sas` retain their names.

## Stages and evidence

| Stage | Result | Evidence under `build/` |
| --- | --- | --- |
| Engine namespace | Engine library build passed | `wave9-rename-stage-a-build.log` |
| Engine macros/options | Engine library build passed | `wave9-rename-stage-a-macros-build.log` |
| Engine tests | Executables built; 28/28 tests passed | `wave9-rename-stage-b-build.log`, `wave9-rename-stage-b-tests.log` |
| Editor/script/modules/game/visual/template integration | Full build passed | `wave9-rename-stage-cd-build.log` |
| Directory/target rename and final paths | Reconfigured; full Debug build passed (exit 0) | `wave9-rename-configure.log`, `wave9-rename-build.log` |
| Full baseline comparison | 184 tests; exact baseline match | `wave9-rename-tests.log`, `wave9-rename-tests.xml` |

The final configured suite has 184 tests. Its names exactly match the saved
pre-rename suite (`wave9-rename-baseline-tests.json` and
`wave9-rename-final-tests.json`). Obsolete cached diagnostic options were removed;
the renamed options are enabled as before. Dependencies remained disconnected.

Full validation used `cmd /c tools\vega-build.cmd`, then
`ctest --test-dir build -j 8 --timeout 300`, with failure output and JUnit evidence
enabled. All default targets built, including game, editor, Blank, GAS and tests.

| Suite | Total | Passed | Known failures | Known timeouts |
| --- | --- | --- | --- | --- |
| Documented Wave 9 baseline | 184 | 160 | 23 | 1 |
| Vega rename | 184 | 160 | 23 | 1 |

Every non-passing test's identity and Failed/Timeout status matches the list in
`engine-context-validation.md`; there are zero status differences. This includes
the sandbox GetCursorPos failure in `LightYearsAbilityBalance-second-E2E` and
the existing timeout in `LightYearsRareZoneRulesE2E`. CTest's expected nonzero
result reflects these known failures. Full-suite wall time was 186.02 seconds.
Machine-readable comparisons are in `wave9-expected-failures.json` and
`wave9-actual-failures.json` under `build/`.

## Compatibility and intentional leftovers

The reflection golden SHA-256 before and after the directory move is:

`85EFAECE9519414889C8ACA8BD17672A5E33E1269775109A41A308EB6D738C53`

All 57 JSON/Lua files in the compatibility snapshot remain byte-for-byte
unchanged after the full suite. All 357 game assets retain their pre-test
contents and last-write timestamps. The reflection golden is unchanged.

The legacy namespace and macro-prefix scan found zero matches in first-party
source, documentation, templates and tools. The only old engine-name matches are
these deliberately preserved CTest identifiers and references to their results:

- `LightYearsEngineContext`
- `LightYearsEngineDiagnostics`
- `LightYearsEngineDiagnosticsNDebug`
- `LightYearsEngineLifetime`
- `LightYearsEngineHierarchyCost`
- `LightYearsEngineHierarchy`

Generated `build*` trees and `.sol` history are outside the rename scope.
The two bare `ly` strings in the visual gallery/astronomy test denote the
physical light-year unit and remain unchanged. The build dependency seed's
existing `D:\LightYears` location and game branding also remain unchanged.

## Environment caveats

The main worktree index at `D:\LightYears\.git\worktrees\Vega\index` is read-only
in this sandbox. The first ordinary `git mv` failed without moving files. A
second `git mv` succeeded using a copied index at `build/wave9-rename.index`.
The working-tree directory is renamed, but Claude must stage the rename and
edits in the main index. No other Git commands, commits or pushes were run.

Validation uses Debug/MSVC/C++17; Release is not tested. The codebase graph tool
was blocked by the session's approval policy, so discovery used local searches.
The Codex account usage limit is not visible in this session.
