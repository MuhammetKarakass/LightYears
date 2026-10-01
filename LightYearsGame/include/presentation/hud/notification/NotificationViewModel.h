#pragma once

#include "widget/UIViewModel.h"
#include <SFML/Graphics/Color.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace ly
{
	struct NotificationRequest
	{
		std::string text;
		float fadeIn{ 0.f }, hold{ 0.f }, fadeOut{ 0.f };
		unsigned int size{ 50 };
		sf::Color color{ sf::Color::Red };
		std::uint32_t id{ 0 };
		bool operator==(const NotificationRequest& other) const
		{
			return text == other.text && fadeIn == other.fadeIn && hold == other.hold && fadeOut == other.fadeOut && size == other.size && color == other.color && id == other.id;
		}
	};

	struct NotificationViewModel
	{
		std::vector<NotificationRequest> pending;
		bool timerVisible{ false };
		int timerSeconds{ 0 };
		float timerFadeIn{ 0.f };
		std::uint32_t timerActivation{ 0 };
		UIRevision revision;
	};
}
