# Engine contexts

`ly::EngineContext` (`framework/EngineContext.h`) owns a host's services.
`Application` adopts `EngineContext::Current()` by default: the innermost bound
Scope, or the function-local static legacy context when no scope exists. This
preserves assets, defaults and registrations configured before construction.
For isolation, a host owns an EngineContext and passes it as the optional final
constructor argument. Application never owns or destroys either context. The
context must outlive the application and all its worlds, actors, components,
widgets and module callbacks.

| API | Ownership and initialization |
| --- | --- |
| `Types()` | Reflection descriptors, factories and constructor-component inventories; core engine types registered once per registry |
| `Prefabs()` | Project root, document cache and resolved prefab cache |
| `Assets()` | Texture, font, sound-buffer and shader caches, default texture, asset root |
| `Defaults()` | Project-owned engine font, button and light/shader paths |
| `Audio()` / `HasAudio()` / `ShutdownAudio()` | Lazy audio service, playback, volume and time-scale state |
| `SceneSettings()` | Scene setting descriptors, including the engine's bounds descriptor |
| `ContentSchemas()` | Content descriptors, tags, attributes and validators |
| `Scripts()` | Optional owning `shared_ptr<vega::script::ScriptRuntime>` slot; empty in engine-only hosts |
| `SceneSerialization()` | Component serializers and construction-property spawn hooks |
| `Style()` | Lazy project UI style |

Lua construction and destruction stay in VegaScript. The engine uses only a
forward declaration and an owning slot, and does not link Lua or sol2. Worlds
and script components must die before their context. Scripting is released
before audio and cached assets during context destruction.

## Application and object access

Existing application constructors continue to work and adopt the current context:

```cpp
class MyApplication : public ly::Application
{
public:
	MyApplication()
		: Application{ { 1280, 720 }, 32, "My game", sf::Style::Default }
	{
		GetEngineContext().Assets().SetAssetRootDirectory("assets/");
		GetEngineContext().Defaults().font = "fonts/game.ttf";
		RegisterGameModule(mModule, false);
	}
private:
	MyModule mModule;
};
```

For isolated services, pass a host-owned context as `Application{ size, bits, title,
style, &context }`. `Application::GetEngineContext()`,
`World::GetEngineContext()`, `Actor::GetEngineContext()` and
`Widget::GetEngineContext()` return the retained context directly. A world's
application takes precedence over an optional explicit context. Actor components
use `GetOwner().GetEngineContext()`. These paths perform pointer dereferences;
they do not search a service locator or consult the compatibility binding.
Avoid storing service pointers beyond the owning context's lifetime.
Include the concrete service header (for example `framework/AssetManager.h`
or `framework/EngineAssets.h`) where its methods or fields are used;
`EngineContext.h` forward-declares services to keep object headers lightweight.

The editor's `EditorContext::Engine()` follows its world, or the host-supplied
service pointer while a scene transition has no world. Scene/prefab staging
worlds receive this same context explicitly.
Standalone prefab libraries can use `PrefabLibrary{ context, root }`; their
validation worlds also use that context, independently of the compatibility binding.

Ownerless widgets accept an explicit context: `Button{ context, "Play" }`,
`TextWidget{ context, "Score" }`, `ImageWidget{ context, "ui/icon.png" }` and
`ValueGauge{ context }`. `SoundComponent` accepts a context after its existing
path/type/loop arguments. `Widget::AddChild` binds the parent's context during
legacy child construction. Old ownerless constructors capture the current
context once at construction; their subsequent resource accesses use that pointer.

## Games, reflection and scripting

New registration code can supply its registry explicitly:

```cpp
ly::reflect::RegisterEngineTypes(context.Types());
VEGA_REGISTER_IN(MyActor, context.Types());
ly::reflect::Class<MyComponent, ly::ActorComponent>{ "MyComponent", context.Types() }
	.Addable();
vega::script::RegisterScriptTypes(context);
vega::script::ScriptRuntime::Get(context).SetRoot("assets");
```

`VEGA_REGISTER` and the one-argument reflection builder still work for existing
modules. `Application::RegisterGameModule` binds its context while invoking the
unchanged `GameModule::RegisterTypes()` and optional setup callback.
Registration guards must check the destination registry, never a process-wide
`static bool`. Core engine registration now follows this rule. Repeated
registration preserves descriptor addresses and order; the reflection golden
file and all scene/prefab formats are unchanged.

Scene extension registrations accept a final explicit context. Existing game
calls default to the current binding. Both serialization and deserialization use
the target actor/world's hooks. Constructor-component inventories are cached in
their `TypeInfo`, preventing equal persistent names in separate registries from
sharing a constructor inventory.

