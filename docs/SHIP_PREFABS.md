# Player ship prefabs

Stage 2a extends the Fighter pilot to all six catalog player ships. Every shipped
`Ship.Player.*` entry has a `prefabPath` under `prefabs/ships/player/`. Behaviour
remains in C++; the editor-authored prefab chooses the catalog ship. Gameplay
numbers, weapon selection, progression and module schematics remain in `ships.json`.

## Variant structure

All six shipped assets are variants of the shared, unselected
`player_ship.prefab.json` base. The base has `base.type=PlayerSpaceShip` and no
Ship override; each variant links it and authors only its catalog id:

```json
{
  "format": "vega.prefab",
  "version": 1,
  "name": "Fighter",
  "base": { "prefab": "prefabs/ships/player/player_ship.prefab.json" },
  "properties": { "Ship": "Ship.Player.Fighter.Basic" }
}
```

Vega 471176f makes variant authoring sparse: Apply property/section, Apply All
and scratch **Save Prefab** realize the comparison parent with the edited
descendant's construction selectors and retain an explicitly authored selector
even when it equals the factory default. Ship and intentional Render Enabled
therefore survive editor saves while constructor component values stay
inherited. This is the intended `C++ base class -> base prefab -> variants`
workflow; the earlier flat-prefab fallback was removed. See the resolved
[variant authoring request](engine-requests/ship-prefab-variant-authoring.md).

The game E2E still checks a descendant overriding an explicitly Fighter-selected
parent with every ship choice. It compares component inventory, attributes,
grants, movement and engine lights, rather than only checking the final Ship
string. Runtime variant resolution and sparse editor authoring are separate
acceptance boundaries.

All paths below are relative to `prefabs/ships/player/`; all IDs have the prefix
`Ship.Player.` and suffix `.Basic`.

| Ship ID middle | Prefab file | Preserved special case |
| --- | --- | --- |
| Fighter | `fighter.prefab.json` | Rapid laser; expanded schematic with locked cells; original engine mounts. |
| Breacher | `breacher.prefab.json` | Rapid shotgun cone; its health, movement and module layout. |
| Interceptor | `interceptor.prefab.json` | Dual kinetic blaster; its lighter hull and movement profile. |
| Conductor | `conductor.prefab.json` | Electric arc launcher; its energy/progression profile and schematic. |
| Aegis | `aegis.prefab.json` | Continuous heat laser beam; its defense/energy profile and wide schematic. |
| CryoController | `cryo_controller.prefab.json` | Cryo projector wave; its shield/energy profile and schematic. |

The five non-Fighter entries author engine offsets at (-29,-16) and (29,-16).
The existing loader takes their light style from the Fighter presentation base
(cyan), then PlayerSpaceShip creates both lights. There is no separate per-ship
light-style selector to migrate. Tests compare actual light data on both paths.
Default ability slots still come from `DefaultAbilityLoadout.cpp`; the primary
weapon grant comes from each catalog definition through the existing SpaceShip
constructor. Module seeding and gating still use the selected ship's schematic.

## Ownership and construction

`ships.json` remains the canonical owner of ship numbers, movement, weapon and
schematic data. `ShipDefinition::catalogId` carries the parsed record ID through
the engine's value-copy spawn API. It is metadata from the same JSON `id`, not a
second authored selector. Legacy hand-built definitions/constructors remain
usable; definitions without a catalog ID have an empty reflected `Ship` and are
not catalog-backed prefab construction inputs.

`PlayerSpaceShip` adds one reflected property, **Ship** (string, category Player,
getter and setter, marked `.Construction()`). Existing fields remain Speed, Use Screen Clamp, In Combat
(read-only), and Combat Window Remaining (read-only), plus inherited properties.
The property-aware scene factory validates the player catalog ID and passes its
definition to the existing constructor before properties are applied. The class
also opts into the engine's Spawnable contract. Registration is guarded by the
owning context's factory registry so standalone Player spawning and editor hosts
can both register this path.

Changing **Ship** on an existing actor is refused with an actionable error:
place a prefab with the desired catalog Ship instead. Reapplying the same ID is
idempotent. To author another choice, edit a prefab's JSON selection and reload
it; the scene factory reconstructs the actor. Existing constructors and APIs are
preserved. No Player, world timer, physics start, module loadout application or
gameplay tick is added to the constructor. Existing local ability grants and
owned module observer bindings remain construction state. Physics and the
invulnerability timer start in BeginPlay as before.

