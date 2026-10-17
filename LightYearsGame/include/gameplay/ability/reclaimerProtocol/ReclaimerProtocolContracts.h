#pragma once

#include "framework/Core.h"
#include "gameplay/ability/content/NumericSettingContract.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ReclaimerProtocol
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Defense.ReclaimerProtocol.Basic";
	};

	inline const vega::GameplayTag CategoryTag = vega::GameplayTags::Ability::Defense;
	inline const vega::GameplayTag FamilyTag =
		vega::GameplayTags::Ability::Family::ReclaimerProtocol;

	struct State
	{
		inline static const vega::GameplayTag Active =
			vega::GameplayTags::State::Ability::ReclaimerProtocol::Active;
	};

	struct Event
	{
		inline static const vega::GameplayTag Started =
			vega::GameplayTags::Event::Ability::ReclaimerProtocol::Started;
		inline static const vega::GameplayTag RepairKitSpawned =
			vega::GameplayTags::Event::Ability::ReclaimerProtocol::RepairKitSpawned;
		inline static const vega::GameplayTag Ended =
			vega::GameplayTags::Event::Ability::ReclaimerProtocol::Ended;
	};

	struct Attribute
	{
		inline static const sas::AttributeId HealRatio{
			"Ability.Defense.ReclaimerProtocol.HealRatio"
		};
	};

	struct Setting
	{
		inline static constexpr char MaxHealthReference[] = "maxHealthReference";
		inline static constexpr char MaxHealthPerStep[] = "maxHealthPerStep";
		inline static constexpr char HealRatioPerStep[] = "healRatioPerStep";
		inline static const vega::content::NumericSettingContract Contract{
			{ MaxHealthReference, MaxHealthPerStep, HealRatioPerStep },
			{ MaxHealthReference, MaxHealthPerStep, HealRatioPerStep }
		};
	};

	struct Actor
	{
		struct RepairKit
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.ReclaimerProtocol.RepairKit.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.ReclaimerProtocol.RepairKit"
			};
			inline static const sas::AttributeId HealRatio{
				"AbilityActor.ReclaimerProtocol.RepairKit.HealRatio"
			};
		};
	};

	inline constexpr float DefaultDuration = 6.f;
	inline constexpr float DefaultCooldown = 16.f;
	inline constexpr float DefaultHealRatio = 0.08f;
	inline constexpr float DefaultKitLifetime = 10.f;
	inline constexpr float DefaultKitCollisionRadius = 16.f;
}
