# Wave 5 B4: optional Sky and GAS engine modules

Sky and GAS now provide independent runtime and editor targets. LightYears uses
both through the same `VegaEditorCore` used by Blank. The editor source guard
has **no exceptions**. LightYears' nine deterministic sky captures match the
baseline PNGs **byte for byte**.

## Layout decision

The top-level directories remain. Reusable renderer implementation moved to
`Modules/Sky/{include/sky,src,assets,editor}` with the `vega::sky` API and
`VegaSky` / `VegaSkyAstro` targets. Existing `visual/*.h` paths forward to it;
`LightYearsAstro` remains a compatibility interface target. `LightYearsVisual`
now compiles only ship definitions, ship rendering, the ship-aware derived asset
loader and its file helper. The generic `VisualAssets` never imports or loads
ship definitions.

Ship JSON, ship palettes and the authored sky collection retain their original
paths and bytes. The module owns reusable shaders; legacy shader paths remain
compatibility copies, with a CTest hash guard preventing drift. This avoids
breaking asset roots, hot reload, packaging and other branches' paths.

`Modules/GAS` defines `VegaGAS` (the existing `SpaceAbilitySystem` runtime target),
generic content descriptors and `VegaGASEditor`. Its established runtime source
and include paths stay under `SpaceAbilitySystem`; moving those widely imported
paths would add disruption without improving the dependency boundary.

## Runtime and editor behavior

- `VEGA_BUILD_SKY` and `VEGA_BUILD_GAS` independently control availability.
  The engine and editor core link neither. LightYears explicitly requires both.
- Sky's panel, View menu, viewport handles, document/history and script execution
  moved to `VegaSkyEditor`. The game supplies a route-free `SkyEditorHost` adapter
  with opaque IDs, geometry and deterministic rendering. The UI says
  **Preview sky**. `SkyIdForRouteStop` owns the legacy identity mapping game-side;
  `ProceduralBackdrop` and `GameVisualPipeline` remain game composition glue.
- GAS registers its debugger panel, layout settings, commands and Play/session
  recording through `EditorExtensions`. Generic ability/effect metadata is
  module-owned; LightYears retains game-specific fields, templates, references,
  ID/tag vocabularies and production validators.
- `EditorModuleTool` supplies lifecycle, menu, active input, history and dirty
  exit hooks. One editor core handles both module-enabled and plain hosts.
  The old `VEGA_EDITOR_WITH_SKY` / `VEGA_EDITOR_WITH_GAS` assemblies and stubs
  are gone.
- LightYears installs `EngineAssets` from module initialization through
  `InstallLightYearsEngineAssets`. Its 64 direct test entry points call the same
  function explicitly. Reflection linking no longer changes process defaults.
  The project already specifies `runtime.log: "LightYears.log"`; it remains so.
- Blank remains independent by default. Module enablement and the neutral
  single-asset example are documented instead of adding a template option.

## Sky formats

| Format | Envelope |
| --- | --- |
| Individual `skies/aurora.sky.json` | `{"id":"aurora","sky":{...}}` |
| Named collection | `{"presets":{...},"skies":{"aurora":{...}}}` |
| Existing collection | `{"presets":{...},"stops":{"legacy-id":{...}}}` |

Preset references, appended arrays, opaque fields and ID-based seeds retain
their existing behavior. `DeepSkyCatalog::Load` accepts all three formats.
`VisualAssets::Load(root, error, relativeSkyFile)` hot-reloads the selected
file and restores it after preview; its default remains `deepsky.json`.
`SkyDocument(root, relativeSkyFile)` edits each format while preserving original
bytes. New tests exercise edits, undo, save/reopen, opaque numeric spelling,
format-independent seeds, failed reload retention and preview restoration.

## Files

