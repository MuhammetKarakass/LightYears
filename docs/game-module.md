# Game modules

`vega::GameModule` (`framework/GameModule.h`) is the boundary between a game
and its host. Both standalone LightYears and Vega Editor use
`ly::LightYearsGameModule`. The editor derives from `ly::Application`; it no
longer inherits the game's application or includes its scene, player or spawn
marker classes.

Each application owns one module. Call
`application.RegisterGameModule(module, editor)` once before running it. This
registers types, then initializes services through `IModuleSetup` when present.
The module must outlive the worlds.
Each application owns an [EngineContext](engine-context.md). Module registration
and setup bind it for existing static-accessor calls. New code should get services
from `application.GetEngineContext()` or `world.GetEngineContext()`, and register
types with `VEGA_REGISTER_IN(Type, context.Types())`. Registration guards must be
local to that registry so a second application can register the same module.
The editor owns the module passed to `vega::CreateEditorApplication` and calls
`IModuleSetup::ConfigureProject` with the selected project file before registration.
Hosts query optional capabilities through `QueryCapability<T>(module)`; a missing
capability keeps the previous no-op, empty or zero behavior. Queries return
non-owning pointers, preserve constness and accept null modules. RTTI is confined
to this helper.

Required methods are only `Name`, `RegisterTypes` and `CreateLevel`.
`CreateLevel(application, sceneJson, edit)` returns an unstarted world. The
host queues it through `Application::LoadWorld(shared_ptr<World>)`, using the
same deferred swap as the typed overload. Edit worlds report `IsEditMode()`,
pause gameplay and load entities without starting gameplay lifecycles. Play
worlds use the game's normal scene and player setup. Standalone scene loading
and Play in editor use this same factory.

| Optional interface | Methods |
| --- | --- |
| `IModuleSetup` | `ConfigureProject`, `Initialize` |
| `IPlayHooks` | `BeginPlay`, `EndPlay` |
| `IPlayerProvider` | `PlayerActor` |
| `IActorDisplayInfo` | `DisplayInfo` |
| `IEditorProvider` | `RegisterEditorExtensions` |
| `ISceneSettingsHandler` | `ApplySceneSettings`, `SceneBackdropState` |
| `IAppLoopHooks` | `Tick`, `HandleEvent`, `AfterRender`, `ExitCode` |
| `IContentReload` | `ReloadContent` |

Implement only the interfaces the game uses, with public multiple inheritance.
Blank implements `GameModule`, `IModuleSetup` and `IPlayerProvider`. LightYears
also implements display information, scene settings, app loop and content reload;
its editor adapter adds `IEditorProvider`. Neither needs `IPlayHooks` today.

```cpp
class MinimalModule final : public vega::GameModule
{
public:
	const char* Name() const override { return "Minimal"; }
	void RegisterTypes() override { ly::reflect::RegisterEngineTypes(); }
	std::shared_ptr<ly::World> CreateLevel(ly::Application& application,
		const nlohmann::json& scene, bool edit) override
	{
		return std::make_shared<MySceneWorld>(&application, scene, edit);
	}
};

if (auto* hooks = vega::QueryCapability<vega::IPlayHooks>(module))
	hooks->BeginPlay(world);
```

`MySceneWorld` is the game's world implementation; its constructor loads the
scene and records the edit flag, as Blank's `BlankWorld` does.

Optional `IPlayHooks::BeginPlay` runs after a play world's normal `BeginPlayInternal`;
`EndPlay` runs before replacement or shutdown. Neither runs for edit worlds.
`PlayerActor` returns an optional actor from the requested world. `DisplayInfo`
returns spawn marker metadata and a label, letting the editor render and pick
markers without knowing their C++ types. `ApplySceneSettings` and
`SceneBackdropState` bridge presentation settings.

`IContentReload::ReloadContent(const std::vector<std::filesystem::path>& paths)`
is optional. The editor calls it on the main thread for changed, valid JSON
files in the selected authoring asset root, including saves during Play. Paths
are absolute. A half-second poll checks timestamps and sizes; it excludes scenes,
project files, linked directories and `.vega`. An incomplete JSON write waits
for the next poll. Deleting a catalog retains its last valid runtime state.
Prefab caches are invalidated before the hook; nested prefab changes affect
future instances, while existing instances keep their realized state.

LightYears reloads abilities, effects, modules plus side features, weapons and
ships. It validates candidate catalog documents, logs each accepted/rejected
reload in Console, and retains replaced definition generations for the process
lifetime because active objects can hold definition pointers. New lookups,
spawns, grants and loadout rebuilds use the current catalogs; active actors,
already granted abilities and active effect specs keep their copied state.
Existing behaviors that resolve an effect or numeric setting by ID can observe
the new value on their next lookup. This is not a migration of a running world.

