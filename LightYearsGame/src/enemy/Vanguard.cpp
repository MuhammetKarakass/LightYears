#include "enemy/Vanguard.h"
#include <framework/World.h>
#include "gameplay/ability/controllers/PrimaryWeaponController.h"

namespace ly
{
	Vanguard::Vanguard(World* ownningWorld, const ShipDefinition& shipDef):
		EnemySpaceShip(ownningWorld, shipDef),
		mAbilitySystem{ this }
	{
		mAbilitySystem.AddController(
			AbilitySlot::PrimaryFire,
			std::make_unique<PrimaryWeaponController>(this, shipDef.primaryWeaponDefinition)
		);

		SetVelocity(shipDef.speed);
		SetActorRotation(180.f);
		mGameplayTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
	}

	Vanguard::~Vanguard()
	{
	}



	void Vanguard::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
		mAbilitySystem.Tick(deltaTime);
	}

	void Vanguard::Shoot()
	{
		mAbilitySystem.SetSlotInput(AbilitySlot::PrimaryFire, true);
	}
}
