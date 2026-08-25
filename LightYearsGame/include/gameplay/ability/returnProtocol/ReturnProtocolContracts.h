#pragma once

#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ReturnProtocol
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.ReturnProtocol.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.ReturnProtocol" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::ReturnProtocol
	};

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::ReturnProtocol::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::ReturnProtocol::Started
		};
		inline static const ly::GameplayTag Reflected{
			ly::GameplayTags::Event::Ability::ReturnProtocol::Reflected
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::ReturnProtocol::Ended
		};
	};

	struct Attribute
	{
		// MaxHealth remains an owner attribute. These values only describe how
		// this ability converts that owner stat into reflected damage.
		inline static const sas::AttributeId BaseReflectDamageMultiplier{
			"Ability.Defense.ReturnProtocol.BaseReflectDamageMultiplier"
		};
		inline static const sas::AttributeId MaxHealthReference{
			"Ability.Defense.ReturnProtocol.MaxHealthReference"
		};
		inline static const sas::AttributeId MaxHealthDamageScale{
			"Ability.Defense.ReturnProtocol.MaxHealthDamageScale"
		};
	};
}
