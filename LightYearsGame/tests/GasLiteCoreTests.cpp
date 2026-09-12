#include "gameplay/attributes/AttributeIds.h"
#include "framework/Core.h"
#include "attributes/AttributeMath.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/progression/ShipProgression.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/runtime/AbilityLifecycleDispatcher.h"
#include "gameplay/ability/runtime/AbilityUseHistory.h"
#include "gameplay/ability/validation/GameAbilityDefinitionValidator.h"
#include "gameplay/ability/validation/GameplayEffectDefinitionValidator.h"
#include "abilities/AbilityLifecycleOrchestrator.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "effects/GameplayEffectSpec.h"
#include "effects/GameplayEffectSystem.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/dash/DashMovementController.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h"
#include "gameplay/ability/nullPulse/NullPulseContracts.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierContracts.h"
#include "gameplay/ability/nullPulse/NullPulseVisualActor.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreProjectileActor.h"
#include "gameplay/ability/orbitalDrones/OrbitalDronesContracts.h"
#include "gameplay/ability/relayPrism/RelayPrismActor.h"
#include "gameplay/ability/relayPrism/RelayPrismContracts.h"
#include "gameplay/ability/orbitalDrones/OrbitingDroneActor.h"
#include "gameplay/ability/emberSwarm/EmberSwarmContracts.h"
#include "gameplay/ability/emberSwarm/EmberDroneActor.h"
#include "gameplay/ability/lanceDrive/LanceDriveContracts.h"
#include "gameplay/ability/lanceDrive/LanceDriveActor.h"
#include "presentation/ability/emberSwarm/EmberSwarmPresentationIds.h"
#include "presentation/ability/emberSwarm/EmberSwarmPresentationProfile.h"
#include "presentation/ability/lanceDrive/LanceDrivePresentationIds.h"
#include "presentation/ability/lanceDrive/LanceDrivePresentationProfile.h"
#include "gameplay/ability/executionDrive/ExecutionDriveContracts.h"
#include "gameplay/ability/echoProtocol/EchoProtocolContracts.h"
#include "gameplay/ability/scorchDrive/ScorchDriveContracts.h"
#include "gameplay/ability/ionStorm/IonStormContracts.h"
#include "gameplay/ability/ionStorm/IonStormProjectileActor.h"
#include "gameplay/ability/phaseDrift/PhaseDriftContracts.h"
#include "gameplay/ability/zeroDrag/ZeroDragContracts.h"
#include "gameplay/ability/hullShock/HullShockContracts.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/rocket/RocketVisualActor.h"
#include "gameplay/ability/railBurst/RailBurstProjectileActor.h"
#include "gameplay/ability/astralSurge/AstralSurgeContracts.h"
#include "gameplay/ability/chainLightning/ChainLightningContracts.h"
#include "gameplay/ability/combatSentry/CombatSentryContracts.h"
#include "gameplay/ability/astralSurge/AstralSurgeProjectileActor.h"
#include "gameplay/ability/mineLayer/MineLayerContracts.h"
#include "gameplay/ability/crescentReaver/CrescentReaverContracts.h"
#include "gameplay/ability/crescentReaver/CrescentReaverProjectileActor.h"
#include "gameplay/ability/actors/DirectionalChargeTelegraphActor.h"
#include "gameplay/ability/energySpear/EnergySpearContracts.h"
#include "gameplay/ability/energySpear/EnergySpearTraversalActor.h"
#include "gameplay/ability/frostMaelstrom/FrostMaelstromContracts.h"
#include "gameplay/ability/frozenThrong/FrozenThrongContracts.h"
#include "gameplay/ability/wingSentinels/WingSentinelsContracts.h"
#include "gameplay/ability/crystalBarricade/CrystalBarricadeContracts.h"
#include "gameplay/ability/crystalBarricade/CrystalBarricadeActor.h"
#include "gameplay/ability/seismicCharge/SeismicChargeContracts.h"
#include "gameplay/ability/temporalConvergence/TemporalConvergenceContracts.h"
#include "gameplay/ability/arcScythes/ArcScythesContracts.h"
#include "gameplay/ability/ironcladProtocol/IroncladProtocolContracts.h"
#include "gameplay/ability/temporalRecall/TemporalRecallContracts.h"
#include "gameplay/ability/timeSlip/TimeSlipContracts.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitContracts.h"
#include "gameplay/ability/foldspaceArena/FoldspaceArenaContracts.h"
#include "gameplay/ability/aegisReaver/AegisReaverContracts.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/ability/shieldGraft/ShieldGraftContracts.h"
#include "gameplay/ability/reclaimerProtocol/ReclaimerProtocolContracts.h"
#include "gameplay/ability/reclaimerProtocol/ReclaimerRepairKitActor.h"
#include "presentation/ability/reclaimerProtocol/ReclaimerProtocolPresentationIds.h"
#include "presentation/ability/reclaimerProtocol/ReclaimerProtocolPresentationProfile.h"
#include "gameConfigs/ability/defensive/ReclaimerProtocolConfig.h"
#include "gameplay/ability/returnProtocol/ReturnProtocolVisualActor.h"
#include "gameplay/ability/ionStorm/IonStormBoundary.h"
#include "enemy/DummyEnemy.h"
#include "framework/AssetManager.h"
#include "gameplay/ability/sunBeam/SunBeamStrikeActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationIds.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.h"
#include "presentation/ability/rocket/RocketPresentationIds.h"
#include "presentation/ability/rocket/RocketPresentationProfile.h"
#include "presentation/ability/sunBeam/SunBeamPresentationIds.h"
#include "presentation/ability/sunBeam/SunBeamPresentationProfile.h"
#include "presentation/ability/orbitalDrones/OrbitalDronesPresentationIds.h"
#include "presentation/ability/orbitalDrones/OrbitalDronesPresentationProfile.h"
#include "presentation/ability/ionStorm/IonStormPresentationProfile.h"
#include "presentation/ability/directionalBarrier/DirectionalBarrierPresentationIds.h"
#include "presentation/ability/directionalBarrier/DirectionalBarrierPresentationProfile.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierVisualActor.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/ContactDamageGuardRegistry.h"
#include "gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h"
#include "gameplay/effects/content/directionalBarrier/DirectionalBarrierEffectBehavior.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisual.h"
#include "gameplay/ship/ShipRuntime.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/EnergyComponent.h"
#include "gameplay/ability/dash/DashMovementMath.h"
#include "gameplay/ability/dash/DashContracts.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/camera/CameraManager.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/ability/defensive/ReturnProtocolConfig.h"
#include "gameConfigs/ability/control/GravityAnomalyConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ship/ShipConfig.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationProfile.h"
#include "player/PlayerSpaceShip.h"
#include "gameplay/input/AbilityInputSchema.h"
#include "spaceShip/SpaceShip.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/weapon/PrimaryWeaponHandlerRegistry.h"
#include "gameplay/weapon/impact/ShotgunVolleyImpactGroup.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.h"
#include "gameplay/weapon/wave/ExpandingWaveWeaponActor.h"
#include "gameplay/weapon/visuals/ElectricArcVisualActor.h"
#include "gameplay/attachment/AttachmentLoadout.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameConfigs/combat/AttachmentConfig.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace
{
	bool NearlyEqual(float left, float right)
	{
		return std::abs(left - right) < 0.0001f;
	}

	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}

	const PrimaryWeaponDefinition& LoadedWeapon(const char* weaponId)
	{
		return *ly::content::WeaponContentCatalog::FindById(weaponId);
	}

	bool IsEffectBehaviorRegisteredForValidation(const sas::GameplayEffectBehaviorKey& behaviorKey)
	{
		return ly::GetEffectBehaviorRuntime().IsRegistered(behaviorKey);
	}

	bool ValidateEffectDefinition(
		const sas::GameplayEffectDefinition& definition,
		std::string* failureReason = nullptr
	)
	{
		return ly::GameplayEffectDefinitionValidator::Validate(
			definition,
			IsEffectBehaviorRegisteredForValidation,
			failureReason
		);
	}

	bool ValidateAbilityDefinition(
		const ly::GameAbilityDefinition& definition,
		std::string* failureReason = nullptr
	)
	{
		return ly::GameAbilityDefinitionValidator::Validate(
			definition,
			ValidateEffectDefinition,
			failureReason
		);
	}

	bool ValidateAbilityCatalog(
		const ly::List<const ly::GameAbilityDefinition*>& definitions,
		std::string* failureReason = nullptr
	)
	{
		return ly::GameAbilityDefinitionValidator::ValidateCatalog(
			definitions,
			ValidateEffectDefinition,
			failureReason
		);
	}

	const ly::AbilityActorDefinition& LoadedAbilityActor(const char* actorDefinitionId)
	{
		return *AbilityData::FindAbilityActorDefinition(actorDefinitionId);
	}

	class TestCombatant final
		: public ly::Actor
		, public ly::Combatant
		, public ly::DashMovementController
	{
	public:
		explicit TestCombatant(ly::World* world = nullptr, float maximumHealth = 100.f)
			: Actor{ world }
			, mHealth{ maximumHealth, maximumHealth }
			, mCombatRuntime{ *this }
		{
			mCombatRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(ly::OwnerAttributeIds::Armor, 0.f);
			mCombatRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(ly::OwnerAttributeIds::AbilityHaste, 0.f, -0.95f);
			mCombatRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(ly::OwnerAttributeIds::MoveSpeedHorizontal, 0.f, -0.95f);
			mCombatRuntime.GetAbilitySystemComponent().GetAttributes().RegisterAttribute(ly::OwnerAttributeIds::MoveSpeedVertical, 0.f, -0.95f);
		}

		ly::CombatRuntime& GetCombatRuntime() override { return mCombatRuntime; }
		const ly::CombatRuntime& GetCombatRuntime() const override { return mCombatRuntime; }
		float GetHealth() const { return mHealth.GetHealth(); }

		void SetDashMovementInput(const sf::Vector2f& input) { mDashMovementInput = input; }
		void SetDashAimDirection(const sf::Vector2f& direction) { mDashAimDirection = direction; }
		float GetLastDashDistance() const { return mLastDashDistance; }
		const sf::Vector2f& GetLastDashDirection() const { return mLastDashDirection; }
		bool IsDashActive() const { return mDashActive; }
		int GetDashStartCount() const { return mDashStartCount; }
		int GetDashEndCount() const { return mDashEndCount; }

		sf::Vector2f ResolveDashDirection() const override
		{
			sf::Vector2f direction = mDashMovementInput;
			if (NormalizeDirection(direction))
			{
				return direction;
			}

			direction = mDashAimDirection;
			return NormalizeDirection(direction) ? direction : sf::Vector2f{ 0.f, 0.f };
		}

		bool StartDash(const ly::DashRequest& request) override
		{
			if (request.baseDistance <= 0.f || request.duration <= 0.f)
			{
				return false;
			}

			sf::Vector2f direction = request.direction;
			if (!NormalizeDirection(direction))
			{
				return false;
			}

			mLastDashDistance = ly::DashMovementMath::ResolveDistance(
				request.baseDistance,
				mCombatRuntime.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::MoveSpeedHorizontal),
				mCombatRuntime.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::MoveSpeedVertical)
			);
			mLastDashDirection = direction;
			mDashActive = mLastDashDistance > 0.f;
			if (mDashActive)
			{
				++mDashStartCount;
			}
			return mDashActive;
		}

		void EndDash() override
		{
			if (mDashActive)
			{
				++mDashEndCount;
			}
			mDashActive = false;
		}

		void ReceiveDamage(ly::DamageContext context) override
		{
			mCombatRuntime.ProcessIncomingDamage(context);
			if (context.remainingDamage > 0.f)
			{
				const float healthBeforeDamage = mHealth.GetHealth();
				mHealth.ChangeHealth(-context.remainingDamage);
				context.appliedDamage = healthBeforeDamage - mHealth.GetHealth();
			}
			mCombatRuntime.NotifyDamageResolved(context);
		}

	private:
		static bool NormalizeDirection(sf::Vector2f& direction)
		{
			const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			if (length <= 0.001f)
			{
				return false;
			}
			direction.x /= length;
			direction.y /= length;
			return true;
		}

		ly::HealthComponent mHealth;
		ly::CombatRuntime mCombatRuntime;
		sf::Vector2f mDashMovementInput{ 0.f, 0.f };
		sf::Vector2f mDashAimDirection{ 0.f, 0.f };
		sf::Vector2f mLastDashDirection{ 0.f, 0.f };
		float mLastDashDistance = 0.f;
		bool mDashActive = false;
		int mDashStartCount = 0;
		int mDashEndCount = 0;
	};

	class TestClearableProjectile final : public ly::AbilityWorldActor
	{
	public:
		TestClearableProjectile(ly::World* world, ly::Actor* owner)
			: AbilityWorldActor{ world, owner }
		{
			SetAbilityPhysicsEnabled(false);
		}

		bool IsProjectileActor() const override { return true; }
	};

	class TestPersistentAbilityActor final : public ly::AbilityWorldActor
	{
	public:
		TestPersistentAbilityActor(ly::World* world, ly::Actor* owner)
			: AbilityWorldActor{ world, owner }
		{
			SetAbilityPhysicsEnabled(false);
		}
	};

	struct DashEventRecorder
	{
		void Record(const sas::AbilityEvent& event)
		{
			events.push_back(event.eventTag);
		}

		ly::List<ly::GameplayTag> events;
	};

	struct PrimaryWeaponScalingSample
	{
		float damage = 0.f;
		float fireRate = 0.f;
		float attackPower = 0.f;
		float attackSpeed = 0.f;
		float energyPower = 0.f;
		float luck = 0.f;
	};

	// Synthetic progression fixture used to exercise owner-attribute scaling independently.
	const ShipProgressionDefinition FighterProgressionDefinition{
		100.f,
		1.25f,
		{
			{ ly::OwnerAttributeIds::AttackPower, 3.f },
			{ ly::OwnerAttributeIds::AttackSpeed, 2.f },
			{ ly::OwnerAttributeIds::CriticalChance, 2.f },
			{ ly::OwnerAttributeIds::MaxHealth, 1.f },
			{ ly::OwnerAttributeIds::Armor, 1.f },
			{ ly::OwnerAttributeIds::EnergyPower, 0.5f },
			{ ly::OwnerAttributeIds::MoveSpeedHorizontal, 0.5f },
			{ ly::OwnerAttributeIds::MoveSpeedVertical, 0.5f }
		}
	};

	bool ResolveFighterPrimaryWeaponScaling(
		const PrimaryWeaponDefinition& weapon,
		int level,
		PrimaryWeaponScalingSample& sample,
		float additionalAttackPower = 0.f,
		float additionalAttackSpeed = 0.f,
		float additionalEnergyPower = 0.f
	)
	{
		TestCombatant owner;
		ly::CombatRuntime& runtime = owner.GetCombatRuntime();
		runtime.InitializeOwnerAttributes(100.f);

		ly::ShipProgression progression;
		progression.Configure(FighterProgressionDefinition);
		progression.BindAttributes(runtime.GetAbilitySystemComponent().GetAttributes());
		while (progression.GetLevel() < level)
		{
			progression.AddXP(progression.GetXPRequiredForNextLevel());
		}
		runtime.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ ly::OwnerAttributeIds::AttackPower, additionalAttackPower }
		);
		runtime.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ ly::OwnerAttributeIds::AttackSpeed, additionalAttackSpeed }
		);
		runtime.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ ly::OwnerAttributeIds::EnergyPower, additionalEnergyPower }
		);

		const sas::AbilityHandle handle = runtime.GetAbilitySystemComponent().GrantAbility(
			AbilityData::MakePrimaryFireAbilityDefinition(weapon)
		);
		if (!handle.IsValid())
		{
			return false;
		}
		runtime.GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
		runtime.GetAbilitySystemComponent().Tick(0.f);
		const ly::GameAbility* instance = runtime.GetAbilitySystemComponent().GetAbility(handle);
		if (!instance || instance->GetPrimaryWeaponRuntimeAttributes().empty())
		{
			return false;
		}

		const sas::GameplayAttributeList& attributes = instance->GetPrimaryWeaponRuntimeAttributes();
		sample.damage = sas::FindAttributeValue(attributes, ly::CommonAttributeIds::Damage, 0.f);
		sample.fireRate = sas::FindAttributeValue(attributes, ly::CommonAttributeIds::FireRate, 0.f);
		sample.attackPower = runtime.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::AttackPower);
		sample.attackSpeed = runtime.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::AttackSpeed);
		sample.energyPower = runtime.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::EnergyPower);
		sample.luck = runtime.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::Luck);
		return true;
	}

	struct ElectricArcShotSample
	{
		ly::List<float> targetDamage;
	};

	bool FireElectricArcShot(
		int targetCount,
		float luckRating,
		ElectricArcShotSample& sample,
		bool includeDamageTypes = true
	)
	{
		ly::World world{ nullptr };
		const ly::shared_ptr<TestCombatant> owner = world.SpawnActor<TestCombatant>(1000.f).lock();
		if (!owner)
		{
			return false;
		}
		owner->GetCombatRuntime().InitializeOwnerAttributes(1000.f);
		owner->GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ ly::OwnerAttributeIds::Luck, luckRating }
		);
		owner->SetCollisionLayer(CollisionLayer::Player);

		ly::List<ly::shared_ptr<TestCombatant>> targets;
		for (int targetIndex = 0; targetIndex < targetCount; ++targetIndex)
		{
			const ly::shared_ptr<TestCombatant> target =
				world.SpawnActor<TestCombatant>(1000.f).lock();
			if (!target)
			{
				return false;
			}
			target->SetCollisionLayer(CollisionLayer::Enemy);
			target->SetCollisionMask(CollisionLayer::PlayerBullet);
			target->SetActorLocation({ static_cast<float>(targetIndex) * 100.f, 100.f });
			targets.push_back(target);
		}
		world.TickInternal(0.f);

		PrimaryWeaponDefinition definition = LoadedWeapon("Weapon.Arc.ElectricLauncher.Basic");
		if (!includeDamageTypes)
		{
			definition.damageTags.clear();
		}
		ly::PrimaryWeaponRuntimeState runtime;
		if (!ly::PrimaryWeaponExecutionSystem::InitializeRuntime(definition, runtime).isValid)
		{
			return false;
		}
		const ly::PrimaryWeaponExecutionContext context{
			*owner,
			definition,
			definition.attributes,
			definition.damageTags
		};
		ly::PrimaryWeaponExecutionSystem::BeginFire(context, runtime);
		if (!ly::PrimaryWeaponExecutionSystem::FireOnce(context, runtime))
		{
			return false;
		}
		for (const ly::shared_ptr<TestCombatant>& target : targets)
		{
			sample.targetDamage.push_back(1000.f - target->GetHealth());
		}
		ly::PrimaryWeaponExecutionSystem::EndFire(context, runtime);
		return true;
	}

	float MeasureHighLuckElectricBonusChainRate(int shotCount)
	{
		ly::World world{ nullptr };
		const ly::shared_ptr<TestCombatant> owner = world.SpawnActor<TestCombatant>(1000000.f).lock();
		if (!owner)
		{
			return -1.f;
		}
		owner->GetCombatRuntime().InitializeOwnerAttributes(1000000.f);
		owner->GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ ly::OwnerAttributeIds::Luck, 100000.f }
		);
		owner->SetCollisionLayer(CollisionLayer::Player);

		ly::List<ly::shared_ptr<TestCombatant>> targets;
		for (int targetIndex = 0; targetIndex < 5; ++targetIndex)
		{
			const ly::shared_ptr<TestCombatant> target =
				world.SpawnActor<TestCombatant>(1000000.f).lock();
			if (!target)
			{
				return -1.f;
			}
			target->SetCollisionLayer(CollisionLayer::Enemy);
			target->SetCollisionMask(CollisionLayer::PlayerBullet);
			target->SetActorLocation({ static_cast<float>(targetIndex) * 100.f, 100.f });
			targets.push_back(target);
		}
		world.TickInternal(0.f);

		PrimaryWeaponDefinition definition = LoadedWeapon("Weapon.Arc.ElectricLauncher.Basic");
		definition.damageTags.clear();
		ly::PrimaryWeaponRuntimeState runtime;
		if (!ly::PrimaryWeaponExecutionSystem::InitializeRuntime(definition, runtime).isValid)
		{
			return -1.f;
		}
		const ly::PrimaryWeaponExecutionContext context{
			*owner,
			definition,
			definition.attributes,
			definition.damageTags
		};
		ly::PrimaryWeaponExecutionSystem::BeginFire(context, runtime);
		for (int shotIndex = 0; shotIndex < shotCount; ++shotIndex)
		{
			if (!ly::PrimaryWeaponExecutionSystem::FireOnce(context, runtime))
			{
				return -1.f;
			}
		}
		ly::PrimaryWeaponExecutionSystem::EndFire(context, runtime);

		const float baseDamage = sas::FindAttributeValue(
			definition.attributes,
			ly::CommonAttributeIds::Damage,
			0.f
		);
		const float falloff = sas::FindAttributeValue(
			definition.attributes,
			PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain,
			1.f
		);
		const float damagePerBonusChain = baseDamage * std::pow(falloff, 4.f);
		return damagePerBonusChain > 0.f
			? (1000000.f - targets.back()->GetHealth()) / damagePerBonusChain / static_cast<float>(shotCount)
			: -1.f;
	}

	float MeasureBeamHeatAfterFiring(float startingHeat, float attackSpeed)
	{
		TestCombatant owner;
		owner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
		owner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ ly::OwnerAttributeIds::AttackSpeed, attackSpeed }
		);
		ly::PrimaryWeaponRuntimeState runtime;
		const PrimaryWeaponDefinition& definition = LoadedWeapon("Weapon.Beam.ContinuousHeatLaser.Basic");
		if (!ly::PrimaryWeaponExecutionSystem::InitializeRuntime(definition, runtime).isValid)
		{
			return -1.f;
		}
		const ly::PrimaryWeaponExecutionContext context{
			owner,
			definition,
			definition.attributes,
			definition.damageTags,
			nullptr,
			&runtime
		};
		ly::PrimaryWeaponExecutionSystem::BeginFire(context, runtime);
		runtime.SetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue, startingHeat);
		ly::PrimaryWeaponExecutionSystem::TickFire(context, runtime, 0.1f);
		const float heat = runtime.GetFeatureValue(
			PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
		);
		ly::PrimaryWeaponExecutionSystem::EndFire(context, runtime);
		return heat;
	}

	float MeasureBeamTimeToOverheat(float attackSpeed)
	{
		TestCombatant owner;
		owner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
		owner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ ly::OwnerAttributeIds::AttackSpeed, attackSpeed }
		);
		ly::PrimaryWeaponRuntimeState runtime;
		const PrimaryWeaponDefinition& definition = LoadedWeapon("Weapon.Beam.ContinuousHeatLaser.Basic");
		if (!ly::PrimaryWeaponExecutionSystem::InitializeRuntime(definition, runtime).isValid)
		{
			return -1.f;
		}
		const ly::PrimaryWeaponExecutionContext context{
			owner,
			definition,
			definition.attributes,
			definition.damageTags,
			nullptr,
			&runtime
		};
		ly::PrimaryWeaponExecutionSystem::BeginFire(context, runtime);
		for (int step = 1; step <= 2000; ++step)
		{
			ly::PrimaryWeaponExecutionSystem::TickFire(context, runtime, 0.01f);
			if (ly::PrimaryWeaponExecutionSystem::ConsumeRequestedCooldown(runtime) > 0.f)
			{
				ly::PrimaryWeaponExecutionSystem::EndFire(context, runtime);
				return static_cast<float>(step) * 0.01f;
			}
		}
		ly::PrimaryWeaponExecutionSystem::EndFire(context, runtime);
		return -1.f;
	}

	float MeasureBeamSustainedDps(float attackSpeed, float duration)
	{
		ly::World world{ nullptr };
		const ly::shared_ptr<TestCombatant> owner = world.SpawnActor<TestCombatant>(100000.f).lock();
		const ly::shared_ptr<TestCombatant> target = world.SpawnActor<TestCombatant>(100000.f).lock();
		if (!owner || !target || duration <= 0.f)
		{
			return -1.f;
		}
		owner->GetCombatRuntime().InitializeOwnerAttributes(100000.f);
		owner->GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ ly::OwnerAttributeIds::AttackSpeed, attackSpeed }
		);
		owner->SetCollisionLayer(CollisionLayer::Player);
		owner->SetActorRotation(90.f);
		target->SetCollisionLayer(CollisionLayer::Enemy);
		target->SetCollisionMask(CollisionLayer::PlayerBullet);
		target->SetActorLocation({ 400.f, 0.f });
		world.TickInternal(0.f);

		ly::PrimaryWeaponRuntimeState runtime;
		const PrimaryWeaponDefinition& definition = LoadedWeapon("Weapon.Beam.ContinuousHeatLaser.Basic");
		if (!ly::PrimaryWeaponExecutionSystem::InitializeRuntime(definition, runtime).isValid)
		{
			return -1.f;
		}
		const ly::PrimaryWeaponExecutionContext context{
			*owner,
			definition,
			definition.attributes,
			definition.damageTags,
			nullptr,
			&runtime
		};
		ly::PrimaryWeaponExecutionSystem::BeginFire(context, runtime);
		world.TickInternal(0.f);
		float elapsed = 0.f;
		while (elapsed < duration)
		{
			const float tick = std::min(0.05f, duration - elapsed);
			ly::PrimaryWeaponExecutionSystem::TickFire(context, runtime, tick);
			elapsed += tick;
			const float cooldown = ly::PrimaryWeaponExecutionSystem::ConsumeRequestedCooldown(runtime);
			if (cooldown <= 0.f)
			{
				continue;
			}

			ly::PrimaryWeaponExecutionSystem::EndFire(context, runtime);
			elapsed += std::min(cooldown, duration - elapsed);
			if (elapsed < duration)
			{
				ly::PrimaryWeaponExecutionSystem::BeginFire(context, runtime);
				world.TickInternal(0.f);
			}
		}
		ly::PrimaryWeaponExecutionSystem::EndFire(context, runtime);
		return (100000.f - target->GetHealth()) / duration;
	}

}

int main()
{
	using namespace ly;

	{
		ContactDamageGuardRegistry registry;
		std::vector<int> callOrder;
		const ContactDamageGuardHandle first = registry.Register(
			[&](const Actor&, const Actor&)
			{
				callOrder.push_back(1);
				return true;
			}
		);
		const ContactDamageGuardHandle second = registry.Register(
			[&](const Actor&, const Actor&)
			{
				callOrder.push_back(2);
				return false;
			}
		);
		Actor source{ nullptr };
		Actor target{ nullptr };
		if (!first.IsValid() || !second.IsValid() || first == second ||
			registry.IsEmpty() || registry.Allows(source, target) ||
			callOrder != std::vector<int>{ 1, 2 })
		{
			return Fail("Contact damage guards did not preserve order and false veto semantics");
		}
		if (!registry.Unregister(first) || registry.Unregister(first) ||
			registry.Allows(source, target))
		{
			return Fail("Contact damage guard unregister did not preserve the remaining veto");
		}
		if (!registry.Unregister(second) || !registry.IsEmpty() ||
			!registry.Allows(source, target))
		{
			return Fail("Contact damage guard registry did not recover after unregistering guards");
		}

		const ContactDamageGuardHandle stale = registry.Register(
			[](const Actor&, const Actor&)
			{
				return false;
			}
		);
		registry.Clear();
		const ContactDamageGuardHandle afterClear = registry.Register(
			[](const Actor&, const Actor&)
			{
				return true;
			}
		);
		if (!stale.IsValid() || !afterClear.IsValid() || stale == afterClear ||
			registry.Unregister(stale) || !registry.Allows(source, target))
		{
			return Fail("Contact damage guard handles were reused after Clear");
		}
	}

	{
		TestCombatant source;
		TestCombatant target;
		const ContactDamageGuardHandle sourceGuard = source.GetCombatRuntime()
			.GetContactDamageGuardRegistry()
			.Register(
				[](const Actor&, const Actor&)
				{
					return false;
				}
			);
		if (!sourceGuard.IsValid() || CanApplyContactDamage(source, target))
		{
			return Fail("Contact damage did not query the source combat runtime guard registry");
		}
		if (!source.GetCombatRuntime().GetContactDamageGuardRegistry().Unregister(sourceGuard))
		{
			return Fail("Source contact damage guard could not be unregistered");
		}

		const ContactDamageGuardHandle targetGuard = target.GetCombatRuntime()
			.GetContactDamageGuardRegistry()
			.Register(
				[](const Actor&, const Actor&)
				{
					return false;
				}
			);
		if (!targetGuard.IsValid() || CanApplyContactDamage(source, target))
		{
			return Fail("Contact damage did not query the target combat runtime guard registry");
		}
		target.GetCombatRuntime().Clear();
		if (!CanApplyContactDamage(source, target))
		{
			return Fail("Combat runtime Clear did not clear contact damage guards");
		}
	}

	// Common lifecycle infrastructure must remain testable without booting the
	// full game content catalog. This verifies the reusable LIFO semantics and
	// proves that observers and veto guards are independent concerns.
	{
		AbilityUseHistory history;
		for (int index = 0; index < 12; ++index)
		{
			AbilityUseRecord record;
			record.abilityId = sas::ContentId{
				"Ability.Test.History." + std::to_string(index)
			};
			record.level = index + 1;
			history.Record(std::move(record));
		}

		if (history.GetRecords().size() != AbilityUseHistory::MaxRecords ||
			history.GetRecords().front().abilityId !=
				sas::ContentId{ "Ability.Test.History.11" } ||
			history.GetRecords().back().abilityId !=
				sas::ContentId{ "Ability.Test.History.2" })
		{
			return Fail("Ability-use history did not retain the newest ten records");
		}

		const AbilityUseRecord* latest = history.FindLatestUnconsumed(
			[](const AbilityUseRecord& record)
			{
				return record.abilityId == "Ability.Test.History.7" ||
					record.abilityId == "Ability.Test.History.8";
			}
		);
		if (!latest || latest->abilityId != "Ability.Test.History.8" ||
			!history.Consume(latest->sequence))
		{
			return Fail("Ability-use history did not expose or consume the LIFO record");
		}
		if (history.GetRecords().size() != AbilityUseHistory::MaxRecords)
		{
			return Fail("Ability-use history did not preserve a consumed snapshot for future systems");
		}
		const AbilityUseRecord* previous = history.FindLatestUnconsumed(
			[](const AbilityUseRecord& record)
			{
				return record.abilityId == "Ability.Test.History.7" ||
					record.abilityId == "Ability.Test.History.8";
			}
		);
		if (!previous || previous->abilityId != "Ability.Test.History.7")
		{
			return Fail("Ability-use history did not expose the next unconsumed record");
		}
	}

	{
		AbilityLifecycleDispatcher dispatcher;
		int observed = 0;
		int guardCalls = 0;
		AbilityLifecycleObserverHandle observer = dispatcher.RegisterObserver(
			AbilityLifecycleObserverFilter{
				std::nullopt,
				std::nullopt,
				sas::AbilityActivationOrigin::NormalInput,
				std::nullopt
			},
			[&](const sas::AbilityLifecycleEvent&)
			{
				++observed;
			}
		);
		AbilityActivationGuardHandle guard = dispatcher.RegisterActivationGuard(
			[&](const sas::AbilityLifecycleEvent& event)
			{
				++guardCalls;
				return event.abilityId != "Ability.Test.Blocked";
			}
		);

		sas::AbilityLifecycleEvent allowed;
		allowed.abilityId = sas::ContentId{ "Ability.Test.Allowed" };
		allowed.activationOrigin = sas::AbilityActivationOrigin::NormalInput;
		sas::AbilityLifecycleEvent blocked;
		blocked.abilityId = sas::ContentId{ "Ability.Test.Blocked" };
		blocked.activationOrigin = sas::AbilityActivationOrigin::NormalInput;
		if (!dispatcher.CanActivate(allowed) ||
			dispatcher.CanActivate(blocked) ||
			guardCalls != 2)
		{
			return Fail("Ability activation guard did not veto only the blocked event");
		}
		dispatcher.Publish(allowed);
		dispatcher.Publish(blocked);
		if (observed != 2 ||
			!dispatcher.UnregisterObserver(observer) ||
			!dispatcher.UnregisterActivationGuard(guard))
		{
			return Fail("Ability lifecycle observer registration or dispatch failed");
		}
	}

	if (!GameContentBootstrap::Register())
	{
		return Fail("Game ability-system content could not be registered");
	}
	std::string effectValidationFailure;
	const List<const sas::GameplayEffectDefinition*>& shippedEffects =
		EffectData::GetShippedGameplayEffectDefinitions();
	const bool shippedEffectsValid = GameplayEffectDefinitionValidator::ValidateCatalog(
		EffectData::GetShippedGameplayEffectDefinitions(),
		IsEffectBehaviorRegisteredForValidation,
		&effectValidationFailure
	);
	const auto* barrierDefinition = EffectData::FindGameplayEffectDefinition("Effect.Barrier.Basic");
	const auto* gravityInsideDefinition = EffectData::FindGameplayEffectDefinition(
		AbilityData::GravityAnomaly::Effect::InsideEffectId
	);
	const auto* nullPulseStunDefinition = EffectData::FindGameplayEffectDefinition(
		AbilityData::NullPulse::Effect::StunId
	);
	const auto* nullPulseStaggerDefinition = EffectData::FindGameplayEffectDefinition(
		AbilityData::NullPulse::Effect::StaggerId
	);
	const auto* directionalBarrierDefinition = EffectData::FindGameplayEffectDefinition(
		AbilityData::DirectionalBarrier::Effect::ActiveEffectId
	);
	const auto* missingDefinition = EffectData::FindGameplayEffectDefinition("Effect.Does.NotExist");
	if (shippedEffects.size() != 19 ||
		!shippedEffectsValid ||
		!barrierDefinition ||
		barrierDefinition->effectId != "Effect.Barrier.Basic" ||
		!barrierDefinition->sourceParameterized ||
		!NearlyEqual(barrierDefinition->duration, 0.f) ||
		!barrierDefinition->attributes.empty() ||
		!gravityInsideDefinition ||
		gravityInsideDefinition->effectId !=
			AbilityData::GravityAnomaly::Effect::InsideEffectId ||
		!nullPulseStunDefinition ||
		nullPulseStunDefinition->effectId != AbilityData::NullPulse::Effect::StunId ||
		!nullPulseStunDefinition->sourceParameterized ||
		!nullPulseStaggerDefinition ||
		nullPulseStaggerDefinition->effectId != AbilityData::NullPulse::Effect::StaggerId ||
		!nullPulseStaggerDefinition->sourceParameterized ||
		!directionalBarrierDefinition ||
		directionalBarrierDefinition->effectId !=
			AbilityData::DirectionalBarrier::Effect::ActiveEffectId ||
		!(directionalBarrierDefinition->behaviorKey ==
			EffectData::DirectionalBarrierBehaviorKey) ||
		directionalBarrierDefinition->grantedTags.size() != 1 ||
		directionalBarrierDefinition->grantedTags.front() !=
			AbilityData::DirectionalBarrier::State::Active ||
		!directionalBarrierDefinition->sourceParameterized ||
		missingDefinition != nullptr)
	{
		return Fail("Central gameplay-effect catalog lookup or validation failed");
	}

	// Toggle abilities share one lifecycle rule: a second press cannot end
	// the ability until the common one-second minimum active duration elapses.
	{
		sas::AbilityRuntimeState toggleState{ 1 };
		toggleState.SetInputHeld(true);
		const sas::AbilityLifecycleDecision activation =
			sas::AbilityLifecycleOrchestrator::EvaluateInput(
				sas::AbilityActivationPolicy::Toggle,
				sas::AbilityLifetimePolicy::Duration,
				toggleState
			);
		if (!activation.requestActivation)
		{
			return Fail("Toggle ability did not request activation on its first press");
		}

		toggleState.BeginActivation(5.f, 1);
		toggleState.CommitInputFrame();
		toggleState.SetInputHeld(false);
		toggleState.CommitInputFrame();
		toggleState.SetInputHeld(true);
		const sas::AbilityLifecycleDecision earlyDeactivation =
			sas::AbilityLifecycleOrchestrator::EvaluateInput(
				sas::AbilityActivationPolicy::Toggle,
				sas::AbilityLifetimePolicy::Duration,
				toggleState
			);
		if (earlyDeactivation.requestEnd)
		{
			return Fail("Toggle ability deactivated before the common one-second minimum");
		}

		toggleState.TickActiveTime(1.01f);
		const sas::AbilityLifecycleDecision allowedDeactivation =
			sas::AbilityLifecycleOrchestrator::EvaluateInput(
				sas::AbilityActivationPolicy::Toggle,
				sas::AbilityLifetimePolicy::Duration,
				toggleState
			);
		if (!allowedDeactivation.requestEnd)
		{
			return Fail("Toggle ability did not deactivate after the common one-second minimum");
		}
	}

	const GameAbilityDefinition* nullPulseDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::NullPulse::AbilityId::Basic);
	if (!nullPulseDefinition || nullPulseDefinition->behaviorType != AbilityBehaviorType::NullPulse)
	{
		return Fail("Null Pulse shipped definition was not registered with its behavior");
	}
	const GameAbilityDefinition* phaseDriftDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::PhaseDrift::AbilityId::Basic);
	const GameAbilityDefinition* orbitalDronesDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::OrbitalDrones::AbilityId::Basic);
	const GameAbilityDefinition* executionDriveDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::ExecutionDrive::AbilityId::Basic);
	const GameAbilityDefinition* echoProtocolDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::EchoProtocol::AbilityId::Basic);
	const GameAbilityDefinition* lanceDriveDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::LanceDrive::AbilityId::Basic);
	std::string abilityOwnershipFailure;
	if (!phaseDriftDefinition || !orbitalDronesDefinition || !executionDriveDefinition ||
		!echoProtocolDefinition || !lanceDriveDefinition ||
		!ValidateAbilityCatalog(AbilityData::GetShippedAbilityDefinitions(), &abilityOwnershipFailure))
	{
		return Fail("Shipped ability catalog failed family-owned attribute validation");
	}
	if (lanceDriveDefinition->behaviorType != AbilityBehaviorType::LanceDrive ||
		lanceDriveDefinition->lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
		lanceDriveDefinition->activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
		!NearlyEqual(lanceDriveDefinition->duration, 6.f) ||
		!NearlyEqual(lanceDriveDefinition->cooldown, 15.f) ||
		lanceDriveDefinition->attributes.size() != 12 ||
		lanceDriveDefinition->damageTags != List<GameplayTag>{ DamageTypeSchema::Kinetic } ||
		!AbilityData::FindAbilityActorDefinition(
			AbilityData::LanceDrive::Actor::Lance::BasicDefinitionId
		) ||
		!PresentationProfileRegistry<LanceDrivePresentationProfile>::Find(
			LanceDrivePresentationIds::LanceBasic.ToString()
		))
	{
		return Fail("Lance Drive shipped behavior, actor, or presentation registration is incomplete");
	}
	if (orbitalDronesDefinition->slot != sas::AbilitySlot::Ability4 ||
		orbitalDronesDefinition->activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
		orbitalDronesDefinition->lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
		!NearlyEqual(orbitalDronesDefinition->cooldown, 12.f) ||
		!NearlyEqual(orbitalDronesDefinition->duration, 6.f) ||
		orbitalDronesDefinition->maxCharges != 1 ||
		orbitalDronesDefinition->behaviorType != AbilityBehaviorType::OrbitalDrones ||
		orbitalDronesDefinition->abilityTags !=
			List<GameplayTag>{
				GameplayTags::Ability::Offense,
				GameplayTags::Ability::Family::OrbitalDrones
			} ||
		orbitalDronesDefinition->damageTags !=
			List<GameplayTag>{ DamageTypeSchema::Kinetic } ||
		orbitalDronesDefinition->attributes.size() != 8 ||
		sas::FindAttributeValue(
			orbitalDronesDefinition->attributes,
			CommonAttributeIds::Radius,
			0.f
		) <= 0.f ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesDefinition->attributes,
				CommonAttributeIds::Damage,
				0.f
			),
			18.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesDefinition->attributes,
				AbilityData::OrbitalDrones::Attribute::DroneCount,
				0.f
			),
			4.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesDefinition->attributes,
				AbilityData::OrbitalDrones::Attribute::SameTargetHitCooldown,
				0.f
			),
			0.5f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesDefinition->attributes,
				AbilityData::OrbitalDrones::Attribute::BaseAngularSpeedRadiansPerSecond,
				0.f
			),
			2.5f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesDefinition->attributes,
				AbilityData::OrbitalDrones::Attribute::ContactRadius,
				0.f
			),
			12.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesDefinition->attributes,
				AbilityData::OrbitalDrones::Attribute::EnergyPowerReference,
				0.f
			),
			50.f
		) ||
		!NearlyEqual(
			sas::FindAttributeValue(
				orbitalDronesDefinition->attributes,
				AbilityData::OrbitalDrones::Attribute::EnergyPowerDurationScale,
				0.f
			),
			0.02f
		) ||
		orbitalDronesDefinition->scalingRules.size() != 1 ||
		orbitalDronesDefinition->scalingRules.front().targetAttributeId != CommonAttributeIds::Damage ||
		orbitalDronesDefinition->scalingRules.front().sourceAttributeId != OwnerAttributeIds::AttackPower ||
		orbitalDronesDefinition->scalingRules.front().operation != sas::AttributeModifierOperation::Add ||
		!NearlyEqual(orbitalDronesDefinition->scalingRules.front().coefficient, 0.50f) ||
		orbitalDronesDefinition->levelProgression.size() != 14 ||
		orbitalDronesDefinition->levelUpgradeScrapCosts.size() != 14)
	{
		return Fail("Orbital Drones shipped ability contract is invalid");
	}
	if (executionDriveDefinition->behaviorType != AbilityBehaviorType::ExecutionDrive ||
		executionDriveDefinition->activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
		executionDriveDefinition->lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
		executionDriveDefinition->maxCharges != 1 ||
		!NearlyEqual(executionDriveDefinition->cooldown, 14.f) ||
		!NearlyEqual(executionDriveDefinition->duration, 5.f) ||
		executionDriveDefinition->abilityTags !=
			List<GameplayTag>{
				GameplayTags::Ability::Offense,
				GameplayTags::Ability::Family::ExecutionDrive
			} ||
		executionDriveDefinition->attributes.size() != 7 ||
		!EffectData::FindGameplayEffectDefinition(
			AbilityData::ExecutionDrive::Effect::AttackPowerId
		))
	{
		return Fail("Execution Drive shipped ability contract is invalid");
	}
	if (echoProtocolDefinition->slot != sas::AbilitySlot::Ability3 ||
		echoProtocolDefinition->activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
		echoProtocolDefinition->lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
		echoProtocolDefinition->cooldown <= 0.f ||
		echoProtocolDefinition->maxCharges != 1 ||
		echoProtocolDefinition->behaviorType != AbilityBehaviorType::EchoProtocol ||
		echoProtocolDefinition->recordInAbilityHistory ||
		echoProtocolDefinition->abilityTags !=
			List<GameplayTag>{
				GameplayTags::Ability::Utility,
				GameplayTags::Ability::Family::EchoProtocol
			} ||
		echoProtocolDefinition->attributes.size() != 8 ||
		echoProtocolDefinition->levelProgression.size() != 14 ||
		echoProtocolDefinition->levelUpgradeScrapCosts.size() != 14)
	{
		return Fail("Echo Protocol shipped ability contract is invalid");
	}
	for (const AbilityLevelStep& step : orbitalDronesDefinition->levelProgression)
	{
		bool hasDamageUpgrade = false;
		bool hasCooldownUpgrade = false;
		if (step.attributeModifiers.size() != 2)
		{
			return Fail("Orbital Drones shipped progression does not contain two modifiers per level");
		}
		for (const sas::AttributeModifier& modifier : step.attributeModifiers)
		{
			if (modifier.attributeId == CommonAttributeIds::Damage &&
				modifier.operation == sas::AttributeModifierOperation::Add &&
				NearlyEqual(modifier.magnitude, 2.f))
			{
				hasDamageUpgrade = true;
			}
			if (modifier.attributeId == CommonAttributeIds::Cooldown &&
				modifier.operation == sas::AttributeModifierOperation::Add &&
				NearlyEqual(modifier.magnitude, -0.25f))
			{
				hasCooldownUpgrade = true;
			}
		}
		if (!hasDamageUpgrade || !hasCooldownUpgrade)
		{
			return Fail("Orbital Drones shipped progression has an unexpected modifier");
		}
	}
	GameAbilityDefinition configuredFamilyFixture;
	configuredFamilyFixture.abilityId = "Ability.Utility.ConfiguredFixture.Basic";
	configuredFamilyFixture.abilityTags = { GameplayTagSchema::AbilityUtility };
	configuredFamilyFixture.attributes = {
		sas::GameplayAttribute{
			sas::AttributeId{ "Ability.Utility.ConfiguredFixture.OwnedValue" },
			1.f,
			0.f
		}
	};
	if (!ValidateAbilityDefinition(configuredFamilyFixture, &abilityOwnershipFailure))
	{
		return Fail("Configured non-primary ability did not derive its attribute family from the ID");
	}
	configuredFamilyFixture.attributes.emplace_back(
		sas::AttributeId{ "Ability.Utility.ConfiguredFixtureOther.ForeignValue" },
		1.f,
		0.f
	);
	if (ValidateAbilityDefinition(configuredFamilyFixture, &abilityOwnershipFailure))
	{
		return Fail("Configured non-primary ability accepted a non-ID attribute family");
	}

	// A concrete ability may use shared Common.* values and its own exact
	// family namespace. Its Ability.* targets must also be declared locally;
	// other attribute domains remain governed by their consumer contracts.
	GameAbilityDefinition ownedPhaseDrift = *phaseDriftDefinition;
	ownedPhaseDrift.attributes.emplace_back(
		sas::AttributeId{ "Ability.Movement.PhaseDrift.TestOwnedValue" },
		1.f,
		0.f
	);
	ownedPhaseDrift.attributeModifiers = {
		sas::AttributeModifier{
			AbilityData::PhaseDrift::Attribute::MovementSpeedBonus,
			0.01f
		},
		sas::AttributeModifier{ sas::AttributeId{ "Effect.Test.Parameter" }, 1.f },
		sas::AttributeModifier{
			sas::AttributeId{ "AbilityActor.PhaseDrift.VisualIntensity" },
			1.f
		}
	};
	ownedPhaseDrift.scalingRules = {
		sas::AttributeScalingRule{
			AbilityData::PhaseDrift::Attribute::MovementSpeedBonus,
			sas::AttributeId{ "Owner.Test.Mobility" },
			sas::AttributeModifierOperation::Add,
			1.f
		}
	};
	ownedPhaseDrift.levelProgression.front().attributeModifiers.emplace_back(
		AbilityData::PhaseDrift::Attribute::ShieldRegenBonus,
		0.01f
	);
	if (!ValidateAbilityDefinition(ownedPhaseDrift, &abilityOwnershipFailure))
	{
		return Fail("Ability validator rejected valid family-owned attribute targets");
	}
	GameAbilityDefinition commonTargetPhaseDrift = *phaseDriftDefinition;
	const sas::AttributeId commonTestAttribute{ "Common.Test.AbilityValidator" };
	commonTargetPhaseDrift.attributes.emplace_back(commonTestAttribute, 1.f, 0.f);
	commonTargetPhaseDrift.attributeModifiers.emplace_back(commonTestAttribute, 0.01f);
	commonTargetPhaseDrift.scalingRules.emplace_back(sas::AttributeScalingRule{
		commonTestAttribute,
		sas::AttributeId{ "Owner.Test.Mobility" },
		sas::AttributeModifierOperation::Add,
		1.f
	});
	commonTargetPhaseDrift.levelProgression.front().attributeModifiers.emplace_back(
		commonTestAttribute,
		0.01f
	);
	if (!ValidateAbilityDefinition(commonTargetPhaseDrift, &abilityOwnershipFailure))
	{
		return Fail("Ability validator rejected Common.* modifier, scaling, or level targets");
	}

	GameAbilityDefinition foreignAbilityAttribute = *phaseDriftDefinition;
	foreignAbilityAttribute.attributes.emplace_back(
		sas::AttributeId{ "Ability.Offense.OverdriveCore.ForeignValue" },
		1.f,
		0.f
	);
	if (ValidateAbilityDefinition(foreignAbilityAttribute, &abilityOwnershipFailure))
	{
		return Fail("Ability validator accepted a foreign-family base attribute");
	}
	GameAbilityDefinition similarPrefixAbilityAttribute = *phaseDriftDefinition;
	similarPrefixAbilityAttribute.attributes.emplace_back(
		sas::AttributeId{ "Ability.Movement.PhaseDriftExtended.ForeignValue" },
		1.f,
		0.f
	);
	if (ValidateAbilityDefinition(similarPrefixAbilityAttribute, &abilityOwnershipFailure))
	{
		return Fail("Ability validator accepted a similar-prefix base attribute");
	}

	GameAbilityDefinition foreignDefinitionModifier = *phaseDriftDefinition;
	foreignDefinitionModifier.attributeModifiers.emplace_back(
		sas::AttributeId{ "Ability.Offense.OverdriveCore.ForeignValue" },
		1.f
	);
	if (ValidateAbilityDefinition(foreignDefinitionModifier, &abilityOwnershipFailure))
	{
		return Fail("Ability validator accepted a foreign-family definition modifier");
	}
	GameAbilityDefinition undeclaredDefinitionModifier = *phaseDriftDefinition;
	undeclaredDefinitionModifier.attributeModifiers.emplace_back(
		sas::AttributeId{ "Ability.Movement.PhaseDrift.UndeclaredValue" },
		1.f
	);
	if (ValidateAbilityDefinition(undeclaredDefinitionModifier, &abilityOwnershipFailure))
	{
		return Fail("Ability validator accepted an undeclared family definition modifier");
	}

	GameAbilityDefinition foreignScalingTarget = *phaseDriftDefinition;
	foreignScalingTarget.scalingRules.emplace_back(sas::AttributeScalingRule{
		sas::AttributeId{ "Ability.Offense.OverdriveCore.ForeignValue" },
		sas::AttributeId{ "Owner.Test.Mobility" },
		sas::AttributeModifierOperation::Add,
		1.f
	});
	if (ValidateAbilityDefinition(foreignScalingTarget, &abilityOwnershipFailure))
	{
		return Fail("Ability validator accepted a foreign-family scaling target");
	}
	GameAbilityDefinition undeclaredScalingTarget = *phaseDriftDefinition;
	undeclaredScalingTarget.scalingRules.emplace_back(sas::AttributeScalingRule{
		sas::AttributeId{ "Ability.Movement.PhaseDrift.UndeclaredValue" },
		sas::AttributeId{ "Owner.Test.Mobility" },
		sas::AttributeModifierOperation::Add,
		1.f
	});
	if (ValidateAbilityDefinition(undeclaredScalingTarget, &abilityOwnershipFailure))
	{
		return Fail("Ability validator accepted an undeclared family scaling target");
	}

	GameAbilityDefinition foreignLevelModifier = *phaseDriftDefinition;
	foreignLevelModifier.levelProgression.front().attributeModifiers.emplace_back(
		sas::AttributeId{ "Ability.Offense.OverdriveCore.ForeignValue" },
		1.f
	);
	if (ValidateAbilityDefinition(foreignLevelModifier, &abilityOwnershipFailure))
	{
		return Fail("Ability validator accepted a foreign-family level modifier");
	}
	GameAbilityDefinition undeclaredLevelModifier = *phaseDriftDefinition;
	undeclaredLevelModifier.levelProgression.front().attributeModifiers.emplace_back(
		sas::AttributeId{ "Ability.Movement.PhaseDrift.UndeclaredValue" },
		1.f
	);
	if (ValidateAbilityDefinition(undeclaredLevelModifier, &abilityOwnershipFailure))
	{
		return Fail("Ability validator accepted an undeclared family level modifier");
	}

	World nullPulseWorld{ nullptr };
	const shared_ptr<TestCombatant> nullPulseOwner =
		nullPulseWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestCombatant> nullPulseTarget =
		nullPulseWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestCombatant> nullPulseOutsideTarget =
		nullPulseWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestClearableProjectile> nullPulseProjectile =
		nullPulseWorld.SpawnActor<TestClearableProjectile>(nullPulseOwner.get()).lock();
	const shared_ptr<TestPersistentAbilityActor> nullPulsePersistentActor =
		nullPulseWorld.SpawnActor<TestPersistentAbilityActor>(nullPulseOwner.get()).lock();
	if (!nullPulseOwner || !nullPulseTarget || !nullPulseOutsideTarget ||
		!nullPulseProjectile || !nullPulsePersistentActor)
	{
		return Fail("Null Pulse runtime test actors could not spawn");
	}

	nullPulseOwner->GetCombatRuntime().InitializeOwnerAttributes(1000.f);
	nullPulseOwner->GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::EnergyPower, 100.f }
	);
	nullPulseOwner->SetCollisionLayer(CollisionLayer::Player);
	nullPulseOwner->SetCollisionMask(CollisionLayer::Enemy);
	nullPulseTarget->SetCollisionLayer(CollisionLayer::Enemy);
	nullPulseTarget->SetCollisionMask(CollisionLayer::Player);
	nullPulseOutsideTarget->SetCollisionLayer(CollisionLayer::Enemy);
	nullPulseOutsideTarget->SetCollisionMask(CollisionLayer::Player);
	nullPulseTarget->SetActorLocation({ 120.f, 0.f });
	// Keep this assertion target outside the updated 500 radius.
	nullPulseOutsideTarget->SetActorLocation({ 700.f, 0.f });
	nullPulseProjectile->SetActorLocation({ 90.f, 0.f });
	nullPulsePersistentActor->SetActorLocation({ 100.f, 0.f });
	nullPulseWorld.TickInternal(0.f);

	const sas::AbilityHandle nullPulseHandle =
		nullPulseOwner->GetAbilitySystemComponent().GrantAbility(*nullPulseDefinition);
	if (!nullPulseHandle.IsValid())
	{
		return Fail("Null Pulse could not be granted to the control slot");
	}
	nullPulseOwner->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability2,
		true
	);
	nullPulseOwner->GetAbilitySystemComponent().Tick(0.f);
	nullPulseWorld.TickInternal(0.f);
	const auto& nullPulseTargetTags =
		nullPulseTarget->GetAbilitySystemComponent().GetOwnedTags();
	if (!NearlyEqual(nullPulseTarget->GetHealth(), 990.f) ||
		!nullPulseTargetTags.HasTag(GameplayTags::State::Effect::Control::Stunned) ||
		nullPulseOutsideTarget->GetHealth() != 1000.f ||
		nullPulseOutsideTarget->GetAbilitySystemComponent().GetOwnedTags().HasTag(
			GameplayTags::State::Effect::Control::Stunned
		) ||
		!nullPulseWorld.GetActorsByType<NullPulseVisualActor>().size() ||
		!nullPulseWorld.GetActorsByType<TestClearableProjectile>().empty() ||
		nullPulseWorld.GetActorsByType<TestPersistentAbilityActor>().empty())
	{
		return Fail("Null Pulse did not damage, control, visualize, and filter its pulse targets correctly");
	}
	nullPulseWorld.TickInternal(0.3f);
	if (!nullPulseWorld.GetActorsByType<NullPulseVisualActor>().empty())
	{
		return Fail("Null Pulse pulse visual did not clean itself up");
	}

	sas::GameplayEffectSpec firstSlowSpec =
		sas::MakeGameplayEffectSpec(EffectData::CryoSlowEffect);
	sas::GameplayEffectSpec secondSlowSpec =
		sas::MakeGameplayEffectSpec(EffectData::CryoSlowEffect);
	SetGameplayEffectModifierMagnitude(
		firstSlowSpec,
		OwnerAttributeIds::MovementSlow,
		0.10f
	);
	SetGameplayEffectModifierMagnitude(
		secondSlowSpec,
		OwnerAttributeIds::MovementSlow,
		0.30f
	);
	firstSlowSpec.duration = 0.5f;
	const float catalogSlowMagnitude =
		EffectData::CryoSlowEffect.modifiers.empty()
			? -1.f
			: EffectData::CryoSlowEffect.modifiers.front().magnitude;
	if (!NearlyEqual(catalogSlowMagnitude, 0.25f) ||
		!NearlyEqual(firstSlowSpec.modifiers.front().magnitude, 0.10f) ||
		!NearlyEqual(secondSlowSpec.modifiers.front().magnitude, 0.30f) ||
		!NearlyEqual(firstSlowSpec.duration, 0.5f) ||
		!NearlyEqual(secondSlowSpec.duration, EffectData::CryoSlowEffect.duration))
	{
		return Fail("Resolved gameplay-effect specs mutated shared content or leaked between sources");
	}

	sas::GameplayEffectDefinition invalidEffect = EffectData::CryoSlowEffect;
	invalidEffect.effectId = "Effect.Test.InvalidBehavior";
	invalidEffect.behaviorKey = sas::GameplayEffectBehaviorKey{ "EffectBehavior.NotRegistered" };
	effectValidationFailure.clear();
	if (ValidateEffectDefinition(invalidEffect, &effectValidationFailure) ||
		effectValidationFailure.empty())
	{
		return Fail("Gameplay-effect validation accepted an unregistered behavior");
	}
	invalidEffect = EffectData::CryoSlowEffect;
	invalidEffect.effectId = "Effect.Test.InvalidVisual";
	invalidEffect.activeVisualId = "Visual.Effect.NotRegistered";
	effectValidationFailure.clear();
	if (ValidateEffectDefinition(invalidEffect, &effectValidationFailure) ||
		effectValidationFailure.empty())
	{
		return Fail("Gameplay-effect validation accepted an unregistered visual");
	}
	invalidEffect = EffectData::CryoSlowEffect;
	invalidEffect.grantedTags = { GameplayTag{ "Ability.Offense.Rocket" } };
	effectValidationFailure.clear();
	if (ValidateEffectDefinition(invalidEffect, &effectValidationFailure) ||
		effectValidationFailure.empty())
	{
		return Fail("Gameplay-effect validation accepted a granted tag from the ability domain");
	}

	const RocketPresentationProfile* rocketPresentationProfile =
		PresentationProfileRegistry<RocketPresentationProfile>::Find(
			RocketPresentationIds::ProjectileBasic
		);
	const SunBeamPresentationProfile* sunBeamPresentationProfile =
		PresentationProfileRegistry<SunBeamPresentationProfile>::Find(
			SunBeamPresentationIds::StrikeBasic
		);
	const GravityAnomalyProjectilePresentationProfile* gravityProjectilePresentationProfile =
		PresentationProfileRegistry<GravityAnomalyProjectilePresentationProfile>::Find(
			GravityAnomalyPresentationIds::ProjectileBasic
		);
	const GravityAnomalyFieldPresentationProfile* gravityFieldPresentationProfile =
		PresentationProfileRegistry<GravityAnomalyFieldPresentationProfile>::Find(
			GravityAnomalyPresentationIds::FieldBasic
		);
	const OrbitalDronesPresentationProfile* orbitalDronesPresentationProfile =
		PresentationProfileRegistry<OrbitalDronesPresentationProfile>::Find(
			OrbitalDronesPresentationIds::DroneBasic
		);
	const DirectionalBarrierPresentationProfile* directionalBarrierPresentationProfile =
		PresentationProfileRegistry<DirectionalBarrierPresentationProfile>::Find(
			DirectionalBarrierPresentationIds::Basic
		);
	if (!rocketPresentationProfile
		|| !sunBeamPresentationProfile
		|| !gravityProjectilePresentationProfile
		|| !gravityFieldPresentationProfile
		|| !orbitalDronesPresentationProfile
		|| !directionalBarrierPresentationProfile
		|| rocketPresentationProfile->telegraph.outlineThickness <= 0.f
		|| rocketPresentationProfile->visual.impactVisualDuration <= 0.f
		|| sunBeamPresentationProfile->visual.impactFlashDuration <= 0.f
		|| orbitalDronesPresentationProfile->profileId.ToString() !=
			OrbitalDronesPresentationIds::DroneBasic
		|| orbitalDronesPresentationProfile->visual.bodyRadius <= 0.f
		|| orbitalDronesPresentationProfile->visual.trailSegments <= 0
		|| orbitalDronesPresentationProfile->visual.expiryFadeDuration <= 0.f
		|| directionalBarrierPresentationProfile->radius <= 0.f
		|| directionalBarrierPresentationProfile->halfAngleDegrees <= 0.f
		|| directionalBarrierPresentationProfile->edgeThickness <= 0.f)
	{
		return Fail("Feature-local ability presentation profiles were not registered correctly");
	}
	if (PresentationProfileRegistry<RocketPresentationProfile>::Find(
			SunBeamPresentationIds::StrikeBasic
		) != nullptr
		|| PresentationProfileRegistry<SunBeamPresentationProfile>::Find(
			RocketPresentationIds::ProjectileBasic
		) != nullptr
		|| PresentationProfileRegistry<GravityAnomalyProjectilePresentationProfile>::Find(
			GravityAnomalyPresentationIds::FieldBasic
		) != nullptr
		|| PresentationProfileRegistry<GravityAnomalyFieldPresentationProfile>::Find(
			GravityAnomalyPresentationIds::ProjectileBasic
		) != nullptr
		|| PresentationProfileRegistry<OrbitalDronesPresentationProfile>::Find(
			RocketPresentationIds::ProjectileBasic
		) != nullptr
		|| PresentationProfileRegistry<RocketPresentationProfile>::Find(
			OrbitalDronesPresentationIds::DroneBasic
		) != nullptr)
	{
		return Fail("Typed presentation profile registries leaked profiles across ability families");
	}
	if (PresentationProfileRegistry<RocketPresentationProfile>::Register(
		*rocketPresentationProfile
	))
	{
		return Fail("Presentation profile registry accepted a duplicate profile id");
	}
	// A family-local registry must fail closed when a drone variant has not been
	// registered; the behavior relies on this lookup before it spawns anything.
	if (PresentationProfileRegistry<OrbitalDronesPresentationProfile>::Find(
		"Presentation.Ability.OrbitalDrones.Drone.Missing"
	) != nullptr)
	{
		return Fail("Orbital Drones accepted a missing presentation profile");
	}

	const EmberSwarmPresentationProfile* emberSwarmPresentationProfile =
		PresentationProfileRegistry<EmberSwarmPresentationProfile>::Find(
			EmberSwarmPresentationIds::DroneBasic
		);
	if (!emberSwarmPresentationProfile
		|| emberSwarmPresentationProfile->profileId.ToString() !=
			EmberSwarmPresentationIds::DroneBasic
		|| emberSwarmPresentationProfile->visual.bodyRadius <= 0.f
		|| emberSwarmPresentationProfile->visual.coreRadius <= 0.f
		|| emberSwarmPresentationProfile->visual.glowRadius <= 0.f
		|| emberSwarmPresentationProfile->visual.expiryFadeDuration <= 0.f)
	{
		return Fail("Ember Swarm presentation profile was not registered correctly");
	}
	if (PresentationProfileRegistry<EmberSwarmPresentationProfile>::Find(
			OrbitalDronesPresentationIds::DroneBasic
		) != nullptr
		|| PresentationProfileRegistry<OrbitalDronesPresentationProfile>::Find(
			EmberSwarmPresentationIds::DroneBasic
		) != nullptr)
	{
		return Fail("Typed presentation profile registries leaked profiles between Ember Swarm and Orbital Drones");
	}
	if (PresentationProfileRegistry<EmberSwarmPresentationProfile>::Find(
		"Presentation.Ability.EmberSwarm.Drone.Missing"
	) != nullptr)
	{
		return Fail("Ember Swarm accepted a missing presentation profile");
	}

	// Directional Barrier is a real Toggle ability, so exercise its complete
	// activation/lifecycle path in a live player world. The second press is
	// intentionally attempted twice: the first one is rejected before one
	// second, the second one is accepted after the shared minimum has elapsed.
	World directionalBarrierWorld{ nullptr };
	const shared_ptr<SpaceShip> directionalBarrierOwner =
		directionalBarrierWorld.SpawnActor<SpaceShip>(
		ShipData::Ship_Player_Fighter
	).lock();
	if (!directionalBarrierOwner)
	{
		return Fail("Directional Barrier could not spawn a live player owner");
	}
	directionalBarrierWorld.TickInternal(0.f);
	directionalBarrierOwner->GetAbilitySystemComponent().ClearAbilitySlot(
		sas::AbilitySlot::Ability1
	);
	const GameAbilityDefinition* directionalBarrierAbilityDefinition =
		AbilityData::FindShippedAbilityDefinition(
			AbilityData::DirectionalBarrier::AbilityId::Basic
		);
	if (!directionalBarrierAbilityDefinition)
	{
		return Fail("Directional Barrier shipped definition could not be found");
	}
	const sas::AbilityHandle directionalBarrierHandle =
		directionalBarrierOwner->GetAbilitySystemComponent().GrantAbility(
			*directionalBarrierAbilityDefinition
		);
	directionalBarrierOwner->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability1,
		true
	);
	directionalBarrierWorld.TickInternal(0.f);
	directionalBarrierWorld.TickInternal(0.f);
	GameAbility* directionalBarrierAbility =
		directionalBarrierOwner->GetAbilitySystemComponent().GetAbility(
			directionalBarrierHandle
		);
	if (!directionalBarrierHandle.IsValid())
	{
		return Fail("Directional Barrier could not be granted to Ability1");
	}
	if (!directionalBarrierAbility)
	{
		return Fail("Directional Barrier handle could not resolve its GameAbility instance");
	}
	if (!directionalBarrierAbility->IsActive())
	{
		return Fail("Directional Barrier did not become active after its first press");
	}
	if (!directionalBarrierOwner->GetAbilitySystemComponent().HasOwnedTag(
		AbilityData::DirectionalBarrier::State::Active
	))
	{
		return Fail("Directional Barrier active effect did not grant its canonical state tag");
	}
	const shared_ptr<TestClearableProjectile> incomingBarrierProjectile =
		directionalBarrierWorld.SpawnActor<TestClearableProjectile>(nullptr).lock();
	if (!incomingBarrierProjectile)
	{
		return Fail("Directional Barrier projectile interception test actor could not spawn");
	}
	directionalBarrierWorld.TickInternal(0.f);
	directionalBarrierOwner->SetCollisionLayer(CollisionLayer::Player);
	directionalBarrierOwner->SetCollisionMask(
		CollisionLayer::Enemy | CollisionLayer::EnemyBullet
	);
	incomingBarrierProjectile->SetCollisionLayer(CollisionLayer::EnemyBullet);
	incomingBarrierProjectile->SetCollisionMask(CollisionLayer::Player);
	incomingBarrierProjectile->SetAbilityCollisionRadius(5.f);
	incomingBarrierProjectile->SetActorLocation({ 0.f, -90.f });
	incomingBarrierProjectile->SetVelocity({ 0.f, 200.f });
	if (!DirectionalBarrierEffectBehavior::TryInterceptProjectile(
		*incomingBarrierProjectile,
		{ 0.f, -140.f }
	))
	{
		return Fail("Directional Barrier did not intercept a projectile at its outer edge");
	}
	incomingBarrierProjectile->Destroy();
	if (directionalBarrierOwner->GetMovementSpeedMultiplier() >= 1.f)
	{
		return Fail("Directional Barrier did not apply its movement modifier");
	}
	if (directionalBarrierWorld.GetActorsByType<DirectionalBarrierVisualActor>().size() != 1)
	{
		return Fail("Directional Barrier did not spawn its visual actor");
	}
	directionalBarrierOwner->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability1,
		false
	);
	directionalBarrierWorld.TickInternal(0.f);
	directionalBarrierOwner->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability1,
		true
	);
	directionalBarrierWorld.TickInternal(0.1f);
	if (!directionalBarrierAbility->IsActive())
	{
		return Fail("Directional Barrier deactivated before its one-second minimum");
	}
	directionalBarrierWorld.TickInternal(1.0f);
	directionalBarrierOwner->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability1,
		false
	);
	directionalBarrierWorld.TickInternal(0.f);
	directionalBarrierOwner->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability1,
		true
	);
	directionalBarrierWorld.TickInternal(0.f);
	directionalBarrierWorld.TickInternal(0.f);
	if (directionalBarrierAbility->IsActive())
	{
		return Fail("Directional Barrier remained active after the valid second press");
	}
	if (directionalBarrierOwner->GetAbilitySystemComponent().HasOwnedTag(
		AbilityData::DirectionalBarrier::State::Active
	))
	{
		return Fail("Directional Barrier retained its active effect tag after ending");
	}
	if (directionalBarrierOwner->GetMovementSpeedMultiplier() < 1.f)
	{
		return Fail("Directional Barrier retained its movement modifier after ending");
	}
	if (!directionalBarrierWorld.GetActorsByType<DirectionalBarrierVisualActor>().empty())
	{
		return Fail("Directional Barrier did not clean up its visual actor after ending");
	}

	// The effect behavior consumes only projectile deliveries approaching the
	// ship's forward hemisphere. Area, beam, contact, and rear deliveries remain
	// available to the normal combat pipeline.
	TestCombatant directionalBarrierDamageTarget;
	sas::GameplayEffectSpec directionalBarrierDamageSpec =
		sas::MakeGameplayEffectSpec(EffectData::DirectionalBarrierActiveEffect);
	directionalBarrierDamageSpec.duration = 5.f;
	directionalBarrierDamageTarget.GetAbilitySystemComponent().ApplyGameplayEffect(
		directionalBarrierDamageSpec
	);
	TestClearableProjectile directionalBarrierProjectile{ nullptr, nullptr };
	directionalBarrierProjectile.SetActorLocation({ 0.f, -100.f });
	const float shieldedTargetHealth = directionalBarrierDamageTarget.GetHealth();
	ApplyCombatDamage(
		directionalBarrierDamageTarget,
		10.f,
		&directionalBarrierProjectile,
		{ DamageTypeSchema::Kinetic },
		{},
		sas::ContentId{},
		{},
		DamageDeliveryType::Projectile,
		&directionalBarrierProjectile
	);
	if (!NearlyEqual(directionalBarrierDamageTarget.GetHealth(), shieldedTargetHealth))
	{
		return Fail("Directional Barrier did not block a forward projectile delivery");
	}
	directionalBarrierProjectile.SetActorLocation({ 0.f, 100.f });
	ApplyCombatDamage(
		directionalBarrierDamageTarget,
		10.f,
		&directionalBarrierProjectile,
		{ DamageTypeSchema::Kinetic },
		{},
		sas::ContentId{},
		{},
		DamageDeliveryType::Projectile,
		&directionalBarrierProjectile
	);
	if (!(directionalBarrierDamageTarget.GetHealth() < shieldedTargetHealth))
	{
		return Fail("Directional Barrier incorrectly blocked a rear projectile delivery");
	}

	// The actor owns only formation motion and lifetime. This direct lifecycle
	// test keeps it independent from input/UI while proving four equal phases,
	// owner following, and automatic duration cleanup.
	World orbitalDronesWorld{ nullptr };
	const shared_ptr<TestCombatant> orbitalDronesOwner =
		orbitalDronesWorld.SpawnActor<TestCombatant>(1000.f).lock();
	if (!orbitalDronesOwner)
	{
		return Fail("Orbital Drones runtime owner could not be spawned");
	}
	orbitalDronesOwner->SetCollisionLayer(CollisionLayer::Player);
	orbitalDronesOwner->SetActorLocation({ 100.f, 200.f });
	for (std::size_t droneIndex = 0; droneIndex < 4; ++droneIndex)
	{
		const float phase = OrbitingDroneActor::CalculateFormationPhase(
			droneIndex,
			4
		);
		const shared_ptr<OrbitingDroneActor> drone =
			orbitalDronesWorld.SpawnActor<OrbitingDroneActor>(
				orbitalDronesOwner.get(),
				*orbitalDronesPresentationProfile,
				OrbitingDroneActor::OrbitConfiguration{ 200.f, 2.5f, phase }
			).lock();
		if (!drone || !NearlyEqual(drone->GetOrbitConfiguration().radius, 200.f) ||
			!NearlyEqual(drone->GetOrbitAngleRadians(), phase))
		{
			return Fail("Orbital Drones did not spawn a stable evenly phased formation");
		}
		drone->SetLifeTime(0.1f);
		drone->SetContactRadius(12.f);
	}
	orbitalDronesWorld.TickInternal(0.f);
	const List<weak_ptr<OrbitingDroneActor>> orbitalDrones =
		orbitalDronesWorld.GetActorsByType<OrbitingDroneActor>();
	if (orbitalDrones.size() != 4)
	{
		return Fail("Orbital Drones did not retain four spawned runtime actors");
	}
	for (std::size_t droneIndex = 0; droneIndex < orbitalDrones.size(); ++droneIndex)
	{
		const shared_ptr<OrbitingDroneActor> drone = orbitalDrones[droneIndex].lock();
		if (!drone || !NearlyEqual(
			drone->GetOrbitAngleRadians(),
			OrbitingDroneActor::CalculateFormationPhase(droneIndex, 4)
		))
		{
			return Fail("Orbital Drones formation phase was not preserved at spawn");
		}
	}
	const shared_ptr<OrbitingDroneActor> firstOrbitalDrone = orbitalDrones.front().lock();
	const float firstOrbitalAngle = firstOrbitalDrone
		? firstOrbitalDrone->GetOrbitAngleRadians()
		: 0.f;
	orbitalDronesWorld.TickInternal(0.05f);
	if (!firstOrbitalDrone || NearlyEqual(
			firstOrbitalDrone->GetOrbitAngleRadians(),
			firstOrbitalAngle
		))
	{
		return Fail("Orbital Drones did not advance their orbit angle during Tick");
	}
	orbitalDronesWorld.TickInternal(0.06f);
	if (!orbitalDronesWorld.GetActorsByType<OrbitingDroneActor>().empty())
	{
		return Fail("Orbital Drones actor lifetime did not clean up the formation");
	}

	AreaTelegraphVisualDefinition radialTelegraphDefinition;
	radialTelegraphDefinition.fillMode = AreaTelegraphFillMode::RadialProgress;
	radialTelegraphDefinition.radialGrowthLogStrength = 2.f;
	radialTelegraphDefinition.radialGrowthPrimaryPhaseEnd = 0.5f;
	radialTelegraphDefinition.radialGrowthPrimaryPhaseFill = 0.4f;
	if (!NearlyEqual(ResolveAreaTelegraphRadialProgress(radialTelegraphDefinition, 0.f), 0.f) ||
		!NearlyEqual(ResolveAreaTelegraphRadialProgress(radialTelegraphDefinition, 0.5f), 0.4f) ||
		!NearlyEqual(ResolveAreaTelegraphRadialProgress(radialTelegraphDefinition, 1.f), 1.f))
	{
		return Fail("Area telegraph radial resolver did not honor its phase boundaries");
	}

	AreaTelegraphVisualDefinition lifecycleTelegraphDefinition;
	lifecycleTelegraphDefinition.completionFeedbackDuration = 0.1f;
	{
		World timedTelegraphWorld{ nullptr };
		timedTelegraphWorld.SpawnActor<AreaTelegraphActor>(
			AreaTelegraphActor::SpawnParams{
				{ 0.f, 0.f },
				50.f,
				0.1f,
				lifecycleTelegraphDefinition,
				AreaTelegraphAnchorMode::FixedLocation,
				AreaTelegraphProgressDriver::Timed
			}
		);
		timedTelegraphWorld.TickInternal(0.2f);
		if (!timedTelegraphWorld.GetActorsByType<AreaTelegraphActor>().empty())
		{
			return Fail("Timed area telegraph did not clean itself up");
		}
	}
	{
		World externalTelegraphWorld{ nullptr };
		const shared_ptr<AreaTelegraphActor> externalTelegraph =
			externalTelegraphWorld.SpawnActor<AreaTelegraphActor>(
				AreaTelegraphActor::SpawnParams{
					{ 0.f, 0.f },
					50.f,
					0.1f,
					lifecycleTelegraphDefinition,
					AreaTelegraphAnchorMode::FixedLocation,
					AreaTelegraphProgressDriver::External
				}
			).lock();
		if (!externalTelegraph)
		{
			return Fail("External area telegraph could not be spawned");
		}
		externalTelegraphWorld.TickInternal(0.2f);
		if (externalTelegraphWorld.GetActorsByType<AreaTelegraphActor>().size() != 1)
		{
			return Fail("External area telegraph cleaned up without an explicit end");
		}
		externalTelegraph->SetExternalProgress(0.5f);
		externalTelegraph->Complete(0.5f);
		if (!externalTelegraph->IsInCompletionFeedback())
		{
			return Fail("Area telegraph did not enter completion feedback");
		}
		externalTelegraph->SetExternalProgress(0.f);
		externalTelegraphWorld.TickInternal(0.2f);
		if (!externalTelegraphWorld.GetActorsByType<AreaTelegraphActor>().empty())
		{
			return Fail("Area telegraph completion feedback did not clean itself up");
		}
	}

	GameplayTagContainer tags;
	const GameplayTag shieldTag{ "State.Defense.Shield" };
	tags.AddTag(shieldTag);
	tags.AddTag(shieldTag);
	if (!tags.HasTag(GameplayTag{ "State.Defense" }))
	{
		return Fail("Hierarchical tag matching failed");
	}
	tags.RemoveTag(shieldTag);
	if (!tags.HasTag(shieldTag, true))
	{
		return Fail("Counted tag was removed too early");
	}
	tags.RemoveTag(shieldTag);
	if (tags.HasTag(shieldTag))
	{
		return Fail("Counted tag was not removed");
	}

	sas::AttributeSystem attributes;
	attributes.RegisterAttribute(OwnerAttributeIds::AttackPower, 10.f);
	const sas::AttributeModifierHandle additive = attributes.AddModifier(
		sas::AttributeModifier{ OwnerAttributeIds::AttackPower, sas::AttributeModifierOperation::Add, 5.f }
	);
	const sas::AttributeModifierHandle multiplicative = attributes.AddModifier(
		sas::AttributeModifier{ OwnerAttributeIds::AttackPower, sas::AttributeModifierOperation::Multiply, 2.f }
	);
	if (!NearlyEqual(attributes.GetCurrentValue(OwnerAttributeIds::AttackPower), 30.f))
	{
		return Fail("Attribute modifier order failed");
	}
	attributes.RemoveModifier(multiplicative);
	attributes.ApplyBaseModifier(sas::AttributeModifier{ OwnerAttributeIds::AttackPower, 5.f });
	if (!NearlyEqual(attributes.GetCurrentValue(OwnerAttributeIds::AttackPower), 20.f))
	{
		return Fail("Instant base modifier failed");
	}
	attributes.RemoveModifier(additive);

	attributes.RegisterAttribute(OwnerAttributeIds::AbilityHaste, 0.f);
	attributes.AddModifier(sas::AttributeModifier{ OwnerAttributeIds::AbilityHaste, 0.1f });
	attributes.AddModifier(sas::AttributeModifier{ OwnerAttributeIds::AbilityHaste, 0.1f });
	if (!NearlyEqual(attributes.GetSequentialReductionMultiplier(OwnerAttributeIds::AbilityHaste), 0.81f))
	{
		return Fail("Sequential haste calculation failed");
	}

	HealthComponent regeneratingHealth{ 100.f, 100.f };
	regeneratingHealth.ChangeHealth(-60.f);
	regeneratingHealth.Regenerate((50.f / 600.f) * 600.f);
	if (!NearlyEqual(regeneratingHealth.GetHealth(), 90.f))
	{
		return Fail("Health regeneration did not restore 50 health in 10 minutes");
	}

	ShieldComponent regeneratingShield{ 100.f, 100.f, 3.f };
	if (!NearlyEqual(regeneratingShield.AbsorbDamage(20.f, 1.25f, 0.75f), 20.f) ||
		!NearlyEqual(regeneratingShield.GetShield(), 75.f))
	{
		return Fail("Ship shield did not absorb damage using the shield multiplier");
	}
	ShieldComponent frameRateIndependentOvershield{ 150.f, 150.f, 3.f };
	frameRateIndependentOvershield.GrantTemporaryOvershield(
		"Test.Overshield",
		100.f,
		0.f,
		100.f
	);
	frameRateIndependentOvershield.TickTemporaryOvershields(0.01f);
	if (!NearlyEqual(frameRateIndependentOvershield.GetShield(), 249.f))
	{
		return Fail("Temporary overshield decay was not proportional to a 0.01 second tick");
	}
	frameRateIndependentOvershield.TickTemporaryOvershields(0.09f);
	if (!NearlyEqual(frameRateIndependentOvershield.GetShield(), 240.f))
	{
		return Fail("Temporary overshield decay did not preserve its per-second rate");
	}
	regeneratingShield.Tick(3.74f, 40.f);
	if (!NearlyEqual(regeneratingShield.GetShield(), 75.f))
	{
		return Fail("Ship shield started recharging before its damage delay elapsed");
	}
	regeneratingShield.Tick(0.26f, 40.f);
	if (!NearlyEqual(regeneratingShield.GetShield(), 85.f))
	{
		return Fail("Ship shield did not recharge after its damage delay elapsed");
	}
	regeneratingShield.AbsorbDamage(20.f, 1.f, 0.f);
	const float shieldDelayBeforeBoostPause = regeneratingShield.GetRechargeDelayRemaining();
	regeneratingShield.Tick(10.f, 40.f, false);
	if (!NearlyEqual(regeneratingShield.GetShield(), 65.f) ||
		!NearlyEqual(regeneratingShield.GetRechargeDelayRemaining(), shieldDelayBeforeBoostPause))
	{
		return Fail("Ship shield recharged while afterburner recharge blocking was active");
	}

	EnergyComponent regeneratingEnergy{ 50.f, 50.f, 2.f };
	if (!NearlyEqual(regeneratingEnergy.Consume(30.f), 30.f) ||
		!NearlyEqual(regeneratingEnergy.GetEnergy(), 20.f))
	{
		return Fail("Ship energy did not consume afterburner capacity");
	}
	regeneratingEnergy.Tick(2.5f, 20.f);
	if (!NearlyEqual(regeneratingEnergy.GetEnergy(), 30.f))
	{
		return Fail("Ship energy did not honor its recharge delay");
	}
	regeneratingEnergy.Consume(10.f);
	const float energyDelayBeforeBoostPause = regeneratingEnergy.GetRechargeDelayRemaining();
	regeneratingEnergy.Tick(10.f, 20.f, false);
	if (!NearlyEqual(regeneratingEnergy.GetEnergy(), 20.f) ||
		!NearlyEqual(regeneratingEnergy.GetRechargeDelayRemaining(), energyDelayBeforeBoostPause))
	{
		return Fail("Ship energy recharged while afterburner recharge blocking was active");
	}

	const sf::Vector2f projectileFireDirection{ 0.f, -1.f };
	const sf::Vector2f forwardCarrierVelocity = ProjectileMotion::ResolveCarrierVelocity(
		{ 0.f, -200.f },
		projectileFireDirection
	);
	const sf::Vector2f backwardCarrierVelocity = ProjectileMotion::ResolveCarrierVelocity(
		{ 0.f, 200.f },
		projectileFireDirection
	);
	const sf::Vector2f lateralCarrierVelocity = ProjectileMotion::ResolveCarrierVelocity(
		{ 200.f, 0.f },
		projectileFireDirection
	);
	const sf::Vector2f diagonalCarrierVelocity = ProjectileMotion::ResolveCarrierVelocity(
		{ 200.f, -200.f },
		projectileFireDirection
	);
	if (!NearlyEqual(forwardCarrierVelocity.x, 0.f) ||
		!NearlyEqual(forwardCarrierVelocity.y, -150.f) ||
		!NearlyEqual(backwardCarrierVelocity.x, 0.f) ||
		!NearlyEqual(backwardCarrierVelocity.y, 0.f) ||
		!NearlyEqual(lateralCarrierVelocity.x, 10.f) ||
		!NearlyEqual(lateralCarrierVelocity.y, 0.f) ||
		!NearlyEqual(diagonalCarrierVelocity.x, 10.f) ||
		!NearlyEqual(diagonalCarrierVelocity.y, -150.f))
	{
		return Fail("Projectile momentum inheritance did not preserve the arcade forward and lateral rule");
	}

	ShipEnergyAttributes tankEnergyAttributes;
	tankEnergyAttributes.baseMaxShield = 100.f;
	tankEnergyAttributes.shieldFullRechargeDuration = 5.f;
	tankEnergyAttributes.baseShieldRechargeDelay = 4.f;
	tankEnergyAttributes.baseAfterburnerCapacity = 50.f;
	tankEnergyAttributes.afterburnerFullRechargeDuration = 8.f;
	tankEnergyAttributes.baseAfterburnerRechargeDelay = 2.f;
	tankEnergyAttributes.baseAfterburnerSpeedMultiplier = 1.4f;
	tankEnergyAttributes.baseAfterburnerAccelerationMultiplier = 1.8f;
	tankEnergyAttributes.baseAfterburnerEnergyDrainPerSecond = 12.f;
	tankEnergyAttributes.shieldAffinity = 0.5f;
	tankEnergyAttributes.afterburnerAffinity = 0.5f;
	tankEnergyAttributes.baseAfterburnerRampUpDuration = 0.18f;
	tankEnergyAttributes.baseAfterburnerRampDownDuration = 0.25f;
	tankEnergyAttributes.baseAfterburnerManeuverabilityMultiplier = 0.8f;
	const ShipDefinition energyTestShip{
		"",
		100.f,
		{},
		0.f,
		0.f,
		0,
		{},
		{},
		"",
		ShipMovementAttributes{},
		tankEnergyAttributes
	};
	TestCombatant energyTestCombatant;
	CombatRuntime& energyRuntime = energyTestCombatant.GetCombatRuntime();
	energyRuntime.InitializeOwnerAttributes(energyTestShip.health);
	ShipRuntime energyShipRuntime{
		energyRuntime.GetAbilitySystemComponent().GetAttributes()
	};
	energyShipRuntime.InitializeFromShipDefinition(energyTestShip);
	energyRuntime.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(sas::AttributeModifier{ OwnerAttributeIds::EnergyPower, 100.f });
	const sas::AttributeSystem& shipAttributes = energyShipRuntime.GetAttributes();
	if (!NearlyEqual(shipAttributes.GetCurrentValue(ShipAttributeIds::MaxShield), 200.f) ||
		!NearlyEqual(shipAttributes.GetCurrentValue(ShipAttributeIds::ShieldRegen), 40.f) ||
		!NearlyEqual(shipAttributes.GetCurrentValue(ShipAttributeIds::ShieldRechargeDelay), 4.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerCapacity(), 150.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerRegenPerSecond(), 18.75f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerRechargeDelay(), 2.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerEnergyDrainPerSecond(), 12.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerSpeedMultiplier(), 1.4f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerAccelerationMultiplier(), 1.8f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerRampUpDuration(), 0.18f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerManeuverabilityMultiplier(), 0.8f))
	{
		return Fail("Ship energy attributes did not derive shield and afterburner values correctly");
	}
	energyRuntime.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(sas::AttributeModifier{ OwnerAttributeIds::EnergyPower, 50.f });
	if (!NearlyEqual(energyShipRuntime.GetAfterburnerCapacity(), 200.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerSpeedMultiplier(), 1.4f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerAccelerationMultiplier(), 1.8f))
	{
		return Fail("Energy capacity incorrectly changed afterburner speed or acceleration tuning");
	}

	const ShipMovementAttributes movementRatingTest{};
	const float firstMovementContribution = movementRatingTest.GetDiminishingRatingContribution(10.f, 20.f);
	const float secondMovementContribution = movementRatingTest.GetDiminishingRatingContribution(20.f, 20.f);
	if (firstMovementContribution <= 0.f ||
		secondMovementContribution <= firstMovementContribution ||
		(secondMovementContribution - firstMovementContribution) >= firstMovementContribution)
	{
		return Fail("Movement rating curve was not monotonic with diminishing returns");
	}

	if (!NearlyEqual(sas::AttributeMath::GetArmorDamageReduction(50.f), 1.f / 3.f) ||
		sas::AttributeMath::GetAbilityHasteReduction(500.f) >= 1.f ||
		sas::AttributeMath::GetCriticalChance(500.f) >= 1.f ||
		sas::AttributeMath::GetCombatLuckFactor(500.f) >= 1.f ||
		sas::AttributeMath::GetCombatLuckFactor(20.f) <= sas::AttributeMath::GetCombatLuckFactor(10.f))
	{
		return Fail("Percentage rating curves were not asymptotic and monotonic");
	}

	const ShipProgressionDefinition fighterProgressionDefinition{
		100.f,
		1.25f,
		{
			{ OwnerAttributeIds::AttackPower, 3.f },
			{ OwnerAttributeIds::AttackSpeed, 2.f },
			{ OwnerAttributeIds::CriticalChance, 2.f },
			{ OwnerAttributeIds::MaxHealth, 1.f },
			{ OwnerAttributeIds::Armor, 1.f },
			{ OwnerAttributeIds::EnergyPower, 0.5f },
			{ OwnerAttributeIds::MoveSpeedHorizontal, 0.5f },
			{ OwnerAttributeIds::MoveSpeedVertical, 0.5f }
		}
	};
	TestCombatant fighterProgressionCombatant;
	CombatRuntime& fighterRuntime = fighterProgressionCombatant.GetCombatRuntime();
	fighterRuntime.InitializeOwnerAttributes(energyTestShip.health);
	ShipRuntime fighterShipRuntime{
		fighterRuntime.GetAbilitySystemComponent().GetAttributes()
	};
	fighterShipRuntime.InitializeFromShipDefinition(energyTestShip);
	ShipProgression fighterProgression;
	fighterProgression.Configure(fighterProgressionDefinition);
	fighterProgression.BindAttributes(
		fighterRuntime.GetAbilitySystemComponent().GetAttributes()
	);
	fighterProgression.AddXP(100.f);
	const sas::AttributeSystem& fighterAttributes =
		fighterRuntime.GetAbilitySystemComponent().GetAttributes();
	if (fighterProgression.GetLevel() != 2 ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::MaxHealth), 101.f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::AttackPower), 3.f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::AttackSpeed), 2.f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::CriticalChance), 2.f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::Luck), 0.f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::AbilityHaste), 0.f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::EnergyPower), 0.5f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::HealthRegen), 101.f / 1200.f))
	{
		return Fail("Natural growth did not apply the configured owner attributes");
	}
	fighterProgression.BindAttributes(
		fighterRuntime.GetAbilitySystemComponent().GetAttributes()
	);
	if (!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::AttackPower), 3.f))
	{
		return Fail("Ship progression applied the same level bonus twice when rebound");
	}
	ShipProgression multiLevelProgression;
	multiLevelProgression.Configure(fighterProgressionDefinition);
	multiLevelProgression.AddXP(10000.f);
	if (multiLevelProgression.GetLevel() <= 2)
	{
		return Fail("Ship progression did not support multiple levels from one XP grant");
	}

	sas::ActiveGameplayEffect barrier;
	barrier.spec.definition.effectId = "Effect.Test.Barrier";
	barrier.spec.definition.behaviorKey = EffectBehaviorKeys::Barrier;
	barrier.spec.attributes = {
		sas::GameplayAttribute{ BarrierEffectSchema::Capacity, 30.f, 0.f },
		sas::GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f }
	};
	barrier.ResetRuntimeAttributesFromSpec();

	DamageContext firstHit;
	firstHit.originalDamage = 12.f;
	firstHit.remainingDamage = 12.f;
	const sas::GameplayEffectBehaviorResult firstResult =
		GetEffectBehaviorRuntime().ProcessEvent(barrier, firstHit);
	if (!firstResult.changed || firstResult.removeEffect ||
		!NearlyEqual(firstHit.remainingDamage, 0.f) ||
		!NearlyEqual(sas::FindAttributeValue(
			barrier.runtimeAttributes,
			BarrierEffectSchema::Capacity
		), 18.f))
	{
		return Fail("Barrier partial absorption failed");
	}

	DamageContext breakingHit;
	breakingHit.originalDamage = 20.f;
	breakingHit.remainingDamage = 20.f;
	const sas::GameplayEffectBehaviorResult breakResult =
		GetEffectBehaviorRuntime().ProcessEvent(barrier, breakingHit);
	if (!breakResult.removeEffect || breakResult.events.empty() ||
		!NearlyEqual(breakingHit.remainingDamage, 2.f))
	{
		return Fail("Barrier break behavior failed");
	}

	barrier.ResetRuntimeAttributesFromSpec();
	GetEffectBehaviorRuntime().AddStack(barrier);
	if (!NearlyEqual(sas::FindAttributeValue(
		barrier.runtimeAttributes,
		BarrierEffectSchema::Capacity
	), 60.f))
	{
		return Fail("Barrier stack behavior failed");
	}

	const DamagePayload energyPayload = DamageTypeSystem::BuildPayload(
		{ DamageTypeSchema::Energy },
		{
			sas::GameplayAttribute{ DamageAttributeIds::ShieldDamageMultiplier, 1.25f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::ShieldRegenerationDelay, 0.75f, 0.f }
		}
	);
	const DamagePayload identityOnlyThermalPayload =
		DamageTypeSystem::BuildPayload({ DamageTypeSchema::Thermal });
	if (identityOnlyThermalPayload.igniteStacks != 0 ||
		!NearlyEqual(identityOnlyThermalPayload.burnDamagePerSecond, 0.f))
	{
		return Fail("Damage tag identity unexpectedly supplied hidden balance defaults");
	}
	if (!NearlyEqual(energyPayload.shieldDamageMultiplier, 1.25f))
	{
		return Fail("Energy damage payload was not resolved");
	}
	if (!NearlyEqual(energyPayload.shieldRegenerationDelay, 0.75f))
	{
		return Fail("Energy damage shield regeneration delay was not resolved");
	}

	sas::GameplayEffectDefinition regeneratingBarrier;
	regeneratingBarrier.effectId = "Effect.Test.RegeneratingBarrier";
	regeneratingBarrier.behaviorKey = EffectBehaviorKeys::Barrier;
	regeneratingBarrier.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
	regeneratingBarrier.attributes = {
		sas::GameplayAttribute{ BarrierEffectSchema::Capacity, 10.f, 0.f },
		sas::GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f },
		sas::GameplayAttribute{ BarrierEffectSchema::RegenerationPerSecond, 5.f, 0.f },
		sas::GameplayAttribute{ BarrierEffectSchema::RegenerationDelay, 1.f, 0.f },
		sas::GameplayAttribute{ BarrierEffectSchema::RegenerationDelayRemaining, 0.f, 0.f }
	};
	TestCombatant regenerationTarget;
	const sas::GameplayEffectHandle regenerationHandle = regenerationTarget.GetAbilitySystemComponent().ApplyGameplayEffect(
		regeneratingBarrier
	);
	ApplyCombatDamage(regenerationTarget, 2.f, nullptr, { DamageTypeSchema::Energy }, energyPayload);
	regenerationTarget.GetCombatRuntime().Tick(1.5f);
	const sas::ActiveGameplayEffect* delayedBarrier = regenerationTarget.GetAbilitySystemComponent().FindGameplayEffect(regenerationHandle);
	if (!delayedBarrier || !NearlyEqual(
		sas::FindAttributeValue(delayedBarrier->runtimeAttributes, BarrierEffectSchema::Capacity),
		7.5f
	))
	{
		return Fail("Energy damage did not delay shield regeneration");
	}
	regenerationTarget.GetCombatRuntime().Tick(0.5f);
	const sas::ActiveGameplayEffect* regeneratingBarrierState = regenerationTarget.GetAbilitySystemComponent().FindGameplayEffect(regenerationHandle);
	if (!regeneratingBarrierState || !NearlyEqual(
		sas::FindAttributeValue(regeneratingBarrierState->runtimeAttributes, BarrierEffectSchema::Capacity),
		8.75f
	))
	{
		return Fail("Barrier did not regenerate after its delay elapsed");
	}

	AttachmentLoadout energyAttachmentLoadout;
	if (!energyAttachmentLoadout.TryEquip(
		AttachmentData::Definitions::EnergyCoupler,
		AttachmentHostKind::PrimaryWeapon,
		{ AttachmentSchema::Capability::Damage },
		1
	))
	{
		return Fail("Energy attachment could not be equipped on a damage host");
	}
	const List<GameplayTag> energyAttachmentTags = energyAttachmentLoadout.ResolveDamageTags(
		AttachmentHostKind::PrimaryWeapon,
		{ DamageTypeSchema::Photonic }
	);
	sas::GameplayAttributeList energyAttachmentAttributes = energyAttachmentLoadout.MergeGrantedAttributes(
		AttachmentHostKind::PrimaryWeapon,
		{ sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f } }
	);
	energyAttachmentAttributes = energyAttachmentLoadout.ApplyConditionalModifiers(
		AttachmentHostKind::PrimaryWeapon,
		energyAttachmentAttributes,
		energyAttachmentTags
	);
	const DamagePayload attachmentEnergyPayload = DamageTypeSystem::BuildPayload(
		energyAttachmentTags,
		energyAttachmentAttributes
	);
	if (energyAttachmentTags.size() != 1 || energyAttachmentTags.front() != DamageTypeSchema::Energy ||
		!NearlyEqual(sas::FindAttributeValue(energyAttachmentAttributes, CommonAttributeIds::Damage), 11.5f) ||
		!NearlyEqual(attachmentEnergyPayload.shieldRegenerationDelay, 0.75f))
	{
		return Fail("Damage type attachment did not resolve its converted tag and conditional attributes");
	}
	sas::GameplayEffectDefinition smallShield;
	smallShield.effectId = "Effect.Test.SmallShield";
	smallShield.behaviorKey = EffectBehaviorKeys::Barrier;
	smallShield.durationPolicy = sas::GameplayEffectDurationPolicy::Infinite;
	smallShield.attributes = {
		sas::GameplayAttribute{ BarrierEffectSchema::Capacity, 5.f, 0.f },
		sas::GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f }
	};
	TestCombatant shieldTarget;
	shieldTarget.GetAbilitySystemComponent().ApplyGameplayEffect(smallShield);
	ApplyCombatDamage(shieldTarget, 8.f, nullptr, { DamageTypeSchema::Energy }, energyPayload);
	if (!NearlyEqual(shieldTarget.GetHealth(), 96.f))
	{
		return Fail("Energy damage did not apply its shield multiplier correctly");
	}

	TestCombatant armoredTarget;
	armoredTarget.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::Armor, sas::AttributeModifierOperation::Override, 50.f }
	);
	const DamagePayload kineticPayload = DamageTypeSystem::BuildPayload(
		{ DamageTypeSchema::Kinetic },
		{ sas::GameplayAttribute{ DamageAttributeIds::ArmorPenetration, 0.10f, 0.f, 0.25f } }
	);
	if (!NearlyEqual(kineticPayload.armorPenetration, 0.10f))
	{
		return Fail("Kinetic armor penetration was not reduced");
	}
	ApplyCombatDamage(armoredTarget, 10.f, nullptr, { DamageTypeSchema::Kinetic }, kineticPayload);
	const float effectiveKineticReduction =
		sas::AttributeMath::GetArmorDamageReduction(50.f) * (1.f - kineticPayload.armorPenetration);
	const float expectedKineticHealth = 100.f - 10.f * (1.f - effectiveKineticReduction);
	if (!NearlyEqual(armoredTarget.GetHealth(), expectedKineticHealth))
	{
		return Fail("Kinetic armor penetration was not applied");
	}

	TestCombatant thermalTarget;
	const DamagePayload thermalPayload = DamageTypeSystem::BuildPayload(
		{ DamageTypeSchema::Thermal },
		{
			sas::GameplayAttribute{ DamageAttributeIds::IgniteStacks, 1.f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::BurnDamagePerSecond, 1.f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::BurnDuration, 3.f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::BurnMaxStacks, 4.f, 1.f }
		}
	);
	if (!NearlyEqual(thermalPayload.burnDamagePerSecond, 1.f) ||
		thermalPayload.burnMaxStacks != 4)
	{
		return Fail("Thermal damage payload was not resolved to the four-hit profile");
	}
	ApplyCombatDamage(thermalTarget, 10.f, nullptr, { DamageTypeSchema::Thermal }, thermalPayload);
	thermalTarget.GetCombatRuntime().Tick(1.f);
	if (!NearlyEqual(thermalTarget.GetHealth(), 90.f))
	{
		return Fail("Thermal damage applied before its four-hit threshold");
	}
	for (int hit = 0; hit < 3; ++hit)
	{
		ApplyCombatDamage(thermalTarget, 10.f, nullptr, { DamageTypeSchema::Thermal }, thermalPayload);
	}
	thermalTarget.GetCombatRuntime().Tick(1.f);
	if (!NearlyEqual(thermalTarget.GetHealth(), 56.f))
	{
		return Fail("Thermal damage did not activate at its four-hit threshold");
	}
	ApplyCombatDamage(thermalTarget, 1.f, nullptr, { DamageTypeSchema::Thermal }, thermalPayload);
	const sas::ActiveGameplayEffect* cappedIgnite =
		thermalTarget.GetAbilitySystemComponent().FindGameplayEffectById(
			"Effect.Status.Damage.Ignite"
		);
	if (!cappedIgnite || cappedIgnite->stackCount != 4)
	{
		return Fail("Ignite did not remain at its stack cap after another hit");
	}

	TestCombatant cryoTarget;
	const DamagePayload cryoPayload = DamageTypeSystem::BuildPayload(
		{ DamageTypeSchema::Cryo },
		{
			sas::GameplayAttribute{ DamageAttributeIds::CryoBuildupPerHit, 1.f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::CryoBuildupRequired, 4.f, 1.f },
			sas::GameplayAttribute{ DamageAttributeIds::CryoBuildupDuration, 2.5f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::CryoSlowPercent, 0.25f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::CryoSlowDuration, 1.5f, 0.f }
		}
	);
	if (cryoPayload.cryoBuildupRequired != 4 ||
		!NearlyEqual(cryoPayload.cryoSlowPercent, 0.25f) ||
		!NearlyEqual(cryoPayload.cryoSlowDuration, 1.5f))
	{
		return Fail("Cryo damage payload was not resolved to the four-hit profile");
	}
	for (int expectedStacks = 1; expectedStacks < 4; ++expectedStacks)
	{
		ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
		const sas::ActiveGameplayEffect* buildup =
			cryoTarget.GetAbilitySystemComponent().FindGameplayEffectById(
				DamageStatusEffectIds::CryoBuildupEffectId
			);
		if (!buildup || buildup->stackCount != expectedStacks ||
			!NearlyEqual(
				cryoTarget.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
					OwnerAttributeIds::MovementSlow
				),
				0.f
			))
		{
			return Fail("Cryo slowed before its four-hit buildup was complete");
		}
	}
	ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
	const sas::ActiveGameplayEffect* fullCryoBuildup =
		cryoTarget.GetAbilitySystemComponent().FindGameplayEffectById(
			DamageStatusEffectIds::CryoBuildupEffectId
		);
	if (!NearlyEqual(
		cryoTarget.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::MovementSlow),
		0.25f
	) || !fullCryoBuildup || fullCryoBuildup->stackCount != 4 ||
		!cryoTarget.GetAbilitySystemComponent().FindGameplayEffectById(
			DamageStatusEffectIds::CryoSlowedEffectId
		))
	{
		return Fail("Four-hit Cryo buildup did not retain full stacks and apply slow");
	}
	ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
	fullCryoBuildup = cryoTarget.GetAbilitySystemComponent().FindGameplayEffectById(
		DamageStatusEffectIds::CryoBuildupEffectId
	);
	if (!fullCryoBuildup || fullCryoBuildup->stackCount != 4)
	{
		return Fail("Cryo buildup did not remain at its cap while slow was active");
	}
	cryoTarget.GetCombatRuntime().Tick(1.f);
	ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
	cryoTarget.GetCombatRuntime().Tick(0.75f);
	if (!NearlyEqual(
		cryoTarget.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::MovementSlow),
		0.25f
	))
	{
		return Fail("Cryo hit did not refresh the active slow duration");
	}
	cryoTarget.GetCombatRuntime().Tick(0.8f);
	if (!NearlyEqual(
		cryoTarget.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::MovementSlow),
		0.f
	))
	{
		return Fail("Cryo slow did not expire");
	}
	ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
	cryoTarget.GetCombatRuntime().Tick(2.6f);
	if (cryoTarget.GetAbilitySystemComponent().FindGameplayEffectById(
		DamageStatusEffectIds::CryoBuildupEffectId
	))
	{
		return Fail("Incomplete Cryo buildup did not expire");
	}

	TestCombatant cryoPriorityTarget;
	const DamagePayload strongCryoPayload = DamageTypeSystem::BuildPayload(
		{ DamageTypeSchema::Cryo },
		{
			sas::GameplayAttribute{ DamageAttributeIds::CryoBuildupPerHit, 4.f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::CryoBuildupRequired, 4.f, 1.f },
			sas::GameplayAttribute{ DamageAttributeIds::CryoBuildupDuration, 2.5f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::CryoSlowPercent, 0.50f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::CryoSlowDuration, 2.f, 0.f }
		}
	);
	ApplyCombatDamage(
		cryoPriorityTarget,
		1.f,
		nullptr,
		{ DamageTypeSchema::Cryo },
		strongCryoPayload
	);
	cryoPriorityTarget.GetCombatRuntime().Tick(1.f);
	ApplyCombatDamage(
		cryoPriorityTarget,
		1.f,
		nullptr,
		{ DamageTypeSchema::Cryo },
		cryoPayload
	);
	cryoPriorityTarget.GetCombatRuntime().Tick(1.1f);
	if (!NearlyEqual(
		cryoPriorityTarget.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
			OwnerAttributeIds::MovementSlow
		),
		0.f
	))
	{
		return Fail("Weaker Cryo slow replaced or refreshed the stronger slow");
	}
	ApplyCombatDamage(
		cryoPriorityTarget,
		1.f,
		nullptr,
		{ DamageTypeSchema::Cryo },
		cryoPayload
	);
	if (!NearlyEqual(
		cryoPriorityTarget.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
			OwnerAttributeIds::MovementSlow
		),
		0.25f
	))
	{
		return Fail("Full Cryo stacks did not apply a slow after the old one expired");
	}
	ApplyCombatDamage(
		cryoPriorityTarget,
		1.f,
		nullptr,
		{ DamageTypeSchema::Cryo },
		strongCryoPayload
	);
	if (!NearlyEqual(
		cryoPriorityTarget.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
			OwnerAttributeIds::MovementSlow
		),
		0.50f
	))
	{
		return Fail("Stronger Cryo slow did not replace the weaker slow");
	}
	cryoPriorityTarget.GetCombatRuntime().Tick(1.25f);
	ApplyCombatDamage(
		cryoPriorityTarget,
		1.f,
		nullptr,
		{ DamageTypeSchema::Cryo },
		strongCryoPayload
	);
	cryoPriorityTarget.GetCombatRuntime().Tick(1.f);
	if (!NearlyEqual(
		cryoPriorityTarget.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
			OwnerAttributeIds::MovementSlow
		),
		0.50f
	))
	{
		return Fail("Equal Cryo slow did not refresh its duration");
	}


	TestCombatant electricTarget;
	const DamagePayload electricPayload = DamageTypeSystem::BuildPayload(
		{ DamageTypeSchema::Electric },
		{
			sas::GameplayAttribute{ DamageAttributeIds::ElectricStacks, 1.f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::ElectricDamageTakenMultiplierPerStack, 0.04f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::ElectricDuration, 3.f, 0.f },
			sas::GameplayAttribute{ DamageAttributeIds::ElectricMaxStacks, 4.f, 1.f }
		}
	);
	if (electricPayload.electricStacks != 1 ||
		!NearlyEqual(electricPayload.electricDamageTakenMultiplierPerStack, 0.04f) ||
		electricPayload.electricMaxStacks != 4)
	{
		return Fail("Electric damage payload was not resolved to the four-hit profile");
	}
	for (int hit = 0; hit < 3; ++hit)
	{
		ApplyCombatDamage(electricTarget, 1.f, nullptr, { DamageTypeSchema::Electric }, electricPayload);
	}
	ApplyCombatDamage(electricTarget, 10.f, nullptr);
	if (!NearlyEqual(electricTarget.GetHealth(), 87.f))
	{
		return Fail("Electric amplified damage before its four-hit threshold");
	}
	ApplyCombatDamage(electricTarget, 1.f, nullptr, { DamageTypeSchema::Electric }, electricPayload);
	ApplyCombatDamage(electricTarget, 10.f, nullptr);
	if (!NearlyEqual(electricTarget.GetHealth(), 74.4f))
	{
		return Fail("Electric did not apply its reduced full-stack vulnerability");
	}
	electricTarget.GetCombatRuntime().Tick(3.1f);
	ApplyCombatDamage(electricTarget, 10.f, nullptr);
	if (!NearlyEqual(electricTarget.GetHealth(), 64.4f))
	{
		return Fail("Electric vulnerability did not expire");
	}

	PrimaryWeaponDefinition projectileWeapon;
	projectileWeapon.weaponType = PrimaryWeaponType::ProjectileStandard;
	projectileWeapon.attributes = {
		sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.f, 0.f }
	};
	if (!PrimaryWeaponExecutionSystem::ValidateDefinition(projectileWeapon).isValid)
	{
		return Fail("Valid projectile weapon was rejected");
	}
	PrimaryWeaponDefinition mismatchedWeaponFamily = projectileWeapon;
	mismatchedWeaponFamily.weaponId = "Weapon.Beam.Mismatched.Basic";
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(mismatchedWeaponFamily).isValid)
	{
		return Fail("Weapon content ID family was allowed to diverge from its type tag family");
	}
	PrimaryWeaponDefinition electricArcWeapon;
	electricArcWeapon.weaponType = PrimaryWeaponType::ArcElectric;
	electricArcWeapon.attributes = {
		sas::GameplayAttribute{ CommonAttributeIds::Damage, 12.f, 0.f },
		sas::GameplayAttribute{ CommonAttributeIds::Range, 700.f, 1.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Arc::Electric::ChainCount, 2.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Arc::Electric::ChainRange, 240.f, 1.f },
		sas::GameplayAttribute{
			PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain,
			0.75f,
			0.01f,
			1.f
		}
	};
	if (!PrimaryWeaponExecutionSystem::ValidateDefinition(electricArcWeapon).isValid)
	{
		return Fail("Valid arc projectile weapon was rejected");
	}
	PrimaryWeaponDefinition invalidElectricArcWeapon = electricArcWeapon;
	sas::FindAttribute(
		invalidElectricArcWeapon.attributes,
		PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain
	)->baseValue = 1.1f;
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidElectricArcWeapon).isValid)
	{
		return Fail("Arc projectile accepted an invalid chain damage multiplier");
	}
	PrimaryWeaponDefinition projectileFamilyDefinition = projectileWeapon;
	projectileFamilyDefinition.weaponType = PrimaryWeaponType::BeamContinuous;
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(projectileFamilyDefinition).isValid)
	{
		return Fail("A primary weapon family tag was accepted as a concrete weapon type");
	}
	const GameAbilityDefinition automaticPrimaryAbility = AbilityData::MakePrimaryFireAbilityDefinition(projectileWeapon);
	PrimaryWeaponDefinition semiAutomaticWeapon = projectileWeapon;
	semiAutomaticWeapon.automaticFire = false;
	const GameAbilityDefinition semiAutomaticPrimaryAbility = AbilityData::MakePrimaryFireAbilityDefinition(semiAutomaticWeapon);
	if (automaticPrimaryAbility.activationPolicy != sas::AbilityActivationPolicy::WhileHeld ||
		automaticPrimaryAbility.actions.empty() ||
		automaticPrimaryAbility.actions.front().phase != sas::AbilityActionPhase::WhileActive ||
		semiAutomaticPrimaryAbility.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
		semiAutomaticPrimaryAbility.actions.empty() ||
		semiAutomaticPrimaryAbility.actions.front().phase != sas::AbilityActionPhase::OnActivate)
	{
		return Fail("Primary weapon automatic fire policy was not generated correctly");
	}
	const GameAbilityDefinition validPrimaryAbility = AbilityData::MakePrimaryFireAbilityDefinition(
		LoadedWeapon("Weapon.Projectile.DualKineticBlaster.Basic")
	);
	std::string primaryAbilityValidationFailure;
	if (!ValidateAbilityDefinition(validPrimaryAbility, &primaryAbilityValidationFailure))
	{
		return Fail("PrimaryFire ability was rejected by family-owned attribute validation");
	}
	GameAbilityDefinition primaryAbilityWithAbilityAttribute = validPrimaryAbility;
	primaryAbilityWithAbilityAttribute.attributes.emplace_back(
		sas::AttributeId{ "Ability.Offense.OverdriveCore.ForeignValue" },
		1.f,
		0.f
	);
	if (ValidateAbilityDefinition(primaryAbilityWithAbilityAttribute, &primaryAbilityValidationFailure))
	{
		return Fail("PrimaryFire ability accepted an Ability.* base attribute");
	}
	GameAbilityDefinition primaryAbilityWithAbilityModifier = validPrimaryAbility;
	primaryAbilityWithAbilityModifier.attributeModifiers.emplace_back(
		sas::AttributeId{ "Ability.Offense.OverdriveCore.ForeignValue" },
		1.f
	);
	if (ValidateAbilityDefinition(primaryAbilityWithAbilityModifier, &primaryAbilityValidationFailure))
	{
		return Fail("PrimaryFire ability accepted an Ability.* definition modifier target");
	}

	PrimaryWeaponDefinition mixedWeapon = projectileWeapon;
	mixedWeapon.attributes.push_back(
		sas::GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Width, 20.f, 0.f }
	);
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(mixedWeapon).isValid)
	{
		return Fail("Mixed projectile and beam attributes were accepted");
	}
	PrimaryWeaponDefinition standardProjectileWithShotgunAttribute = projectileWeapon;
	standardProjectileWithShotgunAttribute.attributes.push_back(
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::PelletCount, 3.f, 1.f }
	);
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(standardProjectileWithShotgunAttribute).isValid)
	{
		return Fail("Standard projectile accepted a shotgun-only attribute");
	}

	PrimaryWeaponDefinition shotgunWeapon;
	shotgunWeapon.weaponType = PrimaryWeaponType::ProjectileShotgun;
	shotgunWeapon.attributes = {
		sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::PelletCount, 5.f, 1.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle, 42.f, 0.f }
	};
	if (!PrimaryWeaponExecutionSystem::ValidateDefinition(shotgunWeapon).isValid)
	{
		return Fail("Valid shotgun weapon was rejected");
	}
	if (!IsProjectileWeaponType(shotgunWeapon.weaponType))
	{
		return Fail("Shotgun type is not nested under the projectile family");
	}

	PrimaryWeaponDefinition invalidShotgunWeapon = shotgunWeapon;
	invalidShotgunWeapon.attributes.erase(invalidShotgunWeapon.attributes.begin() + 3);
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidShotgunWeapon).isValid)
	{
		return Fail("Shotgun without pellet count was accepted");
	}

	PrimaryWeaponDefinition continuousBeamWeapon;
	continuousBeamWeapon.weaponId = "Weapon.Beam.TestContinuousHeatBeam.Basic";
	continuousBeamWeapon.weaponType = PrimaryWeaponType::BeamContinuous;
	continuousBeamWeapon.attributes = {
		sas::GameplayAttribute{ CommonAttributeIds::Damage, 20.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Range, 800.f, 1.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Width, 20.f, 1.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Gain, 50.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Capacity, 100.f, 1.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Dissipation, 20.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::OverheatCooldown, 2.5f, 0.1f },
		sas::GameplayAttribute{
			PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat,
			1.5f,
			1.f
		}
	};
	continuousBeamWeapon.featureTypes = { PrimaryWeaponFeatureType::Heat };
	continuousBeamWeapon.heatGainCurve = {
		HeatGainCurveSegmentDefinition{ 50.f, 1.f },
		HeatGainCurveSegmentDefinition{ 75.f, 0.5f },
		HeatGainCurveSegmentDefinition{ 90.f, 0.25f },
		HeatGainCurveSegmentDefinition{ 100.f, 0.15f }
	};
	if (!PrimaryWeaponExecutionSystem::ValidateDefinition(continuousBeamWeapon).isValid)
	{
		return Fail("Valid continuous heat beam weapon was rejected");
	}
	PrimaryWeaponDefinition invalidContinuousBeamWeapon = continuousBeamWeapon;
	invalidContinuousBeamWeapon.attributes.erase(invalidContinuousBeamWeapon.attributes.begin() + 2);
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidContinuousBeamWeapon).isValid)
	{
		return Fail("Continuous beam without width was accepted");
	}
	PrimaryWeaponDefinition invalidHeatCurveWeapon = continuousBeamWeapon;
	invalidHeatCurveWeapon.heatGainCurve.back().endHeatPercentage = 95.f;
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidHeatCurveWeapon).isValid)
	{
		return Fail("Heat gain curve without a full-capacity segment was accepted");
	}

	PrimaryWeaponDefinition undeclaredFeatureWeapon = projectileWeapon;
	undeclaredFeatureWeapon.attributes.push_back(
		sas::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Capacity, 100.f, 0.f }
	);
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(undeclaredFeatureWeapon).isValid)
	{
		return Fail("An undeclared feature attribute was accepted");
	}

	PrimaryWeaponDefinition heatedProjectileWeapon = projectileWeapon;
	heatedProjectileWeapon.featureTypes = { PrimaryWeaponFeatureType::Heat };
	heatedProjectileWeapon.attributes.push_back(
		sas::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Gain, 3.f, 0.f }
	);
	heatedProjectileWeapon.attributes.push_back(
		sas::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Capacity, 10.f, 0.f }
	);
	heatedProjectileWeapon.attributes.push_back(
		sas::GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Dissipation, 2.f, 0.f }
	);
	PrimaryWeaponRuntimeState heatedRuntime;
	if (!PrimaryWeaponExecutionSystem::InitializeRuntime(heatedProjectileWeapon, heatedRuntime).isValid)
	{
		return Fail("Valid heat feature was rejected");
	}
	World heatTestWorld{ nullptr };
	Actor heatTestOwner{ &heatTestWorld };
	const List<GameplayTag> noDamageTags;
	const PrimaryWeaponExecutionContext heatContext{
		heatTestOwner,
		heatedProjectileWeapon,
		heatedProjectileWeapon.attributes,
		noDamageTags
	};
	PrimaryWeaponExecutionSystem::BeginFire(heatContext, heatedRuntime);
	if (!PrimaryWeaponExecutionSystem::FireOnce(heatContext, heatedRuntime) ||
		!NearlyEqual(heatedRuntime.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue), 3.f))
	{
		return Fail("Heat feature did not accumulate on fire");
	}
	PrimaryWeaponExecutionSystem::TickFire(heatContext, heatedRuntime, 1.f);
	if (!NearlyEqual(heatedRuntime.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue), 1.f))
	{
		return Fail("Heat feature did not dissipate over time");
	}
	PrimaryWeaponExecutionSystem::EndFire(heatContext, heatedRuntime);
	const PrimaryWeaponHandler* preservedHandler = heatedRuntime.handler;
	PrimaryWeaponTypeRuntimeState* preservedTypeState = heatedRuntime.typeState.get();
	const float preservedHeat = heatedRuntime.GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	);
	PrimaryWeaponDefinition invalidRuntimeReplacement = heatedProjectileWeapon;
	invalidRuntimeReplacement.weaponType = static_cast<PrimaryWeaponType>(999);
	if (PrimaryWeaponExecutionSystem::InitializeRuntime(
			invalidRuntimeReplacement,
			heatedRuntime
		).isValid ||
		heatedRuntime.handler != preservedHandler ||
		heatedRuntime.typeState.get() != preservedTypeState ||
		!heatedRuntime.isInitialized ||
		!NearlyEqual(
			heatedRuntime.GetFeatureValue(
				PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
			),
			preservedHeat
		))
	{
		return Fail("Failed primary weapon initialization corrupted a valid runtime");
	}

	PrimaryWeaponRuntimeState continuousBeamRuntime;
	if (!PrimaryWeaponExecutionSystem::InitializeRuntime(continuousBeamWeapon, continuousBeamRuntime).isValid)
	{
		return Fail("Continuous beam runtime could not be initialized");
	}
	const PrimaryWeaponExecutionContext continuousBeamContext{
		heatTestOwner,
		continuousBeamWeapon,
		continuousBeamWeapon.attributes,
		noDamageTags
	};
	PrimaryWeaponExecutionSystem::BeginFire(continuousBeamContext, continuousBeamRuntime);
	PrimaryWeaponExecutionSystem::TickFire(continuousBeamContext, continuousBeamRuntime, 1.f);
	if (!NearlyEqual(continuousBeamRuntime.GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 50.f))
	{
		return Fail("Continuous beam heat did not rise over time");
	}
	PrimaryWeaponExecutionSystem::EndFire(continuousBeamContext, continuousBeamRuntime);
	PrimaryWeaponExecutionSystem::TickInactive(continuousBeamContext, continuousBeamRuntime, 1.f);
	if (!NearlyEqual(continuousBeamRuntime.GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 30.f))
	{
		return Fail("Continuous beam heat did not dissipate while inactive");
	}
	PrimaryWeaponExecutionSystem::BeginFire(continuousBeamContext, continuousBeamRuntime);
	if (!NearlyEqual(continuousBeamRuntime.GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 30.f))
	{
		return Fail("Continuous beam heat was reset instead of persisting between fires");
	}
	PrimaryWeaponExecutionSystem::TickFire(continuousBeamContext, continuousBeamRuntime, 1.f);
	if (!NearlyEqual(continuousBeamRuntime.GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 65.f))
	{
		return Fail("Continuous beam heat curve did not reduce gain above fifty percent");
	}
	PrimaryWeaponExecutionSystem::TickFire(continuousBeamContext, continuousBeamRuntime, 3.f);
	if (!NearlyEqual(continuousBeamRuntime.GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 0.f) ||
		!NearlyEqual(PrimaryWeaponExecutionSystem::ConsumeRequestedCooldown(continuousBeamRuntime), 2.5f))
	{
		return Fail("Continuous beam overheat did not request its forced cooldown");
	}
	PrimaryWeaponExecutionSystem::EndFire(continuousBeamContext, continuousBeamRuntime);

	World continuousBeamDamageWorld{ nullptr };
	const weak_ptr<Actor> continuousBeamSourceWeak = continuousBeamDamageWorld.SpawnActor<Actor>();
	const shared_ptr<Actor> continuousBeamSource = continuousBeamSourceWeak.lock();
	const weak_ptr<TestCombatant> continuousBeamTargetWeak = continuousBeamDamageWorld.SpawnActor<TestCombatant>();
	const shared_ptr<TestCombatant> continuousBeamTarget = continuousBeamTargetWeak.lock();
	if (!continuousBeamSource || !continuousBeamTarget)
	{
		return Fail("Continuous beam damage test actors could not be spawned");
	}
	continuousBeamSource->SetCollisionLayer(CollisionLayer::Player);
	continuousBeamSource->SetActorRotation(90.f);
	continuousBeamTarget->SetCollisionLayer(CollisionLayer::Enemy);
	continuousBeamTarget->SetCollisionMask(CollisionLayer::PlayerBullet);
	continuousBeamTarget->SetActorLocation({ 400.f, 0.f });
	continuousBeamDamageWorld.TickInternal(0.f);

	PrimaryWeaponDefinition directDamageBeamWeapon = continuousBeamWeapon;
	directDamageBeamWeapon.muzzleDefinitions = { WeaponMuzzleDefinition{ { 0.f, 0.f }, 0.f } };
	PrimaryWeaponRuntimeState directDamageBeamRuntime;
	if (!PrimaryWeaponExecutionSystem::InitializeRuntime(directDamageBeamWeapon, directDamageBeamRuntime).isValid)
	{
		return Fail("Continuous beam damage runtime could not be initialized");
	}
	const PrimaryWeaponExecutionContext directDamageBeamContext{
		*continuousBeamSource,
		directDamageBeamWeapon,
		directDamageBeamWeapon.attributes,
		noDamageTags
	};
	PrimaryWeaponExecutionSystem::BeginFire(directDamageBeamContext, directDamageBeamRuntime);
	PrimaryWeaponExecutionSystem::TickFire(directDamageBeamContext, directDamageBeamRuntime, 0.5f);
	if (!NearlyEqual(continuousBeamTarget->GetHealth(), 90.f))
	{
		return Fail("Continuous beam did not apply damage along its forward range");
	}
	PrimaryWeaponExecutionSystem::EndFire(directDamageBeamContext, directDamageBeamRuntime);

	const std::string testWeaponUpgrade = "PrimaryWeapon.Upgrade.Test.DamagePulse";
	PrimaryWeaponDefinition progressiveHeatWeapon = heatedProjectileWeapon;
	progressiveHeatWeapon.weaponId = "Weapon.Projectile.ProgressiveHeat.Basic";
	progressiveHeatWeapon.featureTypes.clear();
	progressiveHeatWeapon.progressionProfile = WeaponProgressionProfile{ 3 }
		.AtLevel(
			2,
			{ sas::AttributeModifier{ CommonAttributeIds::Damage, sas::AttributeModifierOperation::Add, 5.f } },
			{ testWeaponUpgrade }
		)
		.AtLevel(
			3,
			{ sas::AttributeModifier{
				PrimaryWeaponSchema::Feature::Heat::Capacity,
				sas::AttributeModifierOperation::Add,
				5.f
			} },
			{},
			{ PrimaryWeaponFeatureType::Heat }
		);
	if (!PrimaryWeaponExecutionSystem::ValidateDefinition(progressiveHeatWeapon).isValid)
	{
		return Fail("A primary weapon with level-specific upgrades and features was rejected");
	}

	const GameAbilityDefinition progressivePrimaryAbility =
		AbilityData::MakePrimaryFireAbilityDefinition(progressiveHeatWeapon);
	if (progressivePrimaryAbility.GetMaxLevel() != 3 ||
		progressivePrimaryAbility.levelProgression.size() != 2 ||
		progressivePrimaryAbility.levelProgression[0].unlockedUpgradeIds.size() != 1 ||
		progressivePrimaryAbility.levelProgression[0].unlockedUpgradeIds.front() != testWeaponUpgrade ||
		progressivePrimaryAbility.levelProgression[1].unlockedUpgradeIds.size() != 1 ||
		progressivePrimaryAbility.levelProgression[1].unlockedUpgradeIds.front() !=
			PrimaryWeaponFeatureUpgradeId(PrimaryWeaponFeatureType::Heat))
	{
		return Fail("Primary weapon progression was not preserved during ability conversion");
	}

	const WeaponProgressionProfile flexibleWeaponProfile = WeaponProgressionProfile{ 7 }
		.EveryLevel({ sas::AttributeModifier{ CommonAttributeIds::Damage, 1.f } })
		.BetweenLevels(3, 5, { sas::AttributeModifier{ CommonAttributeIds::FireRate, 0.25f } })
		.AtLevel(6, { sas::AttributeModifier{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 1.f } })
		.FromLevel(6, { sas::AttributeModifier{ CommonAttributeIds::Range, 50.f } });
	const List<PrimaryWeaponLevelStep> flexibleWeaponSteps = flexibleWeaponProfile.ResolveLevelSteps();
	const auto stepHasModifier = [](const PrimaryWeaponLevelStep& step, const sas::AttributeId& attributeId, float value)
	{
		for (const sas::AttributeModifier& modifier : step.attributeModifiers)
		{
			if (modifier.attributeId == attributeId && NearlyEqual(modifier.magnitude, value))
			{
				return true;
			}
		}
		return false;
	};
	if (flexibleWeaponSteps.size() != 6 ||
		!stepHasModifier(flexibleWeaponSteps[0], CommonAttributeIds::Damage, 1.f) ||
		stepHasModifier(flexibleWeaponSteps[0], CommonAttributeIds::FireRate, 0.25f) ||
		!stepHasModifier(flexibleWeaponSteps[1], CommonAttributeIds::FireRate, 0.25f) ||
		!stepHasModifier(
			flexibleWeaponSteps[4],
			PrimaryWeaponSchema::Projectile::Delivery::PierceCount,
			1.f
		) ||
		!stepHasModifier(flexibleWeaponSteps[5], CommonAttributeIds::Range, 50.f))
	{
		return Fail("Weapon progression profile did not resolve repeated, ranged, and milestone growth");
	}

	PrimaryWeaponRuntimeState lockedProgressionRuntime;
	if (!PrimaryWeaponExecutionSystem::InitializeRuntime(progressiveHeatWeapon, lockedProgressionRuntime).isValid ||
		!lockedProgressionRuntime.features.empty())
	{
		return Fail("A level-locked primary weapon feature became active at level one");
	}

	const List<std::string> unlockedWeaponUpgrades{
		testWeaponUpgrade,
		PrimaryWeaponFeatureUpgradeId(PrimaryWeaponFeatureType::Heat)
	};
	PrimaryWeaponRuntimeState unlockedProgressionRuntime;
	if (!PrimaryWeaponExecutionSystem::InitializeRuntime(
		progressiveHeatWeapon,
		unlockedProgressionRuntime,
		&unlockedWeaponUpgrades
	).isValid || unlockedProgressionRuntime.features.size() != 1)
	{
		return Fail("A primary weapon feature did not activate at its unlock level");
	}

	const PrimaryWeaponExecutionContext unlockedHeatContext{
		heatTestOwner,
		progressiveHeatWeapon,
		progressiveHeatWeapon.attributes,
		noDamageTags,
		&unlockedWeaponUpgrades
	};
	PrimaryWeaponExecutionSystem::BeginFire(unlockedHeatContext, unlockedProgressionRuntime);
	if (!PrimaryWeaponExecutionSystem::FireOnce(unlockedHeatContext, unlockedProgressionRuntime) ||
		!NearlyEqual(unlockedProgressionRuntime.GetFeatureValue(
			PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
		), 3.f))
	{
		return Fail("A level-unlocked primary weapon feature did not execute");
	}
	PrimaryWeaponExecutionSystem::EndFire(unlockedHeatContext, unlockedProgressionRuntime);

	World liveProgressionWorld{ nullptr };
	Actor liveProgressionOwner{ &liveProgressionWorld };
	LightYearsAbilitySystemComponent liveProgressionAbilities{
		liveProgressionOwner
	};
	const sas::AbilityHandle liveProgressionHandle =
		liveProgressionAbilities.GrantAbility(progressivePrimaryAbility);
	if (!liveProgressionHandle.IsValid())
	{
		return Fail("Live primary weapon progression ability could not be granted");
	}
	liveProgressionAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	liveProgressionAbilities.Tick(0.f);
	GameAbility* liveProgressionInstance =
		liveProgressionAbilities.GetAbility(liveProgressionHandle);
	if (!liveProgressionInstance ||
		!liveProgressionInstance->GetPrimaryWeaponRuntime().features.empty())
	{
		return Fail("Live primary weapon runtime activated a locked feature");
	}
	liveProgressionAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
	liveProgressionAbilities.Tick(0.f);
	if (!liveProgressionAbilities.SetAbilityLevel(
			liveProgressionHandle,
			progressivePrimaryAbility.GetMaxLevel()
		))
	{
		return Fail("Live primary weapon ability could not reach its feature unlock level");
	}
	if (liveProgressionInstance->GetPrimaryWeaponRuntime().features.size() != 1)
	{
		return Fail("Level-up did not immediately reconfigure the live primary weapon feature set");
	}
	liveProgressionAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	liveProgressionAbilities.Tick(1.f);
	if (!NearlyEqual(
			liveProgressionInstance->GetPrimaryWeaponRuntime().GetFeatureValue(
				PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
			),
			3.f
		))
	{
		return Fail("Level-up did not reconfigure the live primary weapon feature set");
	}
	liveProgressionAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
	liveProgressionAbilities.Tick(0.f);
	if (!liveProgressionAbilities.SetAbilityLevel(liveProgressionHandle, 1))
	{
		return Fail("Live primary weapon ability could not return to level one");
	}
	if (!liveProgressionInstance->GetPrimaryWeaponRuntime().features.empty() ||
		!NearlyEqual(
			liveProgressionInstance->GetPrimaryWeaponRuntime().GetFeatureValue(
				PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
			),
			0.f
		))
	{
		return Fail("Level-down retained a removed primary weapon feature runtime value");
	}

	PrimaryWeaponDefinition invalidLevelTargetWeapon = projectileWeapon;
	invalidLevelTargetWeapon.progressionProfile = WeaponProgressionProfile{ 2 }
		.AtLevel(2, {
		sas::AttributeModifier{ AreaAttributeIds::Radius, sas::AttributeModifierOperation::Add, 10.f }
		});
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidLevelTargetWeapon).isValid)
	{
		return Fail("A primary weapon level modifier targeting an undeclared attribute was accepted");
	}

	AbilityActorDefinition genericActorDefinition{
		"Actor.Ability.Generic.Generic.Basic",
		AbilityActorType::Generic,
		1.f,
		0.f,
		{},
		"Presentation.Ability.Generic.Generic.Basic"
	};
	if (!AbilityActorRegistry::ValidateDefinition(genericActorDefinition).isValid)
	{
		return Fail("Generic ability actor definition was rejected");
	}
	AbilityActorDefinition mismatchedActorPresentation = genericActorDefinition;
	mismatchedActorPresentation.presentationProfileId =
		"Presentation.Ability.Rocket.Projectile.Basic";
	if (AbilityActorRegistry::ValidateDefinition(mismatchedActorPresentation).isValid)
	{
		return Fail("Ability actor accepted a presentation profile from another family and role");
	}
	AbilityActorDefinition mismatchedActorType = genericActorDefinition;
	mismatchedActorType.actorType = AbilityActorType::RocketProjectile;
	if (AbilityActorRegistry::ValidateDefinition(mismatchedActorType).isValid)
	{
		return Fail("Ability actor accepted a type tag from another family and role");
	}
	AbilityActorDefinition invalidGenericActorDefinition = genericActorDefinition;
	invalidGenericActorDefinition.attributes = {
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f }
	};
	if (AbilityActorRegistry::ValidateDefinition(invalidGenericActorDefinition).isValid)
	{
		return Fail("Generic ability actor accepted another mechanic family's attribute");
	}
	AbilityActorDefinition unknownActorDefinition = genericActorDefinition;
	unknownActorDefinition.actorType = static_cast<AbilityActorType>(999);
	if (AbilityActorRegistry::ValidateDefinition(unknownActorDefinition).isValid)
	{
		return Fail("Unknown ability actor type was accepted");
	}
	AreaTelegraphVisualDefinition telegraphVisual;
	AreaTelegraphActor spriteFreeTelegraph{
		nullptr,
		{ 125.f, 275.f },
		80.f,
		1.f,
		telegraphVisual
	};
	spriteFreeTelegraph.SetActorRotation(35.f);
	if (!NearlyEqual(spriteFreeTelegraph.GetActorLocation().x, 125.f) ||
		!NearlyEqual(spriteFreeTelegraph.GetActorLocation().y, 275.f) ||
		!NearlyEqual(spriteFreeTelegraph.GetActorRotation(), 35.f))
	{
		return Fail("Sprite-free actor transform failed");
	}
	if (spriteFreeTelegraph.GetRenderLayer() != RenderLayer::GroundDecal)
	{
		return Fail("Area telegraph is not rendered on the ground decal layer");
	}
	if (!RegisterSunBeamStrikeActorType() ||
		!AbilityActorRegistry::ValidateDefinition(
			LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic")
		).isValid)
	{
		return Fail("Configured Sun Beam strike actor failed validation");
	}
	AbilityActorDefinition missingSunBeamVisual =
		LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic");
	missingSunBeamVisual.presentationProfileId = sas::ContentId{};
	if (AbilityActorRegistry::ValidateDefinition(missingSunBeamVisual).isValid)
	{
		return Fail("Sun Beam strike actor accepted a missing presentation profile");
	}
	World sunBeamSpawnWorld{ nullptr };
	Actor sunBeamOwner{ &sunBeamSpawnWorld };
	weak_ptr<AbilityWorldActor> spawnedSunBeam = AbilityActorRegistry::Spawn(
		AbilityActorSpawnContext{
			sunBeamOwner,
			LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic"),
			LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic").attributes
		}
	);
	shared_ptr<AbilityWorldActor> spawnedSunBeamActor = spawnedSunBeam.lock();
	if (!spawnedSunBeamActor)
	{
		return Fail("Configured Sun Beam strike actor failed to spawn");
	}
	spawnedSunBeamActor->SetActorLocation({ 320.f, 240.f });
	spawnedSunBeamActor->ConfigureFromAttributes(
		LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic").attributes
	);
	spawnedSunBeamActor->BeginPlayInternal();
	if (spawnedSunBeamActor->GetIsPendingDestroy())
	{
		return Fail("Configured Sun Beam strike actor was destroyed during BeginPlay");
	}
	if (spawnedSunBeamActor->GetRenderLayer() != RenderLayer::WorldVfx)
	{
		return Fail("Sun Beam is not rendered above world actors");
	}

	CameraManager cameraManager;
	const sf::View defaultView{ sf::FloatRect{ { 0.f, 0.f }, { 800.f, 600.f } } };
	cameraManager.PlayShake(5.f, 0.1f, 40.f);
	const sf::Vector2f shakenCenter = cameraManager.GetView(defaultView).getCenter();
	if (NearlyEqual(shakenCenter.x, defaultView.getCenter().x) &&
		NearlyEqual(shakenCenter.y, defaultView.getCenter().y))
	{
		return Fail("Camera shake did not affect the impact frame");
	}
	cameraManager.Update(0.1f, defaultView);
	const sf::Vector2f recoveredCenter = cameraManager.GetView(defaultView).getCenter();
	if (!NearlyEqual(recoveredCenter.x, defaultView.getCenter().x) ||
		!NearlyEqual(recoveredCenter.y, defaultView.getCenter().y))
	{
		return Fail("Camera shake did not cleanly return to rest");
	}

	World cameraFollowWorld{ nullptr };
	const weak_ptr<Actor> cameraFollowTargetWeak = cameraFollowWorld.SpawnActor<Actor>();
	const shared_ptr<Actor> cameraFollowTarget = cameraFollowTargetWeak.lock();
	if (!cameraFollowTarget)
	{
		return Fail("Camera follow-offset test target could not be spawned");
	}
	cameraFollowTarget->SetActorLocation({ 100.f, 120.f });

	CameraManager followOffsetCamera;
	followOffsetCamera.SetFollowTarget(cameraFollowTargetWeak);
	followOffsetCamera.Update(0.016f, defaultView);
	const sf::Vector2f offsetBeforeDash =
		followOffsetCamera.GetView(defaultView).getCenter() -
		cameraFollowTarget->GetActorLocation();

	cameraFollowTarget->SetActorLocation({ 360.f, 80.f });
	followOffsetCamera.SetPreserveFollowTargetOffset(true);
	followOffsetCamera.Update(0.016f, defaultView);
	const sf::Vector2f offsetDuringDash =
		followOffsetCamera.GetView(defaultView).getCenter() -
		cameraFollowTarget->GetActorLocation();
	if (!NearlyEqual(offsetDuringDash.x, offsetBeforeDash.x) ||
		!NearlyEqual(offsetDuringDash.y, offsetBeforeDash.y))
	{
		return Fail("Camera did not preserve its follow-target offset during Dash displacement");
	}

	CameraManager slowDashCamera;
	slowDashCamera.SetFollowTarget(cameraFollowTargetWeak);
	slowDashCamera.Update(0.016f, defaultView);
	const float slowZoomBeforeDash =
		slowDashCamera.GetView(defaultView).getSize().x /
		defaultView.getSize().x;
	constexpr float testDashZoomOutRatio = 0.15f;
	slowDashCamera.SetRelativeAdditionalZoomOut(testDashZoomOutRatio);
	slowDashCamera.Update(0.016f, defaultView);
	const float slowZoomDuringDashEntry =
		slowDashCamera.GetView(defaultView).getSize().x /
		defaultView.getSize().x;
	const float slowDashZoomTarget = slowZoomBeforeDash * (1.f + testDashZoomOutRatio);
	if (slowZoomDuringDashEntry <= slowZoomBeforeDash ||
		slowZoomDuringDashEntry >= slowDashZoomTarget)
	{
		return Fail("Dash camera zoom did not ease outward from the existing slow-speed zoom");
	}
	for (int frame = 0; frame < 300; ++frame)
	{
		slowDashCamera.Update(0.016f, defaultView);
	}
	const float slowZoomAtDashTarget =
		slowDashCamera.GetView(defaultView).getSize().x /
		defaultView.getSize().x;
	if (!NearlyEqual(slowZoomAtDashTarget, slowDashZoomTarget))
	{
		return Fail("Dash camera zoom was not additive relative to the existing slow-speed zoom");
	}

	CameraManager fastDashCamera;
	fastDashCamera.SetFollowTarget(cameraFollowTargetWeak);
	const CameraSettings fastDashCameraSettings = fastDashCamera.GetSettings();
	fastDashCamera.SetExternalVelocity({ fastDashCameraSettings.speedForMaxZoomOut, 0.f });
	fastDashCamera.Update(0.016f, defaultView);
	const float fastZoomBeforeDash =
		fastDashCamera.GetView(defaultView).getSize().x /
		defaultView.getSize().x;
	fastDashCamera.SetRelativeAdditionalZoomOut(testDashZoomOutRatio);
	for (int frame = 0; frame < 300; ++frame)
	{
		fastDashCamera.Update(0.016f, defaultView);
	}
	const float fastZoomAtDashTarget =
		fastDashCamera.GetView(defaultView).getSize().x /
		defaultView.getSize().x;
	if (!NearlyEqual(fastZoomAtDashTarget, fastZoomBeforeDash * (1.f + testDashZoomOutRatio)) ||
		fastZoomAtDashTarget - fastZoomBeforeDash <= slowZoomAtDashTarget - slowZoomBeforeDash)
	{
		return Fail("Dash camera zoom did not scale with the current speed-based camera distance");
	}

	slowDashCamera.SetRelativeAdditionalZoomOut(0.f);
	slowDashCamera.Update(0.016f, defaultView);
	const float slowZoomDuringDashRecovery =
		slowDashCamera.GetView(defaultView).getSize().x /
		defaultView.getSize().x;
	if (slowZoomDuringDashRecovery >= slowZoomAtDashTarget ||
		slowZoomDuringDashRecovery <= slowZoomBeforeDash)
	{
		return Fail("Dash camera zoom did not recover smoothly into the normal camera system");
	}

	CameraManager continuousDashZoomCamera;
	continuousDashZoomCamera.SetFollowTarget(cameraFollowTargetWeak);
	continuousDashZoomCamera.Update(0.016f, defaultView);
	continuousDashZoomCamera.SetRelativeAdditionalZoomOut(testDashZoomOutRatio);
	float zoomBeforeLastDashFrame =
		continuousDashZoomCamera.GetView(defaultView).getSize().x /
		defaultView.getSize().x;
	float zoomOnLastDashFrame = zoomBeforeLastDashFrame;
	for (int frame = 0; frame < 15; ++frame)
	{
		zoomBeforeLastDashFrame = zoomOnLastDashFrame;
		continuousDashZoomCamera.Update(0.016f, defaultView);
		zoomOnLastDashFrame =
			continuousDashZoomCamera.GetView(defaultView).getSize().x /
			defaultView.getSize().x;
	}
	continuousDashZoomCamera.SetRelativeAdditionalZoomOut(0.f);
	continuousDashZoomCamera.Update(0.016f, defaultView);
	const float zoomImmediatelyAfterDash =
		continuousDashZoomCamera.GetView(defaultView).getSize().x /
		defaultView.getSize().x;
	if (zoomOnLastDashFrame <= zoomBeforeLastDashFrame ||
		zoomImmediatelyAfterDash < zoomOnLastDashFrame)
	{
		return Fail("Dash camera zoom velocity reversed abruptly when the Dash target ended");
	}
	for (int frame = 0; frame < 300; ++frame)
	{
		continuousDashZoomCamera.Update(0.016f, defaultView);
	}
	const float recoveredContinuousDashZoom =
		continuousDashZoomCamera.GetView(defaultView).getSize().x /
		defaultView.getSize().x;
	if (!NearlyEqual(recoveredContinuousDashZoom, slowZoomBeforeDash))
	{
		return Fail("Velocity-continuous Dash camera zoom did not settle at its normal distance");
	}

	CameraSettings boundedDashSettings;
	boundedDashSettings.worldBoundsPadding = 0.f;
	CameraManager boundedDashCamera;
	boundedDashCamera.SetSettings(boundedDashSettings);
	boundedDashCamera.SetWorldBounds(sf::FloatRect{ { 0.f, 0.f }, { 3000.f, 1500.f } });
	boundedDashCamera.SetFollowTarget(cameraFollowTargetWeak);
	cameraFollowTarget->SetActorLocation({ 2900.f, 750.f });
	boundedDashCamera.Update(0.016f, defaultView);
	boundedDashCamera.SetPreserveFollowTargetOffset(true);
	boundedDashCamera.SetRelativeAdditionalZoomOut(testDashZoomOutRatio);
	boundedDashCamera.Update(0.016f, defaultView);
	const sf::View boundedDashView = boundedDashCamera.GetView(defaultView);
	const float boundedDashRightEdge =
		boundedDashView.getCenter().x +
		boundedDashView.getSize().x * 0.5f;
	if (!NearlyEqual(boundedDashRightEdge, 3000.f))
	{
		return Fail("Dash camera zoom and arena-bound clamp did not follow the same smoothed view size");
	}

	auto SetTestAttribute = [](sas::GameplayAttributeList& testAttributes, const sas::AttributeId& id, float value)
	{
		if (sas::GameplayAttribute* attribute = sas::FindAttribute(testAttributes, id))
		{
			attribute->baseValue = value;
			attribute->currentValue = value;
		}
	};

	World timingWorld{ nullptr };
	Actor timingOwner{ &timingWorld };
	timingOwner.SetCollisionLayer(CollisionLayer::Player);
	weak_ptr<TestCombatant> timingTargetWeak = timingWorld.SpawnActor<TestCombatant>();
	shared_ptr<TestCombatant> timingTarget = timingTargetWeak.lock();
	if (!timingTarget)
	{
		return Fail("Sun Beam timing target failed to spawn");
	}
	timingTarget->SetActorLocation({ 100.f, 100.f });
	timingTarget->SetCollisionLayer(CollisionLayer::Enemy);
	timingTarget->SetCollisionMask(CollisionLayer::PlayerBullet);

	sas::GameplayAttributeList timingAttributes =
		LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic").attributes;
	SetTestAttribute(timingAttributes, CommonAttributeIds::Damage, 10.f);
	SetTestAttribute(
		timingAttributes,
		AbilityData::SunBeam::Actor::Strike::TelegraphDuration,
		0.1f
	);
	SetTestAttribute(
		timingAttributes,
		AbilityData::SunBeam::Actor::Strike::ArrivalDuration,
		0.1f
	);
	SetTestAttribute(
		timingAttributes,
		AbilityData::SunBeam::Actor::Strike::ImpactDelay,
		0.05f
	);
	SetTestAttribute(
		timingAttributes,
		AbilityData::SunBeam::Actor::Strike::ImpactVisualDuration,
		0.1f
	);
	weak_ptr<AbilityWorldActor> timedBeamWeak = AbilityActorRegistry::Spawn(
		AbilityActorSpawnContext{
			timingOwner,
			LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic"),
			timingAttributes
		}
	);
	if (shared_ptr<AbilityWorldActor> timedBeam = timedBeamWeak.lock())
	{
		timedBeam->SetActorLocation({ 100.f, 100.f });
		timedBeam->ConfigureFromAttributes(timingAttributes);
	}
	else
	{
		return Fail("Timed Sun Beam failed to spawn");
	}

	timingWorld.TickInternal(0.f);
	timingWorld.TickInternal(0.2f);
	timingWorld.TickInternal(0.049f);
	if (!NearlyEqual(timingTarget->GetHealth(), 100.f))
	{
		return Fail("Sun Beam damage opened before the post-telegraph settle delay");
	}
	timingWorld.TickInternal(0.002f);
	if (!NearlyEqual(timingTarget->GetHealth(), 90.f))
	{
		return Fail("Sun Beam damage did not synchronize with the impact frame");
	}
	timingWorld.TickInternal(0.2f);
	if (!timingWorld.GetActorsByType<AreaTelegraphActor>().empty() || !timedBeamWeak.expired())
	{
		return Fail("Sun Beam or telegraph remained after recovery completed");
	}

	World stressWorld{ nullptr };
	Actor stressOwner{ &stressWorld };
	stressOwner.SetCollisionLayer(CollisionLayer::Player);
	List<weak_ptr<AbilityWorldActor>> stressBeams;
	for (int beamIndex = 0; beamIndex < 32; ++beamIndex)
	{
		weak_ptr<AbilityWorldActor> stressBeam = AbilityActorRegistry::Spawn(
			AbilityActorSpawnContext{
				stressOwner,
				LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic"),
				LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic").attributes
			}
		);
		if (shared_ptr<AbilityWorldActor> beam = stressBeam.lock())
		{
			beam->SetActorLocation({ 200.f + beamIndex * 3.f, 200.f });
		beam->ConfigureFromAttributes(
			LoadedAbilityActor("Actor.Ability.SunBeam.Strike.Basic").attributes
		);
		}
		stressBeams.push_back(stressBeam);
	}
	stressWorld.TickInternal(0.f);
	stressWorld.TickInternal(1.f);
	stressWorld.TickInternal(1.f);
	for (const weak_ptr<AbilityWorldActor>& stressBeam : stressBeams)
	{
		if (!stressBeam.expired())
		{
			return Fail("A multi-Sun-Beam actor remained after cleanup");
		}
	}
	if (!stressWorld.GetActorsByType<AreaTelegraphActor>().empty())
	{
		return Fail("Multi-Sun-Beam telegraphs left scene residue");
	}

	Actor abilityOwner{ nullptr };
	LightYearsAbilitySystemComponent abilitySystem{ abilityOwner };
	const GameAbilityDefinition persistentHeatAbility = AbilityData::MakePrimaryFireAbilityDefinition(continuousBeamWeapon);
	const sas::AbilityHandle persistentHeatHandle = abilitySystem.GrantAbility(persistentHeatAbility);
	if (!persistentHeatHandle.IsValid())
	{
		return Fail("Continuous heat primary ability could not be granted");
	}
	abilitySystem.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	abilitySystem.Tick(1.f);
	GameAbility* persistentHeatInstance = abilitySystem.GetAbility(sas::AbilitySlot::PrimaryFire);
	if (!persistentHeatInstance || !NearlyEqual(persistentHeatInstance->GetPrimaryWeaponRuntime().GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 50.f))
	{
		return Fail("Continuous heat was not retained by the primary ability runtime");
	}
	abilitySystem.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
	abilitySystem.Tick(1.f);
	if (!NearlyEqual(persistentHeatInstance->GetPrimaryWeaponRuntime().GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 30.f))
	{
		return Fail("Released continuous heat did not dissipate through the ability runtime");
	}
	abilitySystem.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	abilitySystem.Tick(0.f);
	if (!NearlyEqual(persistentHeatInstance->GetPrimaryWeaponRuntime().GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 30.f))
	{
		return Fail("Reactivated continuous heat did not preserve its partially cooled value");
	}
	abilitySystem.RemoveAbility(persistentHeatHandle);
	GameAbilityDefinition invalidSpawnActorAbility;
	invalidSpawnActorAbility.abilityId = "Ability.Test.InvalidSpawnActor";
	invalidSpawnActorAbility.slot = sas::AbilitySlot::Ability1;
	invalidSpawnActorAbility.actions = {
		AbilityActionSpec{
			sas::AbilityActionPhase::OnActivate,
			SpawnActorAction{ "Actor.Test.Unknown", sas::AbilitySpawnPolicy::AtOwner },
			0.f,
			1
		}
	};
	std::string invalidSpawnActorReason;
	if (abilitySystem.GrantAbility(invalidSpawnActorAbility, &invalidSpawnActorReason).IsValid() ||
		invalidSpawnActorReason.empty())
	{
		return Fail("Ability grant accepted an invalid actor definition");
	}
	GameAbilityDefinition invalidEffectAbility;
	invalidEffectAbility.abilityId = "Ability.Test.InvalidEffect";
	invalidEffectAbility.slot = sas::AbilitySlot::Ability1;
	invalidEffectAbility.actions = {
		AbilityActionSpec{
			sas::AbilityActionPhase::OnActivate,
			ApplyEffectAction{
				"Effect.Test.Unknown",
				sas::AbilityTargetPolicy::Self
			},
			0.f,
			1
		}
	};
	std::string invalidEffectReason;
	if (abilitySystem.GrantAbility(invalidEffectAbility, &invalidEffectReason).IsValid() ||
		invalidEffectReason.empty())
	{
		return Fail("Ability grant accepted an unknown gameplay-effect definition");
	}
	std::string sunBeamGrantFailure;
	const GameAbilityDefinition* shippedSunBeamDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::SunBeam::AbilityId::Strike::Basic);
	if (!shippedSunBeamDefinition ||
		!abilitySystem.GrantAbility(*shippedSunBeamDefinition, &sunBeamGrantFailure).IsValid())
	{
		return Fail("Configured Sun Beam ability failed grant validation");
	}
	if (shippedSunBeamDefinition->GetMaxLevel() != 5 ||
		shippedSunBeamDefinition->levelProgression.size() != 4)
	{
		return Fail("Repeated ability level progression produced the wrong level count");
	}

	const std::string firstUpgradeId = "Upgrade.Ability.Test.First";
	const std::string secondUpgradeId = "Upgrade.Ability.Test.Second";
	GameAbilityDefinition progressionAbility;
	progressionAbility.abilityId = "Ability.Utility.Test.Progression";
	progressionAbility.slot = sas::AbilitySlot::Ability3;
	progressionAbility.abilityTags = {
		GameplayTagSchema::AbilityUtility,
		GameplayTag{ "Ability.Utility.Test" }
	};
	progressionAbility.duration = 4.f;
	progressionAbility.attributeModifiers = {
		sas::AttributeModifier{ CommonAttributeIds::Radius, sas::AttributeModifierOperation::Add, 1.f }
	};
	progressionAbility.levelProgression = {
		AbilityLevelStep{
			{
				sas::AttributeModifier{ CommonAttributeIds::Damage, sas::AttributeModifierOperation::Add, 5.f }
			},
			{ firstUpgradeId },
			{
				AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					ApplyImpulseAction{ 10.f, sas::AbilityDirectionPolicy::OwnerForward },
					0.f,
					1
				}
			},
			{}
		},
		AbilityLevelStep{
			{
				sas::AttributeModifier{ CommonAttributeIds::Damage, sas::AttributeModifierOperation::Multiply, 2.f },
				sas::AttributeModifier{ CommonAttributeIds::Duration, sas::AttributeModifierOperation::Add, 2.f }
			},
			{ secondUpgradeId },
			{},
			{
				AbilityTriggerSpec{
					GameplayTag{ "Event.Test.Progression" },
					0.f,
					{},
					{},
					{},
					{},
					{},
					{},
					false,
					0,
					{},
					{},
					{
						AbilityActionSpec{
							sas::AbilityActionPhase::OnActivate,
							ApplyImpulseAction{ 5.f, sas::AbilityDirectionPolicy::OwnerForward },
							0.f,
							1
						}
					}
				}
			}
		}
	};
	const sas::AbilityHandle progressionHandle = abilitySystem.GrantAbility(progressionAbility);
	GameAbility* progressionInstance = abilitySystem.GetAbility(progressionHandle);
	if (!progressionInstance)
	{
		return Fail("Per-ability level progression failed to grant");
	}
	if (!abilitySystem.SetAbilityLevel(progressionHandle, 999))
	{
		return Fail("LightYearsAbilitySystemComponent failed to set an ability level");
	}
	const GameAbilityDefinition& maxLevelDefinition = progressionInstance->GetDefinition();
	const sas::AbilityRuntimeSnapshot maxLevelSnapshot = progressionInstance->BuildSnapshot();
	const float progressedDamage = sas::CalculateModifiedAttributeValue(
		sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
		maxLevelDefinition.attributeModifiers
	);
	if (progressionInstance->GetLevel() != 3 ||
		maxLevelSnapshot.level != 3 ||
		maxLevelSnapshot.maxLevel != 3 ||
		!NearlyEqual(progressedDamage, 30.f) ||
		!NearlyEqual(progressionInstance->GetActiveDuration(), 6.f) ||
		!maxLevelDefinition.HasUnlockedUpgrade(firstUpgradeId) ||
		!maxLevelDefinition.HasUnlockedUpgrade(secondUpgradeId) ||
		maxLevelDefinition.actions.size() != 1 ||
		maxLevelDefinition.triggers.size() != 1)
	{
		return Fail("Per-ability level steps did not compose modifiers, upgrades, actions, and triggers");
	}
	if (abilitySystem.LevelUpAbility(progressionHandle))
	{
		return Fail("LightYearsAbilitySystemComponent leveled an ability beyond its maximum level");
	}
	if (!abilitySystem.SetAbilityLevel(sas::AbilitySlot::Ability3, 1))
	{
		return Fail("LightYearsAbilitySystemComponent failed to restore an ability level");
	}
	const GameAbilityDefinition& resetDefinition = progressionInstance->GetDefinition();
	if (resetDefinition.attributeModifiers.size() != 1 ||
		!resetDefinition.unlockedUpgradeIds.empty() ||
		!resetDefinition.actions.empty() ||
		!resetDefinition.triggers.empty() ||
		!NearlyEqual(progressionInstance->GetActiveDuration(), 4.f))
	{
		return Fail("Ability level reset did not rebuild from the base definition");
	}
	if (!abilitySystem.LevelUpAbility(sas::AbilitySlot::Ability3) || progressionInstance->GetLevel() != 2)
	{
		return Fail("LightYearsAbilitySystemComponent failed to level up an ability");
	}

	GameAbilityDefinition invalidProgressionAbility;
	invalidProgressionAbility.abilityId = "Ability.Utility.Test.InvalidProgression";
	invalidProgressionAbility.slot = sas::AbilitySlot::Ability4;
	invalidProgressionAbility.abilityTags = {
		GameplayTagSchema::AbilityUtility,
		GameplayTag{ "Ability.Utility.Test" }
	};
	invalidProgressionAbility.levelProgression = {
		AbilityLevelStep{
			{},
			{ std::string{ "Upgrade.Ability.Test.Invalid" } },
			{
				AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					SpawnActorAction{ "Actor.Test.Unknown", sas::AbilitySpawnPolicy::AtOwner },
					0.f,
					1
				}
			},
			{}
		}
	};
	std::string invalidProgressionReason;
	if (abilitySystem.GrantAbility(invalidProgressionAbility, &invalidProgressionReason).IsValid() ||
		invalidProgressionReason.empty())
	{
		return Fail("Ability grant accepted an invalid level-step action");
	}
	GameAbilityDefinition passiveOne;
	passiveOne.abilityId = "Ability.Utility.Test.Passive.One";
	passiveOne.slot = sas::AbilitySlot::None;
	passiveOne.abilityTags = {
		GameplayTagSchema::AbilityUtility,
		GameplayTag{ "Ability.Utility.Test" }
	};
	passiveOne.activationPolicy = sas::AbilityActivationPolicy::Passive;
	passiveOne.lifetimePolicy = sas::AbilityLifetimePolicy::UntilCancelled;
	GameAbilityDefinition passiveTwo = passiveOne;
	passiveTwo.abilityId = "Ability.Utility.Test.Passive.Two";
	GameAbilityDefinition passiveThree = passiveOne;
	passiveThree.abilityId = "Ability.Utility.Test.Passive.Three";
	const sas::AbilityHandle passiveOneHandle = abilitySystem.GrantAbility(passiveOne);
	const sas::AbilityHandle passiveTwoHandle = abilitySystem.GrantAbility(passiveTwo);
	const sas::AbilityHandle passiveThreeHandle = abilitySystem.GrantAbility(passiveThree);
	if (!passiveOneHandle.IsValid() || !passiveTwoHandle.IsValid() || passiveThreeHandle.IsValid() ||
		abilitySystem.GetPassiveAbilityCount() != LightYearsAbilitySystemComponent::MaxPassiveAbilities ||
		abilitySystem.GetAbility(passiveOneHandle) == nullptr ||
		abilitySystem.GetAbilityById(passiveTwo.abilityId) == nullptr)
	{
		return Fail("Passive ability handle ownership or limit failed");
	}

	TestCombatant damageTarget;
	damageTarget.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::Armor, 50.f }
	);
	sas::GameplayEffectDefinition integrationBarrier;
	integrationBarrier.effectId = "Effect.Test.IntegrationBarrier";
	integrationBarrier.behaviorKey = EffectBehaviorKeys::Barrier;
	integrationBarrier.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
	integrationBarrier.duration = 10.f;
	integrationBarrier.attributes = {
		sas::GameplayAttribute{ BarrierEffectSchema::Capacity, 20.f, 0.f },
		sas::GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f }
	};
	damageTarget.GetAbilitySystemComponent().ApplyGameplayEffect(integrationBarrier);
	ApplyCombatDamage(damageTarget, 30.f);
	if (!NearlyEqual(damageTarget.GetHealth(), 100.f - 10.f * (100.f / 150.f)))
	{
		return Fail("Combat damage did not flow through barrier, armor, and health");
	}

	World weaponCadenceWorld{ nullptr };
	Actor weaponCadenceOwner{ &weaponCadenceWorld };
	LightYearsAbilitySystemComponent weaponCadenceAbilities{
		weaponCadenceOwner
	};
	PrimaryWeaponDefinition weaponCadenceDefinition{
		"Weapon.Projectile.TestCadence.Basic",
		PrimaryWeaponType::ProjectileStandard,
		WeaponPresentationDefinition{},
		{
			sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
			sas::GameplayAttribute{ CommonAttributeIds::FireRate, 2.f, 0.01f },
			sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 100.f, 0.f },
			sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 10.f, 0.f },
			sas::GameplayAttribute{ CommonAttributeIds::Range, 10000.f, 0.f },
			sas::GameplayAttribute{ CollisionAttributeIds::Radius, 7.f, 0.1f }
		},
		{},
		true
	};
	if (!weaponCadenceAbilities.GrantAbility(
		AbilityData::MakePrimaryFireAbilityDefinition(weaponCadenceDefinition)
	).IsValid())
	{
		return Fail("Weapon cadence test ability could not be granted");
	}
	// A producer grants the shared lock; PrimaryFire must consume it without
	// needing a weapon- or ability-specific blocked tag in its definition.
	weaponCadenceAbilities.AddOwnedTag(GameplayTagSchema::BlockPrimaryWeaponFire);
	weaponCadenceAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	weaponCadenceAbilities.Tick(0.f);
	weaponCadenceWorld.TickInternal(0.f);
	if (!weaponCadenceWorld.GetActorsByType<PrimaryWeaponProjectileActor>().empty())
	{
		return Fail("Shared primary-weapon action lock did not block activation");
	}
	weaponCadenceAbilities.RemoveOwnedTag(GameplayTagSchema::BlockPrimaryWeaponFire);
	weaponCadenceAbilities.Tick(0.f);
	weaponCadenceWorld.TickInternal(0.f);
	if (weaponCadenceWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 1)
	{
		return Fail("Weapon cadence test did not fire its initial shot");
	}
	weaponCadenceAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, false);
	weaponCadenceAbilities.Tick(0.1f);
	weaponCadenceAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	weaponCadenceAbilities.Tick(0.1f);
	weaponCadenceWorld.TickInternal(0.f);
	if (weaponCadenceWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 1)
	{
		return Fail("Re-pressing primary fire bypassed its fire-rate interval");
	}
	weaponCadenceAbilities.Tick(0.29f);
	weaponCadenceWorld.TickInternal(0.f);
	if (weaponCadenceWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 1)
	{
		return Fail("Primary fire completed its interval too early");
	}
	weaponCadenceAbilities.Tick(0.02f);
	weaponCadenceWorld.TickInternal(0.f);
	if (weaponCadenceWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 2)
	{
		return Fail("Primary fire did not resume after its fire-rate interval");
	}
	weaponCadenceAbilities.Tick(1.01f);
	weaponCadenceWorld.TickInternal(0.f);
	if (weaponCadenceWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 4)
	{
		return Fail("Primary fire discarded owed shots after a long frame");
	}
	GameAbility* activePrimaryWeapon =
		weaponCadenceAbilities.GetAbility(sas::AbilitySlot::PrimaryFire);
	if (!activePrimaryWeapon || !activePrimaryWeapon->IsActive())
	{
		return Fail("Primary weapon cadence test was not active before interruption");
	}
	weaponCadenceAbilities.AddOwnedTag(GameplayTagSchema::BlockPrimaryWeaponFire);
	if (activePrimaryWeapon->IsActive() ||
		weaponCadenceAbilities.GetOwnedTags().HasTag(
			GameplayTagSchema::BlockPrimaryWeaponFire
		) == false)
	{
		return Fail("Primary weapon action lock did not interrupt active fire");
	}
	weaponCadenceAbilities.Tick(0.5f);
	weaponCadenceWorld.TickInternal(0.f);
	if (weaponCadenceWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 4)
	{
		return Fail("Primary weapon fired while the shared action lock was active");
	}

	World sweptProjectileWorld{ nullptr };
	Actor sweptProjectileOwner{ &sweptProjectileWorld };
	sweptProjectileOwner.SetCollisionLayer(CollisionLayer::Player);
	const shared_ptr<TestCombatant> sweptProjectileTarget =
		sweptProjectileWorld.SpawnActor<TestCombatant>().lock();
	if (!sweptProjectileTarget)
	{
		return Fail("Swept projectile target could not spawn");
	}
	sweptProjectileTarget->SetActorLocation({ 100.f, 0.f });
	sweptProjectileTarget->SetCollisionLayer(CollisionLayer::Enemy);
	sweptProjectileTarget->SetCollisionMask(CollisionLayer::PlayerBullet);
	const sas::GameplayAttributeList sweptProjectileAttributes{
		sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 1000.f, 0.f },
		sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 2.f, 0.f },
		sas::GameplayAttribute{ CommonAttributeIds::Range, 1000.f, 0.f },
		sas::GameplayAttribute{ CollisionAttributeIds::Radius, 5.f, 0.1f }
	};
	const shared_ptr<PrimaryWeaponProjectileActor> sweptProjectile =
		sweptProjectileWorld.SpawnActor<PrimaryWeaponProjectileActor>(
			&sweptProjectileOwner,
			WeaponPresentationDefinition{},
			sweptProjectileAttributes
		).lock();
	if (!sweptProjectile)
	{
		return Fail("Swept projectile could not spawn");
	}
	sweptProjectile->SetActorLocation({ 0.f, 0.f });
	sweptProjectile->SetLaunchVelocity({ 1000.f, 0.f });
	sweptProjectileWorld.TickInternal(0.f);
	if (sweptProjectile->HasPhysicsBody())
	{
		return Fail("Primary projectile created a redundant physics body");
	}
	sweptProjectileWorld.TickInternal(0.2f);
	if (!NearlyEqual(sweptProjectileTarget->GetHealth(), 90.f))
	{
		return Fail("High-speed primary projectile tunneled through its target");
	}

	// A reflection must move the projectile out of the surface before the
	// same-surface lock starts. Without that separation, the next tick can
	// interpret the still-overlapping wall as a normal projectile impact.
	World crystalReflectionWorld{ nullptr };
	// A stationary enemy must be displaced when Lance moves across it. Testing
	// only an enemy's requested movement misses the owner-following wall case.
	{
		World wallWorld{ nullptr };
		auto owner = wallWorld.SpawnActor<TestCombatant>().lock();
		auto target = wallWorld.SpawnActor<DummyEnemy>(ShipData::Ship_Enemy_Hexagon).lock();
		owner->SetCollisionLayer(CollisionLayer::Player);
		owner->GetAbilitySystemComponent().AddOwnedTag(AbilityData::LanceDrive::State::Active);
		owner->SetActorLocation({ 1000.f, 1000.f });
		owner->SetActorRotation(0.f);
		target->SetCollisionLayer(CollisionLayer::Enemy);
		target->SetActorLocation({ 1091.f, 820.f });
		target->SetVelocity({});
		auto wall = wallWorld.SpawnActor<LanceDriveActor>(owner.get(), LanceDrivePresentationProfile{}).lock();
		wallWorld.TickInternal(0.f);
		const sf::Vector2f before = target->GetActorLocation();
		owner->SetActorLocation({ 1000.f, 960.f });
		wall->Tick(0.1f);
		if (GetVectorLength(target->GetActorLocation() - before) < 1.f)
		{
			return Fail("Moving Lance wall did not separate a stationary enemy");
		}
		const sf::Vector2f separated = target->GetActorLocation();
		target->Tick(0.1f);
		if (GetVectorLength(target->GetActorLocation() - separated) < 0.1f)
		{
			return Fail("Lance contact did not produce movement-component knockback");
		}
	}
	Actor crystalReflectionOwner{ &crystalReflectionWorld };
	crystalReflectionOwner.SetCollisionLayer(CollisionLayer::Player);
	const shared_ptr<CrystalBarricadeActor> crystalWall =
		crystalReflectionWorld.SpawnActor<CrystalBarricadeActor>(
			&crystalReflectionOwner,
			CrystalBarricadeWallPresentationProfile{}
		).lock();
	const shared_ptr<PrimaryWeaponProjectileActor> crystalReflectionProjectile =
		crystalReflectionWorld.SpawnActor<PrimaryWeaponProjectileActor>(
			&crystalReflectionOwner,
			WeaponPresentationDefinition{},
			sweptProjectileAttributes
		).lock();
	if (!crystalWall || !crystalReflectionProjectile)
	{
		return Fail("Crystal Barricade reflection test actors could not spawn");
	}
	crystalWall->SetActorLocation({ 300.f, 0.f });
	crystalWall->SetActorRotation(0.f);
	crystalReflectionProjectile->SetActorLocation({ 0.f, 0.f });
	crystalReflectionProjectile->SetLaunchVelocity({ 1000.f, 0.f });
	crystalReflectionWorld.TickInternal(0.f);
	crystalReflectionWorld.TickInternal(0.2f);
	crystalReflectionWorld.TickInternal(0.02f);
	if (crystalReflectionProjectile->GetIsPendingDestroy() ||
		crystalReflectionProjectile->GetVelocity().x >= 0.f ||
		crystalReflectionProjectile->GetActorLocation().x >= 150.f)
	{
		return Fail("Crystal Barricade did not keep a reflected projectile outside its surface");
	}

	// Return Protocol is a receiver registration, not a condition hard-coded
	// into the player ship. A compatible hostile projectile must be reflected
	// before it resolves damage, then stop reflecting after the one-second window.
	{
	World returnProtocolWorld{ nullptr };
	const shared_ptr<TestCombatant> returnProtocolDefender =
		returnProtocolWorld.SpawnActor<TestCombatant>().lock();
	const shared_ptr<TestCombatant> returnProtocolAttacker =
		returnProtocolWorld.SpawnActor<TestCombatant>().lock();
	if (!returnProtocolDefender || !returnProtocolAttacker)
	{
		return Fail("Return Protocol test combatants could not spawn");
	}
	returnProtocolDefender->SetCollisionLayer(CollisionLayer::Player);
	returnProtocolDefender->SetCollisionMask(CollisionLayer::EnemyBullet);
	returnProtocolAttacker->SetCollisionLayer(CollisionLayer::Enemy);
	returnProtocolAttacker->SetCollisionMask(CollisionLayer::PlayerBullet);
	returnProtocolAttacker->SetActorLocation({ -100.f, 0.f });
	if (!returnProtocolDefender->GetAbilitySystemComponent().GrantAbility(
		AbilityData::Definitions::ReturnProtocol_Basic
	).IsValid())
	{
		return Fail("Return Protocol ability could not be granted");
	}
	returnProtocolDefender->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability1,
		true
	);
	returnProtocolDefender->GetAbilitySystemComponent().Tick(0.f);
	returnProtocolDefender->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability1,
		false
	);
	if (!returnProtocolDefender->GetAbilitySystemComponent().HasOwnedTag(
		AbilityData::ReturnProtocol::State::Active
	))
	{
		return Fail("Return Protocol did not enter its active reflection window");
	}
	returnProtocolWorld.TickInternal(0.f);
	if (returnProtocolWorld.GetActorsByType<ReturnProtocolVisualActor>().empty())
	{
		return Fail("Return Protocol did not spawn its active-window visual");
	}
	const shared_ptr<PrimaryWeaponProjectileActor> reflectedProjectile =
		returnProtocolWorld.SpawnActor<PrimaryWeaponProjectileActor>(
			returnProtocolAttacker.get(),
			WeaponPresentationDefinition{},
			sweptProjectileAttributes
		).lock();
	if (!reflectedProjectile)
	{
		return Fail("Return Protocol incoming projectile could not spawn");
	}
	reflectedProjectile->SetActorLocation({ -10.f, 0.f });
	reflectedProjectile->SetLaunchVelocity({ 1000.f, 0.f });
	returnProtocolWorld.TickInternal(0.f);
	reflectedProjectile->OnActorBeginOverlap(returnProtocolDefender.get());
	if (!NearlyEqual(returnProtocolDefender->GetHealth(), 100.f) ||
		reflectedProjectile->GetOwnerActor() != returnProtocolDefender.get() ||
		reflectedProjectile->GetOriginalProjectileOwner() != returnProtocolAttacker.get() ||
		!NearlyEqual(reflectedProjectile->GetDamage(), 8.f) ||
		reflectedProjectile->GetVelocity().x >= 0.f)
	{
		return Fail("Return Protocol did not preserve and reverse the incoming projectile");
	}
	returnProtocolDefender->GetAbilitySystemComponent().Tick(1.01f);
	if (returnProtocolDefender->GetAbilitySystemComponent().HasOwnedTag(
		AbilityData::ReturnProtocol::State::Active
	))
	{
		return Fail("Return Protocol reflection window did not end after one second");
	}
	returnProtocolWorld.TickInternal(0.f);
	if (!returnProtocolWorld.GetActorsByType<ReturnProtocolVisualActor>().empty())
	{
		return Fail("Return Protocol did not clean up its active-window visual");
	}
	const shared_ptr<PrimaryWeaponProjectileActor> unreflectedProjectile =
		returnProtocolWorld.SpawnActor<PrimaryWeaponProjectileActor>(
			returnProtocolAttacker.get(),
			WeaponPresentationDefinition{},
			sweptProjectileAttributes
		).lock();
	if (!unreflectedProjectile)
	{
		return Fail("Return Protocol post-window projectile could not spawn");
	}
	unreflectedProjectile->OnActorBeginOverlap(returnProtocolDefender.get());
	if (!NearlyEqual(returnProtocolDefender->GetHealth(), 90.f) ||
		unreflectedProjectile->GetOwnerActor() != returnProtocolAttacker.get())
	{
		return Fail("Return Protocol reflected a projectile outside its active window");
	}
	}

	World dualKineticWorld{ nullptr };
	Actor dualKineticOwner{ &dualKineticWorld };
	LightYearsAbilitySystemComponent dualKineticAbilities{
		dualKineticOwner
	};
	if (!dualKineticAbilities.GrantAbility(
		AbilityData::MakePrimaryFireAbilityDefinition(LoadedWeapon("Weapon.Projectile.DualKineticBlaster.Basic"))
	).IsValid())
	{
		return Fail("Dual kinetic blaster ability could not be granted");
	}
	dualKineticAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	dualKineticAbilities.Tick(0.f);
	dualKineticWorld.TickInternal(0.f);
	if (dualKineticWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 2)
	{
		return Fail("Dual kinetic blaster did not fire one projectile from each muzzle");
	}

	World electricArcWorld{ nullptr };
	Actor electricArcOwner{ &electricArcWorld };
	electricArcOwner.SetCollisionLayer(CollisionLayer::Player);
	LightYearsAbilitySystemComponent electricArcAbilities{
		electricArcOwner
	};
	const shared_ptr<TestCombatant> firstArcTarget = electricArcWorld.SpawnActor<TestCombatant>().lock();
	const shared_ptr<TestCombatant> secondArcTarget = electricArcWorld.SpawnActor<TestCombatant>().lock();
	const shared_ptr<TestCombatant> thirdArcTarget = electricArcWorld.SpawnActor<TestCombatant>().lock();
	if (!firstArcTarget || !secondArcTarget || !thirdArcTarget)
	{
		return Fail("Electric arc targets could not be spawned");
	}
	for (const shared_ptr<TestCombatant>& target : { firstArcTarget, secondArcTarget, thirdArcTarget })
	{
		target->SetCollisionLayer(CollisionLayer::Enemy);
		target->SetCollisionMask(CollisionLayer::PlayerBullet);
	}
	firstArcTarget->SetActorLocation({ 0.f, 100.f });
	secondArcTarget->SetActorLocation({ 100.f, 100.f });
	thirdArcTarget->SetActorLocation({ 200.f, 100.f });
	electricArcWorld.TickInternal(0.f);
	if (!electricArcAbilities.GrantAbility(
		AbilityData::MakePrimaryFireAbilityDefinition(LoadedWeapon("Weapon.Arc.ElectricLauncher.Basic"))
	).IsValid())
	{
		return Fail("Electric arc launcher ability could not be granted");
	}
	electricArcAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	electricArcAbilities.Tick(0.f);
	electricArcWorld.TickInternal(0.f);
	if (!electricArcWorld.GetActorsByType<PrimaryWeaponProjectileActor>().empty() ||
		electricArcWorld.GetActorsByType<ElectricArcVisualActor>().size() != 3)
	{
		return Fail("Electric arc launcher did not create three target-to-target electric arcs");
	}
	if (!NearlyEqual(firstArcTarget->GetHealth(), 85.f) ||
		!NearlyEqual(secondArcTarget->GetHealth(), 89.2f) ||
		!NearlyEqual(thirdArcTarget->GetHealth(), 92.224f))
	{
		return Fail("Electric arc launcher did not chain to nearby targets with damage falloff");
	}

	ElectricArcShotSample oneTargetArc;
	ElectricArcShotSample twoTargetArc;
	ElectricArcShotSample fourTargetArc;
	ElectricArcShotSample fiveTargetArc;
	ElectricArcShotSample fourTargetHighLuckArc;
	ElectricArcShotSample noTargetHighLuckArc;
	if (!FireElectricArcShot(1, 0.f, oneTargetArc) ||
		!FireElectricArcShot(2, 0.f, twoTargetArc) ||
		!FireElectricArcShot(4, 0.f, fourTargetArc) ||
		!FireElectricArcShot(5, 0.f, fiveTargetArc) ||
		!FireElectricArcShot(4, 100000.f, fourTargetHighLuckArc, false) ||
		!FireElectricArcShot(0, 100000.f, noTargetHighLuckArc, false) ||
		oneTargetArc.targetDamage.size() != 1 || twoTargetArc.targetDamage.size() != 2 ||
		fourTargetArc.targetDamage.size() != 4 || fiveTargetArc.targetDamage.size() != 5 ||
		fourTargetHighLuckArc.targetDamage.size() != 4 || !noTargetHighLuckArc.targetDamage.empty())
	{
		return Fail("Electric arc target-count runtime test setup failed");
	}
	if (!NearlyEqual(oneTargetArc.targetDamage[0], 15.f) ||
		!NearlyEqual(twoTargetArc.targetDamage[1], 10.8f) ||
		!NearlyEqual(fourTargetArc.targetDamage[3], 5.59872f) ||
		!NearlyEqual(fiveTargetArc.targetDamage[4], 0.f) ||
		!NearlyEqual(fourTargetHighLuckArc.targetDamage[0], 15.f) ||
		!NearlyEqual(fourTargetHighLuckArc.targetDamage[3], 5.59872f))
	{
		return Fail("Electric arc did not preserve normal-chain falloff, target exclusion, or no-target behavior");
	}
	const float highLuckBonusChainRate = MeasureHighLuckElectricBonusChainRate(1000);
	if (highLuckBonusChainRate < 0.29f || highLuckBonusChainRate > 0.41f)
	{
		return Fail("Electric arc high-Luck extra-chain rate did not converge near the 35 percent cap");
	}

	World emptyArcWorld{ nullptr };
	Actor emptyArcOwner{ &emptyArcWorld };
	emptyArcOwner.SetCollisionLayer(CollisionLayer::Player);
	PrimaryWeaponRuntimeState emptyArcRuntime;
	if (!PrimaryWeaponExecutionSystem::InitializeRuntime(
		LoadedWeapon("Weapon.Arc.ElectricLauncher.Basic"),
		emptyArcRuntime
	).isValid)
	{
		return Fail("Electric arc launcher runtime could not be initialized without targets");
	}
	const PrimaryWeaponExecutionContext emptyArcContext{
		emptyArcOwner,
		LoadedWeapon("Weapon.Arc.ElectricLauncher.Basic"),
		LoadedWeapon("Weapon.Arc.ElectricLauncher.Basic").attributes,
		LoadedWeapon("Weapon.Arc.ElectricLauncher.Basic").damageTags
	};
	PrimaryWeaponExecutionSystem::BeginFire(emptyArcContext, emptyArcRuntime);
	if (!PrimaryWeaponExecutionSystem::FireOnce(emptyArcContext, emptyArcRuntime))
	{
		return Fail("Electric arc launcher could not fire without targets");
	}
	emptyArcWorld.TickInternal(0.f);
	if (emptyArcWorld.GetActorsByType<ElectricArcVisualActor>().size() != 1 ||
		!emptyArcWorld.GetActorsByType<PrimaryWeaponProjectileActor>().empty())
	{
		return Fail("Electric arc launcher did not show a placeholder arc without targets");
	}

	World cryoWaveWorld{ nullptr };
	Actor cryoWaveOwner{ &cryoWaveWorld };
	cryoWaveOwner.SetCollisionLayer(CollisionLayer::Player);
	const shared_ptr<TestCombatant> leftCryoWaveTarget =
		cryoWaveWorld.SpawnActor<TestCombatant>().lock();
	const shared_ptr<TestCombatant> rightCryoWaveTarget =
		cryoWaveWorld.SpawnActor<TestCombatant>().lock();
	if (!leftCryoWaveTarget || !rightCryoWaveTarget)
	{
		return Fail("Cryo wave targets could not be spawned");
	}
	for (const shared_ptr<TestCombatant>& target :
		{ leftCryoWaveTarget, rightCryoWaveTarget })
	{
		target->SetCollisionLayer(CollisionLayer::Enemy);
		target->SetCollisionMask(CollisionLayer::PlayerBullet);
	}
	leftCryoWaveTarget->SetActorLocation({ -35.f, -120.f });
	rightCryoWaveTarget->SetActorLocation({ 35.f, -120.f });
	cryoWaveWorld.TickInternal(0.f);

	PrimaryWeaponRuntimeState cryoWaveRuntime;
	const PrimaryWeaponValidationResult cryoWaveValidation =
		PrimaryWeaponExecutionSystem::InitializeRuntime(
		LoadedWeapon("Weapon.Wave.CryoProjector.Basic"),
		cryoWaveRuntime
	);
	if (!cryoWaveValidation.isValid)
	{
		const std::string validationFailure =
			"Cryo wave projector runtime could not be initialized: " +
			cryoWaveValidation.reason;
		return Fail(validationFailure.c_str());
	}
	const PrimaryWeaponExecutionContext cryoWaveContext{
		cryoWaveOwner,
		LoadedWeapon("Weapon.Wave.CryoProjector.Basic"),
		LoadedWeapon("Weapon.Wave.CryoProjector.Basic").attributes,
		LoadedWeapon("Weapon.Wave.CryoProjector.Basic").damageTags
	};
	PrimaryWeaponExecutionSystem::BeginFire(cryoWaveContext, cryoWaveRuntime);
	if (!PrimaryWeaponExecutionSystem::FireOnce(
		cryoWaveContext,
		cryoWaveRuntime
	))
	{
		return Fail("Cryo wave projector could not fire");
	}
	cryoWaveWorld.TickInternal(0.f);
	if (cryoWaveWorld.GetActorsByType<ExpandingWaveWeaponActor>().size() != 1)
	{
		return Fail("Cryo wave projector did not spawn an expanding wave");
	}
	cryoWaveWorld.TickInternal(0.2f);
	if (!NearlyEqual(leftCryoWaveTarget->GetHealth(), 93.f) ||
		!NearlyEqual(rightCryoWaveTarget->GetHealth(), 93.f))
	{
		return Fail("Cryo wave did not pierce and damage multiple targets");
	}
	cryoWaveWorld.TickInternal(0.2f);
	if (!NearlyEqual(leftCryoWaveTarget->GetHealth(), 93.f) ||
		!NearlyEqual(rightCryoWaveTarget->GetHealth(), 93.f))
	{
		return Fail("A single Cryo wave damaged the same target more than once");
	}
	PrimaryWeaponExecutionSystem::EndFire(cryoWaveContext, cryoWaveRuntime);

	World shotgunImpactWorld{ nullptr };
	const weak_ptr<TestCombatant> shotgunSourceWeak = shotgunImpactWorld.SpawnActor<TestCombatant>();
	const weak_ptr<TestCombatant> shotgunTargetWeak = shotgunImpactWorld.SpawnActor<TestCombatant>();
	const shared_ptr<TestCombatant> shotgunSource = shotgunSourceWeak.lock();
	const shared_ptr<TestCombatant> shotgunTarget = shotgunTargetWeak.lock();
	if (!shotgunSource || !shotgunTarget)
	{
		return Fail("Shotgun impact test combatants could not be spawned");
	}
	ShotgunVolleyImpactGroup threePelletGroup{ *shotgunSource, {}, 10.f, 0.1f, 0.5f };
	for (int pelletIndex = 0; pelletIndex < 3; ++pelletIndex)
	{
		threePelletGroup.AddPellet();
	}
	for (int pelletIndex = 0; pelletIndex < 3; ++pelletIndex)
	{
		threePelletGroup.RegisterImpact(*shotgunTarget);
	}
	if (!NearlyEqual(shotgunTarget->GetHealth(), 73.f))
	{
		return Fail("Shotgun pellets were not applied immediately with progressive falloff");
	}
	for (int pelletIndex = 0; pelletIndex < 3; ++pelletIndex)
	{
		threePelletGroup.CompletePellet();
	}
	if (!NearlyEqual(shotgunTarget->GetHealth(), 73.f))
	{
		return Fail("Completing shotgun projectiles changed already-resolved pellet damage");
	}

	const weak_ptr<TestCombatant> splitShotTargetWeak = shotgunImpactWorld.SpawnActor<TestCombatant>();
	const weak_ptr<TestCombatant> singleShotTargetWeak = shotgunImpactWorld.SpawnActor<TestCombatant>();
	const shared_ptr<TestCombatant> splitShotTarget = splitShotTargetWeak.lock();
	const shared_ptr<TestCombatant> singleShotTarget = singleShotTargetWeak.lock();
	if (!splitShotTarget || !singleShotTarget)
	{
		return Fail("Shotgun split-impact test combatants could not be spawned");
	}
	ShotgunVolleyImpactGroup splitImpactGroup{ *shotgunSource, {}, 10.f, 0.1f, 0.5f };
	for (int pelletIndex = 0; pelletIndex < 3; ++pelletIndex)
	{
		splitImpactGroup.AddPellet();
	}
	splitImpactGroup.RegisterImpact(*splitShotTarget);
	splitImpactGroup.RegisterImpact(*splitShotTarget);
	splitImpactGroup.RegisterImpact(*singleShotTarget);
	for (int pelletIndex = 0; pelletIndex < 3; ++pelletIndex)
	{
		splitImpactGroup.CompletePellet();
	}
	if (!NearlyEqual(splitShotTarget->GetHealth(), 81.f) ||
		!NearlyEqual(singleShotTarget->GetHealth(), 90.f))
	{
		return Fail("Shotgun damage falloff was not calculated separately per target");
	}

	const weak_ptr<TestCombatant> minimumDamageTargetWeak = shotgunImpactWorld.SpawnActor<TestCombatant>();
	const shared_ptr<TestCombatant> minimumDamageTarget = minimumDamageTargetWeak.lock();
	if (!minimumDamageTarget)
	{
		return Fail("Shotgun minimum-damage test combatant could not be spawned");
	}
	ShotgunVolleyImpactGroup minimumDamageGroup{ *shotgunSource, {}, 10.f, 0.1f, 0.5f };
	for (int pelletIndex = 0; pelletIndex < 8; ++pelletIndex)
	{
		minimumDamageGroup.AddPellet();
		minimumDamageGroup.RegisterImpact(*minimumDamageTarget);
	}
	for (int pelletIndex = 0; pelletIndex < 8; ++pelletIndex)
	{
		minimumDamageGroup.CompletePellet();
	}
	if (!NearlyEqual(minimumDamageTarget->GetHealth(), 45.f))
	{
		return Fail("Shotgun minimum damage multiplier was not respected");
	}

	const List<const PrimaryWeaponDefinition*> configuredWeapons{
		&LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic"),
		&LoadedWeapon("Weapon.Projectile.RapidShotgun.Basic"),
		&LoadedWeapon("Weapon.Projectile.DualKineticBlaster.Basic"),
		&LoadedWeapon("Weapon.Arc.ElectricLauncher.Basic"),
		&LoadedWeapon("Weapon.Beam.ContinuousHeatLaser.Basic"),
		&LoadedWeapon("Weapon.Wave.CryoProjector.Basic")
	};
	for (const PrimaryWeaponDefinition* weapon : configuredWeapons)
	{
		if (!weapon || !PrimaryWeaponExecutionSystem::ValidateDefinition(*weapon).isValid)
		{
			return Fail("A shipped primary weapon definition failed validation");
		}
	}

	const PrimaryWeaponDefinition& basicLaser = LoadedWeapon("Weapon.Projectile.FighterRapidLaser.Basic");
	if (basicLaser.weaponId != "Weapon.Projectile.FighterRapidLaser.Basic" ||
		basicLaser.weaponType != PrimaryWeaponType::ProjectileStandard ||
		basicLaser.progressionProfile.ResolveLevelSteps().size() != 3)
	{
		return Fail("Fighter basic rapid laser has the wrong base profile");
	}

	const PrimaryWeaponDefinition& dualKineticBlaster = LoadedWeapon("Weapon.Projectile.DualKineticBlaster.Basic");
	if (dualKineticBlaster.weaponType != PrimaryWeaponType::ProjectileStandard ||
		dualKineticBlaster.muzzleDefinitions.size() != 2 ||
		sas::FindAttributeValue(dualKineticBlaster.attributes, CommonAttributeIds::Range) <= 400.f ||
		dualKineticBlaster.damageTags.size() != 1 ||
		dualKineticBlaster.damageTags.front() != DamageTypeSchema::Kinetic)
	{
		return Fail("Dual kinetic blaster has the wrong primary weapon profile");
	}
	const PrimaryWeaponDefinition& electricArcLauncher = LoadedWeapon("Weapon.Arc.ElectricLauncher.Basic");
	if (electricArcLauncher.weaponType != PrimaryWeaponType::ArcElectric ||
		sas::FindAttributeValue(
			electricArcLauncher.attributes,
			PrimaryWeaponSchema::Arc::Electric::ChainCount
		) != 3.f ||
		sas::FindAttributeValue(
			electricArcLauncher.attributes,
			PrimaryWeaponSchema::Arc::Electric::ChainRange
		) <= 0.f)
	{
		return Fail("Electric arc launcher has the wrong primary weapon profile");
	}
	const auto hasSingleDamageType = [](const PrimaryWeaponDefinition& weapon, const GameplayTag& damageType)
	{
		return weapon.damageTags.size() == 1 && weapon.damageTags.front() == damageType;
	};
	if (!hasSingleDamageType(basicLaser, DamageTypeSchema::Photonic) ||
		!hasSingleDamageType(LoadedWeapon("Weapon.Projectile.RapidShotgun.Basic"), DamageTypeSchema::Thermal) ||
		!hasSingleDamageType(LoadedWeapon("Weapon.Beam.ContinuousHeatLaser.Basic"), DamageTypeSchema::Energy) ||
		!hasSingleDamageType(dualKineticBlaster, DamageTypeSchema::Kinetic) ||
		!hasSingleDamageType(electricArcLauncher, DamageTypeSchema::Electric) ||
		!hasSingleDamageType(LoadedWeapon("Weapon.Wave.CryoProjector.Basic"), DamageTypeSchema::Cryo))
	{
		return Fail("Player primary weapon default damage types were not configured correctly");
	}
	const PrimaryWeaponDefinition& continuousHeatLaser = LoadedWeapon("Weapon.Beam.ContinuousHeatLaser.Basic");
	const auto hasAdditiveScaling = [](
		const PrimaryWeaponDefinition& weapon,
		const sas::AttributeId& target,
		const sas::AttributeId& source,
		float coefficient
	)
	{
		return std::any_of(
			weapon.scalingRules.begin(),
			weapon.scalingRules.end(),
			[&](const sas::AttributeScalingRule& rule)
			{
				return rule.targetAttributeId == target &&
					rule.sourceAttributeId == source &&
					rule.operation == sas::AttributeModifierOperation::Add &&
					NearlyEqual(rule.coefficient, coefficient);
			}
		);
	};
	if (!hasAdditiveScaling(basicLaser, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.40f) ||
		hasAdditiveScaling(basicLaser, CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 1.f) ||
		basicLaser.cadenceMode != PrimaryWeaponCadenceMode::OwnerAttackSpeedPercentage)
	{
		return Fail("Basic laser JSON scaling coefficients are invalid");
	}
	if (!hasAdditiveScaling(LoadedWeapon("Weapon.Projectile.RapidShotgun.Basic"), CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.75f) ||
		!hasAdditiveScaling(LoadedWeapon("Weapon.Projectile.RapidShotgun.Basic"), CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 0.5f))
	{
		return Fail("Rapid shotgun JSON scaling coefficients are invalid");
	}
	if (!hasAdditiveScaling(dualKineticBlaster, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.45f) ||
		!hasAdditiveScaling(dualKineticBlaster, CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 1.f))
	{
		return Fail("Dual kinetic blaster JSON scaling coefficients are invalid");
	}
	if (!hasAdditiveScaling(electricArcLauncher, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.85f) ||
		!hasAdditiveScaling(electricArcLauncher, CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 0.60f))
	{
		return Fail("Electric arc launcher JSON scaling coefficients are invalid");
	}
	if (!hasAdditiveScaling(continuousHeatLaser, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.75f) ||
		!hasAdditiveScaling(continuousHeatLaser, CommonAttributeIds::Damage, OwnerAttributeIds::EnergyPower, 0.50f))
	{
		return Fail("Continuous heat laser JSON scaling coefficients are invalid");
	}
	if (!hasAdditiveScaling(LoadedWeapon("Weapon.Wave.CryoProjector.Basic"), CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.75f) ||
		!hasAdditiveScaling(LoadedWeapon("Weapon.Wave.CryoProjector.Basic"), CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 0.5f))
	{
		return Fail("Cryo wave projector JSON scaling coefficients are invalid");
	}
	bool hasEnergyPowerBeamScaling = false;
	for (const sas::AttributeScalingRule& scalingRule : continuousHeatLaser.scalingRules)
	{
		if (scalingRule.targetAttributeId == CommonAttributeIds::Damage &&
			scalingRule.sourceAttributeId == OwnerAttributeIds::EnergyPower &&
			scalingRule.operation == sas::AttributeModifierOperation::Add &&
			NearlyEqual(scalingRule.coefficient, 0.50f))
		{
			hasEnergyPowerBeamScaling = true;
		}
		if (scalingRule.sourceAttributeId == ShipAttributeIds::MaxShield)
		{
			return Fail("Continuous heat laser must not scale from current or maximum shield");
		}
	}
	if (!hasEnergyPowerBeamScaling)
	{
		return Fail("Continuous heat laser is missing its EnergyPower damage scaling");
	}

	const List<int> scalingTestLevels{ 1, 10, 25, 50 };
	float previousCryoSlowTempo = std::numeric_limits<float>::max();
	for (const int level : scalingTestLevels)
	{
		PrimaryWeaponScalingSample rapid;
		PrimaryWeaponScalingSample shotgun;
		PrimaryWeaponScalingSample dual;
		PrimaryWeaponScalingSample electric;
		PrimaryWeaponScalingSample beam;
		PrimaryWeaponScalingSample cryo;
		if (!ResolveFighterPrimaryWeaponScaling(basicLaser, level, rapid) ||
			!ResolveFighterPrimaryWeaponScaling(LoadedWeapon("Weapon.Projectile.RapidShotgun.Basic"), level, shotgun) ||
			!ResolveFighterPrimaryWeaponScaling(dualKineticBlaster, level, dual) ||
			!ResolveFighterPrimaryWeaponScaling(electricArcLauncher, level, electric) ||
			!ResolveFighterPrimaryWeaponScaling(continuousHeatLaser, level, beam) ||
			!ResolveFighterPrimaryWeaponScaling(LoadedWeapon("Weapon.Wave.CryoProjector.Basic"), level, cryo))
		{
			return Fail("A player primary weapon did not resolve from the fighter progression runtime");
		}

		const float rapidDps = rapid.damage * rapid.fireRate;
		const float shotgunPerPelletDamage = shotgun.damage * 0.8f;
		const float shotgunAggregateDps = shotgunPerPelletDamage * 3.f * shotgun.fireRate;
		const float dualDps = dual.damage * dual.fireRate * 2.f;
		const float electricSingleTargetDps = electric.damage * electric.fireRate;
		const float beamSingleTargetDps = beam.damage;
		const float cryoSlowTempo = 3.f / cryo.fireRate;
		if (rapidDps <= 0.f || shotgunAggregateDps <= 0.f || dualDps <= 0.f ||
			electricSingleTargetDps <= 0.f || beamSingleTargetDps <= 0.f ||
			cryoSlowTempo > previousCryoSlowTempo + 0.0001f)
		{
			return Fail("Player primary weapon theory DPS or Cryo slow tempo regressed across fighter levels");
		}
		previousCryoSlowTempo = cryoSlowTempo;
		const auto verifiesAttackPower = [&](const PrimaryWeaponDefinition& weapon, float coefficient)
		{
			PrimaryWeaponScalingSample base;
			PrimaryWeaponScalingSample withAttackPower;
			return ResolveFighterPrimaryWeaponScaling(weapon, level, base) &&
				ResolveFighterPrimaryWeaponScaling(weapon, level, withAttackPower, 1.f) &&
				NearlyEqual(withAttackPower.damage - base.damage, coefficient);
		};
		const auto verifiesAttackSpeed = [&](const PrimaryWeaponDefinition& weapon, float coefficient)
		{
			PrimaryWeaponScalingSample base;
			PrimaryWeaponScalingSample withAttackSpeed;
			return ResolveFighterPrimaryWeaponScaling(weapon, level, base) &&
				ResolveFighterPrimaryWeaponScaling(weapon, level, withAttackSpeed, 0.f, 1.f) &&
				NearlyEqual(withAttackSpeed.fireRate - base.fireRate, coefficient);
		};
		if (!verifiesAttackPower(basicLaser, 0.40f))
		{
			return Fail("Basic laser runtime scaling did not match JSON");
		}
		if (!verifiesAttackPower(LoadedWeapon("Weapon.Projectile.RapidShotgun.Basic"), 0.75f) ||
			!verifiesAttackSpeed(LoadedWeapon("Weapon.Projectile.RapidShotgun.Basic"), 0.5f))
		{
			return Fail("Rapid shotgun runtime scaling did not match JSON");
		}
		if (!verifiesAttackPower(dualKineticBlaster, 0.45f) ||
			!verifiesAttackSpeed(dualKineticBlaster, 1.f))
		{
			return Fail("Dual kinetic blaster runtime scaling did not match JSON");
		}
		if (!verifiesAttackPower(electricArcLauncher, 0.85f) ||
			!verifiesAttackSpeed(electricArcLauncher, 0.60f))
		{
			return Fail("Electric arc launcher runtime scaling did not match JSON");
		}
		if (!verifiesAttackPower(continuousHeatLaser, 0.75f))
		{
			return Fail("Continuous heat laser AttackPower scaling did not match JSON");
		}
		if (!verifiesAttackPower(LoadedWeapon("Weapon.Wave.CryoProjector.Basic"), 0.75f) ||
			!verifiesAttackSpeed(LoadedWeapon("Weapon.Wave.CryoProjector.Basic"), 0.5f))
		{
			return Fail("Cryo wave projector runtime scaling did not match JSON");
		}

		PrimaryWeaponScalingSample beamWithAttackSpeed;
		PrimaryWeaponScalingSample beamWithEnergy;
		if (!ResolveFighterPrimaryWeaponScaling(continuousHeatLaser, level, beamWithAttackSpeed, 0.f, 100.f) ||
			!ResolveFighterPrimaryWeaponScaling(continuousHeatLaser, level, beamWithEnergy, 0.f, 0.f, 1.f) ||
			!NearlyEqual(beamWithAttackSpeed.damage, beam.damage) ||
			!NearlyEqual(beamWithAttackSpeed.fireRate, 0.f) ||
			!NearlyEqual(beamWithEnergy.damage - beam.damage, 0.5f))
		{
		return Fail("Continuous beam AttackSpeed or EnergyPower runtime scaling did not match its balance contract");
		}
	}

	const float baselineBeamHeatAt50 = MeasureBeamHeatAfterFiring(50.f, 0.f);
	const float highAttackSpeedBeamHeatAt50 = MeasureBeamHeatAfterFiring(50.f, 100000.f);
	const float baselineBeamHeatAt75 = MeasureBeamHeatAfterFiring(75.f, 0.f);
	const float highAttackSpeedBeamHeatAt75 = MeasureBeamHeatAfterFiring(75.f, 100000.f);
	const float baselineBeamHeatAt90 = MeasureBeamHeatAfterFiring(90.f, 0.f);
	const float highAttackSpeedBeamHeatAt90 = MeasureBeamHeatAfterFiring(90.f, 100000.f);
	const float baselineBeamOverheatTime = MeasureBeamTimeToOverheat(0.f);
	const float highAttackSpeedBeamOverheatTime = MeasureBeamTimeToOverheat(100000.f);
	if (!NearlyEqual(baselineBeamHeatAt50, highAttackSpeedBeamHeatAt50) ||
		!(highAttackSpeedBeamHeatAt75 < baselineBeamHeatAt75) ||
		!(highAttackSpeedBeamHeatAt90 < baselineBeamHeatAt90) ||
		!NearlyEqual(
			(highAttackSpeedBeamHeatAt75 - 75.f) / (baselineBeamHeatAt75 - 75.f),
			0.65f
		) ||
		!NearlyEqual(
			(highAttackSpeedBeamHeatAt90 - 90.f) / (baselineBeamHeatAt90 - 90.f),
			0.65f
		) ||
		baselineBeamOverheatTime <= 0.f ||
		highAttackSpeedBeamOverheatTime <= baselineBeamOverheatTime ||
		highAttackSpeedBeamOverheatTime > baselineBeamOverheatTime * 1.4f)
	{
		return Fail("Continuous beam AttackSpeed heat reduction did not respect its low-heat, high-heat, or overheat limits");
	}
	const float baselineBeamDps10 = MeasureBeamSustainedDps(0.f, 10.f);
	const float baselineBeamDps30 = MeasureBeamSustainedDps(0.f, 30.f);
	const float highAttackSpeedBeamDps10 = MeasureBeamSustainedDps(100000.f, 10.f);
	const float highAttackSpeedBeamDps30 = MeasureBeamSustainedDps(100000.f, 30.f);
	if (baselineBeamDps10 <= 0.f || baselineBeamDps30 <= 0.f ||
		highAttackSpeedBeamDps10 < baselineBeamDps10 ||
		highAttackSpeedBeamDps30 < baselineBeamDps30)
	{
		return Fail("Continuous beam sustained 10/30 second runtime DPS did not benefit from high-heat AttackSpeed cooling");
	}

	const GameAbilityDefinition basicLaserAbility = AbilityData::MakePrimaryFireAbilityDefinition(basicLaser);
	if (basicLaser.progressionProfile.GetScrapCostToReachLevel(2) != 40u ||
		basicLaser.progressionProfile.GetScrapCostToReachLevel(3) != 45u ||
		basicLaser.progressionProfile.GetScrapCostToReachLevel(4) != 50u ||
		basicLaser.progressionProfile.GetScrapCostToReachLevel(15) != 105u ||
		!basicLaserAbility.HasScrapCostToReachLevel(2) ||
		basicLaserAbility.GetScrapCostToReachLevel(2) != 40u ||
		basicLaserAbility.GetScrapCostToReachLevel(4) != 50u ||
		basicLaserAbility.GetScrapCostToReachLevel(15) != 105u)
	{
		return Fail("Primary weapon scrap costs were not preserved during ability conversion");
	}
	const sas::AbilityHandle basicLaserHandle = abilitySystem.GrantAbility(basicLaserAbility);
	if (!basicLaserHandle.IsValid() ||
		!abilitySystem.SetAbilityLevel(sas::AbilitySlot::PrimaryFire, basicLaserAbility.GetMaxLevel()))
	{
		return Fail("Fighter basic rapid laser could not reach its maximum level");
	}
	const GameAbility* basicLaserInstance = abilitySystem.GetAbility(sas::AbilitySlot::PrimaryFire);
	const sas::GameplayAttribute* basicDamage = sas::FindAttribute(basicLaser.attributes, CommonAttributeIds::Damage);
	const sas::GameplayAttribute* basicEmpoweredDamage = sas::FindAttribute(
		basicLaser.attributes,
		PrimaryWeaponSchema::Empowered::BonusDamage
	);
	const sas::GameplayAttribute* basicFireRate = sas::FindAttribute(basicLaser.attributes, CommonAttributeIds::FireRate);
	const sas::GameplayAttribute* basicSpeed = sas::FindAttribute(
		basicLaser.attributes,
		PrimaryWeaponSchema::Projectile::Delivery::Speed
	);
	const sas::GameplayAttribute* basicRange = sas::FindAttribute(basicLaser.attributes, CommonAttributeIds::Range);
	if (!basicLaserInstance || basicLaserInstance->GetLevel() != basicLaserAbility.GetMaxLevel() ||
		!basicDamage || !basicEmpoweredDamage || !basicFireRate || !basicSpeed || !basicRange ||
		sas::CalculateModifiedAttributeValue(
			*basicDamage,
			basicLaserInstance->GetDefinition().attributeModifiers
		) <= basicDamage->currentValue ||
		sas::CalculateModifiedAttributeValue(
			*basicEmpoweredDamage,
			basicLaserInstance->GetDefinition().attributeModifiers
		) <= basicEmpoweredDamage->currentValue ||
		!NearlyEqual(sas::CalculateModifiedAttributeValue(
			*basicFireRate,
			basicLaserInstance->GetDefinition().attributeModifiers
		), basicFireRate->currentValue) ||
		!NearlyEqual(sas::CalculateModifiedAttributeValue(
			*basicSpeed,
			basicLaserInstance->GetDefinition().attributeModifiers
		), basicSpeed->currentValue) ||
		!NearlyEqual(sas::CalculateModifiedAttributeValue(
			*basicRange,
			basicLaserInstance->GetDefinition().attributeModifiers
		), basicRange->currentValue))
	{
		return Fail("Fighter basic rapid laser progression produced the wrong final profile");
	}

	AttachmentLoadout thermalLoadout;
	const List<GameplayTag> damageCapabilities{ AttachmentSchema::Capability::Damage };
	if (!thermalLoadout.TryEquip(
		AttachmentData::Definitions::ThermalConverter,
		AttachmentHostKind::PrimaryWeapon,
		damageCapabilities,
		2
	))
	{
		return Fail("Thermal attachment could not be equipped on a damage host");
	}
	const List<GameplayTag> convertedDamageTags = thermalLoadout.ResolveDamageTags(
		AttachmentHostKind::PrimaryWeapon,
		{ DamageTypeSchema::Photonic }
	);
	if (convertedDamageTags.size() != 1 || convertedDamageTags.front() != DamageTypeSchema::Thermal)
	{
		return Fail("Thermal attachment did not replace the default damage type");
	}
	const sas::GameplayAttributeList thermalDamageAttributes = thermalLoadout.ApplyConditionalModifiers(
		AttachmentHostKind::PrimaryWeapon,
		{ sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f } },
		{ DamageTypeSchema::Thermal }
	);
	if (!NearlyEqual(sas::FindAttributeValue(thermalDamageAttributes, CommonAttributeIds::Damage), 12.f))
	{
		return Fail("Thermal attachment did not grant its already-thermal damage bonus");
	}

	GameAbilityDefinition attachmentAbility;
	attachmentAbility.abilityId = "Ability.Offense.Test.AttachmentHost";
	attachmentAbility.slot = sas::AbilitySlot::Ability4;
	attachmentAbility.cooldown = 10.f;
	attachmentAbility.abilityTags = {
		GameplayTagSchema::AbilityOffense,
		GameplayTag{ "Ability.Offense.Test" }
	};
	const sas::AbilityHandle attachmentAbilityHandle = abilitySystem.GrantAbility(attachmentAbility);
	if (!attachmentAbilityHandle.IsValid() || !abilitySystem.TryEquipAttachment(
		attachmentAbilityHandle,
		AttachmentData::Definitions::HeavyCapacitor,
		AttachmentHostKind::Ability
	))
	{
		return Fail("Ability-only cooldown attachment could not be equipped on an offensive ability");
	}
	const GameAbility* attachmentAbilityInstance = abilitySystem.GetAbility(attachmentAbilityHandle);
	if (!attachmentAbilityInstance || !NearlyEqual(attachmentAbilityInstance->GetCooldownDuration(), 12.5f))
	{
		return Fail("Ability attachment modifiers did not use the shared attribute math");
	}
	if (abilitySystem.TryEquipAttachment(
		attachmentAbilityHandle,
		AttachmentData::Definitions::EmergencySalvo,
		AttachmentHostKind::Ability
	))
	{
		return Fail("Primary-weapon-only attachment was accepted by an ability host");
	}

	World attachmentWeaponWorld{ nullptr };
	Actor attachmentWeaponOwner{ &attachmentWeaponWorld };
	LightYearsAbilitySystemComponent attachmentWeaponAbilities{
		attachmentWeaponOwner
	};
	PrimaryWeaponDefinition attachmentWeaponDefinition{
		"Weapon.Projectile.TestAttachmentSalvo.Basic",
		PrimaryWeaponType::ProjectileStandard,
		WeaponPresentationDefinition{},
		{
			sas::GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
			sas::GameplayAttribute{ CommonAttributeIds::FireRate, 3.f, 0.01f },
			sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 100.f, 0.f },
			sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 10.f, 0.f },
			sas::GameplayAttribute{ CommonAttributeIds::Range, 10000.f, 0.f },
			sas::GameplayAttribute{ CollisionAttributeIds::Radius, 7.f, 0.1f }
		},
		{},
		true
	};
	if (!attachmentWeaponAbilities.GrantAbility(
		AbilityData::MakePrimaryFireAbilityDefinition(attachmentWeaponDefinition)
	).IsValid() || !attachmentWeaponAbilities.TryEquipAttachment(
		sas::AbilitySlot::PrimaryFire,
		AttachmentData::Definitions::EmergencySalvo,
		AttachmentHostKind::PrimaryWeapon
	) || !attachmentWeaponAbilities.TryEquipAttachment(
		sas::AbilitySlot::PrimaryFire,
		AttachmentData::Definitions::ThermalConverter,
		AttachmentHostKind::PrimaryWeapon
	))
	{
		return Fail("Compatible primary weapon attachments could not be equipped");
	}
	const GameAbility* attachmentPrimaryInstance = attachmentWeaponAbilities.GetAbility(sas::AbilitySlot::PrimaryFire);
	const List<GameplayTag> attachmentPrimaryDamageTags = attachmentPrimaryInstance
		? attachmentPrimaryInstance->GetResolvedDamageTags(AttachmentHostKind::PrimaryWeapon)
		: List<GameplayTag>{};
	if (attachmentPrimaryDamageTags.size() != 1 || attachmentPrimaryDamageTags.front() != DamageTypeSchema::Thermal)
	{
		return Fail("Primary weapon attachment damage type was not resolved");
	}
	attachmentWeaponAbilities.SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	attachmentWeaponAbilities.Tick(0.f);
	attachmentWeaponWorld.TickInternal(0.f);
	if (attachmentWeaponWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 2)
	{
		return Fail("Low fire-rate conditional attachment did not add a projectile");
	}

	std::string rocketValidationFailure;
	const GameAbilityDefinition* rocketDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::Rocket::AbilityId::Basic);
	if (!ValidateAbilityCatalog(AbilityData::GetShippedAbilityDefinitions(), &rocketValidationFailure) ||
		!rocketDefinition || rocketDefinition->slot != sas::AbilitySlot::Ability3 ||
		rocketDefinition->activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
		rocketDefinition->lifetimePolicy != sas::AbilityLifetimePolicy::Instant ||
	rocketDefinition->maxCharges != 1 || rocketDefinition->behaviorType != AbilityBehaviorType::Rocket ||
		rocketDefinition->damageTags.size() != 1 || rocketDefinition->damageTags.front() != DamageTypeSchema::Kinetic ||
		!AbilityActorRegistry::ValidateDefinition(
			LoadedAbilityActor("Actor.Ability.Rocket.Projectile.Basic")
		).isValid)
	{
		return Fail("Basic Rocket configuration or shipped catalog validation failed");
	}
	AbilityActorDefinition rocketWithoutPresentation =
		LoadedAbilityActor("Actor.Ability.Rocket.Projectile.Basic");
	rocketWithoutPresentation.presentationProfileId = sas::ContentId{};
	if (AbilityActorRegistry::ValidateDefinition(rocketWithoutPresentation).isValid)
	{
		return Fail("Basic Rocket accepted a missing presentation profile");
	}
	AbilityActorDefinition rocketWithLegacyAttributePath =
		LoadedAbilityActor("Actor.Ability.Rocket.Projectile.Basic");
	for (sas::GameplayAttribute& attribute : rocketWithLegacyAttributePath.attributes)
	{
		if (attribute.id == AbilityData::Rocket::Actor::Projectile::ProjectileSpeed)
		{
			attribute.id = sas::AttributeId{ "Attribute.AbilityActor.Rocket.ProjectileSpeed" };
		}
	}
	if (AbilityActorRegistry::ValidateDefinition(rocketWithLegacyAttributePath).isValid)
	{
		return Fail("Ability actor accepted a custom attribute without its role segment");
	}

	const AbilityActorDefinition& rocketActor =
		LoadedAbilityActor("Actor.Ability.Rocket.Projectile.Basic");
	const auto rocketAttribute = [&](const sas::AttributeId& attributeId)
	{
		return sas::FindAttributeValue(rocketActor.attributes, attributeId, 0.f);
	};
	const auto rocketLevelModifier = [&](const sas::AttributeId& attributeId)
	{
		for (const sas::AttributeModifier& modifier :
			rocketDefinition->levelProgression.front().attributeModifiers)
		{
			if (modifier.attributeId == attributeId)
			{
				return modifier.magnitude;
			}
		}
		return 0.f;
	};
	struct RocketTestSettings
	{
		float baseDamage;
		float cooldown;
		float projectileSpeed;
		float range;
		float explosionRadius;
		int projectileCount;
		float spawnDistance;
		float damagePerLevel;
		float cooldownReductionPerLevel;
		float explosionRadiusPerLevel;
	};
	const RocketTestSettings rocketSettingsValue{
		rocketAttribute(CommonAttributeIds::Damage),
		rocketDefinition->cooldown,
		rocketAttribute(AbilityData::Rocket::Actor::Projectile::ProjectileSpeed),
		rocketAttribute(CommonAttributeIds::Range),
		rocketAttribute(CommonAttributeIds::Radius),
		1,
		rocketActor.spawnDistance,
		rocketLevelModifier(CommonAttributeIds::Damage),
		-rocketLevelModifier(CommonAttributeIds::Cooldown),
		rocketLevelModifier(CommonAttributeIds::Radius)
	};
	const RocketTestSettings* rocketSettings = &rocketSettingsValue;
	if (rocketSettings->baseDamage <= 0.f || rocketSettings->cooldown <= 0.f ||
		rocketSettings->projectileSpeed <= 0.f || rocketSettings->range <= 0.f ||
		rocketSettings->explosionRadius <= 0.f || rocketSettings->projectileCount != 1 ||
		rocketSettings->damagePerLevel <= 0.f || rocketSettings->cooldownReductionPerLevel <= 0.f ||
		rocketSettings->explosionRadiusPerLevel <= 0.f)
	{
		return Fail("Basic Rocket settings are invalid");
	}

	auto SpawnRocket = [&](
		World& world,
		TestCombatant& owner,
		int level = 1,
		float attackPower = 0.f,
		float attackSpeed = 0.f,
		float luck = 0.f,
		float energyPower = 0.f
	)
	{
		owner.GetCombatRuntime().InitializeOwnerAttributes(1000.f);
		owner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ OwnerAttributeIds::AttackPower, attackPower }
		);
		owner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ OwnerAttributeIds::AttackSpeed, attackSpeed }
		);
		owner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ OwnerAttributeIds::Luck, luck }
		);
		owner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ OwnerAttributeIds::EnergyPower, energyPower }
		);
		owner.SetCollisionLayer(CollisionLayer::Player);
		owner.SetActorRotation(90.f);

		LightYearsAbilitySystemComponent& abilities = owner.GetCombatRuntime().GetAbilitySystemComponent();
		const sas::AbilityHandle handle = abilities.GrantAbility(*rocketDefinition);
		if (!handle.IsValid() || (level > 1 && !abilities.SetAbilityLevel(handle, level)))
		{
			return shared_ptr<RocketProjectileActor>{};
		}
		abilities.SetAbilitySlotInput(sas::AbilitySlot::Ability3, true);
		abilities.Tick(0.f);
		world.TickInternal(0.f);

		const List<weak_ptr<RocketProjectileActor>> rockets =
			world.GetActorsByType<RocketProjectileActor>();
		return rockets.size() == 1 ? rockets.front().lock() : shared_ptr<RocketProjectileActor>{};
	};

	World baselineRocketWorld{ nullptr };
	TestCombatant baselineRocketOwner{ &baselineRocketWorld, 1000.f };
	const shared_ptr<RocketProjectileActor> baselineRocket = SpawnRocket(
		baselineRocketWorld,
		baselineRocketOwner
	);
	baselineRocketWorld.TickInternal(0.f);
	if (!baselineRocket || !NearlyEqual(baselineRocket->GetDamage(), rocketSettings->baseDamage) ||
		!NearlyEqual(baselineRocket->GetProjectileSpeed(), rocketSettings->projectileSpeed) ||
		!NearlyEqual(baselineRocket->GetMaximumRange(), rocketSettings->range) ||
		!NearlyEqual(baselineRocket->GetExplosionRadius(), rocketSettings->explosionRadius) ||
		baselineRocket->GetDamageTags().size() != 1 ||
		baselineRocket->GetDamageTags().front() != DamageTypeSchema::Kinetic)
	{
		return Fail("Basic Rocket did not spawn with its base Kinetic configuration");
	}
	if (!baselineRocket->HasVisualActor() || !baselineRocket->HasTelegraph() ||
		baselineRocketWorld.GetActorsByType<RocketVisualActor>().size() != 1 ||
		baselineRocketWorld.GetActorsByType<AreaTelegraphActor>().size() != 1 ||
		!NearlyEqual(
			baselineRocket->GetPredictedImpactLocation().x,
			baselineRocket->GetActorLocation().x + rocketSettings->range
		) ||
		!NearlyEqual(
			baselineRocket->GetPredictedImpactLocation().y,
			baselineRocket->GetActorLocation().y
		))
	{
		return Fail("Basic Rocket did not create its flight visual and predicted impact telegraph");
	}
	const float baselineRocketX = baselineRocket->GetActorLocation().x;
	baselineRocketWorld.TickInternal(0.1f);
	if (!NearlyEqual(baselineRocket->GetActorLocation().x, baselineRocketX + rocketSettings->projectileSpeed * 0.1f) ||
		!NearlyEqual(baselineRocket->GetVelocity().x, rocketSettings->projectileSpeed) ||
		!NearlyEqual(baselineRocket->GetVelocity().y, 0.f))
	{
		return Fail("Basic Rocket did not travel along its resolved aim direction");
	}

	World cursorTargetRocketWorld{ nullptr };
	TestCombatant cursorTargetRocketOwner{ &cursorTargetRocketWorld, 1000.f };
	cursorTargetRocketOwner.SetCollisionLayer(CollisionLayer::Player);
	cursorTargetRocketOwner.SetActorLocation({ 0.f, 0.f });
	cursorTargetRocketOwner.SetActorRotation(90.f);
	const sf::Vector2f cursorTargetLocation{ 342.f, 0.f };
	const shared_ptr<RocketProjectileActor> cursorTargetRocket =
		std::dynamic_pointer_cast<RocketProjectileActor>(
			AbilityActorRegistry::Spawn(
				AbilityActorSpawnContext{
					cursorTargetRocketOwner,
					LoadedAbilityActor("Actor.Ability.Rocket.Projectile.Basic"),
					LoadedAbilityActor("Actor.Ability.Rocket.Projectile.Basic").attributes,
					std::optional<sf::Vector2f>{ cursorTargetLocation }
				}
			).lock()
		);
	if (!cursorTargetRocket)
	{
		return Fail("Basic Rocket could not spawn with an explicit cursor target");
	}
	cursorTargetRocket->SetActorLocation({ rocketSettings->spawnDistance, 0.f });
	cursorTargetRocket->SetActorRotation(90.f);
	cursorTargetRocket->ConfigureFromAttributes(
		LoadedAbilityActor("Actor.Ability.Rocket.Projectile.Basic").attributes
	);
	cursorTargetRocketWorld.TickInternal(0.f);
	const float cursorTargetTravelDistance =
		cursorTargetLocation.x - rocketSettings->spawnDistance;
	if (!NearlyEqual(
			cursorTargetRocket->GetTargetTravelDistance(),
			cursorTargetTravelDistance
		) ||
		!NearlyEqual(
			cursorTargetRocket->GetPredictedImpactLocation().x,
			cursorTargetLocation.x
		) ||
		!NearlyEqual(
			cursorTargetRocket->GetPredictedImpactLocation().y,
			cursorTargetLocation.y
		) ||
		!cursorTargetRocket->HasTelegraph())
	{
		return Fail("Basic Rocket did not target the cursor location inside maximum range");
	}
	cursorTargetRocketWorld.TickInternal(
		cursorTargetTravelDistance / rocketSettings->projectileSpeed
	);
	if (!NearlyEqual(
			cursorTargetRocket->GetTravelDistance(),
			cursorTargetTravelDistance
		) ||
		!cursorTargetRocket->GetIsPendingDestroy())
	{
		return Fail("Basic Rocket did not explode when it reached the cursor location");
	}
	cursorTargetRocketWorld.TickInternal(0.f);
	if (!cursorTargetRocketWorld.GetActorsByType<RocketProjectileActor>().empty()
		|| !cursorTargetRocketWorld.GetActorsByType<AreaTelegraphActor>().empty())
	{
		return Fail("Cursor-targeted Rocket did not clean up its projectile and telegraph");
	}

	World attackPowerRocketWorld{ nullptr };
	TestCombatant attackPowerRocketOwner{ &attackPowerRocketWorld, 1000.f };
	const shared_ptr<RocketProjectileActor> attackPowerRocket = SpawnRocket(
		attackPowerRocketWorld,
		attackPowerRocketOwner,
		1,
		20.f
	);
	if (!attackPowerRocket ||
		!NearlyEqual(attackPowerRocket->GetDamage(), rocketSettings->baseDamage + 20.f * 1.25f))
	{
		return Fail("Basic Rocket did not apply AttackPower x1.25 scaling");
	}

	World irrelevantRocketWorld{ nullptr };
	TestCombatant irrelevantRocketOwner{ &irrelevantRocketWorld, 1000.f };
	const shared_ptr<RocketProjectileActor> irrelevantRocket = SpawnRocket(
		irrelevantRocketWorld,
		irrelevantRocketOwner,
		1,
		0.f,
		250.f,
		250.f,
		250.f
	);
	if (!irrelevantRocket || !NearlyEqual(irrelevantRocket->GetDamage(), rocketSettings->baseDamage))
	{
		return Fail("AttackSpeed, Luck, or EnergyPower changed Basic Rocket damage");
	}

	for (int level = 1; level <= 15; ++level)
	{
		World levelRocketWorld{ nullptr };
		TestCombatant levelRocketOwner{ &levelRocketWorld, 1000.f };
		const shared_ptr<RocketProjectileActor> levelRocket = SpawnRocket(
			levelRocketWorld,
			levelRocketOwner,
			level
		);
		const float levelOffset = static_cast<float>(level - 1);
		const GameAbility* levelInstance =
			levelRocketOwner.GetCombatRuntime().GetAbilitySystemComponent().GetAbility(sas::AbilitySlot::Ability3);
		if (!levelRocket || !levelInstance ||
			!NearlyEqual(levelRocket->GetDamage(), rocketSettings->baseDamage + levelOffset * rocketSettings->damagePerLevel) ||
			!NearlyEqual(levelRocket->GetExplosionRadius(), rocketSettings->explosionRadius + levelOffset * rocketSettings->explosionRadiusPerLevel) ||
			!NearlyEqual(levelRocket->GetProjectileSpeed(), rocketSettings->projectileSpeed) ||
			!NearlyEqual(levelRocket->GetMaximumRange(), rocketSettings->range) ||
			!NearlyEqual(
				levelInstance->GetCooldownDuration(),
				rocketSettings->cooldown - levelOffset * rocketSettings->cooldownReductionPerLevel
			))
		{
			return Fail("Basic Rocket level progression did not preserve fixed speed/range or cumulative values");
		}
	}

	TestCombatant hasteRocketOwner;
	hasteRocketOwner.GetCombatRuntime().InitializeOwnerAttributes(1000.f);
	const sas::AbilityHandle hasteRocketHandle = hasteRocketOwner.GetAbilitySystemComponent().GrantAbility(*rocketDefinition);
	const float baseRocketCooldown = hasteRocketOwner.GetCombatRuntime().GetAbilitySystemComponent()
		.GetAbility(hasteRocketHandle)->GetCooldownDuration();
	hasteRocketOwner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::AbilityHaste, 100.f }
	);
	const GameAbility* hasteRocket = hasteRocketOwner.GetCombatRuntime().GetAbilitySystemComponent().GetAbility(hasteRocketHandle);
	if (!hasteRocket || !(hasteRocket->GetCooldownDuration() < baseRocketCooldown))
	{
		return Fail("AbilityHaste did not reduce Basic Rocket final cooldown");
	}

	World explosionRocketWorld{ nullptr };
	TestCombatant explosionRocketOwner{ &explosionRocketWorld, 1000.f };
	const shared_ptr<TestCombatant> directTarget =
		explosionRocketWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestCombatant> areaTarget =
		explosionRocketWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestCombatant> outsideTarget =
		explosionRocketWorld.SpawnActor<TestCombatant>(1000.f).lock();
	if (!directTarget || !areaTarget || !outsideTarget)
	{
		return Fail("Basic Rocket test targets could not spawn");
	}
	for (const shared_ptr<TestCombatant>& target : { directTarget, areaTarget, outsideTarget })
	{
		target->SetCollisionLayer(CollisionLayer::Enemy);
		target->SetCollisionMask(CollisionLayer::PlayerBullet);
	}
	directTarget->SetActorLocation({ 42.f, 0.f });
	areaTarget->SetActorLocation({ 92.f, 0.f });
	outsideTarget->SetActorLocation({ 120.f, 0.f });
	explosionRocketWorld.TickInternal(0.f);
	const shared_ptr<RocketProjectileActor> explosionRocket = SpawnRocket(
		explosionRocketWorld,
		explosionRocketOwner
	);
	if (!explosionRocket)
	{
		return Fail("Basic Rocket could not spawn for area-damage test");
	}
	explosionRocket->OnActorBeginOverlap(directTarget.get());
	explosionRocket->OnActorBeginOverlap(directTarget.get());
	if (!NearlyEqual(directTarget->GetHealth(), 1000.f - rocketSettings->baseDamage) ||
		!NearlyEqual(areaTarget->GetHealth(), 1000.f - rocketSettings->baseDamage) ||
		!NearlyEqual(outsideTarget->GetHealth(), 1000.f))
	{
		return Fail("Basic Rocket explosion did not damage each eligible target exactly once");
	}
	explosionRocketWorld.TickInternal(0.f);
	if (!explosionRocketWorld.GetActorsByType<RocketProjectileActor>().empty())
	{
		return Fail("Basic Rocket explosion actor was not cleaned up");
	}
	const List<weak_ptr<RocketVisualActor>> impactVisuals =
		explosionRocketWorld.GetActorsByType<RocketVisualActor>();
	if (explosionRocket->HasTelegraph() ||
		!explosionRocketWorld.GetActorsByType<AreaTelegraphActor>().empty() ||
		impactVisuals.size() != 1 || !impactVisuals.front().lock() ||
		!impactVisuals.front().lock()->IsImpacting())
	{
		return Fail("Basic Rocket did not transition its flight visual into the impact effect");
	}
	const float rocketImpactVisualDuration = impactVisuals.front().lock()->GetImpactVisualDuration();
	explosionRocketWorld.TickInternal(rocketImpactVisualDuration + 0.01f);
	if (!explosionRocketWorld.GetActorsByType<RocketVisualActor>().empty())
	{
		return Fail("Basic Rocket impact visual was not cleaned up after its configured duration");
	}

	World rangeRocketWorld{ nullptr };
	TestCombatant rangeRocketOwner{ &rangeRocketWorld, 1000.f };
	const shared_ptr<TestCombatant> rangeTarget =
		rangeRocketWorld.SpawnActor<TestCombatant>(1000.f).lock();
	if (!rangeTarget)
	{
		return Fail("Basic Rocket range-impact target could not spawn");
	}
	rangeTarget->SetCollisionLayer(CollisionLayer::Enemy);
	rangeTarget->SetCollisionMask(CollisionLayer::PlayerBullet);
	rangeTarget->SetActorLocation({
		rocketSettings->spawnDistance + rocketSettings->range,
		0.f
	});
	rangeRocketWorld.TickInternal(0.f);
	const shared_ptr<RocketProjectileActor> rangeRocket = SpawnRocket(rangeRocketWorld, rangeRocketOwner);
	if (!rangeRocket)
	{
		return Fail("Basic Rocket could not spawn for range cleanup test");
	}
	rangeRocketWorld.TickInternal(10.f);
	if (!NearlyEqual(rangeRocket->GetTravelDistance(), rocketSettings->range) ||
		!rangeRocketWorld.GetActorsByType<RocketProjectileActor>().empty() ||
		!NearlyEqual(rangeTarget->GetHealth(), 1000.f - rocketSettings->baseDamage))
	{
		return Fail("Basic Rocket did not explode and clean up at its fixed maximum range");
	}

	std::string railBurstValidationFailure;
	const GameAbilityDefinition* railBurstDefinition =
		AbilityData::FindShippedAbilityDefinition(
			AbilityData::RailBurst::AbilityId::Basic
		);
	const AbilityActorDefinition& railBurstActor = LoadedAbilityActor(
		"Actor.Ability.RailBurst.Projectile.Basic"
	);
	const auto railBurstAttribute = [&](const sas::AttributeId& attributeId)
	{
		return sas::FindAttributeValue(railBurstActor.attributes, attributeId, 0.f);
	};
	if (!railBurstDefinition ||
		!ValidateAbilityDefinition(*railBurstDefinition, &railBurstValidationFailure) ||
		railBurstDefinition->behaviorType != AbilityBehaviorType::RailBurst ||
		railBurstActor.lifeTime <=
			railBurstAttribute(CommonAttributeIds::Range) /
				railBurstAttribute(AbilityData::RailBurst::Actor::Projectile::ProjectileSpeed))
	{
		return Fail("Rail Burst shipped definition or shared pierce contract is invalid");
	}

	struct RailBurstTestSettings
	{
		float baseDamage;
		float projectileSpeed;
		float range;
		float spawnDistance;
	};
	const RailBurstTestSettings railBurstSettings{
		railBurstAttribute(CommonAttributeIds::Damage),
		railBurstAttribute(AbilityData::RailBurst::Actor::Projectile::ProjectileSpeed),
		railBurstAttribute(CommonAttributeIds::Range),
		railBurstActor.spawnDistance
	};
	auto SpawnRailBurst = [&](World& world, TestCombatant& owner)
	{
		owner.GetCombatRuntime().InitializeOwnerAttributes(1000.f);
		owner.SetCollisionLayer(CollisionLayer::Player);
		owner.SetActorRotation(90.f);
		LightYearsAbilitySystemComponent& abilities =
			owner.GetCombatRuntime().GetAbilitySystemComponent();
		const sas::AbilityHandle handle = abilities.GrantAbility(*railBurstDefinition);
		if (!handle.IsValid())
		{
			return shared_ptr<RailBurstProjectileActor>{};
		}
		abilities.SetAbilitySlotInput(sas::AbilitySlot::Ability4, true);
		abilities.Tick(0.f);
		world.TickInternal(0.f);
		const List<weak_ptr<RailBurstProjectileActor>> projectiles =
			world.GetActorsByType<RailBurstProjectileActor>();
		return projectiles.size() == 1
			? projectiles.front().lock()
			: shared_ptr<RailBurstProjectileActor>{};
	};

	World railBurstWorld{ nullptr };
	TestCombatant railBurstOwner{ &railBurstWorld, 1000.f };
	const shared_ptr<TestCombatant> firstRailTarget =
		railBurstWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestCombatant> secondRailTarget =
		railBurstWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestCombatant> thirdRailTarget =
		railBurstWorld.SpawnActor<TestCombatant>(1000.f).lock();
	if (!firstRailTarget || !secondRailTarget || !thirdRailTarget)
	{
		return Fail("Rail Burst test targets could not spawn");
	}
	for (const shared_ptr<TestCombatant>& target : {
		firstRailTarget,
		secondRailTarget,
		thirdRailTarget
	})
	{
		target->SetCollisionLayer(CollisionLayer::Enemy);
		target->SetCollisionMask(CollisionLayer::PlayerBullet);
	}
	firstRailTarget->SetActorLocation({ railBurstSettings.spawnDistance + 80.f, 0.f });
	secondRailTarget->SetActorLocation({ railBurstSettings.spawnDistance + 160.f, 0.f });
	thirdRailTarget->SetActorLocation({ railBurstSettings.spawnDistance + 240.f, 0.f });
	railBurstWorld.TickInternal(0.f);
	const shared_ptr<RailBurstProjectileActor> railBurst =
		SpawnRailBurst(railBurstWorld, railBurstOwner);
	if (!railBurst ||
		!NearlyEqual(railBurst->GetDamage(), railBurstSettings.baseDamage) ||
		!NearlyEqual(railBurst->GetProjectileSpeed(), railBurstSettings.projectileSpeed) ||
		!NearlyEqual(railBurst->GetMaximumRange(), railBurstSettings.range) ||
		railBurst->GetDamageTags().size() != 1 ||
		railBurst->GetDamageTags().front() != DamageTypeSchema::Energy)
	{
		return Fail("Rail Burst did not spawn with its Energy configuration");
	}
	const float firstDamage = railBurstSettings.baseDamage;
	railBurst->OnActorBeginOverlap(firstRailTarget.get());
	railBurst->OnActorBeginOverlap(firstRailTarget.get());
	railBurst->OnActorBeginOverlap(secondRailTarget.get());
	railBurst->OnActorBeginOverlap(thirdRailTarget.get());
	if (!NearlyEqual(firstRailTarget->GetHealth(), 1000.f - firstDamage) ||
		!NearlyEqual(secondRailTarget->GetHealth(), 1000.f - firstDamage) ||
		!NearlyEqual(thirdRailTarget->GetHealth(), 1000.f - firstDamage) ||
		!NearlyEqual(railBurst->GetDamage(), firstDamage) ||
		railBurst->GetHitTargetCount() != 3)
	{
		return Fail("Rail Burst did not apply one-time full-damage pierce hits");
	}
	const float railBurstX = railBurst->GetActorLocation().x;
	railBurstWorld.TickInternal(0.1f);
	if (!NearlyEqual(
				railBurst->GetActorLocation().x,
			railBurstX + railBurstSettings.projectileSpeed * 0.1f
		) ||
		!NearlyEqual(railBurst->GetVelocity().x, railBurstSettings.projectileSpeed) ||
		!NearlyEqual(railBurst->GetVelocity().y, 0.f))
	{
		return Fail("Rail Burst did not travel along its owner-forward direction");
	}

	const GameAbilityDefinition* astralSurgeDefinition =
		AbilityData::FindShippedAbilityDefinition(
			AbilityData::AstralSurge::AbilityId::Basic
		);
	if (!astralSurgeDefinition)
	{
		return Fail("Astral Surge shipped definition is missing");
	}
	World astralSurgeWorld{ nullptr };
	TestCombatant astralSurgeOwner{ &astralSurgeWorld, 1000.f };
	astralSurgeOwner.GetCombatRuntime().InitializeOwnerAttributes(1000.f);
	astralSurgeOwner.SetCollisionLayer(CollisionLayer::Player);
	astralSurgeOwner.SetActorRotation(90.f);
	LightYearsAbilitySystemComponent& astralSurgeAbilities =
		astralSurgeOwner.GetCombatRuntime().GetAbilitySystemComponent();
	if (!astralSurgeAbilities.GrantAbility(*astralSurgeDefinition).IsValid())
	{
		return Fail("Astral Surge could not be granted in the runtime test");
	}
	astralSurgeAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability3, true);
	astralSurgeAbilities.Tick(0.f);
	if (!astralSurgeAbilities.HasOwnedTag(GameplayTags::State::ActionLock::AbilityActivation) ||
		!astralSurgeAbilities.HasOwnedTag(GameplayTags::State::ActionLock::PrimaryWeaponFire) ||
		!astralSurgeAbilities.HasOwnedTag(GameplayTags::State::ActionLock::MovementInput) ||
		!astralSurgeAbilities.HasOwnedTag(AbilityData::AstralSurge::State::Focusing) ||
		!astralSurgeWorld.GetActorsByType<AstralSurgeProjectileActor>().empty())
	{
		return Fail("Astral Surge did not hold its focus locks before release");
	}
	astralSurgeAbilities.Tick(0.50f);
	if (!astralSurgeWorld.GetActorsByType<AstralSurgeProjectileActor>().empty())
	{
		return Fail("Astral Surge fired before its one-second focus completed");
	}
	astralSurgeAbilities.Tick(0.50f);
	astralSurgeWorld.TickInternal(0.f);
	if (astralSurgeWorld.GetActorsByType<AstralSurgeProjectileActor>().size() != 1)
	{
		return Fail("Astral Surge did not spawn its projectile at one second");
	}
	if (astralSurgeAbilities.HasOwnedTag(GameplayTags::State::ActionLock::AbilityActivation) ||
		astralSurgeAbilities.HasOwnedTag(GameplayTags::State::ActionLock::PrimaryWeaponFire) ||
		astralSurgeAbilities.HasOwnedTag(GameplayTags::State::ActionLock::MovementInput) ||
		astralSurgeAbilities.HasOwnedTag(AbilityData::AstralSurge::State::Focusing))
	{
		return Fail("Astral Surge did not release all focus locks at one second");
	}

	std::string energySpearValidationFailure;
	const GameAbilityDefinition* energySpearDefinition =
		AbilityData::FindShippedAbilityDefinition(
			AbilityData::EnergySpear::AbilityId::Basic
		);
	if (!energySpearDefinition ||
		!ValidateAbilityCatalog(
			AbilityData::GetShippedAbilityDefinitions(),
			&energySpearValidationFailure
		) ||
		energySpearDefinition->behaviorType != AbilityBehaviorType::EnergySpear ||
		energySpearDefinition->slot != sas::AbilitySlot::Ability1)
	{
		return Fail("Energy Spear shipped ability or behavior registration is invalid");
	}

	{
		// The ability instance owns only focus. Release hands the bounded movement
		// and piercing phase to a world actor, so input release cannot cancel it.
		World energySpearWorld{ nullptr };
		const shared_ptr<TestCombatant> energySpearOwner =
			energySpearWorld.SpawnActor<TestCombatant>(1000.f).lock();
		const shared_ptr<TestCombatant> energySpearTarget =
			energySpearWorld.SpawnActor<TestCombatant>(1000.f).lock();
		if (!energySpearOwner || !energySpearTarget)
		{
			return Fail("Energy Spear test actors could not spawn");
		}

		energySpearOwner->SetCollisionLayer(CollisionLayer::Player);
		energySpearTarget->SetCollisionLayer(CollisionLayer::Enemy);
		energySpearTarget->SetCollisionMask(CollisionLayer::Player);
		const sf::Vector2f spearDirection{ 0.f, -1.f };
		energySpearTarget->SetActorLocation(
			energySpearOwner->GetActorLocation() + spearDirection * 100.f
		);

		LightYearsAbilitySystemComponent& energySpearAbilities =
			energySpearOwner->GetAbilitySystemComponent();
		const sas::AbilityHandle energySpearHandle =
			energySpearAbilities.GrantAbility(*energySpearDefinition);
		if (!energySpearHandle.IsValid())
		{
			return Fail("Energy Spear could not be granted to its owner");
		}

		energySpearAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
		energySpearAbilities.Tick(0.f);
		energySpearWorld.TickInternal(0.f);
		if (!energySpearAbilities.GetOwnedTags().HasTag(
				GameplayTags::State::Ability::EnergySpear::Focusing
			) ||
			!energySpearAbilities.GetOwnedTags().HasTag(
				GameplayTagSchema::BlockMovementInput
			) ||
			energySpearWorld.GetActorsByType<DirectionalChargeTelegraphActor>().empty())
		{
			return Fail("Energy Spear focus did not apply the shared input lock with a directional telegraph");
		}

		energySpearAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability1, false);
		energySpearAbilities.Tick(0.f);
		energySpearWorld.TickInternal(0.f);
		const List<weak_ptr<EnergySpearTraversalActor>> traversalActors =
			energySpearWorld.GetActorsByType<EnergySpearTraversalActor>();
		const shared_ptr<EnergySpearTraversalActor> traversalActor =
			traversalActors.empty() ? shared_ptr<EnergySpearTraversalActor>{}
			: traversalActors.front().lock();
		if (!traversalActor ||
			!energySpearWorld.GetActorsByType<DirectionalChargeTelegraphActor>().empty() ||
			!energySpearAbilities.GetOwnedTags().HasTag(
				GameplayTagSchema::BlockMovementInput
			) ||
			!energySpearAbilities.GetOwnedTags().HasTag(
				GameplayTags::State::Ability::EnergySpear::Traversing
			))
		{
			return Fail("Energy Spear did not hand release to its traversal actor");
		}
		if (CanApplyContactDamage(*energySpearOwner, *energySpearTarget))
		{
			return Fail("Energy Spear traversal did not register its source-owned contact guard");
		}

		energySpearWorld.TickInternal(0.05f);
		if (traversalActor->GetHitTargetCount() != 1)
		{
			return Fail("Energy Spear traversal did not register its traversed target");
		}
		if (energySpearTarget->GetHealth() >= 1000.f ||
			!energySpearAbilities.GetOwnedTags().HasTag(
				GameplayTags::State::Ability::EnergySpear::Traversing
			))
		{
			return Fail("Energy Spear did not pierce and damage its traversed target");
		}

		// Let the minimum-distance traversal complete. A slightly larger step keeps
		// this lifecycle assertion independent from floating-point substep edges.
		energySpearWorld.TickInternal(0.10f);
		if (energySpearAbilities.GetOwnedTags().HasTag(
				GameplayTags::State::Ability::EnergySpear::Traversing
			) ||
			energySpearAbilities.GetOwnedTags().HasTag(
				GameplayTagSchema::BlockMovementInput
			))
		{
			if (energySpearAbilities.GetOwnedTags().HasTag(
				GameplayTags::State::Ability::EnergySpear::Traversing
			))
			{
				return Fail("Energy Spear did not clean up its traversal state");
			}
			return Fail("Energy Spear did not clean up its movement input lock");
		}
		if (!CanApplyContactDamage(*energySpearOwner, *energySpearTarget))
		{
			return Fail("Energy Spear traversal did not unregister its contact guard on finish");
		}
	}

	std::string gravityValidationFailure;
	const GameAbilityDefinition* gravityDefinition = AbilityData::FindShippedAbilityDefinition(
		AbilityData::GravityAnomaly::AbilityId::Basic
	);
	if (!ValidateAbilityCatalog(AbilityData::GetShippedAbilityDefinitions(), &gravityValidationFailure) ||
		!gravityDefinition || gravityDefinition->slot != sas::AbilitySlot::Ability1 ||
		std::string{ AbilityInputSchema::GetLabel(sas::AbilitySlot::Ability1) } != "Q" ||
		gravityDefinition->behaviorType != AbilityBehaviorType::GravityAnomaly ||
		gravityDefinition->damageTags.size() != 0 ||
		!AbilityActorRegistry::ValidateDefinition(
			LoadedAbilityActor("Actor.Ability.GravityAnomaly.Projectile.Basic")
		).isValid ||
		!AbilityActorRegistry::ValidateDefinition(
			LoadedAbilityActor("Actor.Ability.GravityAnomaly.Field.Basic")
		).isValid)
	{
		return Fail("Gravity Anomaly shipped ability or actor catalog validation failed");
	}
	PlayerSpaceShip defaultLoadoutShip{ nullptr };
	const GameAbility* lanceDriveLoadoutAbility =
		defaultLoadoutShip.GetAbilitySystemComponent()
			.FindAbility<GameAbility>(sas::AbilitySlot::Ability1);
	if (!lanceDriveLoadoutAbility ||
		lanceDriveLoadoutAbility->GetDefinition().abilityId != AbilityData::LanceDrive::AbilityId::Basic ||
		std::string{ AbilityInputSchema::GetLabel(sas::AbilitySlot::Ability1) } != "Q")
	{
		return Fail("Default player loadout did not place Lance Drive on Ability1/Q");
	}
	const GameAbility* relayPrismLoadoutAbility =
		defaultLoadoutShip.GetAbilitySystemComponent().FindAbility<GameAbility>(sas::AbilitySlot::Ability2);
	if (!relayPrismLoadoutAbility ||
		relayPrismLoadoutAbility->GetDefinition().abilityId != AbilityData::RelayPrism::AbilityId::Basic ||
		std::string{ AbilityInputSchema::GetLabel(sas::AbilitySlot::Ability2) } != "E")
	{
		return Fail("Default player loadout did not place Relay Prism on Ability2/E");
	}
	const GameAbility* glacialPressureLoadoutAbility =
		defaultLoadoutShip.GetAbilitySystemComponent().FindAbility<GameAbility>(sas::AbilitySlot::Ability3);
	if (!glacialPressureLoadoutAbility ||
		glacialPressureLoadoutAbility->GetDefinition().abilityId != AbilityData::GlacialPressure::AbilityId::Basic ||
		std::string{ AbilityInputSchema::GetLabel(sas::AbilitySlot::Ability3) } != "F")
	{
		return Fail("Default player loadout did not place Glacial Pressure on Ability3/F");
	}
	const GameAbility* ironcladProtocolLoadoutAbility =
		defaultLoadoutShip.GetAbilitySystemComponent().FindAbility<GameAbility>(sas::AbilitySlot::Ability4);
	if (!ironcladProtocolLoadoutAbility ||
		ironcladProtocolLoadoutAbility->GetDefinition().abilityId != AbilityData::IroncladProtocol::AbilityId::Basic ||
		std::string{ AbilityInputSchema::GetLabel(sas::AbilitySlot::Ability4) } != "R")
	{
		return Fail("Default player loadout did not place Ironclad Protocol on Ability4/R");
	}

	// Exercise the generic activation/action-spawn path used by the player Q
	// binding without involving player-only asset initialization.
	World crescentReaverWorld{ nullptr };
	const shared_ptr<TestCombatant> crescentReaverOwner =
		crescentReaverWorld.SpawnActor<TestCombatant>(100000.f).lock();
	const shared_ptr<TestCombatant> crescentReaverTarget =
		crescentReaverWorld.SpawnActor<TestCombatant>(100000.f).lock();
	if (!crescentReaverOwner || !crescentReaverTarget)
	{
		return Fail("Crescent Reaver input-path test actors could not spawn");
	}
	crescentReaverOwner->SetCollisionLayer(CollisionLayer::Player);
	crescentReaverOwner->SetCollisionMask(CollisionLayer::Enemy);
	crescentReaverTarget->SetCollisionLayer(CollisionLayer::Enemy);
	crescentReaverTarget->SetCollisionMask(CollisionLayer::PlayerBullet);
	crescentReaverTarget->SetActorLocation({ 0.f, -210.f });
	crescentReaverWorld.TickInternal(0.f);
	const sas::AbilityHandle crescentReaverHandle =
		crescentReaverOwner->GetAbilitySystemComponent().GrantAbility(
			*AbilityData::FindShippedAbilityDefinition(
				AbilityData::CrescentReaver::AbilityId::Basic
			),
			sas::AbilitySlot::Ability1
		);
	if (!crescentReaverHandle.IsValid())
	{
		return Fail("Crescent Reaver could not be granted to Ability1/Q");
	}
	crescentReaverOwner->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability1,
		true
	);
	crescentReaverOwner->GetAbilitySystemComponent().Tick(0.f);
	crescentReaverWorld.TickInternal(0.f);
	if (crescentReaverWorld.GetActorsByType<CrescentReaverProjectileActor>().size() != 1)
	{
		return Fail("Ability1/Q activation did not spawn the Crescent Reaver projectile");
	}
	GameAbility* crescentReaverAbility =
		crescentReaverOwner->GetAbilitySystemComponent().GetAbility(crescentReaverHandle);
	if (!crescentReaverAbility)
	{
		return Fail("Crescent Reaver runtime ability instance was not retained");
	}
	const float cooldownBeforeBounce = crescentReaverAbility->GetCooldownRemaining();
	crescentReaverWorld.TickInternal(0.05f);
	if (crescentReaverWorld.GetActorsByType<CrescentReaverProjectileActor>().size() != 1)
	{
		return Fail("Crescent Reaver projectile was removed on its first movement frame");
	}
	crescentReaverWorld.TickInternal(0.10f);
	if (crescentReaverAbility->GetCooldownRemaining() >= cooldownBeforeBounce)
	{
		return Fail("Crescent Reaver bounce did not reduce its source ability cooldown");
	}

	// A catalog slot is only a default binding. A newly acquired ability and an
	// already granted ability must both accept every swappable loadout slot.
	std::string runtimeSlotFailure;
	if (!defaultLoadoutShip.GetAbilityLoadout().EquipAbility(
			AbilityData::HullShock::AbilityId::Basic,
			sas::AbilitySlot::Ability2,
			&runtimeSlotFailure
		))
	{
		return Fail("A fresh loadout ability could not be granted to an alternate slot");
	}
	GameAbility* reboundHullShock = defaultLoadoutShip.GetAbilitySystemComponent().GetAbilityById(
		AbilityData::HullShock::AbilityId::Basic
	);
	if (!reboundHullShock || reboundHullShock->GetDefinition().slot != sas::AbilitySlot::Ability2 ||
		!defaultLoadoutShip.GetAbilityLoadout().EquipAbility(
			AbilityData::HullShock::AbilityId::Basic,
			sas::AbilitySlot::Ability3,
			&runtimeSlotFailure
		) ||
		reboundHullShock->GetDefinition().slot != sas::AbilitySlot::Ability3)
	{
		return Fail("Fresh grant and rebind do not share the runtime loadout-slot contract");
	}

	const GameAbilityDefinition* overdriveDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::OverdriveCore::AbilityId::Basic);
	if (!overdriveDefinition || overdriveDefinition->slot != sas::AbilitySlot::Ability4 ||
		std::string{ AbilityInputSchema::GetLabel(sas::AbilitySlot::Ability4) } != "R")
	{
		return Fail("Overdrive Core shipped definition is not aligned with Ability4/R");
	}
	World overdriveWorld{ nullptr };
	const shared_ptr<TestCombatant> overdriveOwner =
		overdriveWorld.SpawnActor<TestCombatant>(100000.f).lock();
	const shared_ptr<TestCombatant> overdriveTarget =
		overdriveWorld.SpawnActor<TestCombatant>(100000.f).lock();
	if (!overdriveOwner || !overdriveTarget)
	{
		return Fail("Overdrive Core runtime test actors could not spawn");
	}
	overdriveOwner->GetCombatRuntime().InitializeOwnerAttributes(100000.f);
	overdriveOwner->GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::AttackPower, 20.f }
	);
	overdriveOwner->GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::AttackSpeed, 2.f }
	);
	overdriveOwner->GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::CriticalChance, 0.2f }
	);
	overdriveOwner->SetCollisionLayer(CollisionLayer::Player);
	overdriveOwner->SetCollisionMask(CollisionLayer::Enemy);
	overdriveOwner->SetActorLocation({ 0.f, 0.f });
	overdriveTarget->SetCollisionLayer(CollisionLayer::Enemy);
	overdriveTarget->SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet);
	overdriveTarget->SetActorLocation({ 300.f, 0.f });
	overdriveWorld.TickInternal(0.f);
	const float overdriveHealthBefore = overdriveTarget->GetHealth();
	const float overdriveAttackSpeedBefore = overdriveOwner->GetAbilitySystemComponent()
		.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed);
	const sas::AbilityHandle overdriveHandle =
		overdriveOwner->GetAbilitySystemComponent().GrantAbility(*overdriveDefinition);
	if (!overdriveHandle.IsValid())
	{
		return Fail("Overdrive Core could not be granted to the Ability4 slot");
	}
	overdriveOwner->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability4, true);
	overdriveOwner->GetAbilitySystemComponent().Tick(0.f);
	overdriveWorld.TickInternal(0.f);
	if (overdriveWorld.GetActorsByType<OverdriveCoreProjectileActor>().size() != 1)
	{
		return Fail("Overdrive Core did not spawn its first targeted rocket");
	}
	// The selected enemy can move after targeting. The projectile must follow
	// that actor; exploding at the original snapshot position would miss here.
	overdriveTarget->SetActorLocation({ 300.f, 150.f });
	overdriveWorld.TickInternal(0.25f);
	if (overdriveTarget->GetHealth() >= overdriveHealthBefore)
	{
		return Fail("Overdrive Core projectile did not follow a moving target to deal damage");
	}
	overdriveOwner->GetAbilitySystemComponent().Tick(1.f);
	const float overdriveBoostBase = sas::FindAttributeValue(
		overdriveDefinition->attributes,
		AbilityData::OverdriveCore::Attribute::AttackSpeedBoostBase,
		0.f
	);
	const float overdriveBoostCriticalScale = sas::FindAttributeValue(
		overdriveDefinition->attributes,
		AbilityData::OverdriveCore::Attribute::AttackSpeedBoostCriticalChanceScale,
		0.f
	);
	const float expectedOverdriveAttackSpeed = overdriveAttackSpeedBefore +
		overdriveBoostBase * (1.f + 0.2f * overdriveBoostCriticalScale);
	if (!overdriveOwner->GetAbilitySystemComponent().GetOwnedTags().HasTag(
			AbilityData::OverdriveCore::State::AttackSpeedBoostActive
		) ||
		!NearlyEqual(
			overdriveOwner->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
				OwnerAttributeIds::AttackSpeed
			),
			expectedOverdriveAttackSpeed
		))
	{
		return Fail("Overdrive Core did not apply its critical-scaled AttackSpeed effect");
	}
	overdriveWorld.TickInternal(1.f);
	if (overdriveTarget->GetHealth() >= overdriveHealthBefore ||
		!overdriveWorld.GetActorsByType<OverdriveCoreProjectileActor>().empty())
	{
		return Fail("Overdrive Core rockets did not reach and damage the selected target");
	}

	// Repeat the same path with the real gameplay classes. TestCombatant above
	// verifies the generic combat route; this catches differences in the ship,
	// shield, collision, and EnemySpaceShip health implementation.
		ShipDefinition dummyDefinition = ShipData::Ship_Enemy_Hexagon;
		dummyDefinition.health = 99999.f;
		dummyDefinition.speed = { 0.f, 0.f };
		// The real ship classes load their textures through AssetManager. The
		// gameplay test runs from the repository root, so point that manager at
		// the same asset directory used by the game before spawning a ship.
		AssetManager::GetAssetManager().SetAssetRootDirectory("LightYearsGame/assets/");
		World realOverdriveWorld{ nullptr };
	const shared_ptr<PlayerSpaceShip> realOverdriveOwner =
		realOverdriveWorld.SpawnActor<PlayerSpaceShip>().lock();
	const shared_ptr<DummyEnemy> realOverdriveDummy =
		realOverdriveWorld.SpawnActor<DummyEnemy>(dummyDefinition).lock();
	if (!realOverdriveOwner || !realOverdriveDummy)
	{
		return Fail("Overdrive Core real ship/dummy test actors could not spawn");
	}
	std::string overdriveLoadoutFailure;
	if (!realOverdriveOwner->GetAbilityLoadout().EquipAbility(
			AbilityData::OverdriveCore::AbilityId::Basic,
			sas::AbilitySlot::Ability4,
			&overdriveLoadoutFailure
		))
	{
		const std::string failureMessage =
			"Overdrive Core could not replace the default R-slot ability: " +
			overdriveLoadoutFailure;
		return Fail(failureMessage.c_str());
	}
	realOverdriveOwner->SetUseScreenClamp(false);
	realOverdriveOwner->SetActorLocation({ 0.f, 0.f });
	// ArenaTestLevel places both ships at the arena center. Keeping the same
	// position here verifies that a homing rocket still moves and explodes when
	// its initial target snapshot has zero forward distance.
	realOverdriveDummy->SetActorLocation({ 0.f, 0.f });
	realOverdriveWorld.TickInternal(0.f);
	const float realDummyHealthBefore = realOverdriveDummy->GetHealthComponent().GetHealth();
	realOverdriveOwner->GetAbilitySystemComponent().SetAbilitySlotInput(
		sas::AbilitySlot::Ability4,
		true
	);
	realOverdriveOwner->GetAbilitySystemComponent().Tick(0.f);
	realOverdriveWorld.TickInternal(0.f);
	if (realOverdriveWorld.GetActorsByType<OverdriveCoreProjectileActor>().size() != 1)
	{
		return Fail("Overdrive Core did not spawn a projectile against the real dummy");
	}
	realOverdriveOwner->GetAbilitySystemComponent().Tick(1.f);
	realOverdriveWorld.TickInternal(1.f);
	if (realOverdriveDummy->GetHealthComponent().GetHealth() >= realDummyHealthBefore)
	{
		return Fail("Overdrive Core projectile did not damage the real DummyEnemy health component");
	}
	AbilityActorDefinition gravityProjectileWithoutProfile =
		LoadedAbilityActor("Actor.Ability.GravityAnomaly.Projectile.Basic");
	gravityProjectileWithoutProfile.presentationProfileId = sas::ContentId{};
	AbilityActorDefinition gravityProjectileWithFieldProfile =
		LoadedAbilityActor("Actor.Ability.GravityAnomaly.Projectile.Basic");
	gravityProjectileWithFieldProfile.presentationProfileId =
		GravityAnomalyPresentationIds::FieldBasic;
	if (AbilityActorRegistry::ValidateDefinition(gravityProjectileWithoutProfile).isValid ||
		AbilityActorRegistry::ValidateDefinition(gravityProjectileWithFieldProfile).isValid)
	{
		return Fail("Gravity Anomaly actor validation accepted a missing or cross-family typed profile");
	}

	const AbilityActorDefinition& gravityProjectileActor =
		LoadedAbilityActor("Actor.Ability.GravityAnomaly.Projectile.Basic");
	const AbilityActorDefinition& gravityFieldActor =
		LoadedAbilityActor("Actor.Ability.GravityAnomaly.Field.Basic");
	const auto gravityAttribute = [&](const AbilityActorDefinition& actor, const sas::AttributeId& attributeId)
	{
		return sas::FindAttributeValue(actor.attributes, attributeId, 0.f);
	};
	struct GravityTestSettings
	{
		float cooldown;
		float castRange;
		float projectileSpeed;
		float baseDuration;
		float baseRadius;
		float pullStrength;
		float slowMagnitude;
		float spawnDistance;
	};
	const GravityTestSettings gravitySettingsValue{
		gravityDefinition->cooldown,
		gravityAttribute(gravityProjectileActor, CommonAttributeIds::Range),
		gravityAttribute(gravityProjectileActor, AbilityData::GravityAnomaly::Actor::Projectile::ProjectileSpeed),
		gravityAttribute(gravityFieldActor, CommonAttributeIds::Duration),
		gravityAttribute(gravityFieldActor, CommonAttributeIds::Radius),
		gravityAttribute(gravityFieldActor, AbilityData::GravityAnomaly::Actor::Field::PullStrength),
		gravityAttribute(gravityFieldActor, AbilityData::GravityAnomaly::Actor::Field::SlowMagnitude),
		gravityProjectileActor.spawnDistance
	};
	const GravityTestSettings* gravitySettings = &gravitySettingsValue;
	const sas::GameplayEffectDefinition& gravityInsideEffect =
		EffectData::GravityAnomalyInsideEffect;
	const auto MakeGravityInsideSpec = [](float slowMagnitude)
	{
		sas::GameplayEffectSpec spec =
			sas::MakeGameplayEffectSpec(EffectData::GravityAnomalyInsideEffect);
		SetGameplayEffectModifierMagnitude(
			spec,
			OwnerAttributeIds::MovementSlow,
			slowMagnitude
		);
		return spec;
	};
	if (!gravitySettings || gravitySettings->cooldown != 8.f ||
		gravitySettings->castRange != 900.f || gravitySettings->projectileSpeed != 2000.f ||
		gravitySettings->baseDuration != 2.5f || gravitySettings->baseRadius != 320.f ||
		gravitySettings->pullStrength != 1500.f || gravitySettings->slowMagnitude != 0.40f ||
		gravityInsideEffect.effectId != AbilityData::GravityAnomaly::Effect::InsideEffectId ||
		gravityInsideEffect.durationPolicy != sas::GameplayEffectDurationPolicy::Duration ||
		!NearlyEqual(
			gravityInsideEffect.duration,
			AbilityData::GravityAnomaly::Effect::InsideEffectDurationSeconds
		) ||
		gravityInsideEffect.stackingPolicy != sas::GameplayEffectStackingPolicy::RefreshDuration ||
		!gravityInsideEffect.sourceScopedApplication)
	{
		return Fail("Gravity Anomaly base settings or source-scoped inside effect are invalid");
	}

	auto SpawnGravityProjectile = [&](
		World& world,
		TestCombatant& owner,
		int level,
		float resolvedMaxHealth
	)
	{
		owner.GetCombatRuntime().InitializeOwnerAttributes(resolvedMaxHealth);
		owner.SetActorLocation({ 0.f, 0.f });
		owner.SetActorRotation(90.f);
		owner.SetCollisionLayer(CollisionLayer::Player);
		const sas::AbilityHandle handle = owner.GetAbilitySystemComponent().GrantAbility(*gravityDefinition);
		if (!handle.IsValid() ||
			(level > 1 && !owner.GetCombatRuntime().GetAbilitySystemComponent().SetAbilityLevel(handle, level)))
		{
			return shared_ptr<GravityAnomalyProjectileActor>{};
		}
		owner.GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
		owner.GetCombatRuntime().GetAbilitySystemComponent().Tick(0.f);
		world.TickInternal(0.f);
		const List<weak_ptr<GravityAnomalyProjectileActor>> projectiles =
			world.GetActorsByType<GravityAnomalyProjectileActor>();
		return projectiles.size() == 1
			? projectiles.front().lock()
			: shared_ptr<GravityAnomalyProjectileActor>{};
	};

	World gravityCursorWorld{ nullptr };
	TestCombatant gravityCursorOwner{ &gravityCursorWorld, 1000.f };
	gravityCursorOwner.SetActorLocation({ 0.f, 0.f });
	gravityCursorOwner.SetActorRotation(90.f);
	const shared_ptr<GravityAnomalyProjectileActor> clampedGravityProjectile =
		std::dynamic_pointer_cast<GravityAnomalyProjectileActor>(
			AbilityActorRegistry::Spawn(
				AbilityActorSpawnContext{
					gravityCursorOwner,
					LoadedAbilityActor("Actor.Ability.GravityAnomaly.Projectile.Basic"),
					LoadedAbilityActor("Actor.Ability.GravityAnomaly.Projectile.Basic").attributes,
					std::optional<sf::Vector2f>{ sf::Vector2f{ 2000.f, 0.f } }
				}
			).lock()
		);
	if (!clampedGravityProjectile)
	{
		return Fail("Gravity Anomaly projectile could not spawn for target-resolution test");
	}
	clampedGravityProjectile->SetActorLocation({ gravitySettings->spawnDistance, 0.f });
	clampedGravityProjectile->SetActorRotation(90.f);
	clampedGravityProjectile->ConfigureFromAttributes(
		LoadedAbilityActor("Actor.Ability.GravityAnomaly.Projectile.Basic").attributes
	);
	gravityCursorWorld.TickInternal(0.f);
	if (!NearlyEqual(clampedGravityProjectile->GetResolvedTargetLocation().x, gravitySettings->castRange) ||
		!NearlyEqual(clampedGravityProjectile->GetResolvedTargetLocation().y, 0.f) ||
		!gravityCursorWorld.GetActorsByType<GravityAnomalyFieldActor>().empty())
	{
		return Fail("Gravity Anomaly did not clamp its resolved cursor target or spawned a field too early");
	}
	clampedGravityProjectile->OnActorBeginOverlap(&gravityCursorOwner);
	gravityCursorWorld.TickInternal(0.1f);
	if (!gravityCursorWorld.GetActorsByType<GravityAnomalyFieldActor>().empty())
	{
		return Fail("Gravity Anomaly projectile formed a field before reaching its resolved target");
	}
	gravityCursorWorld.TickInternal(0.5f);
	gravityCursorWorld.TickInternal(0.f);
	const List<weak_ptr<GravityAnomalyFieldActor>> resolvedGravityFields =
		gravityCursorWorld.GetActorsByType<GravityAnomalyFieldActor>();
	if (!clampedGravityProjectile->HasSpawnedField() || resolvedGravityFields.size() != 1 ||
		!resolvedGravityFields.front().lock() ||
		!NearlyEqual(resolvedGravityFields.front().lock()->GetActorLocation().x, gravitySettings->castRange))
	{
		return Fail("Gravity Anomaly projectile did not create one field at the resolved target");
	}

	auto SpawnGravityField = [](
		World& world,
		Actor& owner,
		const sf::Vector2f& location
	)
	{
		const sas::GameplayAttributeList attributes =
		LoadedAbilityActor("Actor.Ability.GravityAnomaly.Field.Basic").attributes;
		const shared_ptr<GravityAnomalyFieldActor> field =
			std::dynamic_pointer_cast<GravityAnomalyFieldActor>(
				AbilityActorRegistry::Spawn(
					AbilityActorSpawnContext{
						owner,
						LoadedAbilityActor("Actor.Ability.GravityAnomaly.Field.Basic"),
						attributes,
						location
					}
				).lock()
			);
		if (field)
		{
			field->SetActorLocation(location);
			field->ConfigureFromAttributes(attributes);
		}
		return field;
	};

	World gravityFieldWorld{ nullptr };
	const shared_ptr<TestCombatant> gravityCaster =
		gravityFieldWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestCombatant> gravityPlayerTarget =
		gravityFieldWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestCombatant> gravityEnemyTarget =
		gravityFieldWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<Actor> gravityProjectileTarget = gravityFieldWorld.SpawnActor<Actor>().lock();
	const shared_ptr<Actor> gravityPickupTarget = gravityFieldWorld.SpawnActor<Actor>().lock();
	if (!gravityCaster || !gravityPlayerTarget || !gravityEnemyTarget ||
		!gravityProjectileTarget || !gravityPickupTarget)
	{
		return Fail("Gravity Anomaly field test actors could not spawn");
	}
	gravityCaster->SetCollisionLayer(CollisionLayer::Player);
	gravityPlayerTarget->SetCollisionLayer(CollisionLayer::Player);
	gravityEnemyTarget->SetCollisionLayer(CollisionLayer::Enemy);
	gravityCaster->SetActorLocation({ 20.f, 0.f });
	gravityPlayerTarget->SetActorLocation({ -30.f, 0.f });
	gravityEnemyTarget->SetActorLocation({ 40.f, 0.f });
	gravityProjectileTarget->SetCollisionLayer(CollisionLayer::PlayerBullet);
	gravityProjectileTarget->SetActorLocation({ 10.f, 0.f });
	gravityProjectileTarget->SetVelocity({ 7.f, 0.f });
	gravityPickupTarget->SetCollisionLayer(CollisionLayer::Powerup);
	gravityPickupTarget->SetActorLocation({ -10.f, 0.f });
	gravityPickupTarget->SetVelocity({ 3.f, 0.f });
	const shared_ptr<GravityAnomalyFieldActor> gravityField = SpawnGravityField(
		gravityFieldWorld,
		*gravityCaster,
		{ 0.f, 0.f }
	);
	if (!gravityField)
	{
		return Fail("Gravity Anomaly field could not spawn");
	}
	gravityFieldWorld.TickInternal(0.f);
	gravityFieldWorld.TickInternal(0.f);
	for (const shared_ptr<TestCombatant>& target : {
		gravityCaster,
		gravityPlayerTarget,
		gravityEnemyTarget
	})
	{
		if (!target->GetAbilitySystemComponent().FindGameplayEffectById(
			AbilityData::GravityAnomaly::Effect::InsideEffectId
		))
		{
			return Fail("Gravity Anomaly did not affect caster, player, and enemy combat targets equally");
		}
	}
	if (gravityProjectileTarget->GetVelocity().x != 7.f || gravityPickupTarget->GetVelocity().x != 3.f ||
		gravityField->GetAffectedTargetCount() != 3 ||
		gravityFieldWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty())
	{
		return Fail("Gravity Anomaly affected a non-combat actor or missed effect visual lifecycle setup");
	}

	// Exercise the concrete player lifecycle too: gameplay code reaches this path
	// only after BeginPlay has registered the ship's runtime-attribute listeners.
	const shared_ptr<PlayerSpaceShip> gravityPlayerLifecycleTarget =
		std::make_shared<PlayerSpaceShip>(nullptr);
	if (!gravityPlayerLifecycleTarget)
	{
		return Fail("Gravity Anomaly player lifecycle target could not spawn");
	}
	gravityPlayerLifecycleTarget->BeginPlayInternal();
	const sas::GameplayEffectHandle gravityPlayerLifecycleEffect =
		gravityPlayerLifecycleTarget->GetAbilitySystemComponent().ApplyGameplayEffect(
			MakeGravityInsideSpec(0.20f)
		);
	if (!gravityPlayerLifecycleEffect.IsValid() ||
		!gravityPlayerLifecycleTarget->GetAbilitySystemComponent().FindGameplayEffectById(
			AbilityData::GravityAnomaly::Effect::InsideEffectId
		))
	{
		return Fail("Gravity Anomaly crashed or failed while applying its effect to a begun player ship");
	}

	// A real player is world-owned, so entering the field also queues and runs the
	// effect visual. Keep this covered separately from the world-less lifecycle
	// check above: that path cannot expose actor-spawn/lifecycle regressions.
	World gravityLivePlayerWorld{ nullptr };
	const shared_ptr<PlayerSpaceShip> gravityLivePlayer =
		gravityLivePlayerWorld.SpawnActor<PlayerSpaceShip>().lock();
	if (!gravityLivePlayer)
	{
		return Fail("Gravity Anomaly could not spawn a live player target");
	}
	gravityLivePlayer->SetUseScreenClamp(false);
	gravityLivePlayer->SetInvulnerability(false);
	gravityLivePlayer->SetActorLocation({ 0.f, 0.f });
	gravityLivePlayerWorld.TickInternal(0.f);
	const shared_ptr<GravityAnomalyFieldActor> gravityLivePlayerField = SpawnGravityField(
		gravityLivePlayerWorld,
		*gravityLivePlayer,
		{ 0.f, 0.f }
	);
	if (!gravityLivePlayerField)
	{
		return Fail("Gravity Anomaly could not spawn around a live player target");
	}
	gravityLivePlayerWorld.TickInternal(0.f);
	gravityLivePlayerWorld.TickInternal(0.1f);
	const bool gravityLivePlayerHasEffect =
		gravityLivePlayer->GetAbilitySystemComponent().FindGameplayEffectById(
			AbilityData::GravityAnomaly::Effect::InsideEffectId
		) != nullptr;
	const bool gravityLivePlayerHasVisual =
		!gravityLivePlayerWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty();
	if (!gravityLivePlayerHasEffect || !gravityLivePlayerHasVisual)
	{
		return Fail(gravityLivePlayerHasEffect
			? "Gravity Anomaly did not spawn its live-player effect visual"
			: "Gravity Anomaly did not apply its live-player gameplay effect");
	}
	gravityLivePlayerField->Destroy();
	gravityLivePlayerWorld.TickInternal(0.f);
	if (!gravityLivePlayer->GetAbilitySystemComponent().FindGameplayEffectById(
		AbilityData::GravityAnomaly::Effect::InsideEffectId
	) || gravityLivePlayerWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty())
	{
		return Fail("Gravity Anomaly did not preserve a live player's slow after its field ended");
	}
	gravityLivePlayerWorld.TickInternal(
		AbilityData::GravityAnomaly::Effect::InsideEffectDurationSeconds - 0.1f
	);
	if (!gravityLivePlayer->GetAbilitySystemComponent().FindGameplayEffectById(
		AbilityData::GravityAnomaly::Effect::InsideEffectId
	))
	{
		return Fail("Gravity Anomaly slow expired before its two-second grace period ended");
	}
	gravityLivePlayerWorld.TickInternal(0.2f);
	if (gravityLivePlayer->GetAbilitySystemComponent().FindGameplayEffectById(
		AbilityData::GravityAnomaly::Effect::InsideEffectId
	) || !gravityLivePlayerWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty())
	{
		return Fail("Gravity Anomaly did not clean up a live player's slow and visual after two seconds");
	}

	gravityEnemyTarget->GetCombatRuntime().Tick(0.1f);
	if (!(gravityEnemyTarget->GetVelocity().x < 0.f))
	{
		return Fail("Gravity Anomaly pull did not add a center-directed velocity change");
	}

	TestCombatant pullTarget;
	pullTarget.SetActorLocation({ 50.f, 0.f });
	GravityAnomalyRuntimeContext pullContext;
	pullContext.center = { 0.f, 0.f };
	pullContext.resolvedRadius = 100.f;
	pullContext.resolvedPullStrength = 500.f;
	const sas::GameplayEffectHandle pullHandle = pullTarget.GetAbilitySystemComponent().ApplyGameplayEffect(
		MakeGravityInsideSpec(0.20f),
		sas::GameplayEffectSourceContext{
			nullptr,
			&pullContext,
			std::make_shared<GravityAnomalyRuntimeContext>(pullContext)
		}
	);
	pullTarget.GetCombatRuntime().Tick(0.1f);
	if (!pullHandle.IsValid() || !NearlyEqual(pullTarget.GetVelocity().x, -12.5f) ||
		!NearlyEqual(pullTarget.GetVelocity().y, 0.f))
	{
		return Fail("Gravity Anomaly pull falloff or delta-time integration is incorrect");
	}
	pullTarget.SetActorLocation({ 0.f, 0.f });
	pullTarget.SetVelocity({ 0.f, 0.f });
	pullTarget.GetCombatRuntime().Tick(0.1f);
	if (!std::isfinite(pullTarget.GetVelocity().x) || !std::isfinite(pullTarget.GetVelocity().y))
	{
		return Fail("Gravity Anomaly pull produced a non-finite center velocity");
	}

	SpaceShip playerSlowMovement{ nullptr, ShipData::Ship_Player_Fighter };
	SpaceShip enemySlowMovement{ nullptr, ShipData::Ship_Player_Fighter };
	playerSlowMovement.SetCollisionLayer(CollisionLayer::Player);
	enemySlowMovement.SetCollisionLayer(CollisionLayer::Enemy);
	playerSlowMovement.SetVelocity({ 100.f, 0.f });
	enemySlowMovement.SetVelocity({ 100.f, 0.f });
	const sas::GameplayEffectHandle playerSlowHandle = playerSlowMovement.GetAbilitySystemComponent().ApplyGameplayEffect(
		MakeGravityInsideSpec(0.20f)
	);
	const sas::GameplayEffectHandle enemySlowHandle = enemySlowMovement.GetAbilitySystemComponent().ApplyGameplayEffect(
		MakeGravityInsideSpec(0.20f)
	);
	playerSlowMovement.Tick(0.1f);
	enemySlowMovement.Tick(0.1f);
	if (!NearlyEqual(playerSlowMovement.GetActorLocation().x, 8.f) ||
		!NearlyEqual(enemySlowMovement.GetActorLocation().x, 8.f))
	{
		return Fail("GameplayEffect movement slow did not reduce real player and enemy movement");
	}
	playerSlowMovement.GetAbilitySystemComponent().RemoveGameplayEffect(playerSlowHandle);
	enemySlowMovement.GetAbilitySystemComponent().RemoveGameplayEffect(enemySlowHandle);
	playerSlowMovement.Tick(0.1f);
	enemySlowMovement.Tick(0.1f);
	if (!NearlyEqual(playerSlowMovement.GetActorLocation().x, 18.f) ||
		!NearlyEqual(enemySlowMovement.GetActorLocation().x, 18.f))
	{
		return Fail("GameplayEffect movement slow did not cleanly restore movement");
	}

	World gravitySourceWorld{ nullptr };
	const shared_ptr<TestCombatant> gravitySourceOwner =
		gravitySourceWorld.SpawnActor<TestCombatant>(1000.f).lock();
	const shared_ptr<TestCombatant> gravitySourceTarget =
		gravitySourceWorld.SpawnActor<TestCombatant>(1000.f).lock();
	if (!gravitySourceOwner || !gravitySourceTarget)
	{
		return Fail("Gravity Anomaly source ownership actors could not spawn");
	}
	gravitySourceTarget->SetActorLocation({ 0.f, 0.f });
	const shared_ptr<GravityAnomalyFieldActor> firstGravityField = SpawnGravityField(
		gravitySourceWorld, *gravitySourceOwner, { 0.f, 0.f }
	);
	const shared_ptr<GravityAnomalyFieldActor> secondGravityField = SpawnGravityField(
		gravitySourceWorld, *gravitySourceOwner, { 0.f, 0.f }
	);
	gravitySourceWorld.TickInternal(0.f);
	gravitySourceWorld.TickInternal(0.f);
	const auto CountGravityEffects = [](const TestCombatant& target)
	{
		size_t count = 0;
		for (const sas::GameplayEffectRuntimeSnapshot& snapshot : target.GetAbilitySystemComponent().BuildGameplayEffectSnapshots())
		{
			count += snapshot.effectId ==
				AbilityData::GravityAnomaly::Effect::InsideEffectId ? 1u : 0u;
		}
		return count;
	};
	if (!firstGravityField || !secondGravityField ||
		CountGravityEffects(*gravitySourceTarget) != 2 ||
		firstGravityField->GetAffectedTargetCount() != 2 ||
		secondGravityField->GetAffectedTargetCount() != 2)
	{
		return Fail("Gravity Anomaly source-scoped applications stacked or registered incorrectly");
	}
	firstGravityField->Destroy();
	gravitySourceWorld.TickInternal(0.f);
	if (CountGravityEffects(*gravitySourceTarget) != 2)
	{
		return Fail("Gravity Anomaly field cleanup removed an effect before its grace period ended");
	}
	secondGravityField->Destroy();
	gravitySourceWorld.TickInternal(0.f);
	if (CountGravityEffects(*gravitySourceTarget) != 2)
	{
		return Fail("Gravity Anomaly second field removed an effect before the grace period ended");
	}
	gravitySourceTarget->GetCombatRuntime().Tick(
		AbilityData::GravityAnomaly::Effect::InsideEffectDurationSeconds + 0.1f
	);
	if (CountGravityEffects(*gravitySourceTarget) != 0)
	{
		return Fail("Gravity Anomaly field cleanup left an expired effect active");
	}

	World baselineGravityWorld{ nullptr };
	TestCombatant baselineGravityOwner{ &baselineGravityWorld, 1000.f };
	const shared_ptr<GravityAnomalyProjectileActor> baselineGravityProjectile =
		SpawnGravityProjectile(baselineGravityWorld, baselineGravityOwner, 1, 0.f);
	World scaledGravityWorld{ nullptr };
	TestCombatant scaledGravityOwner{ &scaledGravityWorld, 1000.f };
	const shared_ptr<GravityAnomalyProjectileActor> scaledGravityProjectile =
		SpawnGravityProjectile(scaledGravityWorld, scaledGravityOwner, 1, 100.f);
	if (!baselineGravityProjectile || !scaledGravityProjectile ||
		!NearlyEqual(baselineGravityProjectile->GetResolvedFieldRadius(), 320.f) ||
		!NearlyEqual(baselineGravityProjectile->GetResolvedFieldDuration(), 2.5f) ||
		!NearlyEqual(scaledGravityProjectile->GetResolvedFieldRadius(), 340.f) ||
		!NearlyEqual(scaledGravityProjectile->GetResolvedFieldDuration(), 2.75f) ||
		!NearlyEqual(scaledGravityProjectile->GetProjectileSpeed(), baselineGravityProjectile->GetProjectileSpeed()) ||
		!NearlyEqual(scaledGravityProjectile->GetCastRange(), baselineGravityProjectile->GetCastRange()) ||
		!NearlyEqual(scaledGravityProjectile->GetResolvedPullStrength(), baselineGravityProjectile->GetResolvedPullStrength()) ||
		!NearlyEqual(scaledGravityProjectile->GetResolvedSlowMagnitude(), baselineGravityProjectile->GetResolvedSlowMagnitude()))
	{
		return Fail("Gravity Anomaly MaxHealth scaling affected values other than radius and duration");
	}

	World levelGravityWorld{ nullptr };
	TestCombatant levelGravityOwner{ &levelGravityWorld, 1000.f };
	const shared_ptr<GravityAnomalyProjectileActor> levelGravityProjectile =
		SpawnGravityProjectile(levelGravityWorld, levelGravityOwner, 15, 0.f);
	const GameAbility* levelGravityInstance = levelGravityOwner.GetCombatRuntime().GetAbilitySystemComponent().GetAbility(
		sas::AbilitySlot::Ability1
	);
	if (!levelGravityProjectile || !levelGravityInstance ||
		!NearlyEqual(levelGravityInstance->GetCooldownDuration(), 6.6f) ||
		!NearlyEqual(levelGravityProjectile->GetResolvedFieldDuration(), 2.92f) ||
		!NearlyEqual(levelGravityProjectile->GetResolvedFieldRadius(), 348.f) ||
		!NearlyEqual(levelGravityProjectile->GetResolvedPullStrength(), 1780.f) ||
		!NearlyEqual(levelGravityProjectile->GetResolvedSlowMagnitude(), 0.47f) ||
		!NearlyEqual(levelGravityProjectile->GetProjectileSpeed(), 2350.f) ||
		!NearlyEqual(levelGravityProjectile->GetCastRange(), 970.f))
	{
		return Fail("Gravity Anomaly level progression was not cumulative through level fifteen");
	}
	levelGravityOwner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::AbilityHaste, 100.f }
	);
	if (!(levelGravityInstance->GetCooldownDuration() < 6.6f))
	{
		return Fail("Gravity Anomaly did not use centralized AbilityHaste cooldown resolution");
	}

	// A target inside the field is refreshed to exactly two seconds. Leaving the
	// field stops that refresh, but does not remove the slow prematurely.
	gravityPlayerTarget->SetActorLocation({ gravityField->GetResolvedRadius() + 10.f, 0.f });
	gravityFieldWorld.TickInternal(0.f);
	if (!gravityPlayerTarget->GetAbilitySystemComponent().FindGameplayEffectById(
		AbilityData::GravityAnomaly::Effect::InsideEffectId
	))
	{
		return Fail("Gravity Anomaly removed slow immediately when a target left the field");
	}
	gravityPlayerTarget->GetCombatRuntime().Tick(
		AbilityData::GravityAnomaly::Effect::InsideEffectDurationSeconds - 0.1f
	);
	if (!gravityPlayerTarget->GetAbilitySystemComponent().FindGameplayEffectById(
		AbilityData::GravityAnomaly::Effect::InsideEffectId
	))
	{
		return Fail("Gravity Anomaly slow did not persist for two seconds after leaving the field");
	}
	gravityPlayerTarget->GetCombatRuntime().Tick(0.2f);
	if (gravityPlayerTarget->GetAbilitySystemComponent().FindGameplayEffectById(
		AbilityData::GravityAnomaly::Effect::InsideEffectId
	))
	{
		return Fail("Gravity Anomaly slow did not expire after leaving the field");
	}

	gravityField->Destroy();
	gravityFieldWorld.TickInternal(0.f);
	if (!gravityCaster->GetAbilitySystemComponent().FindGameplayEffectById(
			AbilityData::GravityAnomaly::Effect::InsideEffectId
		) || gravityFieldWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty())
	{
		return Fail("Gravity Anomaly field destruction did not preserve active slows for two seconds");
	}
	gravityCaster->GetCombatRuntime().Tick(
		AbilityData::GravityAnomaly::Effect::InsideEffectDurationSeconds + 0.1f
	);
	gravityEnemyTarget->GetCombatRuntime().Tick(
		AbilityData::GravityAnomaly::Effect::InsideEffectDurationSeconds + 0.1f
	);
	gravityFieldWorld.TickInternal(0.f);
	if (gravityCaster->GetAbilitySystemComponent().FindGameplayEffectById(
			AbilityData::GravityAnomaly::Effect::InsideEffectId
		) || !gravityFieldWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty())
	{
		return Fail("Gravity Anomaly field destruction did not clean up expired slows and visuals");
	}

	std::string dashValidationFailure;
	const GameAbilityDefinition* dashDefinition =
		AbilityData::FindShippedAbilityDefinition(AbilityData::Dash::AbilityId::Basic);
	if (!ValidateAbilityCatalog(AbilityData::GetShippedAbilityDefinitions(), &dashValidationFailure) ||
		!dashDefinition || dashDefinition->slot != sas::AbilitySlot::Ability3 ||
		dashDefinition->activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
		dashDefinition->lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
		dashDefinition->cooldown <= 0.f || dashDefinition->duration <= 0.f ||
		dashDefinition->maxCharges != 1 || !dashDefinition->damageTags.empty() ||
		!dashDefinition->scalingRules.empty() || !dashDefinition->actions.empty() ||
		dashDefinition->behaviorType != AbilityBehaviorType::Dash)
	{
		return Fail("Basic Dash configuration or shipped ability catalog validation failed");
	}

	struct DashTestSettings
	{
		float baseDistance;
		float duration;
		float cameraZoomOutRatio;
		float cooldownReductionPerLevelRatio;
		AbilityData::Dash::DirectionPolicy directionPolicy;
	};
	float cooldownStep = 0.f;
	if (!dashDefinition->levelProgression.empty())
	{
		for (const sas::AttributeModifier& modifier :
			dashDefinition->levelProgression.front().attributeModifiers)
		{
			if (modifier.attributeId == CommonAttributeIds::Cooldown)
			{
				cooldownStep = modifier.magnitude;
				break;
			}
		}
	}
	const DashTestSettings dashSettingsValue{
		content::AbilityContentCatalog::FindNumericSetting(
			dashDefinition->abilityId,
			AbilityData::Dash::Setting::BaseDistance
		).value_or(0.f),
		dashDefinition->duration,
		content::AbilityContentCatalog::FindNumericSetting(
			dashDefinition->abilityId,
			AbilityData::Dash::Setting::CameraZoomOutRatio
		).value_or(0.f),
		dashDefinition->cooldown > 0.f ? -cooldownStep / dashDefinition->cooldown : 0.f,
		AbilityData::Dash::DirectionPolicy::MovementInputOrMouseWorld
	};
	const DashTestSettings* dashSettings = &dashSettingsValue;
	if (dashSettings->baseDistance <= 0.f ||
		!NearlyEqual(dashSettings->duration, dashDefinition->duration) ||
		dashSettings->cameraZoomOutRatio < 0.f || dashSettings->cameraZoomOutRatio > 0.5f ||
		dashSettings->cooldownReductionPerLevelRatio < 0.f ||
		dashSettings->cooldownReductionPerLevelRatio >= 0.25f ||
		dashSettings->directionPolicy != AbilityData::Dash::DirectionPolicy::MovementInputOrMouseWorld ||
		!GameplayTags::State::Ability::Dash::Active.IsValid() ||
		!GameplayTags::Event::Ability::Activated.IsValid() ||
		!GameplayTags::Event::Ability::Ended.IsValid())
	{
		return Fail("Basic Dash behavior configuration is invalid");
	}

	const sf::Vector2f preservedDashVelocity{ 120.f, -40.f };
	const sf::Vector2f resolvedDashVelocity = DashMovementMath::ResolveVelocity(
		{ 1.f, 0.f },
		dashSettings->baseDistance,
		dashSettings->duration,
		preservedDashVelocity
	);
	if (!NearlyEqual(
			resolvedDashVelocity.x,
			preservedDashVelocity.x + dashSettings->baseDistance / dashSettings->duration
		) ||
		!NearlyEqual(resolvedDashVelocity.y, preservedDashVelocity.y))
	{
		return Fail("Basic Dash did not preserve the complete pre-Dash velocity");
	}

	GameAbilityDefinition malformedDash = *dashDefinition;
	malformedDash.duration += 0.01f;
	if (ValidateAbilityDefinition(malformedDash, &dashValidationFailure))
	{
		return Fail("Mismatched Dash movement/lifetime duration was accepted");
	}
	malformedDash = *dashDefinition;
	malformedDash.behaviorType = static_cast<AbilityBehaviorType>(999);
	if (ValidateAbilityDefinition(malformedDash, &dashValidationFailure))
	{
		return Fail("Unknown ability behavior was accepted by validation");
	}
	malformedDash = *dashDefinition;
	malformedDash.abilityId = "Ability.Offense.Dash.Basic";
	dashValidationFailure.clear();
	if (ValidateAbilityDefinition(malformedDash, &dashValidationFailure))
	{
		return Fail("Ability category in the ID was allowed to diverge from the family contract");
	}
	malformedDash = *dashDefinition;
	malformedDash.abilityTags = {
		GameplayTagSchema::AbilityMovement,
		GameplayTagSchema::AbilityOffense,
		GameplayTags::Ability::Family::Dash
	};
	dashValidationFailure.clear();
	if (ValidateAbilityDefinition(malformedDash, &dashValidationFailure))
	{
		return Fail("Ability with multiple category tags was accepted");
	}

	auto ActivateBasicDash = [&](TestCombatant& owner, int level = 1)
	{
		LightYearsAbilitySystemComponent& dashAbilities = owner.GetCombatRuntime().GetAbilitySystemComponent();
		const sas::AbilityHandle handle = dashAbilities.GrantAbility(*dashDefinition);
		if (!handle.IsValid() || (level > 1 && !dashAbilities.SetAbilityLevel(handle, level)))
		{
			return false;
		}
		dashAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability3, true);
		dashAbilities.Tick(0.f);
		return owner.GetDashStartCount() == 1;
	};

	TestCombatant cooldownDashOwner;
	cooldownDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	LightYearsAbilitySystemComponent& cooldownDashAbilities = cooldownDashOwner.GetCombatRuntime().GetAbilitySystemComponent();
	const sas::AbilityHandle cooldownDashHandle = cooldownDashAbilities.GrantAbility(*dashDefinition);
	for (int level = 1; level <= 5; ++level)
	{
		const float expectedDashCooldown =
			dashDefinition->cooldown *
			(1.f - dashSettings->cooldownReductionPerLevelRatio * static_cast<float>(level - 1));
		if ((level > 1 && !cooldownDashAbilities.SetAbilityLevel(cooldownDashHandle, level)) ||
			!cooldownDashAbilities.GetAbility(cooldownDashHandle) ||
			!NearlyEqual(
				cooldownDashAbilities.GetAbility(cooldownDashHandle)->GetCooldownDuration(),
				expectedDashCooldown
			))
		{
			return Fail("Basic Dash level cooldown progression is incorrect");
		}
	}
	cooldownDashOwner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::AbilityHaste, 100.f }
	);
	const float expectedLevelFiveDashCooldown =
		dashDefinition->cooldown *
		(1.f - dashSettings->cooldownReductionPerLevelRatio * 4.f);
	if (!NearlyEqual(
		cooldownDashAbilities.GetAbility(cooldownDashHandle)->GetCooldownDuration(),
		expectedLevelFiveDashCooldown * sas::AttributeMath::GetAbilityCooldownMultiplier(100.f)
	))
	{
		return Fail("Basic Dash cooldown did not use central Ability Haste math");
	}

	TestCombatant baselineDashOwner;
	baselineDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	baselineDashOwner.SetDashMovementInput({ 1.f, 0.f });
	if (!ActivateBasicDash(baselineDashOwner))
	{
		return Fail("Basic Dash could not activate for the baseline movement profile");
	}
	const float baselineDashDistance = baselineDashOwner.GetLastDashDistance();

	TestCombatant horizontalDashOwner;
	horizontalDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	horizontalDashOwner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::MoveSpeedHorizontal, 20.f }
	);
	horizontalDashOwner.SetDashMovementInput({ 1.f, 0.f });
	if (!ActivateBasicDash(horizontalDashOwner) ||
		horizontalDashOwner.GetLastDashDistance() <= baselineDashDistance)
	{
		return Fail("A one-sided movement rating did not improve Basic Dash distance");
	}

	TestCombatant highMovementDashOwner;
	highMovementDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	highMovementDashOwner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::MoveSpeedHorizontal, 20.f }
	);
	highMovementDashOwner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
		sas::AttributeModifier{ OwnerAttributeIds::MoveSpeedVertical, 20.f }
	);
	highMovementDashOwner.SetDashMovementInput({ 1.f, 0.f });
	if (!ActivateBasicDash(highMovementDashOwner) ||
		highMovementDashOwner.GetLastDashDistance() <= horizontalDashOwner.GetLastDashDistance() ||
		highMovementDashOwner.GetLastDashDistance() > dashSettings->baseDistance * 1.5f)
	{
		return Fail("Basic Dash movement rating scaling did not remain diminishing and capped");
	}

	TestCombatant irrelevantStatDashOwner;
	irrelevantStatDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	for (const sas::AttributeModifier& modifier : List<sas::AttributeModifier>{
		{ OwnerAttributeIds::AttackPower, 500.f },
		{ OwnerAttributeIds::AttackSpeed, 500.f },
		{ OwnerAttributeIds::EnergyPower, 500.f },
		{ OwnerAttributeIds::Luck, 500.f },
		{ OwnerAttributeIds::CriticalChance, 500.f }
	})
	{
		irrelevantStatDashOwner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(modifier);
	}
	irrelevantStatDashOwner.SetDashMovementInput({ 1.f, 0.f });
	if (!ActivateBasicDash(irrelevantStatDashOwner) ||
		!NearlyEqual(irrelevantStatDashOwner.GetLastDashDistance(), baselineDashDistance))
	{
		return Fail("Non-movement attributes changed Basic Dash distance");
	}

	TestCombatant levelFiveDashOwner;
	levelFiveDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	levelFiveDashOwner.SetDashMovementInput({ 1.f, 0.f });
	if (!ActivateBasicDash(levelFiveDashOwner, 5) ||
		!NearlyEqual(levelFiveDashOwner.GetLastDashDistance(), baselineDashDistance))
	{
		return Fail("Basic Dash skill levels changed distance instead of only cooldown");
	}

	TestCombatant inputDirectionDashOwner;
	inputDirectionDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	inputDirectionDashOwner.SetDashMovementInput({ 3.f, 4.f });
	inputDirectionDashOwner.SetDashAimDirection({ -1.f, 0.f });
	if (!ActivateBasicDash(inputDirectionDashOwner) ||
		!NearlyEqual(inputDirectionDashOwner.GetLastDashDirection().x, 0.6f) ||
		!NearlyEqual(inputDirectionDashOwner.GetLastDashDirection().y, 0.8f))
	{
		return Fail("Basic Dash did not normalize the movement input direction");
	}

	TestCombatant aimFallbackDashOwner;
	aimFallbackDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	aimFallbackDashOwner.SetDashAimDirection({ 0.f, -10.f });
	if (!ActivateBasicDash(aimFallbackDashOwner) ||
		!NearlyEqual(aimFallbackDashOwner.GetLastDashDirection().x, 0.f) ||
		!NearlyEqual(aimFallbackDashOwner.GetLastDashDirection().y, -1.f))
	{
		return Fail("Basic Dash did not safely fall back to the aim direction");
	}

	TestCombatant zeroDirectionDashOwner;
	zeroDirectionDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	DashEventRecorder zeroDirectionEvents;
	zeroDirectionDashOwner.GetCombatRuntime().GetAbilitySystemComponent().onGameplayEvent.BindAction(
		&zeroDirectionEvents,
		&DashEventRecorder::Record
	);
	const sas::AbilityHandle zeroDirectionHandle = zeroDirectionDashOwner.GetAbilitySystemComponent().GrantAbility(*dashDefinition);
	zeroDirectionDashOwner.GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability3, true);
	zeroDirectionDashOwner.GetCombatRuntime().GetAbilitySystemComponent().Tick(0.f);
	if (!zeroDirectionHandle.IsValid() || zeroDirectionDashOwner.GetDashStartCount() != 0 ||
		zeroDirectionDashOwner.GetAbilitySystemComponent().GetOwnedTags().HasTag(GameplayTags::State::Ability::Dash::Active) ||
		!zeroDirectionEvents.events.empty())
	{
		return Fail("Basic Dash accepted an invalid zero direction");
	}

	TestCombatant lifecycleDashOwner;
	lifecycleDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	lifecycleDashOwner.SetDashMovementInput({ 1.f, 0.f });
	DashEventRecorder lifecycleEvents;
	lifecycleDashOwner.GetCombatRuntime().GetAbilitySystemComponent().onGameplayEvent.BindAction(
		&lifecycleEvents,
		&DashEventRecorder::Record
	);
	if (!ActivateBasicDash(lifecycleDashOwner) ||
		!lifecycleDashOwner.GetAbilitySystemComponent().GetOwnedTags().HasTag(GameplayTags::State::Ability::Dash::Active) ||
		lifecycleEvents.events.size() != 1 ||
		lifecycleEvents.events.front() != GameplayTags::Event::Ability::Activated)
	{
		return Fail("Basic Dash start lifecycle did not publish its state and event");
	}
	lifecycleDashOwner.GetCombatRuntime().GetAbilitySystemComponent().Tick(dashSettings->duration);
	if (lifecycleDashOwner.IsDashActive() || lifecycleDashOwner.GetDashEndCount() != 1 ||
		lifecycleDashOwner.GetAbilitySystemComponent().GetOwnedTags().HasTag(GameplayTags::State::Ability::Dash::Active) ||
		lifecycleEvents.events.size() != 2 ||
		lifecycleEvents.events.back() != GameplayTags::Event::Ability::Ended)
	{
		return Fail("Basic Dash duration cleanup did not publish its end lifecycle");
	}

	TestCombatant clearedDashOwner;
	clearedDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	clearedDashOwner.SetDashMovementInput({ 1.f, 0.f });
	DashEventRecorder clearedEvents;
	clearedDashOwner.GetCombatRuntime().GetAbilitySystemComponent().onGameplayEvent.BindAction(
		&clearedEvents,
		&DashEventRecorder::Record
	);
	if (!ActivateBasicDash(clearedDashOwner))
	{
		return Fail("Basic Dash could not activate before ability-system cleanup");
	}
	clearedDashOwner.GetCombatRuntime().GetAbilitySystemComponent().Clear();
	if (clearedDashOwner.IsDashActive() ||
		clearedDashOwner.GetAbilitySystemComponent().GetOwnedTags().HasTag(GameplayTags::State::Ability::Dash::Active) ||
		clearedEvents.events.size() != 2 ||
		clearedEvents.events.back() != GameplayTags::Event::Ability::Ended)
	{
		return Fail("Basic Dash did not clean up its state on ability-system clear");
	}

	// Echo Protocol must consume the newest normal ability record, replay the
	// source through the common invocation runtime, and keep the replay out of
	// the history so Echo cannot recursively record itself.
	TestCombatant echoOwner;
	echoOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	echoOwner.SetDashMovementInput({ 1.f, 0.f });
	LightYearsAbilitySystemComponent& echoAbilities =
		echoOwner.GetCombatRuntime().GetAbilitySystemComponent();
	GameAbilityDefinition echoSourceDash = *dashDefinition;
	// This isolated replay test places Echo on F/Ability3 explicitly, while the
	// default player loadout reserves F for Scorch Drive.
	echoSourceDash.slot = sas::AbilitySlot::Ability4;
	const sas::AbilityHandle recordedDashHandle = echoAbilities.GrantAbility(echoSourceDash);
	const sas::AbilityHandle echoHandle = echoAbilities.GrantAbility(*echoProtocolDefinition);
	if (!recordedDashHandle.IsValid() || !echoHandle.IsValid())
	{
		return Fail("Echo Protocol test abilities could not be granted");
	}
	// The source copy is intentionally on Ability4; Echo itself is manually
	// granted on Ability3 for this focused replay test.
	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability4, true);
	echoAbilities.Tick(0.f);
	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability4, false);
	echoAbilities.Tick(dashDefinition->duration);
	if (echoAbilities.GetAbilityUseHistory().GetRecords().size() != 1 ||
		echoAbilities.GetAbilityUseHistory().GetRecords().front().abilityId !=
			sas::ContentId{ AbilityData::Dash::AbilityId::Basic } ||
		echoOwner.GetDashStartCount() != 1)
	{
		return Fail("Normal ability activation was not recorded for Echo Protocol");
	}

	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability3, true);
	echoAbilities.Tick(0.f);
	if (echoOwner.GetDashStartCount() != 2 ||
		echoAbilities.GetActiveAbilityInvocationCount() != 1 ||
		echoAbilities.GetAbilityUseHistory().GetRecords().size() != 1 ||
		!echoAbilities.GetAbilityUseHistory().GetRecords().front().consumed)
	{
		return Fail("Echo Protocol did not consume the newest recorded Dash");
	}
	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability3, false);
	echoAbilities.Tick(dashDefinition->duration);
	if (echoAbilities.GetActiveAbilityInvocationCount() != 0)
	{
		return Fail("Echo Protocol invocation runtime did not clean up the replayed ability");
	}
	if (echoAbilities.GetAbilityUseHistory().GetRecords().size() != 1 ||
		!echoAbilities.GetAbilityUseHistory().GetRecords().front().consumed)
	{
		return Fail("Echo Protocol did not preserve the consumed history snapshot");
	}

	// A second Echo with no new ability must not fall back to an older history
	// entry. A later normal activation must become the new one-record Echo
	// candidate while the older snapshots remain available globally.
	if (GameAbility* echoAbility = echoAbilities.GetAbility(echoHandle))
	{
		echoAbility->ReduceCooldownRemaining(0.1f);
	}
	echoAbilities.Tick(20.f);
	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability3, true);
	echoAbilities.Tick(0.f);
	if (echoOwner.GetDashStartCount() != 2 ||
		echoAbilities.GetActiveAbilityInvocationCount() != 0)
	{
		return Fail("Echo Protocol incorrectly fell back to an older ability after consuming the latest one");
	}
	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability3, false);
	// Commit the released control state before pressing the source again. This
	// keeps the test focused on history advancement rather than input edges.
	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability4, false);
	echoAbilities.Tick(0.f);
	if (GameAbility* recordedDash = echoAbilities.GetAbility(recordedDashHandle))
	{
		// Leave a positive remainder so the normal cooldown tick restores the
		// spent charge as well as making the source ready again.
		recordedDash->ReduceCooldownRemaining(0.1f);
	}
	echoAbilities.Tick(20.f);
	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability4, true);
	echoAbilities.Tick(0.f);
	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability4, false);
	echoAbilities.Tick(dashDefinition->duration);
	if (echoAbilities.GetAbilityUseHistory().GetRecords().size() != 2)
	{
		return Fail("Echo Protocol did not record the newer normal activation");
	}
	if (echoAbilities.GetAbilityUseHistory().GetRecords().front().consumed)
	{
		return Fail("Echo Protocol marked the newer normal activation consumed too early");
	}
	if (GameAbility* echoAbility = echoAbilities.GetAbility(echoHandle))
	{
		echoAbility->ReduceCooldownRemaining(0.1f);
	}
	echoAbilities.Tick(20.f);
	echoAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability3, true);
	echoAbilities.Tick(0.f);
	if (echoOwner.GetDashStartCount() != 4 ||
		echoAbilities.GetActiveAbilityInvocationCount() != 1)
	{
		return Fail("Echo Protocol did not replay the newly recorded ability");
	}

	// Echo's final power multiplier must affect declared output channels only.
	// This regression matrix covers the three projectile families that exposed
	// the bug in-game: Gravity keeps its intentional MaxHealth-scaled radius and
	// duration, while delivery speed/range/lifetime/collision remain fixed;
	// Relay Prism keeps its fixed geometry; Rocket keeps its fixed delivery data.
	{
		TestCombatant echoResolutionOwner;
		echoResolutionOwner.GetCombatRuntime().InitializeOwnerAttributes(1000.f);
		LightYearsAbilitySystemComponent& echoResolutionAbilities =
			echoResolutionOwner.GetAbilitySystemComponent();
		echoResolutionAbilities.GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ OwnerAttributeIds::Armor, 1.f }
		);
		echoResolutionAbilities.GetAttributes().ApplyBaseModifier(
			sas::AttributeModifier{ OwnerAttributeIds::AttackSpeed, 2.f }
		);
		const float maxHealth = echoResolutionAbilities.GetAttributes().GetCurrentValue(
			OwnerAttributeIds::MaxHealth,
			1000.f
		);

		GameAbilityDefinition gravityReplayDefinition;
		gravityReplayDefinition.attributeOutputMultiplier = 0.60f;
		gravityReplayDefinition.scalingRules = {
			sas::AttributeScalingRule{
				CommonAttributeIds::Radius,
				OwnerAttributeIds::MaxHealth,
				sas::AttributeModifierOperation::Add,
				0.20f
			},
			sas::AttributeScalingRule{
				CommonAttributeIds::Duration,
				OwnerAttributeIds::MaxHealth,
				sas::AttributeModifierOperation::Add,
				0.0025f
			}
		};
		AbilityExecutionContext gravityReplayContext{
			&echoResolutionAbilities,
			&gravityReplayDefinition,
			nullptr,
			nullptr
		};
		const sas::GameplayAttributeList gravityReplayValues =
			AbilityActionAttributeResolver::ResolveAttributes(
				gravityReplayContext,
				nullptr,
				{
					sas::GameplayAttribute{ CommonAttributeIds::Radius, 320.f, 0.f },
					sas::GameplayAttribute{ CommonAttributeIds::Duration, 2.5f, 0.f },
					sas::GameplayAttribute{
						AbilityData::GravityAnomaly::Actor::Projectile::ProjectileSpeed,
						2000.f,
						0.f
					},
					sas::GameplayAttribute{ CommonAttributeIds::Range, 900.f, 0.f },
					sas::GameplayAttribute{ CollisionAttributeIds::Radius, 8.f, 0.f }
				}
			);
		const float expectedGravityRadius = (320.f + maxHealth * 0.20f) * 0.60f;
		const float expectedGravityDuration = (2.5f + maxHealth * 0.0025f) * 0.60f;
		if (!NearlyEqual(
				sas::FindAttributeValue(gravityReplayValues, CommonAttributeIds::Radius),
				expectedGravityRadius
			) || !NearlyEqual(
				sas::FindAttributeValue(gravityReplayValues, CommonAttributeIds::Duration),
				expectedGravityDuration
			) || !NearlyEqual(
				sas::FindAttributeValue(
					gravityReplayValues,
					AbilityData::GravityAnomaly::Actor::Projectile::ProjectileSpeed
				),
				2000.f
			) || !NearlyEqual(
				sas::FindAttributeValue(gravityReplayValues, CommonAttributeIds::Range),
				900.f
			) || !NearlyEqual(
				sas::FindAttributeValue(gravityReplayValues, CollisionAttributeIds::Radius),
				8.f
			))
		{
			return Fail("Echo Gravity output scaling changed fixed delivery data or duration coefficients");
		}

		GameAbilityDefinition prismReplayDefinition;
		prismReplayDefinition.attributeOutputMultiplier = 0.60f;
		AbilityExecutionContext prismReplayContext{
			&echoResolutionAbilities,
			&prismReplayDefinition,
			nullptr,
			nullptr
		};
		const sas::GameplayAttributeList prismReplayValues =
			AbilityActionAttributeResolver::ResolveAttributes(
				prismReplayContext,
				nullptr,
				{
					sas::GameplayAttribute{ CommonAttributeIds::Radius, 100.f, 0.f },
					sas::GameplayAttribute{ CommonAttributeIds::Range, 600.f, 0.f },
					sas::GameplayAttribute{
						AbilityData::RelayPrism::Actor::Relay::ProjectileSpeed,
						450.f,
						0.f
					},
					sas::GameplayAttribute{ CommonAttributeIds::Duration, 4.f, 0.f }
				}
			);
		if (!NearlyEqual(sas::FindAttributeValue(prismReplayValues, CommonAttributeIds::Radius), 100.f) ||
			!NearlyEqual(sas::FindAttributeValue(prismReplayValues, CommonAttributeIds::Range), 600.f) ||
			!NearlyEqual(
				sas::FindAttributeValue(
					prismReplayValues,
					AbilityData::RelayPrism::Actor::Relay::ProjectileSpeed
				),
				450.f
			) || !NearlyEqual(sas::FindAttributeValue(prismReplayValues, CommonAttributeIds::Duration), 4.f))
		{
			return Fail("Echo Relay Prism changed fixed geometry or delivery values");
		}

		GameAbilityDefinition rocketReplayDefinition;
		rocketReplayDefinition.attributeOutputMultiplier = 0.60f;
		rocketReplayDefinition.scalingRules = {
			sas::AttributeScalingRule{
				CommonAttributeIds::Damage,
				OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				1.30f
			}
		};
		AbilityExecutionContext rocketReplayContext{
			&echoResolutionAbilities,
			&rocketReplayDefinition,
			nullptr,
			nullptr
		};
		const sas::GameplayAttributeList rocketReplayValues =
			AbilityActionAttributeResolver::ResolveAttributes(
				rocketReplayContext,
				nullptr,
				{
					sas::GameplayAttribute{ CommonAttributeIds::Damage, 55.f, 0.f },
					sas::GameplayAttribute{
						AbilityData::Rocket::Actor::Projectile::ProjectileSpeed,
						1000.f,
						0.f
					},
					sas::GameplayAttribute{ CommonAttributeIds::Range, 1100.f, 0.f },
					sas::GameplayAttribute{ CommonAttributeIds::Duration, 1.35f, 0.f },
					sas::GameplayAttribute{ CommonAttributeIds::Radius, 55.f, 0.f },
					sas::GameplayAttribute{ CollisionAttributeIds::Radius, 8.f, 0.f }
				}
			);
		if (!NearlyEqual(sas::FindAttributeValue(rocketReplayValues, CommonAttributeIds::Damage), 33.f) ||
			!NearlyEqual(
				sas::FindAttributeValue(
					rocketReplayValues,
					AbilityData::Rocket::Actor::Projectile::ProjectileSpeed
				),
				1000.f
			) || !NearlyEqual(sas::FindAttributeValue(rocketReplayValues, CommonAttributeIds::Range), 1100.f) ||
			!NearlyEqual(sas::FindAttributeValue(rocketReplayValues, CommonAttributeIds::Duration), 1.35f) ||
			!NearlyEqual(sas::FindAttributeValue(rocketReplayValues, CommonAttributeIds::Radius), 55.f) ||
			!NearlyEqual(sas::FindAttributeValue(rocketReplayValues, CollisionAttributeIds::Radius), 8.f))
		{
			return Fail("Echo Rocket changed fixed delivery or collision values");
		}

		GameAbilityDefinition overdriveReplayDefinition;
		overdriveReplayDefinition.attributeOutputMultiplier = 0.60f;
		overdriveReplayDefinition.scalingRules = {
			sas::AttributeScalingRule{
				CommonAttributeIds::ProjectileCount,
				OwnerAttributeIds::AttackSpeed,
				sas::AttributeModifierOperation::Multiply,
				0.20f
			},
			sas::AttributeScalingRule{
				CommonAttributeIds::Damage,
				OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				1.30f
			}
		};
		AbilityExecutionContext overdriveReplayContext{
			&echoResolutionAbilities,
			&overdriveReplayDefinition,
			nullptr,
			nullptr
		};
		const sas::GameplayAttributeList overdriveReplayValues =
			AbilityActionAttributeResolver::ResolveAttributes(
				overdriveReplayContext,
				nullptr,
				{
					sas::GameplayAttribute{ CommonAttributeIds::ProjectileCount, 8.f, 0.f },
					sas::GameplayAttribute{
						AbilityData::OverdriveCore::Actor::Projectile::ProjectileSpeed,
						1400.f,
						0.f
					},
					sas::GameplayAttribute{ CommonAttributeIds::Range, 1100.f, 0.f },
					sas::GameplayAttribute{ CommonAttributeIds::Duration, 1.f, 0.f },
					sas::GameplayAttribute{ CommonAttributeIds::Radius, 28.f, 0.f }
				}
			);
		if (!NearlyEqual(
				sas::FindAttributeValue(overdriveReplayValues, CommonAttributeIds::ProjectileCount),
				8.f * (1.f + 2.f * 0.20f) * 0.60f
			) || !NearlyEqual(
				sas::FindAttributeValue(
					overdriveReplayValues,
					AbilityData::OverdriveCore::Actor::Projectile::ProjectileSpeed
				),
				1400.f
			) || !NearlyEqual(sas::FindAttributeValue(overdriveReplayValues, CommonAttributeIds::Range), 1100.f) ||
			!NearlyEqual(sas::FindAttributeValue(overdriveReplayValues, CommonAttributeIds::Duration), 1.f) ||
			!NearlyEqual(sas::FindAttributeValue(overdriveReplayValues, CommonAttributeIds::Radius), 28.f))
		{
			return Fail("Echo Overdrive changed Multiply scaling semantics or fixed delivery data");
		}

		GameAbilityDefinition shieldReplayDefinition;
		shieldReplayDefinition.attributeOutputMultiplier = 0.60f;
		shieldReplayDefinition.scalingRules = {
			sas::AttributeScalingRule{
				sas::AttributeId{ "Effect.BarrierCapacity" },
				OwnerAttributeIds::MaxHealth,
				sas::AttributeModifierOperation::Add,
				0.20f
			},
			sas::AttributeScalingRule{
				sas::AttributeId{ "Effect.BarrierCapacity" },
				OwnerAttributeIds::Armor,
				sas::AttributeModifierOperation::Add,
				50.f
			}
		};
		AbilityExecutionContext shieldReplayContext{
			&echoResolutionAbilities,
			&shieldReplayDefinition,
			nullptr,
			nullptr
		};
		const sas::GameplayAttributeList shieldReplayValues =
			AbilityActionAttributeResolver::ResolveAttributes(
				shieldReplayContext,
				nullptr,
				{ sas::GameplayAttribute{
					sas::AttributeId{ "Effect.BarrierCapacity" },
					30.f,
					0.f
				} }
			);
		if (!NearlyEqual(
			sas::FindAttributeValue(
				shieldReplayValues,
				sas::AttributeId{ "Effect.BarrierCapacity" }
			),
			(maxHealth * 0.20f + 50.f + 30.f) * 0.60f
		))
		{
			return Fail("Echo Shield dropped an unsupported Armor scaling channel");
		}
	}

	// Run the real Echo -> Gravity invocation as well as the resolver matrix.
	// This protects the hand-off from AbilityUseHistory into the invocation
	// definition, where the target-specific Duration coefficient used to be
	// lost before the resolver was reached.
	{
		World echoGravityWorld{ nullptr };
		TestCombatant echoGravityOwner{ &echoGravityWorld, 1000.f };
		echoGravityOwner.GetCombatRuntime().InitializeOwnerAttributes(1000.f);
		echoGravityOwner.SetActorRotation(90.f);
		echoGravityOwner.SetCollisionLayer(CollisionLayer::Player);
		LightYearsAbilitySystemComponent& echoGravityAbilities =
			echoGravityOwner.GetAbilitySystemComponent();
		GameAbilityDefinition echoGravitySourceDefinition = *gravityDefinition;
		echoGravitySourceDefinition.slot = sas::AbilitySlot::Ability4;
		if (!echoGravityAbilities.GrantAbility(echoGravitySourceDefinition).IsValid() ||
			!echoGravityAbilities.GrantAbility(*echoProtocolDefinition).IsValid())
		{
			return Fail("Echo Gravity regression abilities could not be granted");
		}
		echoGravityAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability4, true);
		echoGravityAbilities.Tick(0.f);
		echoGravityAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability4, false);
		echoGravityWorld.TickInternal(0.f);
		if (echoGravityAbilities.GetAbilityUseHistory().GetRecords().size() != 1)
		{
			return Fail("Echo Gravity source activation was not recorded");
		}
		echoGravityAbilities.SetAbilitySlotInput(sas::AbilitySlot::Ability3, true);
		echoGravityAbilities.Tick(0.f);
		echoGravityWorld.TickInternal(0.f);
		const List<weak_ptr<GravityAnomalyProjectileActor>> echoGravityProjectiles =
			echoGravityWorld.GetActorsByType<GravityAnomalyProjectileActor>();
		if (echoGravityProjectiles.size() != 2)
		{
			return Fail("Echo Gravity did not create exactly one replay projectile");
		}
		bool foundBaseGravity = false;
		bool foundEchoGravity = false;
		for (const weak_ptr<GravityAnomalyProjectileActor>& projectileWeak : echoGravityProjectiles)
		{
			const shared_ptr<GravityAnomalyProjectileActor> projectile = projectileWeak.lock();
			if (!projectile)
			{
				continue;
			}
			foundBaseGravity = foundBaseGravity ||
				NearlyEqual(projectile->GetProjectileSpeed(), 2000.f) &&
				NearlyEqual(projectile->GetResolvedFieldDuration(), 5.f);
			foundEchoGravity = foundEchoGravity ||
				NearlyEqual(projectile->GetProjectileSpeed(), 2000.f) &&
				NearlyEqual(projectile->GetResolvedFieldDuration(), 3.f);
		}
		if (!foundBaseGravity || !foundEchoGravity)
		{
			return Fail("Echo Gravity replay changed projectile speed or inflated field duration");
		}
	}

	TestCombatant stunnedDashOwner;
	stunnedDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	stunnedDashOwner.SetDashMovementInput({ 1.f, 0.f });
	if (!ActivateBasicDash(stunnedDashOwner))
	{
		return Fail("Stun interruption test could not activate Basic Dash");
	}
	sas::GameplayEffectSpec stunSpec = sas::MakeGameplayEffectSpec(
		*nullPulseStunDefinition
	);
	stunSpec.duration = 1.f;
	if (!stunnedDashOwner.GetAbilitySystemComponent().ApplyGameplayEffect(stunSpec).IsValid())
	{
		return Fail("Stun interruption test could not apply the stun effect");
	}
	stunnedDashOwner.GetCombatRuntime().GetAbilitySystemComponent().Tick(0.f);
	if (stunnedDashOwner.IsDashActive() ||
		stunnedDashOwner.GetDashEndCount() != 1 ||
		stunnedDashOwner.GetAbilitySystemComponent().GetAbilityById(
			dashDefinition->abilityId
		)->IsActive())
	{
		return Fail("Stun did not interrupt an active ability");
	}
	TestCombatant stunDamageTarget;
	ApplyCombatDamage(stunDamageTarget, 10.f, &stunnedDashOwner);
	if (!NearlyEqual(stunDamageTarget.GetHealth(), 90.f))
	{
		return Fail("Stun incorrectly suppressed damage from the stunned owner");
	}

	TestCombatant blockedDashOwner;
	blockedDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	blockedDashOwner.SetDashMovementInput({ 1.f, 0.f });
	if (!blockedDashOwner.GetAbilitySystemComponent().ApplyGameplayEffect(stunSpec).IsValid() ||
		ActivateBasicDash(blockedDashOwner))
	{
		return Fail("Stun did not block a new ability activation");
	}

	// -------------------------------------------------------------------------
	// Inferno Spray Detailed System Tests
	// -------------------------------------------------------------------------
	TestCombatant infernoOwner;
	infernoOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	const sas::AbilityHandle infernoHandle = infernoOwner.GetAbilitySystemComponent().GrantAbility(
		*AbilityData::FindShippedAbilityDefinition(AbilityData::InfernoSpray::AbilityId::Basic)
	);
	if (!infernoHandle.IsValid())
	{
		return Fail("Inferno Spray ability could not be granted");
	}

	// 1. Activation & Movement Slow Attribute Test
	infernoOwner.GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability2, true);
	infernoOwner.GetCombatRuntime().GetAbilitySystemComponent().Tick(0.1f);
	if (!infernoOwner.GetAbilitySystemComponent().GetOwnedTags().HasTag(GameplayTags::State::Ability::InfernoSpray::Active) ||
		!infernoOwner.GetAbilitySystemComponent().GetOwnedTags().HasTag(MovementEffectSchema::SlowGrantedTag))
	{
		return Fail("Inferno Spray activation did not apply state tag and movement slow tag");
	}

	const float slowAttributeValue = infernoOwner.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
		ly::OwnerAttributeIds::MovementSlow
	);
	if (!NearlyEqual(slowAttributeValue, 0.35f))
	{
		return Fail("Inferno Spray did not apply 35% MovementSlow attribute penalty to owner");
	}

	// 2. Early cancel rejection (< 1.0s)
	infernoOwner.GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability2, true);
	infernoOwner.GetCombatRuntime().GetAbilitySystemComponent().Tick(0.4f);
	if (!infernoOwner.GetAbilitySystemComponent().GetOwnedTags().HasTag(GameplayTags::State::Ability::InfernoSpray::Active))
	{
		return Fail("Inferno Spray was cancelled early before 1.0 second elapsed");
	}

	// 3. Early cancel success (>= 1.0s) & Slow Cleanup
	infernoOwner.GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability2, false);
	infernoOwner.GetCombatRuntime().GetAbilitySystemComponent().Tick(0.6f); // Total active time = 1.1s
	infernoOwner.GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability2, true);
	infernoOwner.GetCombatRuntime().GetAbilitySystemComponent().Tick(0.1f);
	infernoOwner.GetCombatRuntime().GetAbilitySystemComponent().Tick(0.1f);
	if (infernoOwner.GetAbilitySystemComponent().GetOwnedTags().HasTag(GameplayTags::State::Ability::InfernoSpray::Active) ||
		infernoOwner.GetAbilitySystemComponent().GetOwnedTags().HasTag(MovementEffectSchema::SlowGrantedTag))
	{
		return Fail("Inferno Spray did not cancel cleanly after 1.0 second");
	}

	const float cleanedSlowValue = infernoOwner.GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
		ly::OwnerAttributeIds::MovementSlow
	);
	if (!NearlyEqual(cleanedSlowValue, 0.f))
	{
		return Fail("Inferno Spray did not remove MovementSlow penalty after ending");
	}

	// 4. World Combat Simulation Test (Damage & Cone Target Filtering & Thermal Ignite Stacks)
	{
		ly::World world{ nullptr };
		const ly::shared_ptr<TestCombatant> shooter = world.SpawnActor<TestCombatant>(100.f).lock();
		const ly::shared_ptr<TestCombatant> targetInCone = world.SpawnActor<TestCombatant>(100.f).lock();
		const ly::shared_ptr<TestCombatant> targetOutsideAngle = world.SpawnActor<TestCombatant>(100.f).lock();
		const ly::shared_ptr<TestCombatant> targetOutsideRange = world.SpawnActor<TestCombatant>(100.f).lock();

		if (!shooter || !targetInCone || !targetOutsideAngle || !targetOutsideRange)
		{
			return Fail("Could not spawn combat simulation actors for Inferno Spray test");
		}

		shooter->SetActorLocation({ 0.f, 0.f });
		shooter->SetActorRotation(0.f); // Forward vector is { 0.f, -1.f }
		shooter->SetCollisionLayer(CollisionLayer::Player);

		targetInCone->SetActorLocation({ 0.f, -200.f }); // Directly in front (dist=200 < 520, angle=0)
		targetInCone->SetCollisionLayer(CollisionLayer::Enemy);
		targetInCone->SetCollisionMask(CollisionLayer::PlayerBullet);

		targetOutsideAngle->SetActorLocation({ 200.f, 0.f }); // 90 degrees to the right (angle=90 > 18)
		targetOutsideAngle->SetCollisionLayer(CollisionLayer::Enemy);
		targetOutsideAngle->SetCollisionMask(CollisionLayer::PlayerBullet);

		targetOutsideRange->SetActorLocation({ 0.f, -600.f }); // Directly in front but out of range (dist=600 > 520)
		targetOutsideRange->SetCollisionLayer(CollisionLayer::Enemy);
		targetOutsideRange->SetCollisionMask(CollisionLayer::PlayerBullet);

		world.TickInternal(0.f);

		const sas::AbilityHandle sprayHandle = shooter->GetAbilitySystemComponent().GrantAbility(
			*AbilityData::FindShippedAbilityDefinition(AbilityData::InfernoSpray::AbilityId::Basic)
		);
		if (!sprayHandle.IsValid())
		{
			return Fail("Could not grant Inferno Spray to simulation shooter");
		}

		shooter->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability2, true);

		// Helper lambda to advance simulation
		auto advanceSimulation = [&](float totalTime)
		{
			const float step = 0.05f;
			float elapsed = 0.f;
			while (elapsed < totalTime)
			{
				shooter->GetCombatRuntime().GetAbilitySystemComponent().Tick(step);
				world.TickInternal(step);
				elapsed += step;
			}
		};

		// Step 1: 0.3s elapsed -> Triggers 1st combat tick at 0.25s
		advanceSimulation(0.3f);

		const float firstTickDamage = 24.f * 0.25f; // 6.0 damage
		if (!NearlyEqual(targetInCone->GetHealth(), 100.f - firstTickDamage))
		{
			return Fail("Inferno Spray did not deal correct 1st tick damage to target inside cone");
		}
		if (!NearlyEqual(targetOutsideAngle->GetHealth(), 100.f) ||
			!NearlyEqual(targetOutsideRange->GetHealth(), 100.f))
		{
			return Fail("Inferno Spray incorrectly damaged targets outside angle or range");
		}

		// Step 2: Advance by another 0.8s (total elapsed time 1.1s -> 4 combat ticks at 0.25s, 0.50s, 0.75s, 1.00s)
		advanceSimulation(0.8f);

		const float expectedDirectDamage = 4.f * (24.f * 0.25f); // 24.0 direct damage from 4 ticks
		if (targetInCone->GetHealth() > (100.f - expectedDirectDamage + 0.1f))
		{
			return Fail("Inferno Spray 4-tick continuous hit damage was not applied");
		}

		// Verify 4th hit triggered full 4-stack Ignite Thermal status
		const bool hasIgniteTag = targetInCone->GetAbilitySystemComponent().GetOwnedTags().HasTag(
			ly::DamageStatusSchema::Ignite
		);
		if (!hasIgniteTag)
		{
			return Fail("Inferno Spray hit did not apply Ignite Thermal status");
		}

		const sas::ActiveGameplayEffect* activeIgnite = targetInCone->GetAbilitySystemComponent().FindGameplayEffectById(
			"Effect.Status.Damage.Ignite"
		);
		if (!activeIgnite || activeIgnite->stackCount != 4)
		{
			return Fail("Inferno Spray 4 hits did not stack Ignite to full 4/4 stack payoff");
		}

		const float healthBeforeBurn = targetInCone->GetHealth();
		// Advance simulation by 1.0s to verify Ignite tick burn damage (4 DPS * 1s = 4.0 damage)
		advanceSimulation(1.0f);
		const float burnDamage = healthBeforeBurn - targetInCone->GetHealth();
		if (burnDamage <= 0.5f)
		{
			return Fail("Ignite 4/4 full stack burn tick damage was not dealt over time");
		}
	}

	{
		World relayProjectileWorld{ nullptr };
		const shared_ptr<TestCombatant> relayProjectileOwner =
			relayProjectileWorld.SpawnActor<TestCombatant>(1000.f).lock();
		if (!relayProjectileOwner)
		{
			return Fail("Relay Prism projectile owner could not spawn");
		}
		relayProjectileOwner->SetActorLocation({ 0.f, 0.f });
		RelayPrismPresentationProfile relayProjectilePresentation;
		const shared_ptr<RelayPrismActor> relayProjectile =
			relayProjectileWorld.SpawnActor<RelayPrismActor>(
				relayProjectileOwner.get(),
				relayProjectilePresentation,
				sf::Vector2f{ 1200.f, 0.f }
			).lock();
		if (!relayProjectile)
		{
			return Fail("Relay Prism projectile could not spawn");
		}
		relayProjectile->SetActorLocation({ 0.f, 0.f });
		relayProjectile->SetLifeTime(4.f);
		relayProjectile->ConfigureFromAttributes({
			sas::GameplayAttribute{ CommonAttributeIds::Radius, 100.f, 1.f },
			sas::GameplayAttribute{ CommonAttributeIds::Range, 600.f, 1.f },
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Actor::Relay::ProjectileSpeed,
				450.f,
				1.f
			}
		});
		relayProjectileWorld.TickInternal(0.f);
		if (!relayProjectile->IsProjectileActor() ||
			!NearlyEqual(relayProjectile->GetMaximumRange(), 600.f) ||
			!NearlyEqual(relayProjectile->GetProjectileSpeed(), 450.f) ||
			relayProjectile->IsCaptureOpen())
		{
			return Fail("Relay Prism opened its capture volume before reaching the destination");
		}
		relayProjectileWorld.TickInternal(1.f);
		if (relayProjectile->HasReachedTarget() ||
			!NearlyEqual(relayProjectile->GetActorLocation().x, 450.f) ||
			relayProjectile->IsCaptureOpen())
		{
			return Fail("Relay Prism projectile did not remain closed during flight");
		}
		relayProjectileWorld.TickInternal(0.5f);
		if (!relayProjectile->HasReachedTarget() ||
			!NearlyEqual(relayProjectile->GetActorLocation().x, 600.f) ||
			relayProjectile->GetTravelDistance() > relayProjectile->GetMaximumRange() + 0.001f ||
			!relayProjectile->IsCaptureOpen())
		{
			return Fail("Relay Prism did not open its capture volume at the destination");
		}
	}

	{
		// Relay Prism must consume the source exactly once and preserve the
		// projectile state that is meaningful after conversion. This exercises
		// the shared ProjectileRelayParticipant path with a real primary projectile.
		World relayWorld{ nullptr };
		const shared_ptr<TestCombatant> relayOwner =
			relayWorld.SpawnActor<TestCombatant>(1000.f).lock();
		if (!relayOwner)
		{
			return Fail("Relay Prism test owner could not spawn");
		}
		relayOwner->GetCombatRuntime().InitializeOwnerAttributes(1000.f);
		relayOwner->SetCollisionLayer(CollisionLayer::Player);
		relayOwner->SetCollisionMask(CollisionLayer::RelayProjectile);
		relayOwner->SetActorLocation({ 500.f, 0.f });

		RelayPrismPresentationProfile relayPresentation;
		const shared_ptr<RelayPrismActor> relay =
			relayWorld.SpawnActor<RelayPrismActor>(relayOwner.get(), relayPresentation).lock();
		if (!relay)
		{
			return Fail("Relay Prism capture volume could not spawn");
		}
		relay->SetActorLocation({ 0.f, 0.f });
		relay->ConfigureFromAttributes({
			sas::GameplayAttribute{ CommonAttributeIds::Radius, 150.f, 1.f }
		});
		relay->ConfigureFromAbilityValues({
			sas::GameplayAttribute{ CommonAttributeIds::ProjectileCount, 2.f, 1.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::DamageTransferRatio, 0.5f, 0.f, 1.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::AttackPowerCoefficient, 0.f, 0.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::MinimumScatterAngle, 60.f, 0.f, 360.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::MaximumScatterAngle, 60.f, 0.f, 360.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::MaximumBonusProjectileCount, 0.f, 0.f }
		});

		const sas::GameplayAttributeList relayProjectileValues{
			sas::GameplayAttribute{ CommonAttributeIds::Damage, 100.f, 0.f },
			sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 200.f, 0.f },
			sas::GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 5.f, 0.f },
			sas::GameplayAttribute{ CommonAttributeIds::Range, 5000.f, 1.f },
			sas::GameplayAttribute{ CollisionAttributeIds::Radius, 8.f, 0.f }
		};
		const shared_ptr<PrimaryWeaponProjectileActor> relaySource =
			relayWorld.SpawnActor<PrimaryWeaponProjectileActor>(
				relayOwner.get(),
				WeaponPresentationDefinition{},
				relayProjectileValues
			).lock();
		if (!relaySource)
		{
			return Fail("Relay Prism source projectile could not spawn");
		}
		relaySource->SetActorLocation({ 0.f, 0.f });
		relaySource->SetLaunchVelocity({ 200.f, 0.f });
		relaySource->SetDamageTags({ DamageTypeSchema::Kinetic });
		relaySource->SetSourceAbility(
			sas::ContentId{ "Ability.Test.RelaySource" },
			{ GameplayTags::Ability::Offense }
		);
		relaySource->AbilityWorldActor::Tick(1.25f);

		if (!relay->TryCaptureProjectile(*relaySource) || !relaySource->GetIsPendingDestroy())
		{
			return Fail("Relay Prism did not consume an eligible source projectile");
		}
		// Flush the newly spawned clone actors through their normal lifecycle.
		// The relay sees them during this tick but rejects its own lineage.
		relayWorld.TickInternal(0.f);

		List<shared_ptr<PrimaryWeaponProjectileActor>> relayClones;
		for (const weak_ptr<PrimaryWeaponProjectileActor>& projectileWeak :
			relayWorld.GetActorsByType<PrimaryWeaponProjectileActor>())
		{
			if (const shared_ptr<PrimaryWeaponProjectileActor> projectile = projectileWeak.lock();
				projectile && projectile.get() != relaySource.get() &&
				!projectile->GetIsPendingDestroy())
			{
				relayClones.push_back(projectile);
			}
		}
		if (relayClones.size() != 2)
		{
			return Fail("Relay Prism did not create its configured clone count");
		}

		for (const shared_ptr<PrimaryWeaponProjectileActor>& clone : relayClones)
		{
			if (!NearlyEqual(clone->GetDamage(), 50.f) ||
				!NearlyEqual(clone->GetLifeTime(), 3.75f) ||
				clone->GetProjectileRelayLineage().generation != 1 ||
				!clone->GetProjectileRelayLineage().HasVisited(relay->GetCaptureVolumeId()) ||
				clone->GetCollisionLayer() != CollisionLayer::RelayProjectile ||
				clone->GetCollisionMask() != CollisionLayer::AllRelayTargets ||
				clone->GetDamageTags().size() != 1 ||
				clone->GetDamageTags().front() != DamageTypeSchema::Kinetic)
			{
				return Fail("Relay Prism clone did not preserve converted combat state");
			}
		}

		const sf::Vector2f firstDirection = relayClones[0]->GetVelocity();
		const sf::Vector2f secondDirection = relayClones[1]->GetVelocity();
		const float directionDot = firstDirection.x * secondDirection.x +
			firstDirection.y * secondDirection.y;
		const float directionLengths = GetVectorLength(firstDirection) * GetVectorLength(secondDirection);
		const float scatterAngle = directionLengths > 0.f
			? std::acos(std::clamp(directionDot / directionLengths, -1.f, 1.f)) * 57.2957795131f
			: 0.f;
		if (!NearlyEqual(scatterAngle, 60.f))
		{
			return Fail("Relay Prism clone scatter direction ignored the configured angle");
		}

		const float ownerHealthBeforeRelayHit = relayOwner->GetHealth();
		relayClones.front()->OnActorBeginOverlap(relayOwner.get());
		if (!NearlyEqual(relayOwner->GetHealth(), ownerHealthBeforeRelayHit - 50.f))
		{
			return Fail("Relay Prism clone did not apply its explicit friendly-fire policy");
		}

		const std::size_t actorsBeforeLoopAttempt =
			relayWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size();
		if (relay->TryCaptureProjectile(*relayClones.back()) ||
			relayClones.back()->GetIsPendingDestroy() ||
			relayWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() !=
				actorsBeforeLoopAttempt)
		{
			return Fail("Relay Prism did not reject a clone from its own relay lineage");
		}
	}

	{
		// Ion Storm is a delivery projectile that creates a field at its
		// destination, so Relay Prism must let it pass instead of consuming it
		// without a compatible relay clone.
		World ionRelayWorld{ nullptr };
		const shared_ptr<TestCombatant> ionRelayOwner =
			ionRelayWorld.SpawnActor<TestCombatant>(1000.f).lock();
		if (!ionRelayOwner)
		{
			return Fail("Ion Storm Relay Prism test owner could not spawn");
		}

		RelayPrismPresentationProfile ionRelayPresentation;
		const shared_ptr<RelayPrismActor> ionRelay =
			ionRelayWorld.SpawnActor<RelayPrismActor>(
				ionRelayOwner.get(),
				ionRelayPresentation
			).lock();
		IonStormProjectilePresentationProfile ionProjectilePresentation;
		const shared_ptr<IonStormProjectileActor> ionProjectile =
			ionRelayWorld.SpawnActor<IonStormProjectileActor>(
				ionRelayOwner.get(),
				ionProjectilePresentation,
				std::nullopt
			).lock();
		if (!ionRelay || !ionProjectile)
		{
			return Fail("Ion Storm Relay Prism test actors could not spawn");
		}

		ionRelay->SetActorLocation({ 0.f, 0.f });
		ionRelay->ConfigureFromAttributes({
			sas::GameplayAttribute{ CommonAttributeIds::Radius, 150.f, 1.f }
		});
		ionProjectile->SetActorLocation({ 0.f, 0.f });
		if (ionProjectile->CanBeCapturedByRelay() ||
			ionRelay->TryCaptureProjectile(*ionProjectile) ||
			ionProjectile->GetIsPendingDestroy())
		{
			return Fail("Relay Prism consumed an Ion Storm delivery projectile");
		}
	}

	{
		// Crescent Reaver keeps a family-local cached launch velocity. Relay Prism
		// must therefore prove both cardinality and scatter: four spawned clones
		// with one shared velocity would render and behave like a single projectile.
		World crescentRelayWorld{ nullptr };
		const shared_ptr<TestCombatant> crescentRelayOwner =
			crescentRelayWorld.SpawnActor<TestCombatant>(1000.f).lock();
		if (!crescentRelayOwner)
		{
			return Fail("Crescent Relay Prism test owner could not spawn");
		}

		CrescentReaverPresentationProfile crescentRelayPresentation;
		const shared_ptr<CrescentReaverProjectileActor> crescentRelaySource =
			crescentRelayWorld.SpawnActor<CrescentReaverProjectileActor>(
				crescentRelayOwner.get(),
				crescentRelayPresentation
			).lock();
		if (!crescentRelaySource)
		{
			return Fail("Crescent Relay Prism source could not spawn");
		}
		crescentRelaySource->SetActorLocation({ 0.f, 0.f });
		crescentRelaySource->ConfigureFromAttributes({
			sas::GameplayAttribute{ CommonAttributeIds::Damage, 100.f, 0.f },
			sas::GameplayAttribute{
				AbilityData::CrescentReaver::Actor::Projectile::ProjectileSpeed,
				1300.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::CrescentReaver::Actor::Projectile::BounceCount,
				5.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::CrescentReaver::Actor::Projectile::BounceDamageGrowth,
				0.15f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::CrescentReaver::Actor::Projectile::BounceCooldownReduction,
				0.30f,
				0.f
			},
			sas::GameplayAttribute{ CollisionAttributeIds::Radius, 11.f, 0.f }
		});
		crescentRelaySource->SetDamageTags({ DamageTypeSchema::Kinetic });

		RelayPrismPresentationProfile crescentRelayPrismPresentation;
		const shared_ptr<RelayPrismActor> crescentRelay =
			crescentRelayWorld.SpawnActor<RelayPrismActor>(
				crescentRelayOwner.get(),
				crescentRelayPrismPresentation
			).lock();
		if (!crescentRelay)
		{
			return Fail("Crescent Relay Prism capture volume could not spawn");
		}
		crescentRelay->SetActorLocation({ 0.f, 0.f });
		crescentRelay->ConfigureFromAttributes({
			sas::GameplayAttribute{ CommonAttributeIds::Radius, 150.f, 1.f }
		});
		crescentRelay->ConfigureFromAbilityValues({
			sas::GameplayAttribute{ CommonAttributeIds::ProjectileCount, 4.f, 1.f },
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::DamageTransferRatio,
				1.f,
				0.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::AttackPowerCoefficient,
				0.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::MinimumScatterAngle,
				60.f,
				0.f,
				360.f
			},
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::MaximumScatterAngle,
				60.f,
				0.f,
				360.f
			},
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::MaximumBonusProjectileCount,
				0.f,
				0.f
			}
		});
		if (!crescentRelay->TryCaptureProjectile(*crescentRelaySource))
		{
			return Fail("Relay Prism did not capture Crescent Reaver");
		}
		// SpawnActor queues new actors until the next world lifecycle pass.
		crescentRelayWorld.TickInternal(0.f);

		List<shared_ptr<CrescentReaverProjectileActor>> crescentRelayClones;
		for (const weak_ptr<CrescentReaverProjectileActor>& projectileWeak :
			crescentRelayWorld.GetActorsByType<CrescentReaverProjectileActor>())
		{
			if (const shared_ptr<CrescentReaverProjectileActor> projectile = projectileWeak.lock();
				projectile && projectile.get() != crescentRelaySource.get() &&
				!projectile->GetIsPendingDestroy())
			{
				crescentRelayClones.push_back(projectile);
			}
		}
		if (crescentRelayClones.size() != 4)
		{
			return Fail("Relay Prism did not create four Crescent Reaver clones");
		}

		const sf::Vector2f firstCrescentDirection = crescentRelayClones[0]->GetVelocity();
		const sf::Vector2f secondCrescentDirection = crescentRelayClones[1]->GetVelocity();
		const float crescentDirectionLengths =
			GetVectorLength(firstCrescentDirection) * GetVectorLength(secondCrescentDirection);
		const float crescentScatterAngle = crescentDirectionLengths > 0.f
			? std::acos(std::clamp(
				(firstCrescentDirection.x * secondCrescentDirection.x +
					firstCrescentDirection.y * secondCrescentDirection.y) /
					crescentDirectionLengths,
				-1.f,
				1.f
			)) * 57.2957795131f
			: 0.f;
		if (!NearlyEqual(crescentScatterAngle, 60.f))
		{
			return Fail("Crescent Reaver Relay clones ignored Prism scatter directions");
		}
	}

	{
		// A Rocket captured after it has already spent part of its source
		// lifetime must still have enough time to travel from the Prism to its
		// own maximum range. The source's remaining lifetime is intentionally
		// shorter than that delivery time in this regression scenario.
		World rocketRelayWorld{ nullptr };
		const shared_ptr<TestCombatant> rocketRelayOwner =
			rocketRelayWorld.SpawnActor<TestCombatant>(1000.f).lock();
		if (!rocketRelayOwner)
		{
			return Fail("Rocket relay regression owner could not spawn");
		}

		const shared_ptr<RocketProjectileActor> rocketSource = SpawnRocket(
			rocketRelayWorld,
			*rocketRelayOwner
		);
		if (!rocketSource)
		{
			return Fail("Rocket relay regression source could not spawn");
		}

		RelayPrismPresentationProfile rocketRelayPresentation;
		const shared_ptr<RelayPrismActor> rocketRelay =
			rocketRelayWorld.SpawnActor<RelayPrismActor>(
				rocketRelayOwner.get(),
				rocketRelayPresentation
			).lock();
		if (!rocketRelay)
		{
			return Fail("Rocket relay regression Prism could not spawn");
		}
		rocketRelay->SetActorLocation(rocketSource->GetActorLocation());
		rocketRelay->ConfigureFromAttributes({
			sas::GameplayAttribute{ CommonAttributeIds::Radius, 150.f, 1.f }
		});
		rocketRelay->ConfigureFromAbilityValues({
			sas::GameplayAttribute{ CommonAttributeIds::ProjectileCount, 1.f, 1.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::DamageTransferRatio, 1.f, 0.f, 1.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::AttackPowerCoefficient, 0.f, 0.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::MinimumScatterAngle, 0.f, 0.f, 360.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::MaximumScatterAngle, 0.f, 0.f, 360.f },
			sas::GameplayAttribute{ AbilityData::RelayPrism::Attribute::MaximumBonusProjectileCount, 0.f, 0.f }
		});

		// Simulate the Rocket entering the Prism after spending most of its
		// original lifetime, without moving it out of the capture radius.
		rocketSource->AbilityWorldActor::Tick(0.5f);
		if (!rocketRelay->TryCaptureProjectile(*rocketSource))
		{
			return Fail("Rocket relay regression source was not captured");
		}
		rocketRelayWorld.TickInternal(0.f);

		shared_ptr<RocketProjectileActor> rocketClone;
		for (const weak_ptr<RocketProjectileActor>& projectileWeak :
			rocketRelayWorld.GetActorsByType<RocketProjectileActor>())
		{
			if (const shared_ptr<RocketProjectileActor> projectile = projectileWeak.lock();
				projectile && projectile.get() != rocketSource.get() &&
				!projectile->GetIsPendingDestroy())
			{
				rocketClone = projectile;
				break;
			}
		}
		if (!rocketClone)
		{
			return Fail("Rocket relay regression clone did not spawn");
		}

		const float fullRangeTravelTime = rocketClone->GetMaximumRange() /
			rocketClone->GetProjectileSpeed();
		if (rocketClone->GetLifeTime() <= fullRangeTravelTime)
		{
			return Fail("Rocket relay clone lifetime did not cover its full range");
		}

		// The clone must still exist immediately before the range endpoint. The
		// old behavior destroyed it here because it inherited the source's
		// already-consumed lifetime.
		rocketRelayWorld.TickInternal(std::max(0.01f, fullRangeTravelTime - 0.1f));
		if (rocketClone->GetIsPendingDestroy())
		{
			return Fail("Rocket relay clone disappeared before reaching full range");
		}
	}

	{
		// Ion Storm's irregular silhouette is generated once per cast. Verify the
		// resolver's stable point count, guaranteed inner core, and shared render
		// boundary contract without requiring a World or a live combat actor.
		const IonStormBoundary boundary = IonStormBoundary::Generate(
			20,
			250.f,
			250.f,
			335.f
		);
		if (boundary.GetControlPointCount() != 20 ||
			!boundary.Contains({ 0.f, 0.f }) ||
			!boundary.Contains({ 249.f, 0.f }) ||
			boundary.Contains({ 336.f, 0.f }))
		{
			return Fail("Ion Storm boundary did not preserve its core and maximum radius rules");
		}
		const List<sf::Vector2f> renderedBoundary = boundary.BuildBoundaryPoints(64);
		if (renderedBoundary.size() != 64)
		{
			return Fail("Ion Storm boundary did not produce the requested render resolution");
		}
		for (const sf::Vector2f& point : renderedBoundary)
		{
			if (!boundary.Contains(point))
			{
				return Fail("Ion Storm gameplay boundary diverged from its render points");
			}
		}
	}

	{
		// Ember Swarm Milestone 1: spawn exactly three drones around owner, fixed 6-second lifetime, clean despawn.
		const GameAbilityDefinition* emberSwarmDefinition =
			AbilityData::FindShippedAbilityDefinition(AbilityData::EmberSwarm::AbilityId::Basic);
		if (!emberSwarmDefinition)
		{
			return Fail("Ember Swarm shipped definition could not be found");
		}
		if (emberSwarmDefinition->behaviorType != AbilityBehaviorType::EmberSwarm ||
			!sas::IsLoadoutAbilitySlot(emberSwarmDefinition->slot) ||
			emberSwarmDefinition->activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			emberSwarmDefinition->lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			!NearlyEqual(emberSwarmDefinition->cooldown, 14.f) ||
			!NearlyEqual(emberSwarmDefinition->duration, 6.f) ||
			emberSwarmDefinition->maxCharges != 1 ||
			emberSwarmDefinition->abilityTags !=
				List<GameplayTag>{
					GameplayTags::Ability::Offense,
					GameplayTags::Ability::Family::EmberSwarm
				} ||
			emberSwarmDefinition->levelProgression.size() != 14)
		{
			return Fail("Ember Swarm definition did not match Milestone 1 contract");
		}

		World emberSwarmWorld{ nullptr };
		const shared_ptr<SpaceShip> emberSwarmOwner =
			emberSwarmWorld.SpawnActor<SpaceShip>(
				ShipData::Ship_Player_Fighter
			).lock();
		if (!emberSwarmOwner)
		{
			return Fail("Ember Swarm could not spawn a live player owner");
		}
		emberSwarmWorld.TickInternal(0.f);

		emberSwarmOwner->GetAbilitySystemComponent().ClearAbilitySlot(
			sas::AbilitySlot::Ability1
		);
		std::string grantFailure;
		const sas::AbilityHandle emberSwarmHandle =
			emberSwarmOwner->GetAbilitySystemComponent().GrantAbility(
				*emberSwarmDefinition,
				sas::AbilitySlot::Ability1,
				&grantFailure
			);
		if (!emberSwarmHandle.IsValid())
		{
			return Fail(("Ember Swarm could not be granted to player: " + grantFailure).c_str());
		}

		// Activate Ember Swarm
		emberSwarmOwner->GetAbilitySystemComponent().SetAbilitySlotInput(
			sas::AbilitySlot::Ability1,
			true
		);
		emberSwarmWorld.TickInternal(0.f);
		emberSwarmWorld.TickInternal(0.f);

		GameAbility* emberSwarmAbility =
			emberSwarmOwner->GetAbilitySystemComponent().GetAbility(
				emberSwarmHandle
			);
		if (!emberSwarmAbility || !emberSwarmAbility->IsActive())
		{
			return Fail("Ember Swarm did not activate on input");
		}
		if (!emberSwarmOwner->GetAbilitySystemComponent().HasOwnedTag(
			AbilityData::EmberSwarm::State::Active
		))
		{
			return Fail("Ember Swarm did not add its active state tag");
		}

		// Exactly three visible non-colliding drones spawned around owner
		const List<weak_ptr<EmberDroneActor>> drones =
			emberSwarmWorld.GetActorsByType<EmberDroneActor>();
		if (drones.size() != 3)
		{
			return Fail("Ember Swarm did not spawn exactly three drones");
		}
		for (const auto& droneWeak : drones)
		{
			const shared_ptr<EmberDroneActor> drone = droneWeak.lock();
			if (!drone || drone->GetIsPendingDestroy())
			{
				return Fail("Ember Swarm spawned an invalid drone");
			}
			if (drone->IsPhysicsEnabled() || drone->HasPhysicsBody())
			{
				return Fail("Ember Swarm drone must be non-colliding");
			}
			if (drone->IsProjectileActor())
			{
				return Fail("Ember Swarm drone must not be a projectile actor");
			}
			if (!NearlyEqual(drone->GetLifeTime(), 6.f))
			{
				return Fail("Ember Swarm drone does not have a 6-second lifetime");
			}
		}

		// Drones remain active and present at 3 seconds
		emberSwarmWorld.TickInternal(3.0f);
		if (!emberSwarmAbility->IsActive())
		{
			return Fail("Ember Swarm deactivated prematurely at 3 seconds");
		}
		if (emberSwarmWorld.GetActorsByType<EmberDroneActor>().size() != 3)
		{
			return Fail("Ember Swarm drones despawned prematurely at 3 seconds");
		}

		// Complete 6 seconds: drones cleanly despawn
		emberSwarmWorld.TickInternal(3.1f);
		emberSwarmWorld.TickInternal(0.f);

		if (emberSwarmAbility->IsActive())
		{
			return Fail("Ember Swarm remained active after 6 seconds");
		}
		if (emberSwarmOwner->GetAbilitySystemComponent().HasOwnedTag(
			AbilityData::EmberSwarm::State::Active
		))
		{
			return Fail("Ember Swarm retained active tag after 6 seconds");
		}
		if (!emberSwarmWorld.GetActorsByType<EmberDroneActor>().empty())
		{
			return Fail("Ember Swarm drones did not cleanly despawn after 6 seconds");
		}
	}

	{
		// Ember Swarm Milestone 2: Travel no damage, pulse thermal + one Ignite request, under-cap preference, single target capped fallback, leash/death/expiry cleanup.
		const GameAbilityDefinition* emberSwarmDefinition =
			AbilityData::FindShippedAbilityDefinition(AbilityData::EmberSwarm::AbilityId::Basic);
		if (!emberSwarmDefinition)
		{
			return Fail("Ember Swarm shipped definition could not be found for Milestone 2 tests");
		}

		// 1. Travel no damage test
		{
			World world{ nullptr };
			const shared_ptr<SpaceShip> owner =
				world.SpawnActor<SpaceShip>(ShipData::Ship_Player_Fighter).lock();
			const shared_ptr<TestCombatant> target =
				world.SpawnActor<TestCombatant>(100.f).lock();
			if (!owner || !target)
			{
				return Fail("Could not spawn owner and target for travel test");
			}

			owner->SetActorLocation({ 0.f, 0.f });
			owner->SetCollisionLayer(CollisionLayer::Player);

			target->SetActorLocation({ 500.f, 0.f }); // within search 750
			target->SetCollisionLayer(CollisionLayer::Enemy);
			target->SetCollisionMask(CollisionLayer::PlayerBullet);

			world.TickInternal(0.f);

			owner->GetAbilitySystemComponent().ClearAbilitySlot(sas::AbilitySlot::Ability1);
			const sas::AbilityHandle handle =
				owner->GetAbilitySystemComponent().GrantAbility(*emberSwarmDefinition, sas::AbilitySlot::Ability1);
			if (!handle.IsValid())
			{
				return Fail("Could not grant Ember Swarm for travel test");
			}

			owner->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			world.TickInternal(0.f);
			world.TickInternal(0.f);

			const List<weak_ptr<EmberDroneActor>> drones = world.GetActorsByType<EmberDroneActor>();
			if (drones.size() != 3)
			{
				return Fail("Ember Swarm did not spawn 3 drones for travel test");
			}

			// Tick 0.15s: drones are traveling at 1100 to target at 500
			world.TickInternal(0.15f);

			for (const auto& droneWeak : drones)
			{
				const shared_ptr<EmberDroneActor> drone = droneWeak.lock();
				if (!drone)
				{
					return Fail("Ember Swarm drone was invalid during travel");
				}
				if (drone->GetState() != EmberDroneActor::State::TravelingToTarget)
				{
					return Fail("Ember Swarm drone must be in TravelingToTarget state while traveling");
				}
			}

			// Target must have taken NO damage and NO ignite during travel
			if (!NearlyEqual(target->GetHealth(), 100.f))
			{
				return Fail("Ember Swarm drones dealt damage during travel");
			}
			const sas::ActiveGameplayEffect* igniteEffect =
				target->GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::IgniteEffectId);
			if (igniteEffect != nullptr && igniteEffect->stackCount > 0)
			{
				return Fail("Ember Swarm drones applied Ignite during travel");
			}
		}

		// 2. Pulse thermal + one Ignite request + damage calculation test
		{
			World world{ nullptr };
			const shared_ptr<SpaceShip> owner =
				world.SpawnActor<SpaceShip>(ShipData::Ship_Player_Fighter).lock();
			const shared_ptr<TestCombatant> target =
				world.SpawnActor<TestCombatant>(100.f).lock();
			if (!owner || !target)
			{
				return Fail("Could not spawn owner and target for pulse combat test");
			}

			owner->SetActorLocation({ 0.f, 0.f });
			owner->SetCollisionLayer(CollisionLayer::Player);

			// Place target at { 80.f, 0.f } (close to owner so drones arrive rapidly)
			target->SetActorLocation({ 80.f, 0.f });
			target->SetCollisionLayer(CollisionLayer::Enemy);
			target->SetCollisionMask(CollisionLayer::PlayerBullet);

			world.TickInternal(0.f);

			owner->GetAbilitySystemComponent().ClearAbilitySlot(sas::AbilitySlot::Ability1);
			const sas::AbilityHandle handle =
				owner->GetAbilitySystemComponent().GrantAbility(*emberSwarmDefinition, sas::AbilitySlot::Ability1);
			if (!handle.IsValid())
			{
				return Fail("Could not grant Ember Swarm for pulse combat test");
			}

			owner->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			world.TickInternal(0.f);
			world.TickInternal(0.f);

			const List<weak_ptr<EmberDroneActor>> drones = world.GetActorsByType<EmberDroneActor>();
			if (drones.empty())
			{
				return Fail("No drones spawned for pulse combat test");
			}

			// Advance by 0.15s: drones arrive at orbit and pulse
			world.TickInternal(0.15f);

			if (target->GetHealth() >= 100.f)
			{
				return Fail("Ember Swarm drone did not deal pulse damage");
			}
			const sas::ActiveGameplayEffect* igniteEffect =
				target->GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::IgniteEffectId);
			if (!igniteEffect || igniteEffect->stackCount < 1)
			{
				return Fail("Ember Swarm pulse did not apply Ignite status effect");
			}

			// Test with AttackPower attribute: 4 + AP*0.08
			owner->GetAbilitySystemComponent().GetAttributes().RegisterAttribute(OwnerAttributeIds::AttackPower, 50.f);
			const float healthBeforeAPHit = target->GetHealth();

			// Advance another 0.25s (full pulse cycle with AP 50)
			world.TickInternal(0.25f);

			const float damageDealt = healthBeforeAPHit - target->GetHealth();
			// Expected damage per pulse with AP 50: 4 + 50 * 0.08 = 8.0f (or higher on crit)
			if (damageDealt < 7.99f)
			{
				return Fail("Ember Swarm pulse did not scale damage with AttackPower (4 + AP*0.08)");
			}
		}

		// 3. Under-cap preference and batch-reservation test
		{
			World world{ nullptr };
			const shared_ptr<SpaceShip> owner =
				world.SpawnActor<SpaceShip>(ShipData::Ship_Player_Fighter).lock();
			const shared_ptr<TestCombatant> targetCapped =
				world.SpawnActor<TestCombatant>(100.f).lock();
			const shared_ptr<TestCombatant> targetUnderCap =
				world.SpawnActor<TestCombatant>(100.f).lock();
			if (!owner || !targetCapped || !targetUnderCap)
			{
				return Fail("Could not spawn actors for under-cap preference test");
			}

			owner->SetActorLocation({ 0.f, 0.f });
			owner->SetCollisionLayer(CollisionLayer::Player);

			// Target A (capped, 4 stacks) is closer to owner (150)
			targetCapped->SetActorLocation({ 150.f, 0.f });
			targetCapped->SetCollisionLayer(CollisionLayer::Enemy);
			targetCapped->SetCollisionMask(CollisionLayer::PlayerBullet);

			// Target B (under-cap, 0 stacks) is further (250)
			targetUnderCap->SetActorLocation({ 250.f, 0.f });
			targetUnderCap->SetCollisionLayer(CollisionLayer::Enemy);
			targetUnderCap->SetCollisionMask(CollisionLayer::PlayerBullet);

			// Apply 4 stacks of Ignite to Target A
			DamagePayload fourStackPayload;
			fourStackPayload.igniteStacks = 4;
			fourStackPayload.burnDamagePerSecond = 1.f;
			fourStackPayload.burnDuration = 3.f;
			fourStackPayload.burnMaxStacks = 4;
			ApplyCombatDamage(*targetCapped, 1.f, owner.get(), { DamageTypeSchema::Thermal }, fourStackPayload);

			const sas::ActiveGameplayEffect* activeIgnite =
				targetCapped->GetAbilitySystemComponent().FindGameplayEffectById(DamageStatusEffectIds::IgniteEffectId);
			if (!activeIgnite || activeIgnite->stackCount != 4)
			{
				return Fail("Target A was not initialized with 4 Ignite stacks");
			}

			world.TickInternal(0.f);

			owner->GetAbilitySystemComponent().ClearAbilitySlot(sas::AbilitySlot::Ability1);
			const sas::AbilityHandle handle =
				owner->GetAbilitySystemComponent().GrantAbility(*emberSwarmDefinition, sas::AbilitySlot::Ability1);
			owner->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			world.TickInternal(0.f);
			world.TickInternal(0.f);

			const List<weak_ptr<EmberDroneActor>> drones = world.GetActorsByType<EmberDroneActor>();
			if (drones.size() != 3)
			{
				return Fail("Drones did not spawn for under-cap preference test");
			}

			// Since Target B is under-cap (< 4) and Target A is capped (= 4),
			// drones must prefer Target B despite Target A being closer!
			for (const auto& droneWeak : drones)
			{
				const shared_ptr<EmberDroneActor> drone = droneWeak.lock();
				if (!drone || drone->GetTarget() != targetUnderCap.get())
				{
					return Fail("Ember Swarm drone did not prefer under-cap target over capped target");
				}
			}
		}

		// 4. Single target capped fallback test
		{
			World world{ nullptr };
			const shared_ptr<SpaceShip> owner =
				world.SpawnActor<SpaceShip>(ShipData::Ship_Player_Fighter).lock();
			const shared_ptr<TestCombatant> soleTarget =
				world.SpawnActor<TestCombatant>(100.f).lock();
			if (!owner || !soleTarget)
			{
				return Fail("Could not spawn actors for single target capped fallback test");
			}

			owner->SetActorLocation({ 0.f, 0.f });
			owner->SetCollisionLayer(CollisionLayer::Player);

			soleTarget->SetActorLocation({ 200.f, 0.f });
			soleTarget->SetCollisionLayer(CollisionLayer::Enemy);
			soleTarget->SetCollisionMask(CollisionLayer::PlayerBullet);

			// Pre-apply 4 stacks of Ignite to sole target
			DamagePayload fourStackPayload;
			fourStackPayload.igniteStacks = 4;
			fourStackPayload.burnDamagePerSecond = 1.f;
			fourStackPayload.burnDuration = 3.f;
			fourStackPayload.burnMaxStacks = 4;
			ApplyCombatDamage(*soleTarget, 1.f, owner.get(), { DamageTypeSchema::Thermal }, fourStackPayload);

			world.TickInternal(0.f);

			owner->GetAbilitySystemComponent().ClearAbilitySlot(sas::AbilitySlot::Ability1);
			const sas::AbilityHandle handle =
				owner->GetAbilitySystemComponent().GrantAbility(*emberSwarmDefinition, sas::AbilitySlot::Ability1);
			owner->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			world.TickInternal(0.f);
			world.TickInternal(0.f);

			const List<weak_ptr<EmberDroneActor>> drones = world.GetActorsByType<EmberDroneActor>();
			if (drones.size() != 3)
			{
				return Fail("Drones did not spawn for single target fallback test");
			}

			// When no under-cap targets exist, drones must choose and retain the capped target so single target damage continues
			for (const auto& droneWeak : drones)
			{
				const shared_ptr<EmberDroneActor> drone = droneWeak.lock();
				if (!drone || drone->GetTarget() != soleTarget.get())
				{
					return Fail("Ember Swarm drones did not fall back to capped target when no under-cap target exists");
				}
			}

			const float initialHealth = soleTarget->GetHealth();
			// Advance time to allow drones to travel, orbit, and pulse the capped target
			for (int step = 0; step < 10; ++step)
			{
				world.TickInternal(0.1f);
			}

			if (soleTarget->GetHealth() >= initialHealth)
			{
				return Fail("Ember Swarm drones failed to continue dealing damage to single capped target");
			}
		}

		// 5. Leash / death / expiry cleanup test
		{
			World world{ nullptr };
			const shared_ptr<SpaceShip> owner =
				world.SpawnActor<SpaceShip>(ShipData::Ship_Player_Fighter).lock();
			const shared_ptr<TestCombatant> target =
				world.SpawnActor<TestCombatant>(100.f).lock();
			if (!owner || !target)
			{
				return Fail("Could not spawn actors for cleanup test");
			}

			owner->SetActorLocation({ 0.f, 0.f });
			owner->SetCollisionLayer(CollisionLayer::Player);

			target->SetActorLocation({ 200.f, 0.f });
			target->SetCollisionLayer(CollisionLayer::Enemy);
			target->SetCollisionMask(CollisionLayer::PlayerBullet);

			world.TickInternal(0.f);

			owner->GetAbilitySystemComponent().ClearAbilitySlot(sas::AbilitySlot::Ability1);
			const sas::AbilityHandle handle =
				owner->GetAbilitySystemComponent().GrantAbility(*emberSwarmDefinition, sas::AbilitySlot::Ability1);
			owner->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			world.TickInternal(0.f);
			world.TickInternal(0.f);

			const List<weak_ptr<EmberDroneActor>> drones = world.GetActorsByType<EmberDroneActor>();
			if (drones.size() != 3)
			{
				return Fail("Drones did not spawn for cleanup test");
			}

			// Drones should have targeted the target
			const shared_ptr<EmberDroneActor> drone0 = drones[0].lock();
			if (!drone0 || drone0->GetTarget() != target.get())
			{
				return Fail("Drone did not acquire target initially");
			}

			// Move target beyond leash (> 1000 from owner, e.g. 1050)
			target->SetActorLocation({ 1050.f, 0.f });
			world.TickInternal(0.26f); // Triggers next 0.25s evaluation

			if (drone0->GetTarget() != nullptr)
			{
				return Fail("Ember Swarm drone did not drop target after target exceeded leash of 1000");
			}

			// Move target back into search range (< 750)
			target->SetActorLocation({ 300.f, 0.f });
			world.TickInternal(0.26f);

			if (drone0->GetTarget() != target.get())
			{
				return Fail("Ember Swarm drone did not re-acquire target after returning into search range");
			}

			// Target death: destroy target
			target->Destroy();
			world.TickInternal(0.05f);

			if (drone0->GetTarget() != nullptr)
			{
				return Fail("Ember Swarm drone did not drop target upon target death/destruction");
			}

			// Ability expiry: tick until 6.0s lifetime ends
			world.TickInternal(5.5f);
			world.TickInternal(0.f);

			if (!world.GetActorsByType<EmberDroneActor>().empty())
			{
				return Fail("Ember Swarm drones were not cleaned up upon ability duration expiry");
			}
		}
	}

		{
		// =========================================================================
		// Reclaimer Protocol Tests
		// =========================================================================

		// 1. Content/progression JSON + fallback & definition validation
		const GameAbilityDefinition* reclaimerDefinition =
			AbilityData::FindShippedAbilityDefinition(AbilityData::ReclaimerProtocol::AbilityId::Basic);
		if (!reclaimerDefinition)
		{
			return Fail("Reclaimer Protocol shipped definition could not be found");
		}
		if (reclaimerDefinition->behaviorType != AbilityBehaviorType::ReclaimerProtocol ||
			!sas::IsLoadoutAbilitySlot(reclaimerDefinition->slot) ||
			reclaimerDefinition->activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			reclaimerDefinition->lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			!NearlyEqual(reclaimerDefinition->cooldown, 16.f) ||
			!NearlyEqual(reclaimerDefinition->duration, 6.f) ||
			reclaimerDefinition->maxCharges != 1 ||
			reclaimerDefinition->abilityTags !=
				List<GameplayTag>{
					GameplayTags::Ability::Defense,
					GameplayTags::Ability::Family::ReclaimerProtocol
				} ||
			reclaimerDefinition->levelProgression.size() != 14)
		{
			return Fail("Reclaimer Protocol definition did not match baseline contract");
		}

		std::string reclaimerValidationFailure;
		if (!ValidateAbilityDefinition(*reclaimerDefinition, &reclaimerValidationFailure))
		{
			return Fail(("Reclaimer Protocol definition validation failed: " + reclaimerValidationFailure).c_str());
		}

		// Verify fallback equals JSON progression
		const sas::GameplayAttribute* healRatioAttr = sas::FindAttribute(
			reclaimerDefinition->attributes,
			AbilityData::ReclaimerProtocol::Attribute::HealRatio
		);
		if (!healRatioAttr || !NearlyEqual(healRatioAttr->baseValue, 0.04f))
		{
			return Fail("Reclaimer Protocol base HealRatio attribute is missing or not 0.04");
		}

		// Level progression check: 14 steps, each +.0015 HealRatio and -.25 Cooldown
		// Level 15 = level 1 + 14 upgrades => HealRatio = 0.04 + 14 * 0.0015 = 0.061, Cooldown = 16 - 14 * 0.25 = 12.5
		float simulatedHealRatio = healRatioAttr->baseValue;
		float simulatedCooldown = reclaimerDefinition->cooldown;
		for (const auto& step : reclaimerDefinition->levelProgression)
		{
			for (const auto& mod : step.attributeModifiers)
			{
				if (mod.attributeId == AbilityData::ReclaimerProtocol::Attribute::HealRatio)
				{
					simulatedHealRatio += mod.magnitude;
				}
				else if (mod.attributeId == CommonAttributeIds::Cooldown)
				{
					simulatedCooldown += mod.magnitude;
				}
			}
		}
		if (!NearlyEqual(simulatedHealRatio, 0.061f) || !NearlyEqual(simulatedCooldown, 12.5f))
		{
			return Fail("Reclaimer Protocol level 15 progression does not match expected 0.061 HealRatio or 12.5 Cooldown");
		}

		// Actor definition check
		const AbilityActorDefinition* kitActorDef =
			AbilityData::FindAbilityActorDefinition(AbilityData::ReclaimerProtocol::Actor::RepairKit::BasicDefinitionId);
		if (!kitActorDef)
		{
			return Fail("Reclaimer Repair Kit actor definition could not be found");
		}
		if (kitActorDef->actorType != AbilityActorType::ReclaimerRepairKit ||
			!NearlyEqual(kitActorDef->lifeTime, 10.f))
		{
			return Fail("Reclaimer Repair Kit actor definition does not match expected actorType or 10s lifetime");
		}
		const sas::GameplayAttribute* kitDurationAttr = sas::FindAttribute(kitActorDef->attributes, CommonAttributeIds::Duration);
		const sas::GameplayAttribute* kitRadiusAttr = sas::FindAttribute(kitActorDef->attributes, CollisionAttributeIds::Radius);
		const sas::GameplayAttribute* kitHealRatioAttr = sas::FindAttribute(kitActorDef->attributes, AbilityData::ReclaimerProtocol::Actor::RepairKit::HealRatio);
		if (!kitDurationAttr || !NearlyEqual(kitDurationAttr->baseValue, 10.f) ||
			!kitRadiusAttr || !NearlyEqual(kitRadiusAttr->baseValue, 16.f) ||
			!kitHealRatioAttr || !NearlyEqual(kitHealRatioAttr->baseValue, 0.04f))
		{
			return Fail("Reclaimer Repair Kit actor attributes do not match expected Duration, Radius, or HealRatio");
		}

		// 2. Profile registration / isolation / missing rejection
		const ReclaimerProtocolPresentationProfile* registeredProfile =
			PresentationProfileRegistry<ReclaimerProtocolPresentationProfile>::Find(
				ReclaimerProtocolPresentationIds::RepairKitBasic
			);
		if (!registeredProfile)
		{
			return Fail("Reclaimer Protocol typed presentation profile was not registered");
		}
		if (registeredProfile->profileId.ToString() != ReclaimerProtocolPresentationIds::RepairKitBasic)
		{
			return Fail("Reclaimer Protocol presentation profile ID mismatch");
		}
		if (PresentationProfileRegistry<ReclaimerProtocolPresentationProfile>::Find("Presentation.Ability.NonExistent.Profile") != nullptr)
		{
			return Fail("Reclaimer Protocol profile registry returned an unregistered profile");
		}

		// Missing profile rejection during actor validation
		AbilityActorDefinition invalidActorDef = *kitActorDef;
		invalidActorDef.presentationProfileId = sas::ContentId{ "Presentation.Ability.Invalid.Missing.Profile" };
		if (AbilityActorRegistry::ValidateDefinition(invalidActorDef).isValid)
		{
			return Fail("ReclaimerRepairKit actor handler must reject definition with missing presentation profile");
		}

		// Missing HealRatio rejection
		AbilityActorDefinition missingAttrDef = *kitActorDef;
		missingAttrDef.attributes.clear();
		missingAttrDef.attributes.push_back(sas::GameplayAttribute{ CommonAttributeIds::Duration, 10.f, 0.01f });
		missingAttrDef.attributes.push_back(sas::GameplayAttribute{ CollisionAttributeIds::Radius, 16.f, 0.f });
		if (AbilityActorRegistry::ValidateDefinition(missingAttrDef).isValid)
		{
			return Fail("ReclaimerRepairKit actor handler must reject definition missing HealRatio attribute");
		}

		// 3. Activation has NO health, shield, regen, or damage effect
		{
			World world{ nullptr };
			const shared_ptr<SpaceShip> owner =
				world.SpawnActor<SpaceShip>(ShipData::Ship_Player_Fighter).lock();
			if (!owner)
			{
				return Fail("Could not spawn player spaceship for activation test");
			}
			world.TickInternal(0.f);

			// Injure owner and deplete shield slightly to verify activation does NOT heal, shield, or add regen
			owner->GetHealthComponent().ChangeHealth(-30.f);
			owner->GetShieldComponent().ChangeShield(-20.f);
			const float healthBefore = owner->GetHealthComponent().GetHealth();
			const float shieldBefore = owner->GetShieldComponent().GetShield();

			owner->GetAbilitySystemComponent().ClearAbilitySlot(sas::AbilitySlot::Ability1);
			std::string grantFailure;
			const sas::AbilityHandle handle = owner->GetAbilitySystemComponent().GrantAbility(
				*reclaimerDefinition,
				sas::AbilitySlot::Ability1,
				&grantFailure
			);
			if (!handle.IsValid())
			{
				return Fail(("Failed to grant Reclaimer Protocol: " + grantFailure).c_str());
			}

			// Activate
			owner->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			world.TickInternal(0.f);
			world.TickInternal(0.f);

			GameAbility* ability = owner->GetAbilitySystemComponent().GetAbility(handle);
			if (!ability || !ability->IsActive())
			{
				return Fail("Reclaimer Protocol failed to activate");
			}
			if (!owner->GetAbilitySystemComponent().HasOwnedTag(AbilityData::ReclaimerProtocol::State::Active))
			{
				return Fail("Reclaimer Protocol did not add Active state tag");
			}

			if (!NearlyEqual(owner->GetHealthComponent().GetHealth(), healthBefore))
			{
				return Fail("Reclaimer Protocol activation modified health");
			}
			if (!NearlyEqual(owner->GetShieldComponent().GetShield(), shieldBefore))
			{
				return Fail("Reclaimer Protocol activation modified shield");
			}
		}

		// 4. Ability Haste applies to cooldown
		{
			TestCombatant hasteOwner;
			hasteOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
			hasteOwner.GetAbilitySystemComponent().GetAttributes().ApplyBaseModifier(
				sas::AttributeModifier{ OwnerAttributeIds::AbilityHaste, 100.f }
			);
			const sas::AbilityHandle handle =
				hasteOwner.GetAbilitySystemComponent().GrantAbility(*reclaimerDefinition, sas::AbilitySlot::Ability1);
			if (!handle.IsValid())
			{
				return Fail("Could not grant Reclaimer Protocol to haste owner");
			}
			GameAbility* ability = hasteOwner.GetAbilitySystemComponent().GetAbility(handle);
			if (!ability)
			{
				return Fail("Could not find ability on haste owner");
			}
			const float expectedCooldown = 16.f * sas::AttributeMath::GetAbilityCooldownMultiplier(100.f);
			if (!NearlyEqual(ability->GetCooldownDuration(), expectedCooldown))
			{
				return Fail("Reclaimer Protocol cooldown did not scale with Ability Haste");
			}
		}

		// 5. Qualifying owner kill creates exactly one kit at target death snapshot location;
		// invalid events (non-owner source, target not killed, target not enemy, non-finite loc) do NOT create kit.
		{
			World world{ nullptr };
			const shared_ptr<SpaceShip> owner =
				world.SpawnActor<SpaceShip>(ShipData::Ship_Player_Fighter).lock();
			const shared_ptr<TestCombatant> nonOwner =
				world.SpawnActor<TestCombatant>(100.f).lock();
			if (!owner || !nonOwner)
			{
				return Fail("Could not spawn owner or nonOwner for event testing");
			}
			world.TickInternal(0.f);

			owner->GetAbilitySystemComponent().ClearAbilitySlot(sas::AbilitySlot::Ability1);
			const sas::AbilityHandle handle =
				owner->GetAbilitySystemComponent().GrantAbility(*reclaimerDefinition, sas::AbilitySlot::Ability1);
			owner->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			world.TickInternal(0.f);
			world.TickInternal(0.f);

			// Test A: Kill confirmed from a non-owner source -> 0 kits
			{
				DamageContext nonOwnerContext;
				nonOwnerContext.source = nonOwner.get();
				nonOwnerContext.targetWasKilled = true;
				nonOwnerContext.targetWasEnemyCombatant = true;
				nonOwnerContext.targetLocationAtResolution = { 100.f, 200.f };

				sas::AbilityEvent killEvent;
				killEvent.eventTag = GameplayTags::Event::Combat::KillConfirmed;
				killEvent.SetSource(nonOwner.get());
				killEvent.SetContext(&nonOwnerContext);

				owner->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
				world.TickInternal(0.f);

				if (!world.GetActorsByType<ReclaimerRepairKitActor>().empty())
				{
					return Fail("Reclaimer Protocol spawned a kit for a non-owner kill event");
				}
			}

			// Test B: targetWasKilled = false -> 0 kits
			{
				DamageContext notKilledContext;
				notKilledContext.source = owner.get();
				notKilledContext.targetWasKilled = false;
				notKilledContext.targetWasEnemyCombatant = true;
				notKilledContext.targetLocationAtResolution = { 100.f, 200.f };

				sas::AbilityEvent killEvent;
				killEvent.eventTag = GameplayTags::Event::Combat::KillConfirmed;
				killEvent.SetSource(owner.get());
				killEvent.SetContext(&notKilledContext);

				owner->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
				world.TickInternal(0.f);

				if (!world.GetActorsByType<ReclaimerRepairKitActor>().empty())
				{
					return Fail("Reclaimer Protocol spawned a kit when target was not killed");
				}
			}

			// Test C: targetWasEnemyCombatant = false -> 0 kits
			{
				DamageContext nonEnemyContext;
				nonEnemyContext.source = owner.get();
				nonEnemyContext.targetWasKilled = true;
				nonEnemyContext.targetWasEnemyCombatant = false;
				nonEnemyContext.targetLocationAtResolution = { 100.f, 200.f };

				sas::AbilityEvent killEvent;
				killEvent.eventTag = GameplayTags::Event::Combat::KillConfirmed;
				killEvent.SetSource(owner.get());
				killEvent.SetContext(&nonEnemyContext);

				owner->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
				world.TickInternal(0.f);

				if (!world.GetActorsByType<ReclaimerRepairKitActor>().empty())
				{
					return Fail("Reclaimer Protocol spawned a kit when target was not an enemy combatant");
				}
			}

			// Test D: Non-finite location -> 0 kits
			{
				DamageContext nonFiniteContext;
				nonFiniteContext.source = owner.get();
				nonFiniteContext.targetWasKilled = true;
				nonFiniteContext.targetWasEnemyCombatant = true;
				nonFiniteContext.targetLocationAtResolution = { std::numeric_limits<float>::quiet_NaN(), 200.f };

				sas::AbilityEvent killEvent;
				killEvent.eventTag = GameplayTags::Event::Combat::KillConfirmed;
				killEvent.SetSource(owner.get());
				killEvent.SetContext(&nonFiniteContext);

				owner->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
				world.TickInternal(0.f);

				if (!world.GetActorsByType<ReclaimerRepairKitActor>().empty())
				{
					return Fail("Reclaimer Protocol spawned a kit with non-finite coordinates");
				}
			}

			// Test E: Qualifying owner kill -> exactly ONE kit at target location snapshot
			const sf::Vector2f killLocation{ 320.f, -150.f };
			{
				DamageContext validContext;
				validContext.source = owner.get();
				validContext.targetWasKilled = true;
				validContext.targetWasEnemyCombatant = true;
				validContext.targetLocationAtResolution = { killLocation.x, killLocation.y };

				sas::AbilityEvent killEvent;
				killEvent.eventTag = GameplayTags::Event::Combat::KillConfirmed;
				killEvent.SetSource(owner.get());
				killEvent.SetContext(&validContext);

				owner->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
				world.TickInternal(0.f);

				const List<weak_ptr<ReclaimerRepairKitActor>> kits =
					world.GetActorsByType<ReclaimerRepairKitActor>();
				if (kits.size() != 1)
				{
					return Fail("Reclaimer Protocol did not spawn exactly one kit on qualifying kill");
				}

				const shared_ptr<ReclaimerRepairKitActor> kit = kits.front().lock();
				if (!kit)
				{
					return Fail("Reclaimer Repair Kit is invalid after spawn");
				}
				if (!NearlyEqual(kit->GetActorLocation().x, killLocation.x) ||
					!NearlyEqual(kit->GetActorLocation().y, killLocation.y))
				{
					return Fail("Reclaimer Repair Kit was not spawned at the immutable target death snapshot location");
				}
				if (!NearlyEqual(kit->GetResolvedHealRatio(), 0.04f))
				{
					return Fail("Reclaimer Repair Kit did not snapshot resolved HealRatio");
				}
			}

			// Multiple qualifying kills spawn multiple kits (no cap)
			for (int i = 0; i < 3; ++i)
			{
				DamageContext anotherContext;
				anotherContext.source = owner.get();
				anotherContext.targetWasKilled = true;
				anotherContext.targetWasEnemyCombatant = true;
				anotherContext.targetLocationAtResolution = { 10.f * static_cast<float>(i), 20.f };

				sas::AbilityEvent killEvent;
				killEvent.eventTag = GameplayTags::Event::Combat::KillConfirmed;
				killEvent.SetSource(owner.get());
				killEvent.SetContext(&anotherContext);

				owner->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
				world.TickInternal(0.f);
			}

			if (world.GetActorsByType<ReclaimerRepairKitActor>().size() != 4)
			{
				return Fail("Reclaimer Protocol did not spawn kit for each qualifying kill without cap");
			}
		}

		// 6. Kit survives ability end; end blocks future kits; kit expires at 10s
		{
			World world{ nullptr };
			const shared_ptr<SpaceShip> owner =
				world.SpawnActor<SpaceShip>(ShipData::Ship_Player_Fighter).lock();
			if (!owner)
			{
				return Fail("Could not spawn owner for lifecycle survival test");
			}
			world.TickInternal(0.f);

			owner->GetAbilitySystemComponent().ClearAbilitySlot(sas::AbilitySlot::Ability1);
			owner->GetAbilitySystemComponent().GrantAbility(*reclaimerDefinition, sas::AbilitySlot::Ability1);
			owner->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			world.TickInternal(0.f);
			world.TickInternal(0.f);

			// Spawn 1 kit at t = 2s
			world.TickInternal(2.0f);
			DamageContext killContext;
			killContext.source = owner.get();
			killContext.targetWasKilled = true;
			killContext.targetWasEnemyCombatant = true;
			killContext.targetLocationAtResolution = { 50.f, 50.f };

			sas::AbilityEvent killEvent;
			killEvent.eventTag = GameplayTags::Event::Combat::KillConfirmed;
			killEvent.SetSource(owner.get());
			killEvent.SetContext(&killContext);
			owner->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
			world.TickInternal(0.f);

			if (world.GetActorsByType<ReclaimerRepairKitActor>().size() != 1)
			{
				return Fail("Failed to spawn initial kit for survival test");
			}

			// Advance beyond ability duration: 6s total (2s already elapsed + 4.1s)
			world.TickInternal(4.1f);

			// Ability must be ended
			if (owner->GetAbilitySystemComponent().HasOwnedTag(AbilityData::ReclaimerProtocol::State::Active))
			{
				return Fail("Reclaimer Protocol remained active after its duration");
			}

			// Kit spawned at t=2s has age 4.1s (out of 10s lifetime). It must still exist!
			if (world.GetActorsByType<ReclaimerRepairKitActor>().empty())
			{
				return Fail("Reclaimer Repair Kit was prematurely destroyed when ability ended");
			}

			// Now that ability ended, future kills must NOT spawn kits
			owner->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
			world.TickInternal(0.f);
			if (world.GetActorsByType<ReclaimerRepairKitActor>().size() != 1)
			{
				return Fail("Reclaimer Protocol spawned a kit after ability ended");
			}

			// Advance until kit expires (total age > 10s, remaining 5.9s)
			world.TickInternal(6.0f);
			world.TickInternal(0.f);

			if (!world.GetActorsByType<ReclaimerRepairKitActor>().empty())
			{
				return Fail("Reclaimer Repair Kit failed to expire after its 10-second lifetime");
			}
		}

		// 7. Healing math, clamp, full health retains kit, no shield/regen mutation
		{
			World world{ nullptr };
			const shared_ptr<PlayerSpaceShip> player =
				world.SpawnActor<PlayerSpaceShip>(ShipData::Ship_Player_Fighter).lock();
			if (!player)
			{
				return Fail("Could not spawn player for kit overlap test");
			}
			world.TickInternal(0.f);

			const float maxHealth = player->GetHealthComponent().GetMaxHealth();
			// Place player at { 0, 0 }
			player->SetActorLocation({ 0.f, 0.f });

			// Case A: Full health retains kit on overlap
			const shared_ptr<ReclaimerRepairKitActor> kit1 =
				world.SpawnActor<ReclaimerRepairKitActor>(
					player.get(),
					*registeredProfile
				).lock();
			if (!kit1)
			{
				return Fail("Could not spawn kit1");
			}
			kit1->SetActorLocation({ 0.f, 0.f });
			kit1->SetResolvedHealRatio(0.04f);
			world.TickInternal(0.f);

			// Overlap at full health
			kit1->OnActorBeginOverlap(player.get());
			world.TickInternal(0.f);

			if (kit1->GetIsPendingDestroy())
			{
				return Fail("Reclaimer Repair Kit was consumed when player was at full health");
			}
			if (!NearlyEqual(player->GetHealthComponent().GetHealth(), maxHealth))
			{
				return Fail("Player health was mutated during full health overlap");
			}

			// Case B: Injured player heals min(maxHealth * ratio, missing)
			// Injure player by 20 points
			player->GetHealthComponent().ChangeHealth(-20.f);
			const float currentHealthBefore = player->GetHealthComponent().GetHealth();
			const float shieldBefore = player->GetShieldComponent().GetShield();

			// Expected heal: maxHealth * 0.04 = 100 * 0.04 = 4.0 (missing = 20 > 4.0)
			const float expectedHeal = maxHealth * 0.04f;

			kit1->OnActorBeginOverlap(player.get());
			world.TickInternal(0.15f); // allow flash to complete and kit to destroy

			if (!NearlyEqual(player->GetHealthComponent().GetHealth(), currentHealthBefore + expectedHeal))
			{
				return Fail("Reclaimer Repair Kit healing did not equal maxHealth * HealRatio");
			}
			if (!NearlyEqual(player->GetShieldComponent().GetShield(), shieldBefore))
			{
				return Fail("Reclaimer Repair Kit overlap mutated shield");
			}
			if (!kit1->GetIsPendingDestroy())
			{
				return Fail("Reclaimer Repair Kit was not consumed after positive healing");
			}

			// Case C: Missing health is less than maxHealth * ratio -> clamped to missing
			// Heal player to maxHealth - 1.0f
			player->GetHealthComponent().SetInitialHealth(maxHealth - 1.f, maxHealth);
			const shared_ptr<ReclaimerRepairKitActor> kit2 =
				world.SpawnActor<ReclaimerRepairKitActor>(
					player.get(),
					*registeredProfile
				).lock();
			if (!kit2)
			{
				return Fail("Could not spawn kit2");
			}
			kit2->SetActorLocation({ 0.f, 0.f });
			kit2->SetResolvedHealRatio(0.04f);
			world.TickInternal(0.f);

			kit2->OnActorBeginOverlap(player.get());
			world.TickInternal(0.15f);

			if (!NearlyEqual(player->GetHealthComponent().GetHealth(), maxHealth))
			{
				return Fail("Reclaimer Repair Kit did not clamp healing to missing health");
			}
			if (!kit2->GetIsPendingDestroy())
			{
				return Fail("Reclaimer Repair Kit was not consumed when healing clamped to missing health");
			}

			// Case D: Non-player ship cannot consume or heal from repair kit even via direct overlap
			const shared_ptr<SpaceShip> nonPlayerShip =
				world.SpawnActor<SpaceShip>(ShipData::Ship_Player_Fighter).lock();
			if (!nonPlayerShip)
			{
				return Fail("Could not spawn non-player ship for overlap rejection test");
			}
			nonPlayerShip->SetActorLocation({ 0.f, 0.f });
			nonPlayerShip->GetHealthComponent().ChangeHealth(-20.f);
			const float nonPlayerHealthBefore = nonPlayerShip->GetHealthComponent().GetHealth();

			const shared_ptr<ReclaimerRepairKitActor> kit3 =
				world.SpawnActor<ReclaimerRepairKitActor>(
					player.get(),
					*registeredProfile
				).lock();
			if (!kit3)
			{
				return Fail("Could not spawn kit3");
			}
			kit3->SetActorLocation({ 0.f, 0.f });
			kit3->SetResolvedHealRatio(0.04f);
			world.TickInternal(0.f);

			kit3->OnActorBeginOverlap(nonPlayerShip.get());
			world.TickInternal(0.15f);

			if (kit3->GetIsPendingDestroy())
			{
				return Fail("Reclaimer Repair Kit was consumed by non-player ship overlap");
			}
			if (!NearlyEqual(nonPlayerShip->GetHealthComponent().GetHealth(), nonPlayerHealthBefore))
			{
				return Fail("Non-player ship health was modified by Reclaimer Repair Kit overlap");
			}
		}

		// 8. Pre-existing player-owned source case
		{
			World world{ nullptr };
			const shared_ptr<PlayerSpaceShip> player =
				world.SpawnActor<PlayerSpaceShip>(ShipData::Ship_Player_Fighter).lock();
			const shared_ptr<TestCombatant> enemy =
				world.SpawnActor<TestCombatant>(100.f).lock();
			if (!player || !enemy)
			{
				return Fail("Could not spawn player or enemy for player-owned source test");
			}
			player->SetActorLocation({ 0.f, 0.f });
			player->SetCollisionLayer(CollisionLayer::Player);
			enemy->SetActorLocation({ 200.f, 0.f });
			enemy->SetCollisionLayer(CollisionLayer::Enemy);
			enemy->SetCollisionMask(CollisionLayer::PlayerBullet);
			world.TickInternal(0.f);

			player->GetAbilitySystemComponent().ClearAbilitySlot(sas::AbilitySlot::Ability1);
			player->GetAbilitySystemComponent().GrantAbility(*reclaimerDefinition, sas::AbilitySlot::Ability1);
			player->GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::Ability1, true);
			world.TickInternal(0.f);
			world.TickInternal(0.f);

			// Simulate CombatRuntime damage resolution resulting in kill from player
			DamageContext killContext;
			killContext.source = player.get();
			killContext.target = enemy.get();
			killContext.targetWasKilled = true;
			killContext.targetWasEnemyCombatant = true;
			killContext.targetLocationAtResolution = { enemy->GetActorLocation().x, enemy->GetActorLocation().y };

			sas::AbilityEvent killEvent;
			killEvent.eventTag = GameplayTags::Event::Combat::KillConfirmed;
			killEvent.SetSource(player.get());
			killEvent.SetTarget(enemy.get());
			killEvent.SetContext(&killContext);

			player->GetAbilitySystemComponent().HandleGameplayEvent(killEvent);
			world.TickInternal(0.f);

			const List<weak_ptr<ReclaimerRepairKitActor>> kits =
				world.GetActorsByType<ReclaimerRepairKitActor>();
			if (kits.size() != 1)
			{
				return Fail("Player-owned kill confirmed did not spawn a Reclaimer Repair Kit");
			}
		}
	}
	return 0;
}
