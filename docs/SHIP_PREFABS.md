# Ship prefab pilot: Fighter

Stage 1 implements `Ship.Player.Fighter.Basic` as `PlayerSpaceShip` plus
`prefabs/ships/player/fighter.prefab.json`. Only Fighter's catalog entry has an
optional `prefabPath`; other player ships retain direct construction. Sparse
editor authoring uses Vega e3bce0c's construction-property contract.

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
`PrefabLibrary::Load`, including canonical asset-root confinement. It checks the
resolved base type is exactly PlayerSpaceShip and the inherited/overridden Ship
equals the catalog record ID, without constructing scratch actors during catalog
bootstrap. Stage 1 permits only `Ship` and boolean `Render Enabled` properties;
component snapshots, scripts, children, other ship IDs and other overrides are rejected.
Apply and scratch Save Prefab write sparse deltas against a freshly constructed
property-aware base, always retaining Ship and any intentional Render Enabled
override. Equal constructor component values are omitted. Generic editor edits
outside this narrow contract remain rejected by the game catalog/spawner.
This deliberately narrow pilot does not prescribe
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
build-tree asset copy. It does not edit shipped source assets. The scenario
retains Render Enabled=false through Apply and scratch Save, verifies catalog
values through Revert, reopen and reload-prefabs refresh, and checks Ship mutation
refusal. The postcheck requires exactly Ship and the visibility override, no
component snapshots, and acceptance by both the game validator and spawner; it
compares the spawned ship with direct catalog construction. A postcheck-only
wrong visibility expectation must fail. See the implemented
[engine request](engine-requests/ship-prefab-sparse-authoring.md).
Placement, sparse Apply/Save, scene save/reopen, Revert and refresh are supported.
Construction diff bases are fresh; resolved prefab caches must be reloaded after
catalog changes (the editor content watcher does this around its reload hook).
Existing expanded assets remain explicit overrides and are still rejected by the
game; loading alone does not migrate them. Automated editor success does not
establish manual visual acceptance.
