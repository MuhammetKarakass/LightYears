Run the editor script runner from the editor's executable directory.

Reflected connections and lists:

* `set-ref <Target> "<Property>" <OtherTarget>|none`
* `expect-ref <Target> "<Property>" <OtherTarget>|none`
* `list-add <Target> "<Property>" <value>`
* `list-remove <Target> "<Property>" <index>`
* `list-set <Target> "<Property>" <index> <value>`
* `expect-list-size <Target> "<Property>" N`
* `list-move <Target> "<Property>" <from> <to>`
* `set-prop <Target> "<Property>.<Field>" <value>` (also `"<Property>[i].<Field>"` for a struct list element; `expect-prop` takes the same paths)

Target is `Actor` or a component type on the selection, or a scene actor name/GUID.
OtherTarget is a scene actor name/GUID. List indices are zero-based. Values use
set-prop scalar syntax: Vec2 `x y`, Color `r g b a`, enum integer, quoted strings;
ActorRef elements use a name/GUID or `none`. A struct element is appended with `list-add-default`, which starts from its field defaults. Each mutation is one undo step.
`Templates/Blank/Tests/properties.vegascript` exercises the Lua patrol/target demo,
save/reopen, and prefab/copy connections in `VegaEditorRichProperties`.

Sky authoring commands (scene Edit mode):

```text
sky-open <stopId>
sky-set <field.path> <JSON-value>
sky-drag <handle> <dx> <dy>
sky-save
expect-sky <field.path> <JSON-value>
capture-sky <path>
```

Paths accept `planet.at`, `objects[0].size` and JSON pointers. Enum strings use
JSON quotes; vector/RGB values use arrays. Deltas are viewport texture pixels;
handles include `planet`, `planet.scale`, `object.0`, `object.0.scale`,
`object.0.rotate`, `band.angle`, `band.offset`, `orbit.disk`, `orbit.horizon`,
`orbit.angle` and `orbit.radius`. `undo`/`redo` use Sky history while it is active.
Captures contain only the time-zero backdrop at 960 x 540. Before `sky-open`,
`capture-sky` captures the scene's disk sky; afterward it flushes pending edits.
Unknown fields, invalid enums/types/ranges and invalid handles fail the script.

`VegaEditorSkyEditor` configures `sky-editor.vegascript` and runs it with both
`--asset-root` and `--visual-assets` pointing at fresh **build-tree copies**.
Never run a saving Sky test against the real visual asset root. Captures,
checksums, unchanged-data comparisons and unrelated-stop byte checks live under
`build/VegaEditor/sky-editor-test`. See [Sky editor](../../../docs/sky-editor.md).

Content authoring commands (Edit mode; values are JSON, including arrays/objects):

```text
content-open <ability|effect|module|ship|weapon> <id>
content-set <field.path> <value>
content-new <type> <template-or-source-id> <newId>
content-rename <type> <oldId> <newId>
content-validate
content-save
content-reload
expect-content <type> <id> <field.path> <value>
```

Paths accept `modifiers[0].magnitude`, `modifiers.0.magnitude`, or JSON pointers
such as `/modifiers/0/magnitude`. Quote string values as JSON strings. Composite
values may contain spaces: `content-set modifiers [{"attributeId":"Owner.AttackPower",
"operation":"Add","magnitude":4.0}]` (keep each command on one line).
`content-new` accepts the game's `TimedModifier` effect template or an existing
definition ID. Duplication uses the same command. `undo`/`redo` include reference
updates in a rename as one operation. Unknown fields and direct ID edits are
refused. `content-validate` reports all definitions and fails the script on errors;
`content-save` validates before atomically replacing changed files. Reload refuses
unsaved changes and invalidates old content history. Root ability IDs selecting
compiled behavior cannot be renamed; duplicate a data variant first.

`VegaEditorGasContent` runs `gas-content.vegascript` on reset build-tree copies
using `--asset-root`, checks rename undo/redo, saves and reopens, and invokes the
existing `LightYearsContentTests --content-root` loader path on the result. The
runner verifies source hashes remain unchanged. See [content authoring](../../../docs/gas-content-editor.md).

Ability System inspection commands (Play and Pause):

```text
gas-select-player
expect-ability <id> ready|cooldown|active
expect-effect <id> true|false
expect-attribute <id> <op> <finite number>
expect-tag <tag> true|false
gas-panel on|off
gas-filter abilities|effects|attributes|tags|events "search text"
```

Debug mutations require unpaused Play and use ASC public APIs:

```text
gas-apply-effect <id>
gas-remove-effect <id>
gas-reset-cooldowns
gas-activate <ability-id>
```

`gas-select-player` selects the registered player and opens/focuses the panel.
An empty selection falls back to the player; other selections require an ASC.
Assertions read fresh snapshots, even while the panel is frozen in Pause.
Attribute comparisons support `==`, `<`, `>`, `<=`, `>=`; unknown IDs fail.
`gas-activate` uses the game adapter's normal TryActivate path through the ASC
debug API, including its usual activation gates and costs.
Effect apply uses content defaults or the first authored ability effect spec;
unknown/refused effects fail. Remove removes all active instances with that ID
and fails if none exist. `VegaEditorGasDebugger` runs the build-tree copy of
`gas-debugger.vegascript` with build-tree assets. Its capture and 300-frame
profiles are under `build/VegaEditor/Saved`. See [debugger details](../../../docs/gas-debugger.md).

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
write succeeded. Results are logged through `VEGA_GAME_INFO` and written alongside
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
# Prefab commands

Prefab commands operate on the selection in Edit mode:

