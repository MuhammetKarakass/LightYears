#pragma once

#include "presentation/hud/weaponChoice/WeaponChoiceViewModel.h"

#include <cstddef>
#include <memory>
#include <string>

namespace vega
{
	// Reads the player's pending Second Hardpoint choices and turns a picked option into Player::ChooseSecondaryWeapon.
	class WeaponChoicePresenter
	{
	public:
		WeaponChoicePresenter();
		void Tick(float deltaTime);
		// Picks the option at index for the module currently asking; false when nothing is open or the index is invalid.
		bool Choose(std::size_t index);
		std::shared_ptr<const WeaponChoiceViewModel> GetViewModel() const { return mViewModel; }

		// "Weapon.Projectile.FighterRapidLaser.Basic" -> "Fighter Rapid Laser".
		static std::string MakeWeaponLabel(const std::string& weaponId);

	private:
		std::shared_ptr<WeaponChoiceViewModel> mViewModel;
	};
}
