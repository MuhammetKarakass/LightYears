#include "gameplay/enemy/EnemyBehaviorProfileValidator.h"

#include "gameplay/content/ContentIdSchema.h"

#include <cmath>
#include <set>

namespace ly
{
	namespace
	{
		bool Fail(std::string* failureReason, const std::string& message)
		{
			if (failureReason) *failureReason = message;
			return false;
		}

		bool IsFiniteNonNegative(float value)
		{
			return std::isfinite(value) && value >= 0.f;
		}
	}

	bool EnemyBehaviorProfileValidator::Validate(const EnemyBehaviorProfile& profile, std::string* failureReason)
	{
		std::string idFailure;
		if (!content::ContentIdSchema::ValidateEnemyBehaviorProfileId(profile.profileId, &idFailure))
		{
			return Fail(failureReason, "Invalid enemy behavior profile ID '" + profile.profileId + "': " + idFailure);
		}
		if (!std::isfinite(profile.targetSearchRange) || profile.targetSearchRange <= 0.f ||
			!std::isfinite(profile.targetRefreshInterval) || profile.targetRefreshInterval <= 0.f ||
			!IsFiniteNonNegative(profile.desiredDistance) || !IsFiniteNonNegative(profile.minimumDistance) ||
			!IsFiniteNonNegative(profile.maximumDistance) || !IsFiniteNonNegative(profile.strafeDirectionChangeInterval) ||
			!IsFiniteNonNegative(profile.aimTurnSpeed))
		{
			return Fail(failureReason, "Invalid numeric values in enemy behavior profile '" + profile.profileId + "'.");
		}
		std::set<sas::AbilitySlot> usedRuleSlots;
		for (const EnemySlotDecisionRule& rule : profile.slotRules)
		{
			if (rule.slot == sas::AbilitySlot::None || !usedRuleSlots.insert(rule.slot).second)
			{
				return Fail(failureReason, "Enemy behavior profile '" + profile.profileId + "' has an invalid or duplicate slot rule.");
			}
			if (rule.inputMode != EnemySlotInputMode::Hold && rule.inputMode != EnemySlotInputMode::Pulse)
			{
				return Fail(failureReason, "Enemy behavior profile '" + profile.profileId + "' has an unknown slot input mode.");
			}
			if (!IsFiniteNonNegative(rule.minimumRange) || !IsFiniteNonNegative(rule.maximumRange) ||
				!std::isfinite(rule.aimConeThreshold) || rule.aimConeThreshold < -1.f || rule.aimConeThreshold > 1.f ||
				!std::isfinite(rule.pulseRetryInterval) || rule.pulseRetryInterval <= 0.f ||
				(rule.inputMode == EnemySlotInputMode::Hold && rule.pulseRetryInterval != 0.25f))
			{
				return Fail(failureReason, "Enemy behavior profile '" + profile.profileId + "' has invalid slot rule values.");
			}
			if (rule.minimumRange > rule.maximumRange || rule.maximumRange > profile.targetSearchRange)
			{
				return Fail(failureReason, "Enemy behavior profile '" + profile.profileId + "' has an out-of-range slot rule.");
			}
			if (!rule.requiresTarget &&
				(rule.minimumRange != 0.f || rule.maximumRange != 0.f || rule.aimConeThreshold != -1.f))
			{
				return Fail(failureReason, "Targetless slot rules must use zero ranges and the default cone threshold.");
			}
		}

		switch (profile.movementMode)
		{
		case EnemyMovementMode::Approach:
			if (profile.maximumDistance != 0.f || profile.minimumDistance > profile.desiredDistance)
			{
				return Fail(failureReason, "Approach behavior profile '" + profile.profileId + "' has incompatible distance bounds.");
			}
			return true;
		case EnemyMovementMode::HoldRange:
			if (profile.minimumDistance <= 0.f || profile.maximumDistance <= profile.minimumDistance || profile.desiredDistance != 0.f)
			{
				return Fail(failureReason, "HoldRange behavior profile '" + profile.profileId + "' requires positive ordered distance bounds and desiredDistance of zero.");
			}
			return true;
		case EnemyMovementMode::Strafe:
			if (profile.minimumDistance <= 0.f || profile.maximumDistance <= profile.minimumDistance ||
				profile.strafeDirectionChangeInterval <= 0.f || profile.desiredDistance != 0.f)
			{
				return Fail(failureReason, "Strafe behavior profile '" + profile.profileId + "' requires ordered distance bounds, a positive direction interval, and desiredDistance of zero.");
			}
			return true;
		default:
			return Fail(failureReason, "Enemy behavior profile '" + profile.profileId + "' has an unknown movement mode.");
		}
	}
}
