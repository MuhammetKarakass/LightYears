# Enemy actor prefabs

Stage 2b makes all shipped enemy actors sparse prefabs. Content is authored in
assets; C++ still owns construction, combat, behavior and lifecycle. Stage 2c
does the same for dummy targets; level/scene migration remains a separate stage.
No stage, arena, wave or level source changes are needed to adopt the enemy spawn
path.

## Catalog and assets

`assets/content/data/enemy_definitions.json` remains the sole mapping from enemy
ID to chassis, combat profile and behavior profile. Chassis numbers/rewards stay
in `ships.json`; combat loadouts/progression and behavior settings stay in their
existing profile catalogs. The optional `EnemyDefinition::prefabPath` selects an
asset relative to the asset root. Every shipped enemy defines one:

| Enemy ID | Prefab under `prefabs/enemies/` | Preserved special case |
| --- | --- | --- |
| `Enemy.ApproachGunner.Basic` | `Enemy.ApproachGunner.Basic.prefab.json` | Approach movement; Vanguard pulse primary from combat profile; chassis health 60, shield 25, contact damage 50, score 10, XP 1. |
| `Enemy.StrafeSkirmisher.Basic` | `Enemy.StrafeSkirmisher.Basic.prefab.json` | Strafe movement (300–500 band); TwinBlade scatter primary; chassis health 60, shield 10, contact damage 50, score 20, XP 1. |
| `Enemy.RangeKeeper.Basic` | `Enemy.RangeKeeper.Basic.prefab.json` | HoldRange movement (800–1200 band); Hexagon cryo wave primary; chassis health 100, shield 60, contact damage 60, score 30, XP 1. |

These are flat `vega.prefab` version 1 documents, with `base.type=EnemyActor`
and exactly `properties: {"Enemy ID": "<catalog id>"}`. They copy no gameplay
numbers, profile IDs or constructor component values. Chassis primary weapons
remain prohibited: combat profiles own grants, and EnemyActor still strips a
chassis primary before construction and rejects such a chassis at BeginPlay.

## Construction and runtime ownership

`EnemyActor` is spawnable. Its existing **Enemy ID** property is now writable
and marked `.Construction()`, like PlayerSpaceShip's **Ship**. The setter permits
the current ID and refuses another ID with a construction-only error; place the
desired prefab instead. Existing read-only profile/level/contact observations and
the writable Ship XP Reward inspector property remain. That inspector property
is deliberately outside the catalog prefab allowlist.

The property-aware scene factory receives the resolved Enemy ID and calls
`content::ConstructEnemy`, which resolves the canonical enemy/chassis/profiles and
uses the original EnemyActor constructor. Creating from the EnemyActor class
uses ApproachGunner as the neutral factory default. Neither factory initializes
combat/behavior, starts physics/timers, ticks a world or creates a Player.

Both `content::SpawnEnemy` overloads converge. With prefabPath they use
`SpawnEnemyPrefab`, then install the caller's encounter multiplier and complete
EnemySpawnContext (level, variation seed and window culling) through
`ConfigureSpawnContext`. Without prefabPath they construct directly with the
original constructor arguments. Both paths set the requested position before
BeginPlay. Context configuration is refused after BeginPlay. Combat loadout,
level/seed progression and behavior still initialize in BeginPlay, in the same
order. Caller context is runtime-only; it is never copied into the prefab or
saved scene. Editor-placed enemies use neutral level 1 / seed 0 / multiplier 1.

EncounterWaveRuntime's callback, Vanguard/TwinBlade/Hexagon/Chaos stages and
ArenaTestLevel already call SpawnEnemy, so they all adopt the same path. The
arena's existing wave-enable flag is unchanged.

## Validation, loading and cleanup

Enemy and player prefabs share the existing ShipPrefabSpawner implementation.
Their small type/selector contracts retain separate allowlists: enemies admit
only Enemy ID; players retain Ship and boolean Render Enabled. Catalog loading
validates enemy assets before installing the new definitions, without scratch
actor construction. PrefabLibrary enforces canonical asset-root confinement;
the game walks every parent, checks the final EnemyActor type and selected ID,
and rejects cycles, missing assets, component/script/hierarchy snapshots and
other properties. Inherited selectors and explicit selector variants are
supported; shipped documents remain flat.

