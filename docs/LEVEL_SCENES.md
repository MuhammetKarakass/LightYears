# Level scenes (stages 3–4)

Content belongs in the editor; rules stay in C++. `ArenaTestLevel` now reads
`LightYearsGame/assets/scenes/arena-test.scene.json`. `sandbox.scene.json` is a
separate SceneLevel fixture and is unchanged. Stage 4 loads LevelOne's campaign
plan from `scenes/level-one.scene.json` into the existing LevelOne C++ rules.

## ArenaTest schema

The envelope is `vega.scene`, integer version 1, with ordinary engine entities.
Required settings are `bounds: [x,y,width,height]`, `playerShip` (player catalog
ID), `routeStop` (registered galaxy route index), and `levelRules: "ArenaTest"`.
`levelRules` is a registered string in World Settings. Empty selects the existing
SceneLevel; LevelOne selects the campaign owner described below; unknown nonempty
rules are rejected by the game module.

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
* `DummyArmor`: base Armor contribution for scene-placed dummy prefab instances
  that author no Armor of their own.

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

Four heavy targets are ordinary instances of
`prefabs/dummies/dummy_heavy.prefab.json`, each with only a Location. Their
9000 health / 40 Armor / rapid-laser construction lives in the catalog-owned
`Ship.Enemy.HeavyDummy.Basic` chassis in `ships.json`, so no HeavyDummy fields or
C++ override remain. The loader applies the same BeginPlay dummy rebind as for
the small targets; because that chassis already authors Armor 40, `DummyArmor` is
not stacked on top of it. They appear in Edit and on Play like the small targets.
See [enemy/dummy prefabs](ENEMY_PREFABS.md).

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
player, dummy-attribute, music or encounter setup, even when Enabled is true. Invalid
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
Select a small or heavy dummy to move it; a small dummy also exposes
HealthComponent > Max Health. Use World
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
playtest metric remain C++. Nested wave authoring would require a separately
owned Vega reflection/Inspector change.

## LevelOne campaign (stage 4)

`LightYearsGame/assets/scenes/level-one.scene.json` is an ordinary `vega.scene`
version 1 with `settings.levelRules: "LevelOne"` and exactly one passive
`CampaignSetupActor`. The game module maps this registered setting to LevelOne,
not ArenaLevel or SceneLevel. The game's startup selection remains unchanged;
the existing campaign entry/restart reads this file, and explicit scene Play
selects its C++ owner. No PlayerStart is needed: LevelOne still creates the
selected player's ship at the original window-relative position. Arena/encounter
setup actors, player markers, dummies, player and enemy actor placements are
incompatible with these campaign rules. Ordinary decorative entities may be added.

CampaignSetupActor has five top-level reflected lists:

| Property | Leaf entry fields |
| --- | --- |
| `Stages` | `kind` (Wait, Vanguard, TwinBlade, Hexagon, Chaos), `duration`, `configuration` |
| `Vanguard` | minSpawnInterval, maxSpawnInterval, spawnDistanceToEdge, switchInterval, rows, perRow |
| `TwinBlade` | minSpawnInterval, maxSpawnInterval, spawnCount |
| `Hexagon` | spawnInterval, spawnGroups |
| `Chaos` | spawnInterval, spawnIntervalDecrement, survivalDuration, spawnMinDistanceTop |

Each Wait entry has nonnegative seconds in `duration` and `configuration: -1`.
Other entries require `duration: 0`; their zero-based `configuration` selects
the list matching `kind`. Their existing C++ rules determine completion. Chaos's
survivalDuration is the countdown after TotalChaos starts, not the entire stage's
duration. No new independent stage timeout is added. Lists may hold multiple
configurations, including repeated kinds with different indices; at most one
Chaos stage is permitted by the existing single Chaos HUD binding. The shipped
order is Wait(5), Vanguard(0), Wait(5), TwinBlade(0), Wait(5), Hexagon(0), Wait(5),
Chaos(0), where parentheses on combat stages are configuration indices.

