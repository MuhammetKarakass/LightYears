# Docs audit: Vega documentation vs code

Scope: `game-module`, `new-game`, `engine-modules`, `reflection`, `prefabs`,
`hierarchy`, `lua-api`, `scene-authoring`, `project`, `sky-editor`,
`gas-debugger`, `gas-content-editor`, `console-profiler`, `autosave` and
`editor-script-commands` in `docs/`, checked against the source on branch
`vega-engine`. Nothing was built or run; every item below was verified by reading
code. Line numbers for doc files refer to the files **before** this audit's fixes.

## A. Fixed in place (clearly wrong)

| # | Doc | Statement | Code evidence | Fix |
| --- | --- | --- | --- | --- |
| 1 | `console-profiler.md:66` | Layout saved in `vega_editor_layout_v4.ini` | `VegaEditor/src/EditorApplication.cpp:75` sets `IniFilename = "vega_editor_layout_v5.ini"` | Now `v5` |
| 2 | `prefabs.md:133` | "Prefabs have no nested entity hierarchy or entity-reference remapping in this version" (contradicts `prefabs.md:61-68` and `:170-176`) | Children and `$prefab:` addresses: `LightYearsEngine/src/framework/scene/PrefabLibrary.cpp:281-313` (`replaceChildren`, `deleted`), `Scene.cpp:403-406` (`RemapReferences`) | Sentence replaced with a pointer to hierarchies and `$prefab:` remapping |
| 3 | `scene-authoring.md:195-196` | Setting types listed as bool, int, float, string, enum, catalog ID, vec2, color | `LightYearsEngine/include/framework/scene/SceneSettings.h:13` also has `Rect` (used for `bounds`, `scene-authoring.md:344`) | `rect` added |
| 4 | `scene-authoring.md:285-286` | "general reflected entity-reference properties remain future work" | `ActorRef`/`VEGA_ACTOR_REF` exist (`Reflection.h:22-25`) and duplicate/paste remap them (`EditorCommands.cpp:679-703`, `Scene.cpp:398-410`); `reflection.md:198+` documents them | Reworded: reflected `ActorRef` properties are remapped with subtree copies |
| 5 | `lua-api.md:286` | Player resolver installed in `LightYearsGame/src/gameFramework/GameApplication.cpp` | `LightYearsGame/src/gameFramework/LightYearsGameModule.cpp:124-126` (`SetScriptRoot`, `SetPlayerResolver`) | Path corrected |
| 6 | `lua-api.md:230-233` | New CTest groups omit Hierarchy and Audio | `VegaScript/CMakeLists.txt:39-41` loops over `World Hierarchy Events Timers Messages InputVectors Audio LuaTypes ExamplesCost` | List completed |
| 7 | `project.md:109-110` | "the remaining Sky integration planned for B4" | Sky is a finished optional module: `docs/engine-modules.md`, `Modules/Sky/` | Replaced by a link to engine modules |
| 8 | `project.md:3-13` | The shipped project file is shown as only name/startupScene/window | `LightYearsGame/assets/vega.project.json` also has `editor` and `runtime` objects | Marked as abbreviated |
| 9 | `game-module.md:169-171` | `vega_add_game_editor(Name MODULE ... ENTRY ...)` | `VegaEditor/cmake/VegaGameEditor.cmake:6` also parses an optional `CORE` argument (default `VegaEditorCore`, line 12-14) | Optional `CORE` and the helper's path documented |

## B. Checked and found accurate

* Reflection macros in `reflection.md:47-62` match `ReflectionMacros.h:13-43`
  (`VEGA_CLASS` is in `ReflectionDeclare.h:17`). `VEGA_ACTOR_REF` and `VEGA_LIST`
  are plain aliases of `VEGA_PROPERTY` (lines 36-37); the doc already says the
  type is inferred. `.RequiredType`, `.Labels`, `.Flags`, `.Range`, `.Speed`,
  `.Tooltip` exist at `Reflection.h:226-265`.
* `PropertyValue`/`PropertyKind` (`reflection.md:28-29`, `:217-222`) match
  `Reflection.h:22-25`.
