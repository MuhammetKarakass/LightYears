# Wave 5 B2: independent Blank project

`Templates/Blank` supplies a second game with no current-game code or assets.
`BlankGameModule` registers `BlankPlayer` and `BlankMovement` through
`VEGA_CLASS`, `VEGA_REFLECT` and `VEGA_REGISTER`. The player has a sprite and
WASD/arrow-key movement; its Lua script rotates to 90 degrees during Play.
The scene contains the player and two stationary tiles. The 32 x 32 texture is
original generated geometric art, with a reproducible PowerShell generator.
No font file is required: the editor uses ImGui's default font.

## Independence fixes

- Split editor assembly into `VegaEditorCore` and a LightYears assembly of the
  same source list. The plain core excludes Sky and GAS implementations and
  imports; disabled tools have no menu entries. LightYears opts into both and
  retains its existing extensions and behavior.
- Added `VegaEditor/cmake/VegaGameEditor.cmake`. Its
  `vega_add_game_editor` helper builds each project's editor with its module,
  runtime dependency, project configuration, entry point, DLLs and a separate
  synchronized asset folder. `EditorHost.h` exposes the factory without
  importing editor implementation types.
- Moved the runtime-DLL copy helper out of the game's CMake and into the
  engine. Added root `VEGA_BUILD_LIGHTYEARS` and `VEGA_BUILD_GAS` switches to
  configure and build Blank with those directories excluded. Template targets
  are gated by `VEGA_BUILD_TEMPLATES`, default ON.
- Removed engine-owned SpaceShooterRedux widget font, button image/sound,
  point-light shader and default shader paths. `vega::EngineAssets` contains
  optional project-installed defaults. LightYears installs the previous values
  in its own reflection module, including direct gameplay test hosts.
  Missing optional engine fonts/textures use empty SFML fonts/the white pixel;
  absent optional sounds/shaders trigger no external loads.
  The engine UI test now checks empty defaults and a project-installed example
  font rather than asserting a dependency on SpaceShooterRedux.
- Engine entry-point logging now uses the project's runtime log setting, with
  `Vega.log` as the generic fallback. LightYears keeps `LightYears.log` and
  Blank uses `Blank.log`.
- Editor startup honors the selected project's start scene, retaining the
  sandbox fallback for an empty setting and the command-line override.
- Export portability checks ignore the compiled source-location suffix on log
  records while still rejecting actual source-tree asset paths in messages.
  Regression checks cover both cases.
- Renamed `LightYearsGame/src/editor/LightYearsGameModule.cpp` to
  `LightYearsEditorModule.cpp` and updated its CMake source.

Blank runtime paths resolve beside its executable or through `--asset-root`.
Its capture host supports `--visual-capture` and `--capture-frames`; those flags
previously existed only in the current game's module, not in Application.

## Per-game declaration

```cmake
vega_add_game_editor(BlankEditor MODULE BlankGameModule RUNTIME BlankGame
    PROJECT "${CMAKE_CURRENT_SOURCE_DIR}/Assets/vega.project.json"
    ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/Source/BlankEditorMain.cpp")
```

LightYears uses the same helper with `CORE VegaEditorLightYearsCore` and its
existing editor module/entry. See [make a new game](new-game.md) for copying,
renaming, registering types, project paths, builds, scripts and export.

## Files

| Area | Files |
| --- | --- |
| Template build | `Templates/Blank/CMakeLists.txt` |
| Module and hosts | `Templates/Blank/Source/BlankGameModule.h`, `BlankGameModule.cpp`, `BlankMain.cpp`, `BlankEditorMain.cpp` |
| Template assets | `Templates/Blank/Assets/vega.project.json`, `scenes/main.scene.json`, `scripts/spin.lua`, `textures/tile.png`, `textures/README.md` |
| Original art generator | `Templates/Blank/Tools/GenerateTexture.ps1` |
| Acceptance fixtures | `Templates/Blank/Tests/Dependencies.cmake`, `NoGameDeps.cmake`, `RunGame.cmake`, `RunEditor.cmake`, `blank.vegascript.in` |
| Build integration | `CMakeLists.txt`, `VegaEngine/CMakeLists.txt`, `LightYearsGame/CMakeLists.txt`, `LightYearsGame/editor.cmake`, `VegaEditor/CMakeLists.txt`, `VegaEditor/cmake/VegaGameEditor.cmake` |
| Engine asset defaults | `VegaEngine/include/framework/EngineAssets.h`, `include/engineConfigs/EngineStructs.h`, `include/widget/Button.h`, `include/widget/TextWidget.h`, `src/framework/AssetManager.cpp`, `src/framework/LightComponent.cpp`, `src/framework/ShaderManager.cpp`, `src/widget/Button.cpp`, `src/widget/UIStyle.cpp`, `src/widget/ValueGauge.cpp`, `tests/EngineLifetimeTests.cpp`, `tests/UIFoundationTests.cpp` |
| Logging | `VegaEngine/src/EntryPoint.cpp` |
| LightYears defaults/adapter | `LightYearsGame/src/reflection/GameReflection.cpp`, `LightYearsGame/src/editor/LightYearsEditorModule.cpp` (renamed) |
| Editor boundary/options | `VegaEditor/include/editor/EditorHost.h`, `EditorSkyEditor.h`, `EditorGasDebugger.h`, `VegaEditor/app/LightYearsEditorMain.cpp`, `VegaEditor/src/EditorApplication.cpp`, `EditorScenes.cpp` |
| Export checks | `VegaEditor/src/GameExporter.cpp`, `VegaEditor/tests/GameExporterTests.cpp` |
| Documentation | `docs/new-game.md`, `docs/game-module.md`, `docs/project.md`, this report |

