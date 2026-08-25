#include "gameplay/ability/crystalBarricade/CrystalBarricadeAbility.h"

#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/crystalBarricade/CrystalBarricadeContracts.h"
#include "framework/MathUtility.h"
#include "framework/Actor.h"
#include "framework/World.h"

namespace ly
{
	bool CrystalBarricadeAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		const AbilityActorDefinition* wallDefinition = AbilityData::FindAbilityActorDefinition(
			AbilityData::CrystalBarricade::Actor::Wall::BasicDefinitionId
		);
		const bool valid =
			definition.abilityId == AbilityData::CrystalBarricade::AbilityId::Basic &&
			definition.behaviorType == AbilityBehaviorType::CrystalBarricade &&
			sas::IsLoadoutAbilitySlot(definition.slot) &&
			definition.activationPolicy == sas::AbilityActivationPolicy::OnPressed &&
			definition.lifetimePolicy == sas::AbilityLifetimePolicy::Instant &&
			definition.maxCharges == 1 && definition.duration == 0.f &&
			definition.cooldown > 0.f && definition.levelProgression.size() == 14 &&
			wallDefinition && AbilityActorRegistry::ValidateDefinition(*wallDefinition).isValid;
		if (!valid && failureReason)
		{
			*failureReason = "Crystal Barricade requires a valid instant defensive wall actor and fourteen progression steps.";
		}
		return valid;
	}

	bool CrystalBarricadeAbility::Activate(GameAbilityBehaviorContext& context)
	{
		World* world = context.owner.GetWorld();
		if (!world)
		{
			return false;
		}

		const sf::Vector2f location = world->GetMouseWorldPosition();
		sf::Vector2f ownerToCursor = location - context.owner.GetActorLocation();
		if (GetVectorLength(ownerToCursor) <= 0.001f)
		{
			ownerToCursor = context.owner.GetActorForwardDirection();
		}
		else
		{
			NormalizeVector(ownerToCursor);
		}

		// The actor's local X axis is the physical wall length. The spawner maps
		// its direction to actor forward, which leaves local X perpendicular to
		// owner-to-cursor as the design requires.
		AbilityExecutionContext executionContext{
			&context.abilitySystem, &context.definition, nullptr, &context.instance
		};
		const weak_ptr<AbilityWorldActor> wall = AbilityActorSpawner::SpawnAtLocation(
			AbilityData::CrystalBarricade::Actor::Wall::BasicDefinitionId,
			executionContext,
			context.owner,
			location,
			ownerToCursor
		);
		if (wall.expired())
		{
			return false;
		}

		sas::AbilityEvent event;
		event.eventTag = AbilityData::CrystalBarricade::Event::Placed;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(wall.lock().get());
		context.abilitySystem.HandleGameplayEvent(event);
		return true;
	}
}
