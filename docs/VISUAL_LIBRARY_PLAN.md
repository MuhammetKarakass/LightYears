# Visual Library Plan

Status: V1 and V3 are in the source tree (`LightYearsVisual/`), and the first game integration pass and the V3 player-ship integration are done; V2, V4 and V5 are planned and not implemented.

## Direction

The game gets a procedural visual library instead of sprites: shapes, vertex arrays and GLSL shaders. Three visual languages are layered:

- World layer: rendered at a low virtual resolution and upscaled nearest-neighbor; hard silhouettes and outlines.
- Background layer: soft, low-frequency, atmospheric; sparse sharp stars; parallax-capable; full resolution.
- Energy layer: additive light, tiny very bright core, limited halo, dark surroundings; bloom; full resolution.

Color is organized as a color script of named ramps (shadow to light, shadows hue-shifted, never gray) and roles; cool ambient (cyan, violet, magenta) against warm combat (amber, orange, red). Readability and eye comfort are hard requirements, stricter than the reference games (Solfall, Animal Well): luminance tiers, an effect budget, adjustable bloom, flash and vignette strength.

## Background art direction (2026-10-13)

LightYears uses dark space, softly coloured star clouds and crisp combat light. The Milky Way, planetary surfaces and physical star colours keep the setting recognizable; authored colour ramps and broad asymmetrical shapes give it a stylized identity. The background should reward looking at it while letting a player track the ship and incoming threats during fast combat.

- Keep most background detail broad and soft. Small stars can be numerous enough to convey space, but remain points with limited brightness; long flares and a dense carpet of photographic grain do not define the scene.
- Blend cool indigo/azure light into rose, amber and cream over wide transitions. A faint rose/lavender bridge softens the yellow/blue boundary; it does not recolour every region violet. Preserve darker dust lanes and the distinction between warm old-star light and cool young-star clouds.
- Use the brightest, sharpest light for combat. Background colour richness comes from hue and large shapes rather than raising all its brightness. Avoid applying background blending to the HUD, ships or projectiles.
- Give a location a stable composition and slow camera depth. Planet type, band orientation, dust and local celestial features provide variation; continuous flashing, whole-galaxy animation and a repeated symmetric fog frame are outside this direction.

Normal-flight movement is conveyed by distinct depth speeds: dim nearby dust and close stars pass faster, the planet moves moderately, and the galactic band barely moves. Dust drifts gently at rest so the sky is not a frozen image. These cues use the continuous game camera and the existing background `NearField` mote renderer; their brightness and size stay below combat signals. Ship controls and camera-follow behaviour are not changed by this presentation pass.

The reference set includes games outside space shooters and sources outside Steam. Solfall here means the browser MMO at [solfall.online](https://solfall.online/), not the unrelated survival-horror game named SolFall on Steam.

| Reference and evidence | What it contributes to LightYears |
| --- | --- |
| [Solfall developer comparison and audience feedback](https://www.reddit.com/r/IndieDev/comments/1v2tgtc/does_the_game_look_better_now/): readers praise the background and pixel art, while others prefer the older version's cohesion and criticize sharpness, mixed pixel scales and battle clutter. | Keep recognizable material surfaces and depth; make the sharp/soft layer split deliberate. Extra detail must not hide gameplay. |
| [A Short Hike on itch.io](https://adamgryu.itch.io/a-short-hike), with [colour/atmosphere feedback](https://itch.io/t/1189138/a-tiny-and-lifeful-experience-like-a-japanese-garden): a player values its colourful, detailed, relaxing world. This is an exploration game, so its pacing does not transfer to combat. | Colour can be rich and welcoming while large shapes and gentle transitions keep the scene calm. This is an interpretation for our background. |
| [Animal Well, described by its developer](https://blog.playstation.com/2022/07/20/how-animal-well-taps-into-ps5-hardware-to-elevate-2d-pixel-art-platforming/): pixel art is combined with layered background effects and lighting. | Retain the crisp gameplay layer while soft light and atmospheric depth enrich the background. No new render framework is implied. |
| [Nova Drift developer retrospective](https://blog.novadrift.io/novadrift-released/) and [player reviews](https://steamcommunity.com/app/858210/reviews/?browsefilter=toprated): deliberate entity/background contrast, audiovisual praise and useful threat cues. [Geometry Wars 3 feedback](https://steamcommunity.com/app/310790/reviews/?browsefilter=toprated) includes criticism of obscured visibility and warnings. | The brightest and clearest information remains the ship, threat and impact. Visual spectacle is judged in motion as well as in a still image. |

This is an art decision informed by a small qualitative sample, not a population survey or a guarantee of appeal. Our chosen treatment is soft, coloured celestial depth around sharply readable combat. The Milky Way blends indigo/azure through a faint rose/lavender transition toward amber/cream, with darker dust preserving structure. Stars regain some density and brightness while the background luminance ceiling stays below combat. The canonical numeric tuning remains in `LightYearsVisual/assets/pipeline.json` and the galaxy colour construction in `assets/shaders/galaxy.frag`.

## V1 (done): foundation

Where: `LightYearsVisual/` (static library `LightYearsVisual`, namespace `vega::visual`) and the viewer `LightYearsVisualGallery` (CMake option `LIGHTYEARS_BUILD_VISUAL_GALLERY`). Depends on SFML and nlohmann_json only. See `LightYearsVisual/README.md` for usage.

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