Runtime uses fresh resolution and the same one-entity scene envelope as player
ships, retaining prefab identity. It verifies the resulting type/ID, warning-free
single-actor composition and unchanged unresolved-prefab state. Partial failure
or exceptions during scene loading destroy every newly created actor and restore
unresolved data, preserving actors already in the caller's world. The spawner
never ticks the caller to reclaim storage; the world's normal cleanup does that.

## Verification

`LightYearsEnemyPrefab` compares every ID with the original direct constructor
before BeginPlay, after BeginPlay and after a tick: component inventory, sprite,
engine lights, movement, attributes, ability grants/levels/slots, combat/behavior
selection, resources, XP/score/contact damage, collision and simulation domain.
Four caller contexts include neutral defaults, seeded high levels, disabled
culling and zero damage multiplier. It also exercises selector immutability,
sparse save/reopen and type authoring, variant construction, validator rejection,
invalid catalog paths, legacy optional-path fallback, normal destruction, partial
failure cleanup and a real EncounterWaveRuntime callback/completion flow.
Inputs and results live in `build/e2e-artifacts/enemy-prefab/`.

`VegaEditorEnemyPrefab` uses a disposable build-tree asset copy to place all
three IDs, check their construction values and refusal behavior, Revert a health
edit, Apply the selector, scratch Save, scene save/reopen and reload-prefabs.
Its C++ postcheck requires sparse selector-only assets and scene instances and
compares every edited prefab with direct construction through the game validator
and spawner. `LightYearsDummyPrefab` does the equivalent for the standard dummy
prefab and `VegaEditorDummyPrefab` places, saves and reopens it. Editor
automation does not establish manual visual acceptance.

The requested build/full-suite comparisons and focused runs are recorded in
`build/w10-enemies-report.md`; the reflection golden is regenerated with
`LightYearsReflectionRegistryGoldenTests --dump` and reviewed separately.

## Dummies (stage 2c)

Dummy targets use the same construction-selector contract, but their selector is
the existing `DummyEnemy` **Ship** property: a catalog ship ID, not an enemy
definition ID. `DummyEnemy` carries the resolved `ShipDefinition::catalogId` in
its own member. **Ship** is now writable and marked `.Construction()`. Reapplying
the same ID is idempotent; changing it on an existing actor is refused with a
construction-only error. The scene factory still constructs the requested
catalog chassis, and `DummyEnemy`'s constructor keeps its training-target shield
override (maximum shield 50).

`assets/prefabs/dummies/dummy_standard.prefab.json` is the shipped dummy prefab:

| Prefab | Base type | Selector |
| --- | --- | --- |
| `prefabs/dummies/dummy_standard.prefab.json` | `DummyEnemy` | `Ship.Enemy.RangeKeeper.Basic` |

It is a flat `vega.prefab` version 1 document with exactly
`properties: {"Ship": "Ship.Enemy.RangeKeeper.Basic"}`. It copies no gameplay
numbers or component values; chassis numbers remain in `ships.json`. Dummy
prefabs share the ship/enemy `PrefabLibrary` resolution and sparse validation
path through `ValidateDummyPrefab` / `SpawnDummyPrefab`; only the `Ship` selector
is admitted, and the selected ID must exist in the ship catalog.

ArenaTestLevel's four 9000-health / 40-armor targets and twenty 50-health
targets are unchanged. Their overrides (health, armor, speed, weapon) are
arena-only values with no catalog home, so no heavy-dummy prefab is shipped.
Expressing them as prefabs needs a data-model change: a catalog-owned dummy
definition (for example health/armor/speed/weapon overrides, or dummy variants
in `ships.json`) that the prefab can select without copying numbers into assets.

## Remaining stages

Migrating levels to editor-authored scenes follows separately. This stage does
not change ArenaTestLevel's dummy construction or level/scene authoring.
