# Wave 7 step 4: structural file splits

This refactor moves engine, editor and scripting definitions into responsibility-specific
translation units. Public headers, game-facing APIs, serialization formats and reflection
registrations are unchanged. Three Codex subagents (`gpt-6-luna`, reasoning `max`) handled
one original source each: ScriptComponent, EditorCommands and EditorPanels. The primary
agent designed the boundaries, reviewed the moves, split World and Actor, updated CMake,
and owned the build and full test runs. No git commands were run.

## Original files

Counts include all physical lines, including includes and blank lines.

| File | Before | After | Retained responsibility |
| --- | ---: | ---: | --- |
| `VegaScript/src/ScriptComponent.cpp` | 1157 | 398 | Component lifecycle, callbacks, reload, properties, errors, reflection and serialization registration |
| `VegaEditor/src/EditorCommands.cpp` | 1043 | 150 | Command history, compound commands and pause command |
| `VegaEditor/src/EditorPanels.cpp` | 899 | 44 | Stats panel |
| `LightYearsEngine/src/framework/World.cpp` | 926 | 355 | World construction/destruction, actor lifecycle/tick, simulation time, stages and HUD event dispatch |
| `LightYearsEngine/src/framework/Actor.cpp` | 470 | 333 | Actor/component lifecycle, rendering, collision forwarding and reflection |

## New source files and private headers

All new files have a before count of zero. Shared implementation details stay under `src/`;
no public headers were added.

| File | After | Responsibility |
| --- | ---: | --- |
| `VegaScript/src/ScriptRuntime.cpp` | 124 | Interpreter ownership/settings, allocator, instruction budget and device input fallback |
| `VegaScript/src/ScriptBindings.cpp` | 612 | Lua usertypes, reflected value conversion and environment APIs for world, audio, input and timers |
| `VegaScript/src/ScriptRuntimeInternal.h` | 49 | Private interpreter state and sandbox declarations |
| `VegaScript/src/ScriptBindingsInternal.h` | 78 | Private Lua handle types and shared conversion/binding declarations |
| `VegaScript/src/ScriptComponentInternal.h` | 67 | Private component state and implementation method declarations |
| `VegaEditor/src/EditorContext.cpp` | 134 | Editor camera synchronization, actor cache/resolution and type display names |
| `VegaEditor/src/EditorPropertyCommands.cpp` | 301 | Reflected/script/scene properties, selection transforms and common-property inspection |
| `VegaEditor/src/EditorEntityCommands.cpp` | 159 | Entity creation, destruction, duplication, naming and reparenting |
| `VegaEditor/src/EditorComponentCommands.cpp` | 55 | Component add/remove commands |
| `VegaEditor/src/EditorPrefabCommands.cpp` | 177 | Serialized entity replacement and prefab identity/reference restoration |
| `VegaEditor/src/EditorSelectionCommands.cpp` | 100 | Actor/rectangle selection, selection deletion and clipboard copy/paste |
| `VegaEditor/src/EditorCommandsInternal.h` | 37 | Shared selection-subtree and entity snapshot helpers |
| `VegaEditor/src/EditorHierarchyPanel.cpp` | 235 | Hierarchy panel |
| `VegaEditor/src/EditorInspectorPanel.cpp` | 461 | Inspector panel and its property drawing helpers |
| `VegaEditor/src/EditorViewportPanel.cpp` | 163 | Viewport panel and actor picking |
| `VegaEditor/src/EditorExportDialog.cpp` | 76 | Export dialog |
| `LightYearsEngine/src/framework/WorldSpatialQueries.cpp` | 318 | Spatial index maintenance, bounds queries and collision-layer lookup |
| `LightYearsEngine/src/framework/WorldRendering.cpp` | 142 | Render culling, layer submission, pipeline composition and HUD rendering |
| `LightYearsEngine/src/framework/WorldCamera.cpp` | 146 | Camera controls, viewport mapping and input focus |
| `LightYearsEngine/src/framework/WorldInternal.h` | 47 | Shared bounds intersection and slow-actor profiling helpers |
| `LightYearsEngine/src/framework/ActorTransform.cpp` | 151 | Actor transforms, attachment hierarchy and transform propagation |

## Actor decision and review

Actor has a clear contiguous seam from `SetActorLocation` through
`GetActorLocalLocation`: world/relative transforms, attachment validation and propagation.
That block moved byte-for-byte to ActorTransform.cpp; the actor's lifecycle and component
coordination remain in Actor.cpp. World has no separate out-of-line parenting block;
the concrete hierarchy work belongs to Actor.

Exact-text comparisons preserved all 52 World member definitions, all 50 Actor member
definitions, all six panel methods and the Inspector helper block. The command worker
checked all 63 function bodies, including the three shared helpers; primary review also
checked the extracted command definitions. Scripting body comparisons allow indentation
changes required by out-of-line definitions. Its only statement qualification change is
`Bind(lua)` becoming `detail::Bind(lua)` in the moved interpreter constructor. Other
mechanical changes are member/namespace qualification, private declarations, default
arguments declared once, inline shared helpers and explicit includes.

The engine, editor and scripting CMake lists register the new translation units. A shared
`VEGA_EDITOR_COMMAND_SOURCES` list also covers the five existing test targets that compiled
EditorCommands.cpp directly. No test logic was changed.

| Build list | Before | After |
| --- | ---: | ---: |
| `LightYearsEngine/CMakeLists.txt` | 328 | 332 |
| `VegaEditor/CMakeLists.txt` | 394 | 408 |
| `VegaScript/CMakeLists.txt` | 55 | 59 |

## Validation

The primary agent built with `cmd /c tools\vega-build.cmd`. The final build exited 0;
see [build log](../build/task-042-build.log). Initial compile attempts exposed missing
direct includes for the runtime implementation and SceneEntityComponent; those were
fixed, and all direct command-source test targets were updated before the final build.
The existing shadowed `ref` warning in the moved prefab restoration body was preserved.

Both full runs used `ctest --test-dir build -j 8 --timeout 300`, adding
`--output-on-failure` and `--output-junit` for saved evidence. No tests were filtered out.

| Run | Tests | Passed | Known failures | Wall time |
| --- | ---: | ---: | ---: | ---: |
| Before refactor | 169 | 145 | 24 | 120.06 s |
| After refactor | 169 | 145 | 24 | 120.09 s |

Both invocations returned nonzero for the known failures (the PowerShell tool wrapper
reported exit 1). Every other test passed. Exact comparison by test name and failure
kind found **zero status changes**, including no added or removed tests. The failure
set remains 23 Failed and one Timeout (`LightYearsRareZoneRulesE2E`), including the
documented sandbox-only `LightYearsAbilityBalance-second-E2E` failure. The reflection
registry golden, context, scripting, editor script, prefab, hierarchy and Blank tests
all passed.

- [Baseline log](../build/task-042-baseline-tests.log) and [JUnit XML](../build/task-042-baseline-tests.xml)
- [Final log](../build/task-042-tests.log) and [JUnit XML](../build/task-042-tests.xml)
- [Machine-readable status comparison and full failure list](../build/task-042-validation.json)

A SHA-256 inventory of 1,748 files covering LightYearsGame (including authoring assets),
LightYearsVisual, Modules and all engine/editor/script public headers found zero changes
after the build and final suite. No game, visual, optional-module or public-header files
were edited. No new test logic was introduced, and tests did not write into authoring
assets. All 29 source/header/build-list line counts in this report were checked against
the final files.
