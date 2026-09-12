#include "gameplay/ability/closedCircuit/ClosedCircuitAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitContracts.h"
#include "gameplay/ability/closedCircuit/ClosedCircuitFieldActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/AbilityContentCatalog.h"
#include "framework/MathUtility.h"
#include "framework/World.h"

#include <algorithm>

namespace ly
{
	namespace
	{
		float Setting(const GameAbilityDefinition& definition, const char* name, float fallback)
		{
			return content::AbilityContentCatalog::FindNumericSetting(definition.abilityId, name).value_or(fallback);
		}
	}

	bool ClosedCircuitAbility::Validate(const GameAbilityDefinition& definition, std::string* failureReason) const
	{
		const bool valid = definition.abilityId == AbilityData::ClosedCircuit::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::ClosedCircuit &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Duration &&
			definition.maxCharges == 1 && definition.duration > 0.f && definition.cooldown > 0.f &&
			definition.levelProgression.size() == 14 &&
			AbilityData::FindAbilityActorDefinition(AbilityData::ClosedCircuit::Actor::Delivery::BasicDefinitionId);
		if (!valid && failureReason)
		{
			*failureReason = "Closed Circuit requires a short cast duration, progression, and delivery actor.";
		}
		return valid;
	}

	bool ClosedCircuitAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world) return false;
		const sf::Vector2f ownerLocation = context.owner.GetActorLocation();
		const sf::Vector2f mouseLocation = world->GetMouseWorldPosition();
		sf::Vector2f direction = mouseLocation - ownerLocation;
		const float cursorDistance = GetVectorLength(direction);
		if (cursorDistance <= 0.001f) direction = context.owner.GetActorForwardDirection();
		else NormalizeVector(direction);

		const float range = std::max(1.f, Setting(context.definition, AbilityData::ClosedCircuit::Setting::MaximumDeliveryRange, 200.f));
		const sf::Vector2f target = ownerLocation + direction * std::min(range, cursorDistance);
		AbilityExecutionContext executionContext{ &context.abilitySystem, &context.definition, nullptr, &context.instance };
		const sas::GameplayAttributeList values = AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		const auto* owner = dynamic_cast<const Combatant*>(&context.owner);
		const float energyPower = owner ? std::max(0.f, owner->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)) : 0.f;
		const float health = std::max(0.f,
			sas::FindAttributeValue(values, AbilityData::ClosedCircuit::Attribute::BaseBarrierHealth, 180.f) +
			energyPower * std::max(0.f, sas::FindAttributeValue(values, AbilityData::ClosedCircuit::Attribute::EnergyPowerBarrierHealthScale, 0.60f))
		);
		mPendingDelivery = {
			target,
			Setting(context.definition, AbilityData::ClosedCircuit::Setting::DeliverySpeed, 280.f),
			Setting(context.definition, AbilityData::ClosedCircuit::Setting::FormationDuration, 0.50f),
			Setting(context.definition, AbilityData::ClosedCircuit::Setting::BarrierRadius, 250.f),
			health
		};
		mHasPendingDelivery = true;
		context.abilitySystem.AddOwnedTag(AbilityData::ClosedCircuit::State::Deploying);
		EmitEvent(context, AbilityData::ClosedCircuit::Event::Started);
		return true;
	}

	void ClosedCircuitAbility::End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason)
	{
		context.abilitySystem.RemoveOwnedTag(AbilityData::ClosedCircuit::State::Deploying);
		const bool completedCast = reason == sas::AbilityEndReason::DurationExpired ||
			reason == sas::AbilityEndReason::Completed;
		if (completedCast && mHasPendingDelivery)
		{
			AbilityExecutionContext executionContext{
				&context.abilitySystem, &context.definition, nullptr, &context.instance
			};
			if (const shared_ptr<ClosedCircuitFieldActor> delivery =
				std::dynamic_pointer_cast<ClosedCircuitFieldActor>(
					AbilityActorSpawner::SpawnAtLocation(
						AbilityData::ClosedCircuit::Actor::Delivery::BasicDefinitionId,
						executionContext,
						context.owner,
						context.owner.GetActorLocation(),
						context.owner.GetActorForwardDirection()
					).lock()
				))
			{
				delivery->ConfigureDelivery(
					mPendingDelivery.target,
					mPendingDelivery.speed,
					mPendingDelivery.formationDuration,
					mPendingDelivery.radius,
					mPendingDelivery.health
				);
			}
		}
		mHasPendingDelivery = false;
		EmitEvent(context, AbilityData::ClosedCircuit::Event::Ended);
	}

	void ClosedCircuitAbility::EmitEvent(GameAbilityBehaviorContext& context, const GameplayTag& eventTag) const
	{
		sas::AbilityEvent event;
		event.eventTag = eventTag;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(&context.owner);
		context.abilitySystem.HandleGameplayEvent(event);
	}
}
