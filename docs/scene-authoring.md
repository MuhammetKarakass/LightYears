# Scene data

Dirty scenes and prefabs are periodically captured under `.vega/Autosave`,
including authored scene state during Play. **File > Recover Autosaves...** opens
recovered state as a dirty buffer for review before Save. See
[autosave and recovery](autosave.md) for intervals, crash detection and automation.

Reflected actor connections use GUID objects, including null as `{"guid":""}`.
Lists use JSON arrays with the ordinary scalar encoding for each element (Vec2
`[x,y]`, Color `[r,g,b,a]`, enum label, or GUID object).

```json
"properties": {"Target":{"guid":"door"},"Patrol":[[300,230],[340,230],[300,270]]}
```

Forward references resolve after loading all entities. Missing targets warn and
remain saveable; deleting a target makes runtime access return null. Duplicate
and paste remap links within the copied set, including list elements and children,
and preserve outside connections. The Inspector picker searches and filters by
required type. Pick in viewport connects a clicked actor, Select target changes
selection, and dashed lines show connections. Vec2 lists display draggable
world-space points in Edit mode. Add/remove/reorder each create one undo step;
a point drag creates one undo step. Unequal lists in multi-selection show one em dash.

Templates/Blank demonstrates BlankPlayer's Target and Patrol fields. Left Tile
follows its Lua patrol and changes Right Tile's Size after the last point.
`VegaEditorRichProperties` authors the behavior through script mode, plays it,
saves/reopens it, and checks prefab/copy connections in a build-tree asset root.

See [reflection](reflection.md) for the runtime model, registration macros, and a
walkthrough from a new C++ class to Inspector, scene, prefab, and Lua properties.

Scene entities may also use an asset-relative `"prefab"` in place of `"type"`.
The loader resolves prefab inheritance before applying instance overrides; saves
retain only differences plus GUID, name, and Location. Missing prefab instances
produce warnings, spawn nothing, and preserve their authored JSON on save.
See [prefabs](prefabs.md) for the format, variants, editor workflow, and API.

`LightYearsGame/assets/scenes/sandbox.scene.json` is a runnable version 1
`vega.scene` example. It contains one PlayerStart, three DummyEnemy actors,
two Asteroid actors, and a SpriteActor with `scripts/spin.lua` and a `speed`
override of 45 degrees per second.

```json
{
	"format": "vega.scene",
	"version": 1,
	"settings": { "routeStop": 0, "playerShip": "Ship.Player.Fighter.Basic" },
	"entities": [{
		"guid": "a0000000000000000000000000000007",
		"name": "Spinning star",
		"type": "SpriteActor",
		"properties": { "Location": [960, 950] },
		"components": [{
			"type": "SpriteComponent",
			"properties": {
				"Texture": "SpaceShooterRedux/PNG/Effects/star3.png",
				"Scale": [3, 3]
			}
		}],
		"scripts": [{ "path": "scripts/spin.lua", "properties": { "speed": 45 } }]
	}]
}
```

