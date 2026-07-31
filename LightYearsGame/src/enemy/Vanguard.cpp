#include "enemy/Vanguard.h"
#include <framework/World.h>

namespace ly
{
	Vanguard::Vanguard(World* ownningWorld, const ShipDefinition& shipDef):
		EnemySpaceShip(ownningWorld, shipDef)
	{
		SetVelocity(shipDef.speed);
		SetActorRotation(180.f);
		mAttachedLightTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
	}

	Vanguard::~Vanguard()
	{
	}



	void Vanguard::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
	}

	void Vanguard::Shoot()
	{
		GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	}
}


