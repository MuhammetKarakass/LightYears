#pragma once

#include "spaceShip/SpaceShip.h"

namespace vega
{
	class DummyEnemy : public SpaceShip
	{
		VEGA_CLASS(DummyEnemy)
	public:
		DummyEnemy(World* owningWorld, const ShipDefinition& shipDef);

		void Tick(float deltaTime) override;
		void SetupCollisionLayers() override;

	protected:
		void Shoot() override;
	};
}
