#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ShieldHarvest
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.ShieldHarvest.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.ShieldHarvest" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::ShieldHarvest };

	struct State
	{
		inline static const ly::GameplayTag Focusing{
			ly::GameplayTags::State::Ability::ShieldHarvest::Focusing
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::ShieldHarvest::Started
		};
		inline static const ly::GameplayTag Harvested{
			ly::GameplayTags::Event::Ability::ShieldHarvest::Harvested
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::ShieldHarvest::Ended
		};
	};

	struct Attribute
	{
		// Radius is a common spatial concept; the ability only aliases it.
		inline static const sas::AttributeId Radius = ly::CommonAttributeIds::Radius;
		inline static const sas::AttributeId ShieldPerEnemy{
			"Ability.Defense.ShieldHarvest.ShieldPerEnemy"
		};
		inline static const sas::AttributeId OvershieldHoldDuration{
			"Ability.Defense.ShieldHarvest.OvershieldHoldDuration"
		};
		inline static const sas::AttributeId OvershieldDecayPerSecond{
			"Ability.Defense.ShieldHarvest.OvershieldDecayPerSecond"
		};
	};
}
