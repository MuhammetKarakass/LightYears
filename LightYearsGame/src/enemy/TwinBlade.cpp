#include "enemy/TwinBlade.h"
#include <framework/World.h>

namespace ly
{
	TwinBlade::TwinBlade(World* owningWorld, const ShipDefinition& shipDef, float weaponSpreadWidth)
		: EnemySpaceShip(owningWorld, shipDef)
	{
		(void)weaponSpreadWidth;
		SetVelocity(shipDef.speed);
		SetActorRotation(180.f);
		SetScoreAmt(shipDef.scoreAmt);
		mAttachedLightTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
	}
	
	TwinBlade::~TwinBlade()
	{
	}
	
	void TwinBlade::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
	}
	
	void TwinBlade::Shoot()
	{
		GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	}
}


