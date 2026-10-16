# Sky editor

Open **View > Sky** in scene Edit mode. The panel opens the active visual asset
root's `deepsky.json`, initially at the scene's `routeStop`. Debug builds normally
use `LightYearsVisual/assets`; `--visual-assets <directory>` chooses another root.
`--asset-root` independently chooses the game/scene asset root.

The preview picker and thumbnail strip change the running viewport's stop without
changing scene settings. **Use in scene** makes the preview stop the scene's
`routeStop`, using the existing undoable World Settings command. Changing
`routeStop` in World Settings also selects that stop in Sky. Play and prefab mode
suspend sky authoring and restore the scene's backdrop. Returning to Edit resumes
the document. Closing Sky restores the scene's stop and the disk catalog.

## Fields and presets

Expand band, skyPaint, planet, orbit, hostStar, grade, asteroids or Objects.
Number, integer, boolean, RGB, position and enum controls come from
`ly::visual::DeepSkyFields()` in `LightYearsVisual/src/DeepSky.cpp`. Object kinds
come from the renderer's existing `kKinds`; planet kinds are checked through
`ParsePlanetKind`. Numeric parser clamps share the table with the authoring
validator. Angles and positions that the parser leaves unbounded remain
unbounded. Ring outer radius must also be at least inner radius + 0.05.

The parser's field spelling is authoritative: position is `at`, hiding is
`hide`, object type is `kind`, stars use `stars`, and rotation uses `angleDeg`.
Orbit framing is `horizon` or `disk` (the normal framing is `horizon`). The
existing orbit behavior suppresses objects, nebula, skyPaint and asteroids.
Editing these while an orbit is active preserves their data for later use.

Viewing a field never adds an override. Hover shows whether a value is authored
or inherited/generated. Right-click a field to reset its override to its preset
or generated default. Object `kind` resets to `emission`, since the runtime
requires a kind. Unknown keys remain in the document and appear read-only under
**Other**. Legacy nebula ramp layers are also retained there.

**Presets and duplicate** can store the current effective sky as a named preset,
edit an existing preset, apply that preset to the preview stop, or duplicate the
authored sky to another route stop. Preset editing previews that preset on the
picked route stop; stops referencing it update together. Runtime object/nebula
arrays append to preset arrays. Editing an inherited object therefore materializes
the effective sky and detaches that stop from its preset, avoiding duplicate
objects. This is undoable. Copy/Paste block transfers a matching band, planet,
orbit, ring, skyPaint, hostStar, grade, asteroids or object block within the panel.
Removing a block removes its override. Object rows also support duplication.

## Viewport handles

Focus Sky, leave **Sky handles** enabled, then drag in Viewport. Focusing another
panel returns control to entity tools; focusing Sky restores Sky mode. Sky mode
owns viewport left drags and disables entity gizmos, selection and entity drops.
Each gesture has one undo entry, even when it spans many frames.

* Planet: drag its center to move; drag its scale ring to resize.
* Objects: center moves, ring resizes, outer rotation point rotates.
* Band: drag the line's center to offset it or its angle point to turn it.
* Orbit disk: drag the radius point; horizon: drag coverage or curvature points.
  The orbit-direction point rotates either framing.

Mirrored stops show a notice. Planet X and band angles follow the renderer's
mirroring; object positions/angles follow the `mirror` flag. Drags convert back to
authored coordinates. The geometry accessor reads the existing seeded composition
and camera drift. Off-screen handles can be brought back using the field controls.
Snap rounds positions/offsets and object/disk radii to 0.01, planet scale to 0.05,
angles to 15 degrees and horizon radius to 0.1. Hold **B** while Sky or Viewport is
focused to compare the saved sky; release it to return to the edits. Comparison
does not change the document or undo history.

## Preview, saving and costs

Document edits use `JsonDocument` and structural `ReplaceJsonCommand` commands.
Parsed catalogs, form rows and unknown-key text are cached by document revision.
Ordinary stop edits compile only changed stops against the current presets;
preset edits and undo/reopen rebuild the complete catalog.
There is no per-frame JSON parsing or preview file write. The editor applies a
validated catalog to `VisualAssets`, using the same versioned parser/cache path
as file reload. Only deep-sky file polling is suspended during a preview; shaders,
palettes and pipeline settings retain their existing reload behavior.

