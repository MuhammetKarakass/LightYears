#include "framework/Core.h"
#include "gameplay/attributes/AttributeMath.h"
#include "gameplay/attributes/AttributeSystem.h"
#include "gameplay/progression/ShipProgression.h"
#include "gameplay/effects/GameplayEffectBehavior.h"
#include "gameplay/effects/GameplayEffectContent.h"
#include "gameplay/effects/GameplayEffectSpec.h"
#include "gameplay/effects/GameplayEffectSystem.h"
#include "gameplay/effects/GameplayEffectValidation.h"
#include "gameplay/ability/AbilitySystem.h"
#include "gameplay/ability/AbilityBehaviorRegistration.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/dash/DashMovementController.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyFieldActor.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"
#include "gameplay/ability/rocket/RocketVisualActor.h"
#include "gameplay/ability/sunBeam/SunBeamStrikeActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationIds.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationProfile.h"
#include "presentation/ability/rocket/RocketPresentationIds.h"
#include "presentation/ability/rocket/RocketPresentationProfile.h"
#include "presentation/ability/sunBeam/SunBeamPresentationIds.h"
#include "presentation/ability/sunBeam/SunBeamPresentationProfile.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/effects/gravityAnomaly/GravityAnomalyEffectBehavior.h"
#include "presentation/effects/gravityAnomaly/GravityAnomalyEffectVisual.h"
#include "gameplay/ship/ShipRuntime.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/EnergyComponent.h"
#include "gameplay/ability/dash/DashMovementMath.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/camera/CameraManager.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameConfigs/combat/WeaponConfig.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameConfigs/ability/GravityAnomalyConfig.h"
#include "gameConfigs/ship/ShipConfig.h"
#include "player/PlayerSpaceShip.h"
#include "spaceShip/SpaceShip.h"
#include "gameplay/weapon/PrimaryWeaponExecutionSystem.h"
#include "gameplay/weapon/PrimaryWeaponHandlerRegistry.h"
#include "gameplay/weapon/impact/ShotgunVolleyImpactGroup.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h"
#include "gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.h"
#include "gameplay/weapon/wave/ExpandingWaveWeaponActor.h"
#include "gameplay/weapon/visuals/ElectricArcVisualActor.h"
#include "gameplay/attachment/AttachmentLoadout.h"
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
			mCombatRuntime.GetAttributes().RegisterAttribute(ly::OwnerAttributeIds::Armor, 0.f);
			mCombatRuntime.GetAttributes().RegisterAttribute(ly::OwnerAttributeIds::AbilityHaste, 0.f, -0.95f);
			mCombatRuntime.GetAttributes().RegisterAttribute(ly::OwnerAttributeIds::MoveSpeedHorizontal, 0.f, -0.95f);
			mCombatRuntime.GetAttributes().RegisterAttribute(ly::OwnerAttributeIds::MoveSpeedVertical, 0.f, -0.95f);
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
				mCombatRuntime.GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::MoveSpeedHorizontal),
				mCombatRuntime.GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::MoveSpeedVertical)
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

	struct DashEventRecorder
	{
		void Record(const ly::AbilityEvent& event)
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
		float energyMax = 0.f;
		float luck = 0.f;
	};

	// Mirrors the current player fighter profile so this focused GAS-Lite target remains
	// independent from reward actor linkage in the complete ship content header.
	const ShipProgressionDefinition FighterProgressionDefinition{
		100.f,
		1.25f,
		{
			{ ly::OwnerAttributeIds::AttackPower, 3.f },
			{ ly::OwnerAttributeIds::AttackSpeed, 2.f },
			{ ly::OwnerAttributeIds::CriticalChance, 2.f },
			{ ly::OwnerAttributeIds::MaxHealth, 1.f },
			{ ly::OwnerAttributeIds::Armor, 1.f },
			{ ly::OwnerAttributeIds::EnergyMax, 0.5f },
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
		float additionalEnergyMax = 0.f
	)
	{
		TestCombatant owner;
		ly::CombatRuntime& runtime = owner.GetCombatRuntime();
		runtime.InitializeOwnerAttributes(100.f);

		ly::ShipProgression progression;
		progression.Configure(FighterProgressionDefinition);
		progression.BindAttributes(runtime.GetAttributes());
		while (progression.GetLevel() < level)
		{
			progression.AddXP(progression.GetXPRequiredForNextLevel());
		}
		runtime.GetAttributes().ApplyBaseModifier(
			ly::AttributeModifier{ ly::OwnerAttributeIds::AttackPower, additionalAttackPower }
		);
		runtime.GetAttributes().ApplyBaseModifier(
			ly::AttributeModifier{ ly::OwnerAttributeIds::AttackSpeed, additionalAttackSpeed }
		);
		runtime.GetAttributes().ApplyBaseModifier(
			ly::AttributeModifier{ ly::OwnerAttributeIds::EnergyMax, additionalEnergyMax }
		);

		const ly::AbilityHandle handle = runtime.GetAbilities().GrantAbility(
			AbilityData::MakePrimaryFireAbilityDefinition(weapon)
		);
		if (!handle.IsValid())
		{
			return false;
		}
		runtime.GetAbilities().SetSlotInput(ly::AbilitySlot::PrimaryFire, true);
		runtime.GetAbilities().Tick(0.f);
		const ly::AbilityInstance* instance = runtime.GetAbilities().GetAbility(handle);
		if (!instance || instance->GetPrimaryWeaponRuntimeAttributes().empty())
		{
			return false;
		}

		const ly::GameplayAttributeList& attributes = instance->GetPrimaryWeaponRuntimeAttributes();
		sample.damage = ly::FindGameplayAttributeValue(attributes, ly::CommonAttributeIds::Damage, 0.f);
		sample.fireRate = ly::FindGameplayAttributeValue(attributes, ly::CommonAttributeIds::FireRate, 0.f);
		sample.attackPower = runtime.GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::AttackPower);
		sample.attackSpeed = runtime.GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::AttackSpeed);
		sample.energyMax = runtime.GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::EnergyMax);
		sample.luck = runtime.GetAttributes().GetCurrentValue(ly::OwnerAttributeIds::Luck);
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
		owner->GetCombatRuntime().GetAttributes().ApplyBaseModifier(
			ly::AttributeModifier{ ly::OwnerAttributeIds::Luck, luckRating }
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

		PrimaryWeaponDefinition definition = WeaponData::PrimaryWeapons::ElectricArcLauncher;
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
		owner->GetCombatRuntime().GetAttributes().ApplyBaseModifier(
			ly::AttributeModifier{ ly::OwnerAttributeIds::Luck, 100000.f }
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

		PrimaryWeaponDefinition definition = WeaponData::PrimaryWeapons::ElectricArcLauncher;
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

		const float baseDamage = ly::FindGameplayAttributeValue(
			definition.attributes,
			ly::CommonAttributeIds::Damage,
			0.f
		);
		const float falloff = ly::FindGameplayAttributeValue(
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
		owner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
			ly::AttributeModifier{ ly::OwnerAttributeIds::AttackSpeed, attackSpeed }
		);
		ly::PrimaryWeaponRuntimeState runtime;
		const PrimaryWeaponDefinition& definition = WeaponData::PrimaryWeapons::ContinuousHeatLaser;
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
		owner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
			ly::AttributeModifier{ ly::OwnerAttributeIds::AttackSpeed, attackSpeed }
		);
		ly::PrimaryWeaponRuntimeState runtime;
		const PrimaryWeaponDefinition& definition = WeaponData::PrimaryWeapons::ContinuousHeatLaser;
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
		owner->GetCombatRuntime().GetAttributes().ApplyBaseModifier(
			ly::AttributeModifier{ ly::OwnerAttributeIds::AttackSpeed, attackSpeed }
		);
		owner->SetCollisionLayer(CollisionLayer::Player);
		owner->SetActorRotation(90.f);
		target->SetCollisionLayer(CollisionLayer::Enemy);
		target->SetCollisionMask(CollisionLayer::PlayerBullet);
		target->SetActorLocation({ 400.f, 0.f });
		world.TickInternal(0.f);

		ly::PrimaryWeaponRuntimeState runtime;
		const PrimaryWeaponDefinition& definition = WeaponData::PrimaryWeapons::ContinuousHeatLaser;
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

	const ly::GameplayTag ExtensionWeaponType{
		"PrimaryWeapon.Test.Extension"
	};
	const ly::GameplayTag ExtensionWeaponAttributeRoot{
		"Attribute.PrimaryWeapon.Test.Extension"
	};
	const ly::GameplayTag ExtensionWeaponAttribute{
		"Attribute.PrimaryWeapon.Test.Extension.Value"
	};
	const ly::GameplayTag ExtensionFeatureTag{
		"PrimaryWeapon.Feature.Test.Extension"
	};
	const ly::GameplayTag ExtensionFeatureAttributeRoot{
		"Attribute.PrimaryWeapon.Feature.Test.Extension"
	};
	const ly::GameplayTag ExtensionFeatureAttribute{
		"Attribute.PrimaryWeapon.Feature.Test.Extension.Value"
	};
	const ly::GameplayTag ExtensionFeatureRuntimeValue{
		"Runtime.PrimaryWeapon.Feature.Test.Extension.Value"
	};

	class ExtensionPrimaryWeaponHandler final : public ly::PrimaryWeaponHandler
	{
	public:
		const ly::GameplayTag& GetTypeTag() const override
		{
			return ExtensionWeaponType;
		}

		const ly::List<ly::GameplayTag>& GetOwnedAttributeRoots() const override
		{
			static const ly::List<ly::GameplayTag> roots{
				ExtensionWeaponAttributeRoot
			};
			return roots;
		}

		void FireOnce(
			const ly::PrimaryWeaponExecutionContext&,
			ly::PrimaryWeaponTypeRuntimeState&
		) const override
		{
		}
	};

	class ExtensionPrimaryWeaponFeature final : public ly::PrimaryWeaponFeatureHandler
	{
	public:
		const ly::GameplayTag& GetFeatureTag() const override
		{
			return ExtensionFeatureTag;
		}

		const ly::List<ly::GameplayTag>& GetAttributeRoots() const override
		{
			static const ly::List<ly::GameplayTag> roots{
				ExtensionFeatureAttributeRoot
			};
			return roots;
		}

		const ly::List<ly::GameplayTag>& GetRuntimeValueKeys() const override
		{
			static const ly::List<ly::GameplayTag> keys{
				ExtensionFeatureRuntimeValue
			};
			return keys;
		}

		void AfterFire(
			const ly::PrimaryWeaponExecutionContext&,
			ly::PrimaryWeaponRuntimeState& state
		) const override
		{
			state.SetFeatureValue(ExtensionFeatureRuntimeValue, 1.f);
		}
	};
}

