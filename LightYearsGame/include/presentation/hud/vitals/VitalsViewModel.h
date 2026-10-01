#pragma once

#include <SFML/Graphics/Color.hpp>
#include <widget/UIViewModel.h>

namespace ly
{
	struct VitalsViewModel
	{
		bool hasShip{ false };
		float health{ 0.f }, healthMax{ 1.f };
		float displayHealth{ 0.f }, displayHealthMax{ 1.f };
		float shield{ 0.f }, shieldMax{ 1.f };
		float energy{ 0.f }, energyMax{ 1.f };
		bool hasPlayer{ false };
		unsigned int life{ 0 }, score{ 0 };
		UIRevision revision;
	};

	sf::Color ComputeHealthBarColor(const VitalsViewModel& vm);
}
