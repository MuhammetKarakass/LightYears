#include "gameplay/ability/orbitalDrones/OrbitalDronesAbility.h"

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/orbitalDrones/OrbitalDronesContracts.h"
#include "gameplay/ability/orbitalDrones/OrbitingDroneActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/orbitalDrones/OrbitalDronesPresentationIds.h"
#include "presentation/ability/orbitalDrones/OrbitalDronesPresentationProfile.h"
#include "framework/World.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr std::size_t RequiredDroneCount = 4;
		constexpr std::size_t RequiredAttributeCount = 8;
		constexpr std::size_t RequiredProgressionStepCount = 14;
		constexpr float BaseCooldown = 12.f;
		constexpr float BaseDuration = 6.f;
		constexpr float BaseOrbitRadius = 200.f;
		constexpr float BaseDamage = 18.f;
		constexpr float BaseSameTargetHitCooldown = 0.5f;
		constexpr float BaseAngularSpeed = 2.5f;
		constexpr float BaseContactRadius = 12.f;
		constexpr float BaseEnergyReference = 50.f;
		constexpr float BaseEnergyDurationScale = 0.02f;
		constexpr float DamagePerLevel = 2.f;
		constexpr float CooldownPerLevel = -0.25f;
		constexpr float Epsilon = 0.0001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		const sas::GameplayAttribute* FindAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}

		bool HasValidAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId,
			float minimumBaseValue,
			bool requireWholeNumber = false
		)
		{
			const sas::GameplayAttribute* attribute = FindAttribute(
				definition,
				attributeId
			);
			if (!attribute ||
				!std::isfinite(attribute->baseValue) ||
				!std::isfinite(attribute->currentValue) ||
				!std::isfinite(attribute->minValue) ||
				!std::isfinite(attribute->maxValue) ||
				attribute->minValue > attribute->maxValue ||
				attribute->baseValue < minimumBaseValue ||
				attribute->baseValue < attribute->minValue ||
				attribute->baseValue > attribute->maxValue)
			{
				return false;
			}
			return !requireWholeNumber ||
				std::round(attribute->baseValue) == attribute->baseValue;
		}

		bool HasExactAbilityTags(const GameAbilityDefinition& definition)
		{
			if (definition.abilityTags.size() != 2)
			{
				return false;
			}

			bool hasCategory = false;
			bool hasFamily = false;
			for (const GameplayTag& tag : definition.abilityTags)
			{
				hasCategory = hasCategory || tag.MatchesTagExact(
					AbilityData::OrbitalDrones::CategoryTag
				);
				hasFamily = hasFamily || tag.MatchesTagExact(
					AbilityData::OrbitalDrones::FamilyTag
				);
			}
			return hasCategory && hasFamily;
		}

		bool HasExpectedScaling(const GameAbilityDefinition& definition)
		{
			if (definition.scalingRules.size() != 1)
			{
				return false;
			}

			const sas::AttributeScalingRule& rule = definition.scalingRules.front();
			return rule.targetAttributeId == CommonAttributeIds::Damage &&
				rule.sourceAttributeId == OwnerAttributeIds::AttackPower &&
				rule.operation == sas::AttributeModifierOperation::Add &&
				NearlyEqual(rule.coefficient, 0.50f);
		}

		bool HasExpectedModifier(
			const AbilityLevelStep& step,
			const sas::AttributeId& attributeId,
			float expectedMagnitude
		)
		{
			for (const sas::AttributeModifier& modifier : step.attributeModifiers)
			{
				if (modifier.attributeId == attributeId &&
					modifier.operation == sas::AttributeModifierOperation::Add &&
					NearlyEqual(modifier.magnitude, expectedMagnitude))
				{
					return true;
				}
			}
			return false;
		}

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			const float value = sas::FindAttributeValue(values, attributeId, fallback);
			return std::isfinite(value) ? value : fallback;
		}

		sas::GameplayAttributeList ResolveAbilityValues(
			const GameAbilityBehaviorContext& context
		)
		{
			AbilityExecutionContext executionContext{
				const_cast<LightYearsAbilitySystemComponent*>(&context.abilitySystem),
				&context.definition,
				nullptr,
				const_cast<GameAbility*>(&context.instance)
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(
				executionContext
			);
		}

		float ResolveEnergyDurationBonus(
			const GameAbilityBehaviorContext& context,
			const sas::GameplayAttributeList& values
		)
		{
			const float energyReference = std::max(
				0.f,
				FindValue(
					values,
					AbilityData::OrbitalDrones::Attribute::EnergyMaxReference,
					BaseEnergyReference
				)
			);
			const float durationScale = std::max(
				0.f,
				FindValue(
					values,
					AbilityData::OrbitalDrones::Attribute::EnergyMaxDurationScale,
					BaseEnergyDurationScale
				)
			);
			const float energyMax = context.abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::EnergyMax
			);
			if (!std::isfinite(energyMax))
			{
				return 0.f;
			}

			return std::max(0.f, energyMax - energyReference) * durationScale;
		}
	}

	bool OrbitalDronesAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::OrbitalDrones::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::OrbitalDrones &&
			HasExactAbilityTags(definition);
		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.maxCharges == 1 &&
			NearlyEqual(definition.cooldown, BaseCooldown) &&
			NearlyEqual(definition.duration, BaseDuration);

		if (!validIdentity || !validLifecycle)
		{
			if (failureReason)
			{
				*failureReason =
					"Orbital Drones requires a loadout slot, pressed activation, and its exact lifecycle.";
			}
			return false;
		}

		const bool validAttributes =
			definition.attributes.size() == RequiredAttributeCount &&
			HasValidAttribute(
				definition,
				AbilityData::OrbitalDrones::Attribute::Radius,
				0.1f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::OrbitalDrones::Attribute::Damage,
				0.f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::OrbitalDrones::Attribute::DroneCount,
				1.f,
				true
			) &&
			HasValidAttribute(
				definition,
				AbilityData::OrbitalDrones::Attribute::SameTargetHitCooldown,
				0.f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::OrbitalDrones::Attribute::BaseAngularSpeedRadiansPerSecond,
				0.f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::OrbitalDrones::Attribute::ContactRadius,
				0.1f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::OrbitalDrones::Attribute::EnergyMaxReference,
				0.f
			) &&
			HasValidAttribute(
				definition,
				AbilityData::OrbitalDrones::Attribute::EnergyMaxDurationScale,
				0.f
			);
		if (!validAttributes)
		{
			if (failureReason)
			{
				*failureReason =
					"Orbital Drones must declare exactly its eight shipped runtime attributes.";
			}
			return false;
		}

		if (definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Kinetic ||
			!HasExpectedScaling(definition))
		{
			if (failureReason)
			{
				*failureReason =
					"Orbital Drones requires Kinetic damage and only Common.Damage + 0.50 Owner.AttackPower scaling.";
			}
			return false;
		}

		if (definition.levelProgression.size() != RequiredProgressionStepCount)
		{
			if (failureReason)
			{
				*failureReason =
					"Orbital Drones requires fourteen progression steps through level fifteen.";
			}
			return false;
		}

		float resolvedCooldown = definition.cooldown;
		for (const AbilityLevelStep& step : definition.levelProgression)
		{
			if (step.attributeModifiers.size() != 2 ||
				!step.unlockedUpgradeIds.empty() ||
				!step.addedActions.empty() ||
				!step.addedTriggers.empty() ||
				!HasExpectedModifier(step, CommonAttributeIds::Damage, DamagePerLevel) ||
				!HasExpectedModifier(step, CommonAttributeIds::Cooldown, CooldownPerLevel))
			{
				if (failureReason)
				{
					*failureReason =
						"Orbital Drones progression may only add 2 Common.Damage and -0.25 Common.Cooldown per level.";
				}
				return false;
			}

			resolvedCooldown += CooldownPerLevel;
			if (resolvedCooldown <= 0.f)
			{
				if (failureReason)
				{
					*failureReason =
						"Orbital Drones progression must keep cooldown positive at every level.";
				}
				return false;
			}
		}

		return true;
	}

	float OrbitalDronesAbility::ResolveActiveDuration(
		const GameAbilityBehaviorContext& context,
		float defaultDuration
	) const
	{
		const sas::GameplayAttributeList values = ResolveAbilityValues(context);
		const float safeDefaultDuration = std::max(
			0.f,
			std::isfinite(defaultDuration) ? defaultDuration : BaseDuration
		);
		return safeDefaultDuration + ResolveEnergyDurationBonus(context, values);
	}

	bool OrbitalDronesAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mActive || !dynamic_cast<SpaceShip*>(&context.owner))
		{
			return false;
		}

		World* world = context.owner.GetWorld();
		const OrbitalDronesPresentationProfile* presentationProfile =
			PresentationProfileRegistry<OrbitalDronesPresentationProfile>::Find(
				OrbitalDronesPresentationIds::DroneBasic
			);
		if (!world || !presentationProfile)
		{
			return false;
		}

		const sas::GameplayAttributeList abilityValues = ResolveAbilityValues(context);
		const float droneCountValue = FindValue(
			abilityValues,
			AbilityData::OrbitalDrones::Attribute::DroneCount,
			static_cast<float>(RequiredDroneCount)
		);
		if (!NearlyEqual(droneCountValue, static_cast<float>(RequiredDroneCount)))
		{
			return false;
		}

		// GetActiveDuration() invokes the same GameAbility virtual duration path
		// that BeginActivation() immediately uses after Activate() returns. This
		// keeps actor lifetime identical to the ability's runtime duration.
		const float resolvedDuration = context.instance.GetActiveDuration();
		if (!std::isfinite(resolvedDuration) || resolvedDuration <= 0.f)
		{
			return false;
		}

		const float orbitRadius = std::max(
			0.f,
			FindValue(
				abilityValues,
				AbilityData::OrbitalDrones::Attribute::Radius,
				BaseOrbitRadius
			)
		);
		const float baseAngularSpeed = std::max(
			0.f,
			FindValue(
				abilityValues,
				AbilityData::OrbitalDrones::Attribute::BaseAngularSpeedRadiansPerSecond,
				BaseAngularSpeed
			)
		);
		// Angular speed is a family-owned formation value. Orbital Drones does
		// not declare an Owner.AttackSpeed scaling rule, and the owner attribute
		// can legitimately be zero before progression is applied. Multiplying by
		// that value would silently freeze every drone at its spawn phase.
		const float angularSpeed = baseAngularSpeed;
		const float contactRadius = std::max(
			0.f,
			FindValue(
				abilityValues,
				AbilityData::OrbitalDrones::Attribute::ContactRadius,
				BaseContactRadius
			)
		);
		const float sameTargetHitCooldown = std::max(
			0.f,
			FindValue(
				abilityValues,
				AbilityData::OrbitalDrones::Attribute::SameTargetHitCooldown,
				BaseSameTargetHitCooldown
			)
		);
		const float damage = std::max(
			0.f,
			FindValue(
				abilityValues,
				AbilityData::OrbitalDrones::Attribute::Damage,
				BaseDamage
			)
		);
		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		const DamagePayload damagePayload = DamageTypeSystem::BuildPayload(
			damageTags,
			abilityValues
		);

		mDrones.clear();
		for (std::size_t droneIndex = 0; droneIndex < RequiredDroneCount; ++droneIndex)
		{
			const float phase = OrbitingDroneActor::CalculateFormationPhase(
				droneIndex,
				RequiredDroneCount
			);
			const OrbitingDroneActor::OrbitConfiguration orbitConfiguration{
				orbitRadius,
				angularSpeed,
				phase
			};
			const weak_ptr<OrbitingDroneActor> droneWeak =
				world->SpawnActor<OrbitingDroneActor>(
					&context.owner,
					*presentationProfile,
					orbitConfiguration
				);
			const shared_ptr<OrbitingDroneActor> drone = droneWeak.lock();
			if (!drone)
			{
				DestroyDrones();
				return false;
			}

			// The behavior supplies all resolved combat metadata before the pending
			// actor enters the world, so contact callbacks cannot observe defaults.
			drone->SetDamage(damage);
			drone->SetDamageTags(damageTags);
			drone->SetDamageAttributes(abilityValues);
			drone->SetDamagePayload(damagePayload);
			drone->SetAbilityUpgradeIds(context.definition.unlockedUpgradeIds);
			drone->SetSourceAbility(
				sas::ContentId{ context.definition.abilityId },
				context.definition.abilityTags
			);
			drone->SetLifeTime(resolvedDuration);
			drone->SetOrbitConfiguration(orbitConfiguration);
			drone->SetContactRadius(contactRadius);
			drone->SetSameTargetHitCooldown(sameTargetHitCooldown);
			mDrones.push_back(droneWeak);
		}

		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::OrbitalDrones::State::Active);
		EmitEvent(context, AbilityData::OrbitalDrones::Event::Started);
		return true;
	}

	void OrbitalDronesAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		DestroyDrones();
		if (!mActive)
		{
			return;
		}

		context.abilitySystem.RemoveOwnedTag(AbilityData::OrbitalDrones::State::Active);
		mActive = false;
		EmitEvent(context, AbilityData::OrbitalDrones::Event::Ended);
	}

	void OrbitalDronesAbility::EmitEvent(
		GameAbilityBehaviorContext& context,
		const GameplayTag& eventTag
	) const
	{
		sas::AbilityEvent event;
		event.eventTag = eventTag;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(&context.owner);
		context.abilitySystem.HandleGameplayEvent(event);
	}

	void OrbitalDronesAbility::DestroyDrones()
	{
		for (const weak_ptr<OrbitingDroneActor>& droneWeak : mDrones)
		{
			if (const shared_ptr<OrbitingDroneActor> drone = droneWeak.lock())
			{
				drone->Destroy();
			}
		}
		mDrones.clear();
	}
}
