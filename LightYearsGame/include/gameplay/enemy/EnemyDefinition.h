#pragma once

#include <string>

namespace ly
{
	struct EnemyDefinition
	{
		std::string id;
		std::string shipId;
		std::string combatProfileId;
		std::string behaviorProfileId;
	};
}