Run with `--scene scenes/sandbox.scene.json`; relative paths resolve against the
asset root, and absolute paths are accepted. Automated runs must also supply
`--visual-capture <png> --capture-frames 120` (or use the editor's script runner).

## Engine API

* `reflect::Class<T>::Spawnable()` uses `World::SpawnActor<T>()` when `T` accepts
  `World*`. `Spawnable(factory)` accepts a custom `shared_ptr<Actor>(World&)`
  factory. `TypeInfo::spawnable` and `TypeInfo::create` expose the opt-in.
* `SceneEntityComponent` stores the stable 32-digit random hex GUID, editable
  name, and construction-only `spawnData`. Reflection exposes writable `Name`
  and read-only `GUID`. Non-scene actors need no identity component.
* `SceneSerializer::Save(World&, settings)` returns JSON for live scene actors,
  including pending actors and excluding actors pending destruction. Writable
  properties are reflected; runtime IDs and read-only values are omitted.
* `SceneSerializer::SaveEntity(Actor&)` returns the same representation for one
  entity, or null for a destroyed/non-scene actor. Create, duplicate and delete
  use this path so editing one entity never serializes the rest of the scene.
* `SceneSerializer::Load(World&, json)` appends pending entities and returns
  `SceneLoadResult { spawned, warnings }`. It never ticks the world. Missing
  GUIDs are generated; duplicate GUIDs are rejected, including collisions with
  entities already in the destination world. Invalid entities/properties warn
  and do not stop subsequent entities/properties from loading.
* `RegisterComponentSerializer(typeName, ComponentSerializer)` lets optional
  modules supply their own array serialization. VegaScript registers
  `ScriptComponent` in `scripts`, so the engine has no scripting dependency.
* `RegisterSpawnFactory(typeName, factory)` supplies property-aware constructor
  arguments for a type that has already opted into reflection spawning.
* `Application::LoadWorld<T>(args...)` forwards constructor arguments and
  preserves `LoadWorld<T>()`.
* `SpriteComponent::GetTexturePath()`, `GetScale()`, and `SetScale()` expose
  authored sprite data. Scale can be set before the texture loads.

Vectors encode as `[x,y]`, colors as `[r,g,b,a]`, and enums as their reflected
label strings. Booleans, integers, floats, and strings retain their JSON types.
Collision flags save as arrays of bit labels; see [project settings and flags](project.md).
Saving emits complete writable property sets; hand-written sparse inputs gain
defaults on their first save. Complete supported scenes round-trip exactly,
including the shipped sandbox. Settings include engine bounds and game fields and must be passed back
to Save by its caller.

Component composition remains in version 1. Entries without `added` update
constructor components, preserving old scene behavior. An entry with `"added": true`
creates an instance through its reflected addable factory. Entity-level
`"removed": ["TypeName", ...]` removes constructor components before any properties
are applied. Unknown/non-addable additions and unknown, required, non-removable,
or unavailable removals warn and are skipped. All added components are created
before reflected properties are applied. Scene identity stays in `guid` and `name`.

`SaveEntity` compares component counts against a freshly spawned actor in an
unticked scratch `World{nullptr}`, cached once per actor type. Extra occurrences
get `added`; missing occurrences produce `removed` (repeated names remove repeated
occurrences). Component array order supplies each type's zero-based occurrence;
writable properties preserve the surviving instances' state. Saving emits
matching components in constructor order, then additions, so removing and
re-adding a default type cannot change JSON order on reload. Empty `removed`
arrays and false `added` markers are omitted.

```json
{
	"guid": "a0000000000000000000000000000007",
	"name": "Spinning star",
	"type": "SpriteActor",
	"removed": ["SpriteComponent"],
	"components": [{
		"type": "HealthComponent",
		"added": true,
		"properties": { "Active": true, "Max Health": 42 }
	}]
}
```

Reflection builders expose `.Addable()` for an `Actor&` constructor or
`.Addable(factory)` for explicit defaults. Addable types are removable by default;
`.Removable(false)` can opt out. Other types default to non-removable.
`Class<T>::RequiresComponent("HealthComponent")` declares an inherited actor-type
requirement, enforced by raw actor removal, scene loading, and editor commands.
Requirements conservatively protect every occurrence of that component type.

Sprite, Light, Collision, Health, Shield, and Energy are addable. Health starts
at 100/100; Shield and Energy use their zero-capacity defaults. Collision needs
only an owner and starts disabled; enabling physics still requires suitable
actor geometry. Light starts empty; tagged light editing is outside this scalar
property editor. Movement requires a ship definition; PlayerMovement assumes a
PlayerSpaceShip owner. Both remain constructor-owned and non-removable. Ships
require Health, Shield, Energy, and Movement; PlayerSpaceShip also requires
PlayerMovement. Asteroid requires Health. Scripts use the separate `scripts`
array and Add Script flow; their removal uses the same undoable component command.

## Game types

All four scene types inherit the writable Actor properties `Location`,
`Rotation`, `RenderLayer`, `Render Enabled`, and `Tick When Paused`.

| Type | Additional authored data |
| --- | --- |
| Asteroid | Writable `Velocity`; constructor defaults: velocity `[0,100]`, health 50, contact damage 20. Health maximum is writable through HealthComponent. Contact damage has no setter and stays at its constructor default. |
| DummyEnemy | Writable string `Ship`, default `Ship.Enemy.RangeKeeper.Basic`. Construction only: changing it stores the requested catalog ID in SceneEntityComponent::spawnData; reload to rebuild the ship. |
| PlayerStart | Writable string `Ship`, default `Ship.Player.Fighter.Basic`. No gameplay sprite or automatic player spawn. `IsEditorOnlyMarker()` identifies the editor-only triangle/Ship-label overlay. |
| SpriteActor | Constructs a removable SpriteComponent, with editable `Texture` and vector `Scale`. |

Reflected components also expose writable inherited `Active`. Explicit component
properties override constructor defaults, including a dummy's selected ship
texture. Clear those overrides when the new ship's defaults should be used.
Asteroids retain existing gameplay behavior: BeginPlay randomizes their speed
slightly and ticking rotates them and removes them outside the view. Scene
saving records current state rather than a frozen copy of initial authoring data.

`SceneLevel` inherits ArenaLevel's player/respawn, HUD controllers, camera, and
boundary setup. ArenaTestLevel and SceneLevel share the extracted arena and
respawn defaults in ArenaSandboxSetup; ArenaTestLevel's dummies, encounters, and
music behavior are unchanged. PlayerStart takes precedence over the
`settings.playerShip` catalog ID; the first marker chooses the spawn location
and ship. Without a marker, the arena center `[3000,1500]` is used. The game
application applies `settings.routeStop` to its existing backdrop pipeline
through SceneLevel's optional settings callback.

## World Settings

Per-stop procedural backdrop authoring is available in **View > Sky**. Its preview
picker leaves the scene clean; **Use in scene** applies the chosen `routeStop`
through World Settings. See [Sky editor](sky-editor.md) for forms, viewport
handles, presets, comparison, saving and scripted validation.

Open **View > World Settings** or the Hierarchy header button to edit game-owned
scene settings. Categories, labels, tooltips, typed widgets and validation come
from the engine's game-agnostic `ly::SceneSettingsSchema` API
(`framework/scene/SceneSettings.h`). Games register `SceneSettingField` descriptors
with a key, label, category, type, default, optional bounds and options callback.
Supported types are bool, int, float, string, enum, catalog ID, vec2, color
(RGBA bytes) and rect (`SceneSettingType::Rect`). Registration rejects invalid defaults; `Validate` and `Set` reject
invalid edits. `Value` reads an effective default without modifying authored JSON.

```cpp
ly::SceneSettingField exposure;
exposure.key = "exposure";
exposure.label = "Exposure";
exposure.category = "Lighting";
exposure.type = ly::SceneSettingType::Float;
exposure.defaultValue = 1.0;
exposure.min = 0.0;
exposure.max = 4.0;
ly::SceneSettingsSchema::Get().Register(std::move(exposure));
```

LightYears registers `routeStop` from the complete galaxy route (including generated
stops and the centre tour) and `playerShip` from the player ship catalog, in
`level/GameSceneSettings.cpp`, called after content startup. Route changes apply
immediately through SceneLevel's settings callback, including undo and reset.
Player ship changes apply on Play; PlayerStart retains precedence over this
world default. The panel is read-only during Play and prefab editing.

Each edit/reset is undoable and marks the scene dirty. Missing defaults are never
materialized by viewing, saving or playing; Reset removes an override. Existing
explicit defaults stay in old files until reset. Unknown keys are shown read-only
under **Other** and retained verbatim, including structured JSON. Invalid legacy
values remain visible with warnings and can be corrected or reset. Panel rows,
validation and option previews are cached by scene/schema revision.

Editor scripts support `set-setting "routeStop" 1`,
`expect-setting "playerShip" "Ship.Player.Breacher.Basic"` and
`reset-setting "routeStop"`. Values use JSON syntax; vectors/colors use compact
arrays. `VegaEditorSceneSettings` verifies live backdrop state, undo/reset,
player spawning, and build-tree save/reopen with unknown settings preserved.

## Editor authoring

VegaEditor opens `scenes/sandbox.scene.json` by default, or
`--editor-scene <path>`. A missing startup scene produces an empty Edit world.
SceneLevel's optional `editMode` constructor flag defaults to false for the game;
Edit skips arena gameplay setup and the World authoring lifecycle suppresses
actor/component BeginPlay and ticking, including `Tick When Paused`. Actors are
still promoted for rendering and picking, and the visual pipeline still draws
the backdrop. PlayerStart markers are drawn by the editor UI only.

Play serializes the edit world and loads a fresh SceneLevel with gameplay enabled.
Stop reloads the captured JSON, restores the editor camera and selected GUID, and
keeps undo/redo. Runtime simulation cannot dirty or change the edit snapshot.
The orange viewport border and PLAYING toolbar identify Play; the Inspector is
read-only and explains that runtime changes are discarded on Stop. Pause and
single-frame Step are available during Play.

Hierarchy supports every reflected spawnable Actor through Create, plus
Duplicate (Ctrl+D), Delete (Del), and inline Rename (F2/double-click). Entities
have names in Hierarchy and Inspector. All these operations are undoable;
deletion retains full serialized entity data and recreates the same GUID.
Commands capture scene GUIDs on execution and use runtime IDs only for actors
without scene identity. History clears for a different scene or New Scene.

File supports New, Open, Save and Save As. The Open modal lists scene files under
the editor asset root's `scenes/` and accepts a path. Relative paths use that root;
absolute paths are accepted. Save writes SceneSerializer JSON through the same
exclusive temporary sibling/atomic replacement helper as Data Editor. Dirty
state compares the current edit revision with the loaded/saved revision in O(1).
Scene commands, undo/redo and script attachment advance a monotonic revision;
undo back to saved content therefore keeps the title/toolbar `*` until Save or
Reload. This deliberately avoids serializing the scene to recognize that state.
Play/Stop preserve both revisions; gameplay cannot mark the edit scene dirty.
New/Open/Exit prompt
Save / Discard / Cancel, and Content scene double-click opens the authoring scene.

See [editor script commands](../VegaEditor/tests/scripts/README.md) for automation
and the `VegaEditorAuthoring` build-tree-only save/capture test.

Asset-typed properties (for example a sprite's `Texture`) show an asset picker in the
Inspector. Choose a file from the list, or drag one from the Content panel onto the field;
files of another kind are refused with a message. Clear empties the field, and a `Missing asset`
warning marks a path whose file is not under the asset root. Moving or renaming the file in the
Content panel updates every scene and prefab reference in one undo step. Scene files still store
the path as a plain string. See [reflection](reflection.md#asset-paths).

## Lua scene components

VegaScript's `Overrides()` reads explicit overrides; `SetOverrides()` replaces
them and can stage values before BeginPlay. `Prepare()` reads declarations
without lifecycle callbacks, allowing the scene hook to warn about unavailable
scripts, unknown properties, and incompatible types. Lua top-level declarations
are evaluated during preparation; `on_create` runs only on BeginPlay and sees
the authored overrides. Script properties support float, bool, string, and
Vector2. Paths retain the existing relative `.lua` asset-root restriction.

The Inspector's Add Component popup filters addable types by name. Right-click a
component header for Remove Component; required/non-removable types explain why
removal is disabled. Both commands are Edit-only, capture entity JSON before and
after the change, and recreate the same GUID on undo/redo while preserving
selection. Older property commands continue resolving that GUID. Scripts retain
their separate Add Script path. Parent GUID links and reflected actor-reference
properties (`ActorRef`, see [reflection](reflection.md)) are remapped with subtree copies.

`VegaEditorComponents` runs a configured build-tree script and saves only to
`components-temp.scene.json` in the build tree. It verifies added Health properties,
constructor Sprite removal/undo, and loading the saved scene.

## Implementation files

* Engine: `include/framework/scene/Scene.h`, `src/framework/scene/Scene.cpp`,
  `include/framework/scene/SceneEntityComponent.h`,
  `src/framework/scene/SceneEntityComponent.cpp`,
  `include/framework/reflection/Reflection.h`,
  `src/framework/reflection/EngineReflection.cpp`,
  `include/framework/Application.h`, `include/framework/SpriteComponent.h`,
  `src/framework/SpriteComponent.cpp`, `tests/SceneSerializerTests.cpp`, and
  `LightYearsEngine/CMakeLists.txt`.
* Game: `include/level/SceneLevel.h`, `src/level/SceneLevel.cpp`,
  `include/level/ArenaSandboxSetup.h`, `src/level/ArenaSandboxSetup.cpp`,
  `include/level/PlayerStart.h`, `src/level/PlayerStart.cpp`,
  `include/level/SpriteActor.h`, `src/level/ArenaTestLevel.cpp`,
  `include/environment/Asteroid.h`, `src/reflection/GameReflection.cpp`,
  `include/gameFramework/GameApplication.h`,
  `src/gameFramework/GameApplication.cpp`, `tests/SceneSandboxTests.cpp`,
  `assets/scenes/sandbox.scene.json`, and `LightYearsGame/CMakeLists.txt`.
* Scripting: `VegaScript/include/script/ScriptComponent.h`,
  `VegaScript/src/ScriptComponent.cpp`, and
  `VegaScript/tests/ScriptComponentTests.cpp`.
* Documentation: `docs/scene-authoring.md`.

## Verification, 2026-10-15

* `cmd /c tools\vega-build.cmd`: exit 0, including the final runtime asset sync.
* New CTest entries `LightYearsSceneSerializer` and `LightYearsSceneSandbox`
  pass. The existing `VegaScriptComponent` test also covers script scene
  round trips, staged lifecycle timing, and override diagnostics.
* Full `ctest --output-on-failure` in `build`: exit 8, 79/103 passed, 24 failed,
  541.88 seconds. Every engine test, VegaScriptComponent, and all twelve
  VegaEditor tests pass. The failure names exactly match the before-change
  run: 23 gameplay failures plus `LightYearsRareZoneRulesE2E` timing out at
  120 seconds. The baseline also had that timeout, so the observed failure
  count differs from the anticipated 23 without adding a new failure.
  Logs: `build/scene-baseline-ctest.log` and `build/scene-ctest-final.log`.
* `LightYearsGame.exe --scene scenes/sandbox.scene.json --visual-capture
  D:\Vega\build\scene_sandbox.png --capture-frames 120`: exit 0. The PNG exists
  (1920 x 1080, 1,476,994 bytes) and was visually inspected. It shows the
  player, HUD, backdrop, three dummies, and scripted sprite; asteroids are not
  visible in that captured frame. The headless test verifies all seven scene
  entities, including both asteroids, before gameplay ticking.

## Actor attachments

Scenes accept optional `parent` GUIDs. Parented Location/Rotation values are relative;
loading resolves forward references, and saving orders parents first. Prefab instances
retain child overrides by stable child ID. Hierarchy authoring, transform/physics rules,
format examples and verification are described in [actor hierarchy](hierarchy.md).

## Optional arena bounds

World Settings includes the engine's optional `bounds` rectangle, encoded as
`"bounds": [x, y, width, height]`. Missing or null disables it. Coordinates must
be finite and within -1,000,000..1,000,000; width and height must be positive.
Invalid legacy values remain visible and saveable but have no runtime effect.
Use the checkbox and four values, or drag the green viewport corner handles.
Each drag is one undo step; Escape restores the starting rectangle. The outline
remains visible during Play, with authoring disabled. Reset removes the override.

`ReadSceneBounds(settings)` returns an optional `sf::FloatRect`.
`SceneSerializer::Load` installs authored bounds on `World`; appending a prefab
with empty settings preserves them. Games read `World::GetSceneBounds()` and
choose enforcement. Blank clamps player origins on movement ticks. LightYears
maps scene bounds into `ArenaDefinition::legalBounds` and size on Play, retaining
its existing boundary warning and out-of-bounds gameplay rules. Stop before
changing runtime arena bounds. The engine does not impose collision walls.

Saved content JSON reloads through the game-module hook during Play. Nested
prefab saves and external edits affect new spawns. Existing prefab instances
keep their realized state. See [game modules](game-module.md) for supported
LightYears catalogs and restart requirements. `VegaEditorDailyUse` exercises
bounds, undo, Play/Stop, nested prefab reload and Lua audio in a Blank project copy.