#pragma once

#include "abilities/AbilityPolicies.h"
#include "framework/Core.h"

#include <optional>
#include <string>
#include <string_view>

namespace ly
{
	enum class EnemyMovementMode
	{
		Approach,
		HoldRange,
		Strafe
	};

	enum class EnemySlotInputMode
	{
		Hold,
		Pulse
	};

	struct EnemySlotDecisionRule
	{
		sas::AbilitySlot slot = sas::AbilitySlot::None;
		EnemySlotInputMode inputMode = EnemySlotInputMode::Hold;
		bool requiresTarget = true;
		float minimumRange = 0.f;
		float maximumRange = 0.f;
		float aimConeThreshold = -1.f;
		float pulseRetryInterval = 0.25f;
		int priority = 0;
	};

	struct EnemyBehaviorProfile
	{
		std::string profileId;
		float targetSearchRange = 0.f;
		float targetRefreshInterval = 0.2f;
		float desiredDistance = 0.f;
		float minimumDistance = 0.f;
		float maximumDistance = 0.f;
		EnemyMovementMode movementMode = EnemyMovementMode::Approach;
		float strafeDirectionChangeInterval = 2.f;
		float aimTurnSpeed = 1.f;
		List<EnemySlotDecisionRule> slotRules;
	};

	inline std::optional<sas::AbilitySlot> ParseEnemySlot(std::string_view value)
	{
		if (value == "None") return sas::AbilitySlot::None;
		if (value == "PrimaryFire") return sas::AbilitySlot::PrimaryFire;
		if (value == "Ability1") return sas::AbilitySlot::Ability1;
		if (value == "Ability2") return sas::AbilitySlot::Ability2;
		if (value == "Ability3") return sas::AbilitySlot::Ability3;
		if (value == "Ability4") return sas::AbilitySlot::Ability4;
		return std::nullopt;
	}

	inline std::string_view EnemySlotToString(sas::AbilitySlot slot)
	{
		switch (slot)
		{
		case sas::AbilitySlot::None: return "None";
		case sas::AbilitySlot::PrimaryFire: return "PrimaryFire";
		case sas::AbilitySlot::Ability1: return "Ability1";
		case sas::AbilitySlot::Ability2: return "Ability2";
		case sas::AbilitySlot::Ability3: return "Ability3";
		case sas::AbilitySlot::Ability4: return "Ability4";
		}
		return "Unknown";
	}
}
