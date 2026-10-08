#pragma once
#include <cstdint>
#include <set>

namespace ly
{
	// Shared by all hits from one ability use or one Primary shot. It owns no actor.
	struct DamageHitGroup
	{
		enum class Kind { AbilityUse, PrimaryShot };
		explicit DamageHitGroup(Kind value = Kind::AbilityUse) : kind{ value } {}
		Kind kind;
		std::set<std::pair<std::uint64_t, std::uint64_t>> consumedModuleRules;
	};
}
