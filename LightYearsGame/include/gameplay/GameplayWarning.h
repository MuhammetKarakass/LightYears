#pragma once

#include <string>

namespace ly
{
	enum class GameplayWarningType
	{
		ArenaBoundary
	};

	struct GameplayWarning
	{
		GameplayWarningType type = GameplayWarningType::ArenaBoundary;
		std::string title;
		std::string message;
		float remainingTime = 0.f;
		float totalTime = 0.f;
		bool hasCountdown = false;
	};
}
