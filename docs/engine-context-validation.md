# EngineContext validation (2026-10-17)

Implemented the gradual engine/editor service migration. No git commands were
used. LightYearsGame files, including all test hosts and production sources,
match their committed content. Authoring assets were not modified. No scene,
prefab, project or reflection golden format/file was changed.

## API and changed files

See [EngineContext](engine-context.md) for ownership, injection, compatibility
bindings and lifetime rules. The core API is in
[EngineContext.h](../LightYearsEngine/include/framework/EngineContext.h).
Application adopts the current context by default and accepts an explicit
host-owned context for isolation; it never owns either context. World, Actor,
Widget and EditorContext retain access to their host's services. The optional
VegaScript slot owns its interpreter without an engine dependency on Lua.

The four required services are migrated throughout production engine/editor/script
code. Audio, schema ownership, scene extension hooks and UI style were small
necessary ownership follow-ups. Scene constructor inventories now belong to
TypeInfo; compatibility scope removal is safe when hosts close out of order.
Twenty-two existing service-using test executables now create an EngineContext.
The UI defaults fixture no longer resets shared defaults/style to clean up.
All edits to the 57 game test source files were reversed and checked against
committed content without git commands. No LightYearsGame source change is
needed: default Application construction retains the current context's configured
asset root, defaults and registrations. Engine/editor/script fixture contexts and
two-context isolation checks remain. LightYearsEngineContext now also checks
preconfigured asset roots and types through default Applications with both the
legacy context and an explicit Scope, nested adoption, explicit Application
isolation, surviving services after destruction, and out-of-order host destruction.
The retained Application Scope uses the existing unlinking implementation.
EngineContext::Current() still owns its function-local static legacy context;
Application shutdown still tears down worlds, audio, timers, shaders and assets
in that order. Context destruction still releases scripts before audio and assets.

## Call-site audit

Counts are textual invocations in first-party production .h/.cpp files,
excluding accessor definitions, tests and docs. Explicit-context Get(context)
calls are not legacy singleton calls. The migrated column is the pre-edit audit
of engine/editor/script call sites changed or eliminated in this step. Remaining
counts were checked by build/context-audit.cjs and recorded in
build/context-accessor-audit.json. Constructor/default-registration compatibility
boundaries using Current() remain intentionally and are documented separately.

| Service | Migrated in engine/editor/script | Remaining legacy calls there | Remaining in other production code |
| --- | ---: | ---: | ---: |
| TypeRegistry | 37 (19 / 10 / 8) | 0 | 0 |
| PrefabLibrary | 22 (2 / 19 / 1) | 0 | 0 |
| AssetManager | 14 (14 / 0 / 0) | 0 | 7 |
| EngineAssets | 13 (13 / 0 / 0) | 0 | 1 |
| AudioManager | 15 (7 / 0 / 8) | 0 | 10 |
| SceneSettingsSchema | 3 (1 / 2 / 0) | 5 (0 / 5 / 0) | 4 |
| ContentSchemaRegistry | 0 | 15 (0 / 15 / 0) | 4 |
| ScriptRuntime | 11 (0 / 1 / 10) | 1 (SetScriptRoot compatibility wrapper) | 3 |

Other production code means LightYearsGame, Templates, optional modules and
LightYearsVisual. Existing tests also intentionally exercise compatibility APIs;
their counts, including untouched game test hosts, are in the audit JSON.

## Build and tests

Built with cmd /c tools\vega-build.cmd, Debug/MSVC/C++17. The existing vcvars
setup emits a vswhere.exe warning in this environment; the final build's actual
exit code is reported by the invoking shell. The adoption-fix build exited 0;
see build/context-adopt-build.log.

Baseline and final suite: ctest --test-dir build -j 8 --timeout 300
(with --output-on-failure added for the saved logs).
The baseline is the preserved pre-migration run from the interrupted task; the
final suite was rerun after default adoption and restoration of all game hosts.

| Run | Tests | Passed | Known failures |
| --- | ---: | ---: | ---: |
| Baseline | 163 | 139 | 24 |
| Final | 165 | 141 | 24 |

Final suite wall time: 166.28 sec.
CTest exited 8 for the same 24 known failures (23 Failed, one Timeout); the build
exited 0. No existing test status changed versus either the pre-migration baseline
or the prior validation run.

