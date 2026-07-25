#pragma once

#include "framework/Core.h"
#include "gameConfigs/ability/AbilityActorStructs.h"

#include <SFML/System/Vector2.hpp>
#include <optional>

namespace ly
{
	class Actor;
	class AbilityWorldActor;

	struct AbilityActorValidationResult
	{
		bool isValid = false;
		std::string reason;
	};

	struct AbilityActorSpawnContext
	{
		Actor& owner;
		const AbilityActorDefinition& definition;
		const GameplayAttributeList& attributes;
		std::optional<sf::Vector2f> targetLocation;
	};

	class AbilityActorTypeHandler
	{
	public:
		virtual ~AbilityActorTypeHandler() = default;
		virtual const GameplayTag& GetActorTypeTag() const = 0;
		virtual const List<GameplayTag>& GetOwnedAttributeRoots() const;
		virtual const List<GameplayTag>& GetAllowedCommonAttributeIds() const;
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
