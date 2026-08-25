#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

#include <array>

namespace AbilityData::GlacialPressure
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Offense.GlacialPressure.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.GlacialPressure" };
	inline const ly::GameplayTag FamilyTag =
		ly::GameplayTags::Ability::Family::GlacialPressure;

	struct State
	{
		inline static const ly::GameplayTag Focusing =
			ly::GameplayTags::State::Ability::GlacialPressure::Focusing;
		inline static const ly::GameplayTag Pushing =
			ly::GameplayTags::State::Ability::GlacialPressure::Pushing;
	};

	struct Event
	{
		inline static const ly::GameplayTag Started =
			ly::GameplayTags::Event::Ability::GlacialPressure::Started;
		inline static const ly::GameplayTag Blasted =
			ly::GameplayTags::Event::Ability::GlacialPressure::Blasted;
		inline static const ly::GameplayTag Collision =
			ly::GameplayTags::Event::Ability::GlacialPressure::Collision;
		inline static const ly::GameplayTag Ended =
			ly::GameplayTags::Event::Ability::GlacialPressure::Ended;
	};

	struct Attribute
	{
		// Range and the damage type are shared concepts. The ability owns only
		// its unique balance values and aliases the common range attribute.
		inline static const sas::AttributeId Range = ly::CommonAttributeIds::Range;
		inline static const sas::AttributeId InitialDamage{
			"Ability.Offense.GlacialPressure.InitialDamage"
		};
		inline static const sas::AttributeId CollisionDamage{
			"Ability.Offense.GlacialPressure.CollisionDamage"
		};
		inline static const sas::AttributeId EnergyMaxInitialScale{
			"Ability.Offense.GlacialPressure.EnergyMaxInitialScale"
		};
		inline static const sas::AttributeId EnergyMaxCollisionScale{
			"Ability.Offense.GlacialPressure.EnergyMaxCollisionScale"
		};
		inline static const sas::AttributeId MaxHealthReference{
			"Ability.Offense.GlacialPressure.MaxHealthReference"
		};
		inline static const sas::AttributeId MaxHealthPushScale{
			"Ability.Offense.GlacialPressure.MaxHealthPushScale"
		};
		inline static const sas::AttributeId PushDistance{
			"Ability.Offense.GlacialPressure.PushDistance"
		};
		inline static const sas::AttributeId PushDuration{
			"Ability.Offense.GlacialPressure.PushDuration"
		};
		inline static const sas::AttributeId ConeHalfAngleDegrees{
			"Ability.Offense.GlacialPressure.ConeHalfAngleDegrees"
		};
		inline static const sas::AttributeId SegmentCount{
			"Ability.Offense.GlacialPressure.SegmentCount"
		};
		inline static const sas::AttributeId SegmentOneExtraStun{
			"Ability.Offense.GlacialPressure.SegmentOneExtraStun"
		};
		// A target is briefly stunned while the initial impulse is applied. A
		// later ship collision is intentionally stronger and owns its own timer.
		inline static const sas::AttributeId PushStunDuration{
			"Ability.Offense.GlacialPressure.PushStunDuration"
		};
		inline static const sas::AttributeId CollisionStunDuration{
			"Ability.Offense.GlacialPressure.CollisionStunDuration"
		};
	};

	struct Effect
	{
		// Glacial Pressure reuses the project-wide Stun effect. No ability-local
		// duplicate is created for a control effect that already exists globally.
		inline static constexpr char StunId[] = "Effect.Control.Stun.Basic";
	};

	struct SegmentProfile
	{
		float damageMultiplier;
		// Collision damage has its own falloff. It must not reuse pushMultiplier:
		// designers can tune distance travelled without unintentionally changing
		// the damage dealt when a pushed ship hits another ship.
		float collisionDamageMultiplier;
		float pushMultiplier;
		int cryoStacks;
		bool appliesExtraStun;
	};

	inline constexpr std::array<SegmentProfile, 5> SegmentProfiles{
		SegmentProfile{ 1.00f, 1.0000f, 1.00f, 4, true },
		SegmentProfile{ 0.80f, 0.9000f, 0.85f, 4, false },
		SegmentProfile{ 0.60f, 0.8000f, 0.70f, 3, false },
		SegmentProfile{ 0.40f, 0.7333f, 0.55f, 2, false },
		SegmentProfile{ 0.25f, 0.6667f, 0.40f, 1, false }
	};

	inline constexpr float DefaultConeLength = 700.f;
	inline constexpr float DefaultConeHalfAngleDegrees = 22.f;
	inline constexpr float DefaultPushDistance = 800.f;
	inline constexpr float DefaultImpulseWindowDuration = 1.5f;
	inline constexpr float DefaultPushStunDuration = 1.5f;
	inline constexpr float DefaultCollisionStunDuration = 2.f;
	// A collision caused immediately after the impulse is full strength. As the
	// target slows, impact damage approaches this lower bound smoothly.
	inline constexpr float MinimumCollisionDamageMultiplier = 0.25f;
	inline constexpr float MaximumCollisionDamageMultiplier = 1.f;
	inline constexpr int DefaultSegmentCount = 5;
}
