#pragma once

#include "framework/Core.h"
#include "widget/UIViewModel.h"
#include "abilities/AbilityHandle.h"
#include "gameplay/ability/AbilityActivationPolicy.h"
#include <SFML/Graphics/Color.hpp>
#include <array>
#include <cstdint>
#include <string>

namespace vega
{
	enum class AbilitySlotState { Ready, Cooldown, Active };

	struct AbilitySlotViewData
	{
		bool visible{ false };
		std::string iconPath;
		std::string inputLabel;
		sf::Color accentColor{ sf::Color::White };
		AbilitySlotState state{ AbilitySlotState::Ready };
		int cooldownTenths{ 0 };
		std::string statsText;
		AbilityTimingView timing;

		bool operator==(const AbilitySlotViewData& other) const
		{
			return visible == other.visible && iconPath == other.iconPath && inputLabel == other.inputLabel &&
				accentColor == other.accentColor && state == other.state && cooldownTenths == other.cooldownTenths &&
				statsText == other.statsText && timing == other.timing;
		}
		bool operator!=(const AbilitySlotViewData& other) const { return !(*this == other); }
	};

	struct AbilityBarViewModel
	{
		std::string weaponReadout;
		std::array<AbilitySlotViewData, 8> slots{};
		std::uint32_t shipGeneration{ 0 };
		UIRevision revision;
	};
}
