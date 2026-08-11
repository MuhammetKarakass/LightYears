#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::NullPulse
{
	struct AbilityId
	{
		// Ability IDs must include the category, family and variant segments.
		inline static constexpr char Basic[] = "Ability.Control.NullPulse.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Control };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::NullPulse };

	struct Attribute
	{
		// Radius and damage are shared numeric concepts. They are aliases here so
		// the behavior remains readable without creating duplicate AttributeIds.
		inline static const sas::AttributeId Radius = ly::CommonAttributeIds::Radius;
		inline static const sas::AttributeId Damage = ly::CommonAttributeIds::Damage;

		// Stun tuning is specific to Null Pulse's control formula and therefore
		// belongs to the ability-scoped attribute list in abilities.json.
		inline static const sas::AttributeId BaseStunDuration{
			"Ability.Control.NullPulse.BaseStunDuration"
		};
		inline static const sas::AttributeId MaxBonusStun{
			"Ability.Control.NullPulse.MaxBonusStun"
		};
		inline static const sas::AttributeId ReferenceEnergyMax{
			"Ability.Control.NullPulse.ReferenceEnergyMax"
		};
		inline static const sas::AttributeId EnergyScale{
			"Ability.Control.NullPulse.EnergyScale"
		};
		inline static const sas::AttributeId BossStaggerDuration{
			"Ability.Control.NullPulse.BossStaggerDuration"
		};
	};

	struct Effect
	{
		inline static constexpr char StunId[] = "Effect.Control.Stun.Basic";
		inline static constexpr char StaggerId[] = "Effect.Control.Stagger.Basic";
	};

	struct Event
	{
		inline static const ly::GameplayTag Activated =
			ly::GameplayTags::Event::Ability::NullPulse::Activated;
		inline static const ly::GameplayTag ProjectilesCleared =
			ly::GameplayTags::Event::Ability::NullPulse::ProjectilesCleared;
		inline static const ly::GameplayTag Completed =
			ly::GameplayTags::Event::Ability::NullPulse::Completed;
	};
}
