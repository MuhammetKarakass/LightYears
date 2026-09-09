#pragma once

#include "gameplay/movement/MovementPolicyTypes.h"

namespace ly
{
	class Actor;
}

namespace ly::movement
{
	// Gameplay-facing adapter for movement policies. The adapter depends on the
	// ship implementation, while MovementPolicyController itself remains a
	// standalone, ability-agnostic value resolver.
	class MovementPolicyService final
	{
	public:
		static bool SetPolicy(Actor& target, const MovementPolicyRequest& request);
		static bool ReleasePolicy(
			Actor& target,
			const MovementPolicySourceId& sourceId,
			MovementPolicyReleaseMode releaseMode,
			float normalizationDuration
		);
		static void ClearPolicies(Actor& target);
		static bool SupportsPolicies(const Actor& target);
	};
}
