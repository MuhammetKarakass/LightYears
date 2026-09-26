#include "player/PlayerManager.h"
#include "widget/GameHUD.h"
#include <iostream>
#include <string>

namespace ly
{
	struct GameHUDPlayerRestartTestAccess
	{
		static void ConnectStatus(GameHUD& hud) { hud.ConnectStatus(); }
		static std::string GetLifeText(const GameHUD& hud)
		{
			return hud.mPlayerLifeText->mText.getString().toAnsiString();
		}
		static std::string GetScoreText(const GameHUD& hud)
		{
			return hud.mPlayerScoreText->mText.getString().toAnsiString();
		}
	};

	namespace
	{
		struct PlayerLifecycleProbe
		{
			int createdCount{ 0 };
			int removingCount{ 0 };

			void OnPlayerCreated(Player*) { ++createdCount; }
			void OnPlayerAboutToBeDestroyed(Player*) { ++removingCount; }
		};

		struct PlayerRemovalMutation
		{
			Player* expectedPlayer{ nullptr };
			int calls{ 0 };

			void MutateDuringRemoval(Player* player)
			{
				if (player != expectedPlayer) return;
				++calls;
				player->AddLifeCount(1);
				player->AddScore(17);
			}
		};

		bool HasStatus(const GameHUD& hud, unsigned int life, unsigned int score)
		{
			return GameHUDPlayerRestartTestAccess::GetLifeText(hud) == std::to_string(life) &&
				GameHUDPlayerRestartTestAccess::GetScoreText(hud) == std::to_string(score);
		}

		int Fail(const char* message)
		{
			std::cerr << "[GameHUD player restart test] " << message << '\n';
			return 1;
		}
	}
}

int RunGameHUDPlayerRestartTests()
{
	using namespace ly;
	PlayerManager& playerManager = PlayerManager::GetPlayerManager();
	playerManager.Reset();

	PlayerLifecycleProbe lifecycleProbe;
	const DelegateHandle createdProbeHandle = playerManager.onPlayerCreated.BindAction(
		&lifecycleProbe, &PlayerLifecycleProbe::OnPlayerCreated
	);
	const DelegateHandle removingProbeHandle = playerManager.onPlayerAboutToBeDestroyed.BindAction(
		&lifecycleProbe, &PlayerLifecycleProbe::OnPlayerAboutToBeDestroyed
	);

	Player& firstPlayer = playerManager.CreateNewPlayer();
	auto hud = std::make_shared<GameHUD>();
	GameHUDPlayerRestartTestAccess::ConnectStatus(*hud);
	if (!HasStatus(*hud, firstPlayer.GetLifeCount(), firstPlayer.GetScore()))
	{
		return Fail("first Player life and score values were not synchronized when the HUD connected");
	}

	firstPlayer.AddLifeCount(2);
	firstPlayer.AddScore(40);
	if (!HasStatus(*hud, firstPlayer.GetLifeCount(), firstPlayer.GetScore()))
	{
		return Fail("first Player life and score changes did not update the HUD");
	}

	PlayerRemovalMutation firstRemoval{ &firstPlayer };
	const unsigned int firstLifeBeforeReset = firstPlayer.GetLifeCount();
	const unsigned int firstScoreBeforeReset = firstPlayer.GetScore();
	const DelegateHandle firstRemovalHandle = playerManager.onPlayerAboutToBeDestroyed.BindAction(
		&firstRemoval, &PlayerRemovalMutation::MutateDuringRemoval
	);
	playerManager.Reset();
	playerManager.onPlayerAboutToBeDestroyed.UnbindAction(firstRemovalHandle);
	if (firstRemoval.calls != 1 || playerManager.GetPlayer() != nullptr ||
		!HasStatus(*hud, firstLifeBeforeReset, firstScoreBeforeReset))
	{
		return Fail("HUD remained subscribed while the first Player was being destroyed");
	}

	Player& secondPlayer = playerManager.CreateNewPlayer();
	if (!HasStatus(*hud, secondPlayer.GetLifeCount(), secondPlayer.GetScore()))
	{
		return Fail("HUD did not synchronize the replacement Player immediately after creation");
	}
	GameHUDPlayerRestartTestAccess::ConnectStatus(*hud);
	GameHUDPlayerRestartTestAccess::ConnectStatus(*hud);
	secondPlayer.AddLifeCount(2);
	secondPlayer.AddScore(20);
	if (!HasStatus(*hud, secondPlayer.GetLifeCount(), secondPlayer.GetScore()))
	{
		return Fail("repeated HUD refreshes changed Player life or score notifications");
	}

	PlayerRemovalMutation secondRemoval{ &secondPlayer };
	const unsigned int secondLifeBeforeReset = secondPlayer.GetLifeCount();
	const unsigned int secondScoreBeforeReset = secondPlayer.GetScore();
	const DelegateHandle secondRemovalHandle = playerManager.onPlayerAboutToBeDestroyed.BindAction(
		&secondRemoval, &PlayerRemovalMutation::MutateDuringRemoval
	);
	playerManager.Reset();
	playerManager.onPlayerAboutToBeDestroyed.UnbindAction(secondRemovalHandle);
	if (secondRemoval.calls != 1 ||
		!HasStatus(*hud, secondLifeBeforeReset, secondScoreBeforeReset))
	{
		return Fail("HUD did not disconnect exactly once before the second Player destruction");
	}

	Player& thirdPlayer = playerManager.CreateNewPlayer();
	if (!HasStatus(*hud, thirdPlayer.GetLifeCount(), thirdPlayer.GetScore()))
	{
		return Fail("HUD did not synchronize after the second restart");
	}
	thirdPlayer.AddLifeCount(1);
	thirdPlayer.AddScore(9);
	if (!HasStatus(*hud, thirdPlayer.GetLifeCount(), thirdPlayer.GetScore()))
	{
		return Fail("HUD stopped observing life or score after the second restart");
	}

	const weak_ptr<Object> weakHud = hud->GetWeakPtr();
	hud.reset();
	if (!weakHud.expired())
	{
		return Fail("HUD remained alive after its final shared owner was released");
	}
	thirdPlayer.AddLifeCount(1);
	thirdPlayer.AddScore(5);
	playerManager.Reset();
	Player& fourthPlayer = playerManager.CreateNewPlayer();
	fourthPlayer.AddLifeCount(1);
	fourthPlayer.AddScore(3);
	playerManager.Reset();
	playerManager.onPlayerCreated.UnbindAction(createdProbeHandle);
	playerManager.onPlayerAboutToBeDestroyed.UnbindAction(removingProbeHandle);
	if (lifecycleProbe.createdCount != 4 || lifecycleProbe.removingCount != 4)
	{
		return Fail("PlayerManager emitted duplicate or missing creation/destruction lifecycle events");
	}

	std::cout << "[C4] GameHUD Player restart lifecycle passed" << std::endl;
	return 0;
}
