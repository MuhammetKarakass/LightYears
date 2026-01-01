#pragma once

#include "enemy/EnemySpaceShip.h"

namespace ly 
{
	class BulletShooter;

	class Hexagon : public EnemySpaceShip
	{
	public:

		Hexagon(World* owningWorld, const ShipDefinition& shipDef);

		virtual void Tick(float deltaTime) override;

	protected:

	private:
		virtual void Shoot() override;
		void SetupShooters(const ShipDefinition& shipDef);

		List<unique_ptr<BulletShooter>> mShooters;
	};

}