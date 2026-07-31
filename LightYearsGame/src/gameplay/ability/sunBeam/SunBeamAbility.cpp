#include "gameplay/ability/sunBeam/SunBeamAbility.h"

#include <variant>

namespace ly
{
	bool SunBeamAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason) const
	{
		for (const AbilityActionSpec& action : definition.actions)
		{
			if (action.phase == sas::AbilityActionPhase::OnActivate &&
				std::holds_alternative<SpawnActorAction>(action.action))
			{
				return true;
			}
		}

		if (failureReason)
		{
			*failureReason = "Sun Beam abilities require an OnActivate actor spawn action.";
		}
		return false;
	}
}