| Command | Effect |
| --- | --- |
| `create-prefab <path>` | Create an asset from the selection and convert it to an instance; undo restores the entity and leaves the file. |
| `place-prefab <path> [x y]` | Place and select an instance; omitted coordinates use the camera centre. |
| `create-variant <parent> <path>` | Create a variant document that inherits the parent prefab. |
| `apply-prefab-prop <Target> "<Property>"` | Write the selected property to its prefab and refresh instances. |
| `revert-prefab-prop <Target> "<Property>"` | Restore the selected property from the prefab. |
| `expect-prefab <path>\|none` | Assert the selection's prefab link. |
| `expect-overridden <Target> "<Property>" true\|false` | Assert a property differs from the resolved baseline. |
| `reload-prefabs` | Reload documents and refresh instances while retaining overrides. |
| `open-prefab <path>` | Edit a temporary prefab instance in the scratch Inspector. |
| `save-prefab` | Save scratch changes atomically and refresh the edit scene. |
| `close-prefab` | Return to scene inspection; save changes first. |

Targets use the same names as `set-prop` (`Actor`, `HealthComponent`, etc.).
`ScriptComponent` addresses the first script's declared properties. Prefab paths
must be asset-relative `.prefab.json` files under `prefabs/`. Automation must pass
`--asset-root` with a disposable build-tree copy before any prefab-writing command.
`VegaEditorPrefabs` configures and runs `prefabs.vegascript` this way. Scene edits
and refreshes enter undo history; writing an asset is immediate and is not undone.


Scene settings commands:

- `set-setting "<key>" <JSON-value>` validates and writes an undoable Edit-mode override.
- `expect-setting "<key>" <JSON-value>` compares an authored value or its schema default; unknown keys can be inspected.
- `reset-setting "<key>"` removes the override through undo history.
- `expect-backdrop "<stop-id>"` checks the procedural backdrop's actual stop state.
- `expect-player-ship "<catalog-id>"` requires a live player ship in the Play world and checks the Player's construction catalog ID.

Values use JSON scalars or compact arrays such as `[1,2]` and `[255,128,0,255]`; quote strings, including empty strings. Unknown keys are read-only.

`scene-settings.vegascript` is configured into the build tree and registered as
`VegaEditorSceneSettings`. Its CMake runner creates a sandbox copy with an unknown
setting, verifies route set/undo/redo/reset against backdrop state, captures both
backdrops, removes PlayerStart to exercise the world ship default, plays a different
catalog ship, then saves/reopens the temporary scene. No source assets are written.


### Multiple selection

The selection is an ordered set of weak actor references; the last clicked actor
is primary. Existing single-target commands inspect/edit that primary. GUIDs
restore all selected actors after undo/redo, component replacement, scene reload,
and Play/Stop. Delete, duplicate, paste, group transforms and multi-property edits
each occupy one undo step. Duplicate/paste select their copies, and undo restores
the previous ordered selection.

In the Hierarchy, Ctrl+click toggles a row, Shift+click selects a range from the
anchor, Ctrl+Shift adds the range, Ctrl+A selects the filtered rows, and Esc clears.
In the viewport, Ctrl+click toggles and Shift+click adds. Drag empty space for a
marquee: Shift adds hits, Ctrl removes hits. Marquees intersect world bounds.
Every selected actor has an outline; the primary outline is gold. Move/rotate
handles use the selection centre; rotation also orbits actor positions. Snap
settings (or Ctrl during a drag) apply to the group pivot/angle. Focus frames the
union of all selected bounds. Ctrl+D duplicates, Ctrl+C copies JSON to the system
clipboard, and Ctrl+V pastes at the viewport mouse or camera centre.

The multi Inspector shows reflected properties on the common actor base and
components present in every selected actor. Mixed fields show an em dash and use
ImGui's mixed-value flag. Edits write every target, preserving individual old
values for undo. The header shows the number of prefab instances only when the
selection contains one, with "mixed prefabs" for different prefabs or a mix of
prefab and plain actors. Its descriptors and mixed values are cached by world,
scene revision and selection revision; drawing does not serialize actors.

```text
select-add <Target>
select-remove <Target>
select-all
select-rect x0 y0 x1 y1
expect-selection-count N
move-selection dx dy
set-prop-all "Property" <value...>
duplicate
copy
paste [x y]
```

Targets accept an exact quoted entity name, GUID, or runtime ID. Adding an already
selected target promotes it to primary. Script rectangles replace selection and
accept either endpoint order, in world coordinates. select-all includes every
live row (the script has no Hierarchy filter). move-selection is a relative delta;
legacy move remains an absolute primary edit. set-prop-all requires a unique
common writable reflected property name and uses the same scalar/vector/color
syntax as set-prop. paste without coordinates uses the camera centre.

VegaEditorMultiSelect runs multi-select.vegascript with a dedicated build-tree
asset root, temporary scene and Inspector capture. It checks three-actor move
undo, shared Render Enabled edits, duplicate/copy/paste, group selection/history,
save/reopen, marquee, select-all and group delete undo. VegaEditorMultiSelectionCommands
covers atomic rollback, merged transforms, pivot rotation, common-base/mixed
properties, focus bounds, GUID rebinding and component replacement without a UI.

Hierarchy automation: `parent <Child> <Parent>`, `unparent <Child>`,
`expect-parent <Child> <Parent>|none`, and
`expect-world-location <Target> x y [tolerance]`. Names with spaces are quoted;
targets also accept scene GUIDs. Parenting keeps world transforms and is undoable.
`hierarchy.vegascript` is registered as `VegaEditorHierarchy`, runs with BlankEditor
and writes only to a build-tree copy of Blank assets. It exercises parenting,
subtree transforms, child prefab overrides/apply/revert, save/reopen and delete undo.
