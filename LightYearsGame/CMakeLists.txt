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
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/dash/DashMovementMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/dash/DashMovementController.h

	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/GameplayAttribute.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/GameplayWarning.h

	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/AbilityStructs.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/AbilityActorStructs.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/AbilityCatalog.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameConfigs/ability/AbilityCatalog.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/DashConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/GravityAnomalyConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/RocketConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/ShieldConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ability/SunBeamConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/AttachmentConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/EffectStructs.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/EffectConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/world/EnvironmentConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/presentation/PointLightConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/ship/ShipConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/WeaponConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/WeaponProgressionConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/attributes/GameplayAttribute.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/attributes/AttributeMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/attributes/AttributeSystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/attributes/AttributeSystem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/progression/ShipProgression.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/progression/ShipProgression.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/attachment/AttachmentDefinition.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/attachment/AttachmentLoadout.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/attachment/AttachmentLoadout.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/damage/DamageContext.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameConfigs/combat/DamageTypeConfig.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/damage/DamageTypeSystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/damage/DamageTypeSystem.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/GameplayEffectHandle.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/GameplayEffectSpec.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/ActiveGameplayEffect.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/GameplayEffectValidation.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/GameplayEffectValidation.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/GameplayEffectContent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/GameplayEffectContent.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/AreaGameplayEffectApplicator.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/AreaGameplayEffectApplicator.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/GameplayEffectBehavior.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/GameplayEffectBehavior.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/BarrierEffectBehavior.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/BarrierEffectBehavior.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/effects/GameplayEffectSystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/GameplayEffectSystem.cpp
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
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/AbilityEvent.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/AbilityExecution.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/AbilityBehavior.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityBehavior.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/AbilityBehaviorRegistry.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityBehaviorRegistry.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/AbilityBehaviorRegistration.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityBehaviorRegistration.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/AbilityInstance.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityInstance.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/AbilityExecutor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityExecutor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/AbilitySystem.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilitySystem.cpp
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
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/rocket/RocketAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/rocket/RocketAbility.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/rocket/RocketProjectileActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/rocket/RocketProjectileActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/rocket/RocketVisualActor.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/rocket/RocketVisualActor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/shield/ShieldAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/shield/ShieldAbility.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/ability/sunBeam/SunBeamAbility.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/sunBeam/SunBeamAbility.cpp
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

target_link_libraries(${LIGHT_YEARS_GAME_TARGET_NAME} PUBLIC ${LIGHT_YEARS_ENGINE_TARGET_NAME})

if(BUILD_TESTING)
	add_executable(LightYearsGasLiteTests
		${CMAKE_CURRENT_SOURCE_DIR}/tests/GasLiteCoreTests.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameConfigs/ability/AbilityCatalog.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/attributes/AttributeSystem.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/progression/ShipProgression.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/attachment/AttachmentLoadout.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/damage/DamageTypeSystem.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/HealthComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ShieldComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/EnergyComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/MovementComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/GameplayEffectBehavior.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/GameplayEffectValidation.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/GameplayEffectContent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/AreaGameplayEffectApplicator.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/BarrierEffectBehavior.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/effects/GameplayEffectSystem.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/combat/Combatant.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/combat/CombatRuntime.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ship/ShipRuntime.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/spaceShip/SpaceShip.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/Reward.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/Player.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerManager.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerMovementComponent.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerSpaceShip.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityBehavior.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityBehaviorRegistry.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityBehaviorRegistration.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityInstance.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilitySystem.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/AbilityExecutor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/dash/DashAbility.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyAbility.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.cpp
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
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/rocket/RocketPresentationProfile.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/ability/sunBeam/SunBeamPresentationProfile.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/GameplayEffectVisual.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/presentation/effects/GameplayEffectVisualRegistry.cpp
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
	target_link_libraries(LightYearsGasLiteTests PRIVATE ${LIGHT_YEARS_ENGINE_TARGET_NAME})
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
