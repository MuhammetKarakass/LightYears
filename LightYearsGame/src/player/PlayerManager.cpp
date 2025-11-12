#include "player/PlayerManager.h"

namespace ly
{
	unique_ptr<PlayerManager> PlayerManager::playerManager = nullptr;

	PlayerManager::PlayerManager()
	{
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
		mPlayers.emplace(mPlayers.begin(), Player());
		return mPlayers.back();
	}
	Player* PlayerManager::GetPlayer(int playerIndex)
	{
		if (playerIndex < 0 || playerIndex >= mPlayers.size())
			return nullptr;
		return &mPlayers[playerIndex];
	}
	const Player* PlayerManager::GetPlayer(int playerIndex) const
	{
		if (playerIndex < 0 || playerIndex >= mPlayers.size())
			return nullptr;
		return &mPlayers[playerIndex];
	}
	
	
}