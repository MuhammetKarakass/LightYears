# LightYearsVisual

Standalone static library (`LightYearsVisual`, namespace `ly::visual`) for procedural, sprite-free visuals drawn with SFML shapes, vertex arrays and GLSL `#version 120` shaders. It depends only on SFML and nlohmann_json, not on `LightYearsEngine` or the game. Status: V1 foundation; first game integration pass done (see "Game integration" below and `docs/VISUAL_LIBRARY_PLAN.md`).

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

Keys: `Tab` color script, `B` bloom, `D` dither, `V` virtual resolution, `Space` energy-budget stress, `Up`/`Down` bloom strength, `F8` reload, `Esc` quit. The gallery reads the source `assets/` folder directly (`--assets` overrides) so edits hot-reload without rebuilding. Capture mode needs no window: it renders N fixed 1/60 s frames and saves the composited image.

Build: part of the root CMake project; the gallery is behind `LIGHTYEARS_BUILD_VISUAL_GALLERY` (default ON).

## Game integration (pass 1)

The library stays independent of the engine. `LightYearsEngine` only knows the small `WorldRenderPipeline` interface (`framework/WorldRenderPipeline.h`); `LightYearsGame` implements it (`presentation/visual/GameVisualPipeline`) on top of this library. `World::Render` draws `RenderLayer::Background` into the Background layer (the pipeline paints it itself, so old PNG background actors are skipped, not deleted) and every other render layer into the undithered pixel Sprites layer with the pixel-snapped camera view (the procedural planet uses the dithered World layer), composes onto the window, then draws the HUD at full resolution. `ProceduralBackdrop` paints nebula, stars and a small distant planet (tuned by the `planet` section of `pipeline.json`, anchored to the camera start of each world) with camera parallax; `EnergyLightPresenter` submits engine and projectile glows from a read-only look at the world.

Keys in the game: `F7` cycles the color script, `F8` toggles the pipeline (starts ON; a failed asset load keeps it OFF with a log line), `F9` forces an asset reload; files are also polled every frame. The game currently reads `assets/` from the source tree (`LY_VISUAL_ASSETS_DIR`); a packaged copy step can replace that later. Dev command line: `LightYearsGame.exe --visual-capture <png> [--capture-frames N] [--visual-off] [--visual-script <name>]`.
