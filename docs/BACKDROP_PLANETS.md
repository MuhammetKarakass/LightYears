# Backdrop planets — Stage 1

Stage 1 extracts normal-planet uniform preparation and submission into
`ProceduralBackdrop::DrawPlanetBody`, using a resolved `PlanetBodyDescription`.
The ordinary planet remains entry 0: its seeded composition, authored mirroring,
jitter, default surface seed, `time * rotationSpeed * 4` rotation, moon seed and
back-moon → body → front-moon order are preserved. It alone updates
`mPlanetCenter`, `mPlanetRadius` and `mStarDirection`; the shattered moon remains
attached to it. `DrawOrbitBackground` is unchanged and bypasses the extra list.
There are no asset, parser, editor, shader or reflection changes in this stage.
The editor cannot yet change the planet count.

## Test seam

Only the friend fixture in
[`BackdropTestAccess.h`](../LightYearsGame/tests/BackdropTestAccess.h) exposes
`ProceduralBackdropTestAccess::SetAdditionalPlanetBodies(backdrop, bodies)`.
The list is per backdrop, empty in shipped execution and limited by the fixture
to seven extras (eight including entry 0). Each description has an output-pixel
`center` and `radius`, `kind`, independent surface `seed` and `moonSeed`, signed
`rotationSpeed` in radians/second, `rotationScale` (normally 1), `visible`, `moon`
and optional `ring`. Positions are resolved test inputs; they receive the normal
camera parallax, without additional mirroring or random jitter. Each body resolves
its own light direction toward the common host star and uses the pipeline's
appearance settings. Later list entries are drawn in front of earlier entries,
after entry 0 and its shattered-moon attachment, before foreground clouds.

Extras call Vega's existing DrawPlanet/DrawPlanetRing/DrawPlanetMoon functions in
back-ring → back-moon → body → front-ring → front-moon order, omitting disabled
attachments. The fixture's observer records parent parameters, sampled ramp,
moon seed and submission order immediately before those calls. It observes the
game-to-Vega submission boundary, rather than intercepting OpenGL uploads or
the moon's internal derived parameters. Vega remains read-only.

## Deterministic capture and equivalence

The test-only `LightYearsBackdropCapture` executable uses the editor capture's
960×540 offscreen render texture, game backdrop and Vega visual pipeline, with
a 1920×1080 view centered at (960,540). Every case first draws at time zero with
that camera to anchor parallax and start the location/orbit clocks. It then draws
at the requested time and camera; the shifted center is (1330,330). No visible
window or wall clock is used.

Run from `D:\LightYears` in PowerShell:

```powershell
tools\vega-build.cmd --target LightYearsBackdropCapture --parallel 4
build\LightYearsGame\LightYearsBackdropCapture.exe --capture build\w10-backdrop-baseline\run1
build\LightYearsGame\LightYearsBackdropCapture.exe --capture build\w10-backdrop-baseline\run2
build\LightYearsGame\LightYearsBackdropCapture.exe --compare build\w10-backdrop-baseline\run1 build\w10-backdrop-baseline\run2
# Preserve these pre-change captures. After changing/rebuilding the renderer:
build\LightYearsGame\LightYearsBackdropCapture.exe --capture build\w10-backdrop-after
build\LightYearsGame\LightYearsBackdropCapture.exe --compare build\w10-backdrop-baseline\run1 build\w10-backdrop-after
ctest --test-dir build -R '^LightYearsBackdropPlanetsE2E$' --output-on-failure
```

Bulk capture enumerates all 55 authored deepsky stop IDs and resolves each through
the same route construction as the game. All have time-zero captures at both
cameras; the 51 stops with a route planet or authored planet/orbit also have
30/120-second captures at both cameras, totaling 314 PNGs. Each `manifest.json`
contains FNV-1a-64 hashes of **every decoded RGBA8 byte**, in row-major order.
PNG round trips are verified during capture. `--compare` reloads both sets,
checks their manifests and compares every decoded byte, writing `comparison.json`
and failing on any difference. The baseline's `determinism.json` and source
SHA-256 inventory are local artifacts, not portable GPU-independent goldens.
Post-refactor `body-submissions.json` provides parameter/order/owner snapshots;
the original two baseline passes predate that observer. A further reference run
uses the saved pre-refactor source with observation calls added immediately before
its three normal-body submissions. Its source, build script and snapshots are
preserved under `build/w10-backdrop-baseline/` and
`build/w10-backdrop-reference-build.cmd`, allowing direct comparison of entry-0
parameters/order as well as original pixels. This reference executable is a
local verification artifact, outside the shipped CMake targets.

One case can also be captured explicitly:

```powershell
build\LightYearsGame\LightYearsBackdropCapture.exe --capture build\sky-case gc-ringed-giant 120 shifted
```

`LightYearsBackdropPlanetsE2E` writes one/two-body PNG pairs and `two-body.json`
under `build/w10-backdrop-two-body`. Twelve cases cover 0/30/120 seconds, both
cameras, and bare versus ring/moon bodies with opposite rotation signs. It
checks all entry-0 parameters and submissions, owner state, exact pixels in the
primary body's clipped bounding region, second-body visibility, and unchanged
pixels outside the extra body's expected region. Ordinary pipeline enablement
still applies; the seam does not change the orbit scene.

## Remaining stages

The read-only design is
[`task-092-sky-multi-planet-design.out.md`](../../Vega/.sol/task-092-sky-multi-planet-design.out.md).

- **Stage 2 (Vega parser/resolver, then game adoption):** introduce typed planet
  entries, resolve inherited append semantics and legacy entry 0, distinguish
  empty arrays from generated fallback, and validate the effective eight-body
  limit transactionally. Define scene-orbit ownership, per-body motion/framing
  and ring constraints while retaining legacy behavior.
- **Stage 3 (Vega editor and game adapter):** add count/add/duplicate/remove/
  reorder/select controls, indexed handles, owner labeling, signed speeds,
  preset materialization, undo/redo and byte-preserving save/reopen.
- **Stage 4 (regression/docs):** extend parser/document/editor scripts and
  captures, retain legacy equivalence at animated times, update engine/module
  documentation and sample content, and review performance/visual overlap.

Engine work requires a separate Vega task. Stage 1 proves offscreen rendering
on the capture machine; interactive editor acceptance and GPU cost remain open.
