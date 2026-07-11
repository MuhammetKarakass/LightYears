#include "level/ArenaTestLevel.h"
#include "framework/AudioManager.h"

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
		AudioManager::GetAudioManager().FadeToMusic("SpaceShooterRedux/Musics/cosmic_reverie.ogg",
			AudioType::Music,
			5.0f,
			1.0f,
			true,
			1.0f);
	}
}


