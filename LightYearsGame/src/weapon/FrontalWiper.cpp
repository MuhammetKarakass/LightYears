#include "weapon/FrontalWiper.h"

namespace ly
{
	FrontalWiper::FrontalWiper(Actor* owner,
		const BulletDefinition& bulletDef,
		float cooldownTime,
		const sf::Vector2f& localPositionOffset,
		float localRotationOffset,
		float width)
		: Shooter{ owner, ShootSoundMode::Composite },
		mWidth{ width },
		mShooter1{new BulletShooter {owner, bulletDef, cooldownTime, {localPositionOffset.x+width, localPositionOffset.y-15.f}, 4.5f, ShootSoundMode::None}},
		mShooter2{new BulletShooter {owner, bulletDef, cooldownTime, {localPositionOffset.x+width/1.5f, localPositionOffset.y-5.f}, 3.f, ShootSoundMode::None}},
		mShooter3{new BulletShooter {owner, bulletDef, cooldownTime, {localPositionOffset.x-width/1.5f, localPositionOffset.y-5.f}, -3.f, ShootSoundMode::None}},
		mShooter4{new BulletShooter {owner, bulletDef, cooldownTime, {localPositionOffset.x-width, localPositionOffset.y-15.f}, -4.5f, ShootSoundMode::None}}
	{
		SetShootSoundProps("SpaceShooterRedux/Bonus/sfx_laser1.ogg", 40.0f, 0.8f);
		UpdateSoundInterval();
	}

	void FrontalWiper::IncrementLevel(int amt)
	{
		Shooter::IncrementLevel(amt);

		mShooter1->IncrementLevel(amt);
		mShooter2->IncrementLevel(amt);
		mShooter3->IncrementLevel(amt);
		mShooter4->IncrementLevel(amt);

		UpdateSoundInterval();
	}

	void FrontalWiper::SetCurrentLevel(int level)
	{
		Shooter::SetCurrentLevel(level);

		mShooter1->SetCurrentLevel(level);
		mShooter2->SetCurrentLevel(level);
		mShooter3->SetCurrentLevel(level);
		mShooter4->SetCurrentLevel(level);

		UpdateSoundInterval();
	}

	void FrontalWiper::UpdateSoundInterval()
	{
		float effectiveCooldown = mShooter1->GetCooldownTime() / GetCooldownMultiplier();
		SetMinSoundInterval(effectiveCooldown);
	}

	void FrontalWiper::ShootImp()
	{
		mShooter1->Shoot();
		mShooter2->Shoot();
		mShooter3->Shoot();
		mShooter4->Shoot();

		PlayShootSound();
	}
}