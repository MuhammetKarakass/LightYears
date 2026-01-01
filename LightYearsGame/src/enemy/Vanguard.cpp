#include "enemy/Vanguard.h"
#include "weapon/BulletShooter.h"
#include <framework/World.h>

namespace ly
{
	Vanguard::Vanguard(World* ownningWorld, const ShipDefinition& shipDef):
		EnemySpaceShip(ownningWorld, shipDef),
		mShooter{nullptr}
	{
		SetVelocity(shipDef.speed);
		SetActorRotation(180.f);
		if(shipDef.hasWeapon)
		{
			mShooter = std::make_unique<BulletShooter>(
				this,
				shipDef.bulletDefinition,
				shipDef.weaponCooldown,
				shipDef.weaponOffset,
				0.f
			);
		}
		mGameplayTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
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
		if(mShooter)
		{
			mShooter->Shoot();  // Mermi atma iþlemi
		}
		else
		{
			LOG("Vanguard mShooter is not initialized!");  // Hata mesajý
		}
	}
}