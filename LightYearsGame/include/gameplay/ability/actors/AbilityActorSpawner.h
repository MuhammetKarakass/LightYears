#pragma once

#include "gameplay/ability/content/GameAbilityActions.h"

#include <SFML/System/Vector2.hpp>
#include <optional>

namespace ly
{
	class Actor;
	class AbilityWorldActor;
	class GameAbility;
	class LightYearsAbilitySystemComponent;

	struct AbilityExecutionContext;

	// The executor coordinates an ability lifecycle; this service owns the
	// generic SpawnActorAction pipeline so actor creation does not become part
	// of the executor's growing list of unrelated action implementations.
	class AbilityActorSpawner final
	{
	public:
		// Direction policies are shared by projectile spawning and impulse
		// actions, so both actions resolve movement/cursor/forward consistently.
		static sf::Vector2f ResolveDirection(
			const Actor& owner,
			sas::AbilityDirectionPolicy directionPolicy
		);

		// Resolves the actor definition, runtime attributes, spawn transform,
		// collision data, and source-ability metadata for one generic action.
		static weak_ptr<AbilityWorldActor> Spawn(
			const SpawnActorAction& actionData,
			AbilityExecutionContext& context,
			Actor& owner
		);

		// Feature behaviors can reuse the same actor-definition, attribute,
		// collision, damage, and presentation pipeline while supplying a target
		// chosen by their own targeting policy. This keeps target selection out of
		// the generic spawner and avoids duplicating Rocket actor setup in
		// OverdriveCore.
		static weak_ptr<AbilityWorldActor> SpawnToTarget(
			const sas::ContentId& actorDefinitionId,
			AbilityExecutionContext& context,
			Actor& owner,
			const sf::Vector2f& direction,
			const std::optional<sf::Vector2f>& targetLocation,
			float damageMultiplier = 1.f,
			Actor* targetActor = nullptr
		);
	};
}
