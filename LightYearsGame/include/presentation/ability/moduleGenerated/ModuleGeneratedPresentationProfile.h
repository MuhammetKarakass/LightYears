#pragma once
#include "content/ContentId.h"
#include <SFML/Graphics/Color.hpp>

namespace ly
{
	namespace ModuleGeneratedPresentationIds
	{
		inline const sas::ContentId AnnularBurst{ "Presentation.Ability.ModuleGenerated.AnnularBurst.Basic" };
		inline const sas::ContentId SpeedLance{ "Presentation.Ability.ModuleGenerated.SpeedLance.Basic" };
		inline const sas::ContentId Guided{ "Presentation.Ability.ModuleGenerated.Guided.Basic" };
		inline const sas::ContentId Mine{ "Presentation.Ability.ModuleGenerated.Mine.Basic" };
		inline const sas::ContentId Explosion{ "Presentation.Ability.ModuleGenerated.Explosion.Basic" };
		inline const sas::ContentId ThermalExplosion{ "Presentation.Ability.ModuleGenerated.Explosion.Thermal" };
	}
	struct ModuleGuidedPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color body{ 120, 210, 255, 240 };
		float radius = 3.f;
	};
	struct ModuleAnnularBurstPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color innerRing{ 120, 210, 255, 120 };
		sf::Color outerRing{ 120, 210, 255, 180 };
		float thickness = 2.f;
		float duration = .18f;
	};
	struct ModuleSpeedLancePresentationProfile
	{
		sas::ContentId profileId;
		sf::Color body{ 125, 220, 255, 240 };
		float length = 36.f;
		float width = 4.f;
		float collisionRadius = 3.f;
	};
	struct ModuleMinePresentationProfile
	{
		sas::ContentId profileId;
		sf::Color body{ 100, 235, 200, 220 };
		sf::Color fuseRing{ 200, 255, 245, 150 };
		float bodyRadius = 5.f;
		float ringThickness = 1.f;
		float explosionDuration = 0.18f;
	};
	struct ModuleExplosionPresentationProfile
	{
		sas::ContentId profileId;
		sf::Color ring{ 120, 210, 255, 180 };
		float thickness = 2.f;
		float duration = 0.18f;
	};
	bool RegisterModuleGeneratedPresentationProfiles();
}