Every required engine, reflection, scripting, scene/project/prefab, diagnostics,
optional module, Blank, editor, hierarchy, properties, asset/autosave and registry
test passed. The reflection registry golden remained unchanged. Full logs:
build/context-baseline-tests.log and build/context-adopt-tests.log; the final
machine-readable run is build/context-adopt-tests.xml. Status comparisons with
both the pre-migration baseline and the prior validation run are recorded in
build/context-adopt-test-summary.json.

Changed statuses:

| Test | Before | After |
| --- | --- | --- |
| LightYearsEngineContext | Not present | Passed |
| VegaScriptContext | Not present | Passed |

The initial adoption-fix full run stalled in the new windowed EngineContext
regression; that test was stopped after 75.35 seconds (not a CTest timeout).
The cause was not identified. A focused diagnostic run passed, temporary logging
was removed, and the final full suite passed every required test. Five further
consecutive EngineContext runs passed in 0.38-0.40 seconds each, exit 0; see
build/context-adopt-repeat.log. The initial run is retained in
build/context-adopt-tests-initial.log. No application/context logic changed
between those runs.

Existing test status changes versus the pre-migration baseline: none.
The interrupted WIP's local-schema regression in LightYearsSceneSettings was
fixed during this resume: standalone schemas start empty; EngineContext alone
installs engine fields. Bounds loading and editor tools use the owning schema,
with a regression check under a different ambient context.

The first full resume run (build/context-resume-tests-initial.log) exposed nine
additional timeouts from game test hosts losing their preconfigured asset root:
LightYearsVitalsHUD, LightYearsModuleScreen, LightYearsHUDMigrationWarning,
LightYearsHUDMigrationNotification, LightYearsHUDMigrationEncounter,
LightYearsHUDMigrationAbility, LightYearsHUDMigrationMenus,
LightYearsTimerManagerSceneE2E and LightYearsGameHUDDamageEventE2E. Default context adoption
restores their baseline behavior without changing the game test hosts. Three known failing tests also temporarily
timed out in that run: LightYearsCommonModuleE2E, LightYearsPlayerShipSelection
and LightYearsUncommonModuleP7E2E. The final table is the baseline comparison
after fixing the Application default, not the intermediate run.

The final failures below are the pre-existing gameplay set plus the documented
sandbox-only GetCursorPos failure in LightYearsAbilityBalance-second-E2E:

- LightYearsPrimaryWeapon: Failed
- LightYearsCommonModuleE2E: Failed
- LightYearsUncommonModuleP3E2E: Failed
- LightYearsCommonModuleTriggerLifetime: Failed
- LightYearsPlayerShipSelection: Failed
- LightYearsRareSingleModulesE2E: Failed
- LightYearsRareWave3ModulesE2E: Failed
- LightYearsRareWave4PolishE2E: Failed
- LightYearsRareFragmentationE2E: Failed
- LightYearsRareLadderModulesE2E: Failed
- LightYearsRareWave5ContentE2E: Failed
- LightYearsUncommonModuleP4E2E: Failed
- LightYearsUncommonModuleP5E2E: Failed
- LightYearsModuleBalanceE2E: Failed
- LightYearsUncommonModuleP6E2E: Failed
- LightYearsRareSpareCapacitorMechanicsE2E: Failed
- LightYearsWeaponPhase2: Failed
- LightYearsUncommonModuleCompletionE2E: Failed
- LightYearsContinuousBeamWallE2E: Failed
- LightYearsAuditFixesE2E: Failed
- LightYearsAbilityBalance-second-E2E: Failed
- LightYearsGasLiteCore: Failed
- LightYearsUncommonModuleP7E2E: Failed
- LightYearsRareZoneRulesE2E: Timeout

Source asset verification covers all 357 LightYearsGame/assets files, including
SHA-256 contents and last-write timestamps; none changed during validation.
See build/context-source-assets-verification.json. All 1,493 tracked game files
match committed content (including 57 byte-for-byte restored test files); see
build/context-adopt-game-verification.json. The reflection golden's
SHA-256 remained 4A58A386D9625DF82FFBAA957B60A9C4A83EB29B78939B62E7C4CD721984EFA7.

## Busy-scene comparison

