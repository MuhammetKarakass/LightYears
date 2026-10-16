# Game modules

`vega::GameModule` (`framework/GameModule.h`) is the boundary between a game
and its host. Both standalone LightYears and Vega Editor use
`ly::LightYearsGameModule`. The editor derives from `ly::Application`; it no
longer inherits the game's application or includes its scene, player or spawn
marker classes.

Each application owns one module. Call
`application.RegisterGameModule(module, editor)` once before running it. This
registers types, then initializes services. The module must outlive the worlds.
The editor owns the module passed to `vega::CreateEditorApplication` and calls
`ConfigureProject` with the selected project file before registration. Hosts
forward tick, events, after-render and exit-code queries through the contract.

Required methods are `Name`, `RegisterTypes`, `Initialize` and `CreateLevel`.
`CreateLevel(application, sceneJson, edit)` returns an unstarted world. The
host queues it through `Application::LoadWorld(shared_ptr<World>)`, using the
same deferred swap as the typed overload. Edit worlds report `IsEditMode()`,
pause gameplay and load entities without starting gameplay lifecycles. Play
worlds use the game's normal scene and player setup. Standalone scene loading
and Play in editor use this same factory.

Optional `BeginPlay` runs after a play world's normal `BeginPlayInternal`;
`EndPlay` runs before replacement or shutdown. Neither runs for edit worlds.
`PlayerActor` returns an optional actor from the requested world. `DisplayInfo`
returns spawn marker metadata and a label, letting the editor render and pick
markers without knowing their C++ types. `ApplySceneSettings` and
`SceneBackdropState` bridge presentation settings.

Continue registering reflection, content schemas, scene settings, GAS debugger
resolvers and Lua player resolvers through the existing registries from the
module. LightYears retains those registrations and its existing runtime setup.

## Editor extensions

Override `RegisterEditorExtensions(vega::EditorExtensions&)` in the game's
editor adapter. Include `editor/EditorExtensions.h` there; standalone games
need no editor dependency.

- `AddPanel(callback)` draws a panel each frame. The callback owns its ImGui
  window and visibility state.
- `AddViewport(callback)` supplies the image rectangle in ImGui screen
  coordinates, texture size, hover state, context and command history. It
  supports overlays and handles; use history commands for authored changes.
- `AddHierarchyMenu(callback)` emits items in the Hierarchy menus.
- `AddScriptCommand(name, callback)` adds a verb receiving decoded argument
  tokens, context, history and a diagnostic string. It validates arguments and
  returns success. Unregistered verbs remain errors; core verbs have priority.

Callbacks run on the editor thread. Panels and scripts receive the active scene
or prefab context; viewport and Hierarchy callbacks receive the scene context.
Captured state must live as long as the module. Extension arguments use the
existing tokenizer, including quotes, comments and composite JSON.

`LightYearsGame/src/editor/LightYearsGameModule.cpp` registers
`expect-player-ship`, retaining its name, argument rules, world check and report
text. The editor core has no ship-specific script operation.

## Register another game

Implement the runtime contract in the game and an optional editor adapter.
The concrete entry point can be:

```cpp
#include <editor/EditorApplication.h>
#include "MyEditorModule.h"

ly::Application* GetApplication()
{
	return vega::CreateEditorApplication(std::make_unique<MyEditorModule>());
}
```

Select `VEGA_GAME_MODULE_TARGET`, `VEGA_GAME_RUNTIME_TARGET`,
`VEGA_GAME_EDITOR_ENTRY` and `VEGA_DEFAULT_PROJECT_FILE` before adding
`VegaEditor`. The current selection is isolated in `LightYearsGame/editor.cmake`,
included once by the root CMake file, with the concrete entry in
`VegaEditor/app/LightYearsEditorMain.cpp`. Asset roots, fonts, build directories
and package names come from the [project file](project.md). Existing directory,
namespace and executable target names are unchanged.

## Remaining B4 coupling

Sky remains in the editor for this wave. `EditorSkyEditor.h` includes the game's
`presentation/visual/GameVisualPipeline.h`; `SkyDocument.h` includes
`visual/DeepSky.h`. Preview rendering, route thumbnails, handles, schema and
save/undo state still rely on those implementations. Extracting that stateful
tool belongs to B4. `Bind` accepts the engine render-pipeline base and leaves
Sky inactive for a different pipeline. Building without the LightYears
presentation headers still requires the B4 extraction or a Sky adapter.

`VegaEditorNoGameIncludes` scans every file under `VegaEditor/src` and
`VegaEditor/include`, rejects game paths and direct game/visual headers, and
allows exactly those two Sky include lines. The concrete entry is outside those
directories. Game CMake selection, LightYears integration fixtures and Sky
linkage remain integration inputs; core CMake no longer embeds game asset
paths or a concrete game-app target.

`VegaEditorGameModule` exercises a synthetic module linked only against
VegaScript and the engine. Export tests also package another runtime with
different asset directories and verify that visual assets are optional.
