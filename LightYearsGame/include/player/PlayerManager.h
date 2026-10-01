#pragma once

#include <framework/Core.h>
#include "player/Player.h"
#include <deque>
#include <string>

namespace ly
{
	class PlayerManager
	{
	public:

		Player& CreateNewPlayer();
		void SetSelectedShipId(std::string shipId);
		const std::string& GetSelectedShipId() const { return mSelectedShipId; }
		
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
		std::string mSelectedShipId{ DefaultPlayerShipId };
		static unique_ptr<PlayerManager> playerManager;
	};
}

