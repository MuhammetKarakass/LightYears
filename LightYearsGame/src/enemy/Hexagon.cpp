#include "enemy/Hexagon.h"

namespace ly
{
	Hexagon::Hexagon(World* owningWorld, const ShipDefinition& shipDef)
		: EnemySpaceShip(owningWorld, shipDef)
	{
		SetVelocity(shipDef.speed);
		SetActorRotation(180.f);
		SetScoreAmt(shipDef.scoreAmt);

		mAttachedLightTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
	}

	void Hexagon::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
	}

	void Hexagon::Shoot()
	{
		GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	}
}


