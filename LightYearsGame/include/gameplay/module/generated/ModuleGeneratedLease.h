#pragma once
#include "framework/Core.h"
#include <cstdint>

namespace ly
{
	class Actor;
	// Runtime-owned generation; actors only retain a weak lease. Removal cancels
	// existing work immediately and a newer registration gets a different key.
	struct ModuleGeneratedLease
	{
		std::uint64_t key = 0;
		bool active = true;
		List<weak_ptr<Actor>> actors;
	};
}
