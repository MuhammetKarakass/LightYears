#pragma once

#include "framework/Core.h"

namespace vega
{
	// Read-only view of which module systems a ship currently has enabled. The
	// ability system asks it instead of depending on the player ship class; a
	// component without a gate (non-player owners) treats every system as enabled.
	class ModuleSystemGate
	{
	public:
		virtual ~ModuleSystemGate() = default;
		virtual float GetAbilityMainEffectMultiplier(const List<GameplayTag>&) const { return 1.f; }
		virtual bool IsModuleSystemEnabled(const GameplayTag& systemTag) const = 0;
	};
}
