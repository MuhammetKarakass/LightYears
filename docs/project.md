# Projects and collision flags

`LightYearsGame/assets/vega.project.json` describes a `vega.project`, version 1:

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
and the remaining Sky integration planned for B4.

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
