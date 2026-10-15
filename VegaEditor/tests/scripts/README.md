Run the editor script runner from the editor's executable directory.

The editor starts in **Edit** mode with `scenes/sandbox.scene.json`.
`--editor-scene <path>` selects another startup scene; a missing startup file
creates an empty scene. Relative scene paths resolve against the editor asset
root; absolute paths are accepted. Edit always stays paused, uses the editor
camera, and renders authored entities without gameplay BeginPlay, Lua callbacks,
players, HUDs, stages or gameplay timers. PlayerStart triangles and Ship labels
are editor overlays visible only in Edit.

```text
new-scene
open-scene <path>
save-scene [path]
create <SpawnableType>
add-component <Type>
remove-component <Type> [occurrence=0]
expect-component <Type> present|absent
create-many <SpawnableType> <0..10000>
delete
duplicate
rename <name...>
select-name <name...>
play-mode
stop
expect-mode edit|play
expect-entity-count <non-negative integer>
expect-scene-dirty true|false
```

`rename` and `select-name` join the remaining arguments with spaces; the usual
quoted names and comments work. `create` selects a new entity at the editor
camera centre with a new GUID and the first unused name `<Type> N`. `duplicate`
copies serialized entity data, selects the copy, generates a new GUID and adds
32 world units on both axes. Create, duplicate, delete, rename, transforms,
reflected properties and Lua property overrides share undo history. Delete undo
recreates the same GUID; older edits resolve the recreated actor.

Component addition and removal are Edit-only and undoable. Types must opt into
an addable factory; scripts stay under `add-script`. Removal addresses the exact
reflected type and zero-based occurrence, and refuses required/non-removable
components. Presence assertions inspect the selected entity. Commands restore
entity JSON with the same GUID and preserve selection across recreation.
`VegaEditorComponents` runs the configured build-tree copy of
`components.vegascript`, using only `@VEGA_COMPONENTS_SCENE@` for temporary saves.
It saves again after undo because undo advances the scene's dirty revision.

`play-mode` captures the edit world with SceneSerializer and loads a fresh Play
world. `stop` discards gameplay state, restores the snapshot, editor camera and
pre-Play selection by GUID, and preserves undo/redo. Authoring edits and undo/redo
are disabled in Play; selection and inspection remain available. `play` and
`pause` still resume/pause the current mode; Edit cannot be unpaused. Use
`play-mode` when scripts need actual gameplay or Lua updates.

New/Open script commands refuse unsaved scene changes; save first. UI New/Open/
Exit offer Save / Discard / Cancel. Save uses an exclusive sibling temporary
file and atomic replacement, supports new destinations, and retains history.
Scene dirty uses monotonic edit revisions: each scene command, undo/redo and
script attachment advances the revision. Undoing back to saved content remains
dirty until Save or Reload; this avoids serializing the world to recognize that
state. JSON document undo still recognizes its saved text exactly, comparing
only when edited. Ctrl+S saves the scene unless
Data Editor has focus. Scene double-clicks in Content open the authoring scene.

`VegaEditorAuthoring` runs the configured build-tree copy of
`authoring.vegascript`. CMake substitutes an absolute build-tree path for
`@VEGA_AUTHORING_SCENE@`; the test never saves to source assets. It verifies
Edit/Play/Stop, selection restoration, dirty state and deletion undo, and captures
`authoring.png` in the editor executable directory. Smoke/UI now inspect a
DummyEnemy; camera/scripting explicitly enter Play when gameplay is needed.

