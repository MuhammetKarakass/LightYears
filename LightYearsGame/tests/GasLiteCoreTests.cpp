#include "framework/Core.h"
#include "gameplay/attributes/AttributeSystem.h"
#include "gameplay/effects/GameplayEffectBehavior.h"
#include "gameplay/effects/GameplayEffectSystem.h"
#include "gameplay/ability/AbilitySystem.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/actors/sunBeam/SunBeamStrikeActor.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/HealthComponent.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "gameConfigs/WeaponStructs.h"
#include "gameConfigs/WeaponConfig.h"
#include "gameConfigs/AbilityConfig.h"
#include "gameplay/weapon/PrimaryWeaponRegistry.h"
#include <cmath>
#include <iostream>

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

	class TestCombatant final : public ly::Actor, public ly::Combatant
	{
	public:
		TestCombatant()
			: Actor{ nullptr }
			, mHealth{ 100.f, 100.f }
			, mCombatRuntime{ *this }
		{
			mCombatRuntime.GetAttributes().RegisterAttribute(ly::OwnerAttributeIds::Armor, 0.f);
		}

		ly::CombatRuntime& GetCombatRuntime() override { return mCombatRuntime; }
		const ly::CombatRuntime& GetCombatRuntime() const override { return mCombatRuntime; }
		float GetHealth() const { return mHealth.GetHealth(); }

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
		ly::HealthComponent mHealth;
		ly::CombatRuntime mCombatRuntime;
	};
}

