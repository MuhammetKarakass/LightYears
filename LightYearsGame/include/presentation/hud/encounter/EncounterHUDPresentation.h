#pragma once

#include "widget/UIViewModel.h"
#include <SFML/Graphics/Color.hpp>
#include <string>

namespace ly
{
	struct EncounterHUDPresentation
	{
		bool visible{ false };
		std::string title;
		std::string detail;
		sf::Color titleColor{ 190, 230, 255, 255 };
		UIRevision revision;
	};
}
