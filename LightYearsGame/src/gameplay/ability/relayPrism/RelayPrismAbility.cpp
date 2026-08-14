#include "gameplay/ability/relayPrism/RelayPrismAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/relayPrism/RelayPrismActor.h"
#include "gameplay/ability/relayPrism/RelayPrismContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/ability/utility/RelayPrismConfig.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		sas::GameplayAttributeList ResolveValues(GameAbilityBehaviorContext& context)
		{
			AbilityExecutionContext executionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		}

		const sas::GameplayAttribute* FindAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& attributeId
		)
		{
			return sas::FindAttribute(definition.attributes, attributeId);
		}
	}

	bool RelayPrismAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::RelayPrism::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!std::isfinite(definition.duration) || definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Relay Prism requires a loadout slot, pressed activation, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& attributeId : {
			AbilityData::RelayPrism::Attribute::ProjectileCount,
			AbilityData::RelayPrism::Attribute::DamageTransferRatio,
			AbilityData::RelayPrism::Attribute::AttackPowerCoefficient,
			AbilityData::RelayPrism::Attribute::MinimumScatterAngle,
			AbilityData::RelayPrism::Attribute::MaximumScatterAngle,
			AbilityData::RelayPrism::Attribute::MaximumBonusProjectileCount
		})
		{
			const sas::GameplayAttribute* attribute = FindAttribute(definition, attributeId);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason = "Relay Prism must declare all runtime attributes.";
				}
				return false;
			}
		}

		const float projectileCount = FindAttribute(
			definition,
			AbilityData::RelayPrism::Attribute::ProjectileCount
		)->baseValue;
		const float transferRatio = FindAttribute(
			definition,
			AbilityData::RelayPrism::Attribute::DamageTransferRatio
		)->baseValue;
		const float minimumScatter = FindAttribute(
			definition,
			AbilityData::RelayPrism::Attribute::MinimumScatterAngle
		)->baseValue;
		const float maximumScatter = FindAttribute(
			definition,
			AbilityData::RelayPrism::Attribute::MaximumScatterAngle
		)->baseValue;
		if (projectileCount < 1.f || transferRatio < 0.f || transferRatio > 1.f ||
			minimumScatter < 0.f || maximumScatter < minimumScatter ||
			maximumScatter > 360.f || definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason =
					"Relay Prism has invalid transfer, scatter or level progression values.";
			}
			return false;
		}

		const AbilityActorDefinition* relayActor = AbilityData::FindAbilityActorDefinition(
			AbilityData::RelayPrism::Actor::Relay::BasicDefinitionId
		);
		if (!relayActor)
		{
			if (failureReason)
			{
				*failureReason = "Relay Prism requires its capture actor definition.";
			}
			return false;
		}
		return true;
	}

	bool RelayPrismAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (const shared_ptr<AbilityWorldActor> existing = mRelay.lock();
			existing && !existing->GetIsPendingDestroy())
		{
			return false;
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sas::GameplayAttributeList values = ResolveValues(context);
		const weak_ptr<AbilityWorldActor> spawned = AbilityActorSpawner::Spawn(
			SpawnActorAction{
				AbilityData::RelayPrism::Actor::Relay::BasicDefinitionId,
				// Start at the owner; the mouse is only the launch target.
				sas::AbilitySpawnPolicy::AtOwner,
				sas::AbilityDirectionPolicy::MouseWorld
			},
			executionContext,
			context.owner
		);
		const shared_ptr<AbilityWorldActor> actor = spawned.lock();
		RelayPrismActor* relay = actor
			? dynamic_cast<RelayPrismActor*>(actor.get())
			: nullptr;
		if (!relay)
		{
			return false;
		}

		relay->ConfigureFromAbilityValues(values);
		mRelay = spawned;
		context.abilitySystem.AddOwnedTag(AbilityData::RelayPrism::State::Active);
		EmitEvent(context, AbilityData::RelayPrism::Event::Started);
		return true;
	}

	void RelayPrismAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (const shared_ptr<AbilityWorldActor> relay = mRelay.lock())
		{
			relay->Destroy();
		}
		mRelay.reset();
		context.abilitySystem.RemoveOwnedTag(AbilityData::RelayPrism::State::Active);
		EmitEvent(context, AbilityData::RelayPrism::Event::Ended);
	}

	void RelayPrismAbility::EmitEvent(
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
}
