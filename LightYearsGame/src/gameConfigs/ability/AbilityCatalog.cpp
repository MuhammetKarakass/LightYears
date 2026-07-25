#include "gameConfigs/ability/RocketConfig.h"
#include "gameConfigs/ability/SunBeamConfig.h"

namespace AbilityData
{
	const ly::AbilityActorDefinition* FindAbilityActorDefinition(
		const std::string& actorDefinitionId)
	{
		if (const ly::AbilityActorDefinition* rocketDefinition = Rocket::FindActorDefinition(actorDefinitionId))
		{
			return rocketDefinition;
		}
		return SunBeam::FindActorDefinition(actorDefinitionId);
	}
}
