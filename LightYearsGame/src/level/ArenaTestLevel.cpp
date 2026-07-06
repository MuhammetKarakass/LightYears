#include "level/ArenaTestLevel.h"
#include "player/PlayerManager.h"
#include "player/Player.h"
#include "player/PlayerSpaceShip.h"
#include "framework/AudioManager.h"

namespace ly
{
	ArenaTestLevel::ArenaTestLevel(Application* owningApp)
		:ArenaLevel(owningApp),
		mPlayerSpaceShip{}
	{
	}
	ArenaDefinition ArenaTestLevel::CreateArenaDefinition() const
	{
		ArenaDefinition arenaDefinition;
		arenaDefinition.size = sf::Vector2f{ 3000.f, 2000.f };
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
	void ArenaTestLevel::OnGameStart()
	{
		ArenaLevel::OnGameStart();
		SpawnPlayer();
		AudioManager::GetAudioManager().FadeToMusic("SpaceShooterRedux/Musics/cosmic_reverie.ogg",
			AudioType::Music,
			5.0f,
			1.0f,
			true,
			1.0f);
	}
	void ArenaTestLevel::Tick(float deltaTime)
	{
		ArenaLevel::Tick(deltaTime);
	}
	void ArenaTestLevel::SpawnPlayer()
	{
		Player& player = PlayerManager::GetPlayerManager().CreateNewPlayer();

		SpawnPlayerShip(player);
	}

	void ArenaTestLevel::SpawnPlayerShip(Player& player)
	{
		mPlayerSpaceShip = player.SpawnSpaceShip(this);
		if (auto playerShip = mPlayerSpaceShip.lock())
		{
			playerShip->onActorDestroyed.BindAction(GetWeakPtr(), &ArenaTestLevel::PlayerShipDestroyed);
			ConfigurePlayerShip(playerShip);
		}
	}
	void ArenaTestLevel::ConfigurePlayerShip(weak_ptr<PlayerSpaceShip> playerShip)
	{
		if (auto ship = playerShip.lock())
		{
			const ArenaDefinition& arenaDef = GetArenaDefinition();
			const sf::FloatRect& legalBounds = arenaDef.legalBounds;

			const sf::Vector2f arenaCenter{legalBounds.position.x + legalBounds.size.x * 0.5f,
										   legalBounds.position.y + legalBounds.size.y * 0.5f};

			ship->SetActorLocation(arenaCenter);
			ship->SetUseScreenClamp(false);

			SetViewTarget(playerShip);
			SetArenaTrackedActor(playerShip);
		}

	}
	void ArenaTestLevel::PlayerShipDestroyed(Actor* destroyedActor)
	{
		ClearViewTarget();
		SetArenaTrackedActor(weak_ptr<Actor>{});

		Player* player = PlayerManager::GetPlayerManager().GetPlayer();

		if (!player)
		{
			GameOver();
			return;
		}

		SpawnPlayerShip(*player);

		if (mPlayerSpaceShip.expired())
		{
			TimerManager::GetGlobalTimerManager().SetTimer(
				GetWeakPtr(),
				&ArenaTestLevel::GameOver,
				3.f,
				false
			);
		}
	}
}
