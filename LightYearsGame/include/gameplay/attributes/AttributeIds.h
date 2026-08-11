#pragma once

#include "attributes/AttributeId.h"

namespace ly
{
	struct OwnerAttributeIds
	{
		inline static const sas::AttributeId MaxHealth{ "Owner.MaxHealth" };
		inline static const sas::AttributeId HealthRegen{ "Owner.HealthRegen" };
		inline static const sas::AttributeId EnergyMax{ "Owner.Energy.Max" };
		inline static const sas::AttributeId EnergyRegen{ "Owner.Energy.Regen" };
		inline static const sas::AttributeId AttackPower{ "Owner.AttackPower" };
		inline static const sas::AttributeId AttackSpeed{ "Owner.AttackSpeed" };
		inline static const sas::AttributeId AbilityHaste{ "Owner.AbilityHaste" };
		inline static const sas::AttributeId MoveSpeedHorizontal{ "Owner.MoveSpeedHorizontal" };
		inline static const sas::AttributeId MoveSpeedVertical{ "Owner.MoveSpeedVertical" };
		inline static const sas::AttributeId MovementSlow{ "Owner.MovementSlow" };
		inline static const sas::AttributeId Armor{ "Owner.Armor" };
		inline static const sas::AttributeId Luck{ "Owner.Luck" };
		inline static const sas::AttributeId CriticalChance{ "Owner.CriticalChance" };
	};

	struct ShipAttributeIds
	{
		inline static const sas::AttributeId MaxShield{ "Ship.Shield.Max" };
		inline static const sas::AttributeId ShieldRegen{ "Ship.Shield.Regen" };
		inline static const sas::AttributeId ShieldRechargeDelay{ "Ship.Shield.RechargeDelay" };
		inline static const sas::AttributeId AfterburnerCapacity{ "Ship.Afterburner.Capacity" };
		inline static const sas::AttributeId AfterburnerRegen{ "Ship.Afterburner.Regen" };
		inline static const sas::AttributeId AfterburnerRechargeDelay{ "Ship.Afterburner.RechargeDelay" };
		inline static const sas::AttributeId AfterburnerEnergyDrainPerSecond{ "Ship.Afterburner.EnergyDrainPerSecond" };
		inline static const sas::AttributeId AfterburnerSpeedMultiplier{ "Ship.Afterburner.SpeedMultiplier" };
		inline static const sas::AttributeId AfterburnerAccelerationMultiplier{ "Ship.Afterburner.AccelerationMultiplier" };
		inline static const sas::AttributeId AfterburnerRampUpDuration{ "Ship.Afterburner.RampUpDuration" };
		inline static const sas::AttributeId AfterburnerRampDownDuration{ "Ship.Afterburner.RampDownDuration" };
		inline static const sas::AttributeId AfterburnerManeuverabilityMultiplier{ "Ship.Afterburner.ManeuverabilityMultiplier" };
	};

	struct CommonAttributeIds
	{
		inline static const sas::AttributeId Cooldown{ "Common.Cooldown" };
		inline static const sas::AttributeId Damage{ "Common.Damage" };
		inline static const sas::AttributeId Radius{ "Common.Radius" };
		inline static const sas::AttributeId Duration{ "Common.Duration" };
		inline static const sas::AttributeId Interval{ "Common.Interval" };
		inline static const sas::AttributeId FireRate{ "Common.FireRate" };
		inline static const sas::AttributeId Range{ "Common.Range" };
		inline static const sas::AttributeId ProjectileCount{ "Common.ProjectileCount" };
	};

	struct CollisionAttributeIds
	{
		inline static const sas::AttributeId Radius{ "Collision.Radius" };
	};

	struct AreaAttributeIds
	{
		inline static const sas::AttributeId Radius{ "Area.Radius" };
	};
}
