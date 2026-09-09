#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/ability/LanceDriveTags.h"

namespace AbilityData::LanceDrive
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.LanceDrive.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityOffense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.LanceDrive" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::LanceDrive };

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::LanceDrive::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::LanceDrive::Started
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::LanceDrive::Ended
		};
	};

	struct Attribute
	{
		inline static const sas::AttributeId BaseDamage{
			"Ability.Offense.LanceDrive.BaseDamage"
		};
		inline static const sas::AttributeId SpeedDamageConversion{
			"Ability.Offense.LanceDrive.SpeedDamageConversion"
		};
		inline static const sas::AttributeId EnergyMaxReference{
			"Ability.Offense.LanceDrive.EnergyMaxReference"
		};
		inline static const sas::AttributeId EnergyMaxConversionPerPoint{
			"Ability.Offense.LanceDrive.EnergyMaxConversionPerPoint"
		};
		inline static const sas::AttributeId TopSpeedBonus{
			"Ability.Offense.LanceDrive.TopSpeedBonus"
		};
		inline static const sas::AttributeId ThrustBonus{
			"Ability.Offense.LanceDrive.ThrustBonus"
		};
		inline static const sas::AttributeId TurnCapabilityMultiplier{
			"Ability.Offense.LanceDrive.TurnCapabilityMultiplier"
		};
		inline static const sas::AttributeId SameTargetHitCooldown{
			"Ability.Offense.LanceDrive.SameTargetHitCooldown"
		};
		inline static const sas::AttributeId Length{
			"Ability.Offense.LanceDrive.Length"
		};
		inline static const sas::AttributeId EdgeThickness{
			"Ability.Offense.LanceDrive.EdgeThickness"
		};
		inline static const sas::AttributeId OpeningAngleDegrees{
			"Ability.Offense.LanceDrive.OpeningAngleDegrees"
		};
		inline static const sas::AttributeId LateralKnockback{
			"Ability.Offense.LanceDrive.LateralKnockback"
		};
	};

	struct Actor
	{
		struct Lance
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.LanceDrive.Lance.Basic";
		};
	};
}
