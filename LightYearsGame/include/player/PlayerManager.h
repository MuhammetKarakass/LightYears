#pragma once

#include <framework/Core.h>
#include "player/Player.h"
#include <deque>

namespace ly
{
	class PlayerManager
	{
	public:

		Player& CreateNewPlayer();
		
		Player* GetPlayer(int playerIndex = 0);
		const Player* GetPlayer(int playerIndex = 0) const;
		const std::deque<Player>& GetPlayers() const { return mPlayers; };

		void Reset();

		static PlayerManager& GetPlayerManager();

		Delegate<Player*> onPlayerAboutToBeDestroyed;
		Delegate<Player*> onPlayerCreated;
	protected:
		PlayerManager();

	private:

		std::deque<Player> mPlayers;
		static unique_ptr<PlayerManager> playerManager;
	};
}

