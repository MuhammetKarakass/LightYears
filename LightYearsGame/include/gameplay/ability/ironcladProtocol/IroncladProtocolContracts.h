#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::IroncladProtocol
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.IroncladProtocol.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.IroncladProtocol" };
	inline const ly::GameplayTag FamilyTag = ly::GameplayTags::Ability::Family::IroncladProtocol;

	struct State
	{
		inline static const ly::GameplayTag Active = ly::GameplayTags::State::Ability::IroncladProtocol::Active;
		inline static const ly::GameplayTag CancelAvailable = ly::GameplayTags::State::Ability::IroncladProtocol::CancelAvailable;
	};

	struct Event
	{
		inline static const ly::GameplayTag Started = ly::GameplayTags::Event::Ability::IroncladProtocol::Started;
		inline static const ly::GameplayTag CancelAvailable = ly::GameplayTags::Event::Ability::IroncladProtocol::CancelAvailable;
		inline static const ly::GameplayTag Ended = ly::GameplayTags::Event::Ability::IroncladProtocol::Ended;
	};

	struct Attribute
	{
		inline static const sas::AttributeId MovementSpeedMultiplier{
			"Ability.Defense.IroncladProtocol.MovementSpeedMultiplier"
		};
		inline static const sas::AttributeId MinimumFormDuration{
			"Ability.Defense.IroncladProtocol.MinimumFormDuration"
		};
		inline static const sas::AttributeId MinigunBaseDamage{
			"Ability.Defense.IroncladProtocol.MinigunBaseDamage"
		};
		inline static const sas::AttributeId BaseDamageReduction{
			"Ability.Defense.IroncladProtocol.BaseDamageReduction"
		};
		inline static const sas::AttributeId MaxHealthReference{
			"Ability.Defense.IroncladProtocol.MaxHealthReference"
		};
		inline static const sas::AttributeId MaximumDamageReductionBonus{
			"Ability.Defense.IroncladProtocol.MaximumDamageReductionBonus"
		};
		inline static const sas::AttributeId DamageReductionFalloffHealth{
			"Ability.Defense.IroncladProtocol.DamageReductionFalloffHealth"
		};
	};

	struct Effect
	{
		inline static constexpr char DamageReductionId[] =
			"Effect.Defense.IroncladProtocol.DamageReduction.Basic";
	};

	struct Weapon
	{
		inline static constexpr char MinigunId[] =
			"Weapon.Projectile.IroncladMinigun.Basic";
	};
}
