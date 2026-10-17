# EngineContext validation (2026-10-17)

Implemented the gradual engine/editor service migration. No git commands were
used. LightYearsGame files, including all test hosts and production sources,
match their committed content. Authoring assets were not modified. No scene,
prefab, project or reflection golden format/file was changed.

## API and changed files

### Wave 9 GAS schema registration

GAS now requires `sas::RegisterContentSchemas(vega::EngineContext&)`, with no
ownerless overload. Only the destination context's missing ability/effect
descriptors are installed; game overrides and schema contents are preserved.
The editor constructs `EditorExtensions{ GetEngineContext() }` and GAS uses its
`Engine()` reference. The module provider signature and LightYears adapter stay
unchanged. Standalone extensions now require an explicit context that outlives
them; Blank's template guidance documents both runtime and editor setup.

The runtime schema test covers registration under an unrelated ambient context,
independent registration into a second context, and override preservation. The
module contract test checks explicit host propagation. New `VegaGASEditorContext`
checks actual GAS editor registration under an unrelated binding, including panel
and command installation. `VegaNoLegacyContextAccessors` now also scans
Modules/GAS runtime/editor and SpaceAbilitySystem, with no GAS exceptions.
Negative probes in all three roots fail as intended; explicit-context controls
pass (build/wave9-gas-guard-checks.json).

Built with `cmd /c tools\vega-build.cmd` (Debug/MSVC/C++17): final exit 0.
Full CTest used `ctest --test-dir build -j 8 --timeout 300`, adding
`--output-on-failure --output-junit` for evidence:

| Run | Tests | Passed | Failed | Timeout | CTest exit |
| --- | ---: | ---: | ---: | ---: | ---: |
| Wave 9 baseline | 183 | 159 | 23 | 1 | 8 |
| Wave 9 final | 184 | 160 | 23 | 1 | 8 |

Every existing test retained its exact status. All 24 unsuccessful results match
the documented list below: 23 Failed (including the documented sandbox-only
AbilityBalance-second-E2E failure) plus RareZoneRulesE2E's existing 120-second
Timeout. No unexpected failures occurred. The only new test,
VegaGASEditorContext, passed; GAS schemas, module contract, legacy accessor guard,
Blank tests and reflection golden also passed. Final wall time was 120 seconds.

Build attempts are retained in build/wave9-gas-build.log; final build exit is in
build/wave9-gas-build-exit.txt. The initial editor test wiring caused a linker
error, and its initial command check named an unsupported command. Both test
issues were corrected before the final build/run. Initial test evidence is in
build/wave9-gas-tests-initial.{log,xml}. Final results are in
build/wave9-gas-tests.{log,xml}; baseline evidence is in
build/wave9-gas-baseline.{log,xml}. build/wave9-gas-summary.json records the exact
status comparison and protected-file verification.

All 728 protected files (LightYearsGame/src, all 357 LightYearsGame/assets files,
and the reflection golden) retained SHA-256 contents and last-write timestamps.
Only read-only git commands were used. No network installation was performed.
Separate Release and modules-disabled configurations were not run. GAS runtime
global catalogs and other previously documented migration work remain unchanged.

Wave 9 file inventory (16):

- VegaEngine/include/framework/GameModule.h
- Modules/GAS/include/gas/GasContentSchemas.h
- Modules/GAS/src/GasContentSchemas.cpp
- Modules/GAS/tests/GasContentSchemasTests.cpp
- Modules/GAS/editor/CMakeLists.txt
- Modules/GAS/editor/src/GasEditorExtension.cpp
- Modules/GAS/editor/tests/GasEditorContextTests.cpp
- VegaEditor/include/editor/EditorExtensions.h
- VegaEditor/src/EditorApplication.cpp
- VegaEditor/tests/GameModuleTests.cpp
- Templates/Blank/README.md
- tools/CheckContextAccessors.cmake
- docs/engine-context.md
- docs/engine-context-validation.md
- docs/engine-modules.md
- docs/game-module.md

### Prior migration

See [EngineContext](engine-context.md) for ownership, injection, compatibility
bindings and lifetime rules. The core API is in
[EngineContext.h](../VegaEngine/include/framework/EngineContext.h).
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
calls are not legacy singleton calls. The before/after columns below record this wave 7 step 3 follow-up, after the
source split at 20144ab. Counts were checked by build/context-audit.cjs;
build/context-followup-before-audit.json preserves the input audit and
build/context-accessor-audit.json records the result. The earlier content count
of 15 is now 14 in the current sources. Constructor/default-registration compatibility
boundaries using Current() remain intentionally and are documented separately.

