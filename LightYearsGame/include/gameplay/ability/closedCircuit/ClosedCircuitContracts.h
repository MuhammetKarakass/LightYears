#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/content/NumericSettingContract.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::ClosedCircuit
{
	struct AbilityId { inline static constexpr char Basic[] = "Ability.Defense.ClosedCircuit.Basic"; };
	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.ClosedCircuit" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::ClosedCircuit };
	struct State { inline static const ly::GameplayTag Deploying{ ly::GameplayTags::State::Ability::ClosedCircuit::Deploying }; };
	struct Event
	{
		inline static const ly::GameplayTag Started{ ly::GameplayTags::Event::Ability::ClosedCircuit::Started };
		inline static const ly::GameplayTag Deployed{ ly::GameplayTags::Event::Ability::ClosedCircuit::Deployed };
		inline static const ly::GameplayTag Ended{ ly::GameplayTags::Event::Ability::ClosedCircuit::Ended };
	};
	struct Attribute
	{
		inline static const sas::AttributeId BaseBarrierHealth{ "Ability.Defense.ClosedCircuit.BaseBarrierHealth" };
		inline static const sas::AttributeId EnergyMaxBarrierHealthScale{ "Ability.Defense.ClosedCircuit.EnergyMaxBarrierHealthScale" };
	};
	struct Setting
	{
		inline static constexpr char MaximumDeliveryRange[] = "maximumDeliveryRange";
		inline static constexpr char DeliverySpeed[] = "deliverySpeed";
		inline static constexpr char FormationDuration[] = "formationDuration";
		inline static constexpr char BarrierRadius[] = "barrierRadius";
		inline static const ly::content::NumericSettingContract Contract{
			{ MaximumDeliveryRange, DeliverySpeed, FormationDuration, BarrierRadius },
			{ MaximumDeliveryRange, DeliverySpeed, FormationDuration, BarrierRadius }
		};
	};
	struct Actor { struct Delivery { inline static constexpr char BasicDefinitionId[] = "Actor.Ability.ClosedCircuit.Delivery.Basic"; }; };
}
