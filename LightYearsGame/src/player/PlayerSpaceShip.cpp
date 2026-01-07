#include "player/PlayerSpaceShip.h"
#include <framework/World.h>
#include <framework/MathUtility.h>
#include "weapon/BulletShooter.h"
#include "weapon/ThreeWayShooter.h"
#include "weapon/FrontalWiper.h"
#include "weapon/Shooter.h"
#include "framework/AssetManager.h"
#include "gameConfigs/GameplayConfig.h"

namespace ly
{
	PlayerSpaceShip::PlayerSpaceShip(World* owningWorld, const ShipDefinition& shipDef)
		:SpaceShip(owningWorld, shipDef),
		mSpeed(shipDef.speed.x),
		mMoveInput{ 0.f, 0.f },
		mShooter{ new BulletShooter{this, shipDef.bulletDefinition, shipDef.weaponCooldown, shipDef.weaponOffset} },
		mWeaponType{ WeaponType::Default },
		mInvulnerabilityTime{ 2.f },
		mInvulnerable{ true },
		mInvulnerabilityBlinkInterval{ 0.4f },
		mInvulnerabilityBlinkTimer{ 0.f },
		mInvulnerabilityDir{ 1.f },
		mCollisionDamage{ shipDef.collisionDamage }
	{
		SetActorRotation(0.f);
		mGameplayTags.push_back(AddLight(GameTags::Ship::Engine_Left, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
		mGameplayTags.push_back(AddLight(GameTags::Ship::Engine_Right, shipDef.engineMounts[1].pointLightDef, shipDef.engineMounts[1].offset));
	}

	void PlayerSpaceShip::SetShooter(unique_ptr<Shooter>&& shooter, WeaponType type)
	{
		if (!shooter)
		{
			return;
		}
		if (type == mWeaponType && mShooter)
		{
			int oldLevel = mShooter->GetCurrentLevel();
			mShooter->IncrementLevel();
			int newLevel = mShooter->GetCurrentLevel();
			return;
		}

		int oldLevel = mShooter ? mShooter->GetCurrentLevel() : 1;
		mShooter = std::move(shooter);
		mWeaponType = type;

		if (mShooter && oldLevel > 1)
		{
			mShooter->SetCurrentLevel(oldLevel);
		}
		else
		{
		}
	}

	void PlayerSpaceShip::SetupCollisionLayers()
	{
		SetCollisionLayer(CollisionLayer::Player);
		SetCollisionMask(CollisionLayer::Enemy | CollisionLayer::EnemyBullet | CollisionLayer::Powerup);

	}

	void PlayerSpaceShip::BeginPlay()
	{
		SpaceShip::BeginPlay();
		

		TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			&PlayerSpaceShip::StopInvulnerability,
			mInvulnerabilityTime,
			false
		);
	}

	void PlayerSpaceShip::ApplyDamage(float amt)
	{
		if (mInvulnerable)
		{
			return;
		}

		float currentHealth = GetHealthComponent().GetHealth();
		LOG("PlayerSpaceShip::ApplyDamage - Current Health: %.1f, Damage: %.1f", currentHealth, amt);

		if (currentHealth - amt <= 0.f)
		{
			LOG("===== PLAYER SHIP DYING - Broadcasting weapon state =====");
			LOG("WeaponType: %d, Level: %d", (int)mWeaponType, mShooter ? mShooter->GetCurrentLevel() : 0);

			if (mShooter)
			{
				onWeaponStateBeforeDeath.Broadcast(mWeaponType, mShooter->GetCurrentLevel());
			}
			else
			{
				LOG("ERROR: mShooter is null!");
				onWeaponStateBeforeDeath.Broadcast(WeaponType::Default, 1);
			}
		}
		SpaceShip::ApplyDamage(amt);
	}

	void PlayerSpaceShip::Tick(float deltaTime)
	{
		SpaceShip::Tick(deltaTime);
		mShaderTime += deltaTime;
		SetInput();
		ConsumeInput();
		if (mInvulnerable)
		{
			UpdateInvulnerability(deltaTime);
		}

	}

	void PlayerSpaceShip::SetInput()
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
		{
			mMoveInput.y = -1.f;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
		{
			mMoveInput.y = 1.f;
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
		{
			mMoveInput.x = -1.f;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
		{
			mMoveInput.x = 1.f;
		}

		ClampInputOnEdge();
		NormalizeInput();  
	}

	void PlayerSpaceShip::ConsumeInput()
	{
		SetVelocity(mMoveInput * mSpeed);
		mMoveInput.x = 0.f;
		mMoveInput.y = 0.f;
	}

	void PlayerSpaceShip::NormalizeInput()
	{
		NormalizeVector(mMoveInput);
	}

	void PlayerSpaceShip::ClampInputOnEdge()
	{
		if (GetActorLocation().x - 40.f <= 0.f && mMoveInput.x == -1.f)
		{
			mMoveInput.x = 0.f;
		}
		else if (GetActorLocation().x + 40.f >= GetWindowSize().x && mMoveInput.x == 1.f)
		{
			mMoveInput.x = 0.f;
		}

		if (GetActorLocation().y - 40.f <= 0.f && mMoveInput.y == -1.f)
		{
			mMoveInput.y = 0.f;
		}
		else if (GetActorLocation().y + 40.f >= GetWindowSize().y && mMoveInput.y == 1.f)
		{
			mMoveInput.y = 0.f;
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
		{
			Shoot();
		}
	}

	void PlayerSpaceShip::Shoot()
	{
		if (mShooter)
		{
			mShooter->Shoot();
		}
	}

	void PlayerSpaceShip::StopInvulnerability()
	{
		GetSprite().value().setColor({255,255,255,255});
		mInvulnerable = false;

		SetAllLightsIntensity(GameTags::Ship::Engine_Left, 1.5f);
		SetAllLightsIntensity(GameTags::Ship::Engine_Right, 1.5f);
	}

	void PlayerSpaceShip::UpdateInvulnerability(float deltaTime)
	{
		mInvulnerabilityBlinkTimer += deltaTime * mInvulnerabilityDir;
		if (mInvulnerabilityBlinkTimer < 0 || mInvulnerabilityBlinkTimer > mInvulnerabilityBlinkInterval)
		{
			mInvulnerabilityDir *= -1;
		}

		float blinkAlpha = mInvulnerabilityBlinkTimer / mInvulnerabilityBlinkInterval;
		GetSprite().value().setColor(LerpColor({ 255,255, 255, 96 }, { 255, 255, 255, 160 }, blinkAlpha));
		float lightIntensity = 0.3f + (blinkAlpha * 5.f);
		SetAllLightsIntensity(GameTags::Ship::Engine_Left, lightIntensity);
		SetAllLightsIntensity(GameTags::Ship::Engine_Right, lightIntensity);
	}

	void PlayerSpaceShip::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);

		if (otherActor && GetCanCollide() && !mInvulnerable)
		{
			otherActor->ApplyDamage(mCollisionDamage);
		}
	}

	int PlayerSpaceShip::GetWeaponLevel() const
	{
		return mShooter ? mShooter->GetCurrentLevel() : 0;
	}

	void PlayerSpaceShip::ApplyWeaponState(const WeaponState& state)
	{
		if (state.type == WeaponType::Default || state.level < 1)
		{
			mShooter = std::make_unique<BulletShooter>(
				this,
				GameData::Laser_Blue_BulletDef,
				0.2f,
				sf::Vector2f{ 0.f, 50.f }
			);
			mWeaponType = WeaponType::Default;
			return;
		}

		switch (state.type)
		{
		case WeaponType::ThreeWay:
			mShooter = std::make_unique<ThreeWayShooter>(
				this,
				GameData::Laser_Blue_BulletDef,
				0.4f,
				sf::Vector2f{ 0.f, 50.f }
			);
			mWeaponType = WeaponType::ThreeWay;
			break;

		case WeaponType::FrontalWhiper:
			mShooter = std::make_unique<FrontalWiper>(
				this,
				GameData::Laser_Blue_BulletDef,
				0.5f,
				sf::Vector2f{ 0.f, 50.f }
			);
			mWeaponType = WeaponType::FrontalWhiper;
			break;

		default:
			return;
		}

		if (mShooter)
		{
			mShooter->SetCurrentLevel(state.level);
		}
	}
}