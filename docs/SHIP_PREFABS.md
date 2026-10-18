# Ship prefab pilot: Fighter

Stage 1 implements `Ship.Player.Fighter.Basic` as `PlayerSpaceShip` plus
`prefabs/ships/player/fighter.prefab.json`. Only Fighter's catalog entry has an
optional `prefabPath`; other player ships retain direct construction. Engine
sources and APIs are unchanged.

## Ownership and construction

`ships.json` remains the canonical owner of ship numbers, movement, weapon and
schematic data. `ShipDefinition::catalogId` carries the parsed record ID through
the engine's value-copy spawn API. It is metadata from the same JSON `id`, not a
second authored selector. Legacy hand-built definitions/constructors remain
usable; definitions without a catalog ID have an empty reflected `Ship` and are
not catalog-backed prefab construction inputs.

`PlayerSpaceShip` adds one reflected property, **Ship** (string, category Player,
getter and setter). Existing fields remain Speed, Use Screen Clamp, In Combat
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
`PrefabLibrary::Load`, including canonical asset-root confinement. It checks the
resolved base type is exactly PlayerSpaceShip and the inherited/overridden Ship
equals the catalog record ID, without constructing scratch actors during catalog
bootstrap. Stage 1 permits only `Ship` and boolean `Render Enabled` properties;
component snapshots, scripts, children, other ship IDs and other overrides are rejected. Generic
editor authoring can still create broader documents; such documents are rejected
by the game catalog/spawner. This deliberately narrow pilot does not prescribe
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

## Verification

`LightYearsShipPrefab` exercises direct/prefab equivalence, scratch safety,
construction-selector refusal, sparse save/reopen, invalid references and
composition, variants, catalog reload and Player respawn. Its inputs/results are
written under `build/e2e-artifacts/ship-prefab/`.

`VegaEditorFighterPrefab` places the shipped Fighter, saves/reopens its scene,
executes Apply/Revert on Render Enabled, then edits/saves the prefab in the scratch
Inspector and reopens the scene again. The runner recreates only a dedicated
build-tree asset copy. It does not edit shipped source assets. Vega 7ea0f2c expands
type prefab constructor snapshots during Apply/Save; the scenario's postcheck
records that copied document and proves the game validator refuses it. Passing
this characterization does **not** mean sparse Apply/Save authoring is supported.
That part of Stage 1 remains blocked on the [engine request](engine-requests/ship-prefab-sparse-authoring.md).
Placement, sparse scene save/reopen and Revert are supported. Automated editor
success does not establish manual visual acceptance.
