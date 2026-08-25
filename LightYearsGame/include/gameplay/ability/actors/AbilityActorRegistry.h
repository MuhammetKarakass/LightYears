#pragma once

#include "attributes/AttributeSystem.h"

#include "framework/Core.h"
#include "gameConfigs/ability/AbilityActorStructs.h"

#include <SFML/System/Vector2.hpp>
#include <optional>

namespace ly
{
	class Actor;
	class AbilityWorldActor;
	class GameAbility;
	class LightYearsAbilitySystemComponent;
	struct GameAbilityDefinition;

	struct AbilityActorValidationResult
	{
		bool isValid = false;
		std::string reason;
	};

	struct AbilityActorSpawnContext
	{
		Actor& owner;
		const AbilityActorDefinition& definition;
		const sas::GameplayAttributeList& attributes;
		std::optional<sf::Vector2f> targetLocation;
		// Optional live target for projectiles that must follow moving actors.
		weak_ptr<Actor> targetActor;
		// The concrete projectile may need to send a result back to the exact
		// ability instance that spawned it (for example, Crescent Reaver reduces
		// its own cooldown after a successful bounce). This is optional so generic
		// actors remain independent from ability behavior classes.
		GameAbility* sourceAbilityInstance = nullptr;
		// A persistent summon can spawn child actors after its source ability has
		// completed. Keep the immutable execution metadata with that summon instead
		// of making the summon re-discover a transient ability instance later.
		LightYearsAbilitySystemComponent* abilitySystem = nullptr;
		const GameAbilityDefinition* abilityDefinition = nullptr;
	};

	class AbilityActorTypeHandler
	{
	public:
		virtual ~AbilityActorTypeHandler() = default;
		virtual AbilityActorType GetActorType() const = 0;
		virtual const List<sas::AttributeId>& GetOwnedAttributeRoots() const;
		virtual const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const;
		virtual AbilityActorValidationResult ValidateDefinition(const AbilityActorDefinition& definition) const;
		virtual weak_ptr<AbilityWorldActor> Spawn(const AbilityActorSpawnContext& context) const = 0;
	};

	class AbilityActorRegistry
	{
	public:
		static bool RegisterHandler(unique_ptr<AbilityActorTypeHandler> handler);
		static AbilityActorValidationResult ValidateDefinition(const AbilityActorDefinition& definition);
		static weak_ptr<AbilityWorldActor> Spawn(const AbilityActorSpawnContext& context);
	};
}