Restart Play to rebuild existing actors/loadouts consistently. Enemy definitions,
enemy behavior/combat profiles, damage-status balance, galaxy route data and
compiled behavior/registration changes still require restarting the editor.
Sky's separate presentation-root authoring keeps its module-specific reload path.

Continue registering reflection, content schemas, scene settings, GAS debugger
resolvers and Lua player resolvers through the existing registries from the
module. LightYears retains those registrations and its existing runtime setup.

## Editor extensions

Implement `IEditorProvider::RegisterEditorExtensions(vega::EditorExtensions&)` in the game's
editor adapter. Include `editor/EditorExtensions.h` there; standalone games
need no editor dependency.

- `AddPanel(callback)` draws a panel each frame. The callback owns its ImGui
  window and visibility state.
- `AddViewport(callback)` supplies the image rectangle in ImGui screen
  coordinates, texture size, hover state, context and command history. It
  supports overlays and handles; use history commands for authored changes.
- `AddHierarchyMenu(callback)` emits items in the Hierarchy menus.
- `AddScriptCommand(definition)` adds a verb to the same owned registry used by
  core and optional modules. A definition supplies its name, argument schema,
  help, mode requirement, decoder and execution handler. Duplicate names are
  rejected. Unregistered verbs remain errors.

For decoded token callbacks, `TokenScriptCommand` supplies the decoder and
execution adapter:

```cpp
extensions.AddScriptCommand(vega::TokenScriptCommand(
	"expect-widget", vega::Schema("name:text [count:uint]"),
	"Assert a widget by name and optional count.", vega::ScriptMode::Any,
	[](vega::EditorContext& context, vega::CommandHistory& history,
		const std::vector<std::string>& arguments, std::string& detail)
	{
		return CheckWidget(context, arguments, detail);
	}));
```

Schemas declare argument counts, types, optional arguments and repeated values.
`text` requires a nonempty decoded string; `string` permits an empty one. Both
accept quoted strings. `json` uses the original token spelling. Explicit count
sets express paired optional coordinates. Custom decoders preserve semantic
validation such as property-dependent types and JSON pointer rules. Handlers
keep their original mode checks and diagnostics. `help [command]` uses the live
registry, so it includes only installed modules and the current game's commands.
See [the generated command reference](editor-script-commands.md).

`VegaEditorScriptTests --write-doc` regenerates that reference from the core,
Sky, GAS and LightYears adapter declarations. `VegaEditorScript` checks it is
byte-for-byte in sync. New commands need no parser branch, operation enum entry
or central execution switch. Core declarations and handlers live in
`VegaEditor/src/commands`, grouped by area; module declarations live beside
their extensions.

Callbacks run on the editor thread. Panels and scripts receive the active scene
or prefab context; viewport and Hierarchy callbacks receive the scene context.
Captured state must live as long as the module. Extension arguments use the
existing tokenizer, including quotes, comments and composite JSON.

`LightYearsGame/src/editor/LightYearsEditorModule.cpp` registers
`expect-player-ship`, retaining its name, argument rules, world check and report
text. The editor core has no ship-specific script operation.

## Register another game

Implement the runtime contract in the game and an optional editor adapter.
The concrete entry point can be:

```cpp
#include <editor/EditorHost.h>
#include "MyEditorModule.h"

ly::Application* GetApplication()
{
	return vega::CreateEditorApplication(std::make_unique<MyEditorModule>());
}
```

After adding `VegaEditor`, call `vega_add_game_editor(Name MODULE Target
RUNTIME RuntimeTarget PROJECT projectFile ENTRY entrySource [CORE coreTarget])`
for each game (`CORE` defaults to `VegaEditorCore`; the helper lives in
`VegaEditor/cmake/VegaGameEditor.cmake`).
The helper links the game-independent `VegaEditorCore` and configures the host.
LightYears retains its integration selection in `LightYearsGame/editor.cmake`
and links the optional Sky and GAS editor extensions to the same core.
The concrete entry is `VegaEditor/app/LightYearsEditorMain.cpp`. See
[make a new game](new-game.md) and `Templates/Blank` for a complete second host.
Asset roots, fonts, build directories and package names come from the
[project file](project.md).

## Optional modules

Sky and GAS runtime/editor targets are independent optional modules. Sky's
route mapping and deterministic LightYears preview adapter live game-side;
the module edits opaque sky asset IDs. The editor core imports no game visual
headers and its source guard has no exceptions. See
[engine modules](engine-modules.md) for registration, layout and compatibility.

The game-module and export fixtures still exercise synthetic modules without
any current-game dependency.
