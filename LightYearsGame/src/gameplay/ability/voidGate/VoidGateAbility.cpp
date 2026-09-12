#include "gameplay/ability/voidGate/VoidGateAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/voidGate/VoidGateContracts.h"
#include "gameplay/ability/voidGate/VoidGatePortalActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"

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
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(
				executionContext
			);
		}

		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& id,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, id, fallback);
		}
	}

	bool VoidGateAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::VoidGate::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 ||
			definition.recordInAbilityHistory ||
			!std::isfinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!std::isfinite(definition.duration) || definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Void Gate requires pressed activation, duration lifetime, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::VoidGate::Attribute::PortalRadius,
			AbilityData::VoidGate::Attribute::TransferDuration,
			AbilityData::VoidGate::Attribute::ReentryCooldown,
			AbilityData::VoidGate::Attribute::PortalBPlacementTimeout,
			AbilityData::VoidGate::Attribute::EnergyPowerReference,
			AbilityData::VoidGate::Attribute::EnergyPowerDurationScale
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				required
			);
			if (!attribute || !std::isfinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason =
						"Void Gate must declare every runtime attribute.";
				}
				return false;
			}
		}

		const auto value = [&](const sas::AttributeId& id)
		{
			return FindValue(definition.attributes, id, 0.f);
		};
		if (value(AbilityData::VoidGate::Attribute::PortalRadius) <= 0.f ||
			value(AbilityData::VoidGate::Attribute::TransferDuration) <= 0.f ||
			value(AbilityData::VoidGate::Attribute::ReentryCooldown) < 0.f ||
			value(AbilityData::VoidGate::Attribute::PortalBPlacementTimeout) <= 0.f ||
			value(AbilityData::VoidGate::Attribute::EnergyPowerReference) < 0.f ||
			value(AbilityData::VoidGate::Attribute::EnergyPowerDurationScale) < 0.f ||
			definition.levelProgression.size() != 14)
		{
			if (failureReason)
			{
				*failureReason = "Void Gate contains invalid portal or progression values.";
			}
			return false;
		}

		if (!AbilityData::FindAbilityActorDefinition(
			AbilityData::VoidGate::Actor::Portal::BasicDefinitionId
		))
		{
			if (failureReason)
			{
				*failureReason = "Void Gate requires its portal actor definition.";
			}
			return false;
		}
		return true;
	}

	float VoidGateAbility::ResolveActiveDuration(
		const GameAbilityBehaviorContext& context,
		float defaultDuration
	) const
	{
		GameAbilityBehaviorContext mutableContext{
			const_cast<LightYearsAbilitySystemComponent&>(context.abilitySystem),
			const_cast<GameAbility&>(context.instance),
			const_cast<Actor&>(context.owner),
			context.definition
		};
		const sas::GameplayAttributeList values = ResolveValues(mutableContext);
		const float energyPower = std::max(
			0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(
				OwnerAttributeIds::EnergyPower
			)
		);
		const float reference = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::VoidGate::Attribute::EnergyPowerReference,
				AbilityData::VoidGate::DefaultEnergyPowerReference
			)
		);
		const float scale = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::VoidGate::Attribute::EnergyPowerDurationScale,
				AbilityData::VoidGate::DefaultEnergyPowerDurationScale
			)
		);
		return std::max(0.f, defaultDuration) +
			std::max(0.f, energyPower - reference) * scale;
	}

	bool VoidGateAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		if (!ship || !context.owner.GetWorld() || !mPortalA.expired() ||
			ship->GetMovementComponent().IsMovementBurstActive())
		{
			return false;
		}

		const shared_ptr<VoidGatePortalActor> portal = SpawnPortal(
			context,
			context.owner.GetWorld()->GetMouseWorldPosition()
		).lock();
		if (!portal)
		{
			return false;
		}

		// The first click only records a portal position. It deliberately has no
		// transfer pair yet, so this low-alpha marker cannot act as a portal.
		portal->SetActivationAlpha(0.25f);
		mPortalA = portal;
		mPortalB.reset();
		mPairId = 0;
		mWaitingForPortalB = true;
		mPortalBPlacementElapsed = 0.f;
		mPortalBPlacementTimeout = std::max(
			0.01f,
			FindValue(
				ResolveValues(context),
				AbilityData::VoidGate::Attribute::PortalBPlacementTimeout,
				AbilityData::VoidGate::DefaultPortalBPlacementTimeout
			)
		);
		context.instance.DeferActiveDurationStart();
		context.abilitySystem.AddOwnedTag(AbilityData::VoidGate::State::WaitingForPortalB);
		EmitEvent(context, AbilityData::VoidGate::Event::Started);
		EmitEvent(context, AbilityData::VoidGate::Event::PortalAPlaced);
		return true;
	}

	bool VoidGateAbility::OnInputPressed(GameAbilityBehaviorContext& context)
	{
		if (mWaitingForPortalB)
		{
			const shared_ptr<VoidGatePortalActor> first = mPortalA.lock();
			const sf::Vector2f location = context.owner.GetWorld()
				? context.owner.GetWorld()->GetMouseWorldPosition()
				: context.owner.GetActorLocation();
			const shared_ptr<VoidGatePortalActor> second = SpawnPortal(
				context,
				location
			).lock();
			if (!first || !second)
			{
				return true;
			}

			const sas::GameplayAttributeList values = ResolveValues(context);
			const float radius = std::max(
				1.f,
				FindValue(
					values,
					AbilityData::VoidGate::Attribute::PortalRadius,
					AbilityData::VoidGate::DefaultPortalRadius
				)
			);
			const float transferDuration = std::max(
				0.01f,
				FindValue(
					values,
					AbilityData::VoidGate::Attribute::TransferDuration,
					AbilityData::VoidGate::DefaultTransferDuration
				)
			);
			const float reentryCooldown = std::max(
				0.f,
				FindValue(
					values,
					AbilityData::VoidGate::Attribute::ReentryCooldown,
					AbilityData::VoidGate::DefaultReentryCooldown
				)
			);
			mPortalB = second;
			// Both endpoints are known now: promote the preview and the new portal
			// to their normal visual strength immediately before enabling transfer.
			first->SetActivationAlpha(1.f);
			second->SetActivationAlpha(1.f);
			mPairId = PortalTransferService::CreatePair(
				*context.owner.GetWorld(),
				*first,
				*second,
				radius,
				transferDuration,
				reentryCooldown
			);
			mWaitingForPortalB = false;
			mPortalBPlacementElapsed = 0.f;
			context.abilitySystem.RemoveOwnedTag(
				AbilityData::VoidGate::State::WaitingForPortalB
			);
			context.abilitySystem.AddOwnedTag(AbilityData::VoidGate::State::Active);
			context.instance.StartDeferredActiveDuration(
				context.instance.GetActiveDuration()
			);
			EmitEvent(context, AbilityData::VoidGate::Event::PortalBPlaced);
			return true;
		}

		context.instance.Cancel(sas::AbilityEndReason::Cancelled);
		return true;
	}

	void VoidGateAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (!mWaitingForPortalB)
		{
			return;
		}

		mPortalBPlacementElapsed += std::max(0.f, deltaTime);
		if (mPortalBPlacementElapsed >= mPortalBPlacementTimeout)
		{
			// A staged ability must not reserve an active instance indefinitely if
			// the player never supplies its second placement input.
			context.instance.Cancel(sas::AbilityEndReason::Cancelled);
		}
	}

	void VoidGateAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (mPairId != 0 && context.owner.GetWorld())
		{
			PortalTransferService::ClosePair(*context.owner.GetWorld(), mPairId);
		}
		if (const shared_ptr<VoidGatePortalActor> portal = mPortalA.lock())
		{
			portal->Destroy();
		}
		if (const shared_ptr<VoidGatePortalActor> portal = mPortalB.lock())
		{
			portal->Destroy();
		}
		mPortalA.reset();
		mPortalB.reset();
		mPairId = 0;
		mWaitingForPortalB = false;
		mPortalBPlacementElapsed = 0.f;
		context.abilitySystem.RemoveOwnedTag(
			AbilityData::VoidGate::State::WaitingForPortalB
		);
		context.abilitySystem.RemoveOwnedTag(AbilityData::VoidGate::State::Active);
		EmitEvent(context, AbilityData::VoidGate::Event::Ended);
	}

	weak_ptr<VoidGatePortalActor> VoidGateAbility::SpawnPortal(
		GameAbilityBehaviorContext& context,
		const sf::Vector2f& location
	) const
	{
		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const weak_ptr<AbilityWorldActor> spawned = AbilityActorSpawner::SpawnAtLocation(
			AbilityData::VoidGate::Actor::Portal::BasicDefinitionId,
			executionContext,
			context.owner,
			location,
			context.owner.GetActorForwardDirection()
		);
		return std::dynamic_pointer_cast<VoidGatePortalActor>(spawned.lock());
	}

	void VoidGateAbility::EmitEvent(
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
