# Actor Lua scripts

VegaScript is a C++17 static library using Lua 5.4.7 and sol2 3.5.0. GameApplication
calls `SetScriptRoot(getResourceDir())` and `RegisterScriptTypes()` after game
reflection registration. Attach a script with
`actor.AddComponent<vega::script::ScriptComponent>("scripts/spin.lua")`.
Paths must resolve inside the asset root and end in `.lua`. The library has no
dependency on LightYearsGame.

One lazily constructed process runtime owns the interpreter. Use it and all
components on the game/editor thread. Each component gets a separate environment,
`_G`, copies of math/string/table, and its own `self` and property table. Optional
callbacks include `on_start(self)` (`on_create` is its legacy fallback),
`on_update(self, dt)`, `on_destroy`, overlap begin/end, hit and message events.
Scripts may return a table containing these callbacks and property declarations.
Creation runs at component BeginPlay (immediately for a live actor); updates use
the actor's world tick and delta time, including pause/time-domain behavior.
EndPlay calls teardown once. Destroying the actor within a callback defers its
script teardown until that callback returns.

```lua
properties = { speed = 120, enabled = true, color = "#ff8800", offset = {x=0,y=0} }

function on_update(self, dt)
	if self.props.enabled then
		self.actor:set_rotation(self.actor:rotation() + self.props.speed * dt)
	end
end
```

Top-level string-keyed properties accept finite numbers (C++ float), booleans,
strings and `{x=number,y=number}` vectors. Unsupported entries are ignored.
`self.props` contains the effective values before on_create. Editor overrides
survive reload and win over defaults of the same type. Removed or retyped
properties retain dormant overrides, reapplied if a compatible declaration
returns. Undo of a first edit removes the override, restoring the current default.
`properties` is the declaration; scripts should read values through `self.props`.
Script writes to `self.props` are transient and are not editor overrides.

| Lua API | Behavior |
| --- | --- |
| `self.actor:id()`, `:type()`, `:is_valid()` | Unique ID, most-derived registered reflection name, live status |
| `:location()`, `:set_location(x,y)` | Position vector userdata with `x,y` / world position setter |
| `:rotation()`, `:set_rotation(degrees)` | Degrees |
| `:velocity()`, `:set_velocity(x,y)` | Velocity vector userdata with `x,y` / setter |
| `:destroy()` | Destroy the owning actor |
| `:get(name)`, `:set(name,value)` | Most-derived reflected properties, including inherited descriptors |
| `:component(typeName)` | First reflected component matching that name or a registered base name, or nil |
| `component:get(name)`, `component:set(name,value)` | Reflected component properties |
| `log(...)`, `print(...)` | Console Lua info log with path and line |
| `time()` | Wall-clock seconds since component creation; does not reset on reload |
| `input.key_down(name)` | Held key; false without world input focus or for unknown names |

The level API also provides `world.find/find_all_by_type/find_in_radius/spawn/player`,
weak-handle name/lifetime/distance/facing/messages, owned cancelable timers,
input edges/viewport mouse coordinates, and `vec` arithmetic. See
[Lua API](../docs/lua-api.md) for the reference, examples and generated Lua
Language Server types (**Scripts > Regenerate Lua Types** in the editor).

Input names: `A` through `Z`, `Space`, `Left`, `Right`, `Up`, `Down`.
Reflected values support bool, number, string and Vector2 tables. Integer/enum
setters require integral in-range numbers and retain descriptor validation.
Read-only, unknown and unsupported properties return false on set; missing
properties return nil on get. Color descriptors are unsupported (get returns
nil). All actor/component handles hold weak actor references; destroyed/expired
handles return false for is_valid/set, nil for get/component, zero/empty defaults
for transform/ID/type reads, and ignore transform setters/destroy.

Only selected base functions and math/string/table libraries are exposed. There
is no io, os, package, debug, require, load, loadfile, dofile, getmetatable or
setmetatable. Scripts are loaded by C++ as text-only chunks. Lua cannot open files
or launch processes through this API. Protected chunks/callbacks have an instruction
budget (10,000,000 by default) and the interpreter has an allocation cap (256 MiB
by default), configurable through ScriptRuntime. Native scene construction is
still trusted project work. The math random generator is interpreter-wide.

Hot reload defaults on outside Release. `ScriptRuntime::Get().SetHotReload(true)`
also enables it in Release; false disables polling. Components check last-write
time at most once per 0.5 seconds of wall time, during their normal ticks. Paused
or inactive components therefore defer polling until ticking resumes. Reload
calls old on_destroy, executes a fresh environment, reapplies overrides and calls
new on_create. The Inspector Reload button explicitly retries immediately.

Chunks and callbacks use protected Lua calls. Errors are logged with the asset
path and Lua's source file/line/traceback, stored in LastError, and suspend all
script callbacks until a changed file or explicit reload retries. A failing old
on_destroy aborts that reload and waits for another edit/retry. File/path failures
have a path and diagnostic but no source line. Editing after a failure creates a
fresh environment, preserving overrides. Timers and queued messages are canceled
on reload/destroy. C++ exceptions at lifecycle boundaries
are contained as errors as well.

The Inspector lists `.lua` files recursively under the runtime asset root's
`scripts/`, offers reload/error display, and edits properties with undo/redo and
drag merging. Scripts and overrides persist through scenes and prefabs. There is
no Lua module loader or coroutine API. Scripts do not get attached to gameplay actors by
default; `spin.lua` and `bob.lua` are opt-in examples.

Build with `cmd /c tools\vega-build.cmd`; run `ctest --test-dir build`.
VegaScriptComponent runs headlessly, creates temporary assets in the build tree,
and covers lifecycle, isolation, properties, hot reload, error recovery, sandbox,
handles and reflection. Sol2 appears in one `.cpp` only; public headers use pimpl
and never include sol headers.