Used the existing LightYearsHierarchyCostTests benchmark in Debug, with 500 flat
actors and 500 actors attached to one moving root. Each executable reports the
median of ten batches of 2,000 frames after one warm-up batch. The pre-change
executable was preserved before any affected executable was rebuilt as
build/LightYearsEngine/context-baseline-benchmark.exe. Both executables were run
twice, sequentially, with compilation and CTest idle. Times below are microseconds
per headless frame; they exclude context/scene construction and rendering.

| Scene | Before run 1 | After run 1 | Before run 2 | After run 2 | Mean change |
| --- | ---: | ---: | ---: | ---: | ---: |
| 500 flat actors | 263.293 | 258.729 | 265.845 | 264.177 | -1.18% |
| 500 parented actors + root | 1807.08 | 1740.71 | 1774.7 | 1781.25 | -1.67% |

This measures busy engine ticking and hierarchy propagation, not GPU rendering or
all possible game workloads. No service lookup or scope rebinding was added to
the frame/actor tick loop. Object access follows retained context pointers;
script components cache their interpreter reference. Core type-registration
idempotence uses a registry-local bool, avoiding a lookup on repeated scene saves.
No slowdown was observed in these samples; the small differences do not establish
a speedup. Raw measurements: build/context-perf-{before,after}-{1,2}.log.
The earlier interrupted-run measurement in
build/LightYearsEngine/context-baseline-perf.txt overlapped CTest and is excluded.
The resumed measurements use before/after/after/before ordering. Executable
hashes, exit statuses and elapsed times are in build/context-perf-metadata.json.

## Remaining work

- Pass contexts through the content workspace/asset planning and remaining scene
  settings APIs, then retire their forwarding calls.
- Migrate game/optional-module setup and game-owned catalogs. LightYears'
  process-wide RegisterGameTypes guard still prevents registering that game
  twice. Its gameplay files were deliberately left untouched.
- Give global/game timers explicit owners; isolate ShaderManager's named cache,
  optional GAS/Sky registries and diagnostics/profiler state separately. This is
  service isolation for the migrated engine layer, not complete isolation of all
  existing LightYears/global infrastructure.
- Remove deprecated static accessors and ownerless compatibility constructors
  only after downstream game branches migrate.

## File inventory (88)