| Service | Before: engine/editor/script | After: engine/editor/script | Remaining in other production code |
| --- | ---: | ---: | ---: |
| TypeRegistry | 0 | 0 | 0 |
| PrefabLibrary | 0 | 0 | 0 |
| AssetManager | 0 | 0 | 6 |
| EngineAssets | 0 | 0 | 1 |
| AudioManager | 0 | 0 | 10 |
| SceneSettingsSchema | 5 (0 / 5 / 0) | 0 | 4 |
| ContentSchemaRegistry | 14 (0 / 14 / 0) | 0 | 2 |
| ScriptRuntime | 1 (SetScriptRoot compatibility wrapper) | 0 | 3 |

Other production code means LightYearsGame, Templates, optional modules and
LightYearsVisual. Existing tests also intentionally exercise compatibility APIs;
their counts, including untouched game test hosts, are in the audit JSON.

The follow-up passes the host EngineContext into ContentWorkspace and its
authoring panel. Workspace lookups, reference edits, validation and undo retain
that context; the workspace test runs under an unrelated ambient context. Scene
settings commands use EditorContext::Engine(), and the application panel uses
GetEngineContext(). SetScriptRoot remains an ownerless compatibility boundary
and explicitly forwards to ScriptRuntime::Get(EngineContext::Current()).

VegaNoLegacyContextAccessors runs tools/CheckContextAccessors.cmake over production
include/src files in all three roots. Its allow-list removes only the eight shim
definition signatures at their exact paths; calls in those files are still
checked. Negative probes for all eight services (including whitespace, comments
and line breaks), a shim-file call, and an explicit-context control passed; see
build/context-followup-guard-checks.json.

Modules follow-up: Sky has no legacy calls to these eight services. GAS now uses
`RegisterContentSchemas(EngineContext&)`; its editor adapter gets the explicit
host from `EditorExtensions::Engine()`. The original follow-up did not edit
Modules, LightYearsGame or LightYearsVisual; the Wave 9 GAS changes are below.

Follow-up file inventory (14):

- VegaEngine/CMakeLists.txt
- tools/CheckContextAccessors.cmake
- VegaEditor/include/editor/ContentWorkspace.h
- VegaEditor/include/editor/EditorContentAuthoring.h
- VegaEditor/src/ContentWorkspace.cpp
- VegaEditor/src/EditorContentAuthoring.cpp
- VegaEditor/src/EditorApplication.cpp
- VegaEditor/src/EditorPropertyCommands.cpp
- VegaEditor/src/EditorWorldSettings.cpp
- VegaEditor/src/commands/SettingsScriptCommands.cpp
- VegaEditor/tests/ContentWorkspaceTests.cpp
- VegaScript/src/ScriptRuntime.cpp
- docs/engine-context-validation.md
- docs/engine-context.md

## Follow-up build and tests

Built with cmd /c tools\vega-build.cmd: exit 0. The existing vswhere.exe warning
remains harmless; build/context-followup-build.log contains the build output.

Ran ctest --test-dir build -j 8 --timeout 300 for both runs, adding
--output-on-failure and --output-junit to retain evidence.

| Run | Tests | Passed | Known failures | CTest exit | Wall time |
| --- | ---: | ---: | ---: | ---: | ---: |
| Follow-up baseline | 169 | 145 | 24 | 8 | 120.06 sec |
| Follow-up final | 170 | 146 | 24 | 8 | 120.06 sec |

Every existing test retained its exact status, including the same 23 Failed and
one Timeout results listed below. The only added test is
VegaNoLegacyContextAccessors: Passed. The workspace isolation regression and
LightYearsReflectionRegistryGolden both passed. No behavior or file-format
change was introduced.

Logs and machine-readable results: build/context-followup-{baseline,final}.log,
build/context-followup-{baseline,final}.xml and
build/context-followup-test-summary.json.

Source verification found no content or timestamp changes in LightYearsGame or
LightYearsVisual, including all 357 game asset files. The reflection golden also
retained its content and timestamp; its current SHA-256 is
E6DCEBA600C992899EF50B50DD81AB48D199F489EF71C8C58D81BA208A3448BD.
See build/context-followup-source-verification.json. No git commands were run.

