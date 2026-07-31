#pragma once

#include "framework/Core.h"

namespace ly
{
	struct OwnerAttributeIds
	{
		inline static const GameplayTag MaxHealth{ "Attribute.Owner.MaxHealth" };
		inline static const GameplayTag HealthRegen{ "Attribute.Owner.HealthRegen" };
		inline static const GameplayTag EnergyMax{ "Attribute.Owner.Energy.Max" };
		inline static const GameplayTag EnergyRegen{ "Attribute.Owner.Energy.Regen" };
		inline static const GameplayTag AttackPower{ "Attribute.Owner.AttackPower" };
		inline static const GameplayTag AttackSpeed{ "Attribute.Owner.AttackSpeed" };
		inline static const GameplayTag AbilityHaste{ "Attribute.Owner.AbilityHaste" };
		inline static const GameplayTag MoveSpeedHorizontal{ "Attribute.Owner.MoveSpeedHorizontal" };
		inline static const GameplayTag MoveSpeedVertical{ "Attribute.Owner.MoveSpeedVertical" };
		inline static const GameplayTag MovementSlow{ "Attribute.Owner.MovementSlow" };
		inline static const GameplayTag Armor{ "Attribute.Owner.Armor" };
		inline static const GameplayTag Luck{ "Attribute.Owner.Luck" };
		inline static const GameplayTag CriticalChance{ "Attribute.Owner.CriticalChance" };
	};

	struct ShipAttributeIds
	{
		inline static const GameplayTag MaxShield{ "Attribute.Ship.Shield.Max" };
		inline static const GameplayTag ShieldRegen{ "Attribute.Ship.Shield.Regen" };
		inline static const GameplayTag ShieldRechargeDelay{ "Attribute.Ship.Shield.RechargeDelay" };
		inline static const GameplayTag AfterburnerCapacity{ "Attribute.Ship.Afterburner.Capacity" };
		inline static const GameplayTag AfterburnerRegen{ "Attribute.Ship.Afterburner.Regen" };
		inline static const GameplayTag AfterburnerRechargeDelay{ "Attribute.Ship.Afterburner.RechargeDelay" };
		inline static const GameplayTag AfterburnerEnergyDrainPerSecond{ "Attribute.Ship.Afterburner.EnergyDrainPerSecond" };
		inline static const GameplayTag AfterburnerSpeedMultiplier{ "Attribute.Ship.Afterburner.SpeedMultiplier" };
		inline static const GameplayTag AfterburnerAccelerationMultiplier{ "Attribute.Ship.Afterburner.AccelerationMultiplier" };
		inline static const GameplayTag AfterburnerRampUpDuration{ "Attribute.Ship.Afterburner.RampUpDuration" };
		inline static const GameplayTag AfterburnerRampDownDuration{ "Attribute.Ship.Afterburner.RampDownDuration" };
		inline static const GameplayTag AfterburnerManeuverabilityMultiplier{ "Attribute.Ship.Afterburner.ManeuverabilityMultiplier" };
	};

	struct CommonAttributeIds
	{
		inline static const GameplayTag Cooldown{ "Attribute.Common.Cooldown" };
		inline static const GameplayTag Damage{ "Attribute.Common.Damage" };
		inline static const GameplayTag Radius{ "Attribute.Common.Radius" };
		inline static const GameplayTag Duration{ "Attribute.Common.Duration" };
		inline static const GameplayTag Interval{ "Attribute.Common.Interval" };
		inline static const GameplayTag FireRate{ "Attribute.Common.FireRate" };
		inline static const GameplayTag Range{ "Attribute.Common.Range" };
		inline static const GameplayTag CollisionRadius{ "Attribute.Collision.Radius" };
		inline static const GameplayTag AreaRadius{ "Attribute.Area.Radius" };
	};
}