int main()
{
	using namespace ly;

	if (!RegisterGameGameplayEffectContent())
	{
		return Fail("Game gameplay-effect content registration failed");
	}
	if (!RegisterGameAbilityContent())
	{
		return Fail("Game ability content could not be registered");
	}
	std::string effectValidationFailure;
	const List<const GameplayEffectDefinition*>& shippedEffects =
		EffectData::GetShippedGameplayEffectDefinitions();
	if (shippedEffects.size() != 7 ||
		!ValidateShippedGameplayEffectDefinitions(&effectValidationFailure) ||
		EffectData::FindGameplayEffectDefinition("Effect.Barrier.Basic") !=
			&EffectData::BasicBarrierEffect ||
		EffectData::FindGameplayEffectDefinition(
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
		) != &EffectData::GravityAnomalyInsideEffect ||
		EffectData::FindGameplayEffectDefinition("Effect.Does.NotExist") != nullptr)
	{
		return Fail("Central gameplay-effect catalog lookup or validation failed");
	}

	GameplayEffectSpec firstSlowSpec =
		MakeGameplayEffectSpec(EffectData::CryoSlowEffect);
	GameplayEffectSpec secondSlowSpec =
		MakeGameplayEffectSpec(EffectData::CryoSlowEffect);
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

	GameplayEffectDefinition invalidEffect = EffectData::CryoSlowEffect;
	invalidEffect.effectId = "Effect.Test.InvalidBehavior";
	invalidEffect.behaviorTag = GameplayTag{ "EffectBehavior.NotRegistered" };
	effectValidationFailure.clear();
	if (ValidateGameplayEffectDefinition(invalidEffect, &effectValidationFailure) ||
		effectValidationFailure.empty())
	{
		return Fail("Gameplay-effect validation accepted an unregistered behavior");
	}
	invalidEffect = EffectData::CryoSlowEffect;
	invalidEffect.effectId = "Effect.Test.InvalidVisual";
	invalidEffect.activeVisualId = "Visual.Effect.NotRegistered";
	effectValidationFailure.clear();
	if (ValidateGameplayEffectDefinition(invalidEffect, &effectValidationFailure) ||
		effectValidationFailure.empty())
	{
		return Fail("Gameplay-effect validation accepted an unregistered visual");
	}

	const RocketPresentationProfile* rocketPresentationProfile =
		PresentationProfileRegistry<RocketPresentationProfile>::Find(
			RocketPresentationIds::Basic
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
	if (!rocketPresentationProfile
		|| !sunBeamPresentationProfile
		|| !gravityProjectilePresentationProfile
		|| !gravityFieldPresentationProfile
		|| rocketPresentationProfile->telegraph.outlineThickness <= 0.f
		|| rocketPresentationProfile->visual.impactVisualDuration <= 0.f
		|| sunBeamPresentationProfile->visual.impactFlashDuration <= 0.f)
	{
		return Fail("Feature-local ability presentation profiles were not registered correctly");
	}
	if (PresentationProfileRegistry<RocketPresentationProfile>::Find(
			SunBeamPresentationIds::StrikeBasic
		) != nullptr
		|| PresentationProfileRegistry<SunBeamPresentationProfile>::Find(
			RocketPresentationIds::Basic
		) != nullptr
		|| PresentationProfileRegistry<GravityAnomalyProjectilePresentationProfile>::Find(
			GravityAnomalyPresentationIds::FieldBasic
		) != nullptr
		|| PresentationProfileRegistry<GravityAnomalyFieldPresentationProfile>::Find(
			GravityAnomalyPresentationIds::ProjectileBasic
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

	AttributeSystem attributes;
	attributes.RegisterAttribute(OwnerAttributeIds::AttackPower, 10.f);
	const AttributeModifierHandle additive = attributes.AddModifier(
		AttributeModifier{ OwnerAttributeIds::AttackPower, AttributeModifierOperation::Add, 5.f }
	);
	const AttributeModifierHandle multiplicative = attributes.AddModifier(
		AttributeModifier{ OwnerAttributeIds::AttackPower, AttributeModifierOperation::Multiply, 2.f }
	);
	if (!NearlyEqual(attributes.GetCurrentValue(OwnerAttributeIds::AttackPower), 30.f))
	{
		return Fail("Attribute modifier order failed");
	}
	attributes.RemoveModifier(multiplicative);
	attributes.ApplyBaseModifier(AttributeModifier{ OwnerAttributeIds::AttackPower, 5.f });
	if (!NearlyEqual(attributes.GetCurrentValue(OwnerAttributeIds::AttackPower), 20.f))
	{
		return Fail("Instant base modifier failed");
	}
	attributes.RemoveModifier(additive);

	attributes.RegisterAttribute(OwnerAttributeIds::AbilityHaste, 0.f);
	attributes.AddModifier(AttributeModifier{ OwnerAttributeIds::AbilityHaste, 0.1f });
	attributes.AddModifier(AttributeModifier{ OwnerAttributeIds::AbilityHaste, 0.1f });
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
	tankEnergyAttributes.maxShieldPerMaxEnergy = 0.75f;
	tankEnergyAttributes.afterburnerCapacityPerMaxEnergy = 0.30f;
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
		PrimaryWeaponDefinition{},
		ShipMovementAttributes{},
		tankEnergyAttributes
	};
	TestCombatant energyTestCombatant;
	CombatRuntime& energyRuntime = energyTestCombatant.GetCombatRuntime();
	energyRuntime.InitializeOwnerAttributes(energyTestShip.health);
	ShipRuntime energyShipRuntime{ energyRuntime.GetAttributes() };
	energyShipRuntime.InitializeFromShipDefinition(energyTestShip);
	energyRuntime.GetAttributes().ApplyBaseModifier(AttributeModifier{ OwnerAttributeIds::EnergyMax, 100.f });
	const AttributeSystem& shipAttributes = energyShipRuntime.GetAttributes();
	if (!NearlyEqual(shipAttributes.GetCurrentValue(ShipAttributeIds::MaxShield), 175.f) ||
		!NearlyEqual(shipAttributes.GetCurrentValue(ShipAttributeIds::ShieldRegen), 35.f) ||
		!NearlyEqual(shipAttributes.GetCurrentValue(ShipAttributeIds::ShieldRechargeDelay), 4.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerCapacity(), 80.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerRegenPerSecond(), 10.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerRechargeDelay(), 2.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerEnergyDrainPerSecond(), 12.f) ||
		!NearlyEqual(energyRuntime.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyRegen), 10.f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerSpeedMultiplier(), 1.4f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerAccelerationMultiplier(), 1.8f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerRampUpDuration(), 0.18f) ||
		!NearlyEqual(energyShipRuntime.GetAfterburnerManeuverabilityMultiplier(), 0.8f))
	{
		return Fail("Ship energy attributes did not derive shield and afterburner values correctly");
	}
	energyRuntime.GetAttributes().ApplyBaseModifier(AttributeModifier{ OwnerAttributeIds::EnergyMax, 50.f });
	if (!NearlyEqual(energyShipRuntime.GetAfterburnerCapacity(), 95.f) ||
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

	if (!NearlyEqual(AttributeMath::GetArmorDamageReduction(50.f), 0.5f) ||
		AttributeMath::GetAbilityHasteReduction(500.f) >= 1.f ||
		AttributeMath::GetCriticalChance(500.f) >= 1.f ||
		AttributeMath::GetCombatLuckFactor(500.f) >= 1.f ||
		AttributeMath::GetCombatLuckFactor(20.f) <= AttributeMath::GetCombatLuckFactor(10.f))
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
			{ OwnerAttributeIds::EnergyMax, 0.5f },
			{ OwnerAttributeIds::MoveSpeedHorizontal, 0.5f },
			{ OwnerAttributeIds::MoveSpeedVertical, 0.5f }
		}
	};
	TestCombatant fighterProgressionCombatant;
	CombatRuntime& fighterRuntime = fighterProgressionCombatant.GetCombatRuntime();
	fighterRuntime.InitializeOwnerAttributes(energyTestShip.health);
	ShipRuntime fighterShipRuntime{ fighterRuntime.GetAttributes() };
	fighterShipRuntime.InitializeFromShipDefinition(energyTestShip);
	ShipProgression fighterProgression;
	fighterProgression.Configure(fighterProgressionDefinition);
	fighterProgression.BindAttributes(fighterRuntime.GetAttributes());
	fighterProgression.AddXP(100.f);
	const AttributeSystem& fighterAttributes = fighterRuntime.GetAttributes();
	if (fighterProgression.GetLevel() != 2 ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::AttackPower), 9.f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::AttackSpeed), 1.f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::CriticalChance), 0.7f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::Luck), 0.075f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::AbilityHaste), 0.125f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::EnergyMax), 1.f) ||
		!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::HealthRegen), 108.f / 1200.f))
	{
		return Fail("Fighter level growth did not apply its configured and default multipliers");
	}
	fighterProgression.BindAttributes(fighterRuntime.GetAttributes());
	if (!NearlyEqual(fighterAttributes.GetCurrentValue(OwnerAttributeIds::AttackPower), 9.f))
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

	ActiveGameplayEffect barrier;
	barrier.spec.definition.effectId = "Effect.Test.Barrier";
	barrier.spec.definition.behaviorTag = BarrierEffectSchema::BehaviorId;
	barrier.spec.attributes = {
		GameplayAttribute{ BarrierEffectSchema::Capacity, 30.f, 0.f },
		GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f }
	};
	GameplayEffectBehavior::Initialize(barrier);

	DamageContext firstHit;
	firstHit.originalDamage = 12.f;
	firstHit.remainingDamage = 12.f;
	const GameplayEffectBehaviorResult firstResult =
		GameplayEffectBehavior::ProcessIncomingDamage(barrier, firstHit);
	if (!firstResult.changed || firstResult.removeEffect ||
		!NearlyEqual(firstHit.remainingDamage, 0.f) ||
		!NearlyEqual(FindGameplayAttributeValue(
			barrier.runtimeAttributes,
			BarrierEffectSchema::Capacity
		), 18.f))
	{
		return Fail("Barrier partial absorption failed");
	}

	DamageContext breakingHit;
	breakingHit.originalDamage = 20.f;
	breakingHit.remainingDamage = 20.f;
	const GameplayEffectBehaviorResult breakResult =
		GameplayEffectBehavior::ProcessIncomingDamage(barrier, breakingHit);
	if (!breakResult.removeEffect || breakResult.events.empty() ||
		!NearlyEqual(breakingHit.remainingDamage, 2.f))
	{
		return Fail("Barrier break behavior failed");
	}

	GameplayEffectBehavior::Refresh(barrier);
	GameplayEffectBehavior::AddStack(barrier);
	if (!NearlyEqual(FindGameplayAttributeValue(
		barrier.runtimeAttributes,
		BarrierEffectSchema::Capacity
	), 60.f))
	{
		return Fail("Barrier stack behavior failed");
	}

	const DamagePayload energyPayload = DamageTypeSystem::BuildPayload({ DamageTypeSchema::Energy });
	if (!NearlyEqual(energyPayload.shieldDamageMultiplier, 1.25f))
	{
		return Fail("Energy damage payload was not resolved");
	}
	if (!NearlyEqual(energyPayload.shieldRegenerationDelay, 0.75f))
	{
		return Fail("Energy damage shield regeneration delay was not resolved");
	}

	GameplayEffectDefinition regeneratingBarrier;
	regeneratingBarrier.effectId = "Effect.Test.RegeneratingBarrier";
	regeneratingBarrier.behaviorTag = BarrierEffectSchema::BehaviorId;
	regeneratingBarrier.durationPolicy = GameplayEffectDurationPolicy::Infinite;
	regeneratingBarrier.attributes = {
		GameplayAttribute{ BarrierEffectSchema::Capacity, 10.f, 0.f },
		GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f },
		GameplayAttribute{ BarrierEffectSchema::RegenerationPerSecond, 5.f, 0.f },
		GameplayAttribute{ BarrierEffectSchema::RegenerationDelay, 1.f, 0.f },
		GameplayAttribute{ BarrierEffectSchema::RegenerationDelayRemaining, 0.f, 0.f }
	};
	TestCombatant regenerationTarget;
	const GameplayEffectHandle regenerationHandle = regenerationTarget.GetCombatRuntime().GetEffects().ApplyEffect(
		regeneratingBarrier
	);
	ApplyCombatDamage(regenerationTarget, 2.f, nullptr, { DamageTypeSchema::Energy }, energyPayload);
	regenerationTarget.GetCombatRuntime().Tick(1.5f);
	const ActiveGameplayEffect* delayedBarrier = regenerationTarget.GetCombatRuntime().GetEffects().FindEffect(regenerationHandle);
	if (!delayedBarrier || !NearlyEqual(
		FindGameplayAttributeValue(delayedBarrier->runtimeAttributes, BarrierEffectSchema::Capacity),
		7.5f
	))
	{
		return Fail("Energy damage did not delay shield regeneration");
	}
	regenerationTarget.GetCombatRuntime().Tick(0.5f);
	const ActiveGameplayEffect* regeneratingBarrierState = regenerationTarget.GetCombatRuntime().GetEffects().FindEffect(regenerationHandle);
	if (!regeneratingBarrierState || !NearlyEqual(
		FindGameplayAttributeValue(regeneratingBarrierState->runtimeAttributes, BarrierEffectSchema::Capacity),
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
	GameplayAttributeList energyAttachmentAttributes = energyAttachmentLoadout.MergeGrantedAttributes(
		AttachmentHostKind::PrimaryWeapon,
		{ GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f } }
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
		!NearlyEqual(FindGameplayAttributeValue(energyAttachmentAttributes, CommonAttributeIds::Damage), 11.5f) ||
		!NearlyEqual(attachmentEnergyPayload.shieldRegenerationDelay, 0.75f))
	{
		return Fail("Damage type attachment did not resolve its converted tag and conditional attributes");
	}
	GameplayEffectDefinition smallShield;
	smallShield.effectId = "Effect.Test.SmallShield";
	smallShield.behaviorTag = BarrierEffectSchema::BehaviorId;
	smallShield.durationPolicy = GameplayEffectDurationPolicy::Infinite;
	smallShield.attributes = {
		GameplayAttribute{ BarrierEffectSchema::Capacity, 5.f, 0.f },
		GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f }
	};
	TestCombatant shieldTarget;
	shieldTarget.GetCombatRuntime().GetEffects().ApplyEffect(smallShield);
	ApplyCombatDamage(shieldTarget, 8.f, nullptr, { DamageTypeSchema::Energy }, energyPayload);
	if (!NearlyEqual(shieldTarget.GetHealth(), 96.f))
	{
		return Fail("Energy damage did not apply its shield multiplier correctly");
	}

	TestCombatant armoredTarget;
	armoredTarget.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
		AttributeModifier{ OwnerAttributeIds::Armor, AttributeModifierOperation::Override, 50.f }
	);
	const DamagePayload kineticPayload = DamageTypeSystem::BuildPayload({ DamageTypeSchema::Kinetic });
	if (!NearlyEqual(kineticPayload.armorPenetration, 0.10f))
	{
		return Fail("Kinetic armor penetration was not reduced");
	}
	ApplyCombatDamage(armoredTarget, 10.f, nullptr, { DamageTypeSchema::Kinetic }, kineticPayload);
	const float effectiveKineticReduction =
		AttributeMath::GetArmorDamageReduction(50.f) * (1.f - kineticPayload.armorPenetration);
	const float expectedKineticHealth = 100.f - 10.f * (1.f - effectiveKineticReduction);
	if (!NearlyEqual(armoredTarget.GetHealth(), expectedKineticHealth))
	{
		return Fail("Kinetic armor penetration was not applied");
	}

	TestCombatant thermalTarget;
	const DamagePayload thermalPayload = DamageTypeSystem::BuildPayload({ DamageTypeSchema::Thermal });
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

	TestCombatant cryoTarget;
	const DamagePayload cryoPayload = DamageTypeSystem::BuildPayload({ DamageTypeSchema::Cryo });
	if (cryoPayload.cryoBuildupRequired != 4 ||
		!NearlyEqual(cryoPayload.cryoSlowPercent, 0.25f) ||
		!NearlyEqual(cryoPayload.cryoSlowDuration, 1.5f))
	{
		return Fail("Cryo damage payload was not resolved to the four-hit profile");
	}
	for (int expectedStacks = 1; expectedStacks < 4; ++expectedStacks)
	{
		ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
		const ActiveGameplayEffect* buildup =
			cryoTarget.GetCombatRuntime().GetEffects().FindEffectById(
				DamageStatusEffectIds::CryoBuildup
			);
		if (!buildup || buildup->stackCount != expectedStacks ||
			!NearlyEqual(
				cryoTarget.GetCombatRuntime().GetAttributes().GetCurrentValue(
					OwnerAttributeIds::MovementSlow
				),
				0.f
			))
		{
			return Fail("Cryo slowed before its four-hit buildup was complete");
		}
	}
	ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
	if (!NearlyEqual(
		cryoTarget.GetCombatRuntime().GetAttributes().GetCurrentValue(OwnerAttributeIds::MovementSlow),
		0.25f
	) || cryoTarget.GetCombatRuntime().GetEffects().FindEffectById(
		DamageStatusEffectIds::CryoBuildup
	) || !cryoTarget.GetCombatRuntime().GetEffects().FindEffectById(
		DamageStatusEffectIds::CryoSlowed
	))
	{
		return Fail("Four-hit Cryo buildup did not consume stacks and apply slow");
	}
	ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
	if (cryoTarget.GetCombatRuntime().GetEffects().FindEffectById(
		DamageStatusEffectIds::CryoBuildup
	))
	{
		return Fail("Cryo buildup accumulated while the slow was active");
	}
	cryoTarget.GetCombatRuntime().Tick(1.f);
	ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
	cryoTarget.GetCombatRuntime().Tick(0.75f);
	if (!NearlyEqual(
		cryoTarget.GetCombatRuntime().GetAttributes().GetCurrentValue(OwnerAttributeIds::MovementSlow),
		0.25f
	))
	{
		return Fail("Cryo hit did not refresh the active slow duration");
	}
	cryoTarget.GetCombatRuntime().Tick(0.8f);
	if (!NearlyEqual(
		cryoTarget.GetCombatRuntime().GetAttributes().GetCurrentValue(OwnerAttributeIds::MovementSlow),
		0.f
	))
	{
		return Fail("Cryo slow did not expire");
	}
	ApplyCombatDamage(cryoTarget, 1.f, nullptr, { DamageTypeSchema::Cryo }, cryoPayload);
	cryoTarget.GetCombatRuntime().Tick(2.6f);
	if (cryoTarget.GetCombatRuntime().GetEffects().FindEffectById(
		DamageStatusEffectIds::CryoBuildup
	))
	{
		return Fail("Incomplete Cryo buildup did not expire");
	}


	TestCombatant electricTarget;
	const DamagePayload electricPayload = DamageTypeSystem::BuildPayload({ DamageTypeSchema::Electric });
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
	projectileWeapon.weaponTypeTag = PrimaryWeaponSchema::Projectile::Standard::TypeId;
	projectileWeapon.attributes = {
		GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.f, 0.f }
	};
	if (!PrimaryWeaponExecutionSystem::ValidateDefinition(projectileWeapon).isValid)
	{
		return Fail("Valid projectile weapon was rejected");
	}
	if (!PrimaryWeaponHandlerRegistry::RegisterHandler(
			std::make_unique<ExtensionPrimaryWeaponHandler>()
		) ||
		!PrimaryWeaponHandlerRegistry::RegisterFeature(
			std::make_unique<ExtensionPrimaryWeaponFeature>()
		))
	{
		return Fail("Primary weapon registry rejected valid extension handlers");
	}
	PrimaryWeaponDefinition extensionWeapon;
	extensionWeapon.weaponId = "Weapon.Test.Extension";
	extensionWeapon.weaponTypeTag = ExtensionWeaponType;
	extensionWeapon.attributes = {
		GameplayAttribute{ CommonAttributeIds::Damage, 1.f, 0.f },
		GameplayAttribute{ ExtensionWeaponAttribute, 1.f, 0.f },
		GameplayAttribute{ ExtensionFeatureAttribute, 1.f, 0.f }
	};
	extensionWeapon.featureTags = { ExtensionFeatureTag };
	if (!PrimaryWeaponExecutionSystem::ValidateDefinition(extensionWeapon).isValid)
	{
		return Fail("Primary weapon registry extension route failed validation");
	}
	PrimaryWeaponRuntimeState extensionRuntime;
	if (!PrimaryWeaponExecutionSystem::InitializeRuntime(
			extensionWeapon,
			extensionRuntime
		).isValid ||
		extensionRuntime.features.size() != 1)
	{
		return Fail("Primary weapon registry extension route failed runtime setup");
	}
	Actor extensionOwner{ nullptr };
	const PrimaryWeaponExecutionContext extensionContext{
		extensionOwner,
		extensionWeapon,
		extensionWeapon.attributes,
		{}
	};
	PrimaryWeaponExecutionSystem::BeginFire(extensionContext, extensionRuntime);
	if (!PrimaryWeaponExecutionSystem::FireOnce(extensionContext, extensionRuntime) ||
		!NearlyEqual(
			extensionRuntime.GetFeatureValue(ExtensionFeatureRuntimeValue),
			1.f
		))
	{
		return Fail("Registered primary weapon extension handlers did not execute");
	}
	PrimaryWeaponExecutionSystem::EndFire(extensionContext, extensionRuntime);

	PrimaryWeaponDefinition electricArcWeapon;
	electricArcWeapon.weaponTypeTag = PrimaryWeaponSchema::Arc::Electric::TypeId;
	electricArcWeapon.attributes = {
		GameplayAttribute{ CommonAttributeIds::Damage, 12.f, 0.f },
		GameplayAttribute{ CommonAttributeIds::Range, 700.f, 1.f },
		GameplayAttribute{ PrimaryWeaponSchema::Arc::Electric::ChainCount, 2.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Arc::Electric::ChainRange, 240.f, 1.f },
		GameplayAttribute{
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
	FindGameplayAttribute(
		invalidElectricArcWeapon.attributes,
		PrimaryWeaponSchema::Arc::Electric::DamageMultiplierPerChain
	)->baseValue = 1.1f;
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidElectricArcWeapon).isValid)
	{
		return Fail("Arc projectile accepted an invalid chain damage multiplier");
	}
	PrimaryWeaponDefinition projectileFamilyDefinition = projectileWeapon;
	projectileFamilyDefinition.weaponTypeTag = PrimaryWeaponSchema::Projectile::FamilyId;
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(projectileFamilyDefinition).isValid)
	{
		return Fail("A primary weapon family tag was accepted as a concrete weapon type");
	}
	const AbilityDefinition automaticPrimaryAbility = AbilityData::MakePrimaryFireAbilityDefinition(projectileWeapon);
	PrimaryWeaponDefinition semiAutomaticWeapon = projectileWeapon;
	semiAutomaticWeapon.automaticFire = false;
	const AbilityDefinition semiAutomaticPrimaryAbility = AbilityData::MakePrimaryFireAbilityDefinition(semiAutomaticWeapon);
	if (automaticPrimaryAbility.activationPolicy != AbilityActivationPolicy::WhileHeld ||
		automaticPrimaryAbility.actions.empty() ||
		automaticPrimaryAbility.actions.front().phase != AbilityActionPhase::WhileActive ||
		semiAutomaticPrimaryAbility.activationPolicy != AbilityActivationPolicy::OnPressed ||
		semiAutomaticPrimaryAbility.actions.empty() ||
		semiAutomaticPrimaryAbility.actions.front().phase != AbilityActionPhase::OnActivate)
	{
		return Fail("Primary weapon automatic fire policy was not generated correctly");
	}

	PrimaryWeaponDefinition mixedWeapon = projectileWeapon;
	mixedWeapon.attributes.push_back(
		GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Width, 20.f, 0.f }
	);
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(mixedWeapon).isValid)
	{
		return Fail("Mixed projectile and beam attributes were accepted");
	}
	PrimaryWeaponDefinition standardProjectileWithShotgunAttribute = projectileWeapon;
	standardProjectileWithShotgunAttribute.attributes.push_back(
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::PelletCount, 3.f, 1.f }
	);
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(standardProjectileWithShotgunAttribute).isValid)
	{
		return Fail("Standard projectile accepted a shotgun-only attribute");
	}

	PrimaryWeaponDefinition shotgunWeapon;
	shotgunWeapon.weaponTypeTag = PrimaryWeaponSchema::Projectile::Shotgun::TypeId;
	shotgunWeapon.attributes = {
		GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::PelletCount, 5.f, 1.f },
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle, 42.f, 0.f }
	};
	if (!PrimaryWeaponExecutionSystem::ValidateDefinition(shotgunWeapon).isValid)
	{
		return Fail("Valid shotgun weapon was rejected");
	}
	if (!shotgunWeapon.weaponTypeTag.MatchesTag(PrimaryWeaponSchema::Projectile::FamilyId))
	{
		return Fail("Shotgun type tag is not nested under the projectile family");
	}

	PrimaryWeaponDefinition invalidShotgunWeapon = shotgunWeapon;
	invalidShotgunWeapon.attributes.erase(invalidShotgunWeapon.attributes.begin() + 3);
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidShotgunWeapon).isValid)
	{
		return Fail("Shotgun without pellet count was accepted");
	}

	PrimaryWeaponDefinition continuousBeamWeapon;
	continuousBeamWeapon.weaponId = "TestContinuousHeatBeam";
	continuousBeamWeapon.weaponTypeTag = PrimaryWeaponSchema::Beam::Continuous::TypeId;
	continuousBeamWeapon.attributes = {
		GameplayAttribute{ CommonAttributeIds::Damage, 20.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Range, 800.f, 1.f },
		GameplayAttribute{ PrimaryWeaponSchema::Beam::Delivery::Width, 20.f, 1.f },
		GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Gain, 50.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Capacity, 100.f, 1.f },
		GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Dissipation, 20.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::OverheatCooldown, 2.5f, 0.1f },
		GameplayAttribute{
			PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat,
			1.5f,
			1.f
		}
	};
	continuousBeamWeapon.featureTags = { PrimaryWeaponSchema::Feature::Heat::FeatureId };
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
		GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Capacity, 100.f, 0.f }
	);
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(undeclaredFeatureWeapon).isValid)
	{
		return Fail("An undeclared feature attribute was accepted");
	}

	PrimaryWeaponDefinition heatedProjectileWeapon = projectileWeapon;
	heatedProjectileWeapon.featureTags = { PrimaryWeaponSchema::Feature::Heat::FeatureId };
	heatedProjectileWeapon.attributes.push_back(
		GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Gain, 3.f, 0.f }
	);
	heatedProjectileWeapon.attributes.push_back(
		GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Capacity, 10.f, 0.f }
	);
	heatedProjectileWeapon.attributes.push_back(
		GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Dissipation, 2.f, 0.f }
	);
	PrimaryWeaponRuntimeState heatedRuntime;
	if (!PrimaryWeaponExecutionSystem::InitializeRuntime(heatedProjectileWeapon, heatedRuntime).isValid)
	{
		return Fail("Valid heat feature was rejected");
	}
	Actor heatTestOwner{ nullptr };
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
	invalidRuntimeReplacement.weaponTypeTag = GameplayTag{
		"PrimaryWeapon.Test.MissingHandler"
	};
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

	const GameplayTag testWeaponUpgrade{ "PrimaryWeapon.Upgrade.Test.DamagePulse" };
	PrimaryWeaponDefinition progressiveHeatWeapon = heatedProjectileWeapon;
	progressiveHeatWeapon.weaponId = "ProgressiveHeatWeapon";
	progressiveHeatWeapon.featureTags.clear();
	progressiveHeatWeapon.progressionProfile = WeaponProgressionProfile{ 3 }
		.AtLevel(
			2,
			{ AttributeModifier{ CommonAttributeIds::Damage, AttributeModifierOperation::Add, 5.f } },
			{ testWeaponUpgrade }
		)
		.AtLevel(
			3,
			{ AttributeModifier{
				PrimaryWeaponSchema::Feature::Heat::Capacity,
				AttributeModifierOperation::Add,
				5.f
			} },
			{},
			{ PrimaryWeaponSchema::Feature::Heat::FeatureId }
		);
	if (!PrimaryWeaponExecutionSystem::ValidateDefinition(progressiveHeatWeapon).isValid)
	{
		return Fail("A primary weapon with level-specific upgrades and features was rejected");
	}

	const AbilityDefinition progressivePrimaryAbility =
		AbilityData::MakePrimaryFireAbilityDefinition(progressiveHeatWeapon);
	if (progressivePrimaryAbility.GetMaxLevel() != 3 ||
		progressivePrimaryAbility.levelProgression.size() != 2 ||
		progressivePrimaryAbility.levelProgression[0].unlockedUpgradeIds.size() != 1 ||
		progressivePrimaryAbility.levelProgression[0].unlockedUpgradeIds.front() != testWeaponUpgrade ||
		progressivePrimaryAbility.levelProgression[1].unlockedUpgradeIds.size() != 1 ||
		progressivePrimaryAbility.levelProgression[1].unlockedUpgradeIds.front() !=
			PrimaryWeaponSchema::Feature::Heat::FeatureId)
	{
		return Fail("Primary weapon progression was not preserved during ability conversion");
	}

	const WeaponProgressionProfile flexibleWeaponProfile = WeaponProgressionProfile{ 7 }
		.EveryLevel({ AttributeModifier{ CommonAttributeIds::Damage, 1.f } })
		.BetweenLevels(3, 5, { AttributeModifier{ CommonAttributeIds::FireRate, 0.25f } })
		.AtLevel(6, { AttributeModifier{ PrimaryWeaponSchema::Projectile::Delivery::PierceCount, 1.f } })
		.FromLevel(6, { AttributeModifier{ CommonAttributeIds::Range, 50.f } });
	const List<PrimaryWeaponLevelStep> flexibleWeaponSteps = flexibleWeaponProfile.ResolveLevelSteps();
	const auto stepHasModifier = [](const PrimaryWeaponLevelStep& step, const GameplayTag& attributeId, float value)
	{
		for (const AttributeModifier& modifier : step.attributeModifiers)
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

	const List<GameplayTag> unlockedWeaponUpgrades{
		testWeaponUpgrade,
		PrimaryWeaponSchema::Feature::Heat::FeatureId
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

	Actor liveProgressionOwner{ nullptr };
	AttributeSystem liveProgressionAttributes;
	GameplayTagContainer liveProgressionTags;
	GameplayEffectSystem liveProgressionEffects{
		liveProgressionOwner,
		liveProgressionAttributes,
		liveProgressionTags
	};
	AbilitySystem liveProgressionAbilities{
		liveProgressionOwner,
		liveProgressionAttributes,
		liveProgressionEffects,
		liveProgressionTags
	};
	const AbilityHandle liveProgressionHandle =
		liveProgressionAbilities.GrantAbility(progressivePrimaryAbility);
	if (!liveProgressionHandle.IsValid())
	{
		return Fail("Live primary weapon progression ability could not be granted");
	}
	liveProgressionAbilities.SetSlotInput(AbilitySlot::PrimaryFire, true);
	liveProgressionAbilities.Tick(0.f);
	AbilityInstance* liveProgressionInstance =
		liveProgressionAbilities.GetAbility(liveProgressionHandle);
	if (!liveProgressionInstance ||
		!liveProgressionInstance->GetPrimaryWeaponRuntime().features.empty())
	{
		return Fail("Live primary weapon runtime activated a locked feature");
	}
	liveProgressionAbilities.SetSlotInput(AbilitySlot::PrimaryFire, false);
	liveProgressionAbilities.Tick(0.f);
	if (!liveProgressionAbilities.TrySetAbilityLevel(
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
	liveProgressionAbilities.SetSlotInput(AbilitySlot::PrimaryFire, true);
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
	liveProgressionAbilities.SetSlotInput(AbilitySlot::PrimaryFire, false);
	liveProgressionAbilities.Tick(0.f);
	if (!liveProgressionAbilities.TrySetAbilityLevel(liveProgressionHandle, 1))
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
			AttributeModifier{ CommonAttributeIds::AreaRadius, AttributeModifierOperation::Add, 10.f }
		});
	if (PrimaryWeaponExecutionSystem::ValidateDefinition(invalidLevelTargetWeapon).isValid)
	{
		return Fail("A primary weapon level modifier targeting an undeclared attribute was accepted");
	}

	AbilityActorDefinition genericActorDefinition{
		"Actor.Test.Generic",
		AbilityActorSchema::Generic::TypeId,
		"",
		1.f,
		0.f,
		{}
	};
	if (!AbilityActorRegistry::ValidateDefinition(genericActorDefinition).isValid)
	{
		return Fail("Generic ability actor definition was rejected");
	}
	AbilityActorDefinition invalidGenericActorDefinition = genericActorDefinition;
	invalidGenericActorDefinition.attributes = {
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f }
	};
	if (AbilityActorRegistry::ValidateDefinition(invalidGenericActorDefinition).isValid)
	{
		return Fail("Generic ability actor accepted another mechanic family's attribute");
	}
	AbilityActorDefinition unknownActorDefinition = genericActorDefinition;
	unknownActorDefinition.actorTypeTag = GameplayTag{ "AbilityActor.Unknown" };
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
		!AbilityActorRegistry::ValidateDefinition(AbilityData::AbilityActors::Actor_SunBeam_Basic).isValid)
	{
		return Fail("Configured Sun Beam strike actor failed validation");
	}
	AbilityActorDefinition missingSunBeamVisual = AbilityData::AbilityActors::Actor_SunBeam_Basic;
	missingSunBeamVisual.presentationProfileId.clear();
	if (AbilityActorRegistry::ValidateDefinition(missingSunBeamVisual).isValid)
	{
		return Fail("Sun Beam strike actor accepted a missing presentation profile");
	}
	World sunBeamSpawnWorld{ nullptr };
	Actor sunBeamOwner{ &sunBeamSpawnWorld };
	weak_ptr<AbilityWorldActor> spawnedSunBeam = AbilityActorRegistry::Spawn(
		AbilityActorSpawnContext{
			sunBeamOwner,
			AbilityData::AbilityActors::Actor_SunBeam_Basic,
			AbilityData::AbilityActors::Actor_SunBeam_Basic.attributes
		}
	);
	shared_ptr<AbilityWorldActor> spawnedSunBeamActor = spawnedSunBeam.lock();
	if (!spawnedSunBeamActor)
	{
		return Fail("Configured Sun Beam strike actor failed to spawn");
	}
	spawnedSunBeamActor->SetActorLocation({ 320.f, 240.f });
	spawnedSunBeamActor->ConfigureFromAttributes(AbilityData::AbilityActors::Actor_SunBeam_Basic.attributes);
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

	auto SetTestAttribute = [](GameplayAttributeList& testAttributes, const GameplayTag& id, float value)
	{
		if (GameplayAttribute* attribute = FindGameplayAttribute(testAttributes, id))
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

	GameplayAttributeList timingAttributes = AbilityData::AbilityActors::Actor_SunBeam_Basic.attributes;
	SetTestAttribute(timingAttributes, CommonAttributeIds::Damage, 10.f);
	SetTestAttribute(
		timingAttributes,
		AbilityData::SunBeam::ActorSchema::Strike::TelegraphDuration,
		0.1f
	);
	SetTestAttribute(
		timingAttributes,
		AbilityData::SunBeam::ActorSchema::Strike::ArrivalDuration,
		0.1f
	);
	SetTestAttribute(
		timingAttributes,
		AbilityData::SunBeam::ActorSchema::Strike::ImpactDelay,
		0.05f
	);
	SetTestAttribute(
		timingAttributes,
		AbilityData::SunBeam::ActorSchema::Strike::ImpactVisualDuration,
		0.1f
	);
	weak_ptr<AbilityWorldActor> timedBeamWeak = AbilityActorRegistry::Spawn(
		AbilityActorSpawnContext{
			timingOwner,
			AbilityData::AbilityActors::Actor_SunBeam_Basic,
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
				AbilityData::AbilityActors::Actor_SunBeam_Basic,
				AbilityData::AbilityActors::Actor_SunBeam_Basic.attributes
			}
		);
		if (shared_ptr<AbilityWorldActor> beam = stressBeam.lock())
		{
			beam->SetActorLocation({ 200.f + beamIndex * 3.f, 200.f });
			beam->ConfigureFromAttributes(AbilityData::AbilityActors::Actor_SunBeam_Basic.attributes);
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
	AttributeSystem abilityAttributes;
	GameplayTagContainer abilityTags;
	GameplayEffectSystem abilityEffects{ abilityOwner, abilityAttributes, abilityTags };
	AbilitySystem abilitySystem{ abilityOwner, abilityAttributes, abilityEffects, abilityTags };
	const AbilityDefinition persistentHeatAbility = AbilityData::MakePrimaryFireAbilityDefinition(continuousBeamWeapon);
	const AbilityHandle persistentHeatHandle = abilitySystem.GrantAbility(persistentHeatAbility);
	if (!persistentHeatHandle.IsValid())
	{
		return Fail("Continuous heat primary ability could not be granted");
	}
	abilitySystem.SetSlotInput(AbilitySlot::PrimaryFire, true);
	abilitySystem.Tick(1.f);
	AbilityInstance* persistentHeatInstance = abilitySystem.GetAbility(AbilitySlot::PrimaryFire);
	if (!persistentHeatInstance || !NearlyEqual(persistentHeatInstance->GetPrimaryWeaponRuntime().GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 50.f))
	{
		return Fail("Continuous heat was not retained by the primary ability runtime");
	}
	abilitySystem.SetSlotInput(AbilitySlot::PrimaryFire, false);
	abilitySystem.Tick(1.f);
	if (!NearlyEqual(persistentHeatInstance->GetPrimaryWeaponRuntime().GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 30.f))
	{
		return Fail("Released continuous heat did not dissipate through the ability runtime");
	}
	abilitySystem.SetSlotInput(AbilitySlot::PrimaryFire, true);
	abilitySystem.Tick(0.f);
	if (!NearlyEqual(persistentHeatInstance->GetPrimaryWeaponRuntime().GetFeatureValue(
		PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
	), 30.f))
	{
		return Fail("Reactivated continuous heat did not preserve its partially cooled value");
	}
	abilitySystem.RemoveAbility(persistentHeatHandle);
	AbilityDefinition invalidSpawnActorAbility;
	invalidSpawnActorAbility.abilityId = "Ability.Test.InvalidSpawnActor";
	invalidSpawnActorAbility.slot = AbilitySlot::Ability1;
	invalidSpawnActorAbility.actions = {
		AbilityActionSpec{
			AbilityActionPhase::OnActivate,
			SpawnActorAction{ "Actor.Test.Unknown", AbilitySpawnPolicy::AtOwner },
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
	AbilityDefinition invalidEffectAbility;
	invalidEffectAbility.abilityId = "Ability.Test.InvalidEffect";
	invalidEffectAbility.slot = AbilitySlot::Ability1;
	invalidEffectAbility.actions = {
		AbilityActionSpec{
			AbilityActionPhase::OnActivate,
			ApplyEffectAction{
				"Effect.Test.Unknown",
				AbilityTargetPolicy::Self
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
	if (!abilitySystem.GrantAbility(AbilityData::Definitions::SunBeam_Strike_Basic, &sunBeamGrantFailure).IsValid())
	{
		return Fail("Configured Sun Beam ability failed grant validation");
	}
	if (AbilityData::Definitions::SunBeam_Strike_Basic.GetMaxLevel() != 5 ||
		AbilityData::Definitions::SunBeam_Strike_Basic.levelProgression.size() != 4)
	{
		return Fail("Repeated ability level progression produced the wrong level count");
	}

	const GameplayTag firstUpgradeId{ "Upgrade.Ability.Test.First" };
	const GameplayTag secondUpgradeId{ "Upgrade.Ability.Test.Second" };
	AbilityDefinition progressionAbility;
	progressionAbility.abilityId = "Ability.Test.Progression";
	progressionAbility.slot = AbilitySlot::Ability3;
	progressionAbility.duration = 4.f;
	progressionAbility.attributeModifiers = {
		AttributeModifier{ CommonAttributeIds::Radius, AttributeModifierOperation::Add, 1.f }
	};
	progressionAbility.levelProgression = {
		AbilityLevelStep{
			{
				AttributeModifier{ CommonAttributeIds::Damage, AttributeModifierOperation::Add, 5.f }
			},
			{ firstUpgradeId },
			{
				AbilityActionSpec{
					AbilityActionPhase::OnActivate,
					ApplyImpulseAction{ 10.f, AbilityDirectionPolicy::OwnerForward },
					0.f,
					1
				}
			},
			{}
		},
		AbilityLevelStep{
			{
				AttributeModifier{ CommonAttributeIds::Damage, AttributeModifierOperation::Multiply, 2.f },
				AttributeModifier{ CommonAttributeIds::Duration, AttributeModifierOperation::Add, 2.f }
			},
			{ secondUpgradeId },
			{},
			{
				AbilityTriggerSpec{
					GameplayTag{ "Event.Test.Progression" },
					0.f,
					{},
					{},
					{
						AbilityActionSpec{
							AbilityActionPhase::OnActivate,
							ApplyImpulseAction{ 5.f, AbilityDirectionPolicy::OwnerForward },
							0.f,
							1
						}
					}
				}
			}
		}
	};
	const AbilityHandle progressionHandle = abilitySystem.GrantAbility(progressionAbility);
	AbilityInstance* progressionInstance = abilitySystem.GetAbility(progressionHandle);
	if (!progressionInstance)
	{
		return Fail("Per-ability level progression failed to grant");
	}
	if (!abilitySystem.TrySetAbilityLevel(progressionHandle, 999))
	{
		return Fail("AbilitySystem failed to set an ability level");
	}
	const AbilityDefinition& maxLevelDefinition = progressionInstance->GetDefinition();
	const AbilityRuntimeSnapshot maxLevelSnapshot = progressionInstance->BuildSnapshot();
	const float progressedDamage = CalculateModifiedAttributeValue(
		GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
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
	if (abilitySystem.TryLevelUpAbility(progressionHandle))
	{
		return Fail("AbilitySystem leveled an ability beyond its maximum level");
	}
	if (!abilitySystem.TrySetAbilityLevel(AbilitySlot::Ability3, 1))
	{
		return Fail("AbilitySystem failed to restore an ability level");
	}
	const AbilityDefinition& resetDefinition = progressionInstance->GetDefinition();
	if (resetDefinition.attributeModifiers.size() != 1 ||
		!resetDefinition.unlockedUpgradeIds.empty() ||
		!resetDefinition.actions.empty() ||
		!resetDefinition.triggers.empty() ||
		!NearlyEqual(progressionInstance->GetActiveDuration(), 4.f))
	{
		return Fail("Ability level reset did not rebuild from the base definition");
	}
	if (!abilitySystem.TryLevelUpAbility(AbilitySlot::Ability3) || progressionInstance->GetLevel() != 2)
	{
		return Fail("AbilitySystem failed to level up an ability");
	}

	AbilityDefinition invalidProgressionAbility;
	invalidProgressionAbility.abilityId = "Ability.Test.InvalidProgression";
	invalidProgressionAbility.slot = AbilitySlot::Ability4;
	invalidProgressionAbility.levelProgression = {
		AbilityLevelStep{
			{},
			{ GameplayTag{ "Upgrade.Ability.Test.Invalid" } },
			{
				AbilityActionSpec{
					AbilityActionPhase::OnActivate,
					SpawnActorAction{ "Actor.Test.Unknown", AbilitySpawnPolicy::AtOwner },
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
	AbilityDefinition passiveOne;
	passiveOne.abilityId = "Ability.Test.Passive.One";
	passiveOne.slot = AbilitySlot::None;
	passiveOne.activationPolicy = AbilityActivationPolicy::Passive;
	passiveOne.lifetimePolicy = AbilityLifetimePolicy::UntilCancelled;
	AbilityDefinition passiveTwo = passiveOne;
	passiveTwo.abilityId = "Ability.Test.Passive.Two";
	AbilityDefinition passiveThree = passiveOne;
	passiveThree.abilityId = "Ability.Test.Passive.Three";
	const AbilityHandle passiveOneHandle = abilitySystem.GrantAbility(passiveOne);
	const AbilityHandle passiveTwoHandle = abilitySystem.GrantAbility(passiveTwo);
	const AbilityHandle passiveThreeHandle = abilitySystem.GrantAbility(passiveThree);
	if (!passiveOneHandle.IsValid() || !passiveTwoHandle.IsValid() || passiveThreeHandle.IsValid() ||
		abilitySystem.GetPassiveAbilityCount() != AbilitySystem::MaxPassiveAbilities ||
		abilitySystem.GetAbility(passiveOneHandle) == nullptr ||
		abilitySystem.GetAbilityById(passiveTwo.abilityId) == nullptr)
	{
		return Fail("Passive ability handle ownership or limit failed");
	}

	TestCombatant damageTarget;
	damageTarget.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
		AttributeModifier{ OwnerAttributeIds::Armor, 50.f }
	);
	GameplayEffectDefinition integrationBarrier;
	integrationBarrier.effectId = "Effect.Test.IntegrationBarrier";
	integrationBarrier.behaviorTag = BarrierEffectSchema::BehaviorId;
	integrationBarrier.durationPolicy = GameplayEffectDurationPolicy::Duration;
	integrationBarrier.duration = 10.f;
	integrationBarrier.attributes = {
		GameplayAttribute{ BarrierEffectSchema::Capacity, 20.f, 0.f },
		GameplayAttribute{ BarrierEffectSchema::AbsorptionRatio, 1.f, 0.f, 1.f }
	};
	damageTarget.GetCombatRuntime().GetEffects().ApplyEffect(integrationBarrier);
	ApplyCombatDamage(damageTarget, 30.f);
	if (!NearlyEqual(damageTarget.GetHealth(), 95.f))
	{
		return Fail("Combat damage did not flow through barrier, armor, and health");
	}

	World weaponCadenceWorld{ nullptr };
	Actor weaponCadenceOwner{ &weaponCadenceWorld };
	AttributeSystem weaponCadenceAttributes;
	GameplayTagContainer weaponCadenceTags;
	GameplayEffectSystem weaponCadenceEffects{ weaponCadenceOwner, weaponCadenceAttributes, weaponCadenceTags };
	AbilitySystem weaponCadenceAbilities{
		weaponCadenceOwner,
		weaponCadenceAttributes,
		weaponCadenceEffects,
		weaponCadenceTags
	};
	PrimaryWeaponDefinition weaponCadenceDefinition{
		"Weapon.Test.Cadence",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		WeaponData::Laser_Blue_PresentationDef,
		{
			GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
			GameplayAttribute{ CommonAttributeIds::FireRate, 2.f, 0.01f },
			GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 100.f, 0.f },
			GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 10.f, 0.f },
			GameplayAttribute{ CommonAttributeIds::Range, 10000.f, 0.f },
			GameplayAttribute{ CommonAttributeIds::CollisionRadius, 7.f, 0.1f }
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
	weaponCadenceAbilities.SetSlotInput(AbilitySlot::PrimaryFire, true);
	weaponCadenceAbilities.Tick(0.f);
	weaponCadenceWorld.TickInternal(0.f);
	if (weaponCadenceWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 1)
	{
		return Fail("Weapon cadence test did not fire its initial shot");
	}
	weaponCadenceAbilities.SetSlotInput(AbilitySlot::PrimaryFire, false);
	weaponCadenceAbilities.Tick(0.1f);
	weaponCadenceAbilities.SetSlotInput(AbilitySlot::PrimaryFire, true);
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

	World dualKineticWorld{ nullptr };
	Actor dualKineticOwner{ &dualKineticWorld };
	AttributeSystem dualKineticAttributes;
	GameplayTagContainer dualKineticTags;
	GameplayEffectSystem dualKineticEffects{ dualKineticOwner, dualKineticAttributes, dualKineticTags };
	AbilitySystem dualKineticAbilities{
		dualKineticOwner,
		dualKineticAttributes,
		dualKineticEffects,
		dualKineticTags
	};
	if (!dualKineticAbilities.GrantAbility(
		AbilityData::MakePrimaryFireAbilityDefinition(WeaponData::PrimaryWeapons::DualKineticBlaster)
	).IsValid())
	{
		return Fail("Dual kinetic blaster ability could not be granted");
	}
	dualKineticAbilities.SetSlotInput(AbilitySlot::PrimaryFire, true);
	dualKineticAbilities.Tick(0.f);
	dualKineticWorld.TickInternal(0.f);
	if (dualKineticWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 2)
	{
		return Fail("Dual kinetic blaster did not fire one projectile from each muzzle");
	}

	World electricArcWorld{ nullptr };
	Actor electricArcOwner{ &electricArcWorld };
	electricArcOwner.SetCollisionLayer(CollisionLayer::Player);
	AttributeSystem electricArcAttributes;
	GameplayTagContainer electricArcTags;
	GameplayEffectSystem electricArcEffects{ electricArcOwner, electricArcAttributes, electricArcTags };
	AbilitySystem electricArcAbilities{
		electricArcOwner,
		electricArcAttributes,
		electricArcEffects,
		electricArcTags
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
		AbilityData::MakePrimaryFireAbilityDefinition(WeaponData::PrimaryWeapons::ElectricArcLauncher)
	).IsValid())
	{
		return Fail("Electric arc launcher ability could not be granted");
	}
	electricArcAbilities.SetSlotInput(AbilitySlot::PrimaryFire, true);
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
		WeaponData::PrimaryWeapons::ElectricArcLauncher,
		emptyArcRuntime
	).isValid)
	{
		return Fail("Electric arc launcher runtime could not be initialized without targets");
	}
	const PrimaryWeaponExecutionContext emptyArcContext{
		emptyArcOwner,
		WeaponData::PrimaryWeapons::ElectricArcLauncher,
		WeaponData::PrimaryWeapons::ElectricArcLauncher.attributes,
		WeaponData::PrimaryWeapons::ElectricArcLauncher.damageTags
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
		WeaponData::PrimaryWeapons::CryoWaveProjector,
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
		WeaponData::PrimaryWeapons::CryoWaveProjector,
		WeaponData::PrimaryWeapons::CryoWaveProjector.attributes,
		WeaponData::PrimaryWeapons::CryoWaveProjector.damageTags
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
	for (int pelletIndex = 0; pelletIndex < 3; ++pelletIndex)
	{
		threePelletGroup.CompletePellet();
	}
	if (!NearlyEqual(shotgunTarget->GetHealth(), 76.f))
	{
		return Fail("Three shotgun pellets did not apply the expected 8 damage per pellet");
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
	if (!NearlyEqual(splitShotTarget->GetHealth(), 82.f) ||
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
	if (!NearlyEqual(minimumDamageTarget->GetHealth(), 60.f))
	{
		return Fail("Shotgun minimum damage multiplier was not respected");
	}

	const List<const PrimaryWeaponDefinition*> configuredWeapons{
		&WeaponData::PrimaryWeapons::BasicRapidLaser,
		&WeaponData::PrimaryWeapons::RapidShotgun,
		&WeaponData::PrimaryWeapons::DualKineticBlaster,
		&WeaponData::PrimaryWeapons::ElectricArcLauncher,
		&WeaponData::PrimaryWeapons::ContinuousHeatLaser,
		&WeaponData::PrimaryWeapons::CryoWaveProjector,
		&WeaponData::PrimaryWeapons::VanguardBlaster,
		&WeaponData::PrimaryWeapons::VanguardEliteBlaster,
		&WeaponData::PrimaryWeapons::TwinBladeDualBlaster,
		&WeaponData::PrimaryWeapons::HexagonRadialBlaster,
		&WeaponData::PrimaryWeapons::UfoTriBlaster,
		&WeaponData::PrimaryWeapons::BossBaseDualBlaster,
		&WeaponData::PrimaryWeapons::BossThreeWayBlaster,
		&WeaponData::PrimaryWeapons::BossFrontalSweep,
		&WeaponData::PrimaryWeapons::BossLastStageSideBlaster
	};
	for (const PrimaryWeaponDefinition* weapon : configuredWeapons)
	{
		if (!weapon || !PrimaryWeaponExecutionSystem::ValidateDefinition(*weapon).isValid)
		{
			return Fail("A shipped primary weapon definition failed validation");
		}
	}

	const PrimaryWeaponDefinition& basicLaser = WeaponData::PrimaryWeapons::BasicRapidLaser;
	if (basicLaser.weaponId != "FighterBasicRapidLaser" ||
		basicLaser.weaponTypeTag != PrimaryWeaponSchema::Projectile::Standard::TypeId ||
		basicLaser.progressionProfile.ResolveLevelSteps().size() != 3)
	{
		return Fail("Fighter basic rapid laser has the wrong base profile");
	}

	const PrimaryWeaponDefinition& dualKineticBlaster = WeaponData::PrimaryWeapons::DualKineticBlaster;
	if (dualKineticBlaster.weaponTypeTag != PrimaryWeaponSchema::Projectile::Standard::TypeId ||
		dualKineticBlaster.muzzleDefinitions.size() != 2 ||
		FindGameplayAttributeValue(dualKineticBlaster.attributes, CommonAttributeIds::Range) <= 400.f ||
		dualKineticBlaster.damageTags.size() != 1 ||
		dualKineticBlaster.damageTags.front() != DamageTypeSchema::Kinetic)
	{
		return Fail("Dual kinetic blaster has the wrong primary weapon profile");
	}
	const PrimaryWeaponDefinition& electricArcLauncher = WeaponData::PrimaryWeapons::ElectricArcLauncher;
	if (electricArcLauncher.weaponTypeTag != PrimaryWeaponSchema::Arc::Electric::TypeId ||
		FindGameplayAttributeValue(
			electricArcLauncher.attributes,
			PrimaryWeaponSchema::Arc::Electric::ChainCount
		) != 3.f ||
		FindGameplayAttributeValue(
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
		!hasSingleDamageType(WeaponData::PrimaryWeapons::RapidShotgun, DamageTypeSchema::Thermal) ||
		!hasSingleDamageType(WeaponData::PrimaryWeapons::ContinuousHeatLaser, DamageTypeSchema::Energy) ||
		!hasSingleDamageType(dualKineticBlaster, DamageTypeSchema::Kinetic) ||
		!hasSingleDamageType(electricArcLauncher, DamageTypeSchema::Electric) ||
		!hasSingleDamageType(WeaponData::PrimaryWeapons::CryoWaveProjector, DamageTypeSchema::Cryo))
	{
		return Fail("Player primary weapon default damage types were not configured correctly");
	}
	const PrimaryWeaponDefinition& continuousHeatLaser = WeaponData::PrimaryWeapons::ContinuousHeatLaser;
	const auto hasAdditiveScaling = [](
		const PrimaryWeaponDefinition& weapon,
		const GameplayTag& target,
		const GameplayTag& source,
		float coefficient
	)
	{
		return std::any_of(
			weapon.scalingRules.begin(),
			weapon.scalingRules.end(),
			[&](const AttributeScalingRule& rule)
			{
				return rule.targetAttributeId == target &&
					rule.sourceAttributeId == source &&
					rule.operation == AttributeModifierOperation::Add &&
					NearlyEqual(rule.coefficient, coefficient);
			}
		);
	};
	if (!hasAdditiveScaling(basicLaser, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 1.f) ||
		!hasAdditiveScaling(basicLaser, CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 1.f) ||
		!hasAdditiveScaling(WeaponData::PrimaryWeapons::RapidShotgun, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.75f) ||
		!hasAdditiveScaling(WeaponData::PrimaryWeapons::RapidShotgun, CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 0.5f) ||
		!hasAdditiveScaling(dualKineticBlaster, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.45f) ||
		!hasAdditiveScaling(dualKineticBlaster, CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 1.f) ||
		!hasAdditiveScaling(electricArcLauncher, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.85f) ||
		!hasAdditiveScaling(electricArcLauncher, CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 0.60f) ||
		!hasAdditiveScaling(continuousHeatLaser, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.75f) ||
		!hasAdditiveScaling(continuousHeatLaser, CommonAttributeIds::Damage, OwnerAttributeIds::EnergyMax, 0.50f) ||
		!hasAdditiveScaling(WeaponData::PrimaryWeapons::CryoWaveProjector, CommonAttributeIds::Damage, OwnerAttributeIds::AttackPower, 0.75f) ||
		!hasAdditiveScaling(WeaponData::PrimaryWeapons::CryoWaveProjector, CommonAttributeIds::FireRate, OwnerAttributeIds::AttackSpeed, 0.5f))
	{
		return Fail("Player primary weapon scaling coefficients are not configured to the balance targets");
	}
	bool hasEnergyMaxBeamScaling = false;
	for (const AttributeScalingRule& scalingRule : continuousHeatLaser.scalingRules)
	{
		if (scalingRule.targetAttributeId == CommonAttributeIds::Damage &&
			scalingRule.sourceAttributeId == OwnerAttributeIds::EnergyMax &&
			scalingRule.operation == AttributeModifierOperation::Add &&
			NearlyEqual(scalingRule.coefficient, 0.50f))
		{
			hasEnergyMaxBeamScaling = true;
		}
		if (scalingRule.sourceAttributeId == ShipAttributeIds::MaxShield)
		{
			return Fail("Continuous heat laser must not scale from current or maximum shield");
		}
	}
	if (!hasEnergyMaxBeamScaling)
	{
		return Fail("Continuous heat laser is missing its EnergyMax damage scaling");
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
			!ResolveFighterPrimaryWeaponScaling(WeaponData::PrimaryWeapons::RapidShotgun, level, shotgun) ||
			!ResolveFighterPrimaryWeaponScaling(dualKineticBlaster, level, dual) ||
			!ResolveFighterPrimaryWeaponScaling(electricArcLauncher, level, electric) ||
			!ResolveFighterPrimaryWeaponScaling(continuousHeatLaser, level, beam) ||
			!ResolveFighterPrimaryWeaponScaling(WeaponData::PrimaryWeapons::CryoWaveProjector, level, cryo))
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
		if (level == 50 && dualDps >= rapidDps)
		{
			return Fail("Dual kinetic growth remains above the rapid laser after its two-muzzle cadence is included");
		}

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
		if (!verifiesAttackPower(basicLaser, 1.f) || !verifiesAttackSpeed(basicLaser, 1.f) ||
			!verifiesAttackPower(WeaponData::PrimaryWeapons::RapidShotgun, 0.75f) || !verifiesAttackSpeed(WeaponData::PrimaryWeapons::RapidShotgun, 0.5f) ||
			!verifiesAttackPower(dualKineticBlaster, 0.45f) || !verifiesAttackSpeed(dualKineticBlaster, 1.f) ||
			!verifiesAttackPower(electricArcLauncher, 0.85f) || !verifiesAttackSpeed(electricArcLauncher, 0.60f) ||
			!verifiesAttackPower(continuousHeatLaser, 0.75f) ||
			!verifiesAttackPower(WeaponData::PrimaryWeapons::CryoWaveProjector, 0.75f) ||
			!verifiesAttackSpeed(WeaponData::PrimaryWeapons::CryoWaveProjector, 0.5f))
		{
			return Fail("Primary weapon AttackPower or AttackSpeed runtime scaling did not match its configured coefficient");
		}

		PrimaryWeaponScalingSample beamWithAttackSpeed;
		PrimaryWeaponScalingSample beamWithEnergy;
		if (!ResolveFighterPrimaryWeaponScaling(continuousHeatLaser, level, beamWithAttackSpeed, 0.f, 100.f) ||
			!ResolveFighterPrimaryWeaponScaling(continuousHeatLaser, level, beamWithEnergy, 0.f, 0.f, 1.f) ||
			!NearlyEqual(beamWithAttackSpeed.damage, beam.damage) ||
			!NearlyEqual(beamWithAttackSpeed.fireRate, 0.f) ||
			!NearlyEqual(beamWithEnergy.damage - beam.damage, 0.5f))
		{
			return Fail("Continuous beam AttackSpeed or EnergyMax runtime scaling did not match its balance contract");
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

	const AbilityDefinition basicLaserAbility = AbilityData::MakePrimaryFireAbilityDefinition(basicLaser);
	if (basicLaser.progressionProfile.GetScrapCostToReachLevel(2) != 40u ||
		basicLaser.progressionProfile.GetScrapCostToReachLevel(3) != 50u ||
		basicLaser.progressionProfile.GetScrapCostToReachLevel(4) != 65u ||
		!basicLaserAbility.HasScrapCostToReachLevel(2) ||
		basicLaserAbility.GetScrapCostToReachLevel(2) != 40u ||
		basicLaserAbility.GetScrapCostToReachLevel(4) != 65u)
	{
		return Fail("Primary weapon scrap costs were not preserved during ability conversion");
	}
	const AbilityHandle basicLaserHandle = abilitySystem.GrantAbility(basicLaserAbility);
	if (!basicLaserHandle.IsValid() ||
		!abilitySystem.TrySetAbilityLevel(AbilitySlot::PrimaryFire, basicLaserAbility.GetMaxLevel()))
	{
		return Fail("Fighter basic rapid laser could not reach its maximum level");
	}
	const AbilityInstance* basicLaserInstance = abilitySystem.GetAbility(AbilitySlot::PrimaryFire);
	const GameplayAttribute* basicDamage = FindGameplayAttribute(basicLaser.attributes, CommonAttributeIds::Damage);
	const GameplayAttribute* basicFireRate = FindGameplayAttribute(basicLaser.attributes, CommonAttributeIds::FireRate);
	const GameplayAttribute* basicSpeed = FindGameplayAttribute(
		basicLaser.attributes,
		PrimaryWeaponSchema::Projectile::Delivery::Speed
	);
	const GameplayAttribute* basicRange = FindGameplayAttribute(basicLaser.attributes, CommonAttributeIds::Range);
	if (!basicLaserInstance || basicLaserInstance->GetLevel() != basicLaserAbility.GetMaxLevel() ||
		!basicDamage || !basicFireRate || !basicSpeed || !basicRange ||
		CalculateModifiedAttributeValue(
			*basicDamage,
			basicLaserInstance->GetDefinition().attributeModifiers
		) <= basicDamage->currentValue ||
		CalculateModifiedAttributeValue(
			*basicFireRate,
			basicLaserInstance->GetDefinition().attributeModifiers
		) <= basicFireRate->currentValue ||
		CalculateModifiedAttributeValue(
			*basicSpeed,
			basicLaserInstance->GetDefinition().attributeModifiers
		) <= basicSpeed->currentValue ||
		CalculateModifiedAttributeValue(
			*basicRange,
			basicLaserInstance->GetDefinition().attributeModifiers
		) <= basicRange->currentValue)
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
	const GameplayAttributeList thermalDamageAttributes = thermalLoadout.ApplyConditionalModifiers(
		AttachmentHostKind::PrimaryWeapon,
		{ GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f } },
		{ DamageTypeSchema::Thermal }
	);
	if (!NearlyEqual(FindGameplayAttributeValue(thermalDamageAttributes, CommonAttributeIds::Damage), 12.f))
	{
		return Fail("Thermal attachment did not grant its already-thermal damage bonus");
	}

	AbilityDefinition attachmentAbility;
	attachmentAbility.abilityId = "Ability.Test.AttachmentHost";
	attachmentAbility.slot = AbilitySlot::Ability4;
	attachmentAbility.cooldown = 10.f;
	attachmentAbility.abilityTags = { GameplayTag{ "Ability.Offense" } };
	const AbilityHandle attachmentAbilityHandle = abilitySystem.GrantAbility(attachmentAbility);
	if (!attachmentAbilityHandle.IsValid() || !abilitySystem.TryEquipAttachment(
		attachmentAbilityHandle,
		AttachmentData::Definitions::HeavyCapacitor,
		AttachmentHostKind::Ability
	))
	{
		return Fail("Ability-only cooldown attachment could not be equipped on an offensive ability");
	}
	const AbilityInstance* attachmentAbilityInstance = abilitySystem.GetAbility(attachmentAbilityHandle);
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
	AttributeSystem attachmentWeaponAttributes;
	GameplayTagContainer attachmentWeaponTags;
	GameplayEffectSystem attachmentWeaponEffects{
		attachmentWeaponOwner,
		attachmentWeaponAttributes,
		attachmentWeaponTags
	};
	AbilitySystem attachmentWeaponAbilities{
		attachmentWeaponOwner,
		attachmentWeaponAttributes,
		attachmentWeaponEffects,
		attachmentWeaponTags
	};
	PrimaryWeaponDefinition attachmentWeaponDefinition{
		"Weapon.Test.AttachmentSalvo",
		PrimaryWeaponSchema::Projectile::Standard::TypeId,
		WeaponData::Laser_Blue_PresentationDef,
		{
			GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
			GameplayAttribute{ CommonAttributeIds::FireRate, 3.f, 0.01f },
			GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 100.f, 0.f },
			GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 10.f, 0.f },
			GameplayAttribute{ CommonAttributeIds::Range, 10000.f, 0.f },
			GameplayAttribute{ CommonAttributeIds::CollisionRadius, 7.f, 0.1f }
		},
		{},
		true
	};
	if (!attachmentWeaponAbilities.GrantAbility(
		AbilityData::MakePrimaryFireAbilityDefinition(attachmentWeaponDefinition)
	).IsValid() || !attachmentWeaponAbilities.TryEquipAttachment(
		AbilitySlot::PrimaryFire,
		AttachmentData::Definitions::EmergencySalvo,
		AttachmentHostKind::PrimaryWeapon
	) || !attachmentWeaponAbilities.TryEquipAttachment(
		AbilitySlot::PrimaryFire,
		AttachmentData::Definitions::ThermalConverter,
		AttachmentHostKind::PrimaryWeapon
	))
	{
		return Fail("Compatible primary weapon attachments could not be equipped");
	}
	const AbilityInstance* attachmentPrimaryInstance = attachmentWeaponAbilities.GetAbility(AbilitySlot::PrimaryFire);
	const List<GameplayTag> attachmentPrimaryDamageTags = attachmentPrimaryInstance
		? attachmentPrimaryInstance->GetResolvedDamageTags(AttachmentHostKind::PrimaryWeapon)
		: List<GameplayTag>{};
	if (attachmentPrimaryDamageTags.size() != 1 || attachmentPrimaryDamageTags.front() != DamageTypeSchema::Thermal)
	{
		return Fail("Primary weapon attachment damage type was not resolved");
	}
	attachmentWeaponAbilities.SetSlotInput(AbilitySlot::PrimaryFire, true);
	attachmentWeaponAbilities.Tick(0.f);
	attachmentWeaponWorld.TickInternal(0.f);
	if (attachmentWeaponWorld.GetActorsByType<PrimaryWeaponProjectileActor>().size() != 2)
	{
		return Fail("Low fire-rate conditional attachment did not add a projectile");
	}

	std::string rocketValidationFailure;
	const AbilityDefinition* rocketDefinition = AbilityData::FindShippedAbilityDefinition("Ability.Rocket.Basic");
	if (!AbilitySystem::ValidateCatalog(AbilityData::GetShippedAbilityDefinitions(), &rocketValidationFailure) ||
		!rocketDefinition || rocketDefinition->slot != AbilitySlot::Ability4 ||
		rocketDefinition->activationPolicy != AbilityActivationPolicy::OnPressed ||
		rocketDefinition->lifetimePolicy != AbilityLifetimePolicy::Instant ||
		rocketDefinition->maxCharges != 1 || rocketDefinition->behaviorId != AbilityData::Rocket::BehaviorId ||
		rocketDefinition->damageTags.size() != 1 || rocketDefinition->damageTags.front() != DamageTypeSchema::Kinetic ||
		!AbilityActorRegistry::ValidateDefinition(AbilityData::AbilityActors::Actor_Rocket_Basic).isValid)
	{
		return Fail("Basic Rocket configuration or shipped catalog validation failed");
	}
	AbilityActorDefinition rocketWithoutPresentation =
		AbilityData::AbilityActors::Actor_Rocket_Basic;
	rocketWithoutPresentation.presentationProfileId.clear();
	if (AbilityActorRegistry::ValidateDefinition(rocketWithoutPresentation).isValid)
	{
		return Fail("Basic Rocket accepted a missing presentation profile");
	}

	const AbilityData::Rocket::Settings* rocketSettings =
		AbilityData::Rocket::FindSettings(rocketDefinition->abilityId);
	if (!rocketSettings || rocketSettings->baseDamage <= 0.f || rocketSettings->cooldown <= 0.f ||
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
		float energyMax = 0.f
	)
	{
		owner.GetCombatRuntime().InitializeOwnerAttributes(1000.f);
		owner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
			AttributeModifier{ OwnerAttributeIds::AttackPower, attackPower }
		);
		owner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
			AttributeModifier{ OwnerAttributeIds::AttackSpeed, attackSpeed }
		);
		owner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
			AttributeModifier{ OwnerAttributeIds::Luck, luck }
		);
		owner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
			AttributeModifier{ OwnerAttributeIds::EnergyMax, energyMax }
		);
		owner.SetCollisionLayer(CollisionLayer::Player);
		owner.SetActorRotation(90.f);

		AbilitySystem& abilities = owner.GetCombatRuntime().GetAbilities();
		const AbilityHandle handle = abilities.GrantAbility(*rocketDefinition);
		if (!handle.IsValid() || (level > 1 && !abilities.TrySetAbilityLevel(handle, level)))
		{
			return shared_ptr<RocketProjectileActor>{};
		}
		abilities.SetSlotInput(AbilitySlot::Ability4, true);
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
					AbilityData::AbilityActors::Actor_Rocket_Basic,
					AbilityData::AbilityActors::Actor_Rocket_Basic.attributes,
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
		AbilityData::AbilityActors::Actor_Rocket_Basic.attributes
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
		return Fail("AttackSpeed, Luck, or EnergyMax changed Basic Rocket damage");
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
		const AbilityInstance* levelInstance =
			levelRocketOwner.GetCombatRuntime().GetAbilities().GetAbility(AbilitySlot::Ability4);
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
	const AbilityHandle hasteRocketHandle = hasteRocketOwner.GetCombatRuntime().GetAbilities().GrantAbility(*rocketDefinition);
	const float baseRocketCooldown = hasteRocketOwner.GetCombatRuntime().GetAbilities()
		.GetAbility(hasteRocketHandle)->GetCooldownDuration();
	hasteRocketOwner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
		AttributeModifier{ OwnerAttributeIds::AbilityHaste, 100.f }
	);
	const AbilityInstance* hasteRocket = hasteRocketOwner.GetCombatRuntime().GetAbilities().GetAbility(hasteRocketHandle);
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

	std::string gravityValidationFailure;
	const AbilityDefinition* gravityDefinition = AbilityData::FindShippedAbilityDefinition(
		"Ability.GravityAnomaly.Basic"
	);
	if (!AbilitySystem::ValidateCatalog(AbilityData::GetShippedAbilityDefinitions(), &gravityValidationFailure) ||
		!gravityDefinition || gravityDefinition->slot != AbilitySlot::Ability1 ||
		gravityDefinition->inputLabel != "Q" ||
		gravityDefinition->behaviorId != AbilityData::GravityAnomaly::BehaviorId ||
		gravityDefinition->damageTags.size() != 0 ||
		!AbilityActorRegistry::ValidateDefinition(
			AbilityData::AbilityActors::Actor_GravityAnomaly_Projectile_Basic
		).isValid ||
		!AbilityActorRegistry::ValidateDefinition(
			AbilityData::AbilityActors::Actor_GravityAnomaly_Field_Basic
		).isValid)
	{
		return Fail("Gravity Anomaly shipped ability or actor catalog validation failed");
	}
	PlayerSpaceShip gravityLoadout{ nullptr };
	const AbilityInstance* gravityLoadoutAbility = gravityLoadout.GetCombatRuntime()
		.GetAbilities().GetAbility(AbilitySlot::Ability1);
	if (!gravityLoadoutAbility ||
		gravityLoadoutAbility->GetDefinition().abilityId != "Ability.GravityAnomaly.Basic" ||
		gravityLoadout.GetCombatRuntime().GetAbilities().GetAbilityById("Ability.Shield.Basic") != nullptr)
	{
		return Fail("Default player loadout did not place Gravity Anomaly on Ability1/Q");
	}
	AbilityActorDefinition gravityProjectileWithoutProfile =
		AbilityData::AbilityActors::Actor_GravityAnomaly_Projectile_Basic;
	gravityProjectileWithoutProfile.presentationProfileId.clear();
	AbilityActorDefinition gravityProjectileWithFieldProfile =
		AbilityData::AbilityActors::Actor_GravityAnomaly_Projectile_Basic;
	gravityProjectileWithFieldProfile.presentationProfileId =
		GravityAnomalyPresentationIds::FieldBasic;
	if (AbilityActorRegistry::ValidateDefinition(gravityProjectileWithoutProfile).isValid ||
		AbilityActorRegistry::ValidateDefinition(gravityProjectileWithFieldProfile).isValid)
	{
		return Fail("Gravity Anomaly actor validation accepted a missing or cross-family typed profile");
	}

	const AbilityData::GravityAnomaly::Settings* gravitySettings =
		AbilityData::GravityAnomaly::FindSettings(gravityDefinition->abilityId);
	const GameplayEffectDefinition& gravityInsideEffect =
		EffectData::GravityAnomalyInsideEffect;
	const auto MakeGravityInsideSpec = [](float slowMagnitude)
	{
		GameplayEffectSpec spec =
			MakeGameplayEffectSpec(EffectData::GravityAnomalyInsideEffect);
		SetGameplayEffectModifierMagnitude(
			spec,
			OwnerAttributeIds::MovementSlow,
			slowMagnitude
		);
		return spec;
	};
	if (!gravitySettings || gravitySettings->cooldown != 8.f ||
		gravitySettings->castRange != 900.f || gravitySettings->projectileSpeed != 2000.f ||
		gravitySettings->baseDuration != 2.5f || gravitySettings->baseRadius != 220.f ||
		gravitySettings->pullStrength != 500.f || gravitySettings->slowMagnitude != 0.20f ||
		gravityInsideEffect.effectId != AbilityData::GravityAnomaly::EffectSchema::InsideEffectId ||
		gravityInsideEffect.durationPolicy != GameplayEffectDurationPolicy::Duration ||
		!NearlyEqual(
			gravityInsideEffect.duration,
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectDurationSeconds
		) ||
		gravityInsideEffect.stackingPolicy != GameplayEffectStackingPolicy::RefreshDuration ||
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
		const AbilityHandle handle = owner.GetCombatRuntime().GetAbilities().GrantAbility(*gravityDefinition);
		if (!handle.IsValid() ||
			(level > 1 && !owner.GetCombatRuntime().GetAbilities().TrySetAbilityLevel(handle, level)))
		{
			return shared_ptr<GravityAnomalyProjectileActor>{};
		}
		owner.GetCombatRuntime().GetAbilities().SetSlotInput(AbilitySlot::Ability1, true);
		owner.GetCombatRuntime().GetAbilities().Tick(0.f);
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
					AbilityData::AbilityActors::Actor_GravityAnomaly_Projectile_Basic,
					AbilityData::AbilityActors::Actor_GravityAnomaly_Projectile_Basic.attributes,
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
		AbilityData::AbilityActors::Actor_GravityAnomaly_Projectile_Basic.attributes
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
		const GameplayAttributeList attributes =
			AbilityData::AbilityActors::Actor_GravityAnomaly_Field_Basic.attributes;
		const shared_ptr<GravityAnomalyFieldActor> field =
			std::dynamic_pointer_cast<GravityAnomalyFieldActor>(
				AbilityActorRegistry::Spawn(
					AbilityActorSpawnContext{
						owner,
						AbilityData::AbilityActors::Actor_GravityAnomaly_Field_Basic,
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
		if (!target->GetCombatRuntime().GetEffects().FindEffectById(
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
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
	const GameplayEffectHandle gravityPlayerLifecycleEffect =
		gravityPlayerLifecycleTarget->GetCombatRuntime().GetEffects().ApplyEffect(
			MakeGravityInsideSpec(0.20f)
		);
	if (!gravityPlayerLifecycleEffect.IsValid() ||
		!gravityPlayerLifecycleTarget->GetCombatRuntime().GetEffects().FindEffectById(
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
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
	if (!gravityLivePlayer->GetCombatRuntime().GetEffects().FindEffectById(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
	) || gravityLivePlayerWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty())
	{
		return Fail("Gravity Anomaly failed while a live player entered its field");
	}
	gravityLivePlayerField->Destroy();
	gravityLivePlayerWorld.TickInternal(0.f);
	if (!gravityLivePlayer->GetCombatRuntime().GetEffects().FindEffectById(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
	) || gravityLivePlayerWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty())
	{
		return Fail("Gravity Anomaly did not preserve a live player's slow after its field ended");
	}
	gravityLivePlayerWorld.TickInternal(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectDurationSeconds - 0.1f
	);
	if (!gravityLivePlayer->GetCombatRuntime().GetEffects().FindEffectById(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
	))
	{
		return Fail("Gravity Anomaly slow expired before its two-second grace period ended");
	}
	gravityLivePlayerWorld.TickInternal(0.2f);
	if (gravityLivePlayer->GetCombatRuntime().GetEffects().FindEffectById(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
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
	const GameplayEffectHandle pullHandle = pullTarget.GetCombatRuntime().GetEffects().ApplyEffect(
		MakeGravityInsideSpec(0.20f),
		GameplayEffectApplicationContext{
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
	const GameplayEffectHandle playerSlowHandle = playerSlowMovement.GetCombatRuntime().GetEffects().ApplyEffect(
		MakeGravityInsideSpec(0.20f)
	);
	const GameplayEffectHandle enemySlowHandle = enemySlowMovement.GetCombatRuntime().GetEffects().ApplyEffect(
		MakeGravityInsideSpec(0.20f)
	);
	playerSlowMovement.Tick(0.1f);
	enemySlowMovement.Tick(0.1f);
	if (!NearlyEqual(playerSlowMovement.GetActorLocation().x, 8.f) ||
		!NearlyEqual(enemySlowMovement.GetActorLocation().x, 8.f))
	{
		return Fail("GameplayEffect movement slow did not reduce real player and enemy movement");
	}
	playerSlowMovement.GetCombatRuntime().GetEffects().RemoveEffect(playerSlowHandle);
	enemySlowMovement.GetCombatRuntime().GetEffects().RemoveEffect(enemySlowHandle);
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
		for (const GameplayEffectSnapshot& snapshot : target.GetCombatRuntime().GetEffects().BuildSnapshots())
		{
			count += snapshot.effectId ==
				AbilityData::GravityAnomaly::EffectSchema::InsideEffectId ? 1u : 0u;
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
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectDurationSeconds + 0.1f
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
		!NearlyEqual(baselineGravityProjectile->GetResolvedFieldRadius(), 220.f) ||
		!NearlyEqual(baselineGravityProjectile->GetResolvedFieldDuration(), 2.5f) ||
		!NearlyEqual(scaledGravityProjectile->GetResolvedFieldRadius(), 240.f) ||
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
	const AbilityInstance* levelGravityInstance = levelGravityOwner.GetCombatRuntime().GetAbilities().GetAbility(
		AbilitySlot::Ability1
	);
	if (!levelGravityProjectile || !levelGravityInstance ||
		!NearlyEqual(levelGravityInstance->GetCooldownDuration(), 6.6f) ||
		!NearlyEqual(levelGravityProjectile->GetResolvedFieldDuration(), 2.92f) ||
		!NearlyEqual(levelGravityProjectile->GetResolvedFieldRadius(), 248.f) ||
		!NearlyEqual(levelGravityProjectile->GetResolvedPullStrength(), 640.f) ||
		!NearlyEqual(levelGravityProjectile->GetResolvedSlowMagnitude(), 0.27f) ||
		!NearlyEqual(levelGravityProjectile->GetProjectileSpeed(), 2350.f) ||
		!NearlyEqual(levelGravityProjectile->GetCastRange(), 970.f))
	{
		return Fail("Gravity Anomaly level progression was not cumulative through level fifteen");
	}
	levelGravityOwner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
		AttributeModifier{ OwnerAttributeIds::AbilityHaste, 100.f }
	);
	if (!(levelGravityInstance->GetCooldownDuration() < 6.6f))
	{
		return Fail("Gravity Anomaly did not use centralized AbilityHaste cooldown resolution");
	}

	// A target inside the field is refreshed to exactly two seconds. Leaving the
	// field stops that refresh, but does not remove the slow prematurely.
	gravityPlayerTarget->SetActorLocation({ gravityField->GetResolvedRadius() + 10.f, 0.f });
	gravityFieldWorld.TickInternal(0.f);
	if (!gravityPlayerTarget->GetCombatRuntime().GetEffects().FindEffectById(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
	))
	{
		return Fail("Gravity Anomaly removed slow immediately when a target left the field");
	}
	gravityPlayerTarget->GetCombatRuntime().Tick(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectDurationSeconds - 0.1f
	);
	if (!gravityPlayerTarget->GetCombatRuntime().GetEffects().FindEffectById(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
	))
	{
		return Fail("Gravity Anomaly slow did not persist for two seconds after leaving the field");
	}
	gravityPlayerTarget->GetCombatRuntime().Tick(0.2f);
	if (gravityPlayerTarget->GetCombatRuntime().GetEffects().FindEffectById(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
	))
	{
		return Fail("Gravity Anomaly slow did not expire after leaving the field");
	}

	gravityField->Destroy();
	gravityFieldWorld.TickInternal(0.f);
	if (!gravityCaster->GetCombatRuntime().GetEffects().FindEffectById(
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
		) || gravityFieldWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty())
	{
		return Fail("Gravity Anomaly field destruction did not preserve active slows for two seconds");
	}
	gravityCaster->GetCombatRuntime().Tick(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectDurationSeconds + 0.1f
	);
	gravityEnemyTarget->GetCombatRuntime().Tick(
		AbilityData::GravityAnomaly::EffectSchema::InsideEffectDurationSeconds + 0.1f
	);
	gravityFieldWorld.TickInternal(0.f);
	if (gravityCaster->GetCombatRuntime().GetEffects().FindEffectById(
			AbilityData::GravityAnomaly::EffectSchema::InsideEffectId
		) || !gravityFieldWorld.GetActorsByType<GravityAnomalyEffectVisual>().empty())
	{
		return Fail("Gravity Anomaly field destruction did not clean up expired slows and visuals");
	}

	std::string dashValidationFailure;
	const AbilityDefinition* dashDefinition = AbilityData::FindShippedAbilityDefinition("Ability.Dash.Basic");
	if (!AbilitySystem::ValidateCatalog(AbilityData::GetShippedAbilityDefinitions(), &dashValidationFailure) ||
		!dashDefinition || dashDefinition->slot != AbilitySlot::Ability3 ||
		dashDefinition->activationPolicy != AbilityActivationPolicy::OnPressed ||
		dashDefinition->lifetimePolicy != AbilityLifetimePolicy::Duration ||
		dashDefinition->cooldown <= 0.f || dashDefinition->duration <= 0.f ||
		dashDefinition->maxCharges != 1 || !dashDefinition->damageTags.empty() ||
		!dashDefinition->scalingRules.empty() || !dashDefinition->actions.empty() ||
		dashDefinition->behaviorId != AbilityData::Dash::BehaviorId)
	{
		return Fail("Basic Dash configuration or shipped ability catalog validation failed");
	}

	const AbilityData::Dash::Settings* dashSettings =
		AbilityData::Dash::FindSettings(dashDefinition->abilityId);
	if (!dashSettings || dashSettings->baseDistance <= 0.f ||
		!NearlyEqual(dashSettings->duration, dashDefinition->duration) ||
		dashSettings->cameraZoomOutRatio < 0.f || dashSettings->cameraZoomOutRatio > 0.5f ||
		dashSettings->cooldownReductionPerLevelRatio < 0.f ||
		dashSettings->cooldownReductionPerLevelRatio >= 0.25f ||
		dashSettings->directionPolicy != AbilityData::Dash::DirectionPolicy::MovementInputOrMouseWorld ||
		!AbilityData::Dash::StateTag.IsValid() || !AbilityData::Dash::StartEvent.IsValid() ||
		!AbilityData::Dash::EndEvent.IsValid())
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

	AbilityDefinition malformedDash = *dashDefinition;
	malformedDash.duration += 0.01f;
	if (AbilitySystem::ValidateDefinition(malformedDash, &dashValidationFailure))
	{
		return Fail("Mismatched Dash movement/lifetime duration was accepted");
	}
	malformedDash = *dashDefinition;
	malformedDash.behaviorId = GameplayTag{ "AbilityBehavior.Unknown" };
	if (AbilitySystem::ValidateDefinition(malformedDash, &dashValidationFailure))
	{
		return Fail("Unknown ability behavior was accepted by validation");
	}

	auto ActivateBasicDash = [&](TestCombatant& owner, int level = 1)
	{
		AbilitySystem& dashAbilities = owner.GetCombatRuntime().GetAbilities();
		const AbilityHandle handle = dashAbilities.GrantAbility(*dashDefinition);
		if (!handle.IsValid() || (level > 1 && !dashAbilities.TrySetAbilityLevel(handle, level)))
		{
			return false;
		}
		dashAbilities.SetSlotInput(AbilitySlot::Ability3, true);
		dashAbilities.Tick(0.f);
		return owner.GetDashStartCount() == 1;
	};

	TestCombatant cooldownDashOwner;
	cooldownDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	AbilitySystem& cooldownDashAbilities = cooldownDashOwner.GetCombatRuntime().GetAbilities();
	const AbilityHandle cooldownDashHandle = cooldownDashAbilities.GrantAbility(*dashDefinition);
	for (int level = 1; level <= 5; ++level)
	{
		const float expectedDashCooldown =
			dashDefinition->cooldown *
			(1.f - dashSettings->cooldownReductionPerLevelRatio * static_cast<float>(level - 1));
		if ((level > 1 && !cooldownDashAbilities.TrySetAbilityLevel(cooldownDashHandle, level)) ||
			!cooldownDashAbilities.GetAbility(cooldownDashHandle) ||
			!NearlyEqual(
				cooldownDashAbilities.GetAbility(cooldownDashHandle)->GetCooldownDuration(),
				expectedDashCooldown
			))
		{
			return Fail("Basic Dash level cooldown progression is incorrect");
		}
	}
	cooldownDashOwner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
		AttributeModifier{ OwnerAttributeIds::AbilityHaste, 100.f }
	);
	const float expectedLevelFiveDashCooldown =
		dashDefinition->cooldown *
		(1.f - dashSettings->cooldownReductionPerLevelRatio * 4.f);
	if (!NearlyEqual(
		cooldownDashAbilities.GetAbility(cooldownDashHandle)->GetCooldownDuration(),
		expectedLevelFiveDashCooldown * AttributeMath::GetAbilityCooldownMultiplier(100.f)
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
	horizontalDashOwner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
		AttributeModifier{ OwnerAttributeIds::MoveSpeedHorizontal, 20.f }
	);
	horizontalDashOwner.SetDashMovementInput({ 1.f, 0.f });
	if (!ActivateBasicDash(horizontalDashOwner) ||
		horizontalDashOwner.GetLastDashDistance() <= baselineDashDistance)
	{
		return Fail("A one-sided movement rating did not improve Basic Dash distance");
	}

	TestCombatant highMovementDashOwner;
	highMovementDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	highMovementDashOwner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
		AttributeModifier{ OwnerAttributeIds::MoveSpeedHorizontal, 20.f }
	);
	highMovementDashOwner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(
		AttributeModifier{ OwnerAttributeIds::MoveSpeedVertical, 20.f }
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
	for (const AttributeModifier& modifier : List<AttributeModifier>{
		{ OwnerAttributeIds::AttackPower, 500.f },
		{ OwnerAttributeIds::AttackSpeed, 500.f },
		{ OwnerAttributeIds::EnergyMax, 500.f },
		{ OwnerAttributeIds::Luck, 500.f },
		{ OwnerAttributeIds::CriticalChance, 500.f }
	})
	{
		irrelevantStatDashOwner.GetCombatRuntime().GetAttributes().ApplyBaseModifier(modifier);
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
	zeroDirectionDashOwner.GetCombatRuntime().GetAbilities().onGameplayEvent.BindAction(
		&zeroDirectionEvents,
		&DashEventRecorder::Record
	);
	const AbilityHandle zeroDirectionHandle = zeroDirectionDashOwner.GetCombatRuntime().GetAbilities().GrantAbility(*dashDefinition);
	zeroDirectionDashOwner.GetCombatRuntime().GetAbilities().SetSlotInput(AbilitySlot::Ability3, true);
	zeroDirectionDashOwner.GetCombatRuntime().GetAbilities().Tick(0.f);
	if (!zeroDirectionHandle.IsValid() || zeroDirectionDashOwner.GetDashStartCount() != 0 ||
		zeroDirectionDashOwner.GetCombatRuntime().GetOwnedTags().HasTag(AbilityData::Dash::StateTag) ||
		!zeroDirectionEvents.events.empty())
	{
		return Fail("Basic Dash accepted an invalid zero direction");
	}

	TestCombatant lifecycleDashOwner;
	lifecycleDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	lifecycleDashOwner.SetDashMovementInput({ 1.f, 0.f });
	DashEventRecorder lifecycleEvents;
	lifecycleDashOwner.GetCombatRuntime().GetAbilities().onGameplayEvent.BindAction(
		&lifecycleEvents,
		&DashEventRecorder::Record
	);
	if (!ActivateBasicDash(lifecycleDashOwner) ||
		!lifecycleDashOwner.GetCombatRuntime().GetOwnedTags().HasTag(AbilityData::Dash::StateTag) ||
		lifecycleEvents.events.size() != 1 ||
		lifecycleEvents.events.front() != AbilityData::Dash::StartEvent)
	{
		return Fail("Basic Dash start lifecycle did not publish its state and event");
	}
	lifecycleDashOwner.GetCombatRuntime().GetAbilities().Tick(dashSettings->duration);
	if (lifecycleDashOwner.IsDashActive() || lifecycleDashOwner.GetDashEndCount() != 1 ||
		lifecycleDashOwner.GetCombatRuntime().GetOwnedTags().HasTag(AbilityData::Dash::StateTag) ||
		lifecycleEvents.events.size() != 2 ||
		lifecycleEvents.events.back() != AbilityData::Dash::EndEvent)
	{
		return Fail("Basic Dash duration cleanup did not publish its end lifecycle");
	}

	TestCombatant clearedDashOwner;
	clearedDashOwner.GetCombatRuntime().InitializeOwnerAttributes(100.f);
	clearedDashOwner.SetDashMovementInput({ 1.f, 0.f });
	DashEventRecorder clearedEvents;
	clearedDashOwner.GetCombatRuntime().GetAbilities().onGameplayEvent.BindAction(
		&clearedEvents,
		&DashEventRecorder::Record
	);
	if (!ActivateBasicDash(clearedDashOwner))
	{
		return Fail("Basic Dash could not activate before ability-system cleanup");
	}
	clearedDashOwner.GetCombatRuntime().GetAbilities().Clear();
	if (clearedDashOwner.IsDashActive() ||
		clearedDashOwner.GetCombatRuntime().GetOwnedTags().HasTag(AbilityData::Dash::StateTag) ||
		clearedEvents.events.size() != 2 ||
		clearedEvents.events.back() != AbilityData::Dash::EndEvent)
	{
		return Fail("Basic Dash did not clean up its state on ability-system clear");
	}

	return 0;
}
