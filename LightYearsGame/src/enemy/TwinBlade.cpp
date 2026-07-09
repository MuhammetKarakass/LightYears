#include "enemy/TwinBlade.h"
#include <framework/World.h>
#include "gameplay/ability/controllers/PrimaryWeaponController.h"

namespace ly
{
	TwinBlade::TwinBlade(World* owningWorld, const ShipDefinition& shipDef, float weaponSpreadWidth)
		: EnemySpaceShip(owningWorld, shipDef),
		mAbilitySystem{ this }
	{
		mAbilitySystem.AddController(
			AbilitySlot::PrimaryFire,
			std::make_unique<PrimaryWeaponController>(this, shipDef.primaryWeaponDefinition)
		);

		(void)weaponSpreadWidth;
		SetVelocity(shipDef.speed);
		SetActorRotation(180.f);
		SetScoreAmt(shipDef.scoreAmt);
		mGameplayTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
	}
	
	TwinBlade::~TwinBlade()
	{
	}
	
	void TwinBlade::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
		mAbilitySystem.Tick(deltaTime);
	}
	
	void TwinBlade::Shoot()
	{
		mAbilitySystem.SetSlotInput(AbilitySlot::PrimaryFire, true);
	}
}
