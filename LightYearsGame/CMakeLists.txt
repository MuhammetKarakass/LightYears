set(LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME LightYearsGameplayCore)

# Game-specific reusable gameplay services. This target deliberately depends on
# SpaceAbilitySystem instead of extending it with LightYears contracts, content,
# actors, or presentation rules.
add_library(${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME} STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/content/NumericSettingContract.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/content/NumericSettingContractRegistry.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/content/NumericSettingContractRegistry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/validation/AbilityActionValidator.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/validation/AbilityActionValidator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/validation/GameAbilityDefinitionValidator.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/validation/GameAbilityDefinitionValidator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/validation/GameplayEffectDefinitionValidator.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/validation/GameplayEffectDefinitionValidator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/GameContentBootstrap.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/GameContentBootstrap.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/GameAbilitySettingContracts.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/GameAbilitySettingContracts.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/LightYearsEffectBehaviorRuntime.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/LightYearsEffectBehaviorRuntime.cpp
)
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

add_executable(${LIGHT_YEARS_GAME_TARGET_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameFramework/GameApplication.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameFramework/GameApplication.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/spaceship/Spaceship.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/spaceship/Spaceship.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/player/PlayerSpaceship.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerSpaceship.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/player/PlayerMovementComponent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerMovementComponent.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/HealthComponent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/HealthComponent.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ShieldComponent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ShieldComponent.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/EnergyComponent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/EnergyComponent.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/MovementComponent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/MovementComponent.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/tags/GameplayTagSchema.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/targeting/TargetingTypes.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/targeting/AutoTargeting.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/targeting/AutoTargeting.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/AttachmentLoader.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/AttachmentLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/WeaponLoader.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/WeaponLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/WeaponContentCatalog.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/WeaponContentCatalog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/ShipLoader.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/ShipLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/ShipContentCatalog.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/ShipContentCatalog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/AbilityLoader.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/AbilityLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/ContentIdSchema.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/AbilityContentCatalog.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/AbilityContentCatalog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/EffectLoader.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/EffectLoader.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/content/EffectContentCatalog.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/EffectContentCatalog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/dash/DashMovementMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/dash/DashMovementController.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/dash/DashContracts.h

	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/GameplayWarning.h

	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/content/GameAbilityDefinition.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/GameAbilityActionExecutor.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/actions/AbilityActionAttributeResolver.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/actions/FireWeaponActionRuntime.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/AbilityActorStructs.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/AbilityCatalog.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameConfigs/ability/AbilityCatalog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/functional/DashConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/offensive/GravityAnomalyConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/offensive/InfernoSprayConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/offensive/RocketConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/defensive/ShieldConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/offensive/SunBeamConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/AttachmentConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/EffectStructs.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/EffectConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/world/EnvironmentConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/presentation/PointLightConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ship/ShipConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/attributes/AttributeIds.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/progression/ShipProgression.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/progression/ShipProgression.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/attachment/AttachmentDefinition.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/attachment/AttachmentLoadout.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/attachment/AttachmentLoadout.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/damage/DamageContext.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/DamageTypeConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/damage/DamageTypeSystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/damage/DamageTypeSystem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/content/barrier/BarrierEffectBehavior.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/content/barrier/BarrierEffectBehavior.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/PrimaryWeaponHandler.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/PrimaryWeaponHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/PrimaryWeaponHandlerRegistry.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/PrimaryWeaponHandlerRegistry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/PrimaryWeaponExecutionSystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/PrimaryWeaponExecutionSystem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/internal/PrimaryWeaponBuiltIns.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/internal/PrimaryWeaponBuiltIns.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/internal/PrimaryWeaponDefinitionValidator.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/internal/PrimaryWeaponDefinitionValidator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/StandardProjectileWeaponHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/ShotgunWeaponHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/ElectricArcWeaponHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/ContinuousBeamWeaponHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/ExpandingWaveWeaponHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/features/HeatWeaponFeatureHandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/wave/ExpandingWaveWeaponActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/wave/ExpandingWaveWeaponActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/impact/ProjectileImpactBehavior.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/impact/ShotgunVolleyImpactGroup.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/impact/ShotgunVolleyImpactGroup.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/visuals/ContinuousBeamVisualActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/visuals/ContinuousBeamVisualActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/weapon/visuals/ElectricArcVisualActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/visuals/ElectricArcVisualActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/combat/CombatRuntime.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/combat/CombatRuntime.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ship/ShipRuntime.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ship/ShipRuntime.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/combat/Combatant.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/combat/Combatant.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/GameAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/GameAbility.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/GameAbilityActionExecutor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actions/AbilityActionAttributeResolver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actions/FireWeaponActionRuntime.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/LightYearsAbilitySystemComponent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/LightYearsAbilitySystemComponent.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/actors/AbilityWorldActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actors/AbilityWorldActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/actors/AbilityActorRegistry.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actors/AbilityActorRegistry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/actors/AreaTelegraphActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actors/AreaTelegraphActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/dash/DashAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/dash/DashAbility.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/gravityAnomaly/GravityAnomalyAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyAbility.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/gravityAnomaly/GravityAnomalyContracts.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/infernoSpray/InfernoSprayAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/infernoSpray/InfernoSprayAbility.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/infernoSpray/InfernoSprayContracts.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/overdriveCore/OverdriveCoreContracts.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/infernoSpray/InfernoSprayActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/infernoSpray/InfernoSprayActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/rocket/RocketAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/rocket/RocketAbility.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/rocket/RocketContracts.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/rocket/RocketProjectileActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/rocket/RocketProjectileActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/rocket/RocketVisualActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/rocket/RocketVisualActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/shield/ShieldAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/shield/ShieldAbility.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/shield/ShieldContracts.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/sunBeam/SunBeamAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/sunBeam/SunBeamAbility.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/sunBeam/SunBeamContracts.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/sunBeam/SunBeamActorBase.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/sunBeam/SunBeamActorBase.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/sunBeam/SunBeamVisual.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/sunBeam/SunBeamVisual.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/sunBeam/SunBeamStrikeActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/sunBeam/SunBeamStrikeActor.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/AbilityPresentationContent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/AbilityPresentationContent.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/PresentationProfileRegistry.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/common/AreaTelegraphVisualDefinition.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/gravityAnomaly/GravityAnomalyPresentationIds.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/infernoSpray/InfernoSprayPresentationIds.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/infernoSpray/InfernoSprayPresentationProfile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/rocket/RocketPresentationIds.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/rocket/RocketPresentationProfile.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/rocket/RocketPresentationProfile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/sunBeam/SunBeamPresentationIds.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/ability/sunBeam/SunBeamPresentationProfile.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/sunBeam/SunBeamPresentationProfile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/GameplayEffectVisual.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/GameplayEffectVisual.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/GameplayEffectVisualRegistry.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/GameplayEffectVisualRegistry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/GameplayEffectPresentationBinding.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/GameplayEffectPresentationBinding.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualDefinition.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/gravityAnomaly/GravityAnomalyEffectVisual.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/gravityAnomaly/GravityAnomalyEffectVisual.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/shield/ShieldVisualContent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/shield/ShieldVisualContent.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/shield/ShieldVisualDefinition.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/shield/ShieldVisualIds.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/effects/shield/ShieldVisual.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/shield/ShieldVisual.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/VFX/Explosion.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/VFX/Explosion.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/EnemySpaceShip.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/EnemySpaceShip.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/DummyEnemy.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/DummyEnemy.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/Vanguard.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/Vanguard.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/LevelOne.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/LevelOne.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/MainMenuLevel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/MainMenuLevel.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/GameLevel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/GameLevel.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/ArenaDefinition.h

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/ArenaLevel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/ArenaLevel.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/ArenaBoundaryIndicator.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/ArenaBoundaryIndicator.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/ArenaBoundarySystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/ArenaBoundarySystem.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/ArenaTestLevel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/ArenaTestLevel.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/player/PlayerRespawnDefinition.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/player/PlayerRespawnSystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerRespawnSystem.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/VanguardStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/VanguardStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/TwinBlade.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/TwinBlade.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/TwinBladeStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/TwinBladeStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/Hexagon.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/Hexagon.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/HexagonStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/HexagonStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/UFO.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/UFO.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/UFOStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/UFOStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/ChaosStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/ChaosStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/LevelOneBoss.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/LevelOneBoss.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/LevelOneBossStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/LevelOneBossStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/InfiniteStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/InfiniteStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/environment/Asteroid.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/environment/Asteroid.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/environment/AsteroidSpawner.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/environment/AsteroidSpawner.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/player/Reward.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/Reward.cpp
	
	${CMAKE_CURRENT_SOURCE_DIR}/include/player/Player.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/Player.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/player/PlayerManager.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerManager.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/widget/GameHUD.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/widget/GameHUD.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/widget/MainMenuHUD.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/widget/MainMenuHUD.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/widget/PauseMenuHUD.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/widget/PauseMenuHUD.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/widget/GameOverHUD.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/widget/GameOverHUD.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/hud/GameplayWarningHUDController.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/hud/GameplayWarningHUDController.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/hud/HUDController.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/hud/ability/AbilityUIViewModel.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/presentation/hud/ability/AbilityUIController.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/hud/ability/AbilityUIController.cpp


	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/gameplay/GameplayConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/WeaponStructs.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ship/ShipStructs.h
)

target_include_directories(${LIGHT_YEARS_GAME_TARGET_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${LIGHT_YEARS_GAME_TARGET_NAME}
	PUBLIC
		${LIGHT_YEARS_ENGINE_TARGET_NAME}
		${SPACE_ABILITY_SYSTEM_TARGET_NAME}
		${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
		nlohmann_json::nlohmann_json
)

if(BUILD_TESTING)
	add_executable(LightYearsContentTests
		${CMAKE_CURRENT_SOURCE_DIR}/tests/ContentLoaderTests.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/AttachmentLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/WeaponLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/ShipLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/AbilityLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/EffectLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/EffectContentCatalog.cpp
	)
	target_include_directories(LightYearsContentTests PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/LightYearsEngine/include
	)
	target_compile_definitions(LightYearsContentTests PRIVATE
		LIGHT_YEARS_PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
	)
	target_link_libraries(LightYearsContentTests
		PRIVATE
			${LIGHT_YEARS_ENGINE_TARGET_NAME}
			${SPACE_ABILITY_SYSTEM_TARGET_NAME}
			${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
	)
	add_test(
		NAME LightYearsContent
		COMMAND LightYearsContentTests
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)

	add_executable(LightYearsGasLiteTests
		${CMAKE_CURRENT_SOURCE_DIR}/tests/GasLiteCoreTests.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameConfigs/ability/AbilityCatalog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/progression/ShipProgression.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/attachment/AttachmentLoadout.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/AttachmentLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/WeaponLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/WeaponContentCatalog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/ShipLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/ShipContentCatalog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/AbilityLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/AbilityContentCatalog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/EffectLoader.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/content/EffectContentCatalog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/damage/DamageTypeSystem.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/HealthComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ShieldComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/EnergyComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/MovementComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/targeting/AutoTargeting.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/content/barrier/BarrierEffectBehavior.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/combat/Combatant.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/combat/CombatRuntime.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ship/ShipRuntime.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/spaceShip/SpaceShip.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/Reward.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/Player.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerManager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerMovementComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerSpaceShip.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/GameAbility.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/LightYearsAbilitySystemComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/GameAbilityActionExecutor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actions/AbilityActionAttributeResolver.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actions/FireWeaponActionRuntime.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/dash/DashAbility.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyAbility.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/infernoSpray/InfernoSprayAbility.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/infernoSpray/InfernoSprayActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/rocket/RocketAbility.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/rocket/RocketProjectileActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/rocket/RocketVisualActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/shield/ShieldAbility.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/sunBeam/SunBeamAbility.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actors/AbilityWorldActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actors/AbilityActorRegistry.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/actors/AreaTelegraphActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/sunBeam/SunBeamActorBase.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/sunBeam/SunBeamVisual.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/sunBeam/SunBeamStrikeActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/AbilityPresentationContent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/infernoSpray/InfernoSprayPresentationProfile.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/rocket/RocketPresentationProfile.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/sunBeam/SunBeamPresentationProfile.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/GameplayEffectVisual.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/GameplayEffectVisualRegistry.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/GameplayEffectPresentationBinding.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/gravityAnomaly/GravityAnomalyEffectVisualContent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/gravityAnomaly/GravityAnomalyEffectVisual.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/shield/ShieldVisualContent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/shield/ShieldVisual.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/VFX/Explosion.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/PrimaryWeaponHandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/PrimaryWeaponHandlerRegistry.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/PrimaryWeaponExecutionSystem.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/internal/PrimaryWeaponBuiltIns.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/internal/PrimaryWeaponDefinitionValidator.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/StandardProjectileWeaponHandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/ShotgunWeaponHandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/ElectricArcWeaponHandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/ContinuousBeamWeaponHandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/handlers/ExpandingWaveWeaponHandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/features/HeatWeaponFeatureHandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/wave/ExpandingWaveWeaponActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/impact/ShotgunVolleyImpactGroup.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/visuals/ContinuousBeamVisualActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/weapon/visuals/ElectricArcVisualActor.cpp
	)
	target_include_directories(LightYearsGasLiteTests PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/LightYearsEngine/include
	)
	target_link_libraries(LightYearsGasLiteTests
		PRIVATE
			${LIGHT_YEARS_ENGINE_TARGET_NAME}
			${SPACE_ABILITY_SYSTEM_TARGET_NAME}
			${LIGHT_YEARS_GAMEPLAY_CORE_TARGET_NAME}
			nlohmann_json::nlohmann_json
	)
	add_executable(LightYearsAutoTargetingTests
		${CMAKE_CURRENT_SOURCE_DIR}/tests/AutoTargetingTests.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/targeting/AutoTargeting.cpp
	)
	target_include_directories(LightYearsAutoTargetingTests PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/LightYearsEngine/include
	)
	target_link_libraries(LightYearsAutoTargetingTests
		PRIVATE
			${LIGHT_YEARS_ENGINE_TARGET_NAME}
			${SPACE_ABILITY_SYSTEM_TARGET_NAME}
	)
	add_test(NAME LightYearsAutoTargeting COMMAND LightYearsAutoTargetingTests)
	add_executable(LightYearsGameplayTagSchemaTests
		${CMAKE_CURRENT_SOURCE_DIR}/tests/GameplayTagSchemaTests.cpp
	)
	target_include_directories(LightYearsGameplayTagSchemaTests PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${PROJECT_SOURCE_DIR}/LightYearsEngine/include
	)
	target_link_libraries(LightYearsGameplayTagSchemaTests
		PRIVATE
			${LIGHT_YEARS_ENGINE_TARGET_NAME}
	)
	add_test(NAME LightYearsGameplayTagSchema COMMAND LightYearsGameplayTagSchemaTests)
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

add_custom_command(
	TARGET ${LIGHT_YEARS_GAME_TARGET_NAME}
	POST_BUILD
	COMMAND
	${CMAKE_COMMAND} -E copy_directory
	${RESOURCE_SRC_DIR}
	$<TARGET_FILE_DIR:${LIGHT_YEARS_GAME_TARGET_NAME}>/${RESOURCE_FOLDER_NAME}
)

set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_FOLDER_NAME}/SpaceShooterRedux/Shaders")
set(SHADER_DST_DIR "$<TARGET_FILE_DIR:${LIGHT_YEARS_GAME_TARGET_NAME}>/${RESOURCE_FOLDER_NAME}/SpaceShooterRedux/Shaders")

add_custom_command(
	TARGET ${LIGHT_YEARS_GAME_TARGET_NAME}
	POST_BUILD
	COMMAND
	${CMAKE_COMMAND} -E copy_directory
	${SHADER_SRC_DIR}
	${SHADER_DST_DIR}
	COMMENT "Copying shader files to output directory..."
)

configure_file(
	"config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h" ESCAPE_QUOTES
)

target_include_directories(${LIGHT_YEARS_GAME_TARGET_NAME} PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}
)
