#pragma once

#include <string>
#include <optional>

namespace vega
{
	struct EnemyDefinition
	{
		std::string id;
		std::string shipId;
		std::string combatProfileId;
		std::string behaviorProfileId;
		std::optional<std::string> prefabPath;
	};
}
