#include "player/PlayerManager.h"

#include <utility>

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

	void PlayerManager::SetSelectedShipId(std::string shipId)
	{
		mSelectedShipId = std::move(shipId);
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
		mPlayers.emplace_back(mSelectedShipId);
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


