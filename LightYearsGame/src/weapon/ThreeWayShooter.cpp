#include "weapon/ThreeWayShooter.h"

namespace ly
{
	ThreeWayShooter::ThreeWayShooter(Actor* owner,	
		const BulletDefinition& bulletDef,
		float cooldownTime,
		const sf::Vector2f& localPositionOffset,
		float localRotationOffset) :
		Shooter{owner, ShootSoundMode::Composite},
		mLeftShooter{ new BulletShooter{owner, bulletDef,cooldownTime ,localPositionOffset + sf::Vector2f{-20.f,-5.f},localRotationOffset-30.f, ShootSoundMode::None}},
		mMidShooter{ new BulletShooter{owner, bulletDef,cooldownTime ,localPositionOffset,localRotationOffset, ShootSoundMode::None} },
		mRightShooter{ new BulletShooter{owner, bulletDef,cooldownTime ,localPositionOffset + sf::Vector2f{20.f,-5.f},localRotationOffset+30.f, ShootSoundMode::None}}
	{
		SetShootSoundProps("SpaceShooterRedux/Bonus/sfx_laser1.ogg", 30.0f, 0.9f);
		UpdateSoundInterval();
	}

	void ThreeWayShooter::IncrementLevel(int amt)
	{
		Shooter::IncrementLevel(amt);

		mLeftShooter->IncrementLevel(amt);
		mMidShooter->IncrementLevel(amt);
		mRightShooter->IncrementLevel(amt);
		UpdateSoundInterval();
	}

	void ThreeWayShooter::SetCurrentLevel(int level)
	{
		Shooter::SetCurrentLevel(level);
		
		mLeftShooter->SetCurrentLevel(level);
		mMidShooter->SetCurrentLevel(level);
		mRightShooter->SetCurrentLevel(level);

		UpdateSoundInterval();
	}

	void ThreeWayShooter::UpdateSoundInterval()
	{
		float effectiveCooldown = mMidShooter->GetCooldownTime() / GetCooldownMultiplier();
		SetMinSoundInterval(effectiveCooldown);
	}

	void ThreeWayShooter::ShootImp()
	{
		mLeftShooter->Shoot();
		mMidShooter->Shoot();
		mRightShooter->Shoot();

		if (GetOwner()->GetCollisionLayer() == CollisionLayer::Player)
		PlayShootSound();
	}
}