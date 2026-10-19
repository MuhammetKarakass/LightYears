# Level scenes (stage 3)

Content belongs in the editor; rules stay in C++. `ArenaTestLevel` now reads
`LightYearsGame/assets/scenes/arena-test.scene.json`. `sandbox.scene.json` is a
separate SceneLevel fixture and is unchanged. LevelOne is unchanged.

## Schema

The envelope is `vega.scene`, integer version 1, with ordinary engine entities.
Required settings are `bounds: [x,y,width,height]`, `playerShip` (player catalog
ID), `routeStop` (registered galaxy route index), and `levelRules: "ArenaTest"`.
`levelRules` is a registered string in World Settings. Empty selects the existing
SceneLevel; unknown nonempty rules are rejected by the game module.

Exactly one `PlayerStart`, `ArenaSetupActor`, and `EncounterSetupActor` must exist.
Every authored entity GUID must be nonempty and unique, including explicit
children. PlayerStart supplies location and, for explicit scene Play, the ship,
taking precedence over settings.playerShip. The default game bootstrap uses that
ship unless `--visual-ship` explicitly overrides it. Direct ArenaTestLevel callers
retain their selected ship; F1–F6 and restart keep their existing behavior.
Respawn's existing `Default` sentinel is retained.
The default game also applies the scene's routeStop unless an explicit
`--galaxy-stop` overrides it; an absent-scene fallback retains the original CLI
backdrop path.

`ArenaSetupActor` has these reflected fields:

* `Boundary`: enabled, visualType (existing ArenaBoundaryVisualType label),
  outOfBoundsMargin, outOfBoundsTime, outlineColor, warningColor, outlineThickness.
* `Respawn`: respawnDelay, playerIndex, createPlayerIfMissing, useScreenClamp,
  respawnWhenDestroyed, gameOverWhenRespawnFails, playerShipId.
* `Music`: path (sound asset), volume, pitch, loop, fadeDuration. This is the
  existing Music fade request. The inherited GameLevel startup SFX_World music
  request remains unchanged, including its order before the fade.
* `DummyArmor`: base Armor contribution for scene-placed dummy prefab instances.
* `HeavyDummy`: ship catalog ID, health, armor, legacySpeed, primaryWeaponId.
* `HeavyDummyPositions`: ordinary editable Vec2 list in world coordinates.

`EncounterSetupActor` has these reflected fields:

* `Enabled` (shipped false), `SequenceMode` (only EndlessCycle is supported by
  these rules), `MaximumConcurrentEnemies` (0 means uncapped).
* `Progression`: additionalEnemyEveryWaves, enemyLevelEveryWaves,
  maximumAdditionalEnemies, maximumEnemyLevel, seed. These map to the existing
  EncounterProgression; its concurrency member comes from MaximumConcurrentEnemies.
  Reflection v1 uses int32, so authored seeds are 0 through 2147483647.
* `SpawnPoints`: nonempty Vec2 list of absolute world anchors.
* `SpawnOffsetSpacing`: nonnegative spacing for the unchanged C++ spiral.
* `Waves`: list of `{ "interWaveDelay": 1.5 }` templates.
* `Entries`: list of `{ "wave": 0, "enemyId": "Enemy.ApproachGunner.Basic",
  "count": 2, "spawnInterval": 0.5 }`. `wave` is a zero-based Waves index.
  Entries retain their authored relative order within a wave.

The proposed `Waves[].entries[]` cannot be reflected by Vega 378f9aa: its
`Reflection.h` rejects any list/struct inside a struct (lines 306–309), and its
Inspector script paths support one struct level. Two flat reflected lists keep
all wave composition editable without an engine edit or a JSON-text property.
The loader reconstructs existing EnemyWaveDefinition objects; it creates no
second encounter system. Reordering Waves requires updating Entries.wave.

## Dummies and sparse instances

Twenty small targets are ordinary instances of
`prefabs/dummies/dummy_standard.prefab.json`, each with Location and a sparse
HealthComponent `Max Health: 50` override. Ship and gameplay numbers are not
copied into the prefab. Before BeginPlay, the loader synchronizes owner MaxHealth
and rebinds the scene's Armor contribution through the existing ShipRuntime,
also recomputing health-derived regen. This preserves the original 50 health /
40 Armor target behavior. Edit keeps authored component values without applying
gameplay attributes or running actor lifecycles.

Four heavy targets remain code-spawned. Existing reflection exposes Max Health,
but no Armor or primary-weapon constructor override, so the existing dummy
prefab cannot express the original 9000 health / 40 Armor / rapid-laser target.
Their construction inputs and positions now live in ArenaSetupActor; C++ still
uses the original DummyEnemy constructor. They appear on Play, not in Edit.
There is no new heavy-dummy catalog entry or expanded prefab contract. See
[enemy/dummy prefabs](ENEMY_PREFABS.md).

