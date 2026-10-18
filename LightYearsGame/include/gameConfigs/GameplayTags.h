#pragma once
#include "framework/GameplayTag.h"

namespace vega
{
	// The game owns its gameplay tag constants now that the engine Core no longer
	// defines them. The strings match the former engine values exactly so saved
	// data, light lookups and content stay valid. Declared inside vega with the
	// same nested layout as before so existing unqualified `GameTags::...` call
	// sites resolve unchanged.
	namespace GameTags
	{
		namespace Ship
		{
			inline const GameplayTag Engine_Main  = { "Ship.Engine.Main" };
			inline const GameplayTag Engine_Left  = { "Ship.Engine.Left" };
			inline const GameplayTag Engine_Right = { "Ship.Engine.Right" };

			inline const GameplayTag Weapon_Main  = { "Ship.Weapon.Main" };
			inline const GameplayTag Shield_Front = { "Ship.Shield.Front" };
		}

		namespace Bullet
		{
			inline const GameplayTag Laser_Red  = { "Bullet.Laser.Red" };
			inline const GameplayTag Laser_Blue = { "Bullet.Laser.Blue" };
		}

		namespace FX
		{
			inline const GameplayTag Explosion_Small = { "FX.Explosion.Small" };
		}

		namespace Environment
		{
			inline const GameplayTag Planet = { "Environment.Planet" };
			inline const GameplayTag Planet_Large = { "Environment.Planet.Large" };
			inline const GameplayTag Planet_Small = { "Environment.Planet.Small" };
			inline const GameplayTag Asteroid_Large = { "Environment.Asteroid.Large" };
			inline const GameplayTag Asteroid_Small = { "Environment.Asteroid.Small" };
		}
	}
}