int main()
{
	using namespace ly;

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

	ActiveGameplayEffect barrier;
	barrier.definition.effectId = "Effect.Test.Barrier";
	barrier.definition.behaviorTag = BarrierEffectSchema::BehaviorId;
	barrier.definition.attributes = {
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

	PrimaryWeaponDefinition projectileWeapon;
	projectileWeapon.weaponTypeTag = PrimaryWeaponSchema::Projectile::Standard::TypeId;
	projectileWeapon.attributes = {
		GameplayAttribute{ CommonAttributeIds::Damage, 10.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Speed, 500.f, 0.f },
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Delivery::Lifetime, 3.f, 0.f }
	};
	if (!PrimaryWeaponRegistry::ValidateDefinition(projectileWeapon).isValid)
	{
		return Fail("Valid projectile weapon was rejected");
	}
	PrimaryWeaponDefinition projectileFamilyDefinition = projectileWeapon;
	projectileFamilyDefinition.weaponTypeTag = PrimaryWeaponSchema::Projectile::FamilyId;
	if (PrimaryWeaponRegistry::ValidateDefinition(projectileFamilyDefinition).isValid)
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
	if (PrimaryWeaponRegistry::ValidateDefinition(mixedWeapon).isValid)
	{
		return Fail("Mixed projectile and beam attributes were accepted");
	}
	PrimaryWeaponDefinition standardProjectileWithShotgunAttribute = projectileWeapon;
	standardProjectileWithShotgunAttribute.attributes.push_back(
		GameplayAttribute{ PrimaryWeaponSchema::Projectile::Shotgun::PelletCount, 3.f, 1.f }
	);
	if (PrimaryWeaponRegistry::ValidateDefinition(standardProjectileWithShotgunAttribute).isValid)
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
	if (!PrimaryWeaponRegistry::ValidateDefinition(shotgunWeapon).isValid)
	{
		return Fail("Valid shotgun weapon was rejected");
	}
	if (!shotgunWeapon.weaponTypeTag.MatchesTag(PrimaryWeaponSchema::Projectile::FamilyId))
	{
		return Fail("Shotgun type tag is not nested under the projectile family");
	}

	PrimaryWeaponDefinition invalidShotgunWeapon = shotgunWeapon;
	invalidShotgunWeapon.attributes.erase(invalidShotgunWeapon.attributes.begin() + 3);
	if (PrimaryWeaponRegistry::ValidateDefinition(invalidShotgunWeapon).isValid)
	{
		return Fail("Shotgun without pellet count was accepted");
	}

	PrimaryWeaponDefinition unsupportedBeamWeapon;
	unsupportedBeamWeapon.weaponTypeTag = PrimaryWeaponSchema::Beam::Continuous::TypeId;
	if (PrimaryWeaponRegistry::ValidateDefinition(unsupportedBeamWeapon).isValid)
	{
		return Fail("A weapon type without an execution handler was accepted");
	}

	PrimaryWeaponDefinition undeclaredFeatureWeapon = projectileWeapon;
	undeclaredFeatureWeapon.attributes.push_back(
		GameplayAttribute{ PrimaryWeaponSchema::Feature::Heat::Capacity, 100.f, 0.f }
	);
	if (PrimaryWeaponRegistry::ValidateDefinition(undeclaredFeatureWeapon).isValid)
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
	if (!PrimaryWeaponRegistry::InitializeRuntime(heatedProjectileWeapon, heatedRuntime).isValid)
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
	PrimaryWeaponRegistry::BeginFire(heatContext, heatedRuntime);
	if (!PrimaryWeaponRegistry::FireOnce(heatContext, heatedRuntime) ||
		!NearlyEqual(heatedRuntime.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue), 3.f))
	{
		return Fail("Heat feature did not accumulate on fire");
	}
	PrimaryWeaponRegistry::TickFire(heatContext, heatedRuntime, 1.f);
	if (!NearlyEqual(heatedRuntime.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue), 1.f))
	{
		return Fail("Heat feature did not dissipate over time");
	}
	PrimaryWeaponRegistry::EndFire(heatContext, heatedRuntime);

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
	if (!RegisterSunBeamStrikeActorType() ||
		!AbilityActorRegistry::ValidateDefinition(AbilityData::AbilityActors::Actor_SunBeam_Basic).isValid)
	{
		return Fail("Configured Sun Beam strike actor failed validation");
	}
	AbilityActorDefinition missingSunBeamVisual = AbilityData::AbilityActors::Actor_SunBeam_Basic;
	missingSunBeamVisual.visualId.clear();
	if (AbilityActorRegistry::ValidateDefinition(missingSunBeamVisual).isValid)
	{
		return Fail("Sun Beam strike actor accepted a missing visual definition");
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

	Actor abilityOwner{ nullptr };
	AttributeSystem abilityAttributes;
	GameplayTagContainer abilityTags;
	GameplayEffectSystem abilityEffects{ abilityOwner, abilityAttributes, abilityTags };
	AbilitySystem abilitySystem{ abilityOwner, abilityAttributes, abilityEffects, abilityTags };
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
	std::string sunBeamGrantFailure;
	if (!abilitySystem.GrantAbility(AbilityData::Definitions::SunBeam_Strike_Basic, &sunBeamGrantFailure).IsValid())
	{
		return Fail("Configured Sun Beam ability failed grant validation");
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
		AttributeModifier{ OwnerAttributeIds::Armor, 0.2f }
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
	if (!NearlyEqual(damageTarget.GetHealth(), 92.f))
	{
		return Fail("Combat damage did not flow through barrier, armor, and health");
	}

	const List<const PrimaryWeaponDefinition*> configuredWeapons{
		&WeaponData::Fighter_PrimaryWeaponDef,
		&WeaponData::Enemy_Vanguard_PrimaryWeaponDef,
		&WeaponData::Enemy_Vanguard_Elite_PrimaryWeaponDef,
		&WeaponData::Enemy_TwinBlade_PrimaryWeaponDef,
		&WeaponData::Enemy_Hexagon_PrimaryWeaponDef,
		&WeaponData::Enemy_UFO_PrimaryWeaponDef,
		&WeaponData::Boss_Base_PrimaryWeaponDef,
		&WeaponData::Boss_ThreeWay_PrimaryWeaponDef,
		&WeaponData::Boss_FrontalSweep_PrimaryWeaponDef,
		&WeaponData::Boss_LastStage_PrimaryWeaponDef
	};
	for (const PrimaryWeaponDefinition* weapon : configuredWeapons)
	{
		if (!weapon || !PrimaryWeaponRegistry::ValidateDefinition(*weapon).isValid)
		{
			return Fail("A shipped primary weapon definition failed validation");
		}
	}

	return 0;
}
