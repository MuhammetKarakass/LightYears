#pragma once

#include "gameplay/GameplayWarning.h"
#include "widget/UIViewModel.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <string>

namespace ly
{
	struct GameplayWarningViewModel
	{
		bool visible{ false };
		GameplayWarningType type{ GameplayWarningType::ArenaBoundary };
		std::string text;
		std::uint32_t activation{ 0 };
		UIRevision revision;
	};

	std::string FormatGameplayWarningText(const GameplayWarning& warning);

	struct GameplayWarningPulse
	{
		sf::Color color;
		sf::Vector2f shake;
	};

	GameplayWarningPulse ComputeGameplayWarningPulse(float animTime);
}
