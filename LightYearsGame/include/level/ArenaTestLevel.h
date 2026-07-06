#pragma once

#include "level/ArenaLevel.h"

namespace ly
{
	class Player;
	class Actor;
	class PlayerSpaceShip;
	class ArenaTestLevel : public ArenaLevel
	{
	public:
		ArenaTestLevel(Application* owningApp);

	protected:
		virtual ArenaDefinition CreateArenaDefinition() const override;
		virtual void OnGameStart() override;
		virtual void Tick(float deltaTime) override;

	private:
		void SpawnPlayer();

		void SpawnPlayerShip(Player& player);
		void ConfigurePlayerShip(weak_ptr<PlayerSpaceShip> playerShip);
		void PlayerShipDestroyed(Actor* playerShip);

		weak_ptr<PlayerSpaceShip> mPlayerSpaceShip;
	};
}
