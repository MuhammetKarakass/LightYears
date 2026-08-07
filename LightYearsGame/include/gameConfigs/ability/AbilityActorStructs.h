#pragma once

#include "framework/Core.h"
#include "attributes/AttributeSystem.h"
#include <string>

namespace ly
{
	struct AbilityActorSchema
	{
		struct Generic
		{
			inline static const GameplayTag TypeTag{ "AbilityActor.Generic.Generic" };
		};
	};


	struct AbilityActorDefinition
	{
		std::string actorDefinitionId;
		GameplayTag actorTypeTag = AbilityActorSchema::Generic::TypeTag;
		float lifeTime = 0.f;
		float spawnDistance = 0.f;
		sas::GameplayAttributeList attributes;
		std::string presentationProfileId;
	};
}

namespace AbilityData
{
	const ly::AbilityActorDefinition* FindAbilityActorDefinition(const std::string& actorDefinitionId);
}