* `docs/editor-script-commands.md` is generated and is byte-compared to
  `ScriptCommandRegistry::Markdown()` by `VegaEditor/tests/EditorScriptTests.cpp:319-324`.
  Every `##` heading maps to a registered command (checked by name against
  `VegaEditor/src/commands`, `Modules/GAS/editor` and
  `LightYearsGame/src/editor/LightYearsScriptCommands.h`). Not re-generated here
  because that needs a build.
* Command names and argument order quoted elsewhere match the registry:
  `parent`, `unparent`, `expect-parent`, `expect-world-location`
  (`hierarchy.md:104`), `asset-move/rename/delete`, `expect-asset`
  (`project.md:88`), `autosave-now`, `expect-autosave`, `expect-recovery`,
  `recover-autosave`, `simulate-unclean-exit` (`autosave.md:41-49`),
  `set-setting`, `expect-setting`, `reset-setting` (`scene-authoring.md:225`),
  `sky-open/set/drag/save`, `expect-sky`, `capture-sky` (`sky-editor.md:112-119`),
  and the console/profiler commands (`console-profiler.md:110-122`).
* `TokenScriptCommand`, `Schema`, `ScriptMode` and the `EditorExtensions` API
  (`AddPanel`, `AddViewport`, `AddHierarchyMenu`, `AddScriptCommand`) match
  `ScriptCommandRegistry.h:44,69-70` and `EditorExtensions.h:37-49`.
* Lua API names in `lua-api.md` (world/actor/component/timer/input/audio/music,
  limits 256/8/4096/65536/128/1024/64) match `VegaScript/src/ScriptComponent.cpp`.
* Hierarchy rules (`hierarchy.md:25-30`, depth 128, prefab chain 64) match
  `Actor.cpp:325-350`, `Scene.cpp:442`, `PrefabLibrary.cpp:130,161`.
* Project settings keys and defaults match `ProjectSettings.h:18-21`,
  `ProjectSettings.cpp:55-69` and `EditorProject.cpp:33-35`.

## C. Uncertain or not changed

| Doc | Observation | Why not edited |
| --- | --- | --- |
| `lua-api.md:247-277`, `scene-authoring.md:315-333`, `prefabs.md:146-153` | Dated verification logs (build exit codes, test counts, timings, `build/*.log` paths) | Historical records of a past run, cannot be re-verified without building |
| `lua-api.md:213` | Mentions `LightYearsGame/assets/.vega`, which is generated by the editor and absent from a clean checkout | Accurate once the editor has run; fine as guidance |
| `reflection.md:142-146` | "The Hierarchy Create menu now includes ExampleBeacon" | `ExampleBeacon` is an illustrative class that does not exist in the repo; wording reads as fact |
| `scene-authoring.md:156-165` | "All four scene types" lists Asteroid, DummyEnemy, PlayerStart, SpriteActor | `GameReflection.cpp:20-46` also registers SpaceShip, PlayerSpaceShip and EnemyActor; unclear which are `Spawnable` without running the registry |
| `gas-content-editor.md:122-132` | Contains debugger layout notes and an "Optional engine module" paragraph that duplicates `gas-debugger.md:48-54` | Structural duplication, no incorrect fact |
| `game-module.md:150-152` | Says `LightYearsEditorModule.cpp` registers `expect-player-ship` | The declaration is `LightYearsGame/src/editor/LightYearsScriptCommands.h:8-12`; `LightYearsEditorModule.cpp:1` includes it. Correct as worded, but the header is the better pointer |
| `new-game.md:85-97` | Lists only three Blank tests | Blank also registers `VegaEditorAutosaveRecovery`, `VegaEditorDailyUse`, `VegaEditorAssetMoves`, `VegaEditorRichProperties`, `VegaEditorHierarchy` (`Templates/Blank/CMakeLists.txt:27-60`); incomplete rather than wrong |
| `sky-editor.md` | `sky-drag <handle> <dx> <dy>` doc uses argument names that differ from the registry (`handle x y`) | Same positional meaning; the generated reference is authoritative |
