#include "enemy/Hexagon.h"
#include "gameplay/ability/controllers/PrimaryWeaponController.h"

namespace ly
{
	Hexagon::Hexagon(World* owningWorld, const ShipDefinition& shipDef)
		: EnemySpaceShip(owningWorld, shipDef),
		mAbilitySystem{ this }
	{
		mAbilitySystem.AddController(
			AbilitySlot::PrimaryFire,
			std::make_unique<PrimaryWeaponController>(this, shipDef.primaryWeaponDefinition)
		);

		SetVelocity(shipDef.speed);
		SetActorRotation(180.f);
		SetScoreAmt(shipDef.scoreAmt);

		mGameplayTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
	}

	void Hexagon::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
		mAbilitySystem.Tick(deltaTime);
	}

	void Hexagon::Shoot()
	{
		mAbilitySystem.SetSlotInput(AbilitySlot::PrimaryFire, true);
	}
}
