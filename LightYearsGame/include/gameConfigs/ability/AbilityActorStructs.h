#pragma once

#include "framework/Core.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "content/ContentId.h"
#include <string>

namespace ly
{
	struct AbilityActorDefinition
	{
		sas::ContentId actorDefinitionId;
		AbilityActorType actorType = AbilityActorType::Generic;
		float lifeTime = 0.f;
		float spawnDistance = 0.f;
		sas::GameplayAttributeList attributes;
		sas::ContentId presentationProfileId;
	};
}

namespace AbilityData
{
	const ly::AbilityActorDefinition* FindAbilityActorDefinition(const std::string& actorDefinitionId);
}


