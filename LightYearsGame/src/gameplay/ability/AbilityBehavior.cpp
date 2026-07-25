#include "gameplay/ability/AbilityBehavior.h"

namespace ly
{
	bool AbilityBehavior::Validate(const AbilityDefinition&, std::string*) const
	{
		return true;
	}

	bool AbilityBehavior::Activate(AbilityBehaviorContext&)
	{
		return true;
	}

	void AbilityBehavior::Tick(AbilityBehaviorContext&, float)
	{
	}

	void AbilityBehavior::End(AbilityBehaviorContext&, AbilityEndReason)
	{
	}
}
