#pragma once

#include "gameplay/enemy/EnemyBehaviorProfile.h"

#include <string>

namespace vega
{
	class EnemyBehaviorProfileValidator
	{
	public:
		static bool Validate(const EnemyBehaviorProfile& profile, std::string* failureReason = nullptr);
	};
}
