#pragma once

#include <SFML/Graphics/Color.hpp>
#include <widget/UIViewModel.h>

namespace vega
{
	struct VitalsViewModel
	{
		bool hasShip{ false };
		// HealthReadout module state: gates the HP/armor gauge (displayHealth includes barrier).
		bool healthReadoutVisible{ true };
		float health{ 0.f }, healthMax{ 1.f };
		float displayHealth{ 0.f }, displayHealthMax{ 1.f };
		float shield{ 0.f }, shieldMax{ 1.f };
		float shieldReserve{ 0.f }, shieldReserveCap{ 0.f };
		float energy{ 0.f }, energyMax{ 1.f };
		bool blinkDrive{ false };
		int blinkCharges{ 0 }, blinkMaximumCharges{ 0 };
		float blinkRechargeRemaining{ 0.f };
		bool hasPlayer{ false };
		unsigned int life{ 0 }, score{ 0 };
		UIRevision revision;
	};

	sf::Color ComputeHealthBarColor(const VitalsViewModel& vm);
}
