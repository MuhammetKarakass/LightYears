#include "gameplay/ability/arcScythes/ArcScythesAbility.h"

#include "gameplay/ability/arcScythes/ArcScythesContracts.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <cmath>
#include <variant>

namespace ly
{
	namespace
	{
		constexpr float Epsilon = 0.0001f;

		bool NearlyEqual(float left, float right)
		{
			return std::isfinite(left) && std::isfinite(right) &&
				std::abs(left - right) <= Epsilon;
		}

		float ResolveArcScythesSetting(
			const std::string& abilityId,
			const std::string& settingName,
			float fallback
		)
		{
			return content::AbilityContentCatalog::FindNumericSetting(
				abilityId, settingName
			).value_or(fallback);
		}

		bool HasExpectedProgressionStep(const AbilityLevelStep& step)
		{
			if (step.attributeModifiers.size() != 2 ||
				!step.unlockedUpgradeIds.empty() ||
				!step.addedActions.empty() || !step.addedTriggers.empty())
			{
				return false;
			}
			return std::any_of(step.attributeModifiers.begin(), step.attributeModifiers.end(),
				[](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == CommonAttributeIds::Damage &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, 1.f);
				}) && std::any_of(
				step.attributeModifiers.begin(), step.attributeModifiers.end(),
				[](const sas::AttributeModifier& modifier)
				{
					return modifier.attributeId == CommonAttributeIds::Cooldown &&
						modifier.operation == sas::AttributeModifierOperation::Add &&
						NearlyEqual(modifier.magnitude, -0.20f);
				}
			);
		}
	}

	bool ArcScythesAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* beam = AbilityData::FindAbilityActorDefinition(
			AbilityData::ArcScythes::Actor::Beam::BasicDefinitionId
		);
		const bool validIdentity =
			definition.abilityId == AbilityData::ArcScythes::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::ArcScythes &&
			definition.abilityTags.size() == 2 &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::ArcScythes::CategoryTag);
				}) &&
			std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
				[](const GameplayTag& tag)
				{
					return tag.MatchesTagExact(AbilityData::ArcScythes::FamilyTag);
				});
		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.maxCharges == 1 && NearlyEqual(definition.cooldown, 14.f) &&
			NearlyEqual(definition.duration, 4.f);
		const bool validDamage = definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(DamageTypeSchema::Electric);
		const bool validScaling = definition.scalingRules.size() == 1 &&
			definition.scalingRules.front().targetAttributeId == CommonAttributeIds::Damage &&
			definition.scalingRules.front().sourceAttributeId == OwnerAttributeIds::EnergyMax &&
			definition.scalingRules.front().operation == sas::AttributeModifierOperation::Add &&
			NearlyEqual(definition.scalingRules.front().coefficient, 0.06f);
		const bool validSpawn = definition.actions.size() == 1 &&
			definition.actions.front().phase == sas::AbilityActionPhase::OnActivate &&
			std::holds_alternative<SpawnActorAction>(definition.actions.front().action) &&
			std::get<SpawnActorAction>(definition.actions.front().action).actorDefinitionId.ToString() ==
				AbilityData::ArcScythes::Actor::Beam::BasicDefinitionId &&
			std::get<SpawnActorAction>(definition.actions.front().action).spawnPolicy ==
				sas::AbilitySpawnPolicy::AtOwner;
		const bool validProgression = definition.levelProgression.size() == 14 &&
			std::all_of(definition.levelProgression.begin(), definition.levelProgression.end(),
				HasExpectedProgressionStep);
		const bool validActor = beam &&
			beam->actorType == AbilityActorType::ArcScythesBeam &&
			beam->lifeTime >= definition.duration && beam->presentationProfileId.IsValid();

		if (!validIdentity || !validLifecycle || !validDamage || !validScaling ||
			!validSpawn || !validProgression || !validActor ||
			!definition.attributes.empty() || !definition.effectSpecs.empty() ||
			!definition.triggers.empty())
		{
			if (failureReason)
			{
				*failureReason =
					"Arc Scythes requires its four-second Electric dual-beam lifecycle, "
					"EnergyMax damage scaling, and fourteen damage/cooldown progression steps.";
			}
			return false;
		}
		return true;
	}

	bool ArcScythesAbility::Activate(GameAbilityBehaviorContext& context)
	{
		mActiveTime = 0.f;
		mStarted = true;
		context.abilitySystem.AddOwnedTag(AbilityData::ArcScythes::State::Active);
		return true;
	}

	void ArcScythesAbility::Tick(GameAbilityBehaviorContext& context, float deltaTime)
	{
		if (!mStarted)
		{
			return;
		}

		mActiveTime += std::max(0.f, deltaTime);
		const float minActiveDuration = ResolveArcScythesSetting(
			context.definition.abilityId,
			AbilityData::ArcScythes::Setting::MinActiveDuration,
			1.f
		);
		// Re-presses before the lock expires are deliberately ignored. They neither
		// queue nor shorten the promised one-second minimum beam duration.
		if (context.instance.IsPressedThisFrame() && mActiveTime >= minActiveDuration)
		{
			context.instance.Cancel(sas::AbilityEndReason::Cancelled);
		}
	}

	void ArcScythesAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mStarted)
		{
			return;
		}

		context.abilitySystem.RemoveOwnedTag(AbilityData::ArcScythes::State::Active);
		mStarted = false;
	}
}