## Validation

- `cmd /c tools\vega-build.cmd`: passed, exit 0.
- Baseline full suite: 115 passed and 24 failed out of 139.
- Final `ctest --test-dir build -j 8 --timeout 300`: **118 passed and the
  same 24 failed out of 142**, in 151.43 seconds. All required engine,
  Reflection, VegaScript, Scene, Project, Prefab, Diagnostics,
  SpaceAbilitySystem, GameModule, NoGameIncludes and VegaEditor tests passed.
  The failures are the 23 pre-existing gameplay failures (including the same
  RareZoneRules timeout) and the permitted sandbox cursor-access failure in
  `LightYearsAbilityBalance-second-E2E`.
- **No existing test changed final status.** New passing tests:
  `BlankTemplateNoGameDeps`, `BlankGameRuns`, and `BlankEditorScript`.
- Separate `build-blank-only` configuration with `VEGA_BUILD_LIGHTYEARS=OFF`,
  `VEGA_BUILD_GAS=OFF`, `VEGA_BUILD_EDITOR=ON`, `VEGA_BUILD_TEMPLATES=ON`:
  BlankGame and BlankEditor built successfully, exit 0; all **3/3 Blank tests
  passed**. No current-game, presentation or GAS subdirectory was added.
- Both editor fixtures passed on repeated runs. Every authoring operation used
  a build-tree asset copy. Each fixture checked Play/Stop, the Lua rotation,
  reflected properties/components, prefab creation/placement, save/reopen,
  export smoke capture and an additional 12-frame exported executable run.
- The dependency guard accepted an engine-only control and rejected injected
  game links and imports in disposable build-tree probes. Source template
  asset hashes remained unchanged. Runtime and editor captures were visually
  inspected: original cyan tiles and the default ImGui font, with no game art.
- Editor launches used `--editor-script` from their `build*/VegaEditor`
  directories; runtime launches used `--visual-capture`. No git commands were
  run. Graph indexing was unavailable under the automatic approval policy, so
  code discovery used source fallback after the graph returned no worktree index.

Evidence: [final build](../build/b2-build-final.log),
[baseline suite](../build/b2-baseline-tests.log),
[final suite](../build/b2-final-tests.log),
[JUnit](../build/b2-final-tests.xml),
[status comparison and failure list](../build/b2-test-status-comparison.json),
[isolated build](../build/b2-blank-only-verified.log),
[isolated tests](../build/b2-blank-only-tests.log),
[guard probes](../build/b2-dependency-probes.log),
[editor script report](../build/blank-editor-test/blank.vegascript.report.txt),
[runtime capture](../build/Templates/Blank/blank-capture.png), and
[editor capture](../build/blank-editor-test/editor.png).

## Remaining coupling

Sky's implementation still depends on LightYears presentation types and remains
in the optional LightYears editor assembly pending B4. Default repo builds keep
the current game's integration fixtures; they are excluded by
`VEGA_BUILD_LIGHTYEARS=OFF`. The shared editor currently includes VegaScript
services. The runtime engine does not require Lua, GAS or Sky. At that stage, existing engine
directory/target names and the engine namespace remained unchanged, as did shared
collision labels. None of these require game libraries or assets in Blank's
transitive target graph or exported package.

The exporter retains its pre-existing conservative warning for Debug packages
("not portable: reads assets from the source tree"). Blank's runtime has no
source-root fallback; its Debug package is exercised from the export directory
in both configurations. The warning policy was left unchanged for LightYears.
