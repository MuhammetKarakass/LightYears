#include "gameplay/ability/frozenThrong/FrozenThrongAbility.h"

#include "gameplay/ability/frozenThrong/FrozenThrongContracts.h"
#include "gameplay/ability/frozenThrong/FrozenThrongHuskActor.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
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
			return attribute && std::isfinite(attribute->baseValue);
		}
	}

	bool FrozenThrongAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::FrozenThrong::AbilityId::Basic ||
			definition.behaviorType != AbilityBehaviorType::FrozenThrong ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || definition.duration <= 0.f ||
			definition.cooldown <= 0.f || definition.levelProgression.size() != 14 ||
			definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Cryo)
		{
			if (failureReason)
			{
				*failureReason =
					"Frozen Throng requires a duration Cryo ability with one charge and fourteen progression steps.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::FrozenThrong::Attribute::HuskDamage,
			AbilityData::FrozenThrong::Attribute::LuckToHuskScale,
			AbilityData::FrozenThrong::Attribute::HuskDelay,
			AbilityData::FrozenThrong::Attribute::TargetSearchRadius,
			AbilityData::FrozenThrong::Attribute::DensityRadius,
			AbilityData::FrozenThrong::Attribute::ExplosionRadius
		})
		{
			if (!HasFiniteAttribute(definition, required))
			{
				if (failureReason)
				{
					*failureReason = "Frozen Throng must declare all runtime attributes.";
				}
				return false;
			}
		}
		return true;
	}

	bool FrozenThrongAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mActive)
		{
			return false;
		}
		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::FrozenThrong::State::Active);
		EmitEvent(context, AbilityData::FrozenThrong::Event::Started);
		return true;
	}

	void FrozenThrongAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		(void)context;
		(void)deltaTime;
	}

	void FrozenThrongAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		(void)reason;
		if (!mActive)
		{
			return;
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::FrozenThrong::State::Active);
		mActive = false;
		EmitEvent(context, AbilityData::FrozenThrong::Event::Ended);
	}

	void FrozenThrongAbility::OnGameplayEvent(
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
			!damageContext->targetWasCryoAffected ||
			!damageContext->targetWasEnemyCombatant)
		{
			return;
		}

		if (!std::isfinite(damageContext->targetLocationAtResolution.x) ||
			!std::isfinite(damageContext->targetLocationAtResolution.y))
		{
			return;
		}
		SpawnHusksForKill(
			context,
			{
				damageContext->targetLocationAtResolution.x,
				damageContext->targetLocationAtResolution.y
			}
		);
	}

	sas::GameplayAttributeList FrozenThrongAbility::ResolveValues(
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

	void FrozenThrongAbility::SpawnHusksForKill(
		GameAbilityBehaviorContext& context,
		const sf::Vector2f& origin
	)
	{
		const sas::GameplayAttributeList values = ResolveValues(context);
		const float luckScale = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::FrozenThrong::Attribute::LuckToHuskScale,
			AbilityData::FrozenThrong::DefaultLuckToHuskScale
		));
		const float luckRating = std::max(0.f, context.abilitySystem.GetAttributes()
			.GetCurrentValue(OwnerAttributeIds::Luck));
		const float bonusValue = luckRating * luckScale;
		const int guaranteedBonus = static_cast<int>(std::floor(bonusValue));
		const float fractionalBonus = bonusValue - static_cast<float>(guaranteedBonus);
		const int huskCount = 1 + std::max(
			0,
			guaranteedBonus + (RandRange(0.f, 1.f) < fractionalBonus ? 1 : 0)
		);

		// Every husk is created as an independent delayed actor. The delay keeps
		// chain reactions off the current damage call stack, so a dense kill can
		// produce any number of husks without recursive same-frame damage.
		for (int index = 0; index < huskCount; ++index)
		{
			SpawnHusk(context, origin, values);
		}
	}

	void FrozenThrongAbility::SpawnHusk(
		GameAbilityBehaviorContext& context,
		const sf::Vector2f& origin,
		const sas::GameplayAttributeList& values
	)
	{
		World* world = context.owner.GetWorld();
		if (!world)
		{
			return;
		}

		const float searchRadius = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::FrozenThrong::Attribute::TargetSearchRadius,
			AbilityData::FrozenThrong::DefaultTargetSearchRadius
		));
		const float densityRadius = std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::FrozenThrong::Attribute::DensityRadius,
			AbilityData::FrozenThrong::DefaultDensityRadius
		));
		const shared_ptr<Actor> target = targeting::FindDensestOpposingCombatant(
			*world,
			context.owner,
			origin,
			searchRadius,
			densityRadius
		);
		const std::optional<sf::Vector2f> targetLocation = target
			? std::optional<sf::Vector2f>{ target->GetActorLocation() }
			: std::nullopt;
		sf::Vector2f direction = context.owner.GetActorForwardDirection();
		if (targetLocation)
		{
			const sf::Vector2f offset = *targetLocation - origin;
			if (GetVectorLength(offset) > 0.001f)
			{
				direction = offset / GetVectorLength(offset);
			}
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const weak_ptr<AbilityWorldActor> spawned =
			AbilityActorSpawner::SpawnAtLocation(
				AbilityData::FrozenThrong::Actor::Husk::BasicDefinitionId,
				executionContext,
				context.owner,
				origin,
				direction,
				1.f,
				targetLocation,
				target.get()
			);
		const shared_ptr<FrozenThrongHuskActor> husk =
			std::dynamic_pointer_cast<FrozenThrongHuskActor>(spawned.lock());
		if (!husk)
		{
			return;
		}

		husk->SetLaunchDelay(std::max(0.f, sas::FindAttributeValue(
			values,
			AbilityData::FrozenThrong::Attribute::HuskDelay,
			AbilityData::FrozenThrong::DefaultHuskDelay
		)));
		EmitEvent(context, AbilityData::FrozenThrong::Event::HuskSpawned);
	}

	void FrozenThrongAbility::EmitEvent(
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
