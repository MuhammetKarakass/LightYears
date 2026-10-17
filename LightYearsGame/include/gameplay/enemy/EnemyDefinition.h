#pragma once

#include <string>

namespace vega
{
	struct EnemyDefinition
	{
		std::string id;
		std::string shipId;
		std::string combatProfileId;
		std::string behaviorProfileId;
	};
}
