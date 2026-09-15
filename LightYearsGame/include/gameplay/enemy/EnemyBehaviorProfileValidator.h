#pragma once

#include "gameplay/enemy/EnemyBehaviorProfile.h"

#include <string>

namespace ly
{
	class EnemyBehaviorProfileValidator
	{
	public:
		static bool Validate(const EnemyBehaviorProfile& profile, std::string* failureReason = nullptr);
	};
}
