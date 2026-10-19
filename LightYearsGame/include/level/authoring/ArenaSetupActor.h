#pragma once

#include "framework/Actor.h"
#include "framework/reflection/Reflection.h"
#include "level/ArenaDefinition.h"
#include "player/PlayerRespawnDefinition.h"

namespace vega
{
	struct ArenaBoundarySettings
	{
		VEGA_STRUCT(ArenaBoundarySettings)
		bool enabled = true;
		ArenaBoundaryVisualType visualType = ArenaBoundaryVisualType::DebugRectangle;
		float outOfBoundsMargin = 20.f;
		float outOfBoundsTime = 5.f;
		sf::Color outlineColor{ 90, 230, 255, 180 };
		sf::Color warningColor{ 255, 80, 80, 220 };
		float outlineThickness = 4.f;
	};
	struct ArenaRespawnSettings
	{
		VEGA_STRUCT(ArenaRespawnSettings)
		float respawnDelay = 1.f;
		int playerIndex = 0;
		bool createPlayerIfMissing = true;
		bool useScreenClamp = false;
		bool respawnWhenDestroyed = true;
		bool gameOverWhenRespawnFails = true;
		std::string playerShipId = "Default";
	};
	struct ArenaMusicSettings
	{
		VEGA_STRUCT(ArenaMusicSettings)
		std::string path = "SpaceShooterRedux/Musics/cosmic_reverie.ogg";
		float volume = 5.f;
		float pitch = 1.f;
		bool loop = true;
		float fadeDuration = 1.f;
	};
	struct ArenaHeavyDummySettings
	{
		VEGA_STRUCT(ArenaHeavyDummySettings)
		std::string ship = "Ship.Enemy.RangeKeeper.Basic";
		float health = 9000.f;
		float armor = 40.f;
		sf::Vector2f legacySpeed{};
		std::string primaryWeaponId = "Weapon.Projectile.FighterRapidLaser.Basic";
	};
	class ArenaSetupActor : public Actor
	{
		VEGA_CLASS(ArenaSetupActor)
	public:
		explicit ArenaSetupActor(World* world) : Actor{ world } {}
		ArenaBoundarySettings boundary;
		ArenaRespawnSettings respawn;
		ArenaMusicSettings music;
		ArenaHeavyDummySettings heavyDummy;
		float dummyArmor = 40.f;
		std::vector<sf::Vector2f> heavyDummyPositions{ {2100.f,1950.f}, {2700.f,1950.f}, {3300.f,1950.f}, {3900.f,1950.f} };
	};
}
