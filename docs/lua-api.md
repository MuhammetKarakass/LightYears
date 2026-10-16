# Lua level scripting

Lua drives encounters, triggers, timers and simple behaviors. Core gameplay stays
in C++. Attach an asset-relative `.lua` file through the Inspector or
`actor.AddComponent<vega::script::ScriptComponent>("scripts/trigger.lua")`.
Paths must remain inside the selected asset root. Play starts scripts; Stop
restores the authored scene, including transforms and the absence of runtime spawns.

## Lifecycle and events

A chunk may return a script table, or declare functions in its private environment:

```lua
---@type VegaScript
local script = { properties = { speed = 90 } }
function script:on_start()
	log("Started", self.actor:name())
end
function script:on_update(dt)
	self.actor:set_rotation(self.actor:rotation() + self.props.speed * dt)
end
return script
```

Callbacks receive `self`, containing `actor` and the effective `props`. Optional
callbacks are `on_start(self)`, `on_update(self, dt)`, `on_destroy(self)`,
`on_overlap_begin(self, other)`, `on_overlap_end(self, other)`,
`on_hit(self, other)` and `on_message(self, name, data)`. Existing
`on_create(self)` is a fallback when `on_start` is absent. Table members take
precedence over environment declarations. Callbacks can also use Lua's colon
syntax, as above. Property declarations accept finite numbers, booleans, strings
and vectors; editor overrides survive reload.

Overlap and hit events come from the world's physical contact dispatch, only
for actors with CollisionComponent and compatible layers/masks. There is no
script pair polling. Multiple shape contacts count as one actor overlap, ending
after the final contact separates. Destroying the other actor ends its overlap.
Events reach scripts even if a C++ Actor override omits its base call. In this
engine physics reports touches without solver impulses: `on_hit` means first
physical touch, once per overlap, rather than an impact-force threshold.

Destroying an actor within a callback finishes that callback before invoking
`on_destroy` once. Hot reload tears down the old instance, cancels its timers and
messages, creates a fresh environment, reapplies overrides and starts it again.
Errors suspend callbacks until reload and appear in Console's Lua channel with
the script path, source line and traceback. `print(...)` and `log(...)` retain
the calling source location too.

## World and handles

| API | Result |
| --- | --- |
| `world.find(name)` | First live actor with that scene entity name, or nil; includes pending spawns. |
| `world.find_all_by_type(type)` | Array of live handles, including reflected subclasses and pending spawns. |
| `world.find_in_radius(x, y, r[, type])` | Array of spatially registered actors whose origins lie within the radius; optional reflected type filter. |
| `world.spawn(type_or_prefab_path, x, y[, props])` | New actor handle with scene identity. Type must be reflected and Spawnable; prefab path is asset-relative and ends in `.prefab.json`. |
| `world.player()` | The game's current player actor, or nil. |

Radius queries use `World::ForEachActorInBounds`, combining the existing Box2D
and manual spatial indexes, then filter distance. They never scan all actors.
Radius must be finite, nonnegative and at most 1,000,000 world units. Pending
spawns become spatially discoverable when the engine registers them. Name/type
queries use the world's existing actor enumeration; avoid calling them every
frame in large worlds. Cache weak handles in `on_start` instead.

Spawn uses the scene loader's reflection factories and PrefabLibrary resolution,
including construction-specific game factories, prefab components and scripts.
`props` contains reflected **actor** property names, for example
`{ Rotation = 45 }`. Position arguments win over a `Location` prop. Invalid
types, paths or properties raise a protected Lua error; failed partial spawns
are destroyed. Successful spawns appear in the editor Hierarchy during Play.
For component defaults, author a prefab. Spawning and queries allocate on demand;
they are encounter tools, rather than a projectile spawning system.

The game installs `ScriptRuntime::SetPlayerResolver`; the scripting library has
no game dependency. The runtime checks the returned actor's world and lifetime.
The shipped game resolves PlayerManager's current spaceship.

| Actor method | Behavior |
| --- | --- |
| `:id()`, `:name()`, `:type()` | Runtime ID, scene name (empty without identity), most-derived reflected name. |
| `:is_alive()`, `:is_valid()` | True while the weak target exists and is not pending destruction. |
| `:location()`, `:velocity()` | Vector values with `x` and `y`. |
| `:set_location(x,y)`, `:set_velocity(x,y)` | Finite world-space values. |
| `:rotation()`, `:set_rotation(degrees)` | Actor rotation in degrees. |
| `:forward()`, `:look_at(x,y)` | Forward vector and facing setter; zero rotation faces up, matching C++ actors. Looking at the current position leaves rotation unchanged. |
| `:distance_to(other)` | Origin distance; infinity if either actor is dead. |
| `:destroy()` | Destroy once; subsequent operations observe a dead handle. |
| `:get(name)`, `:set(name,value)` | Reflection getter/setter; set returns success and error text. |
| `:component(type)` | First matching reflected component (including a registered base), or nil. |
| `:send(name[, data])` | Queue a copied message on every running script on the target; returns whether any accepted it. |

