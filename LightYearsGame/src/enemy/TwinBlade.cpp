#include "enemy/TwinBlade.h"
#include "weapon/BulletShooter.h"
#include <framework/World.h>

namespace ly
{
	TwinBlade::TwinBlade(World* owningWorld, const ShipDefinition& shipDef,float weaponSpreadWidth)
		: EnemySpaceShip(owningWorld, shipDef),
		mShooterLeft( new BulletShooter(this, shipDef.bulletDefinition, shipDef.weaponCooldown, sf::Vector2f{shipDef.weaponOffset.x - weaponSpreadWidth / 2, shipDef.weaponOffset.y} )),
		mShooterRight( new BulletShooter(this, shipDef.bulletDefinition, shipDef.weaponCooldown, sf::Vector2f{shipDef.weaponOffset.x + weaponSpreadWidth / 2, shipDef.weaponOffset.y} ))
	{
		SetVelocity(shipDef.speed);
		SetActorRotation(180.f);
		SetScoreAmt(shipDef.scoreAmt);
		mShooterLeft->SetBulletSpeed(400.f);
		mShooterRight->SetBulletSpeed(400.f);
		mGameplayTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
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
		if (mShooterLeft && mShooterRight)
		{
			mShooterLeft->Shoot();
			mShooterRight->Shoot();
		}
	}
}