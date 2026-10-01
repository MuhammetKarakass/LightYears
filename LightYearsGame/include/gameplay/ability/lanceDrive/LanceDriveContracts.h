#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/ability/LanceDriveTags.h"

namespace AbilityData::LanceDrive
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Movement.LanceDrive.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityMovement };
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
			"Ability.Movement.LanceDrive.BaseDamage"
		};
		inline static const sas::AttributeId SpeedDamageConversion{
			"Ability.Movement.LanceDrive.SpeedDamageConversion"
		};
		inline static const sas::AttributeId EnergyPowerConversionAmplifierPerPoint{
			"Ability.Movement.LanceDrive.EnergyPowerConversionAmplifierPerPoint"
		};
		inline static const sas::AttributeId TopSpeedBonus{
			"Ability.Movement.LanceDrive.TopSpeedBonus"
		};
		inline static const sas::AttributeId ThrustBonus{
			"Ability.Movement.LanceDrive.ThrustBonus"
		};
		inline static const sas::AttributeId TurnCapabilityMultiplier{
			"Ability.Movement.LanceDrive.TurnCapabilityMultiplier"
		};
		inline static const sas::AttributeId SameTargetHitCooldown{
			"Ability.Movement.LanceDrive.SameTargetHitCooldown"
		};
		inline static const sas::AttributeId Length{
			"Ability.Movement.LanceDrive.Length"
		};
		inline static const sas::AttributeId EdgeThickness{
			"Ability.Movement.LanceDrive.EdgeThickness"
		};
		inline static const sas::AttributeId OpeningAngleDegrees{
			"Ability.Movement.LanceDrive.OpeningAngleDegrees"
		};
		inline static const sas::AttributeId LateralKnockback{
			"Ability.Movement.LanceDrive.LateralKnockback"
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
