#pragma once
#include <string>

namespace vega
{
	namespace ModuleCardText
	{
		inline const std::string BodyFont = "fonts/NotoSans-Regular.ttf";
		inline const std::string TitleFont = "SpaceShooterRedux/Bonus/OrbitronBlack.ttf";
		inline constexpr float DetailWidth = 500.f;
		inline constexpr float InventoryTextWidth = 430.f;
		struct Layout { std::string text; unsigned int lineCount = 0; bool truncated = false; };
		Layout Wrap(const std::string& text, const std::string& fontPath, unsigned int characterSize, float width, unsigned int maxLines);
	}
}
