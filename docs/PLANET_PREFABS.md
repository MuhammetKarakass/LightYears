# Planet prefabs

Planet decor is authored as ordinary prefab actors so it can be placed and tuned
in the Inspector instead of being hard-coded in level code. Each prefab is a
`ships`-style JSON document under `LightYearsGame/assets/prefabs/environment/`
with a base spawnable type and engine/reflected components; no engine or level
source changed for this task.

The textures come from EnvironmentData (`LightYearsGame/include/gameConfigs/world/EnvironmentConfig.h`),
which still owns the codes used by `LevelOne`:

| Prefab | Texture | Size (px) | Depth | Can Rotate | Rotation Speed | Drift Velocity |
| --- | --- | --- | --- | --- | --- | --- |
| `prefabs/environment/planet_blue.prefab.json` | `SpaceShooterRedux/PNG/Planets/Planet2.png` | 703x484 | 0.3 | true | 6 | (0, 0) |
| `prefabs/environment/planet_earth_blue.prefab.json` | `SpaceShooterRedux/PNG/Planets/Planet1.png` | 407x407 | 0.3 | true | 6 | (0, 0) |
| `prefabs/environment/planet_green.prefab.json` | `SpaceShooterRedux/PNG/Planets/Planet7.png` | 357x358 | 0.3 | true | 6 | (0, 0) |
| `prefabs/environment/planet_orange.prefab.json` | `SpaceShooterRedux/PNG/Planets/Planet6.png` | 339x339 | 0.3 | true | 6 | (0, 0) |
| `prefabs/environment/planet_blue_large.prefab.json` | variant of `planet_blue` | 703x484 | 0.5 | true | 6 | (0, 0) |

`Star_Orange` (Planet3.png) and `Meteor1`/`Meteor2` (Planet4.png / Planet5.png)
were identified but are not planets, so no prefab is shipped for them yet.

## Schema

Each planet prefab uses the base spawnable actor `SpriteActor`, whose constructor
already adds a `SpriteComponent` (reflected properties **Texture** and **Scale**).
Because the sprite component already exists, its document entry carries no
`added` flag; `ParallaxComponent` is not created by the constructor and therefore
carries `"added": true`, as `D:\Vega\docs\parallax.md` requires. `ParallaxComponent`
fields are **Depth**, **Can Rotate**, **Rotation Speed** and **Drift Velocity**.

`planet_blue_large.prefab.json` is a variant: its `"base": { "prefab": ... }`
points at `planet_blue`, and its document overrides **Depth** (0.5) and the sprite
**Scale** (1.6) while inheriting the texture and rotation fields. Variants are
documented in `D:\Vega\docs\prefabs.md`.

## Placing and tuning

* Place one from the editor Hierarchy's **Create > Prefabs** menu (at the camera
  centre) or by dragging the asset from Content onto the viewport. Placement is
  undoable. See `D:\Vega\docs\prefabs.md` for the full editor workflow.
* Select an instance and edit **Parallax > Depth** (0 far .. 1 world), **Can
  Rotate** and **Rotation Speed** (degrees/second) in the Inspector. Lower Depth
  makes the planet drift more slowly against the camera.
* `ParallaxComponent` never rewrites the actor's authored `Location`; parallax
  and drift are a render-time offset, so scene saves stay stable.

## LevelOne is unchanged

`LevelOne::SpawnCosmetics` still builds the planet and meteor layers through
`BackgroundLayer` from `EnvironmentData`. No behaviour, look or timing changed.
Migrating a level to place these prefabs directly (or making `BackgroundLayer`
prefab-driven) is a separate decision and is intentionally not part of this task.

## Verification

`LightYearsPlanetPrefab` resolves every prefab in `assets/prefabs/environment`
through `PrefabLibrary`, spawns each as a scene instance into a scratch world and
checks one sprite actor with a loaded sprite and a `ParallaxComponent`, that the
variant inherits the base texture/rotation and overrides Depth, and that spawning
leaves a level world and the Player state untouched. Results are written under
`build/e2e-artifacts/planet-prefabs/`.
