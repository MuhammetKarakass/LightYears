#include "gameplay/ability/reclaimerProtocol/ReclaimerProtocolAbility.h"

#include "gameplay/ability/reclaimerProtocol/ReclaimerProtocolContracts.h"
#include "gameplay/ability/reclaimerProtocol/ReclaimerRepairKitActor.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/tags/GameplayTags.h"
#include "framework/Actor.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		bool HasFiniteAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& id
		)
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				id
			);
			return attribute && std::isfinite(attribute->baseValue) && attribute->baseValue > 0.f;
		}
	}

	bool ReclaimerProtocolAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::ReclaimerProtocol::AbilityId::Basic ||
			definition.behaviorType != AbilityBehaviorType::ReclaimerProtocol ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || definition.duration <= 0.f ||
			definition.cooldown <= 0.f || definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason =
					"Reclaimer Protocol requires a duration defensive ability with one charge, positive duration and cooldown, and fourteen progression steps.";
			}
			return false;
		}

		if (!HasFiniteAttribute(definition, AbilityData::ReclaimerProtocol::Attribute::HealRatio))
		{
			if (failureReason)
			{
				*failureReason = "Reclaimer Protocol must declare a valid positive HealRatio attribute.";
			}
			return false;
		}
		return true;
	}

	bool ReclaimerProtocolAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mActive)
		{
			return false;
		}
		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::ReclaimerProtocol::State::Active);
		EmitEvent(context, AbilityData::ReclaimerProtocol::Event::Started);
		return true;
	}

	void ReclaimerProtocolAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		(void)context;
		(void)deltaTime;
	}

	void ReclaimerProtocolAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mActive)
		{
			return;
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::ReclaimerProtocol::State::Active);
		mActive = false;
		EmitEvent(context, AbilityData::ReclaimerProtocol::Event::Ended);
	}

	void ReclaimerProtocolAbility::OnGameplayEvent(
		GameAbilityBehaviorContext& context,
		const sas::AbilityEvent& event
	)
	{
		if (!mActive || event.eventTag != GameplayTags::Event::Combat::KillConfirmed ||
			event.GetSource<Actor>() != &context.owner)
		{
			return;
		}

		const DamageContext* damageContext = event.GetContext<DamageContext>();
		if (!damageContext || !damageContext->targetWasKilled ||
			!damageContext->targetWasEnemyCombatant)
		{
			return;
		}

		if (!std::isfinite(damageContext->targetLocationAtResolution.x) ||
			!std::isfinite(damageContext->targetLocationAtResolution.y))
		{
			return;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float snapshottedHealRatio = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::ReclaimerProtocol::Attribute::HealRatio,
			AbilityData::ReclaimerProtocol::DefaultHealRatio
		));

		SpawnRepairKitAtLocation(
			context,
			{
				damageContext->targetLocationAtResolution.x,
				damageContext->targetLocationAtResolution.y
			},
			snapshottedHealRatio
		);
	}

	sas::GameplayAttributeList ReclaimerProtocolAbility::ResolveValues(
		GameAbilityBehaviorContext& context
	) const
	{
		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		return AbilityActionAttributeResolver::ResolveAbilityAttributes(
			executionContext
		);
	}

	void ReclaimerProtocolAbility::SpawnRepairKitAtLocation(
		GameAbilityBehaviorContext& context,
		const sf::Vector2f& location,
		float snapshottedHealRatio
	)
	{
		World* world = context.owner.GetWorld();
		if (!world)
		{
			return;
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};

		const weak_ptr<AbilityWorldActor> spawned =
			AbilityActorSpawner::SpawnAtLocation(
				AbilityData::ReclaimerProtocol::Actor::RepairKit::BasicDefinitionId,
				executionContext,
				context.owner,
				location,
				{ 0.f, -1.f },
				1.f
			);

		const shared_ptr<ReclaimerRepairKitActor> kit =
			std::dynamic_pointer_cast<ReclaimerRepairKitActor>(spawned.lock());
		if (!kit)
		{
			return;
		}

		kit->SetResolvedHealRatio(snapshottedHealRatio);
		EmitEvent(context, AbilityData::ReclaimerProtocol::Event::RepairKitSpawned);
	}

	void ReclaimerProtocolAbility::EmitEvent(
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