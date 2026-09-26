#include "player/PlayerManager.h"

namespace ly
{
	unique_ptr<PlayerManager> PlayerManager::playerManager = nullptr;

	PlayerManager::PlayerManager()
	{
	}

	void PlayerManager::Reset()
	{
		for (Player& player : mPlayers)
		{
			onPlayerAboutToBeDestroyed.Broadcast(&player);
		}
		mPlayers.clear();
	}

	PlayerManager& PlayerManager::GetPlayerManager()
	{
		if (!playerManager)
		{
			playerManager = std::move(unique_ptr<PlayerManager>(new PlayerManager{}));
		}
		return *playerManager;
	}

	Player& PlayerManager::CreateNewPlayer()
	{
		mPlayers.emplace_back();
		Player& player = mPlayers.back();
		onPlayerCreated.Broadcast(&player);
		return player;
	}
	Player* PlayerManager::GetPlayer(int playerIndex)
	{
		if (playerIndex < 0 || static_cast<size_t>(playerIndex) >= mPlayers.size())
			return nullptr;
		return &mPlayers[playerIndex];
	}
	const Player* PlayerManager::GetPlayer(int playerIndex) const
	{
		if (playerIndex < 0 || static_cast<size_t>(playerIndex) >= mPlayers.size())
			return nullptr;
		return &mPlayers[playerIndex];
	}
	
	
}


