#include "gameplay/ability/nanoPlague/NanoPlagueAbility.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/ability/nanoPlague/NanoPlagueControllerActor.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetingTypes.h"
#include "framework/World.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/nanoPlague/NanoPlaguePresentationIds.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr std::size_t RequiredAttributeCount = 8;
		constexpr std::size_t RequiredProgressionStepCount = 14;

		bool HasExactAbilityTags(const GameAbilityDefinition& definition)
		{
			return definition.abilityTags.size() == 2 &&
				std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
					[](const GameplayTag& tag)
					{
						return tag.MatchesTagExact(AbilityData::NanoPlague::CategoryTag);
					}) &&
				std::any_of(definition.abilityTags.begin(), definition.abilityTags.end(),
					[](const GameplayTag& tag)
					{
						return tag.MatchesTagExact(AbilityData::NanoPlague::FamilyTag);
					});
		}

		bool HasAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId,
			float minimumValue
		)
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				attributeId
			);
			return attribute && std::isfinite(attribute->baseValue) &&
				attribute->baseValue >= minimumValue &&
				attribute->baseValue >= attribute->minValue &&
				attribute->baseValue <= attribute->maxValue;
		}
	}

	bool NanoPlagueAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validIdentity =
			definition.abilityId == AbilityData::NanoPlague::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::NanoPlague &&
			HasExactAbilityTags(definition) &&
			definition.damageTags.size() == 1 &&
			definition.damageTags.front().MatchesTagExact(DamageTypeSchema::Electric);
		const bool validLifecycle =
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 && definition.duration == 0.f &&
			definition.cooldown > 0.f;
		const bool validAttributes = definition.attributes.size() == RequiredAttributeCount &&
			HasAttribute(definition, AbilityData::NanoPlague::Attribute::BaseTickDamage, 0.f) &&
			HasAttribute(definition, AbilityData::NanoPlague::Attribute::Duration, 0.01f) &&
			HasAttribute(definition, AbilityData::NanoPlague::Attribute::TickInterval, 0.01f) &&
			HasAttribute(definition, AbilityData::NanoPlague::Attribute::InitialTargetRange, 0.01f) &&
			HasAttribute(definition, AbilityData::NanoPlague::Attribute::SpreadRadius, 0.01f) &&
			HasAttribute(definition, AbilityData::NanoPlague::Attribute::EnergyPowerTickScale, 0.f) &&
			HasAttribute(definition, AbilityData::NanoPlague::Attribute::BaseSpreadTargetCount, 1.f) &&
			HasAttribute(definition, AbilityData::NanoPlague::Attribute::MaximumSpreadTargetCount, 1.f);
		const bool validRuntimeOwnership = definition.actions.empty() &&
			definition.triggers.empty() && definition.effectSpecs.empty() &&
			definition.scalingRules.empty();
		const bool validProgression = definition.levelProgression.size() ==
			RequiredProgressionStepCount;

		if (!validIdentity || !validLifecycle || !validAttributes ||
			!validRuntimeOwnership || !validProgression)
		{
			if (failureReason)
			{
				*failureReason =
					"Nano Plague requires an instant electric infection with eight runtime attributes and fourteen progression steps.";
			}
			return false;
		}
		return true;
	}

	bool NanoPlagueAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world)
		{
			return false;
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem, &context.definition, nullptr, &context.instance
		};
		const sas::GameplayAttributeList values =
			AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		const float targetRange = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::NanoPlague::Attribute::InitialTargetRange,
			AbilityData::NanoPlague::DefaultInitialTargetRange
		));

		targeting::TargetingQuery query;
		query.source = &context.owner;
		query.origin = world->GetMouseWorldPosition();
		query.range = targetRange;
		query.requiredTargetLayers = context.owner.GetCollisionLayer() == CollisionLayer::Player
			? CollisionLayer::Enemy : CollisionLayer::Player;
		query.requireCollisionCompatibility = true;
		query.filter = [](const Actor*, const Actor& candidate, const targeting::TargetingCandidate&)
		{
			return dynamic_cast<const Combatant*>(&candidate) != nullptr;
		};
		const shared_ptr<Actor> target = targeting::AutoTargeting::FindTarget(*world, query).lock();
		if (!target)
		{
			return false;
		}

		const NanoPlaguePresentationProfile* profile =
			PresentationProfileRegistry<NanoPlaguePresentationProfile>::Find(
				NanoPlaguePresentationIds::InfectionBasic
			);
		if (!profile)
		{
			return false;
		}

		NanoPlagueControllerActor::Settings settings;
		settings.baseTickDamage = std::max(0.f, sas::FindAttributeValue(
			values, AbilityData::NanoPlague::Attribute::BaseTickDamage,
			AbilityData::NanoPlague::DefaultBaseTickDamage));
		settings.energyPowerTickScale = std::max(0.f, sas::FindAttributeValue(
			values, AbilityData::NanoPlague::Attribute::EnergyPowerTickScale,
			AbilityData::NanoPlague::DefaultEnergyPowerTickScale));
		settings.duration = std::max(0.01f, sas::FindAttributeValue(
			values, AbilityData::NanoPlague::Attribute::Duration,
			AbilityData::NanoPlague::DefaultDuration));
		settings.tickInterval = std::max(0.01f, sas::FindAttributeValue(
			values, AbilityData::NanoPlague::Attribute::TickInterval,
			AbilityData::NanoPlague::DefaultTickInterval));
		settings.spreadRadius = std::max(0.f, sas::FindAttributeValue(
			values, AbilityData::NanoPlague::Attribute::SpreadRadius,
			AbilityData::NanoPlague::DefaultSpreadRadius));
		settings.baseSpreadTargetCount = std::max(1, static_cast<int>(std::lround(
			sas::FindAttributeValue(values, AbilityData::NanoPlague::Attribute::BaseSpreadTargetCount,
				AbilityData::NanoPlague::DefaultBaseSpreadTargetCount))));
		settings.maximumSpreadTargetCount = std::max(settings.baseSpreadTargetCount,
			static_cast<int>(std::lround(sas::FindAttributeValue(values,
				AbilityData::NanoPlague::Attribute::MaximumSpreadTargetCount,
				AbilityData::NanoPlague::DefaultMaximumSpreadTargetCount))));
		settings.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		settings.sourceAbilityTags = context.definition.abilityTags;

		const shared_ptr<NanoPlagueControllerActor> controller =
			NanoPlagueControllerActor::FindOrCreate(*world, context.owner, *profile);
		return controller && controller->ApplyOrRefreshInfection(*target, 0, settings);
	}
}
