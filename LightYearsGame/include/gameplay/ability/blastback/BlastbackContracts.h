#pragma once

#include "framework/Core.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Blastback
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Offense.Blastback.Basic";
	};

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Offense;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.Blastback" };
	inline const ly::GameplayTag FamilyTag = ly::GameplayTags::Ability::Family::Blastback;

	struct State
	{
		inline static const ly::GameplayTag Focusing =
			ly::GameplayTags::State::Ability::Blastback::Focusing;
	};

	struct Event
	{
		inline static const ly::GameplayTag Started =
			ly::GameplayTags::Event::Ability::Blastback::Started;
		inline static const ly::GameplayTag Blasted =
			ly::GameplayTags::Event::Ability::Blastback::Blasted;
		inline static const ly::GameplayTag Ended =
			ly::GameplayTags::Event::Ability::Blastback::Ended;
	};

	struct Attribute
	{
		// Damage and range are cross-family concepts. Everything else below
		// describes Blastback's two-zone cone and its movement contract.
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;
		inline static const sas::AttributeId Range = ly::CommonAttributeIds::Range;
		inline static const sas::AttributeId InnerRange{
			"Ability.Offense.Blastback.InnerRange"
		};
		inline static const sas::AttributeId ConeHalfAngleDegrees{
			"Ability.Offense.Blastback.ConeHalfAngleDegrees"
		};
		inline static const sas::AttributeId InnerDamageMultiplier{
			"Ability.Offense.Blastback.InnerDamageMultiplier"
		};
		inline static const sas::AttributeId InnerIgniteStacks{
			"Ability.Offense.Blastback.InnerIgniteStacks"
		};
		inline static const sas::AttributeId OuterIgniteStacks{
			"Ability.Offense.Blastback.OuterIgniteStacks"
		};
		inline static const sas::AttributeId InnerStunDuration{
			"Ability.Offense.Blastback.InnerStunDuration"
		};
		inline static const sas::AttributeId OuterStunDuration{
			"Ability.Offense.Blastback.OuterStunDuration"
		};
		inline static const sas::AttributeId MaxHealthReference{
			"Ability.Offense.Blastback.MaxHealthReference"
		};
		inline static const sas::AttributeId MaxHealthStunScale{
			"Ability.Offense.Blastback.MaxHealthStunScale"
		};
		inline static const sas::AttributeId MinimumPushInitialSpeed{
			"Ability.Offense.Blastback.MinimumPushInitialSpeed"
		};
		inline static const sas::AttributeId MaximumPushInitialSpeed{
			"Ability.Offense.Blastback.MaximumPushInitialSpeed"
		};
		inline static const sas::AttributeId InnerPushMultiplier{
			"Ability.Offense.Blastback.InnerPushMultiplier"
		};
		inline static const sas::AttributeId RecoilInitialSpeed{
			"Ability.Offense.Blastback.RecoilInitialSpeed"
		};
		inline static const sas::AttributeId RecoilDuration{
			"Ability.Offense.Blastback.RecoilDuration"
		};
		// Thermal status behavior is owned by the ability because direct ability
		// attributes may not declare generic Damage.* override IDs. The payload is
		// built explicitly at the one-shot hit boundary.
		inline static const sas::AttributeId BurnDamagePerSecond{
			"Ability.Offense.Blastback.BurnDamagePerSecond"
		};
		inline static const sas::AttributeId BurnDuration{
			"Ability.Offense.Blastback.BurnDuration"
		};
		inline static const sas::AttributeId BurnMaxStacks{
			"Ability.Offense.Blastback.BurnMaxStacks"
		};
	};

	struct Effect
	{
		inline static constexpr char StunId[] = "Effect.Control.Stun.Basic";
	};

	inline constexpr float DefaultRange = 400.f;
	inline constexpr float DefaultInnerRange = 200.f;
	inline constexpr float DefaultConeHalfAngleDegrees = 32.f;
	inline constexpr float DefaultRecoilDuration = 0.5f;
}