Ordinary edits reach the next viewport render. Orbit edits wait for 180 ms of idle
time before applying, with a **Rebuilding** hint while pending. The final bake
still runs synchronously through the existing renderer. The thumbnail strip
renders only visible entries, at most one stale thumbnail per frame, at 160 x 90.
Textures are cached until the applied document revision changes. No thumbnail
job changes the live route stop.

**Save sky** or Ctrl+S while Sky/Viewport is active writes atomically using the
existing byte-preserving JSON serializer. Untouched entries retain their exact
text, key order, numeric spelling and line endings. Changed containers use the
file's original indentation and newline convention. The game picks up the saved
file through its existing hot reload. Save refuses a conflicting external disk
edit. Closing, reverting or exiting with dirty sky data offers Save, Discard and
Cancel. Sky history is separate from scene and Data Editor history.

The panel and script reports expose parse, catalog-copy/apply, off-screen render
and thumbnail CPU times. Render numbers measure CPU submission, including cache
rebuild submission; they are not GPU timer queries. See the checked-in test
commands below and the generated `.report.txt` for measurements on your machine.
An orbit bake can still cause a single long frame after the debounce interval.

## Script mode and tests

Run only from `build/VegaEditor`, with `--editor-script`:

```text
sky-open <route-stop-id>
sky-set <field.path> <JSON-value>
sky-drag <handle> <dx> <dy>
sky-save
expect-sky <field.path> <JSON-value>
capture-sky <png-path>
```

Paths accept `objects[0].at`, `objects.0.at` or `/objects/0/at`. Quote enum strings
as JSON, for example `sky-set orbit.framing "disk"`. Values can be numbers,
booleans, strings or arrays. Unknown fields, wrong types, invalid enums and
out-of-range values fail instead of silently clamping. Expectations compare
numbers with a 1e-5 tolerance. `undo` and `redo` operate on Sky while it is active.
Opening another stop preserves dirty edits; `sky-open` on a clean document rereads
the file and starts fresh history.

Handle names are `planet`, `planet.scale`, `object.<index>`,
`object.<index>.scale`, `object.<index>.rotate`, `band.angle`, `band.offset`,
`orbit.disk`, `orbit.horizon`, `orbit.angle`, and `orbit.radius`. Script deltas
are pixels of the viewport render texture. Move handles use X/Y displacement;
scale/disk handles use Y displacement; rotation uses horizontal displacement
(one full texture width = 360 degrees); curvature uses horizontal displacement
(one texture width = 10 output heights). Band offset projects the displacement
onto the band's normal. UI rotation and scale handles use angular/radial motion.

`capture-sky` saves a deterministic 960 x 540 backdrop, at time zero, without
entities, overlays or editor chrome, through the existing `ProceduralBackdrop`
and `VisualPipeline`. Before `sky-open` it captures the scene's disk sky, allowing
unchanged-data comparison. Captures explicitly flush pending preview edits.
Use the ordinary `capture` command for the full editor UI.

`VegaEditorSkyDocument` covers schema defaults, invalid edits, preset arrays,
gesture merging, reset, duplication, block validation, atomic saves, external
conflicts, unknown data and exact untouched bytes. The extended
`VegaEditorScript` parser suite checks all six commands and malformed inputs.
`VegaEditorSkyEditor` runs `sky-editor.vegascript` on fresh build-tree copies of
both game and visual assets. It verifies unchanged-data rendering, planet and
band pixel changes, undo's downscaled checksum, save/reopen, unrelated stop text,
object handles and an orbit disk/ring/moon preview. Artifacts are under
`build/VegaEditor/sky-editor-test`; the report is beside the generated script.

```text
cmd /c tools\vega-build.cmd
ctest --test-dir build -R "VegaEditorSky|VegaEditorScript$" --output-on-failure
ctest --test-dir build -j 8 --timeout 300
```

No shader, rendering order or effect was added or changed by this editor.
