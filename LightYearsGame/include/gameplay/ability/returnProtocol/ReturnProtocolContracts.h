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

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Defense };
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::ReturnProtocol
	};

	struct State
	{
		inline static const vega::GameplayTag Active{
			vega::GameplayTags::State::Ability::ReturnProtocol::Active
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::ReturnProtocol::Started
		};
		inline static const vega::GameplayTag Reflected{
			vega::GameplayTags::Event::Ability::ReturnProtocol::Reflected
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::ReturnProtocol::Ended
		};
	};

	struct Attribute
	{
		inline static const sas::AttributeId BaseReflectDamageMultiplier{
			"Ability.Defense.ReturnProtocol.BaseReflectDamageMultiplier"
		};
		inline static const sas::AttributeId AttackPowerReference{
			"Ability.Defense.ReturnProtocol.AttackPowerReference"
		};
		inline static const sas::AttributeId AttackPowerScale{
			"Ability.Defense.ReturnProtocol.AttackPowerScale"
		};
		inline static const sas::AttributeId EnergyPowerReference{
			"Ability.Defense.ReturnProtocol.EnergyPowerReference"
		};
		inline static const sas::AttributeId EnergyPowerScale{
			"Ability.Defense.ReturnProtocol.EnergyPowerScale"
		};
	};
}