Components expose `:get(name)` and `:set(name,value)`. Property names are the
registered display names, including spaces, such as `"Max Health"`. Numbers for
integer/enum/flag setters must be integral and in range. Missing reads return nil;
invalid writes return false and a reason. Read-only setters refuse writes.
Colors remain unsupported. Dead handles return empty/zero defaults for basic
reads, nil for property/component reads, and ignore transform setters/destroy.
There is no actor gameplay tag system, so no tag API is added.

Message data supports nil, booleans, finite numbers, strings and nested tables
with string keys. Data is copied, preventing receivers from mutating a sender's
tables or another receiver's delivery. Functions, actor handles, numeric table
keys and cycles are rejected. Bounds: 256 values, 8 nesting levels, 4,096 bytes
per string, 256 bytes per key/name and 64 KiB per complete payload. Each script queues at most 128 messages;
`send` returns false for dead, stopped or full recipients. Delivery uses the
recipient's next script tick, so pause/inactivity defer delivery. Work queued
during a message callback waits for a later tick, preventing recursive sends.

## Timers, input and vectors

```lua
local once = timer.after(2, function(self) log("two seconds elapsed") end)
local repeating = timer.every(0.1, function(self)
	self.actor:set_rotation(self.actor:rotation() + 5)
end)
once:cancel()
assert(not once:is_active())
```

Timers belong to the script instance. Destroy/reload cancels them even when
another callback is in progress. They use actor tick delta time and therefore
respect pause, activity and simulation time domains. `after` accepts zero and
fires on a subsequent tick; `every` requires a positive interval. Repeating
timers run at most once per tick and skip missed firings after a long frame.
New timers wait until a subsequent timer pass. There are at most 1,024 pending
timers per instance. Callback errors suspend the script as for `on_update`.

`input.key_down(key)`, `input.pressed(key)` and `input.released(key)` accept
`A`–`Z`, `Space`, `Left`, `Right`, `Up`, `Down`. Unknown keys return false.
Input requires the world's viewport focus. Edges are sampled once per script
tick for watched keys, remain stable across repeated calls within that tick,
and pause with the script. Start watching in `on_start` if the first transition
matters. At most 64 names can be watched. `input.mouse_world()` uses the same
World/RenderViewport mapping as game aiming, including the editor viewport and
camera view. Without a viewport it returns the world view center.

`vec(x,y)` creates a small vector userdata. It supports `a+b`, `a-b`, `a*scalar`,
`scalar*a`, `a:length()`, `a:normalize()` and `a:dot(b)`. Normalizing zero returns
zero. `x` and `y` are writable. Reflection and transform getters return these
vectors; setters also accept ordinary `{x=...,y=...}` tables. Tick dispatch,
timer traversal and idle message queues create no Lua tables per frame.

## Examples

These exact fixtures are under `VegaScript/examples/` and are copied into a
disposable build-tree asset root by `VegaEditorLuaApi`.

Attach this trigger to an actor with a visible SpriteComponent (or a C++ shape),
CollisionComponent, physics enabled, a Powerup layer and a Player mask. The
shipped player's mask accepts Powerup; it does not accept Environment:

```lua
local entered = false
function on_overlap_begin(self, other)
	local player = world.player()
	if entered or not player or other:id() ~= player:id() then return end
	entered = true
	for index = 1, 3 do
		world.spawn("DummyEnemy", 1400 + index * 100, 100)
	end
	local receiver = world.find("Receiver")
	if receiver then receiver:send("wave_started", { count = 3 }) end
	log("Wave spawned: 3 DummyEnemy")
end
```

A timer-driven spinning hazard (`spinning-hazard.lua`):

```lua
properties = { interval = 0.05, step = 10 }
function on_start(self)
	timer.every(self.props.interval, function()
		self.actor:set_rotation(self.actor:rotation() + self.props.step)
	end)
end
```

Name another actor **Receiver** and attach `message-receiver.lua`:

```lua
function on_message(self, name, data)
	if name == "wave_started" then
		self.actor:set_rotation(data.count)
		log("Receiver: wave_started", data.count)
	end
end
```

For prefab spawning, replace the type with `"prefabs/Enemy.prefab.json"`.

## Autocomplete

Opening the editor generates `<project asset root>/.vega/vega.lua`. Use
**Scripts > Regenerate Lua Types** after changing registered C++ types.
The file contains the complete API, callback fields, reflected classes,
property-name literal unions (including inherited properties), and typed getter
and writable setter overloads. It is a tooling file, never a runtime script.
Automation using source assets writes its generated file into the build tree.

Install the VS Code Lua extension (Lua Language Server), open your asset root
as the workspace and add `.vscode/settings.json`:

```json
{
	"Lua.runtime.version": "Lua 5.4",
	"Lua.workspace.library": ["${workspaceFolder}/.vega"],
	"Lua.workspace.checkThirdParty": false
}
```

If your workspace is the repository root, use
`${workspaceFolder}/LightYearsGame/assets/.vega` instead. Do not use `require`
to load the annotations. Use `---@param self VegaScript` on environment
callbacks, or annotate returned tables as `VegaScript`. For a known component,
`---@type Vega_HealthComponent` enables exact `"Max Health"` getter/setter
overloads; untyped handles still complete the union of registered names.