| Area | Files |
| --- | --- |
| Build/module selection | `CMakeLists.txt`, `Modules/Sky/CMakeLists.txt`, `Modules/GAS/CMakeLists.txt`, `SpaceAbilitySystem/CMakeLists.txt`, `LightYearsVisual/CMakeLists.txt`, `LightYearsGame/editor.cmake`, `VegaEditor/CMakeLists.txt` |
| Sky runtime | `Modules/Sky/include/sky/*.h`, `Modules/Sky/src/*`: Astro, GalaxyModel, Relativity, color ramps/scripts, settings, shader/uniform helpers, assets/reports, ShapeBrush, Backdrop, Galaxy, Nebula, Anomalies, NearField, AsteroidField, DeepSky, OrbitSky, VisualPipeline and file helpers |
| Sky assets | `Modules/Sky/assets/shaders/*.frag`, `palettes/default.json`, `skies/aurora.sky.json`; existing visual headers forward to the new runtime |
| Sky editor | `Modules/Sky/editor/CMakeLists.txt`, `include/sky/SkyEditorHost.h`, moved `include/editor/{EditorSkyEditor,SkyDocument}.h`, moved `src/{SkyDocument,EditorSkyEditor,EditorSkyHandles,EditorSkyScript}.cpp`, new `src/SkyEditorExtension.cpp` |
| GAS schemas/editor | `Modules/GAS/include/gas/GasContentSchemas.h`, `src/GasContentSchemas.cpp`, `editor/CMakeLists.txt`, `editor/include/gas/GasEditorExtension.h`, moved `editor/include/editor/EditorGasDebugger.h`, moved `editor/src/EditorGasDebugger.cpp`, new `editor/src/GasEditorExtension.cpp` |
| Editor host | `VegaEditor/include/editor/{EditorTool,EditorExtensions,EditorApplication,EditorCommands}.h`, `VegaEditor/src/{EditorApplication,EditorScenes,EditorPanels,EditorChrome,EditorScript,EditorScriptParser}.cpp` |
| Game integration | `LightYearsGame/include/gameFramework/{LightYearsAssets,LightYearsGameModule}.h`, `include/presentation/visual/SkyMapping.h`, `src/{editor/LightYearsEditorModule,gameFramework/LightYearsGameModule,gameplay/content/GameContentSchemas,presentation/visual/ProceduralBackdrop,reflection/GameReflection}.cpp`, explicit startup calls in `LightYearsGame/tests/*.cpp`, `LightYearsVisual/{include/visual/VisualAssets.h,src/VisualAssets.cpp}` |
| Verification | `Modules/Sky/tests/SkyAssetsTests.cpp`, `Modules/GAS/tests/GasContentSchemasTests.cpp`, `Modules/tests/NoGameDependencies.cmake`, `VegaEditor/tests/{SkyDocumentTests.cpp,NoGameIncludes.cmake}`, `Templates/Blank/Tests/NoGameDeps.cmake` |
| Documentation | `docs/engine-modules.md`, `docs/{game-module,new-game,sky-editor,gas-debugger,gas-content-editor}.md`, `LightYearsVisual/README.md`, this report |

Existing unrelated worktree edits were retained. No git commands were run.
No authoring/test operation wrote into `LightYearsGame/assets`.

## Validation

- Build: `cmd /c tools\vega-build.cmd` (with bounded build parallelism), passed.
- Baseline full suite: **118 passed, 24 failed / 142**, 120.06 seconds.
- Full verification: `ctest --test-dir build -j 8 --timeout 300`:
  **121 passed, the same 24 failed / 145**, 168.57 seconds. Every required
  engine, Reflection, VegaScript, Scene, Project, Prefab, Diagnostics,
  SpaceAbilitySystem, GameModule, Blank and VegaEditor test passed.
- **No existing test changed status.** New passing tests:
  `EngineModulesNoGameDependencies`, `VegaSkyAssets`,
  `SpaceAbilitySystemContentSchemas`.
- After the final compatibility-header formatting and preservation of the Sky
  status label, the final source build passed and all **121/121** tests outside
  the unchanged baseline failure list passed again, in **37.89 seconds**.
  The nine sky capture hashes remained identical on this final run.
- The 24 failures are exactly the baseline's 23 allowed gameplay failures and
  the allowed sandbox cursor-access failure in
  `LightYearsAbilityBalance-second-E2E`, including the same RareZoneRules timeout.
- Isolated configuration, LightYears/Sky/GAS OFF: full **23/23 passed**;
  Blank runtime/editor and the engine/scripting suite built without the modules.
- Isolated Sky ON, LightYears/GAS OFF: `VegaSkyEditor` built, full **24/24 passed**.
- Isolated GAS ON, LightYears/Sky OFF: `VegaGASEditor` built, full **26/26 passed**.
- All nine sky-only PNGs matched pre-change SHA-256 hashes: disk baseline,
  unchanged preview, before, moved, edited, restored, reopened, objects and orbit.
  Existing sky pixel-change/undo checks and document byte-stability checks passed.
  Full editor UI images intentionally differ because the picker label changed.
- Guard probes rejected each former Sky include exception at its original
  header path and accepted an engine-only control.
- Editor runs used `--editor-script` from the corresponding `build*/VegaEditor`
  directories. Runtime/export smoke runs used `--visual-capture`. All generated
  authoring data and captures stayed in build-tree copies.

Evidence: [final build](../build/b4-build-complete.log),
[baseline suite](../build/b4-baseline-tests.log),
[full suite](../build/b4-final-tests.log),
[final passing-suite rerun](../build/b4-passing-tests.log),
[status comparison and failure names](../build/b4-test-status-comparison.json),
[sky hash comparison](../build/b4-sky-comparison.json),
[exact file manifest](../build/b4-files.json),
[guard probes](../build/b4-guard-probes.log),
[isolated build](../build/b4-isolated-build.log),
[isolated suite](../build/b4-isolated-tests.log),
[Sky-only build](../build/b4-sky-only-build.log),
[Sky-only suite](../build/b4-sky-only-tests.log),
[GAS-only build](../build/b4-gas-only-build.log),
[GAS-only suite](../build/b4-gas-only-tests.log).

## Remaining coupling

Compatibility include/asset paths and shader copies remain under
`LightYearsVisual`; the generic targets do not depend on its target or game
code. GAS runtime source paths and its `sas` namespace remain unchanged.
The editor retains the established Sky/GAS command grammar; module execution
is registered through extensions and unavailable when disabled. The shared
editor still uses VegaScript. Sky preview composition and route/planet context
are supplied by a game adapter; optional scene `sky` settings are not added to
the engine schema. Modules are statically linked, with no dynamic plugin loader.
