# Wave 5 B1: editor game-module boundary

The editor now derives from the engine application and receives an owned
`vega::GameModule`. LightYears runtime setup is shared by its standalone host
and editor module. Scene creation, settings application, player lookup and
spawn-marker metadata remain game-side. `expect-player-ship` is registered by
the game's editor adapter, with its existing name and behavior.

## Contract and integration

`framework/GameModule.h` defines registration/initialization, scene factory,
BeginPlay/EndPlay, optional player lookup, actor display information, editor
extension registration, presentation settings, tick/event/render and exit-code
hooks. `Application::RegisterGameModule` is the registration point;
`LoadWorld(shared_ptr<World>)` retains deferred world swaps.

`editor/EditorExtensions.h` registers panels, viewport overlays/handles,
Hierarchy menu items and script commands. The pure parser uses the registered
verb list and retains errors for unregistered commands.

The concrete game target, runtime dependency and editor entry are selected in
`LightYearsGame/editor.cmake`. Core editor CMake has no concrete game-app link
or game/visual asset paths. `vega.project.json` supplies editor paths and runtime
packaging names, with engine defaults and command-line overrides. Project
settings saves preserve those fields. Export handles arbitrary executable and
asset directory names, optional visuals, configured runtime logs, Release
selection, smoke captures and the existing overwrite protection.

## Files

| Area | Added or changed files |
| --- | --- |
| Root integration | `CMakeLists.txt` |
| Engine contract/lifecycle | `LightYearsEngine/include/framework/GameModule.h`, `LightYearsEngine/include/framework/Application.h`, `LightYearsEngine/src/framework/Application.cpp` |
| Project schema | `LightYearsEngine/include/framework/ProjectSettings.h`, `LightYearsEngine/src/framework/ProjectSettings.cpp`, `LightYearsEngine/tests/ProjectSettingsTests.cpp` |
| Game integration | `LightYearsGame/CMakeLists.txt`, `LightYearsGame/editor.cmake`, `LightYearsGame/assets/vega.project.json` |
| Shared game services | `LightYearsGame/include/gameFramework/LightYearsGameModule.h`, `LightYearsGame/src/gameFramework/LightYearsGameModule.cpp`, `LightYearsGame/include/gameFramework/GameApplication.h`, `LightYearsGame/src/gameFramework/GameApplication.cpp` |
| Game editor adapter | `LightYearsGame/include/editor/LightYearsEditorModule.h`, `LightYearsGame/src/editor/LightYearsGameModule.cpp`, `VegaEditor/app/LightYearsEditorMain.cpp` |
| Editor public API | `VegaEditor/include/editor/EditorExtensions.h`, `VegaEditor/include/editor/EditorProject.h`, `VegaEditor/include/editor/EditorApplication.h`, `VegaEditor/include/editor/EditorCommands.h`, `VegaEditor/include/editor/EditorScript.h`, `VegaEditor/include/editor/EditorTheme.h`, `VegaEditor/include/editor/EditorSkyEditor.h`, `VegaEditor/include/editor/GameExporter.h` |
| Editor implementation | `VegaEditor/CMakeLists.txt`, `VegaEditor/src/EditorMain.cpp`, `VegaEditor/src/EditorApplication.cpp`, `VegaEditor/src/EditorProject.cpp`, `VegaEditor/src/EditorScenes.cpp`, `VegaEditor/src/EditorPanels.cpp`, `VegaEditor/src/EditorScript.cpp`, `VegaEditor/src/EditorScriptParser.cpp`, `VegaEditor/src/EditorTheme.cpp`, `VegaEditor/src/EditorSkyEditor.cpp`, `VegaEditor/src/GameExporter.cpp` |
| Editor verification | `VegaEditor/tests/NoGameIncludes.cmake`, `VegaEditor/tests/GameModuleTests.cpp`, `VegaEditor/tests/EditorProjectTests.cpp`, `VegaEditor/tests/EditorScriptTests.cpp`, `VegaEditor/tests/GameExporterTests.cpp` |
| Documentation | `docs/game-module.md`, `docs/project.md`, this report |

## Remaining coupling

- Sky: `EditorSkyEditor.h` still imports the game's `GameVisualPipeline`, and
  `SkyDocument.h` still imports `visual/DeepSky.h`. Preview rendering, route
  thumbnails, handles and document state require the B4 extraction. These are
  the only two direct game-header exceptions in the source guard. Sky binds
  through the engine pipeline base and stays inactive for another pipeline;
  its build still requires the current presentation headers/libraries.
- Concrete editor assembly: the single game CMake selection and entry remain
  intentionally game-specific until B3 separates the editor library and game
  executable. Existing integration fixtures still exercise LightYears content,
  scenes and exported runtime. Directory, namespace and existing target names
  remain unchanged.

See [game modules](game-module.md) and [project settings](project.md) for the API
and configuration examples.

## Build and tests

- `cmd /c tools\vega-build.cmd`: passed, exit 0.
- Baseline `ctest --test-dir build -j 8 --timeout 300`: 111 passed, 24 failed
  out of 135. The failures are the 23 pre-existing gameplay failures and the
  allowed sandbox `LightYearsAbilityBalance-second-E2E` failure.
- Verification with the same full-suite command: 115 passed, the same 24
  failed out of 139, in 120.05 seconds. Every required engine, reflection,
  scripting, scene, project, prefab, diagnostics, ability-system and editor
  test passed.
- Final editor rerun after the optional-asset-root capture check:
  `ctest --test-dir build -j 8 --timeout 300 -R VegaEditor`: **33/33 passed**.
- Guard negative probes in the build tree rejected a direct game-header
  include and a hard-coded game asset path, and accepted an engine header.
- No existing test changed status. New passing tests: `VegaEditorGameModule`,
  `VegaEditorNoGameIncludes`, `VegaEditorProjectDefaults`, and
  `VegaEditorProjectOverrides`.

Evidence: [build log](../build/b1-build-verified.log),
[baseline suite](../build/b1-baseline-tests.log),
[verified suite](../build/b1-verified-tests.log),
[final editor suite](../build/b1-editor-verified-tests.log),
[status comparison](../build/b1-test-status-comparison.json).

Editor integration runs used `--editor-script`; exported game smoke tests used
`--visual-capture`. New fixtures and generated outputs stayed in the build
tree. No git commands were run.
