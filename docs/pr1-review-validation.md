# PR #1 review fixes and validation

Validation is performed in `D:\Vega`, Debug/MSVC/C++17, without git commands.
The failure baseline is the 24 tests in [engine-context-validation.md](engine-context-validation.md).
New engine/editor service access uses the owning EngineContext.

| Finding | Fix | Production files | Regression coverage |
| --- | --- | --- | --- |
| 1: prefab recursion | Each PrefabLibrary owns the active resolve chain and dependencies. Re-entrant public resolves share them and bypass the realized cache while resolving. Staging scene loads explicitly use that same library, including standalone libraries. Base subtrees are preserved. | `VegaEngine/include/framework/scene/{PrefabLibrary,Scene}.h`, `VegaEngine/src/framework/scene/{PrefabLibrary,Scene}.cpp` | `LightYearsPrefabLibrary`: self-cycle and mutual cycle through base children, required warning, unrelated scene entity survives, standalone-library resolution. Existing context isolation tests also run. |
| 2: Unicode filenames | Native paths stay native; filenames used by JSON, UI and diagnostics use UTF-8 rather than the Windows ANSI code page. Watcher Begin/Poll and per-entry processing catch filesystem exceptions. Asset plans retain an explicit EngineContext for schema access. ContentWorkspace was audited: it has no filename narrowing or filesystem walker. | Editor and Sky filename audit inventory below; `VegaEditor/include/editor/{ContentWatch,AssetOperations}.h` | `VegaEditorContentWatch`, `VegaEditorAssetOperations`, `VegaEditorAutosaveStore`: CJK plus emoji filenames, watch, move/reference rewrite/undo, autosave/recovery path round trip. |
| 3: reload storm | Watcher skips permission-denied directories, handles entry errors independently, commits successful stamps on partial walks, and reports each persistent error once. Incomplete JSON keeps its prior valid stamp for retry. | `VegaEditor/include/editor/ContentWatch.h`, `VegaEditor/src/ContentWatch.cpp` | `VegaEditorContentWatch`: injected persistent permission-denied stamp failure alongside a changed readable file; following polls are empty, one diagnostic, recovered file dispatches once. |
| 4: dependent catalog validation | The module delegates reload orchestration to GameContentBootstrap with its context. Every schema validates the candidate documents before catalog replacement. Ship validation also resolves primary weapons against the candidate weapon catalog. Module side-feature validation has a non-mutating preflight. Rejection and parse errors reach the Console. | `LightYearsGame/src/gameFramework/LightYearsGameModule.cpp`, `LightYearsGame/{include,src}/gameplay/content/{GameContentBootstrap,ModuleContentCatalog}.{h,cpp}`, `LightYearsGame/src/gameplay/content/GameContentSchemas.cpp` | `LightYearsContentReload`: removing an ability-referenced effect, ship-referenced weapon, or required schematic module rejects the reload; ability/effect/weapon/ship pointers and module revision remain unchanged; Console error required. `VegaEditorHotReload` covers accepted edits. |
| 5: stale contacts | Begin contacts retain collision filtering; end contacts bypass it. Invalidating a layer/mask ends invalid pairs on both components. Physics uninitialization and EndPlay end both sides and unbind destruction listeners. Destruction cleanup unbinds silently when callbacks are no longer appropriate. Callback traversal tolerates contact-map mutation. | `VegaEngine/include/framework/CollisionComponent.h`, `VegaEngine/src/framework/{CollisionComponent,PhysicsScene}.cpp` | `VegaScriptEvents`: physical touch, mask change, separation, restore and touch again; same sequence for physics disable/re-enable; begin/hit and end callbacks checked. Existing compound-contact and actor-destruction checks remain. |
| 6: same-track music fade | Successful PlayMusic records the current track separately from the queued track. Repeated fade requests for the current/fading-in track or queued destination preserve playback. Every request still gets a new ownership revision so a new Lua owner can claim unchanged playback. | `VegaEngine/{include,src}/framework/AudioManager.{h,cpp}` | `VegaScriptAudio`: playback position does not restart during repeat fade-in/playing requests; a second Lua script claims the same track with fade, and destroying the previous owner preserves music. |
| 7: bootstrap exit status | Failed content registration sets exit code 1 before quitting. | `LightYearsGame/src/gameFramework/LightYearsGameModule.cpp` | New `LightYearsContentBootstrapFailure`: actual game process, disposable data root with invalid effects catalog, `--visual-capture`, expected diagnostic and exit 1. |
| 8: snapshot cost | Runtime snapshots omit source formatting and modifier/tag copies. Explicit debug snapshot methods populate these fields only for BuildDebugSnapshot. Scalar/runtime state remains available to gameplay callers. | `SpaceAbilitySystem/include/effects/{GameplayEffectRuntimeEntry,GameplayEffectRuntimeSystem}.h`, `SpaceAbilitySystem/src/AbilitySystemComponent.cpp` | `SpaceAbilitySystemDebugView`: runtime debug-only fields empty; debug snapshots retain modifiers/tags and existing snapshot lifetime checks. |
| 9: duplicate warnings | Prefab resolution returns warning data without logging. Staging scene loads suppress emission; the outer scene caller logs warnings. Repeated cached resolves produce no log records. | `VegaEngine/src/framework/scene/{PrefabLibrary,Scene}.cpp`, `VegaEngine/include/framework/scene/Scene.h` | `LightYearsPrefabLibrary`: initial and cached direct resolves are silent, cached warning data is retained, outer scene logs exactly one record per returned warning. |
| 10: child serialization | One SaveChildren helper implements child serialization and ID collision spelling for SaveEntity and SaveSubtree. | `VegaEngine/src/framework/scene/Scene.cpp` | `LightYearsEngineHierarchy`: both paths produce identical child JSON bytes; duplicate IDs still spell `same`, `b_c`, `b_c_`; subtree load/save bytes are identical. Existing scene/prefab round trips run. |
| 11: Blank samples | New BlankSamplesLoad enumerates every scene under a disposable copy of Assets/scenes. Each runs for 180 frames in BlankGame and BlankEditor, with additional editor time for hierarchy timers, a patrol lap, and trigger contact, messages, Spark prefab spawning, audio and timer cleanup. Any Console warning/error/fatal fails, including Lua errors and shutdown diagnostics. | `Templates/Blank/CMakeLists.txt`, `Templates/Blank/Tests/RunSamples.cmake` | New `BlankSamplesLoad`; per-scene logs, scripts and runtime captures are in `build/blank-samples-test`. |

