#pragma once

#include "framework/Core.h"
#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "content/ContentId.h"
#include <string>

namespace vega
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
	const vega::AbilityActorDefinition* FindAbilityActorDefinition(const std::string& actorDefinitionId);
}


