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
takes precedence. The editor opens its usual sandbox unless `--editor-scene` is
supplied. Exports include the project file and name the executable `<Name>.exe`,
replacing Windows-invalid filename characters and protecting reserved names.

Editor scripts support `set-project-startup <path>` and
`expect-project-startup <path>`; use `""` for the default level. Automated export
tests pass `--asset-root` with a disposable copy, keeping the source project
unchanged. In the editor this option selects authoring documents; in the game it
selects runtime assets, including the project file.

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