## Loader order and failure behavior

1. Register reflected types, existing content, and scene settings in the host's
   EngineContext. New setup actors are registered after their seven leaf structs.
2. Parse the complete scene; check envelope/version/settings, GUIDs, reflected
   JSON kinds (including finite numbers and exact integer counts), prefab
   resolution and dummy compatibility. Do not initialize arena or player state.
3. Preflight the complete scene in an unticked scratch World. Engine setter,
   component, script and prefab warnings reject Play. Resolve typed setup data
   and validate rules/catalog references. No BeginPlay or tick runs here.
4. Load entities into the destination world with a fresh PrefabLibrary. A load
   error destroys only newly created actors and restores unresolved-prefab data.
5. Apply dummy attributes and scene/player selection, then initialize the
   existing arena, follow camera, respawn/player, HUD, music and encounter in
   their existing C++ owners. Camera policy remains ArenaLevel/CameraSettings;
   scene bounds feed the same camera setup.

Bounds must be finite with positive dimensions. Boundary times, respawn delays,
music controls, progression intervals/caps, dummy health/armor, anchors, counts,
wave indices/delays/intervals and concurrency are validated. IDs must exist in
their owning ship, weapon or enemy catalog; enemy IDs come from
enemy_definitions.json. Each wave needs at least one entry. Required components
cannot be removed. PlayerSpaceShip/EnemyActor placements are refused in this
ruleset because arena/encounter rules own those spawns. Unknown reflected keys
are errors, including struct-field typos. Other settings retain engine behavior.
All setup actor fields and their struct fields must be explicitly authored
(or inherited from a resolved prefab). A partial gameplay setup does not silently
fall back to constructor numbers. Inherited generic Actor fields remain sparse.

The default path falls back to the historical C++ configuration **only when the
scene file is absent**, with a warning containing its path. An existing malformed,
unreadable or invalid scene is an error; it never silently selects defaults.
Explicit scene Play never falls back. Edit loads authored entities without arena,
player, heavy-dummy, music or encounter setup, even when Enabled is true. Invalid
gameplay values can be opened and corrected in Edit; Play validates the captured
document before replacing the edit world.

**Engine acceptance gap:** Vega 378f9aa sets editor transition flags before calling
the game factory and does not catch a refused Play transaction. The game loader
rejects invalid configuration before creating a gameplay world, but graceful UI
refusal that preserves a usable Edit session needs the
[engine request](engine-requests/scene-play-validation-refusal.md). Do not treat
loader rejection tests as proof of that editor-host behavior.

## Editing

Open `scenes/arena-test.scene.json` using File > Open or
`LightYearsEditor --editor-scene scenes/arena-test.scene.json`. Select **Arena
setup** or **Encounter setup** in Hierarchy. Expand their typed fields and lists
in Inspector; edits and list operations use the normal reflection/undo path.
Select a small dummy to move it or edit HealthComponent > Max Health. Use World
Settings for bounds/route/player/rules. Save writes an ordinary engine scene;
plain setup actors gain complete writable defaults on the first save, while
dummy instances retain sparse prefab overrides. Stop restores the edit snapshot.

## Verification and next work

Before runtime changes, the effective source values at 50764d21 were recorded in
[`arena-test-50764d21.json`](../LightYearsGame/tests/fixtures/arena-test-50764d21.json).
The original level was also started headlessly and its 24 dummies and first eight
wave plans/spawns/anchors/variation seeds captured in
[`arena-test-50764d21-headless.json`](../LightYearsGame/tests/fixtures/arena-test-50764d21-headless.json).
These fixtures are immutable test inputs, never regenerated by ordinary tests.

`LightYearsLevelLayout` checks scene/default configuration equality against the
fixture, exact headless equality against the pre-change capture, dummy constructor/
BeginPlay/tick attribute equivalence, loader rejection diagnostics, absent-only
fallback, Edit lifecycle suppression and reflected round trips.
`VegaEditorLevelScene` opens the scene, edits/undoes typed struct/list fields,
saves/reopens, enters Play, checks 24 dummies / one player / waves off, stops and
saves again. A C++ postcheck revalidates saved data, sparse dummy overrides and
headless equivalence using a disposable build-tree asset copy.

Endless cycling/scaling, seed derivation, spiral offsets, failure handling,
restart/input, ownership cleanup, boundary/camera/respawn execution and every
playtest metric remain C++. Next work is a separate LevelOne scene migration and
decor layers, including adopting existing planet prefabs. Nested wave authoring
would require a separately owned Vega reflection/Inspector change.
