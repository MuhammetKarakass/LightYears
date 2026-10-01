#include "player/PlayerManager.h"
#include "presentation/hud/vitals/VitalsPresenter.h"
#include <iostream>

namespace ly
{
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

	bool HasStatus(const VitalsPresenter& presenter, unsigned int life, unsigned int score)
		{
			const auto viewModel = presenter.GetViewModel();
			return viewModel && viewModel->hasPlayer && viewModel->life == life && viewModel->score == score;
		}

		bool HasClearedStatus(const VitalsPresenter& presenter)
		{
			const auto viewModel = presenter.GetViewModel();
			return viewModel && !viewModel->hasPlayer && viewModel->life == 0 && viewModel->score == 0;
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
	const DelegateHandle createdProbeHandle = playerManager.onPlayerCreated.BindAction(&lifecycleProbe, &PlayerLifecycleProbe::OnPlayerCreated);
	const DelegateHandle removingProbeHandle = playerManager.onPlayerAboutToBeDestroyed.BindAction(&lifecycleProbe, &PlayerLifecycleProbe::OnPlayerAboutToBeDestroyed);

	Player& firstPlayer = playerManager.CreateNewPlayer();
	VitalsPresenter presenter;
	presenter.Tick();
	if (!HasStatus(presenter, firstPlayer.GetLifeCount(), firstPlayer.GetScore())) return Fail("presenter did not synchronize first Player life and score");

	firstPlayer.AddLifeCount(2);
	firstPlayer.AddScore(40);
	if (!HasStatus(presenter, firstPlayer.GetLifeCount(), firstPlayer.GetScore())) return Fail("first Player life or score event did not update the view model");

	PlayerRemovalMutation firstRemoval{ &firstPlayer };
	const DelegateHandle firstRemovalHandle = playerManager.onPlayerAboutToBeDestroyed.BindAction(&firstRemoval, &PlayerRemovalMutation::MutateDuringRemoval);
	playerManager.Reset();
	playerManager.onPlayerAboutToBeDestroyed.UnbindAction(firstRemovalHandle);
	if (firstRemoval.calls != 1 || playerManager.GetPlayer() != nullptr || !HasClearedStatus(presenter))
		return Fail("presenter remained subscribed while first Player was being destroyed");

	Player& secondPlayer = playerManager.CreateNewPlayer();
	presenter.Tick();
	if (!HasStatus(presenter, secondPlayer.GetLifeCount(), secondPlayer.GetScore())) return Fail("presenter did not synchronize replacement Player immediately");
	presenter.Tick();
	presenter.Tick();
	secondPlayer.AddLifeCount(2);
	secondPlayer.AddScore(20);
	if (!HasStatus(presenter, secondPlayer.GetLifeCount(), secondPlayer.GetScore())) return Fail("repeated presenter refreshes changed or missed Player notifications");

	PlayerRemovalMutation secondRemoval{ &secondPlayer };
	const DelegateHandle secondRemovalHandle = playerManager.onPlayerAboutToBeDestroyed.BindAction(&secondRemoval, &PlayerRemovalMutation::MutateDuringRemoval);
	playerManager.Reset();
	playerManager.onPlayerAboutToBeDestroyed.UnbindAction(secondRemovalHandle);
	if (secondRemoval.calls != 1 || !HasClearedStatus(presenter)) return Fail("presenter did not disconnect exactly once before second Player destruction");

	Player& thirdPlayer = playerManager.CreateNewPlayer();
	presenter.Tick();
	if (!HasStatus(presenter, thirdPlayer.GetLifeCount(), thirdPlayer.GetScore())) return Fail("presenter did not synchronize after second restart");
	thirdPlayer.AddLifeCount(1);
	thirdPlayer.AddScore(9);
	if (!HasStatus(presenter, thirdPlayer.GetLifeCount(), thirdPlayer.GetScore())) return Fail("presenter stopped observing Player life or score after restart");

	playerManager.Reset();
	Player& fourthPlayer = playerManager.CreateNewPlayer();
	presenter.Tick();
	fourthPlayer.AddLifeCount(1);
	fourthPlayer.AddScore(3);
	playerManager.Reset();
	playerManager.onPlayerCreated.UnbindAction(createdProbeHandle);
	playerManager.onPlayerAboutToBeDestroyed.UnbindAction(removingProbeHandle);
	if (lifecycleProbe.createdCount != 4 || lifecycleProbe.removingCount != 4) return Fail("PlayerManager emitted duplicate or missing creation/destruction lifecycle events");

	std::cout << "[C4] GameHUD Player restart lifecycle passed" << std::endl;
	return 0;
}
