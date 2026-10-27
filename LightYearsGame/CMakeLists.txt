set(LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME LightYearsGameplayCore)
set(LIGHT_YEARS_GAME_LIB_TARGET_NAME LightYearsGameLib)
set(LIGHT_YEARS_GAME_APP_TARGET_NAME LightYearsGameApp)

# Sources are discovered by directory; adding a .cpp/.h under src/ or include/ needs no CMake edit.
# Every .cpp under src/ is compiled (there are no orphan sources). The sources are split into three
# groups, each compiled exactly once:
#   LightYearsGameplayCore - reusable gameplay services that depend only on SpaceAbilitySystem
#   LightYearsGameLib      - the rest of the game that tests can link without a window or visual pipeline
#   LightYearsGameApp      - reusable game application (gameFramework/) and LightYearsVisual presenters
file(GLOB_RECURSE VEGA_GAME_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(FILTER VEGA_GAME_CPP EXCLUDE REGEX "/src/editor/")
file(GLOB_RECURSE VEGA_GAME_HEADERS CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

# Shared application setup and presenters let other executables reuse the game's visual pipeline.
set(VEGA_GAME_APP_DIR_REGEX "/(src|include)/(gameFramework|presentation/visual)/")
set(VEGA_GAME_APP_CPP ${VEGA_GAME_CPP})
list(FILTER VEGA_GAME_APP_CPP INCLUDE REGEX "${VEGA_GAME_APP_DIR_REGEX}")
set(VEGA_GAME_APP_HEADERS ${VEGA_GAME_HEADERS})
list(FILTER VEGA_GAME_APP_HEADERS INCLUDE REGEX "${VEGA_GAME_APP_DIR_REGEX}")
list(FILTER VEGA_GAME_CPP EXCLUDE REGEX "${VEGA_GAME_APP_DIR_REGEX}")
list(FILTER VEGA_GAME_HEADERS EXCLUDE REGEX "${VEGA_GAME_APP_DIR_REGEX}")

# Gameplay core: game-specific reusable services. LightYearsGameplayCore deliberately depends on
# SpaceAbilitySystem instead of extending it with LightYears contracts, content, actors, or
# presentation rules. The files
# do not share a directory, so they are selected by file name.
set(VEGA_GAMEPLAY_CORE_FILES
	MovementPolicyController NumericSettingContractRegistry GameAbilityContentRegistration
	EnergySpearContentRegistration AbilityLifecycleDispatcher AbilityUseHistory FocusActionLocks
	TemporalRateModifierLedger AbilityActionValidator GameAbilityDefinitionValidator
	GameplayEffectDefinitionValidator GameContentBootstrap ShipSchematicValidator ModuleLoadout
	LegendaryRules ModuleLoaderLegendary
	ModulePlacementRules ModuleResolver ModuleLoader ModuleLoaderPrimitives ModuleLoaderRules ModuleLoaderSideRules ModuleLoaderVariant ModuleContentCatalog ModuleSideFeatureLoader ModuleSideFeatureCatalog ModuleRoller GameAbilitySettingContracts
	LightYearsEffectBehaviorRuntime
)
string(JOIN "|" VEGA_GAMEPLAY_CORE_REGEX_BODY ${VEGA_GAMEPLAY_CORE_FILES})
set(VEGA_GAMEPLAY_CORE_REGEX "/src/.*/(${VEGA_GAMEPLAY_CORE_REGEX_BODY})\\.cpp$")
set(VEGA_GAMEPLAY_CORE_CPP ${VEGA_GAME_CPP})
list(FILTER VEGA_GAMEPLAY_CORE_CPP INCLUDE REGEX "${VEGA_GAMEPLAY_CORE_REGEX}")
list(FILTER VEGA_GAME_CPP EXCLUDE REGEX "${VEGA_GAMEPLAY_CORE_REGEX}")
list(LENGTH VEGA_GAMEPLAY_CORE_FILES VEGA_GAMEPLAY_CORE_FILE_COUNT)
list(LENGTH VEGA_GAMEPLAY_CORE_CPP VEGA_GAMEPLAY_CORE_CPP_COUNT)
if(NOT VEGA_GAMEPLAY_CORE_CPP_COUNT EQUAL VEGA_GAMEPLAY_CORE_FILE_COUNT)
	message(FATAL_ERROR "VEGA_GAMEPLAY_CORE_FILES lists ${VEGA_GAMEPLAY_CORE_FILE_COUNT} files but ${VEGA_GAMEPLAY_CORE_CPP_COUNT} were found under src/ (renamed, moved, or duplicated?): ${VEGA_GAMEPLAY_CORE_CPP}")
endif()

add_library(${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME} STATIC ${VEGA_GAMEPLAY_CORE_CPP})
target_include_directories(${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
	PUBLIC
		VegaEngine
		VegaGAS
		nlohmann_json::nlohmann_json
)

# The rest of the game, shared by the executable and every gameplay test. Some objects only register
# themselves or are reached indirectly, so consumers that need the full game link it whole-archive.
add_library(${LIGHT_YEARS_GAME_LIB_TARGET_NAME} STATIC ${VEGA_GAME_CPP} ${VEGA_GAME_HEADERS})
target_include_directories(${LIGHT_YEARS_GAME_LIB_TARGET_NAME}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)
# GalaxyRoute uses the pure-math visual/Astro.h and visual/GalaxyModel.h only; no SFML is pulled in.
target_link_libraries(${LIGHT_YEARS_GAME_LIB_TARGET_NAME}
	PUBLIC
		VegaEngine
		VegaGAS
		${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		LightYearsAstro
		nlohmann_json::nlohmann_json
)

add_library(${LIGHT_YEARS_GAME_APP_TARGET_NAME} STATIC ${VEGA_GAME_APP_CPP} ${VEGA_GAME_APP_HEADERS})

# Each executable provides its own GetApplication() hook for the engine entry point.
add_executable(${LIGHT_YEARS_GAME_TARGET_NAME} app/GameMain.cpp)
target_link_libraries(${LIGHT_YEARS_GAME_TARGET_NAME} PUBLIC ${LIGHT_YEARS_GAME_APP_TARGET_NAME})
# Sky shaders come from the engine (LightYearsVisual/assets/shaders holds only game overrides); packaged builds
# ship them next to the executable, Debug reads the engine source tree.
vega_stage_sky_shaders(${LIGHT_YEARS_GAME_TARGET_NAME})

if(MSVC)
	# Incremental relinks have left copied inline tag/attribute identities empty at startup.
	# Use a full link for the game, as for the gameplay test executables below.
	target_link_options(${LIGHT_YEARS_GAME_TARGET_NAME} PRIVATE /INCREMENTAL:NO)
endif()

target_include_directories(${LIGHT_YEARS_GAME_APP_TARGET_NAME} PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${LIGHT_YEARS_GAME_APP_TARGET_NAME}
	PUBLIC
		VegaEngine
		VegaGAS
		${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		LightYearsVisual
		VegaScript
		nlohmann_json::nlohmann_json
		$<LINK_LIBRARY:WHOLE_ARCHIVE,${LIGHT_YEARS_GAME_LIB_TARGET_NAME}>
)

# Debug reads the source tree for hot reload; packaged builds must never fall back to that tree.
target_compile_definitions(${LIGHT_YEARS_GAME_APP_TARGET_NAME} PRIVATE
	VEGA_VISUAL_ASSETS_DIR="$<IF:$<CONFIG:Debug>,${CMAKE_SOURCE_DIR}/LightYearsVisual/assets,visual>"
)

if(BUILD_TESTING)
	set(LIGHT_YEARS_E2E_ARTIFACT_DIR "${CMAKE_BINARY_DIR}/e2e-artifacts")
	file(MAKE_DIRECTORY "${LIGHT_YEARS_E2E_ARTIFACT_DIR}")
	set(VEGA_TEST_TARGETS)

	# vega_add_game_test(<target> SOURCES <files...> [LIBS <libs...>] [INCLUDES <dirs...>] [GAME_WHOLE] [GAME] [PROJECT_DIR])
	#   GAME_WHOLE   link the full game library whole-archive plus engine/ability/core/json (GAS-lite style tests)
	#   GAME         link the game library normally; only the members the test references are pulled in
	#   PROJECT_DIR  define LIGHT_YEARS_PROJECT_SOURCE_DIR
	function(vega_add_game_test target)
		cmake_parse_arguments(PARSE_ARGV 1 T "GAME_WHOLE;GAME;PROJECT_DIR" "" "SOURCES;LIBS;INCLUDES")
		add_executable(${target} ${T_SOURCES})
		target_include_directories(${target} PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${T_INCLUDES}
		)
		if(T_PROJECT_DIR)
			target_compile_definitions(${target} PRIVATE LIGHT_YEARS_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
		endif()
		set(link_libs ${T_LIBS})
		if(T_GAME_WHOLE)
			list(APPEND link_libs
				$<LINK_LIBRARY:WHOLE_ARCHIVE,${LIGHT_YEARS_GAME_LIB_TARGET_NAME}>
				VegaEngine
				VegaGAS
				${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
				nlohmann_json::nlohmann_json
			)
		elseif(T_GAME)
			list(APPEND link_libs ${LIGHT_YEARS_GAME_LIB_TARGET_NAME})
		endif()
		target_link_libraries(${target} PRIVATE ${link_libs})
		set(VEGA_TEST_TARGETS ${VEGA_TEST_TARGETS} ${target} PARENT_SCOPE)
	endfunction()

	# vega_add_game_test_case(<ctest name> <target> [ARGS <args...>] [TIMEOUT <s>] [NO_WORKDIR])
	function(vega_add_game_test_case name target)
		cmake_parse_arguments(PARSE_ARGV 2 C "NO_WORKDIR" "TIMEOUT" "ARGS")
		if(C_NO_WORKDIR)
			add_test(NAME ${name} COMMAND ${target} ${C_ARGS})
		else()
			add_test(NAME ${name} COMMAND ${target} ${C_ARGS} WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
		endif()
		if(C_TIMEOUT)
			set_tests_properties(${name} PROPERTIES TIMEOUT ${C_TIMEOUT})
		endif()
	endfunction()

	set(VEGA_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
	vega_add_game_test(LightYearsOwnerDeathPreventionTests GAME_WHOLE SOURCES ${VEGA_TESTS_DIR}/OwnerDeathPreventionTests.cpp)
	vega_add_game_test_case(LightYearsOwnerDeathPreventionTests LightYearsOwnerDeathPreventionTests)
	vega_add_game_test(LightYearsBackdropCapture PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/BackdropCapture.cpp LIBS ${LIGHT_YEARS_GAME_APP_TARGET_NAME})
	foreach(runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyRuntimeLibToTarget(${runtime} LightYearsBackdropCapture)
	endforeach()
	vega_add_game_test_case(LightYearsBackdropPlanetsE2E LightYearsBackdropCapture ARGS --two-body ${CMAKE_BINARY_DIR}/w10-backdrop-two-body TIMEOUT 90)
	vega_add_game_test_case(LightYearsBackdropPlanetsDataE2E LightYearsBackdropCapture ARGS --planets-data ${CMAKE_BINARY_DIR}/w10-planets-render-data TIMEOUT 240)
	vega_add_game_test_case(LightYearsBackdropEditorGeometryE2E LightYearsBackdropCapture ARGS --editor-geometry ${CMAKE_BINARY_DIR}/w10-skyhandles-geometry TIMEOUT 120)
	vega_add_game_test(LightYearsReflectionRegistryGoldenTests GAME
		SOURCES ${VEGA_TESTS_DIR}/ReflectionRegistryGoldenTests.cpp LIBS VegaScript)
	target_compile_definitions(LightYearsReflectionRegistryGoldenTests PRIVATE
		REFLECTION_GOLDEN_PATH="${VEGA_TESTS_DIR}/reflection-registry.golden.json")
	foreach(runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyRuntimeLibToTarget(${runtime} LightYearsReflectionRegistryGoldenTests)
	endforeach()
	vega_add_game_test_case(LightYearsReflectionRegistryGolden LightYearsReflectionRegistryGoldenTests)
	set(VEGA_E2E ${LIGHT_YEARS_E2E_ARTIFACT_DIR})
	add_test(NAME LightYearsContentBootstrapFailure COMMAND ${CMAKE_COMMAND}
		-DSOURCE_ROOT=${CMAKE_SOURCE_DIR} -DTEST_ROOT=${CMAKE_BINARY_DIR}/bootstrap-failure-test
		-DRUNTIME=$<TARGET_FILE:${LIGHT_YEARS_GAME_TARGET_NAME}>
		-P ${CMAKE_CURRENT_SOURCE_DIR}/tests/RunBootstrapFailure.cmake)
	set_tests_properties(LightYearsContentBootstrapFailure PROPERTIES
		WORKING_DIRECTORY $<TARGET_FILE_DIR:${LIGHT_YEARS_GAME_TARGET_NAME}> TIMEOUT 60)

	vega_add_game_test(LightYearsSceneSettingsTests GAME PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/SceneSettingsTests.cpp)
	vega_add_game_test_case(LightYearsSceneSettings LightYearsSceneSettingsTests)

	vega_add_game_test(LightYearsSceneSandboxTests GAME PROJECT_DIR
		SOURCES ${VEGA_TESTS_DIR}/SceneSandboxTests.cpp LIBS VegaScript)
	target_compile_definitions(LightYearsSceneSandboxTests PRIVATE SCENE_TEST_ARTIFACT_DIR="${LIGHT_YEARS_E2E_ARTIFACT_DIR}")
	vega_add_game_test_case(LightYearsSceneSandbox LightYearsSceneSandboxTests)

	vega_add_game_test(LightYearsLevelLayoutTests GAME PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LevelLayoutTests.cpp)
	vega_add_game_test_case(LightYearsLevelLayout LightYearsLevelLayoutTests)

	# Test-only copies use a reproducible RNG; shipping stage objects are untouched.
	set(campaign_stage_sources)
	foreach(stage IN ITEMS Vanguard TwinBlade Hexagon Chaos)
		set(copy "${CMAKE_CURRENT_BINARY_DIR}/campaign-trace/${stage}Stage.cpp")
		configure_file("src/enemy/${stage}Stage.cpp" "${copy}" COPYONLY)
		if(MSVC)
			set_source_files_properties("${copy}" PROPERTIES COMPILE_OPTIONS "/FI${CMAKE_CURRENT_SOURCE_DIR}/tests/CampaignStageRandom.h")
		else()
			set_source_files_properties("${copy}" PROPERTIES COMPILE_OPTIONS "-include;${CMAKE_CURRENT_SOURCE_DIR}/tests/CampaignStageRandom.h")
		endif()
		list(APPEND campaign_stage_sources "${copy}")
	endforeach()
	vega_add_game_test(LightYearsCampaignSceneTests GAME PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/CampaignSceneTests.cpp ${campaign_stage_sources})
	vega_add_game_test_case(LightYearsCampaignScene LightYearsCampaignSceneTests)

	vega_add_game_test(LightYearsShipPrefabTests GAME PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ShipPrefabTests.cpp)
	target_compile_definitions(LightYearsShipPrefabTests PRIVATE SHIP_PREFAB_ARTIFACT_DIR="${LIGHT_YEARS_E2E_ARTIFACT_DIR}/ship-prefab")
	vega_add_game_test_case(LightYearsShipPrefab LightYearsShipPrefabTests)

	vega_add_game_test(LightYearsEnemyPrefabTests GAME PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/EnemyPrefabTests.cpp LIBS VegaScript)
	target_compile_definitions(LightYearsEnemyPrefabTests PRIVATE ENEMY_PREFAB_ARTIFACT_DIR="${LIGHT_YEARS_E2E_ARTIFACT_DIR}/enemy-prefab")
	vega_add_game_test_case(LightYearsEnemyPrefab LightYearsEnemyPrefabTests)

	vega_add_game_test(LightYearsDummyPrefabTests GAME PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/DummyPrefabTests.cpp LIBS VegaScript)
	target_compile_definitions(LightYearsDummyPrefabTests PRIVATE DUMMY_PREFAB_ARTIFACT_DIR="${LIGHT_YEARS_E2E_ARTIFACT_DIR}/dummy-prefab")
	vega_add_game_test_case(LightYearsDummyPrefab LightYearsDummyPrefabTests)


	vega_add_game_test(LightYearsContentTests
		SOURCES ${VEGA_TESTS_DIR}/ContentLoaderTests.cpp
		INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src
		LIBS VegaEngine VegaGAS ${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		GAME PROJECT_DIR)
	vega_add_game_test_case(LightYearsContent LightYearsContentTests)
	vega_add_game_test(LightYearsContentReloadTests
		SOURCES ${VEGA_TESTS_DIR}/ContentReloadTests.cpp
		LIBS VegaEngine VegaGAS ${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		GAME PROJECT_DIR)
	target_compile_definitions(LightYearsContentReloadTests PRIVATE CONTENT_RELOAD_TEST_ROOT="${CMAKE_CURRENT_BINARY_DIR}/content-reload-test")
	vega_add_game_test_case(LightYearsContentReload LightYearsContentReloadTests)

	vega_add_game_test(LightYearsGasLiteTests GAME_WHOLE
		SOURCES
			${VEGA_TESTS_DIR}/GasLiteCoreTests.cpp
			${VEGA_TESTS_DIR}/EnemyCombatFoundationTests.cpp
			${VEGA_TESTS_DIR}/GameHUDPlayerRestartTests.cpp
			${VEGA_TESTS_DIR}/AbilityLoadoutTransactionTests.cpp
			${VEGA_TESTS_DIR}/PlayerShipLifetimeTests.cpp
			${VEGA_TESTS_DIR}/ChainLightningLifetimeTests.cpp)
	vega_add_game_test_case(LightYearsGasLiteMixedLoadout LightYearsGasLiteTests ARGS --mixed-loadout)
	vega_add_game_test_case(LightYearsAbilityLoadoutTransaction LightYearsGasLiteTests ARGS --loadout-transaction)
	vega_add_game_test_case(LightYearsPlayerShipLifetime LightYearsGasLiteTests ARGS --player-lifetime)
	vega_add_game_test_case(LightYearsChainLightningLifetime LightYearsGasLiteTests ARGS --chain-lightning-lifetime)
	vega_add_game_test_case(LightYearsGameHUDPlayerRestart LightYearsGasLiteTests ARGS --game-hud-player-restart NO_WORKDIR TIMEOUT 60)

	vega_add_game_test(LightYearsTemporaryOvercapTests
		SOURCES ${VEGA_TESTS_DIR}/TemporaryOvercapTests.cpp
		LIBS VegaEngine VegaGAS ${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		GAME)
	vega_add_game_test_case(LightYearsTemporaryOvercap LightYearsTemporaryOvercapTests)

	vega_add_game_test(LightYearsClosedCircuitInterceptionTests GAME_WHOLE SOURCES ${VEGA_TESTS_DIR}/ClosedCircuitInterceptionTests.cpp)
	vega_add_game_test_case(LightYearsClosedCircuitInterception LightYearsClosedCircuitInterceptionTests)

	vega_add_game_test(LightYearsNanoPlagueRuntimeTests GAME_WHOLE SOURCES ${VEGA_TESTS_DIR}/NanoPlagueRuntimeTests.cpp)
	vega_add_game_test_case(LightYearsNanoPlagueRuntime LightYearsNanoPlagueRuntimeTests)

	vega_add_game_test(LightYearsPortalTransferTests GAME_WHOLE SOURCES ${VEGA_TESTS_DIR}/PortalTransferRuntimeTests.cpp)
	vega_add_game_test_case(LightYearsPortalTransfer LightYearsPortalTransferTests)

	vega_add_game_test(LightYearsSecondBugHuntFixesTests GAME_WHOLE SOURCES ${VEGA_TESTS_DIR}/SecondBugHuntFixesTests.cpp)
	vega_add_game_test_case(LightYearsSecondBugHuntFixes LightYearsSecondBugHuntFixesTests)

	vega_add_game_test(LightYearsActorHitCooldownTests
		SOURCES ${VEGA_TESTS_DIR}/ActorHitCooldownTests.cpp
		LIBS VegaEngine)
	vega_add_game_test_case(LightYearsActorHitCooldown LightYearsActorHitCooldownTests NO_WORKDIR)

	vega_add_game_test(LightYearsTimedPresentationActorTests GAME SOURCES ${VEGA_TESTS_DIR}/TimedPresentationActorTests.cpp)
	vega_add_game_test_case(LightYearsTimedPresentationActor LightYearsTimedPresentationActorTests NO_WORKDIR)

	vega_add_game_test(LightYearsProjectileReflectionTests GAME_WHOLE SOURCES ${VEGA_TESTS_DIR}/ProjectileReflectionTests.cpp)
	vega_add_game_test_case(LightYearsProjectileReflection LightYearsProjectileReflectionTests)

	vega_add_game_test(LightYearsStormMarkTimingTests GAME_WHOLE SOURCES ${VEGA_TESTS_DIR}/StormMarkTimingTests.cpp)
	vega_add_game_test_case(LightYearsStormMarkTiming LightYearsStormMarkTimingTests)


	vega_add_game_test(LightYearsPlayerLevelTestProgressionE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/PlayerLevelTestProgressionE2E.cpp)
	vega_add_game_test_case(LightYearsPlayerLevelTestProgressionE2E LightYearsPlayerLevelTestProgressionE2ETests
		ARGS "${VEGA_E2E}/player-level-test-progression.json" TIMEOUT 60)




	vega_add_game_test(LightYearsUncommonModuleE2ETests GAME_WHOLE PROJECT_DIR SOURCES
		${VEGA_TESTS_DIR}/UncommonModuleE2E.cpp
		${VEGA_TESTS_DIR}/UncommonModuleE2E_Content.cpp
		${VEGA_TESTS_DIR}/UncommonModuleE2E_Fixture.cpp
		${VEGA_TESTS_DIR}/UncommonModuleE2E_Roller.cpp
		${VEGA_TESTS_DIR}/UncommonModuleE2E_Runtime.cpp)
	vega_add_game_test_case(LightYearsUncommonModuleE2E LightYearsUncommonModuleE2ETests
		ARGS "${VEGA_E2E}/uncommon-modules.json" TIMEOUT 90)
	vega_add_game_test(LightYearsControlTargetClassLoadE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ControlTargetClassLoadE2E.cpp)
	vega_add_game_test_case(LightYearsControlTargetClassLoadE2E LightYearsControlTargetClassLoadE2ETests ARGS "${VEGA_E2E}/control-target-class-load.json" TIMEOUT 60)
	vega_add_game_test(LightYearsAbilityChargeRestoreE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/AbilityChargeRestoreE2E.cpp)
	vega_add_game_test_case(LightYearsAbilityChargeRestoreE2E LightYearsAbilityChargeRestoreE2ETests ARGS "${VEGA_E2E}/ability-charge-restore.json" TIMEOUT 120)
	vega_add_game_test(LightYearsAbilityMultiChargeE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/AbilityMultiChargeE2E.cpp)
	vega_add_game_test_case(LightYearsAbilityMultiChargeE2E LightYearsAbilityMultiChargeE2ETests ARGS "${VEGA_E2E}/ability-multi-charge.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareDamageConditionsE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareDamageConditionsE2E.cpp)
	vega_add_game_test_case(LightYearsRareDamageConditionsE2E LightYearsRareDamageConditionsE2ETests ARGS "${VEGA_E2E}/rare-damage-conditions.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareShieldRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareShieldRulesE2E.cpp)
	vega_add_game_test_case(LightYearsRareShieldRulesE2E LightYearsRareShieldRulesE2ETests ARGS "${VEGA_E2E}/rare-shield-rules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsModuleConflictE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleConflictE2E.cpp)
	vega_add_game_test_case(LightYearsModuleConflictE2E LightYearsModuleConflictE2ETests ARGS "${VEGA_E2E}/module-conflict.json" TIMEOUT 60)
	vega_add_game_test(LightYearsRareSideFeaturesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareSideFeaturesE2E.cpp)
	vega_add_game_test_case(LightYearsRareSideFeaturesE2E LightYearsRareSideFeaturesE2ETests ARGS "${VEGA_E2E}/rare-side-features.json" TIMEOUT 180)
	vega_add_game_test(LightYearsRareStaticDamageRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareStaticDamageRulesE2E.cpp)
	vega_add_game_test_case(LightYearsRareStaticDamageRulesE2E LightYearsRareStaticDamageRulesE2ETests ARGS "${VEGA_E2E}/rare-static-damage-rules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareMovementRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareMovementRulesE2E.cpp)
	vega_add_game_test_case(LightYearsRareMovementRulesE2E LightYearsRareMovementRulesE2ETests ARGS "${VEGA_E2E}/rare-movement-rules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareRunCounterE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareRunCounterE2E.cpp)
	vega_add_game_test_case(LightYearsRareRunCounterE2E LightYearsRareRunCounterE2ETests ARGS "${VEGA_E2E}/rare-run-counter.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareTrophyCoreMechanicsE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareTrophyCoreMechanicsE2E.cpp)
	vega_add_game_test_case(LightYearsRareTrophyCoreMechanicsE2E LightYearsRareTrophyCoreMechanicsE2ETests ARGS "${VEGA_E2E}/rare-trophy-core-mechanics.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareStatusRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareStatusRulesE2E.cpp)
	vega_add_game_test_case(LightYearsRareStatusRulesE2E LightYearsRareStatusRulesE2ETests ARGS "${VEGA_E2E}/rare-status-rules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareOwnerBurstE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareOwnerBurstE2E.cpp)
	vega_add_game_test_case(LightYearsRareOwnerBurstE2E LightYearsRareOwnerBurstE2ETests ARGS "${VEGA_E2E}/rare-owner-burst.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareDyingStarMechanicsE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareDyingStarMechanicsE2E.cpp)
	vega_add_game_test_case(LightYearsRareDyingStarMechanicsE2E LightYearsRareDyingStarMechanicsE2ETests ARGS "${VEGA_E2E}/rare-dying-star-mechanics.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareRatioRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareRatioRulesE2E.cpp)
	vega_add_game_test_case(LightYearsRareRatioRulesE2E LightYearsRareRatioRulesE2ETests ARGS "${VEGA_E2E}/rare-ratio-rules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareSustainedSpeedE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareSustainedSpeedE2E.cpp)
	vega_add_game_test_case(LightYearsRareSustainedSpeedE2E LightYearsRareSustainedSpeedE2ETests ARGS "${VEGA_E2E}/rare-sustained-speed.json" TIMEOUT 120)
	vega_add_game_test(LightYearsRareStatusModifiersE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareStatusModifiersE2E.cpp)
	vega_add_game_test_case(LightYearsRareStatusModifiersE2E LightYearsRareStatusModifiersE2ETests ARGS "${VEGA_E2E}/rare-status-modifiers.json" TIMEOUT 240)
	vega_add_game_test(LightYearsRareSlipstreamControlE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareSlipstreamControlE2E.cpp)
	vega_add_game_test_case(LightYearsRareSlipstreamControlE2E LightYearsRareSlipstreamControlE2ETests ARGS "${VEGA_E2E}/rare-slipstream-control.json" TIMEOUT 240)
	vega_add_game_test(LightYearsRareShieldBatteryRuptureE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/RareShieldBatteryRuptureE2E.cpp)
	vega_add_game_test_case(LightYearsRareShieldBatteryRuptureE2E LightYearsRareShieldBatteryRuptureE2ETests ARGS "${VEGA_E2E}/rare-shield-battery-rupture.json" TIMEOUT 240)
	vega_add_game_test(LightYearsModuleSidePoolsE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleSidePoolsE2E.cpp)
	vega_add_game_test(LightYearsModuleRefinementE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleRefinementE2E.cpp)
	vega_add_game_test(LightYearsModulePlanTransitionE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModulePlanTransitionE2E.cpp)
	vega_add_game_test(LightYearsModuleGoldHullE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleGoldHullE2E.cpp)
	vega_add_game_test_case(LightYearsModuleGoldHullE2E LightYearsModuleGoldHullE2ETests ARGS "${VEGA_E2E}/ModuleGoldHull-2026-10-14.json" TIMEOUT 120)
	vega_add_game_test(LightYearsLegendaryModulesE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LegendaryModulesE2E.cpp)
	vega_add_game_test_case(LightYearsLegendaryModulesE2E LightYearsLegendaryModulesE2E ARGS "${VEGA_E2E}/legendary-modules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsLegendaryAcquisitionE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LegendaryAcquisitionE2E.cpp)
	vega_add_game_test_case(LightYearsLegendaryAcquisitionE2E LightYearsLegendaryAcquisitionE2E ARGS "${VEGA_E2E}/legendary-acquisition.json" TIMEOUT 120)
	vega_add_game_test(LightYearsLegendarySimpleModulesE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LegendarySimpleModulesE2E.cpp)
	vega_add_game_test_case(LightYearsLegendarySimpleModulesE2E LightYearsLegendarySimpleModulesE2E ARGS "${VEGA_E2E}/legendary-simple-modules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsLegendaryWeaponModulesE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LegendaryWeaponModulesE2E.cpp)
	vega_add_game_test_case(LightYearsLegendaryWeaponModulesE2E LightYearsLegendaryWeaponModulesE2E ARGS "${VEGA_E2E}/legendary-weapon-modules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsLegendaryPowerModulesE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LegendaryPowerModulesE2E.cpp)
	vega_add_game_test_case(LightYearsLegendaryPowerModulesE2E LightYearsLegendaryPowerModulesE2E ARGS "${VEGA_E2E}/legendary-power-modules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsLegendaryTwinRackE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LegendaryTwinRackE2E.cpp)
	vega_add_game_test_case(LightYearsLegendaryTwinRackE2E LightYearsLegendaryTwinRackE2E ARGS "${VEGA_E2E}/legendary-twin-rack.json" TIMEOUT 120)
	vega_add_game_test(LightYearsLegendaryMovementModulesE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LegendaryMovementModulesE2E.cpp)
	vega_add_game_test_case(LightYearsLegendaryMovementModulesE2E LightYearsLegendaryMovementModulesE2E ARGS "${VEGA_E2E}/legendary-movement-modules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsLegendaryReactionCoreE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LegendaryReactionCoreE2E.cpp)
	vega_add_game_test_case(LightYearsLegendaryReactionCoreE2E LightYearsLegendaryReactionCoreE2E ARGS "${VEGA_E2E}/legendary-reaction-core.json" TIMEOUT 120)
	vega_add_game_test(LightYearsLegendaryTakeoverModulesE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/LegendaryTakeoverModulesE2E.cpp)
	vega_add_game_test_case(LightYearsLegendaryTakeoverModulesE2E LightYearsLegendaryTakeoverModulesE2E ARGS "${VEGA_E2E}/legendary-takeover-modules.json" TIMEOUT 180)
	vega_add_game_test(LightYearsEpicStatModulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/EpicStatModulesE2E.cpp)
	vega_add_game_test_case(LightYearsEpicStatModulesE2E LightYearsEpicStatModulesE2ETests ARGS "${VEGA_E2E}/module-epic-stat-2026-10-24.json" TIMEOUT 120)
	vega_add_game_test(LightYearsModulePolarShieldE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModulePolarShieldE2E.cpp)
	vega_add_game_test_case(LightYearsModulePolarShieldE2E LightYearsModulePolarShieldE2ETests ARGS "${VEGA_E2E}/ModulePolarShield-2026-10-14.json" TIMEOUT 120)
	vega_add_game_test(LightYearsEpicOffenseModulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/EpicOffenseModulesE2E.cpp)
	vega_add_game_test_case(LightYearsEpicOffenseModulesE2E LightYearsEpicOffenseModulesE2ETests ARGS "${VEGA_E2E}/epic-offense-modules-2026-10-24.json" TIMEOUT 120)
	vega_add_game_test(LightYearsModuleReleaseValveE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleReleaseValveE2E.cpp)
	vega_add_game_test(LightYearsModuleLevelGrowthE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleLevelGrowthE2E.cpp)
	vega_add_game_test_case(LightYearsModuleLevelGrowthE2E LightYearsModuleLevelGrowthE2ETests ARGS "${VEGA_E2E}/module-level-growth-2026-10-24.json" TIMEOUT 120)
	vega_add_game_test_case(LightYearsModuleReleaseValveE2E LightYearsModuleReleaseValveE2ETests ARGS "${VEGA_E2E}/module-release-valve-2026-10-14.json" TIMEOUT 120)
	vega_add_game_test(LightYearsModuleSpeedLanceE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleSpeedLanceE2E.cpp)
	vega_add_game_test_case(LightYearsModuleSpeedLanceE2E LightYearsModuleSpeedLanceE2ETests ARGS "${VEGA_E2E}/module-speed-lance-2026-10-13.json" TIMEOUT 120)
	vega_add_game_test(LightYearsEpicDefenseModulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/EpicDefenseModulesE2E.cpp)
	vega_add_game_test_case(LightYearsEpicDefenseModulesE2E LightYearsEpicDefenseModulesE2ETests ARGS "${VEGA_E2E}/epic-defense-modules-2026-10-24.json" TIMEOUT 120)
	vega_add_game_test(LightYearsModuleTemperedHullE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleTemperedHullE2E.cpp)
	vega_add_game_test_case(LightYearsModuleTemperedHullE2E LightYearsModuleTemperedHullE2ETests ARGS "${VEGA_E2E}/module-tempered-hull-2026-10-13.json" TIMEOUT 120)
	vega_add_game_test(LightYearsSimpleNewModules20261025E2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/SimpleNewModules20261025E2E.cpp)
	vega_add_game_test_case(LightYearsSimpleNewModules20261025E2E LightYearsSimpleNewModules20261025E2ETests ARGS "${VEGA_E2E}/simple-new-modules-2026-10-25.json" TIMEOUT 120)
	vega_add_game_test(LightYearsEpicNewModules20261025E2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/EpicNewModules20261025E2E.cpp)
	vega_add_game_test_case(LightYearsEpicNewModules20261025E2E LightYearsEpicNewModules20261025E2ETests ARGS "${VEGA_E2E}/epic-new-modules-2026-10-25.json" TIMEOUT 120)
	vega_add_game_test(LightYearsEpicPrimaryModulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/EpicPrimaryModulesE2E.cpp)
	vega_add_game_test_case(LightYearsEpicPrimaryModulesE2E LightYearsEpicPrimaryModulesE2ETests ARGS "${VEGA_E2E}/epic-primary-modules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsModuleDefenseLayerE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleDefenseLayerE2E.cpp)
	vega_add_game_test_case(LightYearsModuleDefenseLayerE2E LightYearsModuleDefenseLayerE2ETests ARGS "${VEGA_E2E}/module-defense-layer-2026-10-13.json" TIMEOUT 120)
	vega_add_game_test(LightYearsEpicUtilityMovementModulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/EpicUtilityMovementModulesE2E.cpp)
	vega_add_game_test(LightYearsUniqueModulesE2E GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/UniqueModulesE2E.cpp)
	vega_add_game_test_case(LightYearsUniqueModulesE2E LightYearsUniqueModulesE2E ARGS "${VEGA_E2E}/unique-modules.json" TIMEOUT 120)
	vega_add_game_test(LightYearsAbilityModuleUltimateProtocolE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/AbilityModuleUltimateProtocolE2E.cpp)
	vega_add_game_test_case(LightYearsAbilityModuleUltimateProtocolE2E LightYearsAbilityModuleUltimateProtocolE2ETests ARGS "${VEGA_E2E}/ability-module-ultimate-protocol.json" TIMEOUT 120)
	vega_add_game_test_case(LightYearsEpicUtilityMovementModulesE2E LightYearsEpicUtilityMovementModulesE2ETests ARGS "${VEGA_E2E}/epic-utility-movement-modules-2026-10-24.json" TIMEOUT 120)
	vega_add_game_test_case(LightYearsModulePlanTransitionE2E LightYearsModulePlanTransitionE2ETests ARGS "${VEGA_E2E}/module-plan-transition-2026-10-13.json" TIMEOUT 120)
	vega_add_game_test_case(LightYearsModuleSidePoolsE2E LightYearsModuleSidePoolsE2ETests ARGS "${VEGA_E2E}/module-side-pools-task-c.json" TIMEOUT 120)
	vega_add_game_test_case(LightYearsModuleRefinementE2E LightYearsModuleRefinementE2ETests ARGS "${VEGA_E2E}/module-refinement-2026-10-13.json" TIMEOUT 120)


	vega_add_game_test(LightYearsVitalsHUDTests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/VitalsHUDTests.cpp)
	vega_add_game_test_case(LightYearsVitalsHUD LightYearsVitalsHUDTests ARGS "${VEGA_E2E}/vitals-hud.json" TIMEOUT 60)

	vega_add_game_test(LightYearsModuleScreenTests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/ModuleScreenTests.cpp)
	vega_add_game_test_case(LightYearsModuleScreen LightYearsModuleScreenTests ARGS "${VEGA_E2E}/module-screen.json" TIMEOUT 60)

	vega_add_game_test(LightYearsHUDMigrationTests GAME_WHOLE PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/HUDMigrationTests.cpp)
	foreach(hud_group IN ITEMS Warning Notification Encounter Ability Menus)
		string(TOLOWER ${hud_group} hud_arg)
		vega_add_game_test_case(LightYearsHUDMigration${hud_group} LightYearsHUDMigrationTests
			ARGS --${hud_arg} "${VEGA_E2E}/hud-migration-${hud_arg}.json" TIMEOUT 60)
	endforeach()

	vega_add_game_test(LightYearsContinuousBeamWallE2ETests GAME_WHOLE PROJECT_DIR
		SOURCES
			${VEGA_TESTS_DIR}/ContinuousBeamWallE2E.cpp
			${VEGA_TESTS_DIR}/AbilityContentRegistrationE2E.cpp
			${VEGA_TESTS_DIR}/AbilityLoaderPublicLoadE2E.cpp
			${VEGA_TESTS_DIR}/TimerManagerSceneE2E.cpp
			${VEGA_TESTS_DIR}/GameHUDDamageEventE2E.cpp
			${VEGA_TESTS_DIR}/AuditFixesE2E.cpp
			${VEGA_TESTS_DIR}/AbilityBalanceSecondThreeE2E.cpp
			${VEGA_TESTS_DIR}/AbilityBalanceFirstThreeE2E.cpp
			${VEGA_TESTS_DIR}/AbilityBalanceThirdThreeE2E.cpp)
	vega_add_game_test_case(LightYearsAbilityContentRegistrationE2E LightYearsContinuousBeamWallE2ETests
		ARGS --ability-content-registration "${VEGA_E2E}/ability-content-registration.json")
	vega_add_game_test_case(LightYearsAbilityLoaderPublicLoadE2E LightYearsContinuousBeamWallE2ETests
		ARGS --ability-loader-public-load "${VEGA_E2E}/ability-loader-public-load.json")
	vega_add_game_test_case(LightYearsTimerManagerSceneE2E LightYearsContinuousBeamWallE2ETests
		ARGS --timer-manager-scene "${VEGA_E2E}/timer-manager-scene.json" TIMEOUT 60)
	vega_add_game_test_case(LightYearsGameHUDDamageEventE2E LightYearsContinuousBeamWallE2ETests
		ARGS --game-hud-damage-event "${VEGA_E2E}/game-hud-damage-event.json" TIMEOUT 60)
	foreach(BALANCE_GROUP IN ITEMS first second third)
		vega_add_game_test_case(LightYearsAbilityBalance-${BALANCE_GROUP}-E2E LightYearsContinuousBeamWallE2ETests
			ARGS --ability-balance-${BALANCE_GROUP}-three "${VEGA_E2E}/ability-balance-${BALANCE_GROUP}.json" TIMEOUT 60)
	endforeach()

	option(
		LIGHT_YEARS_BUILD_SPECIALIZED_TESTS
		"Build small standalone specialist test executables"
		OFF
	)
	if(LIGHT_YEARS_BUILD_SPECIALIZED_TESTS)
		vega_add_game_test(LightYearsAutoTargetingTests
			SOURCES ${VEGA_TESTS_DIR}/AutoTargetingTests.cpp
			LIBS VegaEngine VegaGAS
			GAME)
		vega_add_game_test_case(LightYearsAutoTargeting LightYearsAutoTargetingTests NO_WORKDIR)
	endif()

	# Campaign route data (galaxy_route.json); the test links GAME, which pulls LightYearsVisual via GameLib.
	vega_add_game_test(LightYearsGalaxyRouteTests GAME PROJECT_DIR SOURCES ${VEGA_TESTS_DIR}/GalaxyRouteTests.cpp)
	vega_add_game_test_case(LightYearsGalaxyRoute LightYearsGalaxyRouteTests NO_WORKDIR)

	# Test executables are rebuilt from scratch often; the incremental-link
	# state (.ilk, ~300 MB each) and full link PDBs (up to ~1.9 GB each) are
	# pure disk cost for them. FASTLINK keeps debugging working by referencing
	# the object files' debug info instead of copying it.
	if(MSVC)
		foreach(light_years_test_target IN LISTS VEGA_TEST_TARGETS)
			target_link_options(${light_years_test_target} PRIVATE /INCREMENTAL:NO /DEBUG:FASTLINK)
		endforeach()
	endif()
endif()

if(WIN32)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		set_target_properties(${LIGHT_YEARS_GAME_TARGET_NAME} PROPERTIES
			WIN32_EXECUTABLE TRUE
		)
	endif()

	target_link_libraries(${LIGHT_YEARS_GAME_TARGET_NAME} PRIVATE sfml-main)
endif()

CopyRuntimeLibToTarget(sfml-graphics ${LIGHT_YEARS_GAME_TARGET_NAME})
CopyRuntimeLibToTarget(sfml-window ${LIGHT_YEARS_GAME_TARGET_NAME})
CopyRuntimeLibToTarget(sfml-system ${LIGHT_YEARS_GAME_TARGET_NAME})
CopyRuntimeLibToTarget(sfml-audio ${LIGHT_YEARS_GAME_TARGET_NAME})
CopyRuntimeLibToTarget(box2d ${LIGHT_YEARS_GAME_TARGET_NAME})

set(RESOURCE_FOLDER_NAME "assets")
set(RESOURCE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_FOLDER_NAME}")
set(LIGHT_YEARS_GAME_ASSET_TARGET "${LIGHT_YEARS_GAME_TARGET_NAME}Assets")

# Content JSON and presentation assets are runtime inputs, not C++ link
# dependencies. A POST_BUILD command is skipped when only an asset changes,
# leaving the executable to load stale copied content. This target runs for
# every game build and keeps the output assets synchronized in one pass.
add_custom_target(${LIGHT_YEARS_GAME_ASSET_TARGET} ALL
	COMMAND
	${CMAKE_COMMAND} -E copy_directory
	${RESOURCE_SRC_DIR}
	$<TARGET_FILE_DIR:${LIGHT_YEARS_GAME_TARGET_NAME}>/${RESOURCE_FOLDER_NAME}
	COMMENT "Synchronizing game runtime assets..."
)
add_dependencies(${LIGHT_YEARS_GAME_TARGET_NAME} ${LIGHT_YEARS_GAME_ASSET_TARGET})

if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Release")
	# A distributable must also work without Visual Studio's C++ runtime installed.
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
	include(InstallRequiredSystemLibraries)
	if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
		add_custom_command(TARGET ${LIGHT_YEARS_GAME_ASSET_TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
				$<TARGET_FILE_DIR:${LIGHT_YEARS_GAME_TARGET_NAME}>
			COMMAND_EXPAND_LISTS
		)
	endif()
endif()

# Non-Debug builds package the visual pipeline assets next to the executable (<exe dir>/visual); Debug builds keep
# reading the source tree (VEGA_VISUAL_ASSETS_DIR) so shader/palette/pipeline.json edits hot-reload.
add_custom_command(TARGET ${LIGHT_YEARS_GAME_ASSET_TARGET} POST_BUILD
	COMMAND
	${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,true,copy_directory>
	${CMAKE_SOURCE_DIR}/LightYearsVisual/assets
	$<TARGET_FILE_DIR:${LIGHT_YEARS_GAME_TARGET_NAME}>/visual
)

set(VEGA_GAME_CONFIG_TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
	# Release roots must come from the portable launch folder; Debug keeps source-tree hot reload.
	set(VEGA_GAME_CONFIG_TEMPLATE "${CMAKE_SOURCE_DIR}/tools/vega-release-config.h.in")
endif()
configure_file(
	"${VEGA_GAME_CONFIG_TEMPLATE}"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h" ESCAPE_QUOTES
)
# Keep runtime-root changes visible to Ninja even when an older MSVC dependency database missed Config.h.
set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/gameFramework/LightYearsGameModule.cpp" APPEND PROPERTY
	OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/config.h")

# The checked-in include/Config.h otherwise shadows this configuration-specific generated header.
target_include_directories(${LIGHT_YEARS_GAME_APP_TARGET_NAME} BEFORE PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
)

if(VEGA_BUILD_EDITOR)
    include(editor.cmake)
endif()
