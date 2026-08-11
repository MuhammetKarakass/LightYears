#pragma once

namespace ly
{
	enum class AbilityBehaviorType
	{
		Configured,
		Dash,
		Shield,
		GravityAnomaly,
		Rocket,
		SunBeam,
		InfernoSpray,
		OverdriveCore,
		NullPulse,
		PhaseDrift
	};

	inline const char* ToString(AbilityBehaviorType type)
	{
		switch (type)
		{
		case AbilityBehaviorType::Configured: return "Configured";
		case AbilityBehaviorType::Dash: return "Dash";
		case AbilityBehaviorType::Shield: return "Shield";
		case AbilityBehaviorType::GravityAnomaly: return "GravityAnomaly";
		case AbilityBehaviorType::Rocket: return "Rocket";
		case AbilityBehaviorType::SunBeam: return "SunBeam";
		case AbilityBehaviorType::InfernoSpray: return "InfernoSpray";
		case AbilityBehaviorType::OverdriveCore: return "OverdriveCore";
		case AbilityBehaviorType::NullPulse: return "NullPulse";
		case AbilityBehaviorType::PhaseDrift: return "PhaseDrift";
		}
		return "Unknown";
	}
}
