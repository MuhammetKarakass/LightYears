# Visual Library Plan

Status: planning document. Section "V1" describes what exists in the source tree (`LightYearsVisual/`); V2 to V5 are planned and not implemented.

## Direction

The game gets a procedural visual library instead of sprites: shapes, vertex arrays and GLSL shaders. Three visual languages are layered:

- World layer: rendered at a low virtual resolution and upscaled nearest-neighbor; hard silhouettes and outlines.
- Background layer: soft, low-frequency, atmospheric; sparse sharp stars; parallax-capable; full resolution.
- Energy layer: additive light, tiny very bright core, limited halo, dark surroundings; bloom; full resolution.

Color is organized as a color script of named ramps (shadow to light, shadows hue-shifted, never gray) and roles; cool ambient (cyan, violet, magenta) against warm combat (amber, orange, red). Readability and eye comfort are hard requirements, stricter than the reference games (Solfall, Animal Well): luminance tiers, an effect budget, adjustable bloom, flash and vignette strength.

## V1 (done): foundation

Where: `LightYearsVisual/` (static library `LightYearsVisual`, namespace `ly::visual`) and the viewer `LightYearsVisualGallery` (CMake option `LIGHTYEARS_BUILD_VISUAL_GALLERY`). Depends on SFML and nlohmann_json only. See `LightYearsVisual/README.md` for usage.

- Color script: `ColorRamp`, `ColorScript`, `ColorScriptLibrary`, JSON palettes, runtime switch.
- Pipeline: `VisualPipeline` with background, world (virtual resolution, nearest upscale, optional palette-snapping Bayer dither), energy (additive), bloom (bright pass plus downsampled separable blur levels), flash, vignette; `PipelineSettings` loadable from JSON and editable at runtime.
- Primitives: `ShapeBrush` (SDF circle, ring, capsule, arc with outline and pixel-snapped edges; ear-clipped polygon with outline), `SubmitGlow` and `SubmitTrail` energy primitives, `Backdrop` (gradient, nebula shader, parallax star field).
- Readability: `LuminanceTier`, per-tier brightness caps, per-frame energy budget (draw count and area) with decorative culled first.
- Hot reload: `VisualAssets` polls timestamps for shaders, palettes and `pipeline.json`; a failed reload keeps the previous version.
- Gallery scene and headless `--capture` mode used for visual inspection.

Known limits: layers are 8-bit (no HDR render targets in SFML), so bloom thresholds work on clamped values; the dither palette snap works on the whole script palette, so near-palette colors can flip to a neighboring ramp; every shape is its own draw call (no batching); no unit tests (tests are disabled in this build).

## V2: SDF shapes and effects

- Extend the SDF set (rounded box, triangle, chevron, polyline with round joins, shape union and subtraction) and batch shapes that share a shader.
- Effects: explosion (flash, shock ring, debris sparks, smoke in the world layer) and light pulse (expanding additive ring with fade), driven by data and the effect budget.
- Particle emitter on vertex arrays, tier-aware.

## V3: procedural ship from data

- A ship is described by data (hull polygons, mirror rules, panel lines, lights, engines) and drawn through `ShapeBrush`, with outline and ramp-based lighting.
- Damage layers (cracks, scorch, missing panels, flicker) driven by a health fraction.
- Ability slot ports: named anchor points on the ship where ability visuals attach.

## V4: black hole, planets, light and darkness

- Black hole: lensing distortion shader, accretion ring in the energy layer, event-horizon silhouette in the world layer.
- Planets: promote the gallery planet shader to a library primitive (rings, atmosphere, city lights on the dark side, moons).
- Light and darkness layer: a darkness multiply pass with additive light sources, kept inside the tier and budget rules.

## V5: pilot integration into the game

The library becomes the backend of the existing presentation contract, not a replacement. `AbilityActorDefinition` keeps one `presentationProfileId` resolved through `PresentationProfileRegistry<ConcreteProfile>`; profiles register in `RegisterGameAbilityPresentationContent()`. A pilot picks one ability family, adds a feature-local profile that references a library effect, and renders through `VisualPipeline` layers owned by the world. Requirements before the pilot: link `LightYearsVisual` into `LightYearsGame`, add asset copy for `LightYearsVisual/assets`, decide ownership of the pipeline (world-owned service), map gameplay events to energy submissions without feeding presentation back into gameplay state, and add registration and lifecycle tests per `docs/PROJECT_DOCUMENTATION.md` section 3.0.1.
