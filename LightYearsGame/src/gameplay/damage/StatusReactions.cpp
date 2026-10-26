#include "gameplay/damage/StatusReactions.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

#include <algorithm>

namespace vega
{
	StatusReactionKind ResolveStatusReaction(const GameplayTag& first, const GameplayTag& second)
	{
		const auto is = [&first, &second](const GameplayTag& a, const GameplayTag& b) { return (first == a && second == b) || (first == b && second == a); };
		const GameplayTag& thermal = DamageStatusSchema::Ignite;
		const GameplayTag& cryo = DamageStatusSchema::CryoSlowed;
		const GameplayTag& electric = DamageStatusSchema::Electric;
		const GameplayTag& kinetic = DamageStatusSchema::Kinetic;
		if (is(thermal, cryo)) return StatusReactionKind::Steam;
		if (is(electric, kinetic)) return StatusReactionKind::Magnet;
		if (is(thermal, electric)) return StatusReactionKind::Plasma;
		if (is(cryo, kinetic)) return StatusReactionKind::Fracture;
		if (is(cryo, electric)) return StatusReactionKind::Superconduct;
		if (is(thermal, kinetic)) return StatusReactionKind::Shrapnel;
		return StatusReactionKind::None;
	}

	StatusReactionPair SelectStatusReactionPair(const List<GameplayTag>& added, const List<GameplayTag>& carriedBefore, const List<DamageStatusStackState>& current)
	{
		for (const GameplayTag& status : GetStackingDamageStatuses())
		{
			if (std::find(added.begin(), added.end(), status) == added.end()) continue;
			const DamageStatusStackState* partner = nullptr;
			// current keeps the fixed order, so a strict comparison keeps the earlier status on a tie.
			for (const DamageStatusStackState& state : current)
			{
				if (state.status == status || std::find(carriedBefore.begin(), carriedBefore.end(), state.status) == carriedBefore.end()) continue;
				if (!partner || state.remainingSeconds > partner->remainingSeconds) partner = &state;
			}
			if (partner) return { status, partner->status, ResolveStatusReaction(status, partner->status) };
		}
		return {};
	}

	void ConsumeStatusReactionPair(sas::AbilitySystemComponent& target, const StatusReactionPair& pair)
	{
		ConsumeDamageStatus(target, pair.added);
		ConsumeDamageStatus(target, pair.partner);
	}
}
