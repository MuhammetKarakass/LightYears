#include "presentation/hud/warning/GameplayWarningViewModel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ly
{
	std::string FormatGameplayWarningText(const GameplayWarning& warning)
	{
		char warningText[128];
		if (warning.hasCountdown)
			std::snprintf(warningText, sizeof(warningText), "%s\n%s %.2f", warning.title.c_str(), warning.message.c_str(), warning.remainingTime);
		else
			std::snprintf(warningText, sizeof(warningText), "%s\n%s", warning.title.c_str(), warning.message.c_str());
		return warningText;
	}

	GameplayWarningPulse ComputeGameplayWarningPulse(float animTime)
	{
		const float pulse = (std::sin(animTime * 9.5f) + 1.f) * 0.5f;
		const float flicker = (std::sin(animTime * 37.f) + 1.f) * 0.5f;
		const float threat = std::max(pulse, flicker * 0.65f);
		const std::uint8_t greenBlue = static_cast<std::uint8_t>(35.f + threat * 45.f);
		const std::uint8_t alpha = static_cast<std::uint8_t>(205.f + threat * 50.f);
		return { sf::Color{ 255, greenBlue, greenBlue, alpha },
			{ std::sin(animTime * 51.f) * 1.8f * threat, std::sin(animTime * 29.f) * 1.2f * threat } };
	}
}
