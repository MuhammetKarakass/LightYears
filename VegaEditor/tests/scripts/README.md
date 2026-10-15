Run the editor script runner from the editor's executable directory.

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
two floats for Vector2; four integer channels in 0–255 order RGBA for Color.
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
0.05–20. `zoom` multiplies that scale at the view centre (2 shows twice as much
world); `pan` adds world units to the centre. Both enter the editor camera.
`expect-visible` checks that the selected actor's global bounds intersect the
current world view. These commands do not change actor transforms or history.

In the UI, the camera toolbar button toggles modes. With the editor camera active,
the wheel zooms at the cursor and middle/right drag pans only over the viewport
image; these gestures are consumed before gameplay. F focuses the selection when
the viewport is hovered or focused and text input is inactive. The Inspector
Focus button does the same.

Capture paths are relative to the process working directory. The capture copies
the editor window after ImGui renders; no `--visual-capture` option is needed.
Existing game dev options remain available. Script runs still require an SFML
window and graphics context. Actor deletion cannot be undone; UI deletion asks for
confirmation, executes without adding an undo entry, and invalidates redo.

```text
export <dir> [overwrite]
expect-export-ok
```

`File > Export Game...` and `export` run the same export. It copies
`LightYearsGame.exe`, the DLLs beside it, `assets/` from the game build, and
`LightYearsVisual/assets` as `visual/` into `<dir>`. The game reads both folders
relative to its working directory in Release builds. Debug builds read the source
tree instead, so only a Release export is self-contained. The export always runs the
game's smoke test here: `LightYearsGame.exe --visual-capture export_smoke.png
--capture-frames 120` from `<dir>`, which passes on exit code 0 with a non-empty PNG
and is killed after 60 s. The copy writes `vega_export_manifest.txt`, listing every
file it wrote. `overwrite` removes only the files that manifest lists. A folder
outside the build tree, inside the repository, or inside the game build or visual
assets folder is refused, and an export never replaces a file it did not create.
The `export` line succeeds when the copy succeeds; `expect-export-ok` checks that
the last export also ran its smoke test and that the test passed. The dialog's
"Run smoke test" checkbox is the only way to skip the smoke test. Export paths are
relative to the process working directory, like capture paths.
