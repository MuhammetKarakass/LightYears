#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Cryostasis
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.Cryostasis.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::Cryostasis };

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::Cryostasis::Active
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::Cryostasis::Started
		};
		inline static const ly::GameplayTag IceBroken{
			ly::GameplayTags::Event::Ability::Cryostasis::IceBroken
		};
		inline static const ly::GameplayTag ManuallyEnded{
			ly::GameplayTags::Event::Ability::Cryostasis::ManuallyEnded
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::Cryostasis::Ended
		};
	};

	struct Effect
	{
		inline static constexpr char IceShellId[] =
			"Effect.Defense.Cryostasis.IceShell.Basic";
		inline static const sas::AttributeId IceHealth{
			"Effect.Cryostasis.IceHealth"
		};
	};

	struct Attribute
	{
		inline static const sas::AttributeId BaseIceHealth{
			"Ability.Defense.Cryostasis.BaseIceHealth"
		};
		inline static const sas::AttributeId IceHealthMaxHealthScale{
			"Ability.Defense.Cryostasis.IceHealthMaxHealthScale"
		};
		inline static const sas::AttributeId BaseHealthRegenPerSecond{
			"Ability.Defense.Cryostasis.BaseHealthRegenPerSecond"
		};
		inline static const sas::AttributeId HealthRegenMaxHealthScale{
			"Ability.Defense.Cryostasis.HealthRegenMaxHealthScale"
		};
		inline static const sas::AttributeId BaseAfterburnerRecoveryPerSecond{
			"Ability.Defense.Cryostasis.BaseAfterburnerRecoveryPerSecond"
		};
		inline static const sas::AttributeId AfterburnerRecoveryEnergyPowerScale{
			"Ability.Defense.Cryostasis.AfterburnerRecoveryEnergyPowerScale"
		};
		inline static const sas::AttributeId EnergyPowerReference{
			"Ability.Defense.Cryostasis.EnergyPowerReference"
		};
	};
}
