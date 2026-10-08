# Visual Library Plan

Status: V1 and V3 are in the source tree (`LightYearsVisual/`), and the first game integration pass and the V3 player-ship integration are done; V2, V4 and V5 are planned and not implemented.

## Direction

The game gets a procedural visual library instead of sprites: shapes, vertex arrays and GLSL shaders. Three visual languages are layered:

- World layer: rendered at a low virtual resolution and upscaled nearest-neighbor; hard silhouettes and outlines.
- Background layer: soft, low-frequency, atmospheric; sparse sharp stars; parallax-capable; full resolution.
- Energy layer: additive light, tiny very bright core, limited halo, dark surroundings; bloom; full resolution.

Color is organized as a color script of named ramps (shadow to light, shadows hue-shifted, never gray) and roles; cool ambient (cyan, violet, magenta) against warm combat (amber, orange, red). Readability and eye comfort are hard requirements, stricter than the reference games (Solfall, Animal Well): luminance tiers, an effect budget, adjustable bloom, flash and vignette strength.

## V1 (done): foundation

Where: `LightYearsVisual/` (static library `LightYearsVisual`, namespace `ly::visual`) and the viewer `LightYearsVisualGallery` (CMake option `LIGHTYEARS_BUILD_VISUAL_GALLERY`). Depends on SFML and nlohmann_json only. See `LightYearsVisual/README.md` for usage.

- Color script: `ColorRamp`, `ColorScript`, `ColorScriptLibrary`, JSON palettes, runtime switch.
- Pipeline: `VisualPipeline` with background, world (virtual resolution, nearest upscale, optional palette-snapping Bayer dither), sprites (same virtual resolution, undithered, keeps original colors), energy (additive), bloom (bright pass plus downsampled separable blur levels), flash, vignette; `PipelineSettings` loadable from JSON and editable at runtime.
- Optional nebula downsampling uses `nebula.resolutionDivisor` (code default 1, shipped 4, clamped to 1–8): cloud layers draw to a transparent target, composite over Background, then stars draw above them. The gallery `N` key cycles 1, 2, 4 and 8.
- Primitives: `ShapeBrush` (SDF circle, ring, capsule, arc with outline and pixel-snapped edges; ear-clipped polygon with outline), `SubmitGlow` and `SubmitTrail` energy primitives, `Backdrop` (gradient, nebula shader, parallax star field).
- Readability: `LuminanceTier`, per-tier brightness caps, per-frame energy budget (draw count and area) with decorative culled first.
- Hot reload: `VisualAssets` polls timestamps for shaders, palettes and `pipeline.json`; a failed reload keeps the previous version.
- Gallery scene and headless `--capture` mode used for visual inspection.

Known limits: layers are 8-bit (no HDR render targets in SFML), so bloom thresholds work on clamped values; the dither palette snap works on the whole script palette, so near-palette colors can flip to a neighboring ramp; every shape is its own draw call (no batching); no unit tests (tests are disabled in this build).

## V2: SDF shapes and effects

- Extend the SDF set (rounded box, triangle, chevron, polyline with round joins, shape union and subtraction) and batch shapes that share a shader.
- Effects: explosion (flash, shock ring, debris sparks, smoke in the world layer) and light pulse (expanding additive ring with fade), driven by data and the effect budget.
- Particle emitter on vertex arrays, tier-aware.

## V3 (done): procedural player ships from data

Where: `include/visual/ShipVisual.h`, `ShipRenderer.h`, `src/ShipVisual.cpp`, `ShipRenderer.cpp`, `assets/ships/*.json` (six player ships), `assets/shaders/shield.frag`; game side `presentation/visual/ShipVisualPresenter`. The data format is in `LightYearsVisual/README.md` ("Ships (V3)").

- A ship is data (hull polygons with mirror rules, panel lines, lights, engine mounts, four ability ports, shield size, per-part damage data) drawn into the dithered World layer with outline and ramp-stop colors; engines, lights, ports and the shield are energy.
- Damage in four stages from the hull fraction: darkened plates and cracks, sparks and scarred missing plates, then more missing parts, flicker and smoke. Deterministic per seed.
- Shield bubble following the shield fraction with a hit ripple. Ability ports: dark socket, category-colored light, cooldown pie, ready pulse, activation burst and ring.
- Library additions it uses: `VisualPipeline::SubmitEnergy` and `TierBrightnessCap`, `ShapeStyle::additive`, `VisualAssets::Ships()`, a dither palette of up to 64 colors with exact palette colors left flat, port ramps and roles in both palettes.
- The player ship uses it in the game; the sprite is skipped while the pipeline is ON through `WorldRenderPipeline::HidesActor`. Enemies keep their sprites.

Known limits: at game scale a ship is only about 20 to 28 world pixels across, so detail drops with the level of detail rules and port markers read as colored pixels; the visual is scaled 1.25 over the sprite footprint (a data choice); the presenter does not know the hit direction, so the shield ripple angle is a rolling value; the port category comes from the second token of the ability id.

## V4: black hole, planets, light and darkness

- Black hole: lensing distortion shader, accretion ring in the energy layer, event-horizon silhouette in the world layer.
- Planets: promote the gallery planet shader to a library primitive (rings, atmosphere, city lights on the dark side, moons).
- Light and darkness layer: a darkness multiply pass with additive light sources, kept inside the tier and budget rules.

## V5: pilot integration into the game

The library becomes the backend of the existing presentation contract, not a replacement. `AbilityActorDefinition` keeps one `presentationProfileId` resolved through `PresentationProfileRegistry<ConcreteProfile>`; profiles register in `RegisterGameAbilityPresentationContent()`. A pilot picks one ability family, adds a feature-local profile that references a library effect, and renders through `VisualPipeline` layers owned by the world. Requirements before the pilot: link `LightYearsVisual` into `LightYearsGame`, add asset copy for `LightYearsVisual/assets`, decide ownership of the pipeline (world-owned service), map gameplay events to energy submissions without feeding presentation back into gameplay state, and add registration and lifecycle tests per `docs/PROJECT_DOCUMENTATION.md` section 3.0.1.
