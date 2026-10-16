# Projects and collision flags

`LightYearsGame/assets/vega.project.json` starts with a `vega.project`, version 1 header (abbreviated here; the shipped
file also has the `editor` and `runtime` objects described below):

```json
{
	"format": "vega.project",
	"version": 1,
	"name": "LightYears",
	"startupScene": "",
	"window": { "width": 1920, "height": 1080 }
}
```

Use **File > Project Settings...** to edit the name, choose a startup scene from
`scenes/`, and set the next game's window size. Saving replaces the project file
atomically and does not enter undo history. Dimensions must be integers from 1
to 16384; missing dimensions default to 1920×1080. Without a project file the
game keeps its existing window size and default level.

An empty startup scene keeps the default level. A nonempty asset-relative
`.scene.json` path starts a SceneLevel in play mode; `--scene` or `--editor-scene`
takes precedence. The editor opens the startup scene, or its usual sandbox when
that setting is empty; `--editor-scene` overrides it. Exports include the project file and name the executable `<Name>.exe`,
replacing Windows-invalid filename characters and protecting reserved names.

Editor scripts support `set-project-startup <path>` and
`expect-project-startup <path>`; use `""` for the default level. Automated export
tests pass `--asset-root` with a disposable copy, keeping the source project
unchanged. In the editor this option selects authoring documents; in the game it
selects runtime assets, including the project file.

The editor's default project is selected by the game's CMake integration.
`--project <file>` chooses another project; `--asset-root` still overrides the
authoring root (partial disposable roots need not contain runtime content).
Paths in the optional `editor` object resolve relative to the selected project
file, independent of the working directory:

```json
{
	"editor": {
		"assetRoot": ".",
		"font": "fonts/NotoSans-Regular.ttf",
		"visualAssets": "../../MyVisuals/assets",
		"gameBuildDir": "../../build/MyGame",
		"releaseGameBuildDir": "../../build-release/MyGame",
		"exportDir": "../../build/export/MyGame"
	},
	"runtime": {
		"executable": "MyRuntime.exe",
		"assetDirectory": "assets",
		"visualDirectory": "visual",
		"log": "MyGame.log"
	}
}
```

`editor.autosaveSeconds` sets the editor's periodic autosave interval (default
120 seconds; 0 disables it; maximum 86400). Project Settings exposes this value.
Snapshots go to `.vega/Autosave` inside the asset root and never replace authored
files. See [autosave and recovery](autosave.md) for recovery and script commands.

Content Browser supports dragging files/folders onto folders, F2 rename, **Move
to folder...** and confirmed Delete. Moves preview asset-relative JSON references
and update them in one undo step; this includes scenes, nested prefabs, attached
scripts, project startup scenes and content/sky JSON inside the asset root. JSON
formatting and unrelated values are preserved. Save dirty buffers, close the
prefab and stop Play first. Clean open documents and the scene refresh afterwards.
Destinations must exist as folders inside the asset root; overwrites, case-only
renames, linked assets, malformed JSON and `.vega` paths are refused. Lua source
code and references outside the asset root (except the selected project file)
are not rewritten. Registered content catalogs have module-owned fixed filenames;
moving those files or their containing folders is refused. C++ asset path literals
are not rewritten; games should use authored asset references for movable assets.

Each rename and reference write is atomic. A pending transaction journal under
`.vega/AssetTransactions` lets startup roll back an interrupted move before game
services initialize. A conflicting external edit blocks recovery and further
file operations, preserving the journal and reporting the conflict in Console.
Undo/redo validates the reviewed JSON bytes and refuses external changes while
retaining the undo step. The whole multi-file operation is not an OS-level atomic
transaction; the journal provides recovery between its individual writes.

Delete refuses assets referenced by other JSON documents and the active scene or
project file. It moves other assets to `.vega/Trash` with Undo support. Trash is
retained on disk after undo history expires; no automatic pruning is performed.

Automation supports `asset-move <from> <to>`, `asset-rename <path> <filename>`,
`asset-delete <path> confirm`, and `expect-asset <path> true|false`. Move and rename
commands use asset-relative paths and run the same validation as UI previews.
`VegaEditorAssetMoves` exercises moves, rename, reference updates, Trash undo and
Play in a disposable Blank project. `VegaEditorAssetOperations` additionally
covers stale previews, path refusal, interrupted moves and rollback on a locked
reference file.

These are additional fields in the version 1 project document. Project Settings
and script startup-scene edits preserve them. Defaults are the project directory
for assets, ImGui's built-in font, no visual assets, `build/Game` and
`build-release/Game` for runtime builds, and `build/export/<Name>` for exports.
Runtime defaults are `Game.exe`, `assets`, `visual` and `Game.log`. Package paths
must stay inside the export; executable and log names must be plain filenames.
`--visual-assets` continues to override the game's presentation asset source.

Export copies the configured runtime executable and adjacent DLLs, renames the
executable using the project name, copies authoring assets to `assetDirectory`,
and optionally copies presentation assets to `visualDirectory`. Release uses
the built presentation copy; Debug uses the configured source. Smoke testing
passes the configured asset directory with `--asset-root` and checks the
configured runtime log. See [game modules](game-module.md) for game registration
and [engine modules](engine-modules.md) for the optional Sky and GAS modules.

Collision layer and mask use reflected **Flags** with eight labels: Player,
PlayerBullet, Enemy, EnemyBullet, Powerup, Environment, RelayProjectile, and
FriendlySummon. The Inspector popup has one checkbox per bit. Scenes save flags
as label arrays, for example `"Collision Mask": ["Player", "Enemy"]`; `[]` means
none. Loading also accepts integer masks and older strings such as
`"Player | Enemy"` or `"None"`. Unknown labels and undeclared bits are rejected.
Editor `set-prop` commands and Lua reflected setters accept integer masks.

Component references in editor history and Lua handles resolve by reflected
type name and occurrence within that type. Adding components of other types
does not change their target; reordering components of the same type changes
their occurrence.

Content/data JSON and prefabs are watched on a half-second editor cadence,
including while Play is paused. Saved JSON dispatches to `GameModule::ReloadContent`;
no restart is needed for LightYears abilities/effects/modules/weapons/ships
used by future spawns and grants. See [game modules](game-module.md) for the
remaining restart requirements. Dirty buffers are never silently replaced by
disk reloads; use Reload files/Revert explicitly to refresh an authoring buffer.