Lua actor commands (operate on the selected actor's first ScriptComponent):

```text
add-script scripts/spin.lua
set-script-prop speed 90
expect-script-prop speed 90 0.01
expect-script-error none
expect-script-error "spin.lua:4"
remember-rotation
expect-rotation-changed
wait-seconds 0.5
```

`add-script` attaches a component using the game's runtime asset root; attachment
is not undoable. `set-script-prop` is undoable and accepts one number, bool or
string, or two floats for a vector. Quote strings containing spaces or `#`.
`expect-script-prop` accepts the same values and an optional non-negative tolerance
(default 0.5); scalar types/arity are checked during execution. Error expectations
require an empty LastError for `none`, otherwise a matching substring.
`remember-rotation` stores the selected rotation; `expect-rotation-changed` requires
a change greater than 0.01 degrees modulo 360. `wait-seconds` waits wall time
(0 through 86400 seconds), including while paused, and continues rendering frames.
`wait` still counts frames. `VegaEditorScripting` runs a build-tree copy of
`scripting.vegascript`, verifying property undo/redo and actual rotation after play.
See [VegaScript API](../../../VegaScript/README.md) for Lua callbacks and handles.

The following invocation starts the runner:

```text
VegaEditor.exe --editor-script <file>
```

The Content panel uses the source `LightYearsGame/assets` directory in every
configuration. `--asset-root <dir>` overrides only the editor's content root for
tests; gameplay keeps its normal Debug/source or release/relative runtime root.
`VegaEditorContent` recopies `ships.json` to a disposable build-tree root before
each run, so scripted saves never touch source assets.

```text
open-json <path relative to asset root>
json-set <json-pointer> <JSON scalar literal>
expect-json <json-pointer> <JSON scalar literal>
json-save
json-revert
expect-json-dirty true|false
```

JSON values are literal `true`, `false`, integers, floating point numbers, or
double-quoted JSON strings (including JSON escapes); retain the quotes, for
example `json-set /name "Ship #1"`. A float node requires a float literal such as
`280.0`; integer nodes retain their signed/unsigned type. Objects, arrays, nulls,
missing nodes, and scalar retyping are rejected. JSON pointers use `/` segments
and `~0`/`~1` for literal `~`/`/`; a quoted empty pointer refers to the root.

Dirty means serialized bytes differ from the last saved text. Undo to that text
clears dirty; undo across a save can make it dirty again. Save retains indentation,
line endings, trailing newline, inline container layout, and unchanged scalar
spellings. Any file that cannot round-trip exactly opens read-only with a reason.
The scalar-only serializer preserves the original layout rather than expanding
inline objects and arrays. Save uses a temporary sibling and atomic replacement.
Revert reloads disk and removes commands for the old document ID from history. Script
`open-json` refuses unsaved changes: use `json-save` or `json-revert` first. The UI
asks Save / Discard / Cancel before closing or replacing a dirty document.
Changes apply to the game after restart.

The runner waits for the first live world, executes one command per completed UI
frame, and exits at EOF or `quit`. It continues after failed lines so the report
can include later checks. Exit code is 0 only when every executed line and report
write succeeded. Results are logged through `LY_GAME_INFO` and written alongside
the script to `<file>.report.txt` as `OK|FAIL <source line> -- <detail>`.

Commands are case-sensitive and whitespace-separated. Blank lines and `#`
comments outside double quotes are ignored. Double-quoted arguments can contain
spaces, `#`, or be empty; `\"` and `\\` escape a quote and backslash inside quotes.
Other backslashes are literal. Unclosed or adjoining quotes are errors. Numbers must
be finite; IDs, indices, counts and frame counts must be non-negative integers.

```text
pause
play
camera editor|game
tool move|rotate
snap on|off
grid on|off
colliders on|off
content-view grid|list
focus
zoom <factor>
pan <dx> <dy>
expect-visible
expect-camera editor|game
wait <frames>
select <id>
select-type <TypeName> [index=0]
move <x> <y>
rotate <degrees>
set-prop <Target> "<Property Name>" <values...>
expect-prop <Target> "<Property Name>" <values...> [tolerance=0.5]
undo
redo
expect-selected <TypeName>
expect-location <x> <y> [tolerance=0.5]
expect-count <TypeName> <count>
remember-location
expect-remembered-location
capture <png-path>
quit
```

`select-type` uses the unqualified demangled RTTI type name and the world's live
actor order, excluding pending-destroy actors. `move` and `rotate` use the selected
actor and the same command history as the Inspector. Each script edit is a
separate undo step. `wait N` leaves N complete frames between that command and the
next command. `expect-location` checks each coordinate with the stated tolerance.
`remember-location` stores the selected actor's position for a later
`expect-remembered-location` check (default tolerance 0.5).

Property targets are `Actor` (including its registered derived properties) or a
component type name; the first matching component in component order is used.
Property names are case-sensitive. Use one value for bool (`true`/`false` or
`1`/`0`), int, float, string (quote strings with spaces), or enum (integer value);
two floats for Vector2; four integer channels in 0â€“255 order RGBA for Color.
`expect-prop` compares floats and vector coordinates using its optional non-negative
tolerance; bools, integers, enums, colors and strings compare exactly. Property
arity and value types are validated at execution against the selected descriptor.
Unknown targets/properties, malformed values, and attempts to set read-only
properties fail the line. Each `set-prop` records one undo step through the same
generic command as the Inspector. Range metadata guides Inspector drags; runtime
setters retain their existing validation and side effects.

```text
set-prop Actor "Rotation" 45
expect-prop Actor "Rotation" 45 0.01
set-prop HealthComponent "Max Health" 200
expect-prop HealthComponent "Max Health" 200
undo
redo
```

Pausing enters the editor camera from the current game view; playing restores the
game camera. `camera` switches manually. `focus` enters the editor camera and
centres the selection, fitting its bounds to about 40% of the view, with a minimum
1:1 scale for tiny actors. Zoom is world units per texture pixel, clamped to
0.05â€“20. `zoom` multiplies that scale at the view centre (2 shows twice as much
world); `pan` adds world units to the centre. Both enter the editor camera.
`expect-visible` checks that the selected actor's global bounds intersect the
current world view. These commands do not change actor transforms or history.

In the UI, the camera toolbar button toggles modes. With the editor camera active,
the wheel zooms at the cursor and middle/right drag pans only over the viewport
image; these gestures are consumed before gameplay. F focuses the selection when
the viewport is hovered or focused and text input is inactive. The Inspector
Focus button does the same.

The main toolbar provides Play/Stop, Pause/Resume and a single simulation frame while paused in Play,
Move (W), Rotate (E), camera mode, and snapping. Drag the red X or green Y arrow
to constrain movement, or the centre square for both axes. The rotation ring uses
clockwise screen angles. Handles keep their screen size as the camera zooms.
Each drag is one undo step. Snapping uses absolute world coordinates (16 units by
default, editable beside the snap button); rotation snaps to 15 degrees. Holding
Ctrl temporarily enables snapping. Grid and collider overlays are in the View menu;
the grid appears only in editor camera mode, with every eighth line emphasized.
`tool`, `snap`, `grid`, and `colliders` set these same UI options without changing
actor transforms or history. `content-view` switches the Content panel between
thumbnail tiles and a list. Double-click folders to enter them and JSON files to
edit them; breadcrumbs navigate back up. `ui.vegascript` captures both gizmos and
the content grid as `ui-move.png`, `ui-rotate.png`, and `ui-content.png` in the editor
working directory.

Capture paths are relative to the process working directory. The capture copies
the editor window after ImGui renders; no `--visual-capture` option is needed.
Existing game dev options remain available. Script runs still require an SFML
window and graphics context. Scene entity deletion is undoable, including its GUID, properties, components and scripts.

```text
export <dir> [overwrite]
expect-export-ok
set-project-startup <scene-path>
expect-project-startup <scene-path>
```

`File > Export Game...` and `export` run the same export. It copies
the game executable as the sanitized project name (for example `LightYears.exe`),
the DLLs beside it, the authoring asset root including `vega.project.json`, and
`LightYearsVisual/assets` as `visual/` into `<dir>`. The game reads both folders
relative to its working directory in Release builds. Debug builds read the source
tree instead, so only a Release export is self-contained. The export always runs the
game's smoke test here: `LightYears.exe --visual-capture export_smoke.png
--capture-frames 120 --asset-root <dir>/assets` from `<dir>`, which passes on exit code 0 with a non-empty PNG
and is killed after 60 s. The copy writes `vega_export_manifest.txt`, listing every
file it wrote. `overwrite` removes only the files that manifest lists. A folder
outside the build tree, inside the repository, or inside the game build or visual
assets folder is refused, and an export never replaces a file it did not create.
The `export` line succeeds when the copy succeeds; `expect-export-ok` checks that
the last export also ran its smoke test and that the test passed. The dialog's
"Run smoke test" checkbox is the only way to skip the smoke test. Export paths are
relative to the process working directory, like capture paths.

Startup paths are relative to the asset root; `""` keeps the default level.
`export.vegascript` runs against a temporary asset copy supplied by
`RunExportTest.cmake`, sets a startup scene there, and verifies that the exported
game loads it in play mode. It never writes the source project file.

Copy and smoke testing run on a worker that owns its request/layout values.
The dialog displays an indeterminate progress bar and disables Export/Close
until completion. The script runner waits by polling once per rendered frame.

`perf.vegascript` is a manual benchmark, not a ctest. Copy it to
`build/VegaEditor/scripts/`, then run from `build/VegaEditor`:

```text
VegaEditor.exe --editor-script scripts/perf.vegascript
```

It opens the sandbox, adds 400 asteroids (407 authored entities total), waits
120 frames, and captures `perf.png`. `create-many` uses a centred square grid
with 48-unit spacing and the normal create/move commands. Another 11 seconds
allows complete five-second FrameStats intervals before `perf-stats.png`.
Compare settled `avgRender` and `avgTick` samples from `LightYears.log`, excluding
startup, bulk creation and screenshot frames. Keep the window size, layout and
build configuration the same for both runs.
