set(LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME LightYearsGameplayCore)
set(LIGHT_YEARS_GAME_LIB_TARGET_NAME LightYearsGameLib)
set(LIGHT_YEARS_GAME_APP_TARGET_NAME LightYearsGameApp)

# Sources are discovered by directory; adding a .cpp/.h under src/ or include/ needs no CMake edit.
# Every .cpp under src/ is compiled (there are no orphan sources). The sources are split into three
# groups, each compiled exactly once:
#   LightYearsGameplayCore - reusable gameplay services that depend only on SpaceAbilitySystem
#   LightYearsGameLib      - the rest of the game that tests can link without a window or visual pipeline
#   LightYearsGameApp      - reusable game application (gameFramework/) and LightYearsVisual presenters
file(GLOB_RECURSE LY_GAME_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE LY_GAME_HEADERS CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

# Shared application setup and presenters let other executables reuse the game's visual pipeline.
set(LY_GAME_APP_DIR_REGEX "/(src|include)/(gameFramework|presentation/visual)/")
set(LY_GAME_APP_CPP ${LY_GAME_CPP})
list(FILTER LY_GAME_APP_CPP INCLUDE REGEX "${LY_GAME_APP_DIR_REGEX}")
set(LY_GAME_APP_HEADERS ${LY_GAME_HEADERS})
list(FILTER LY_GAME_APP_HEADERS INCLUDE REGEX "${LY_GAME_APP_DIR_REGEX}")
list(FILTER LY_GAME_CPP EXCLUDE REGEX "${LY_GAME_APP_DIR_REGEX}")
list(FILTER LY_GAME_HEADERS EXCLUDE REGEX "${LY_GAME_APP_DIR_REGEX}")

# Gameplay core: game-specific reusable services. LightYearsGameplayCore deliberately depends on
# SpaceAbilitySystem instead of extending it with LightYears contracts, content, actors, or
# presentation rules. The files
# do not share a directory, so they are selected by file name.
set(LY_GAMEPLAY_CORE_FILES
	MovementPolicyController NumericSettingContractRegistry GameAbilityContentRegistration
	EnergySpearContentRegistration AbilityLifecycleDispatcher AbilityUseHistory FocusActionLocks
	TemporalRateModifierLedger AbilityActionValidator GameAbilityDefinitionValidator
	GameplayEffectDefinitionValidator GameContentBootstrap ShipSchematicValidator ModuleLoadout
	ModulePlacementRules ModuleResolver ModuleLoader ModuleContentCatalog ModuleSideFeatureLoader ModuleSideFeatureCatalog ModuleRoller GameAbilitySettingContracts
	LightYearsEffectBehaviorRuntime
)
string(JOIN "|" LY_GAMEPLAY_CORE_REGEX_BODY ${LY_GAMEPLAY_CORE_FILES})
set(LY_GAMEPLAY_CORE_REGEX "/src/.*/(${LY_GAMEPLAY_CORE_REGEX_BODY})\\.cpp$")
set(LY_GAMEPLAY_CORE_CPP ${LY_GAME_CPP})
list(FILTER LY_GAMEPLAY_CORE_CPP INCLUDE REGEX "${LY_GAMEPLAY_CORE_REGEX}")
list(FILTER LY_GAME_CPP EXCLUDE REGEX "${LY_GAMEPLAY_CORE_REGEX}")
list(LENGTH LY_GAMEPLAY_CORE_FILES LY_GAMEPLAY_CORE_FILE_COUNT)
list(LENGTH LY_GAMEPLAY_CORE_CPP LY_GAMEPLAY_CORE_CPP_COUNT)
if(NOT LY_GAMEPLAY_CORE_CPP_COUNT EQUAL LY_GAMEPLAY_CORE_FILE_COUNT)
	message(FATAL_ERROR "LY_GAMEPLAY_CORE_FILES lists ${LY_GAMEPLAY_CORE_FILE_COUNT} files but ${LY_GAMEPLAY_CORE_CPP_COUNT} were found under src/ (renamed, moved, or duplicated?): ${LY_GAMEPLAY_CORE_CPP}")
endif()

add_library(${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME} STATIC ${LY_GAMEPLAY_CORE_CPP})
target_include_directories(${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
	PUBLIC
		${LIGHT_YEARS_ENGINE_TARGET_NAME}
		${SPACE_ABILITY_SYSTEM_TARGET_NAME}
		nlohmann_json::nlohmann_json
)

# The rest of the game, shared by the executable and every gameplay test. Some objects only register
# themselves or are reached indirectly, so consumers that need the full game link it whole-archive.
add_library(${LIGHT_YEARS_GAME_LIB_TARGET_NAME} STATIC ${LY_GAME_CPP} ${LY_GAME_HEADERS})
target_include_directories(${LIGHT_YEARS_GAME_LIB_TARGET_NAME}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/LightYearsEngine/include
)
# GalaxyRoute uses the pure-math visual/Astro.h and visual/GalaxyModel.h only; no SFML is pulled in.
target_link_libraries(${LIGHT_YEARS_GAME_LIB_TARGET_NAME}
	PUBLIC
		${LIGHT_YEARS_ENGINE_TARGET_NAME}
		${SPACE_ABILITY_SYSTEM_TARGET_NAME}
		${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		LightYearsAstro
		nlohmann_json::nlohmann_json
)

add_library(${LIGHT_YEARS_GAME_APP_TARGET_NAME} STATIC ${LY_GAME_APP_CPP} ${LY_GAME_APP_HEADERS})

# Each executable provides its own GetApplication() hook for the engine entry point.
add_executable(${LIGHT_YEARS_GAME_TARGET_NAME} app/GameMain.cpp)
target_link_libraries(${LIGHT_YEARS_GAME_TARGET_NAME} PUBLIC ${LIGHT_YEARS_GAME_APP_TARGET_NAME})

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
		${LIGHT_YEARS_ENGINE_TARGET_NAME}
		${SPACE_ABILITY_SYSTEM_TARGET_NAME}
		${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		LightYearsVisual
		VegaScript
		nlohmann_json::nlohmann_json
		$<LINK_LIBRARY:WHOLE_ARCHIVE,${LIGHT_YEARS_GAME_LIB_TARGET_NAME}>
)

# First integration pass: the game reads the visual assets from the source tree so edits hot-reload (F9 forces it).
target_compile_definitions(${LIGHT_YEARS_GAME_APP_TARGET_NAME} PRIVATE
	LY_VISUAL_ASSETS_DIR="${CMAKE_SOURCE_DIR}/LightYearsVisual/assets"
)

if(BUILD_TESTING)
	set(LIGHT_YEARS_E2E_ARTIFACT_DIR "${CMAKE_BINARY_DIR}/e2e-artifacts")
	file(MAKE_DIRECTORY "${LIGHT_YEARS_E2E_ARTIFACT_DIR}")
	set(LY_TEST_TARGETS)

	# ly_add_game_test(<target> SOURCES <files...> [LIBS <libs...>] [INCLUDES <dirs...>] [GAME_WHOLE] [GAME] [PROJECT_DIR])
	#   GAME_WHOLE   link the full game library whole-archive plus engine/ability/core/json (GAS-lite style tests)
	#   GAME         link the game library normally; only the members the test references are pulled in
	#   PROJECT_DIR  define LIGHT_YEARS_PROJECT_SOURCE_DIR
	function(ly_add_game_test target)
		cmake_parse_arguments(PARSE_ARGV 1 T "GAME_WHOLE;GAME;PROJECT_DIR" "" "SOURCES;LIBS;INCLUDES")
		add_executable(${target} ${T_SOURCES})
		target_include_directories(${target} PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/include
			${PROJECT_SOURCE_DIR}/LightYearsEngine/include
			${T_INCLUDES}
		)
		if(T_PROJECT_DIR)
			target_compile_definitions(${target} PRIVATE LIGHT_YEARS_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
		endif()
		set(link_libs ${T_LIBS})
		if(T_GAME_WHOLE)
			list(APPEND link_libs
				$<LINK_LIBRARY:WHOLE_ARCHIVE,${LIGHT_YEARS_GAME_LIB_TARGET_NAME}>
				${LIGHT_YEARS_ENGINE_TARGET_NAME}
				${SPACE_ABILITY_SYSTEM_TARGET_NAME}
				${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
				nlohmann_json::nlohmann_json
			)
		elseif(T_GAME)
			list(APPEND link_libs ${LIGHT_YEARS_GAME_LIB_TARGET_NAME})
		endif()
		target_link_libraries(${target} PRIVATE ${link_libs})
		set(LY_TEST_TARGETS ${LY_TEST_TARGETS} ${target} PARENT_SCOPE)
	endfunction()

	# ly_add_game_test_case(<ctest name> <target> [ARGS <args...>] [TIMEOUT <s>] [NO_WORKDIR])
	function(ly_add_game_test_case name target)
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

	set(LY_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
	set(LY_E2E ${LIGHT_YEARS_E2E_ARTIFACT_DIR})

	ly_add_game_test(LightYearsContentTests
		SOURCES ${LY_TESTS_DIR}/ContentLoaderTests.cpp
		INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src
		LIBS ${LIGHT_YEARS_ENGINE_TARGET_NAME} ${SPACE_ABILITY_SYSTEM_TARGET_NAME} ${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		GAME PROJECT_DIR)
	ly_add_game_test_case(LightYearsContent LightYearsContentTests)

	ly_add_game_test(LightYearsGasLiteTests GAME_WHOLE
		SOURCES
			${LY_TESTS_DIR}/GasLiteCoreTests.cpp
			${LY_TESTS_DIR}/EnemyCombatFoundationTests.cpp
			${LY_TESTS_DIR}/GameHUDPlayerRestartTests.cpp
			${LY_TESTS_DIR}/AbilityLoadoutTransactionTests.cpp
			${LY_TESTS_DIR}/PlayerShipLifetimeTests.cpp
			${LY_TESTS_DIR}/ChainLightningLifetimeTests.cpp)
	ly_add_game_test_case(LightYearsGasLiteMixedLoadout LightYearsGasLiteTests ARGS --mixed-loadout)
	ly_add_game_test_case(LightYearsAbilityLoadoutTransaction LightYearsGasLiteTests ARGS --loadout-transaction)
	ly_add_game_test_case(LightYearsPlayerShipLifetime LightYearsGasLiteTests ARGS --player-lifetime)
	ly_add_game_test_case(LightYearsChainLightningLifetime LightYearsGasLiteTests ARGS --chain-lightning-lifetime)
	ly_add_game_test_case(LightYearsGameHUDPlayerRestart LightYearsGasLiteTests ARGS --game-hud-player-restart NO_WORKDIR TIMEOUT 60)

	ly_add_game_test(LightYearsTemporaryOvercapTests
		SOURCES ${LY_TESTS_DIR}/TemporaryOvercapTests.cpp
		LIBS ${LIGHT_YEARS_ENGINE_TARGET_NAME} ${SPACE_ABILITY_SYSTEM_TARGET_NAME} ${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		GAME)
	ly_add_game_test_case(LightYearsTemporaryOvercap LightYearsTemporaryOvercapTests)

	ly_add_game_test(LightYearsClosedCircuitInterceptionTests GAME_WHOLE SOURCES ${LY_TESTS_DIR}/ClosedCircuitInterceptionTests.cpp)
	ly_add_game_test_case(LightYearsClosedCircuitInterception LightYearsClosedCircuitInterceptionTests)

	ly_add_game_test(LightYearsNanoPlagueRuntimeTests GAME_WHOLE SOURCES ${LY_TESTS_DIR}/NanoPlagueRuntimeTests.cpp)
	ly_add_game_test_case(LightYearsNanoPlagueRuntime LightYearsNanoPlagueRuntimeTests)

	ly_add_game_test(LightYearsPortalTransferTests GAME_WHOLE SOURCES ${LY_TESTS_DIR}/PortalTransferRuntimeTests.cpp)
	ly_add_game_test_case(LightYearsPortalTransfer LightYearsPortalTransferTests)

	ly_add_game_test(LightYearsActorHitCooldownTests
		SOURCES ${LY_TESTS_DIR}/ActorHitCooldownTests.cpp
		LIBS ${LIGHT_YEARS_ENGINE_TARGET_NAME})
	ly_add_game_test_case(LightYearsActorHitCooldown LightYearsActorHitCooldownTests NO_WORKDIR)

	ly_add_game_test(LightYearsProjectileReflectionTests GAME_WHOLE SOURCES ${LY_TESTS_DIR}/ProjectileReflectionTests.cpp)
	ly_add_game_test_case(LightYearsProjectileReflection LightYearsProjectileReflectionTests)

	ly_add_game_test(LightYearsStormMarkTimingTests GAME_WHOLE SOURCES ${LY_TESTS_DIR}/StormMarkTimingTests.cpp)
	ly_add_game_test_case(LightYearsStormMarkTiming LightYearsStormMarkTimingTests)

	ly_add_game_test(LightYearsPrimaryWeaponTests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/PrimaryWeaponMagazineTests.cpp)
	ly_add_game_test_case(LightYearsPrimaryWeapon LightYearsPrimaryWeaponTests)

	ly_add_game_test(LightYearsPlayerLevelTestProgressionE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/PlayerLevelTestProgressionE2E.cpp)
	ly_add_game_test_case(LightYearsPlayerLevelTestProgressionE2E LightYearsPlayerLevelTestProgressionE2ETests
		ARGS "${LY_E2E}/player-level-test-progression.json" TIMEOUT 60)

	ly_add_game_test(LightYearsPlayerShipSelectionTests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/PlayerShipSelectionTests.cpp)
	ly_add_game_test_case(LightYearsPlayerShipSelection LightYearsPlayerShipSelectionTests
		ARGS "${LY_E2E}/player-ship-selection.json" TIMEOUT 60)

	ly_add_game_test(LightYearsCommonModuleE2ETests GAME_WHOLE PROJECT_DIR SOURCES
		${LY_TESTS_DIR}/CommonModuleE2ETests.cpp
		${LY_TESTS_DIR}/CommonModuleE2E_Core.cpp
		${LY_TESTS_DIR}/CommonModuleE2E_Cooldown.cpp
		${LY_TESTS_DIR}/CommonModuleE2E_FullMagazine.cpp)
	ly_add_game_test_case(LightYearsCommonModuleE2E LightYearsCommonModuleE2ETests
		ARGS "${LY_E2E}/common-modules.json" TIMEOUT 60)

	ly_add_game_test(LightYearsCommonModuleTriggerLifetimeTests GAME_WHOLE PROJECT_DIR SOURCES
		${LY_TESTS_DIR}/CommonModuleE2E_TriggerLifetime.cpp)
	ly_add_game_test_case(LightYearsCommonModuleTriggerLifetime LightYearsCommonModuleTriggerLifetimeTests
		ARGS "${LY_E2E}/common-module-trigger-lifetime.json" TIMEOUT 60)

	ly_add_game_test(LightYearsUncommonModuleE2ETests GAME_WHOLE PROJECT_DIR SOURCES
		${LY_TESTS_DIR}/UncommonModuleE2E.cpp
		${LY_TESTS_DIR}/UncommonModuleE2E_Content.cpp
		${LY_TESTS_DIR}/UncommonModuleE2E_Fixture.cpp
		${LY_TESTS_DIR}/UncommonModuleE2E_Roller.cpp
		${LY_TESTS_DIR}/UncommonModuleE2E_Runtime.cpp)
	ly_add_game_test_case(LightYearsUncommonModuleE2E LightYearsUncommonModuleE2ETests
		ARGS "${LY_E2E}/uncommon-modules.json" TIMEOUT 90)
	ly_add_game_test(LightYearsUncommonModuleP3E2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/UncommonModuleP3E2E.cpp)
	ly_add_game_test_case(LightYearsUncommonModuleP3E2E LightYearsUncommonModuleP3E2ETests ARGS "${LY_E2E}/uncommon-module-p3.json" TIMEOUT 120)
	ly_add_game_test(LightYearsControlTargetClassLoadE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ControlTargetClassLoadE2E.cpp)
	ly_add_game_test_case(LightYearsControlTargetClassLoadE2E LightYearsControlTargetClassLoadE2ETests ARGS "${LY_E2E}/control-target-class-load.json" TIMEOUT 60)
	ly_add_game_test(LightYearsAbilityChargeRestoreE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/AbilityChargeRestoreE2E.cpp)
	ly_add_game_test_case(LightYearsAbilityChargeRestoreE2E LightYearsAbilityChargeRestoreE2ETests ARGS "${LY_E2E}/ability-charge-restore.json" TIMEOUT 120)
	ly_add_game_test(LightYearsAbilityMultiChargeE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/AbilityMultiChargeE2E.cpp)
	ly_add_game_test_case(LightYearsAbilityMultiChargeE2E LightYearsAbilityMultiChargeE2ETests ARGS "${LY_E2E}/ability-multi-charge.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareDamageConditionsE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareDamageConditionsE2E.cpp)
	ly_add_game_test_case(LightYearsRareDamageConditionsE2E LightYearsRareDamageConditionsE2ETests ARGS "${LY_E2E}/rare-damage-conditions.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareShieldRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareShieldRulesE2E.cpp)
	ly_add_game_test_case(LightYearsRareShieldRulesE2E LightYearsRareShieldRulesE2ETests ARGS "${LY_E2E}/rare-shield-rules.json" TIMEOUT 120)
	ly_add_game_test(LightYearsModuleConflictE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModuleConflictE2E.cpp)
	ly_add_game_test_case(LightYearsModuleConflictE2E LightYearsModuleConflictE2ETests ARGS "${LY_E2E}/module-conflict.json" TIMEOUT 60)
	ly_add_game_test(LightYearsRareZoneRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareZoneRulesE2E.cpp)
	ly_add_game_test_case(LightYearsRareZoneRulesE2E LightYearsRareZoneRulesE2ETests ARGS "${LY_E2E}/rare-zone-rules.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareSingleModulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareSingleModulesE2E.cpp)
	ly_add_game_test_case(LightYearsRareSingleModulesE2E LightYearsRareSingleModulesE2ETests ARGS "${LY_E2E}/rare-single-modules.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareSpareCapacitorMechanicsE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareSpareCapacitorMechanicsE2E.cpp)
	ly_add_game_test_case(LightYearsRareSpareCapacitorMechanicsE2E LightYearsRareSpareCapacitorMechanicsE2ETests ARGS "${LY_E2E}/rare-spare-capacitor-mechanics.json" TIMEOUT 240)
	ly_add_game_test(LightYearsRareSideFeaturesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareSideFeaturesE2E.cpp)
	ly_add_game_test_case(LightYearsRareSideFeaturesE2E LightYearsRareSideFeaturesE2ETests ARGS "${LY_E2E}/rare-side-features.json" TIMEOUT 180)
	ly_add_game_test(LightYearsRareStaticDamageRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareStaticDamageRulesE2E.cpp)
	ly_add_game_test_case(LightYearsRareStaticDamageRulesE2E LightYearsRareStaticDamageRulesE2ETests ARGS "${LY_E2E}/rare-static-damage-rules.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareMovementRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareMovementRulesE2E.cpp)
	ly_add_game_test_case(LightYearsRareMovementRulesE2E LightYearsRareMovementRulesE2ETests ARGS "${LY_E2E}/rare-movement-rules.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareRunCounterE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareRunCounterE2E.cpp)
	ly_add_game_test_case(LightYearsRareRunCounterE2E LightYearsRareRunCounterE2ETests ARGS "${LY_E2E}/rare-run-counter.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareWave3ModulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareWave3ModulesE2E.cpp)
	ly_add_game_test_case(LightYearsRareWave3ModulesE2E LightYearsRareWave3ModulesE2ETests ARGS "${LY_E2E}/rare-wave3-modules.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareTrophyCoreMechanicsE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareTrophyCoreMechanicsE2E.cpp)
	ly_add_game_test_case(LightYearsRareTrophyCoreMechanicsE2E LightYearsRareTrophyCoreMechanicsE2ETests ARGS "${LY_E2E}/rare-trophy-core-mechanics.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareWave4PolishE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareWave4PolishE2E.cpp)
	ly_add_game_test_case(LightYearsRareWave4PolishE2E LightYearsRareWave4PolishE2ETests ARGS "${LY_E2E}/rare-wave4-polish.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareLadderModulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareLadderModulesE2E.cpp)
	ly_add_game_test_case(LightYearsRareLadderModulesE2E LightYearsRareLadderModulesE2ETests ARGS "${LY_E2E}/rare-ladder-modules.json" TIMEOUT 240)
	ly_add_game_test(LightYearsRareStatusRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareStatusRulesE2E.cpp)
	ly_add_game_test_case(LightYearsRareStatusRulesE2E LightYearsRareStatusRulesE2ETests ARGS "${LY_E2E}/rare-status-rules.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareOwnerBurstE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareOwnerBurstE2E.cpp)
	ly_add_game_test_case(LightYearsRareOwnerBurstE2E LightYearsRareOwnerBurstE2ETests ARGS "${LY_E2E}/rare-owner-burst.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareDyingStarMechanicsE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareDyingStarMechanicsE2E.cpp)
	ly_add_game_test_case(LightYearsRareDyingStarMechanicsE2E LightYearsRareDyingStarMechanicsE2ETests ARGS "${LY_E2E}/rare-dying-star-mechanics.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareRatioRulesE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareRatioRulesE2E.cpp)
	ly_add_game_test_case(LightYearsRareRatioRulesE2E LightYearsRareRatioRulesE2ETests ARGS "${LY_E2E}/rare-ratio-rules.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareSustainedSpeedE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareSustainedSpeedE2E.cpp)
	ly_add_game_test_case(LightYearsRareSustainedSpeedE2E LightYearsRareSustainedSpeedE2ETests ARGS "${LY_E2E}/rare-sustained-speed.json" TIMEOUT 120)
	ly_add_game_test(LightYearsRareStatusModifiersE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareStatusModifiersE2E.cpp)
	ly_add_game_test_case(LightYearsRareStatusModifiersE2E LightYearsRareStatusModifiersE2ETests ARGS "${LY_E2E}/rare-status-modifiers.json" TIMEOUT 240)
	ly_add_game_test(LightYearsRareWave5ContentE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareWave5ContentE2E.cpp)
	ly_add_game_test_case(LightYearsRareWave5ContentE2E LightYearsRareWave5ContentE2ETests ARGS "${LY_E2E}/rare-wave5-content.json" TIMEOUT 240)
	ly_add_game_test(LightYearsRareSlipstreamControlE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareSlipstreamControlE2E.cpp)
	ly_add_game_test_case(LightYearsRareSlipstreamControlE2E LightYearsRareSlipstreamControlE2ETests ARGS "${LY_E2E}/rare-slipstream-control.json" TIMEOUT 240)
	ly_add_game_test(LightYearsRareFragmentationE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/RareFragmentationE2E.cpp)
	ly_add_game_test_case(LightYearsRareFragmentationE2E LightYearsRareFragmentationE2ETests ARGS "${LY_E2E}/rare-fragmentation.json" TIMEOUT 240)
	ly_add_game_test(LightYearsUncommonModuleP4E2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/UncommonModuleP4E2E.cpp ${LY_TESTS_DIR}/UncommonModuleP4E2E_Damage.cpp ${LY_TESTS_DIR}/UncommonModuleP4E2E_Reactions.cpp ${LY_TESTS_DIR}/UncommonModuleP4E2E_Bridge.cpp)
	ly_add_game_test_case(LightYearsUncommonModuleP4E2E LightYearsUncommonModuleP4E2ETests ARGS "${LY_E2E}/uncommon-module-p4.json" TIMEOUT 120)
	ly_add_game_test(LightYearsUncommonModuleP5E2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/UncommonModuleP5E2E.cpp ${LY_TESTS_DIR}/UncommonModuleP5E2E_Status.cpp ${LY_TESTS_DIR}/UncommonModuleP5E2E_Content.cpp)
	ly_add_game_test_case(LightYearsUncommonModuleP5E2E LightYearsUncommonModuleP5E2ETests ARGS "${LY_E2E}/uncommon-module-p5.json" TIMEOUT 120)
	ly_add_game_test(LightYearsUncommonModuleP6E2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/UncommonModuleP6E2E.cpp ${LY_TESTS_DIR}/UncommonModuleP6E2E_Ricochet.cpp ${LY_TESTS_DIR}/UncommonModuleP6E2E_Actors.cpp ${LY_TESTS_DIR}/UncommonModuleP6E2E_Mines.cpp ${LY_TESTS_DIR}/UncommonModuleP6E2E_Content.cpp)
	ly_add_game_test_case(LightYearsUncommonModuleP6E2E LightYearsUncommonModuleP6E2ETests ARGS "${LY_E2E}/uncommon-module-p6.json" TIMEOUT 120)
	ly_add_game_test_case(LightYearsModuleBalanceE2E LightYearsUncommonModuleP6E2ETests ARGS "${LY_E2E}/module-balance-2026-10-13.json" "balance" TIMEOUT 120)
	ly_add_game_test(LightYearsModuleRefinementE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModuleRefinementE2E.cpp)
	ly_add_game_test(LightYearsModulePlanTransitionE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModulePlanTransitionE2E.cpp)
	ly_add_game_test(LightYearsModuleGoldHullE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModuleGoldHullE2E.cpp)
	ly_add_game_test_case(LightYearsModuleGoldHullE2E LightYearsModuleGoldHullE2ETests ARGS "${LY_E2E}/ModuleGoldHull-2026-10-14.json" TIMEOUT 120)
	ly_add_game_test(LightYearsModulePolarShieldE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModulePolarShieldE2E.cpp)
	ly_add_game_test_case(LightYearsModulePolarShieldE2E LightYearsModulePolarShieldE2ETests ARGS "${LY_E2E}/ModulePolarShield-2026-10-14.json" TIMEOUT 120)
	ly_add_game_test(LightYearsModuleReleaseValveE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModuleReleaseValveE2E.cpp)
	ly_add_game_test_case(LightYearsModuleReleaseValveE2E LightYearsModuleReleaseValveE2ETests ARGS "${LY_E2E}/module-release-valve-2026-10-14.json" TIMEOUT 120)
	ly_add_game_test(LightYearsModuleSpeedLanceE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModuleSpeedLanceE2E.cpp)
	ly_add_game_test_case(LightYearsModuleSpeedLanceE2E LightYearsModuleSpeedLanceE2ETests ARGS "${LY_E2E}/module-speed-lance-2026-10-13.json" TIMEOUT 120)
	ly_add_game_test(LightYearsModuleTemperedHullE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModuleTemperedHullE2E.cpp)
	ly_add_game_test_case(LightYearsModuleTemperedHullE2E LightYearsModuleTemperedHullE2ETests ARGS "${LY_E2E}/module-tempered-hull-2026-10-13.json" TIMEOUT 120)
	ly_add_game_test(LightYearsModuleDefenseLayerE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModuleDefenseLayerE2E.cpp)
	ly_add_game_test_case(LightYearsModuleDefenseLayerE2E LightYearsModuleDefenseLayerE2ETests ARGS "${LY_E2E}/module-defense-layer-2026-10-13.json" TIMEOUT 120)
	ly_add_game_test_case(LightYearsModulePlanTransitionE2E LightYearsModulePlanTransitionE2ETests ARGS "${LY_E2E}/module-plan-transition-2026-10-13.json" TIMEOUT 120)
	ly_add_game_test_case(LightYearsModuleRefinementE2E LightYearsModuleRefinementE2ETests ARGS "${LY_E2E}/module-refinement-2026-10-13.json" TIMEOUT 120)
	ly_add_game_test(LightYearsUncommonModuleP7E2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/UncommonModuleP7E2E.cpp)
	ly_add_game_test_case(LightYearsUncommonModuleP7E2E LightYearsUncommonModuleP7E2ETests ARGS "${LY_E2E}/uncommon-module-p7.json" TIMEOUT 120)
	ly_add_game_test(LightYearsUncommonModuleCompletionE2ETests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/UncommonModuleCompletionE2E.cpp ${LY_TESTS_DIR}/UncommonModuleCompletionE2E_Damage.cpp ${LY_TESTS_DIR}/UncommonModuleCompletionE2E_Reactive.cpp ${LY_TESTS_DIR}/UncommonModuleCompletionE2E_Content.cpp)
	ly_add_game_test_case(LightYearsUncommonModuleCompletionE2E LightYearsUncommonModuleCompletionE2ETests ARGS "${LY_E2E}/uncommon-module-completion.json" TIMEOUT 120)

	ly_add_game_test(LightYearsWeaponPhase2Tests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/WeaponPhase2Tests.cpp)
	ly_add_game_test_case(LightYearsWeaponPhase2 LightYearsWeaponPhase2Tests TIMEOUT 60)

	ly_add_game_test(LightYearsVitalsHUDTests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/VitalsHUDTests.cpp)
	ly_add_game_test_case(LightYearsVitalsHUD LightYearsVitalsHUDTests ARGS "${LY_E2E}/vitals-hud.json" TIMEOUT 60)

	ly_add_game_test(LightYearsModuleScreenTests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/ModuleScreenTests.cpp)
	ly_add_game_test_case(LightYearsModuleScreen LightYearsModuleScreenTests ARGS "${LY_E2E}/module-screen.json" TIMEOUT 60)

	ly_add_game_test(LightYearsHUDMigrationTests GAME_WHOLE PROJECT_DIR SOURCES ${LY_TESTS_DIR}/HUDMigrationTests.cpp)
	foreach(hud_group IN ITEMS Warning Notification Encounter Ability Menus)
		string(TOLOWER ${hud_group} hud_arg)
		ly_add_game_test_case(LightYearsHUDMigration${hud_group} LightYearsHUDMigrationTests
			ARGS --${hud_arg} "${LY_E2E}/hud-migration-${hud_arg}.json" TIMEOUT 60)
	endforeach()

	ly_add_game_test(LightYearsContinuousBeamWallE2ETests GAME_WHOLE PROJECT_DIR
		SOURCES
			${LY_TESTS_DIR}/ContinuousBeamWallE2E.cpp
			${LY_TESTS_DIR}/AbilityContentRegistrationE2E.cpp
			${LY_TESTS_DIR}/AbilityLoaderPublicLoadE2E.cpp
			${LY_TESTS_DIR}/TimerManagerSceneE2E.cpp
			${LY_TESTS_DIR}/GameHUDDamageEventE2E.cpp
			${LY_TESTS_DIR}/AuditFixesE2E.cpp
			${LY_TESTS_DIR}/AbilityBalanceSecondThreeE2E.cpp
			${LY_TESTS_DIR}/AbilityBalanceFirstThreeE2E.cpp
			${LY_TESTS_DIR}/AbilityBalanceThirdThreeE2E.cpp)
	ly_add_game_test_case(LightYearsContinuousBeamWallE2E LightYearsContinuousBeamWallE2ETests
		ARGS "${LY_E2E}/continuous-beam-wall.json")
	ly_add_game_test_case(LightYearsAbilityContentRegistrationE2E LightYearsContinuousBeamWallE2ETests
		ARGS --ability-content-registration "${LY_E2E}/ability-content-registration.json")
	ly_add_game_test_case(LightYearsAbilityLoaderPublicLoadE2E LightYearsContinuousBeamWallE2ETests
		ARGS --ability-loader-public-load "${LY_E2E}/ability-loader-public-load.json")
	ly_add_game_test_case(LightYearsTimerManagerSceneE2E LightYearsContinuousBeamWallE2ETests
		ARGS --timer-manager-scene "${LY_E2E}/timer-manager-scene.json" TIMEOUT 60)
	ly_add_game_test_case(LightYearsGameHUDDamageEventE2E LightYearsContinuousBeamWallE2ETests
		ARGS --game-hud-damage-event "${LY_E2E}/game-hud-damage-event.json" TIMEOUT 60)
	ly_add_game_test_case(LightYearsAuditFixesE2E LightYearsContinuousBeamWallE2ETests
		ARGS --audit-fixes "${LY_E2E}/audit-fixes.json" TIMEOUT 60)
	foreach(BALANCE_GROUP IN ITEMS first second third)
		ly_add_game_test_case(LightYearsAbilityBalance-${BALANCE_GROUP}-E2E LightYearsContinuousBeamWallE2ETests
			ARGS --ability-balance-${BALANCE_GROUP}-three "${LY_E2E}/ability-balance-${BALANCE_GROUP}.json" TIMEOUT 60)
	endforeach()

	option(
		LIGHT_YEARS_BUILD_SPECIALIZED_TESTS
		"Build small standalone specialist test executables"
		OFF
	)
	if(LIGHT_YEARS_BUILD_SPECIALIZED_TESTS)
		ly_add_game_test(LightYearsAutoTargetingTests
			SOURCES ${LY_TESTS_DIR}/AutoTargetingTests.cpp
			LIBS ${LIGHT_YEARS_ENGINE_TARGET_NAME} ${SPACE_ABILITY_SYSTEM_TARGET_NAME}
			GAME)
		ly_add_game_test_case(LightYearsAutoTargeting LightYearsAutoTargetingTests NO_WORKDIR)
	endif()

	# Campaign route data (galaxy_route.json); the test links GAME, which pulls LightYearsVisual via GameLib.
	ly_add_game_test(LightYearsGalaxyRouteTests GAME PROJECT_DIR SOURCES ${LY_TESTS_DIR}/GalaxyRouteTests.cpp)
	ly_add_game_test_case(LightYearsGalaxyRoute LightYearsGalaxyRouteTests NO_WORKDIR)

	# Test executables are rebuilt from scratch often; the incremental-link
	# state (.ilk, ~300 MB each) and full link PDBs (up to ~1.9 GB each) are
	# pure disk cost for them. FASTLINK keeps debugging working by referencing
	# the object files' debug info instead of copying it.
	if(MSVC)
		foreach(light_years_test_target IN LISTS LY_TEST_TARGETS)
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

function(CopyRuntimeLibToTarget LIB_NAME TARGET_NAME)
	add_custom_command(TARGET ${TARGET_NAME}
	POST_BUILD
	COMMAND
	${CMAKE_COMMAND} -E copy_if_different
	$<TARGET_FILE:${LIB_NAME}>
	$<TARGET_FILE_DIR:${TARGET_NAME}>
	)
endfunction()

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

# Non-Debug builds package the visual pipeline assets next to the executable (<exe dir>/visual); Debug builds keep
# reading the source tree (LY_VISUAL_ASSETS_DIR) so shader/palette/pipeline.json edits hot-reload.
add_custom_command(TARGET ${LIGHT_YEARS_GAME_ASSET_TARGET} POST_BUILD
	COMMAND
	${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,true,copy_directory>
	${CMAKE_SOURCE_DIR}/LightYearsVisual/assets
	$<TARGET_FILE_DIR:${LIGHT_YEARS_GAME_TARGET_NAME}>/visual
)

configure_file(
	"config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h" ESCAPE_QUOTES
)

target_include_directories(${LIGHT_YEARS_GAME_APP_TARGET_NAME} PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
)
