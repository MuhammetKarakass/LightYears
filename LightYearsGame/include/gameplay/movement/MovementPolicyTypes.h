#pragma once

#include <optional>
#include <string>

namespace ly::movement
{
	// Policy sources are opaque to the movement layer. The caller may use an
	// ability ID, an effect ID, or any other stable gameplay-system identifier;
	// MovementComponent never interprets the value as an ability.
	using MovementPolicySourceId = std::string;

	enum class MovementPolicyReleaseMode
	{
		Immediate,
		Normalize
	};

	// A policy changes movement integration rules, not the ship's attributes.
	// Optional fields let independent systems override only the rule they own.
	struct MovementPolicyRequest
	{
		MovementPolicySourceId sourceId;
		std::optional<float> dampingRetentionOverride;
		// Flat cap contributions compose across sources. A movement ability that
		// grants +100 top speed must not replace another active +170 bonus.
		std::optional<float> speedCapFlatBonus;
		std::optional<float> speedCapMultiplierOverride;
		// true removes the speed limit entirely; false explicitly keeps a cap.
		// It shares the speed-cap policy domain with the multiplier override.
		std::optional<bool> speedCapDisabledOverride;
		int priority = 0;
	};

	struct MovementPolicyResolution
	{
		std::optional<float> dampingRetentionOverride;
		float speedCapFlatBonus = 0.f;
		std::optional<float> speedCapMultiplierOverride;
		std::optional<bool> speedCapDisabledOverride;
	};
}