These flat typed lists avoid Vega v1's nested-list restriction, just as arena
Waves/Entries do. Every list and every leaf field must be authored explicitly
(or inherited from a resolved prefab). Unknown keys/kinds, missing/duplicate
setup, bad GUIDs, invalid JSON kinds, nonfinite/negative durations, invalid
configuration indices, nonpositive/reversed spawn intervals, nonpositive or
overflowing counts, and Chaos intervals exhausted before difficulty 11 refuse
Play with a field/list diagnostic. Unused parameter entries are also validated.
Zero Wait/survivalDuration and nonnegative switch intervals retain engine timer
semantics. No setter clamping silently repairs invalid gameplay data.

LevelLayout shares the arena envelope/entity/reflection/prefab preflight and
transactional entity realization, then resolves CampaignPlan instead of arena
data. The rules owner is checked explicitly, so ArenaTest cannot accidentally
consume a LevelOne plan or vice versa. The default LevelOne constructor uses
code defaults **only if the file is absent**, with a path warning. Existing
malformed, unreadable or invalid files fail; explicit scene Play never falls
back. Edit loads entities, suppresses BeginPlay/stages/timers/player/decor and
allows gameplay values to be repaired. The separately owned engine transactional
Play-refusal work is still an acceptance boundary; CreateLevel may throw.

Stage execution and spawn algorithms, random calls and their order, formation
geometry, enemy identities, Chaos difficulty thresholds/asteroid settings,
player creation/death/immediate respawn, the three-second game-over timer,
pause/resume, restart and Chaos HUD binding remain C++. World bounds/playerShip
do not replace campaign player/camera rules; the shipped scene omits them.
An explicitly authored routeStop retains the common backdrop setting behavior.

**Decor decision:** no decor data is migrated. SpawnCosmetics and EnvironmentData
remain unchanged: darkPurple background, the existing planet/meteor definition
order (including duplicate meteor paths), depth color, visibility, two planet
sprites, forty meteors, size/velocity ranges, lights and planet tags. This avoids
changing the visual construction or random sequence without a visual equivalence
proof. No planet prefab replaces BackgroundLayer. Next work is a separate decor
as prefabs decision and migration with its own visual/randomness evidence.

### Campaign verification

The pre-edit effective configuration is frozen at
[`level-one-3e26ef91.json`](../LightYearsGame/tests/fixtures/level-one-3e26ef91.json).
The old constructor/sequence path's headless trace is frozen in
[`level-one-3e26ef91-headless.json`](../LightYearsGame/tests/fixtures/level-one-3e26ef91-headless.json).
Ordinary tests never regenerate these inputs. Test-only build-tree copies of the
four stage sources redirect RandRange to a fixed generator (seed 1741); shipping
stage objects and Vega's random_device-backed RNG are unchanged. The capture
uses the real GameStageSequence, world timers and enemy prefab spawn path, with
a 1920×1080 headless viewport and 0.1-second steps. It records all eight stage
beginnings/endings and 237 enemy IDs/positions/spawn steps through completion
at step 3021. This is controlled stage timing evidence, not a live random seed
or visible gameplay recording.

`LightYearsCampaignScene` compares the reflected scene/defaults against the
configuration fixture, requires exact frozen trace equality for both factory
and legacy-constructor paths, and checks the actual LevelOne's stage execution
has identical enemy spawns and final completion timing. It also exercises loader
rejections, absent-only fallback, Edit lifecycle suppression, reflected round
trips, original player respawns and the game-over timer. Artifacts are written
under `build/e2e-artifacts/campaign-scene/`.

Open `scenes/level-one.scene.json`, select **Campaign setup**, and edit Stages
or the per-kind configuration lists in Inspector. `VegaEditorCampaignScene`
opens this asset in a disposable build-tree copy, edits/undoes every list's
fields, saves/reopens, enters Play, verifies the player/initial Wait, stops and
saves again. Its C++ postcheck revalidates the saved configuration and headless
trace. Automation does not establish manual visible-window acceptance.
