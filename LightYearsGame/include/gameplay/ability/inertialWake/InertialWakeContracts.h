#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::InertialWake
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.InertialWake.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityOffense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.InertialWake" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::InertialWake };

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::InertialWake::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::InertialWake::Started
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::InertialWake::Ended
		};
	};

	struct Effect
	{
		inline static constexpr char StunId[] = "Effect.Control.Stun.Basic";
	};

	// Ability-scoped attributes control the ship policy. They are resolved once
	// on activation because they describe this cast's movement contribution.
	struct Attribute
	{
		inline static const sas::AttributeId TopSpeedBonus{
			"Ability.Offense.InertialWake.TopSpeedBonus"
		};
		inline static const sas::AttributeId ThrustBonus{
			"Ability.Offense.InertialWake.ThrustBonus"
		};
		inline static const sas::AttributeId NormalizationDuration{
			"Ability.Offense.InertialWake.NormalizationDuration"
		};
	};

	struct Actor
	{
		struct Wake
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.InertialWake.Wake.Basic";
			inline static const sas::AttributeId Root{ "AbilityActor.InertialWake.Wake" };

			// These actor values define delivery, geometry and control. The actor
			// evaluates current velocity per hit, so no stale velocity snapshot is
			// stored in the behavior.
			inline static const sas::AttributeId SpeedDamageConversion{
				"AbilityActor.InertialWake.Wake.SpeedDamageConversion"
			};
			inline static const sas::AttributeId EnergyPowerReference{
				"AbilityActor.InertialWake.Wake.EnergyPowerReference"
			};
			inline static const sas::AttributeId EnergyPowerConversionPerPoint{
				"AbilityActor.InertialWake.Wake.EnergyPowerConversionPerPoint"
			};
			inline static const sas::AttributeId SameTargetHitCooldown{
				"AbilityActor.InertialWake.Wake.SameTargetHitCooldown"
			};
			inline static const sas::AttributeId MinimumSpeedRatio{
				"AbilityActor.InertialWake.Wake.MinimumSpeedRatio"
			};
			inline static const sas::AttributeId LengthPerEffectiveRatio{
				"AbilityActor.InertialWake.Wake.LengthPerEffectiveRatio"
			};
			inline static const sas::AttributeId WidthPerEffectiveRatio{
				"AbilityActor.InertialWake.Wake.WidthPerEffectiveRatio"
			};
			inline static const sas::AttributeId OpeningAngleDegrees{
				"AbilityActor.InertialWake.Wake.OpeningAngleDegrees"
			};
			inline static const sas::AttributeId DiminishingStartRatio{
				"AbilityActor.InertialWake.Wake.DiminishingStartRatio"
			};
			inline static const sas::AttributeId DiminishingExcessMultiplier{
				"AbilityActor.InertialWake.Wake.DiminishingExcessMultiplier"
			};
			inline static const sas::AttributeId BaseKnockbackSpeed{
				"AbilityActor.InertialWake.Wake.BaseKnockbackSpeed"
			};
			inline static const sas::AttributeId KnockbackPerEffectiveRatio{
				"AbilityActor.InertialWake.Wake.KnockbackPerEffectiveRatio"
			};
			inline static const sas::AttributeId BaseStunDuration{
				"AbilityActor.InertialWake.Wake.BaseStunDuration"
			};
			inline static const sas::AttributeId StunPerEffectiveRatio{
				"AbilityActor.InertialWake.Wake.StunPerEffectiveRatio"
			};
		};
	};
}
