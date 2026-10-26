#pragma once

#include "gameplay/damage/StatusStacks.h"

namespace sas
{
	class AbilitySystemComponent;
}

namespace vega
{
	// Elemental reactions between two different stacking damage statuses. Which reactions exist is fixed here; what each
	// one does and how strong it is belongs to whoever triggers it.
	enum class StatusReactionKind
	{
		None,
		Steam,        // Thermal + Cryo
		Magnet,       // Electric + Kinetic
		Plasma,       // Thermal + Electric
		Fracture,     // Cryo + Kinetic
		Superconduct, // Cryo + Electric
		Shrapnel      // Thermal + Kinetic
	};

	// Reaction of an unordered pair of stacking damage statuses (None for an identical or non-stacking status).
	StatusReactionKind ResolveStatusReaction(const GameplayTag& first, const GameplayTag& second);

	struct StatusReactionPair
	{
		GameplayTag added;
		GameplayTag partner;
		StatusReactionKind kind = StatusReactionKind::None;
	};

	// Picks the pair for a stack event: the first added status (fixed order Thermal/Cryo/Electric/Kinetic) that has a partner
	// among the statuses the target carried before the hit and still carries now. The partner is the one with the longest
	// remaining time; ties keep the fixed order. Returns kind None when no pair exists.
	StatusReactionPair SelectStatusReactionPair(const List<GameplayTag>& added, const List<GameplayTag>& carriedBefore, const List<DamageStatusStackState>& current);
	// Consumes both statuses of the pair completely; other statuses stay.
	void ConsumeStatusReactionPair(sas::AbilitySystemComponent& target, const StatusReactionPair& pair);
}
