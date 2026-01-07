#include "enemy/Hexagon.h"
#include "weapon/BulletShooter.h"

namespace ly
{
	Hexagon::Hexagon(World* owningWorld, const ShipDefinition& shipDef)
		: EnemySpaceShip(owningWorld, shipDef)
	{
		SetVelocity(shipDef.speed);
		SetActorRotation(180.f);
		SetScoreAmt(shipDef.scoreAmt);
		SetupShooters(shipDef);

		mGameplayTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
	}

	void Hexagon::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
	}

	void Hexagon::Shoot()
	{
		for (auto& shooter : mShooters)
		{
			if (shooter)
			{
				shooter->Shoot();
			}
		}
	}

	void Hexagon::SetupShooters(const ShipDefinition& shipDef)
	{
		mShooters.push_back(std::make_unique<BulletShooter>(this, shipDef.bulletDefinition, shipDef.weaponCooldown, sf::Vector2f{0.f, 50.f}, 0.f));
		mShooters.push_back(std::make_unique<BulletShooter>(this, shipDef.bulletDefinition, shipDef.weaponCooldown, sf::Vector2f{ 0.f, -50.f }, 180.f));

		mShooters.push_back(std::make_unique<BulletShooter>(this, shipDef.bulletDefinition, shipDef.weaponCooldown*2, sf::Vector2f{50.f, 50.f}, 45.f));
		mShooters.push_back(std::make_unique<BulletShooter>(this, shipDef.bulletDefinition, shipDef.weaponCooldown*2, sf::Vector2f{-50.f, 50.f}, -45.f));

		mShooters.push_back(std::make_unique<BulletShooter>(this, shipDef.bulletDefinition, shipDef.weaponCooldown*2, sf::Vector2f{ -50.f, -50.f}, -135.f));
		mShooters.push_back(std::make_unique<BulletShooter>(this, shipDef.bulletDefinition, shipDef.weaponCooldown*2, sf::Vector2f{ 50.f, -50.f}, 135.f));

		for (auto& shooter : mShooters) 
		{
			shooter->SetBulletSpeed(400.f);
		}
	}
}
