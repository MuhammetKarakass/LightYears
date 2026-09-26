#include "framework/Application.h"
#include "framework/AssetManager.h"
#include "framework/World.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include <iostream>
#include <string>

namespace
{
	int Fail(const char* message)
	{
		std::cerr << "[Player ship lifetime test] " << message << '\n';
		return 1;
	}

	struct PlayerManagerResetGuard
	{
		ly::PlayerManager& playerManager;

		~PlayerManagerResetGuard()
		{
			playerManager.Reset();
		}
	};

	struct ShipDestroyedProbe
	{
		int calls{ 0 };

		void OnShipDestroyed(ly::Actor*)
		{
			++calls;
		}
	};
}

int RunPlayerShipLifetimeTests()
{
	using namespace ly;
	AssetManager::GetAssetManager().SetAssetRootDirectory("LightYearsGame/assets/");
	if (!GameContentBootstrap::Register())
	{
		return Fail("Game content bootstrap could not register shipped content");
	}

	PlayerManager& playerManager = PlayerManager::GetPlayerManager();
	playerManager.Reset();
	PlayerManagerResetGuard resetGuard{ playerManager };
	std::string windowTitle = "Player ship lifetime test";
	Application application{ sf::Vector2u{ 32u, 32u }, 32u, windowTitle, sf::Style::None };
	application.GetRenderWindow().setVisible(false);
	World world{ &application };

	Player& firstPlayer = playerManager.CreateNewPlayer();
	Player* firstPlayerAddress = &firstPlayer;
	Player& secondPlayer = playerManager.CreateNewPlayer();
	if (playerManager.GetPlayer(0) != firstPlayerAddress ||
		&playerManager.GetPlayers().front() != firstPlayerAddress)
	{
		return Fail("Appending a Player invalidated the first manager-owned Player address");
	}

	const shared_ptr<PlayerSpaceShip> firstShip = firstPlayer.SpawnSpaceShip(&world).lock();
	const shared_ptr<PlayerSpaceShip> secondShip = secondPlayer.SpawnSpaceShip(&world).lock();
	if (!firstShip || !secondShip ||
		firstPlayer.GetCurrentSpaceShip().lock() != firstShip ||
		secondPlayer.GetCurrentSpaceShip().lock() != secondShip ||
		firstPlayer.GetLifeCount() != 2 || secondPlayer.GetLifeCount() != 2)
	{
		return Fail("Players did not spawn and retain their independent current ships");
	}

	firstShip->Destroy();
	if (firstPlayer.GetCurrentSpaceShip().lock() ||
		secondPlayer.GetCurrentSpaceShip().lock() != secondShip ||
		firstPlayer.GetLifeCount() != 2 || secondPlayer.GetLifeCount() != 2)
	{
		return Fail("Destroying the first ship cleared or changed the other Player's state");
	}

	ShipDestroyedProbe destroyedProbe;
	const DelegateHandle destroyedProbeHandle = secondShip->onActorDestroyed.BindAction(
		&destroyedProbe,
		&ShipDestroyedProbe::OnShipDestroyed
	);
	playerManager.Reset();
	if (!playerManager.GetPlayers().empty())
	{
		return Fail("PlayerManager did not clear its Players during Reset");
	}
	secondShip->Destroy();
	secondShip->Destroy();
	secondShip->onActorDestroyed.UnbindAction(destroyedProbeHandle);
	if (destroyedProbe.calls != 1)
	{
		return Fail("Destroying the retained ship after Player reset did not emit exactly one event");
	}
	return 0;
}
