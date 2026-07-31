#include "gameplay/ability/shield/ShieldAbility.h"

#include <variant>

namespace ly
{
	bool ShieldAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason) const
	{
		for (const AbilityActionSpec& action : definition.actions)
		{
			if (action.phase == sas::AbilityActionPhase::OnActivate &&
				std::holds_alternative<ApplyEffectAction>(action.action))
			{
				return true;
			}
		}

		if (failureReason)
		{
			*failureReason = "Shield abilities require an OnActivate effect action.";
		}
		return false;
	}
}
