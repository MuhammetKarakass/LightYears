# LightYearsVisual

Standalone static library (`LightYearsVisual`, namespace `ly::visual`) for procedural, sprite-free visuals drawn with SFML shapes, vertex arrays and GLSL `#version 120` shaders. It depends only on SFML and nlohmann_json, not on `LightYearsEngine` or the game. Status: V1 foundation, not integrated into the game yet (see `docs/VISUAL_LIBRARY_PLAN.md`).

## Layers

`VisualPipeline` owns three render textures and composites them in this order: background, world, energy, bloom, flash, vignette. All drawing coordinates are output pixels; the pipeline maps them onto each layer.

| Layer | Resolution | Look | Draw with |
| --- | --- | --- | --- |
| Background | full | soft, low-frequency nebula, haze, sparse sharp stars, parallax | `DrawGradient`, `DrawNebula`, `StarField` |
| World | virtual (`virtualHeight`, e.g. 240 rows), upscaled nearest | crisp pixels, hard silhouettes, outlines, optional palette dither | `ShapeBrush` (use `hardEdges`), planet shader |
| Energy | full, additive | tiny bright core, limited halo, dark surroundings, bloom source | `SubmitGlow`, `SubmitTrail`, `SubmitFlash` |

Frame: `pipeline.BeginFrame()`, draw into `pipeline.Target(Layer::...)` and submit energy, then `pipeline.Compose(target)`. Call `Resize(size)` when the output size changes.

## Color script

A script (`assets/palettes/*.json`) holds named ramps (ordered stops, shadow to light) and named roles pointing at ramps (`background`, `ambient`, `ambient2`, `combatWarm`, `player`, `enemy`, `hull`, `planet`; roles are free-form). Shadows are authored hue-shifted (turquoise to deep blue, pink to maroon), never gray. A missing ramp or role samples as loud magenta. Ramp stops are `"#RRGGBB"` strings (evenly spaced) or `{"t": 0.3, "color": "#RRGGBB"}`. Switch at runtime with `assets.Scripts().SetActive(name)` or `CycleActive()`.

## Luminance tiers and effect budget

`LuminanceTier`: Background < Environment < Decorative < EnemyProjectile < Player. `PipelineSettings::tierMaxBrightness` caps the peak channel of every energy draw of a tier; the caps must not decrease with tier (a settings file that violates this is rejected). Over-bright draws are scaled down, not dropped. Energy draws are queued, then at `Compose` sorted by survival rank (Player, EnemyProjectile, Environment, Background, Decorative) and drawn until `maxEnergyDraws` or `maxEnergyAreaRatio` (sum of draw area / output area) is reached; the rest is culled, so decorative effects go first. `pipeline.Stats()` reports submitted, drawn, culled and clamped counts. Bloom is computed from the energy layer only, so only intentional light blooms. Bloom, flash and vignette strength are settings.

## Settings and hot reload

`assets/pipeline.json` maps onto `PipelineSettings` (`virtualHeight`, dither, bloom threshold, strength and levels, vignette, flash scale, energy budget, tier caps). `VisualAssets` owns the loaded settings, shaders (`assets/shaders/<name>.frag`) and palettes. `assets.Poll(dt)` checks file timestamps twice a second and `assets.Reload(true)` forces a reload (F8 in the gallery). A file that fails to parse or compile keeps its previous working version and the error text lands in `ReloadReport::errors`. Settings are plain data: edit `assets.Settings()` at runtime; a reload of `pipeline.json` overwrites such edits.

## Gallery

```
LightYearsVisualGallery [--assets <dir>] [--script <name>]
LightYearsVisualGallery --capture <out.png> [--script <name>] [--frames N] [--size WxH] [--stress]
```

Keys: `Tab` color script, `B` bloom, `D` dither, `V` virtual resolution, `Space` energy-budget stress, `Up`/`Down` bloom strength, `F8` reload, `Esc` quit. The gallery reads the source `assets/` folder directly (`--assets` overrides) so edits hot-reload without rebuilding. Capture mode needs no window: it renders N fixed 1/60 s frames and saves the composited image.

Build: part of the root CMake project; the gallery is behind `LIGHTYEARS_BUILD_VISUAL_GALLERY` (default ON).
