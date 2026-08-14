#pragma once

#include "enemy/EnemySpaceShip.h"

namespace ly
{
	class DummyEnemy : public EnemySpaceShip
	{
	public:
		DummyEnemy(World* owningWorld, const ShipDefinition& shipDef);

		void Tick(float deltaTime) override;

	protected:
		void Shoot() override;
	};
}