- [LightYearsEngine/CMakeLists.txt](../LightYearsEngine/CMakeLists.txt)
- [LightYearsEngine/include/engineConfigs/EngineStructs.h](../LightYearsEngine/include/engineConfigs/EngineStructs.h)
- [LightYearsEngine/include/framework/Actor.h](../LightYearsEngine/include/framework/Actor.h)
- [LightYearsEngine/include/framework/Application.h](../LightYearsEngine/include/framework/Application.h)
- [LightYearsEngine/include/framework/AssetManager.h](../LightYearsEngine/include/framework/AssetManager.h)
- [LightYearsEngine/include/framework/AudioManager.h](../LightYearsEngine/include/framework/AudioManager.h)
- [LightYearsEngine/include/framework/EngineAssets.h](../LightYearsEngine/include/framework/EngineAssets.h)
- [LightYearsEngine/include/framework/EngineContext.h](../LightYearsEngine/include/framework/EngineContext.h)
- [LightYearsEngine/include/framework/ShaderManager.h](../LightYearsEngine/include/framework/ShaderManager.h)
- [LightYearsEngine/include/framework/SoundComponent.h](../LightYearsEngine/include/framework/SoundComponent.h)
- [LightYearsEngine/include/framework/World.h](../LightYearsEngine/include/framework/World.h)
- [LightYearsEngine/include/framework/content/ContentSchema.h](../LightYearsEngine/include/framework/content/ContentSchema.h)
- [LightYearsEngine/include/framework/reflection/Reflection.h](../LightYearsEngine/include/framework/reflection/Reflection.h)
- [LightYearsEngine/include/framework/reflection/ReflectionDeclare.h](../LightYearsEngine/include/framework/reflection/ReflectionDeclare.h)
- [LightYearsEngine/include/framework/reflection/ReflectionMacros.h](../LightYearsEngine/include/framework/reflection/ReflectionMacros.h)
- [LightYearsEngine/include/framework/scene/PrefabLibrary.h](../LightYearsEngine/include/framework/scene/PrefabLibrary.h)
- [LightYearsEngine/include/framework/scene/Scene.h](../LightYearsEngine/include/framework/scene/Scene.h)
- [LightYearsEngine/include/framework/scene/SceneSettings.h](../LightYearsEngine/include/framework/scene/SceneSettings.h)
- [LightYearsEngine/include/widget/Button.h](../LightYearsEngine/include/widget/Button.h)
- [LightYearsEngine/include/widget/ImageWidget.h](../LightYearsEngine/include/widget/ImageWidget.h)
- [LightYearsEngine/include/widget/TextWidget.h](../LightYearsEngine/include/widget/TextWidget.h)
- [LightYearsEngine/include/widget/UIStyle.h](../LightYearsEngine/include/widget/UIStyle.h)
- [LightYearsEngine/include/widget/ValueGauge.h](../LightYearsEngine/include/widget/ValueGauge.h)
- [LightYearsEngine/include/widget/Widget.h](../LightYearsEngine/include/widget/Widget.h)
- [LightYearsEngine/src/framework/Actor.cpp](../LightYearsEngine/src/framework/Actor.cpp)
- [LightYearsEngine/src/framework/Application.cpp](../LightYearsEngine/src/framework/Application.cpp)
- [LightYearsEngine/src/framework/AssetManager.cpp](../LightYearsEngine/src/framework/AssetManager.cpp)
- [LightYearsEngine/src/framework/AudioManager.cpp](../LightYearsEngine/src/framework/AudioManager.cpp)
- [LightYearsEngine/src/framework/BackgroundLayer.cpp](../LightYearsEngine/src/framework/BackgroundLayer.cpp)
- [LightYearsEngine/src/framework/ComponentIdentity.cpp](../LightYearsEngine/src/framework/ComponentIdentity.cpp)
- [LightYearsEngine/src/framework/EngineContext.cpp](../LightYearsEngine/src/framework/EngineContext.cpp)
- [LightYearsEngine/src/framework/LightComponent.cpp](../LightYearsEngine/src/framework/LightComponent.cpp)
- [LightYearsEngine/src/framework/ShaderManager.cpp](../LightYearsEngine/src/framework/ShaderManager.cpp)
- [LightYearsEngine/src/framework/SoundComponent.cpp](../LightYearsEngine/src/framework/SoundComponent.cpp)
- [LightYearsEngine/src/framework/SpriteComponent.cpp](../LightYearsEngine/src/framework/SpriteComponent.cpp)
- [LightYearsEngine/src/framework/World.cpp](../LightYearsEngine/src/framework/World.cpp)
- [LightYearsEngine/src/framework/reflection/EngineReflection.cpp](../LightYearsEngine/src/framework/reflection/EngineReflection.cpp)
- [LightYearsEngine/src/framework/reflection/Reflection.cpp](../LightYearsEngine/src/framework/reflection/Reflection.cpp)
- [LightYearsEngine/src/framework/scene/PrefabLibrary.cpp](../LightYearsEngine/src/framework/scene/PrefabLibrary.cpp)
- [LightYearsEngine/src/framework/scene/Scene.cpp](../LightYearsEngine/src/framework/scene/Scene.cpp)
- [LightYearsEngine/src/framework/scene/SceneSettings.cpp](../LightYearsEngine/src/framework/scene/SceneSettings.cpp)
- [LightYearsEngine/src/widget/Button.cpp](../LightYearsEngine/src/widget/Button.cpp)
- [LightYearsEngine/src/widget/ImageWidget.cpp](../LightYearsEngine/src/widget/ImageWidget.cpp)
- [LightYearsEngine/src/widget/TextWidget.cpp](../LightYearsEngine/src/widget/TextWidget.cpp)
- [LightYearsEngine/src/widget/UIStyle.cpp](../LightYearsEngine/src/widget/UIStyle.cpp)
- [LightYearsEngine/src/widget/ValueGauge.cpp](../LightYearsEngine/src/widget/ValueGauge.cpp)
- [LightYearsEngine/src/widget/Widget.cpp](../LightYearsEngine/src/widget/Widget.cpp)
- [LightYearsEngine/tests/EngineContextTests.cpp](../LightYearsEngine/tests/EngineContextTests.cpp)
- [LightYearsEngine/tests/EngineLifetimeTests.cpp](../LightYearsEngine/tests/EngineLifetimeTests.cpp)
- [LightYearsEngine/tests/HierarchyCostTests.cpp](../LightYearsEngine/tests/HierarchyCostTests.cpp)
- [LightYearsEngine/tests/HierarchyTests.cpp](../LightYearsEngine/tests/HierarchyTests.cpp)
- [LightYearsEngine/tests/PrefabLibraryTests.cpp](../LightYearsEngine/tests/PrefabLibraryTests.cpp)
- [LightYearsEngine/tests/ReflectionMacroTests.cpp](../LightYearsEngine/tests/ReflectionMacroTests.cpp)
- [LightYearsEngine/tests/ReflectionRegistryGoldenTests.cpp](../LightYearsEngine/tests/ReflectionRegistryGoldenTests.cpp)
- [LightYearsEngine/tests/ReflectionTests.cpp](../LightYearsEngine/tests/ReflectionTests.cpp)
- [LightYearsEngine/tests/RenderViewportTests.cpp](../LightYearsEngine/tests/RenderViewportTests.cpp)
- [LightYearsEngine/tests/SceneBoundsTests.cpp](../LightYearsEngine/tests/SceneBoundsTests.cpp)
- [LightYearsEngine/tests/SceneSerializerTests.cpp](../LightYearsEngine/tests/SceneSerializerTests.cpp)
- [LightYearsEngine/tests/UIFoundationTests.cpp](../LightYearsEngine/tests/UIFoundationTests.cpp)
- [LightYearsEngine/tests/WorldSpatialQueryTests.cpp](../LightYearsEngine/tests/WorldSpatialQueryTests.cpp)
- [VegaEditor/include/editor/EditorCommands.h](../VegaEditor/include/editor/EditorCommands.h)
- [VegaEditor/src/EditorApplication.cpp](../VegaEditor/src/EditorApplication.cpp)
- [VegaEditor/src/EditorAssetOperations.cpp](../VegaEditor/src/EditorAssetOperations.cpp)
- [VegaEditor/src/EditorAutosave.cpp](../VegaEditor/src/EditorAutosave.cpp)
- [VegaEditor/src/EditorCommands.cpp](../VegaEditor/src/EditorCommands.cpp)
- [VegaEditor/src/EditorGizmos.cpp](../VegaEditor/src/EditorGizmos.cpp)
- [VegaEditor/src/EditorPanels.cpp](../VegaEditor/src/EditorPanels.cpp)
- [VegaEditor/src/EditorPrefabs.cpp](../VegaEditor/src/EditorPrefabs.cpp)
- [VegaEditor/src/EditorScenes.cpp](../VegaEditor/src/EditorScenes.cpp)
- [VegaEditor/tests/AssetOperationsTests.cpp](../VegaEditor/tests/AssetOperationsTests.cpp)
- [VegaEditor/tests/ContentWorkspaceTests.cpp](../VegaEditor/tests/ContentWorkspaceTests.cpp)
- [VegaEditor/tests/EditorCommandTests.cpp](../VegaEditor/tests/EditorCommandTests.cpp)
- [VegaEditor/tests/EditorSelectionTests.cpp](../VegaEditor/tests/EditorSelectionTests.cpp)
- [VegaEditor/tests/GameExporterTests.cpp](../VegaEditor/tests/GameExporterTests.cpp)
- [VegaEditor/tests/GameModuleTests.cpp](../VegaEditor/tests/GameModuleTests.cpp)
- [VegaEditor/tests/MultiSelectionTests.cpp](../VegaEditor/tests/MultiSelectionTests.cpp)
- [VegaScript/CMakeLists.txt](../VegaScript/CMakeLists.txt)
- [VegaScript/include/script/ScriptRuntime.h](../VegaScript/include/script/ScriptRuntime.h)
- [VegaScript/src/LuaTypes.cpp](../VegaScript/src/LuaTypes.cpp)
- [VegaScript/src/ScriptComponent.cpp](../VegaScript/src/ScriptComponent.cpp)
- [VegaScript/tests/LuaApiTests.cpp](../VegaScript/tests/LuaApiTests.cpp)
- [VegaScript/tests/RichPropertiesTests.cpp](../VegaScript/tests/RichPropertiesTests.cpp)
- [VegaScript/tests/ScriptComponentTests.cpp](../VegaScript/tests/ScriptComponentTests.cpp)
- [VegaScript/tests/ScriptContextTests.cpp](../VegaScript/tests/ScriptContextTests.cpp)
- [docs/engine-context-validation.md](../docs/engine-context-validation.md)
- [docs/engine-context.md](../docs/engine-context.md)
- [docs/game-module.md](../docs/game-module.md)
- [docs/new-game.md](../docs/new-game.md)