The context installs the engine's scene settings into its own schema. A standalone
`SceneSettingsSchema` still starts empty for custom validation. Scene loading and
editor bounds tools pass the owning schema to `ReadSceneBounds(settings, schema)`;
the one-argument overload remains a compatibility wrapper.

VegaScript components retain their own interpreter reference. Roots, reload
settings, instruction/memory limits and input/player adapters belong to that
interpreter. Interleaving worlds from separate contexts does not select the
ambient interpreter. `GenerateLuaTypes(context, output, error)` generates
tooling for a specific registry.

## Tests and compatibility

Headless fixtures need no Application or window:

```cpp
ly::EngineContext context;
ly::EngineContext::Scope legacyCalls{ context };
ly::World world{ context };
context.Assets().SetAssetRootDirectory(buildTreeAssets + "/");
// Register this fixture's types and create actors here.
```

Use fixture lifetime instead of resetting registries/defaults between tests.
When a test configures assets before constructing an Application, the default
constructor adopts those services from the current context. For isolation, own
a fixture context and pass it explicitly; a newly constructed EngineContext
starts with empty project defaults and caches.
`EngineContext::Scope` temporarily binds the calling thread's legacy accessors
and restores the previous surviving binding on destruction. Bindings unlink in
constant time, including when applications close in construction order.
Application construction retains such a binding even when adopting the already
current context, for old derived member initializers. `Run()` and module setup
bind the host again at their entry
boundaries. Outside those boundaries, use explicit contexts to select the host
whose work is being performed. Each scope must be destroyed on its creating thread.

`TypeRegistry::Get()`, `PrefabLibrary::Get()`,
`AssetManager::GetAssetManager()` and `EngineAssets::Get()` are deprecated
compatibility shims forwarding to `EngineContext::Current()`. Audio, scene and
content schema accessors forward there too. Deprecation is documented rather
than a compiler warning so unchanged game branches remain buildable.

When no scope exists, `Current()` lazily creates one legacy context. This is
available for unchanged game/test hosts, including default Applications. New hosts
that need isolation own and pass their context explicitly.
Do not use `Current()` as a replacement for injection in object methods. The
compatibility selection is thread-local; services remain main-thread services
and a context is not safe for simultaneous mutation from multiple threads.

`LightYearsEngineContext` checks independent registries, schema/default/audio
state, identical prefab and texture paths under two roots, cache invalidation,
scene load/save with a different ambient context, and nested forwarding scopes.
It also checks default Application adoption of preconfigured assets and types in
both the legacy and scoped contexts, explicit Application isolation, and
out-of-order destruction of adopting applications.
`VegaScriptContext` checks separate interpreter slots, settings, roots and
interleaved script execution. Other engine/editor/script fixtures now establish
their own context where they use these services. Test assets stay in the build tree.

## Remaining migration

All eight migrated services have no production legacy static-accessor calls left
in LightYearsEngine, VegaEditor or VegaScript. The CTest guard
`VegaNoLegacyContextAccessors` enforces this with an explicit shim-definition
allow-list. Compatibility constructors and
registration wrappers still use `Current()` at entry boundaries.

ContentSchemaRegistry and SceneSettingsSchema are context-owned. ContentWorkspace
retains the host context supplied by the editor authoring panel, and scene
settings tools and commands use their Application or EditorContext. Ownerless
workspace/panel construction captures Current() once for compatibility.
Audio and ScriptRuntime are context-owned too; the engine application loop,
resource-owning objects and script components already use their owning context.
Their old setup functions remain wrappers for unchanged modules; SetScriptRoot
explicitly selects Current() only at that ownerless compatibility boundary.
Sky and GAS have no legacy calls to these eight accessors. GAS runtime/tool hosts
call `sas::RegisterContentSchemas(context)` explicitly. Editor hosts construct
`EditorExtensions{ context }`; its `Engine()` supplies the same context to GAS
through the unchanged `IEditorProvider::RegisterEditorExtensions` contract.
The context must outlive the extensions. Registration still preserves existing
game descriptors and fills only missing ability/effect schemas per context.

LightYearsGame and other optional/game modules retain their accessor calls and
registration guards. In particular LightYears' `RegisterGameTypes` still has a
process-wide guard: hosting two LightYears modules in one process requires
replacing that guard and migrating its game-owned catalogs. This step does not
claim isolation for game-owned globals. New modules should use registry-local
guards from the start.

TimerManager's global/game timers, ShaderManager's named shader cache,
diagnostics/profiler state, optional GAS/Sky runtime registries and game catalogs remain
separate migration work. Timers need explicit world/application ownership before
two live applications can independently tick and shut them down. ShaderManager's
default-loader accepts an explicit context, but its named cache remains global.
Keep these changes incremental; LightYearsGame files are unchanged. Existing
game hosts preserve their pre-construction setup through default adoption.

Build, test status differences, counts and measured frame costs are recorded in
[the validation report](engine-context-validation.md).
