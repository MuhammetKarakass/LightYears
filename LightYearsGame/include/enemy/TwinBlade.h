#pragma once

#include "enemy/EnemySpaceShip.h"
#include "gameplay/ability/AbilitySystem.h"

namespace ly
{
	class TwinBlade : public EnemySpaceShip
	{
	public:
		TwinBlade(World* owningWorld, const ShipDefinition& shipDef,float weaponSpreadWidth=40.f);

		~TwinBlade();

		virtual void Tick(float deltaTime) override;

		virtual void Shoot() override;


	private:
		AbilitySystem mAbilitySystem;
	};
}