## Validation and spawn

Catalog validation reads authored prefab/variant chains through the engine's
`PrefabLibrary::Load`, including canonical asset-root confinement for every
link. It checks the resolved base type is exactly PlayerSpaceShip and the
inherited/overridden Ship equals the catalog record ID, without constructing
scratch actors during catalog bootstrap. Each document in the chain may author
only `Ship` and boolean `Render Enabled` properties; component snapshots,
scripts, children, other ship IDs and other overrides are rejected at any link,
and chain cycles and depth are bounded.
Apply and scratch Save Prefab write sparse deltas against a freshly constructed
property-aware base, always retaining Ship and any intentional Render Enabled
override. Equal constructor component values are omitted. Generic editor edits
outside this narrow contract remain rejected by the game catalog/spawner.
This deliberately narrow player contract does not prescribe
the later enemy or ship-family authoring contract.

Runtime uses a fresh, root-bound `PrefabLibrary::Resolve`, checks its result and
warnings, and loads a one-entity `vega.scene` envelope with a fresh GUID through
`SceneSerializer::Load`. Fresh resolution prevents cached constructor component
snapshots from overriding newly reloaded catalog numbers. Scene identity retains
the prefab path and saving an unchanged instance omits component snapshots.
Warnings, a wrong root/Ship, unresolved data or partial composition fail the
spawn. Any newly created actors are destroyed, including their subtree, and the
world's unresolved-prefab data is restored. Destroyed pending actors use normal
world storage reclamation; the spawner never ticks the caller's world for cleanup.

Only the construction expression in Player::SpawnSpaceShip changes. Life
consumption, progression binding, purchased ability restoration, module loadout,
delegate binding and spawn positioning retain their existing ordering before
BeginPlay. No Player is created by either prefab resolution or the scene factory.

The direct `SpawnActor<PlayerSpaceShip>(definition)` fallback remains. Although
all shipped player entries now have a prefab, `prefabPath` remains optional and
the loader still accepts definitions without it. The E2E reloads a build-tree
catalog without prefab paths and exercises Player's direct spawn branch. Existing
direct constructors also remain necessary for callers and equivalence tests.

## Verification

`LightYearsShipPrefab` iterates all player catalog IDs and exercises direct/prefab
equivalence before and after applying each ship's default modules and BeginPlay,
engine-light data, all movement fields, scratch safety,
construction-selector refusal, sparse save/reopen, invalid references and
composition, construction-selector variants, catalog reload and Player respawn
for every ship (life count, prefab identity, purchased ability levels, module
inventory/placements, positioning and actor/timer cleanup). Its inputs/results are
written under `build/e2e-artifacts/ship-prefab/`.

`VegaEditorFighterPrefab` places Fighter and Interceptor variants, saves/reopens
their scene, executes Apply/Revert on Render Enabled, then edits/saves the prefab
in the scratch Inspector and reopens the scene again. Both variants undergo Apply
and scratch Save through the shared `player_ship.prefab.json` base, retain their
distinct Ship selections, and are checked by the game validator and spawner
against direct construction. The runner recreates only a dedicated
build-tree asset copy. It does not edit shipped source assets. The scenario
retains Render Enabled=false through Apply and scratch Save, verifies catalog
values through Revert, reopen and reload-prefabs refresh, and checks Ship mutation
refusal. The postcheck requires exactly Ship and the visibility override, the
variant base link, no component snapshots, and acceptance by both the game
validator and spawner; it compares the spawned ship with direct catalog
construction. A postcheck-only wrong visibility expectation must fail. See the
implemented [engine request](engine-requests/ship-prefab-sparse-authoring.md)
and the resolved [variant authoring request](engine-requests/ship-prefab-variant-authoring.md).
Placement, sparse Apply/Save, scene save/reopen, Revert and refresh are supported.
Construction diff bases are fresh; resolved prefab caches must be reloaded after
catalog changes (the editor content watcher does this around its reload hook).
Existing expanded assets remain explicit overrides and are still rejected by the
game; loading alone does not migrate them. Automated editor success does not
establish manual visual acceptance.

## Remaining stage

Stage 2b adds [enemy actor prefabs](ENEMY_PREFABS.md) through EnemyFactory,
preserving combat/behavior profiles, caller context and rewards. Stage 2c adds
dummy target prefabs; editor-authored level scenes remain a later stage.
