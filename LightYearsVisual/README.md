# LightYearsVisual

Standalone static library (`LightYearsVisual`, namespace `ly::visual`) for procedural, sprite-free visuals drawn with SFML shapes, vertex arrays and GLSL `#version 120` shaders. It depends only on SFML and nlohmann_json, not on `LightYearsEngine` or the game. Status: V1 foundation, first game integration pass, and V3 procedural player ships (see "Ships (V3)", "Game integration" and `docs/VISUAL_LIBRARY_PLAN.md`).

## Layers

`VisualPipeline` owns four render textures and composites them in this order: background, world, sprites, energy, bloom, flash, vignette. All drawing coordinates are output pixels; the pipeline maps them onto each layer.

| Layer | Resolution | Look | Draw with |
| --- | --- | --- | --- |
| Background | full | soft, low-frequency nebula, haze, sparse sharp stars, parallax | `DrawGradient`, `DrawNebula`, `StarField` |
| World | virtual (`virtualHeight`, e.g. 240 rows), upscaled nearest | crisp pixels, hard silhouettes, outlines, optional palette dither | `ShapeBrush` (use `hardEdges`), `DrawPlanet` |
| Sprites | same virtual resolution as World, upscaled nearest, drawn over World | pixel-scaled content that keeps its own colors, never dithered (game sprite actors) | any `RenderTarget` draw |
| Energy | full, additive | tiny bright core, limited halo, dark surroundings, bloom source | `SubmitGlow`, `SubmitTrail`, `SubmitFlash` |

Why two world-resolution layers: the palette dither snaps every pixel to the active color script, which is right for procedural art (planet, shapes) but recolors authored sprites. Procedural content goes to `Layer::World` (dithered), sprites to `Layer::Sprites` (original colors, composited premultiplied so soft edges are not darkened twice).

Camera jitter: world-layer content shows sub-pixel camera steps as shimmer. `pipeline.SnapWorldView(gameView)` returns the camera view with its top-left corner snapped to the virtual pixel grid and widened to the whole texture; set it on the World/Sprites targets and derive parallax and glow positions from it so everything moves in lock step. Output-pixel layers (Background, Energy) are unaffected.

Frame: `pipeline.BeginFrame()`, draw into `pipeline.Target(Layer::...)` and submit energy, then `pipeline.Compose(target)`. Call `Resize(size)` when the output size changes.

## Color script

A script (`assets/palettes/*.json`) holds named ramps (ordered stops, shadow to light) and named roles pointing at ramps (`background`, `ambient`, `ambient2`, `combatWarm`, `player`, `enemy`, `hull`, `planet`; roles are free-form). Shadows are authored hue-shifted (turquoise to deep blue, pink to maroon), never gray. A missing ramp or role samples as loud magenta. Ramp stops are `"#RRGGBB"` strings (evenly spaced) or `{"t": 0.3, "color": "#RRGGBB"}`. Switch at runtime with `assets.Scripts().SetActive(name)` or `CycleActive()`.

## Luminance tiers and effect budget

`LuminanceTier`: Background < Environment < Decorative < EnemyProjectile < Player. `PipelineSettings::tierMaxBrightness` caps the peak channel of every energy draw of a tier; the caps must not decrease with tier (a settings file that violates this is rejected). Over-bright draws are scaled down, not dropped. Energy draws are queued, then at `Compose` sorted by survival rank (Player, EnemyProjectile, Environment, Background, Decorative) and drawn until `maxEnergyDraws` or `maxEnergyAreaRatio` (sum of draw area / output area) is reached; the rest is culled, so decorative effects go first. `pipeline.Stats()` reports submitted, drawn, culled and clamped counts. Bloom is computed from the energy layer only, so only intentional light blooms. Bloom, flash and vignette strength are settings.

## Settings and hot reload

`assets/pipeline.json` maps onto `PipelineSettings` (`virtualHeight`, dither, bloom threshold, strength and levels, vignette, flash scale, energy budget, tier caps, and the `planet` section: `enabled`, `screenX`/`screenY` (fractions of the output at the camera start), `radius` (fraction of output height), `parallax`, `brightness`, `saturation`, `rotationSpeed`, `dither`). `VisualAssets` owns the loaded settings, shaders (`assets/shaders/<name>.frag`) and palettes. `assets.Poll(dt)` checks file timestamps twice a second and `assets.Reload(true)` forces a reload (F8 in the gallery). A file that fails to parse or compile keeps its previous working version and the error text lands in `ReloadReport::errors`. Settings are plain data: edit `assets.Settings()` at runtime; a reload of `pipeline.json` overwrites such edits.

## Gallery

```
LightYearsVisualGallery [--assets <dir>] [--script <name>]
LightYearsVisualGallery --capture <out.png> [--script <name>] [--frames N] [--size WxH] [--stress]
```

The scene shows all six ship definitions at full size (row 1: shields up, one in afterburner, a hit flash and shield ripple cycling through), the first ship at four damage stages plus two ships with mixed port states (row 2: unequipped, cooling down, ready, active, activation burst), and a game-size strip (row 3, about 20 to 28 world pixels across). Keys: `Tab` color script, `B` bloom, `D` dither, `V` virtual resolution, `Space` energy-budget stress, `Up`/`Down` bloom strength, `F8` reload, `Esc` quit. The gallery reads the source `assets/` folder directly (`--assets` overrides) so edits hot-reload without rebuilding. Capture mode needs no window: it renders N fixed 1/60 s frames and saves the composited image.

