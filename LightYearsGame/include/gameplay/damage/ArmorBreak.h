#pragma once

namespace vega
{
	class Actor;

	namespace ArmorBreakIds
	{
		// Timed Armor multiplier on the target; separate from Kinetic Armor Rend so the two never replace each other.
		inline constexpr char EffectId[] = "Effect.Debuff.ArmorBreak";
		inline constexpr char FractionAttributeId[] = "Effect.Debuff.ArmorBreak.Fraction";
	}

	// Removes fraction (0..1) of the target's Armor for seconds as one multiplier on top of every other Armor modifier, so
	// the others are back unchanged when it ends. Boss and miniboss targets lose at most bossMaxFraction. Reapplying keeps
	// the stronger fraction and the longer remaining time. Returns the applied fraction (0 when nothing was applied).
	float ApplyArmorBreak(Actor& target, float fraction, float seconds, float bossMaxFraction, Actor* source);
}
