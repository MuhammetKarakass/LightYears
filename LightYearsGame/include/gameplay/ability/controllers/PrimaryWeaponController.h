#pragma once

#include "gameConfigs/GameplayStructs.h"
#include "gameplay/ability/AbilityController.h"

namespace ly
{
	class PrimaryWeaponController : public AbilityController
	{
	public:
		PrimaryWeaponController(Actor* owner, const PrimaryWeaponDefinition& weaponDefinition);

		const PrimaryWeaponDefinition& GetWeaponDefinition() const { return mWeaponDefinition; }
		PrimaryWeaponAttributes& GetAttributes() { return mRuntimeAttributes; }
		const PrimaryWeaponAttributes& GetAttributes() const { return mRuntimeAttributes; }

	protected:
		virtual bool CanActivate() const override;
		virtual void OnActivate() override;
		virtual void OnTickActive(float deltaTime) override;
		virtual void OnEnd() override;

	private:
		void TryFire();
		void FireShot();
		void FireProjectile(const WeaponMuzzleDefinition& muzzleDefinition, float localRotationOffset);
		float GetFireInterval() const;
		int GetProjectilesPerShot() const;
		static AbilityDefinition MakeAbilityDefinition(const PrimaryWeaponDefinition& weaponDefinition);

		PrimaryWeaponDefinition mWeaponDefinition;
		PrimaryWeaponAttributes mRuntimeAttributes;
		float mTimeUntilNextShot;
	};
}
