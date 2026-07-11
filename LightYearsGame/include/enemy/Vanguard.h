#pragma once
#include "enemy/EnemySpaceShip.h"

namespace ly
{
	class Vanguard : public EnemySpaceShip
	{
	public:
		Vanguard(World* ownningWorld, const ShipDefinition& shipDef);

        ~Vanguard();

		virtual void Tick(float deltaTime) override;

		virtual void Shoot() override;

	protected:


	};
}


