#pragma once

#include "attributes/AttributeId.h"
#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::EchoProtocol
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Utility.EchoProtocol.Basic";
	};

	inline const ly::GameplayTag CategoryTag{
		ly::GameplayTags::Ability::Utility
	};
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::EchoProtocol
	};

	struct Attribute
	{
		inline static const sas::AttributeId PowerBase{
			"Ability.Utility.EchoProtocol.PowerBase"
		};
		inline static const sas::AttributeId PowerPerLevel{
			"Ability.Utility.EchoProtocol.PowerPerLevel"
		};
		inline static const sas::AttributeId AttackPowerScale{
			"Ability.Utility.EchoProtocol.AttackPowerScale"
		};
		inline static const sas::AttributeId MaxHealthScale{
			"Ability.Utility.EchoProtocol.MaxHealthScale"
		};
		inline static const sas::AttributeId EnergyMaxScale{
			"Ability.Utility.EchoProtocol.EnergyMaxScale"
		};
		inline static const sas::AttributeId AttackSpeedScale{
			"Ability.Utility.EchoProtocol.AttackSpeedScale"
		};
		inline static const sas::AttributeId LuckScale{
			"Ability.Utility.EchoProtocol.LuckScale"
		};
		inline static const sas::AttributeId MovementScale{
			"Ability.Utility.EchoProtocol.MovementScale"
		};
	};
}
