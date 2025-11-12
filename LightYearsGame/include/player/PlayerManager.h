#pragma once

#include <framework/Core.h>
#include "player/Player.h"

namespace ly
{
	class PlayerManager
	{
	public:

		Player& CreateNewPlayer();
		
		Player* GetPlayer(int playerIndex = 0);
		const Player* GetPlayer(int playerIndex = 0) const;
		const List<Player>& GetPlayers() const { return mPlayers; };

		static PlayerManager& GetPlayerManager();
	protected:
		PlayerManager();

	private:

		List<Player> mPlayers;
		static unique_ptr<PlayerManager> playerManager;
	};
}