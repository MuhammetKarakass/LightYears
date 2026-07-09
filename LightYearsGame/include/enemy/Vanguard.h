#pragma once
#include "enemy/EnemySpaceShip.h"
#include "gameplay/ability/AbilitySystem.h"

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


	private:
		AbilitySystem mAbilitySystem;
	};
}
