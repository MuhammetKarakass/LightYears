# Make a new game with Vega

`Templates/Blank` is a working project with its own original texture, scene,
reflected actor and component, runtime, editor and export. It links the engine
and VegaScript. It does not link the current game, its presentation library,
GAS or Sky, and it does not load their assets. The editor uses ImGui's built-in
font when the project's `editor.font` is absent.

The template also ships sample scenes, Lua scripts, a prefab and a sound; see
`Templates/Blank/README.md` and [Learn from the Blank samples](#learn-from-the-blank-samples).

1. Copy `Templates/Blank` to `Templates/MyGame` (or another folder in this
   repository). Rename the `BlankGameModule`, `BlankPlayer`, `BlankMovement`,
   `BlankGame`, and `BlankEditor` identifiers and the `blank` namespace. Rename
   the C++ files and update their CMake source paths. Rename the test names,
   test output folders and exported executable checks in `Tests/` too when
   keeping both projects in one build. Rename the actor type in
   the scene and the executable in the project file too. Reflected type and
   property names become persistent scene/prefab keys.
2. Add `add_subdirectory(Templates/MyGame)` after the editor is configured in
   the root CMake file. Keep the original template or disable its targets with
   `-DVEGA_BUILD_TEMPLATES=OFF` in your own build configuration.
3. Implement your module's `Name`, `RegisterTypes` and `CreateLevel`.
   Add `IModuleSetup` if the game needs project configuration or service
   initialization; `Initialize` is part of that optional capability. Add
   `IPlayerProvider` only if hosts need player lookup. Blank uses both;
   a minimal module needs neither. See the three-method example and optional
   capability table in [game modules](game-module.md).
   The minimal declaration is:

   ```cpp
   class MyGameModule final : public vega::GameModule
   {
   public:
   	const char* Name() const override { return "MyGame"; }
   	void RegisterTypes() override;
   	std::shared_ptr<ly::World> CreateLevel(ly::Application& application,
   		const nlohmann::json& scene, bool edit) override;
   };
   ```

   Declare types with `VEGA_CLASS` in their headers, describe them with
   `VEGA_REFLECT` in their source files, and explicitly call `VEGA_REGISTER`
   from `RegisterTypes`. Register bases before derived types. Opt actors into
   `.Spawnable()` and components into `.Addable()` only when the editor should
   construct them. Mark constructor-required components with `VEGA_REQUIRES`.
4. Edit `Assets/vega.project.json`: set `name`, `startupScene`, window size,
   `editor.assetRoot`, build/export directories and `runtime.executable`.
   Paths in `editor` resolve relative to the project file. The runtime loads
   its packaged `assets/vega.project.json` beside the executable; `--asset-root`
   and `--scene` override the runtime selection. All scene texture and script
   paths are relative to the asset root. See [project settings](project.md).
5. Declare the editor using the shared engine-side helper, after linking your
   module against the services it uses:

   ```cmake
   vega_add_game_editor(MyEditor MODULE MyGameModule RUNTIME MyGame
       PROJECT "${CMAKE_CURRENT_SOURCE_DIR}/Assets/vega.project.json"
       ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/Source/MyEditorMain.cpp")
   ```

   The entry calls `vega::CreateEditorApplication` with your module. The helper
   links `VegaEditorCore`, compiles the project configuration, adds the runtime
   dependency and copies DLLs and project assets. It builds editors in
   `build/VegaEditor`; each additional editor gets its own synchronized asset
   folder. The default project points at authoring assets. LightYears links
   optional module extensions to this same core.
6. Build: `cmd /c tools\vega-build.cmd --target MyGame MyEditor`. In this
   repository, unattended editor runs must use an editor script from
   `build\VegaEditor`, for example:

   ```powershell
   .\MyEditor.exe --project D:/Vega/Templates/MyGame/Assets/vega.project.json --editor-script D:/Vega/build/my-check.vegascript
   ```

   Use a build-tree copy and `--asset-root` for scripts that save documents.
   The Blank acceptance script shows opening/selecting, reflected properties,
   components, Play/Stop, Lua, prefabs and save/reopen.
7. Export through File > Export Game or `export "<build-tree destination>"
   overwrite` in an editor script. Build the runtime first and keep the export
   outside its build directory. The exporter copies the runtime and DLLs,
   renames it from the project name, copies assets, and smoke-tests it. Test
   the resulting package from its own folder with
   `Blank.exe --visual-capture check.png --capture-frames 12`. The package needs
   neither the source tree nor the editor. For a renamed project use its
   project-named executable.

## Learn from the Blank samples

`Templates/Blank/README.md` lists every sample. Open them in the editor or run them with
`BlankGame.exe --scene scenes/<name>.scene.json` before you rename the template:

| Sample | What it teaches |
| --- | --- |
| `scenes/hierarchy.scene.json` + `scripts/orbit.lua` | `parent` links, relative `Location`, Lua `children()`, `detach()` and `attach_to()`. See [actor hierarchy](hierarchy.md). |
| `scenes/trigger.scene.json` + `scripts/trigger.lua`, `lamp.lua` | An added `CollisionComponent`, layers and masks, `on_overlap_begin/end`, a `Target` reference, `actor:send` / `on_message`, `world.spawn` of `prefabs/Spark.prefab.json` and `audio.play`. See [Lua API](lua-api.md) and [prefabs](prefabs.md). |
| `scenes/patrol.scene.json` + `scripts/patrol_loop.lua`, `lap_counter.lua` | A Vec2 list property, a looping patrol and messages to a referenced actor. See [reflection](reflection.md). |
| `scripts/timers.lua` (used by the Spark prefab) | `timer.after`, `timer.every` and cancelling timers. |

The samples use only `BlankPlayer`, `BlankMovement` and engine components, so they keep working
after the rename in step 1 as long as the scene `type` names and the script paths are updated.
Keep, rewrite or delete them for your own game; the CTest drivers in `Tests/` depend only on
`main.scene.json`, `scripts/spin.lua` and `scripts/patrol.lua`.

The original template's verification is:

```powershell
cmd /c tools\vega-build.cmd
ctest --test-dir build -R "^Blank" --output-on-failure
```

`BlankTemplateNoGameDeps` checks template and engine source imports/assets and
the actual transitive CMake target graph, including configuration-specific
links. `BlankGameRuns` captures the start scene. `BlankEditorScript` authors
only a build-tree copy, verifies Lua reached 90 degrees during Play and restored
zero on Stop, round-trips reflected values/components and a prefab instance,
exports, and runs the package again.

To configure without the current game or its optional libraries, use a separate
build directory with `-DVEGA_BUILD_LIGHTYEARS=OFF -DVEGA_BUILD_GAS=OFF
-DVEGA_BUILD_SKY=OFF` and leave
`VEGA_BUILD_EDITOR` and `VEGA_BUILD_TEMPLATES` enabled. This excludes the
LightYears game and both optional modules entirely. Default repo
builds retain all existing targets and integration tests.

GAS and Sky are optional runtime/editor modules; Blank links neither. Lua is
optional at the engine level. To
remove it from this template, remove the scene's `scripts` entries, script
registration/root/player setup, and replace `BlankMovement`'s key reader with
SFML input; link the runtime module to `LightYearsEngine`. The shared editor
currently includes VegaScript services. See [game modules](game-module.md).

Runtime widgets and point lights accept project-owned defaults through
`ly::EngineAssets::Get()` (`framework/EngineAssets.h`). Install font, button
texture/sound and shader paths before creating those widgets. Empty defaults
load no external font or shader; absent button textures use the engine's white
pixel. LightYears calls `InstallLightYearsEngineAssets` during module startup;
direct gameplay test hosts call the same function explicitly.

## Optional Sky and GAS modules

Blank's default runtime and editor do not link Sky or GAS. Enable modules by
linking `VegaSky` / `VegaGAS` at runtime and `VegaSkyEditor` /
`VegaGASEditor` in your editor adapter. Register their editor extensions from
`IEditorProvider::RegisterEditorExtensions`; all games share `VegaEditorCore`.
See [engine modules](engine-modules.md) for the complete setup and individual
sky asset example. For a minimal build, disable `VEGA_BUILD_LIGHTYEARS`,
`VEGA_BUILD_SKY` and `VEGA_BUILD_GAS`. Each module may also be enabled alone.
