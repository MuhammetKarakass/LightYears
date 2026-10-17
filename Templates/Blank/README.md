# Blank template

A small Vega project that does not depend on LightYears, GAS or Sky. It is meant to
be copied and renamed; see [make a new game](../../docs/new-game.md) for the steps.

When adding optional GAS support, link the runtime/editor module targets as
described in [engine modules](../../docs/engine-modules.md). In runtime setup use
`sas::RegisterContentSchemas(application.GetEngineContext())`. An editor adapter's
`IEditorProvider::RegisterEditorExtensions(vega::EditorExtensions& extensions)`
can call `sas::RegisterEditorExtension(extensions)`; the editor supplies its host
context through `extensions.Engine()`. Tool/test hosts construct
`vega::EditorExtensions{ context }` explicitly and keep that context alive.

## Layout

| Path | Purpose |
| --- | --- |
| `Source/BlankGameModule.*` | The game module: `BlankPlayer` actor, `BlankMovement` component, registration, `CreateLevel`. |
| `Source/BlankMain.cpp`, `Source/BlankEditorMain.cpp` | Runtime and editor entry points. |
| `Assets/vega.project.json` | Project file (name, startup scene, window, editor and runtime paths). |
| `Assets/scenes/` | Sample scenes (below). `main.scene.json` is the startup scene. |
| `Assets/scripts/` | Lua samples (below). |
| `Assets/prefabs/Spark.prefab.json` | Prefab used by `world.spawn` in `lamp.lua`. |
| `Assets/audio/beep.wav` | Short generated tone for the audio sample. Replace it with your own sound. |
| `Assets/textures/tile.png` | Original 32 x 32 tile; `Tools/GenerateTexture.ps1` recreates it. |
| `Tests/` | CTest drivers for the template. They use `main.scene.json` and temporary copies of the assets. |

Everything in the samples uses only what the template already registers:
`BlankPlayer` (properties `Size`, `Target`, `Patrol`), `BlankMovement` (`Speed`) and the
engine's `SpriteComponent` and `CollisionComponent`. No extra C++ was added.

## Sample scenes

| Scene | Shows | Scripts |
| --- | --- | --- |
| `main.scene.json` | Minimal scene: a player, an actor reference (`Target`) and a Vec2 list (`Patrol`). | `spin.lua`, `patrol.lua` |
| `hierarchy.scene.json` | Parent/child/grandchild actors with the `parent` field. Child `Location` is relative to its parent. | `orbit.lua` |
| `trigger.scene.json` | A trigger pad with a `CollisionComponent` (added with `"added": true`), a `Target` reference to a lamp, overlap events, messages, `world.spawn` of a prefab and audio. Move the Player with WASD or the arrow keys. | `trigger.lua`, `lamp.lua`, `timers.lua` (through the spawned `Spark` prefab) |
| `patrol.scene.json` | A looping patrol over a `Patrol` Vec2 list and a message to a referenced counter after every lap. | `patrol_loop.lua`, `lap_counter.lua` |

## Lua samples

| Script | Demonstrates (see [Lua API](../../docs/lua-api.md)) |
| --- | --- |
| `spin.lua`, `patrol.lua` | Original samples used by the template tests. Do not change their behaviour without updating `Tests/`. |
| `timers.lua` | `timer.every`, `timer.after`, `:cancel()`, `:is_active()`, `self.props`. |
| `orbit.lua` | `actor:children()`, `actor:parent()`, `actor:detach()`, `actor:attach_to()`, `on_update(self, dt)`. |
| `trigger.lua` | `on_overlap_begin` / `on_overlap_end`, `world.player()`, reflected `get("Target")`, `actor:send()`, `audio.play`. |
| `lamp.lua` | `on_message`, `world.spawn(prefab, x, y, props)`, `:destroy()`, `math` and `actor:location()`. |
| `patrol_loop.lua` | Vec2 list from `actor:get("Patrol")`, vector maths, sending messages to a referenced actor. |
| `lap_counter.lua` | `on_message` with table data. |

## Trying them

* Editor: build `BlankEditor`, use **File > Open** and pick a scene under `scenes/`, then press Play.
  Or set a sample as the startup scene in **File > Project Settings...**.
* Runtime: `BlankGame.exe --scene scenes/trigger.scene.json` (the path is relative to the asset root).
* Watch **Console** for the `log(...)` output of the scripts.

These samples were written without a build in the authoring environment. Open each one once
in the editor after copying the template; a scene that fails to load reports its warnings in Console.
