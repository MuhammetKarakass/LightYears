#include "level/ArenaTestLevel.h"
#include "framework/AudioManager.h"
#include "enemy/DummyEnemy.h"
#include "gameConfigs/ship/ShipConfig.h"

namespace ly
{
	ArenaTestLevel::ArenaTestLevel(Application* owningApp)
		:ArenaLevel(owningApp)
	{
	}

	ArenaDefinition ArenaTestLevel::CreateArenaDefinition() const
	{
		ArenaDefinition arenaDefinition;
		arenaDefinition.size = sf::Vector2f{ 6000.f, 3000.f };
		arenaDefinition.legalBounds = sf::FloatRect{ sf::Vector2f{ 0.f, 0.f }, arenaDefinition.size };
		arenaDefinition.outOfBoundsMargin = 20.f;
		arenaDefinition.outOfBoundsTime = 5.f;

		arenaDefinition.boundaryVisual.enabled = true;
		arenaDefinition.boundaryVisual.visualType = ArenaBoundaryVisualType::DebugRectangle;
		arenaDefinition.boundaryVisual.outlineColor = sf::Color{ 90, 230, 255, 180 };
		arenaDefinition.boundaryVisual.warningColor = sf::Color{ 255, 80, 80, 220 };
		arenaDefinition.boundaryVisual.outlineThickness = 4.f;

		return arenaDefinition;
	}

	PlayerRespawnDefinition ArenaTestLevel::CreatePlayerRespawnDefinition() const
	{
		PlayerRespawnDefinition respawnDefinition = ArenaLevel::CreatePlayerRespawnDefinition();
		respawnDefinition.respawnDelay = 1.f;
		respawnDefinition.playerIndex = 0;
		respawnDefinition.createPlayerIfMissing = true;
		respawnDefinition.useScreenClamp = false;
		respawnDefinition.respawnWhenDestroyed = true;
		respawnDefinition.gameOverWhenRespawnFails = true;
		respawnDefinition.playerShipId = "Default";

		return respawnDefinition;
	}

	void ArenaTestLevel::OnGameStart()
	{
		ArenaLevel::OnGameStart();
		ShipDefinition oldDummyDefinition = ShipData::Ship_Enemy_Hexagon;
		oldDummyDefinition.health = 99999.f;
		oldDummyDefinition.speed = { 0.f, 0.f };
		oldDummyDefinition.primaryWeaponId = "Weapon.Projectile.FighterRapidLaser.Basic";

		ShipDefinition newDummyDefinition = oldDummyDefinition;
		newDummyDefinition.health = 50.f;

		const sf::FloatRect& arenaBounds = GetArenaDefinition().legalBounds;
		const sf::Vector2f arenaCenter{
			arenaBounds.position.x + arenaBounds.size.x * 0.5f,
			arenaBounds.position.y + arenaBounds.size.y * 0.5f
		};

		const float squareHalfExtent = 250.f;
		const auto spawnDummy = [&](const ShipDefinition& definition, float horizontalOffset, float verticalOffset)
		{
			const sf::Vector2f dummyLocation{
				arenaCenter.x + horizontalOffset,
				arenaCenter.y + verticalOffset
			};
			if (auto dummy = SpawnActor<DummyEnemy>(definition).lock())
			{
				dummy->SetActorLocation(dummyLocation);
				dummy->SetVelocity({ 0.f, 0.f });
				dummy->SetActorRotation(0.f);
			}
		};

		// Existing stationary targets remain in their original horizontal layout.
		spawnDummy(oldDummyDefinition, -900.f, 450.f);
		spawnDummy(oldDummyDefinition, -300.f, 450.f);
		spawnDummy(oldDummyDefinition, 300.f, 450.f);
		spawnDummy(oldDummyDefinition, 900.f, 450.f);

		// New 50-health stationary targets at the corners of a square around arena center.
		spawnDummy(newDummyDefinition, -squareHalfExtent, -squareHalfExtent);
		spawnDummy(newDummyDefinition, squareHalfExtent, -squareHalfExtent);
		spawnDummy(newDummyDefinition, -squareHalfExtent, squareHalfExtent);
		spawnDummy(newDummyDefinition, squareHalfExtent, squareHalfExtent);

		AudioManager::GetAudioManager().FadeToMusic("SpaceShooterRedux/Musics/cosmic_reverie.ogg",
			AudioType::Music,
			5.0f,
			1.0f,
			true,
			1.0f);
	}
}


