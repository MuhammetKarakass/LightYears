#include "gameplay/ability/frostMaelstrom/FrostMaelstromAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "gameplay/ability/frostMaelstrom/FrostMaelstromContracts.h"
#include "gameplay/ability/frostMaelstrom/FrostMaelstromFieldActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "spaceShip/SpaceShip.h"

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

		bool IsFinite(float value)
		{
			return std::isfinite(value);
		}

		bool HasFiniteAttribute(
			const GameAbilityDefinition& definition,
			const sas::AttributeId& id
		)
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				id
			);
			return attribute && IsFinite(attribute->baseValue);
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

	bool FrostMaelstromAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::FrostMaelstrom::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 ||
			!IsFinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!IsFinite(definition.duration) || definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Frost Maelstrom requires pressed activation, duration lifetime, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::FrostMaelstrom::Attribute::MinimumRadius,
			AbilityData::FrostMaelstrom::Attribute::MaximumRadius,
			AbilityData::FrostMaelstrom::Attribute::MinimumMovementSpeed,
			AbilityData::FrostMaelstrom::Attribute::MaximumMovementSpeed,
			AbilityData::FrostMaelstrom::Attribute::TickInterval,
			AbilityData::FrostMaelstrom::Attribute::CryoStacksPerTick,
			AbilityData::FrostMaelstrom::Attribute::OrbitalAngularSpeed,
			AbilityData::FrostMaelstrom::Attribute::InwardForce,
			AbilityData::FrostMaelstrom::Attribute::OrbitalRadiusRatio,
			AbilityData::FrostMaelstrom::Attribute::EnergyPowerReference,
			AbilityData::FrostMaelstrom::Attribute::EnergyPowerDamageScale,
			AbilityData::FrostMaelstrom::Attribute::EnergyPowerRadiusScale,
			CommonAttributeIds::Damage
		})
		{
			if (!HasFiniteAttribute(definition, required))
			{
				if (failureReason)
				{
					*failureReason =
						"Frost Maelstrom must declare every runtime attribute.";
				}
				return false;
			}
		}

		const auto value = [&](const sas::AttributeId& id)
		{
			return sas::FindAttributeValue(definition.attributes, id, 0.f);
		};
		if (value(AbilityData::FrostMaelstrom::Attribute::MinimumRadius) <= 0.f ||
			value(AbilityData::FrostMaelstrom::Attribute::MaximumRadius) <
				value(AbilityData::FrostMaelstrom::Attribute::MinimumRadius) ||
			value(AbilityData::FrostMaelstrom::Attribute::MinimumMovementSpeed) < 0.f ||
			value(AbilityData::FrostMaelstrom::Attribute::MaximumMovementSpeed) <
				value(AbilityData::FrostMaelstrom::Attribute::MinimumMovementSpeed) ||
			value(AbilityData::FrostMaelstrom::Attribute::TickInterval) <= 0.f ||
			value(AbilityData::FrostMaelstrom::Attribute::CryoStacksPerTick) != 1.f ||
			value(AbilityData::FrostMaelstrom::Attribute::OrbitalAngularSpeed) < 0.f ||
			value(AbilityData::FrostMaelstrom::Attribute::InwardForce) < 0.f ||
			value(AbilityData::FrostMaelstrom::Attribute::OrbitalRadiusRatio) <= 0.f ||
			value(AbilityData::FrostMaelstrom::Attribute::OrbitalRadiusRatio) > 1.f ||
			value(AbilityData::FrostMaelstrom::Attribute::EnergyPowerReference) < 0.f ||
			value(AbilityData::FrostMaelstrom::Attribute::EnergyPowerDamageScale) < 0.f ||
			value(AbilityData::FrostMaelstrom::Attribute::EnergyPowerRadiusScale) < 0.f ||
			value(CommonAttributeIds::Damage) < 0.f ||
			definition.levelProgression.size() != 14 ||
			definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Cryo)
		{
			if (failureReason)
			{
				*failureReason =
					"Frost Maelstrom contains invalid radius, movement, tick, Cryo, or progression values.";
			}
			return false;
		}

		if (!AbilityData::FindAbilityActorDefinition(
			AbilityData::FrostMaelstrom::Actor::Field::BasicDefinitionId
		))
		{
			if (failureReason)
			{
				*failureReason = "Frost Maelstrom requires its field actor definition.";
			}
			return false;
		}

		return true;
	}

	bool FrostMaelstromAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (!dynamic_cast<SpaceShip*>(&context.owner) ||
			!context.owner.GetWorld() || !mField.expired())
		{
			return false;
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const sf::Vector2f direction = context.owner.GetActorForwardDirection();
		const weak_ptr<AbilityWorldActor> spawned =
			AbilityActorSpawner::SpawnAtLocation(
				AbilityData::FrostMaelstrom::Actor::Field::BasicDefinitionId,
				executionContext,
				context.owner,
				context.owner.GetActorLocation(),
				direction
			);
		const shared_ptr<FrostMaelstromFieldActor> field =
			std::dynamic_pointer_cast<FrostMaelstromFieldActor>(spawned.lock());
		if (!field)
		{
			return false;
		}

		field->ConfigureFromAbilityValues(ResolveValues(context));
		mField = field;
		context.abilitySystem.AddOwnedTag(AbilityData::FrostMaelstrom::State::Active);
		EmitEvent(context, AbilityData::FrostMaelstrom::Event::Started);
		return true;
	}

	void FrostMaelstromAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		(void)context;
		(void)deltaTime;
	}

	void FrostMaelstromAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (const shared_ptr<FrostMaelstromFieldActor> field = mField.lock())
		{
			// Stop applying the field's vector force immediately. The movement
			// component retains each target's resulting velocity and slows it using
			// its normal damping rules.
			field->StopControl();
		}
		mField.reset();
		context.abilitySystem.RemoveOwnedTag(
			AbilityData::FrostMaelstrom::State::Active
		);
		EmitEvent(context, AbilityData::FrostMaelstrom::Event::Released);
		EmitEvent(context, AbilityData::FrostMaelstrom::Event::Ended);
	}

	void FrostMaelstromAbility::EmitEvent(
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