## Original migration build and tests

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
build/VegaEngine/context-baseline-benchmark.exe. Both executables were run
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
build/VegaEngine/context-baseline-perf.txt overlapped CTest and is excluded.
The resumed measurements use before/after/after/before ordering. Executable
hashes, exit statuses and elapsed times are in build/context-perf-metadata.json.

## Remaining work

- Engine/editor/script production calls to all eight deprecated accessors are
  eliminated and guarded by VegaNoLegacyContextAccessors. Compatibility entry
  boundaries and shim APIs remain available to downstream hosts.
- Migrate game/optional-module setup and game-owned catalogs. LightYears'
  process-wide RegisterGameTypes guard still prevents registering that game
  twice. Its gameplay files were deliberately left untouched.
- Give global/game timers explicit owners; isolate ShaderManager's named cache,
  optional GAS/Sky runtime registries and diagnostics/profiler state separately. This is
  service isolation for the migrated engine layer, not complete isolation of all
  existing LightYears/global infrastructure.
- Remove deprecated static accessors and ownerless compatibility constructors
  only after downstream game branches migrate.

## File inventory (88)

- [VegaEngine/CMakeLists.txt](../VegaEngine/CMakeLists.txt)
- [VegaEngine/include/engineConfigs/EngineStructs.h](../VegaEngine/include/engineConfigs/EngineStructs.h)
- [VegaEngine/include/framework/Actor.h](../VegaEngine/include/framework/Actor.h)
- [VegaEngine/include/framework/Application.h](../VegaEngine/include/framework/Application.h)
- [VegaEngine/include/framework/AssetManager.h](../VegaEngine/include/framework/AssetManager.h)
- [VegaEngine/include/framework/AudioManager.h](../VegaEngine/include/framework/AudioManager.h)
- [VegaEngine/include/framework/EngineAssets.h](../VegaEngine/include/framework/EngineAssets.h)
- [VegaEngine/include/framework/EngineContext.h](../VegaEngine/include/framework/EngineContext.h)
- [VegaEngine/include/framework/ShaderManager.h](../VegaEngine/include/framework/ShaderManager.h)
- [VegaEngine/include/framework/SoundComponent.h](../VegaEngine/include/framework/SoundComponent.h)
- [VegaEngine/include/framework/World.h](../VegaEngine/include/framework/World.h)
- [VegaEngine/include/framework/content/ContentSchema.h](../VegaEngine/include/framework/content/ContentSchema.h)
- [VegaEngine/include/framework/reflection/Reflection.h](../VegaEngine/include/framework/reflection/Reflection.h)
- [VegaEngine/include/framework/reflection/ReflectionDeclare.h](../VegaEngine/include/framework/reflection/ReflectionDeclare.h)
- [VegaEngine/include/framework/reflection/ReflectionMacros.h](../VegaEngine/include/framework/reflection/ReflectionMacros.h)
- [VegaEngine/include/framework/scene/PrefabLibrary.h](../VegaEngine/include/framework/scene/PrefabLibrary.h)
- [VegaEngine/include/framework/scene/Scene.h](../VegaEngine/include/framework/scene/Scene.h)
- [VegaEngine/include/framework/scene/SceneSettings.h](../VegaEngine/include/framework/scene/SceneSettings.h)
- [VegaEngine/include/widget/Button.h](../VegaEngine/include/widget/Button.h)
- [VegaEngine/include/widget/ImageWidget.h](../VegaEngine/include/widget/ImageWidget.h)
- [VegaEngine/include/widget/TextWidget.h](../VegaEngine/include/widget/TextWidget.h)
- [VegaEngine/include/widget/UIStyle.h](../VegaEngine/include/widget/UIStyle.h)
- [VegaEngine/include/widget/ValueGauge.h](../VegaEngine/include/widget/ValueGauge.h)
- [VegaEngine/include/widget/Widget.h](../VegaEngine/include/widget/Widget.h)
- [VegaEngine/src/framework/Actor.cpp](../VegaEngine/src/framework/Actor.cpp)
- [VegaEngine/src/framework/Application.cpp](../VegaEngine/src/framework/Application.cpp)
- [VegaEngine/src/framework/AssetManager.cpp](../VegaEngine/src/framework/AssetManager.cpp)
- [VegaEngine/src/framework/AudioManager.cpp](../VegaEngine/src/framework/AudioManager.cpp)
- [VegaEngine/src/framework/BackgroundLayer.cpp](../VegaEngine/src/framework/BackgroundLayer.cpp)
- [VegaEngine/src/framework/ComponentIdentity.cpp](../VegaEngine/src/framework/ComponentIdentity.cpp)
- [VegaEngine/src/framework/EngineContext.cpp](../VegaEngine/src/framework/EngineContext.cpp)
- [VegaEngine/src/framework/LightComponent.cpp](../VegaEngine/src/framework/LightComponent.cpp)
- [VegaEngine/src/framework/ShaderManager.cpp](../VegaEngine/src/framework/ShaderManager.cpp)
- [VegaEngine/src/framework/SoundComponent.cpp](../VegaEngine/src/framework/SoundComponent.cpp)
- [VegaEngine/src/framework/SpriteComponent.cpp](../VegaEngine/src/framework/SpriteComponent.cpp)
- [VegaEngine/src/framework/World.cpp](../VegaEngine/src/framework/World.cpp)
- [VegaEngine/src/framework/reflection/EngineReflection.cpp](../VegaEngine/src/framework/reflection/EngineReflection.cpp)
- [VegaEngine/src/framework/reflection/Reflection.cpp](../VegaEngine/src/framework/reflection/Reflection.cpp)
- [VegaEngine/src/framework/scene/PrefabLibrary.cpp](../VegaEngine/src/framework/scene/PrefabLibrary.cpp)
- [VegaEngine/src/framework/scene/Scene.cpp](../VegaEngine/src/framework/scene/Scene.cpp)
- [VegaEngine/src/framework/scene/SceneSettings.cpp](../VegaEngine/src/framework/scene/SceneSettings.cpp)
- [VegaEngine/src/widget/Button.cpp](../VegaEngine/src/widget/Button.cpp)
- [VegaEngine/src/widget/ImageWidget.cpp](../VegaEngine/src/widget/ImageWidget.cpp)
- [VegaEngine/src/widget/TextWidget.cpp](../VegaEngine/src/widget/TextWidget.cpp)
- [VegaEngine/src/widget/UIStyle.cpp](../VegaEngine/src/widget/UIStyle.cpp)
- [VegaEngine/src/widget/ValueGauge.cpp](../VegaEngine/src/widget/ValueGauge.cpp)
- [VegaEngine/src/widget/Widget.cpp](../VegaEngine/src/widget/Widget.cpp)
- [VegaEngine/tests/EngineContextTests.cpp](../VegaEngine/tests/EngineContextTests.cpp)
- [VegaEngine/tests/EngineLifetimeTests.cpp](../VegaEngine/tests/EngineLifetimeTests.cpp)
- [VegaEngine/tests/HierarchyCostTests.cpp](../VegaEngine/tests/HierarchyCostTests.cpp)
- [VegaEngine/tests/HierarchyTests.cpp](../VegaEngine/tests/HierarchyTests.cpp)
- [VegaEngine/tests/PrefabLibraryTests.cpp](../VegaEngine/tests/PrefabLibraryTests.cpp)
- [VegaEngine/tests/ReflectionMacroTests.cpp](../VegaEngine/tests/ReflectionMacroTests.cpp)
- [VegaEngine/tests/ReflectionRegistryGoldenTests.cpp](../VegaEngine/tests/ReflectionRegistryGoldenTests.cpp)
- [VegaEngine/tests/ReflectionTests.cpp](../VegaEngine/tests/ReflectionTests.cpp)
- [VegaEngine/tests/RenderViewportTests.cpp](../VegaEngine/tests/RenderViewportTests.cpp)
- [VegaEngine/tests/SceneBoundsTests.cpp](../VegaEngine/tests/SceneBoundsTests.cpp)
- [VegaEngine/tests/SceneSerializerTests.cpp](../VegaEngine/tests/SceneSerializerTests.cpp)
- [VegaEngine/tests/UIFoundationTests.cpp](../VegaEngine/tests/UIFoundationTests.cpp)
- [VegaEngine/tests/WorldSpatialQueryTests.cpp](../VegaEngine/tests/WorldSpatialQueryTests.cpp)
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