Build: part of the root CMake project; the gallery is behind `LIGHTYEARS_BUILD_VISUAL_GALLERY` (default ON).

## Ships (V3)

A ship is data (`assets/ships/*.json`, one ship per file, hot-reloaded with the other assets, `assets.Ships()`), drawn by `ShipRenderer::Draw(pipeline, definition, state)`. The library never sees game types: a definition has an opaque `id` and optional opaque `bindings` (the game lists its ship content ids there; `Ships().FindByBinding(id)`), so the id-to-visual mapping lives in these files and not in game content.

Definition fields (local units, nose toward -y, about 100 units across): `scale` (multiplier on the caller's scale), `roles` (palette roles sampled by part roles: `hull`, `plate`, `accent`, `glass`), `shield` (bubble semi-axes), `smoke`, `parts`, `lines`, `lights`, `engines`, `ports` (exactly 4: ability slots Q, E, F, R). A part is a polygon with `name`, `points`, `role` (`hull`, `plate`, `dark`, `accent`, `trim`, `glass`), `shade` (ramp stop offset), `gap` (dark outline over its neighbors), `crack`, `seed`, `detach` (damage stage from which it is missing; a pair is right/left) and `mirror` (points describe the right half; the left copy is expanded at load with a `_L` name suffix; the lit side keeps its shade, the other side is one stop darker). Colors are ramp stops, so every hull color is on the dither palette; the dither shader leaves exact palette colors flat. `size` (bounds of the parts) is computed at load so a caller can fit the design to another footprint.

`ShipVisualState` is filled by the caller each frame: position (output pixels), rotation, scale (output pixels per local unit), `hullFraction`, `shieldFraction`, `hitFlash` and `shieldHit` (1 to 0 timers), `thrust`, `afterburner`, `ghost` (spawn protection flicker), four `ShipPortState` (`equipped`, `category`, `cooldownFraction` 0 just started to 1 done, `isReady`, `isActive`, `justActivated` 1 to 0), `time` and `seed`. Nothing is retained between frames.

Behavior: hull and outline go to the dithered World layer; engines (`SubmitTrail` plus `SubmitGlow`), lights, ports and sparks are Player-tier energy (sparks Decorative); the shield is a custom additive draw (`SubmitEnergy`, shader `shield.frag`). Damage stage from the hull fraction: above 75% clean; above 50% plates one stop darker and cracks; above 25% sparks, a dark scar where a plate is missing, more cracks; below that more missing parts, flickering lights and ports, dark smoke wisps drawn under the hull. All of it is deterministic from part seeds, the ship seed and `time`. Shield: faint shell and rim following the fraction (flickers when nearly empty, nothing at zero), rim brightens with a ripple on `shieldHit`. Ports: unequipped is a dark socket; equipped is lit in the category color (palette roles `portPrimary`, `portOffense`, `portDefense`, `portMovement`, `portUtility`, defined in each palette); cooling down is a dim socket with a pie filling clockwise and a dim glow growing with progress; ready pulses; active is bright; `justActivated` adds a bright burst and an expanding ring. Level of detail: ships under about 34 world pixels across drop the dark gaps, under 28 the panel lines.

## Game integration (pass 1)

The library stays independent of the engine. `LightYearsEngine` only knows the small `WorldRenderPipeline` interface (`framework/WorldRenderPipeline.h`); `LightYearsGame` implements it (`presentation/visual/GameVisualPipeline`) on top of this library. `World::Render` draws `RenderLayer::Background` into the Background layer (the pipeline paints it itself, so old PNG background actors are skipped, not deleted) and every other render layer into the undithered pixel Sprites layer with the pixel-snapped camera view (the procedural planet uses the dithered World layer), composes onto the window, then draws the HUD at full resolution. `ProceduralBackdrop` paints nebula, stars and a small distant planet (tuned by the `planet` section of `pipeline.json`, anchored to the camera start of each world) with camera parallax; `EnergyLightPresenter` submits engine and projectile glows from a read-only look at the world.

Keys in the game: `F7` cycles the color script, `F8` toggles the pipeline (starts ON; a failed asset load keeps it OFF with a log line), `F9` forces an asset reload; files are also polled every frame. The game currently reads `assets/` from the source tree (`LY_VISUAL_ASSETS_DIR`); a packaged copy step can replace that later. Dev command line: `LightYearsGame.exe --visual-capture <png> [--capture-frames N] [--visual-off] [--visual-script <name>] [--visual-player-hull <0..1>] [--visual-ship <0..5>]` (`--visual-player-hull` only changes the hull fraction the procedural ship shows; `--visual-ship` starts with that player ship).

Player ship (V3): `ShipVisualPresenter` reads the current player ship (transform, velocity, health and shield fractions, afterburner, invulnerability, the four ability slots through the same `GetAbility(slot)` read the HUD bar uses) and draws it with `ShipRenderer`. Hits are noticed by watching the health and shield fractions drop, so nothing is bound or written back. The selected ship content id picks the definition through the `bindings` of the ship JSON. While the pipeline is ON the world skips that ship's sprite through `WorldRenderPipeline::HidesActor` (a draw-time decision of the pipeline; the actor's render flags, physics and collision are untouched, and with the pipeline OFF the old path runs unchanged). The visual is fitted to the sprite footprint by area times the definition `scale` (1.25 in the shipped data, an art choice for readability; set it to 1 for the sprite footprint). The old player engine glow in `EnergyLightPresenter` is gone; the ship renderer owns it.
