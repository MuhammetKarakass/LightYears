#include "gameplay/ability/combatSentry/CombatSentryAbility.h"

#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/combatSentry/CombatSentryContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "framework/Actor.h"
#include "framework/World.h"

#include <cmath>

namespace ly
{
	bool CombatSentryAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const bool validLifecycle =
			definition.abilityId == AbilityData::CombatSentry::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::CombatSentry &&
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 && definition.duration == 0.f &&
			definition.cooldown > 0.f && definition.levelProgression.size() == 14;
		if (!validLifecycle)
		{
			if (failureReason)
			{
				*failureReason =
					"Combat Sentry requires an instant pressed ability with one charge and fourteen progression steps.";
			}
			return false;
		}

		for (const char* actorId : {
			AbilityData::CombatSentry::Actor::Turret::BasicDefinitionId,
			AbilityData::CombatSentry::Actor::Projectile::BasicDefinitionId
		})
		{
			const AbilityActorDefinition* actorDefinition =
				AbilityData::FindAbilityActorDefinition(actorId);
			if (!actorDefinition)
			{
				if (failureReason)
				{
					*failureReason = "Combat Sentry requires both turret and projectile actor definitions.";
				}
				return false;
			}
			const AbilityActorValidationResult validation =
				AbilityActorRegistry::ValidateDefinition(*actorDefinition);
			if (!validation.isValid)
			{
				if (failureReason)
				{
					*failureReason = validation.reason;
				}
				return false;
			}
		}
		return true;
	}

	bool CombatSentryAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world)
		{
			return false;
		}

		const sf::Vector2f location = world->GetMouseWorldPosition();
		sf::Vector2f direction = location - context.owner.GetActorLocation();
		if (GetVectorLength(direction) <= 0.001f)
		{
			direction = context.owner.GetActorForwardDirection();
		}
		else
		{
			NormalizeVector(direction);
		}

		AbilityExecutionContext executionContext{
			&context.abilitySystem,
			&context.definition,
			nullptr,
			&context.instance
		};
		const weak_ptr<AbilityWorldActor> turret = AbilityActorSpawner::SpawnAtLocation(
			AbilityData::CombatSentry::Actor::Turret::BasicDefinitionId,
			executionContext,
			context.owner,
			location,
			direction
		);
		if (turret.expired())
		{
			return false;
		}

		sas::AbilityEvent event;
		event.eventTag = AbilityData::CombatSentry::Event::Spawned;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(turret.lock().get());
		context.abilitySystem.HandleGameplayEvent(event);
		return true;
	}
}