The Unicode audit covers these production files; watcher and asset-plan logic changes
are described above:

- `VegaEditor/src/AssetOperations.cpp`, `AutosaveStore.cpp`, `ContentWatch.cpp`
- `VegaEditor/src/EditorAssetOperations.cpp`, `EditorAutosave.cpp`, `EditorContentPanels.cpp`
- `VegaEditor/src/EditorPrefabs.cpp`, `EditorScenes.cpp`, `EditorPanels.cpp`
- `VegaEditor/src/EditorChrome.cpp`, `EditorApplication.cpp`
- `Modules/Sky/src/ColorScript.cpp`, `DeepSky.cpp`, `PipelineSettings.cpp`, `ShaderLibrary.cpp`, `VisualAssets.cpp`
- `Modules/Sky/editor/src/EditorSkyEditor.cpp`

Regression source files changed: `VegaEngine/tests/{PrefabLibraryTests,HierarchyTests}.cpp`,
`VegaEditor/tests/{ContentWatchTests,AssetOperationsTests,AutosaveTests}.cpp`,
`VegaScript/tests/LuaApiTests.cpp`, `SpaceAbilitySystem/tests/AbilitySystemDebugViewTests.cpp`,
`LightYearsGame/tests/ContentReloadTests.cpp`, and `LightYearsGame/tests/RunBootstrapFailure.cmake`.
The bootstrap process test is registered in `LightYearsGame/CMakeLists.txt`.

## Collision labels: later work only (finding 12)

CollisionComponent still registers the eight LightYears labels in engine reflection.
No label/schema/scene-format change is made here. Current ProjectSettings contains
project/window/editor/runtime configuration; it has no typed collision-layer settings.
The existing `editor` JSON object is extensible, but that does not implement a
collision-layer schema or apply labels to reflection.

A follow-up could keep the eight numeric bits in the engine, install neutral defaults,
and let a project setting or game-module setup supply bit-to-label definitions to its
EngineContext's reflection registry before scene loading. The Inspector, scene flag
serializer and Lua reflected setters would then read the same per-context labels.
LightYears should supply its current labels and legacy aliases itself. Preserve numeric
bit assignments and support old names when loading existing scenes/prefabs; validate
unique labels and bits, and avoid modifying a shared/process-global descriptor.

## Build and suite results

Build: `cmd /c tools\vega-build.cmd -j 4`, exit **0**.
Log: [review-build-final.log](../build/review-build-final.log).

Full suite run by the worker:

```text
ctest --test-dir build -j 8 --timeout 300 --output-on-failure --output-junit D:/Vega/build/review-tests.xml
```

| Run | Tests | Passed | Known failures | Unexpected failures |
| --- | ---: | ---: | ---: | ---: |
| Documented baseline | 165 | 141 | 24 | 0 |
| Final | 167 | 143 | 24 | 0 |

Full-suite wall time: **173.81 seconds**. CTest returned nonzero for the same 24
baseline failures: 23 Failed and `LightYearsRareZoneRulesE2E` Timeout. This includes
the documented sandbox-only `LightYearsAbilityBalance-second-E2E` failure.
Every required engine, reflection, scripting, context, scene/project/prefab,
diagnostics, optional module, Blank and editor test passed.

No existing test status changed. Added test statuses:

| Test | Before | Final |
| --- | --- | --- |
| LightYearsContentBootstrapFailure | Not present | Passed (1.50 seconds) |
| BlankSamplesLoad | Not present | Passed (55.50 seconds) |

All four Blank scenes (`hierarchy`, `main`, `patrol`, `trigger`) passed in both
hosts. No sample content corrections were necessary. The new test positions the
trigger player in edit mode before Play; scene-history movement during Play is
rejected by the existing editor. It checks that the three spawned Sparks expire
and the live BlankPlayer count returns to three.

The final asset verification compares file count, SHA-256 contents and modification
times with the pre-edit manifest: **357 before, 357 after, zero changes** in
`LightYearsGame/assets`. Tests use build-tree copies for their writes.

Evidence:

- [Full test log](../build/review-tests.log)
- [JUnit results](../build/review-tests.xml)
- [Baseline comparison and source-asset verification](../build/review-test-summary.json)
- [Focused regression run](../build/review-regressions.log)
- [Per-scene sample logs, automation and captures](../build/blank-samples-test)

Intermediate development checks found two fixture mistakes, both fixed before the
full run: the direct audio-manager regression initially used an unrooted WAV path,
and the first sample automation attempted scene-history movement during Play.
The final audio and sample tests passed. No production change was needed for either
fixture correction.