## Sandbox and verification

Each instance retains its own environment and copies of math/string/table.
There is no io, os, package, require, debug, load, loadfile, dofile or metatable
access. C++ accepts text chunks only. Instruction and interpreter-wide memory
limits remain enabled; callbacks, timers and messages use protected calls and
report errors to Console. These controls bound Lua execution/allocation, not
the cost of deliberately spawning large C++ scenes; level scripts remain
trusted project code. Hot reload polls at most every 0.5 seconds while ticking.

Run `cmd /c tools\vega-build.cmd`, then
`ctest --test-dir build -j 8 --timeout 300`. New groups are `VegaScriptWorld`,
`VegaScriptEvents`, `VegaScriptTimers`, `VegaScriptMessages`,
`VegaScriptInputVectors`, `VegaScriptLuaTypes`, `VegaScriptExamplesCost` and
`VegaEditorLuaApi`. Existing `VegaScriptComponent` retains sandbox, memory,
instruction, hot-reload, error and reflection coverage. The editor test verifies
three DummyEnemy spawns, message delivery, generated game property annotations
and Stop restoring the four authored entities and their transforms. All fixture
writes use build-tree roots.

The editor test captures the same seven-entity realized scene with and without
the three example scripts to `build/Saved/lua-api-with.json` and
`lua-api-without.json`. These contain frame-limited frame durations and measured
World Tick/Lua Script Tick scopes. `VegaScriptExamplesCost` additionally prints
alternating-batch median headless world tick costs without rendering or a frame
limiter. Performance results and build/test logs accompany the implementation
report; do not interpret Debug timing as a shipping-budget guarantee.

Verified 2026-10-16 in Debug: build exits 0. Full CTest passes 103/127 in
142.13 seconds; the 24 failures exactly match `build/scene-baseline-ctest.log`
(23 known gameplay failures, including RareZone's timeout, plus the sandbox's
GetCursorPos failure). All 46 engine/reflection/scene/project/prefab/diagnostics/
scripting/editor group tests pass, including all eight new tests.

An isolated run of the two timing tests passes 2/2. Median measurements:

| Measurement | Without scripts | With scripts |
| --- | ---: | ---: |
| Editor frame, with limiter | 16.5687 ms | 16.5536 ms |
| Editor World Tick | 0.1721 ms | 0.2206 ms |
| Editor Lua Script Tick, all three instances | 0 ms | 0.0147 ms |
| Reduced headless seven-actor World Tick | 4.3421 us | 16.5406 us |

The editor windows contain 92 baseline and 93 script frames. Total frame time
is dominated by the limiter and varies independently of the small script cost.
Headless measurement alternates nine batches of 3,000 ticks after warmup, with
hot reload disabled; its median added cost is 12.1985 us per tick. The editor
keeps normal hot reload enabled. These are separate fixtures: the editor uses
the real DummyEnemy wave; the headless fixture uses lightweight reflected actors
and exercises trigger spawning, a spinning timer and message delivery.

Logs: `build/lua-api-build-final.log`, `build/lua-api-ctest-final.log`, and
`build/lua-api-performance.log`. The editor automation report is
`build/VegaEditor/scripts/lua-api.vegascript.report.txt`.

The profiler retains previous frames when recording resumes. For the comparison,
select only frames in the `with` capture whose IDs exceed the last ID in the
`without` capture. Run the timing tests alone (`ctest --test-dir build -R
"^(VegaScriptExamplesCost|VegaEditorLuaApi)$" -j 1 -V`) to avoid parallel test load.

Implementation files:

* Engine contact bridge: `LightYearsEngine/include/framework/ActorComponent.h`,
  `include/framework/CollisionComponent.h`, `src/framework/CollisionComponent.cpp`
  and `src/framework/PhysicsScene.cpp`.
* Runtime: `VegaScript/include/script/ScriptComponent.h`, `ScriptRuntime.h`,
  `src/ScriptComponent.cpp`, `src/LuaTypes.cpp`, `CMakeLists.txt` and `README.md`.
* Game player hook: `LightYearsGame/src/gameFramework/GameApplication.cpp`.
* Editor: `VegaEditor/include/editor/EditorApplication.h`,
  `src/EditorApplication.cpp`, `CMakeLists.txt`, `tests/RunLuaApiTest.cmake.in`
  and `tests/scripts/lua-api.vegascript`.
* Coverage/examples: `VegaScript/tests/LuaApiTests.cpp` and
  `VegaScript/examples/{trigger,spinning-hazard,message-receiver}.lua`.
* Reference and setup: this document.

## Actor attachment

`:parent()` returns a live parent handle or nil; `:children()` returns an array of
immediate live child handles. `:attach_to(other[, keep_world])` returns success and
defaults to keeping world values. `:detach()` preserves world values. World transform
setters still accept world coordinates. Destroy cascades through descendants, and
`world.spawn` of a hierarchy prefab creates every child and returns the root.
Dynamic physical children are refused; see [hierarchy rules](hierarchy.md).
