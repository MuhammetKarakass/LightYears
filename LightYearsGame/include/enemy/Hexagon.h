#pragma once

#include "enemy/EnemySpaceShip.h"
#include "gameplay/ability/AbilitySystem.h"

namespace ly 
{
	class Hexagon : public EnemySpaceShip
	{
	public:

		Hexagon(World* owningWorld, const ShipDefinition& shipDef);

		virtual void Tick(float deltaTime) override;

	protected:

	private:
		virtual void Shoot() override;

		AbilitySystem mAbilitySystem;
	};

}
