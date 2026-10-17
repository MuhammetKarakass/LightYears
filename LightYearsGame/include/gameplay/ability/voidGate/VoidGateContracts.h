#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::VoidGate
{
	struct AbilityId
	{
		inline static constexpr char Basic[] =
			"Ability.Utility.VoidGate.Basic";
	};

	inline const vega::GameplayTag CategoryTag = vega::GameplayTags::Ability::Utility;
	inline const vega::GameplayTag FamilyTag =
		vega::GameplayTags::Ability::Family::VoidGate;

	struct State
	{
		inline static const vega::GameplayTag WaitingForPortalB =
			vega::GameplayTags::State::Ability::VoidGate::WaitingForPortalB;
		inline static const vega::GameplayTag Active =
			vega::GameplayTags::State::Ability::VoidGate::Active;
	};

	struct Event
	{
		inline static const vega::GameplayTag Started =
			vega::GameplayTags::Event::Ability::VoidGate::Started;
		inline static const vega::GameplayTag PortalAPlaced =
			vega::GameplayTags::Event::Ability::VoidGate::PortalAPlaced;
		inline static const vega::GameplayTag PortalBPlaced =
			vega::GameplayTags::Event::Ability::VoidGate::PortalBPlaced;
		inline static const vega::GameplayTag TransferStarted =
			vega::GameplayTags::Event::Ability::VoidGate::TransferStarted;
		inline static const vega::GameplayTag TransferCompleted =
			vega::GameplayTags::Event::Ability::VoidGate::TransferCompleted;
		inline static const vega::GameplayTag Ended =
			vega::GameplayTags::Event::Ability::VoidGate::Ended;
	};

	struct Attribute
	{
		inline static const sas::AttributeId PortalRadius =
			vega::CommonAttributeIds::Radius;
		inline static const sas::AttributeId TransferDuration{
			"Ability.Utility.VoidGate.TransferDuration"
		};
		inline static const sas::AttributeId ReentryCooldown{
			"Ability.Utility.VoidGate.ReentryCooldown"
		};
		inline static const sas::AttributeId PortalBPlacementTimeout{
			"Ability.Utility.VoidGate.PortalBPlacementTimeout"
		};
		inline static const sas::AttributeId EnergyPowerDurationScale{
			"Ability.Utility.VoidGate.EnergyPowerDurationScale"
		};
	};

	struct Actor
	{
		struct Portal
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.VoidGate.Portal.Basic";
		};
	};

	inline constexpr float DefaultPortalRadius = 70.f;
	inline constexpr float DefaultTransferDuration = 0.25f;
	inline constexpr float DefaultReentryCooldown = 1.f;
	inline constexpr float DefaultPortalBPlacementTimeout = 6.f;
	inline constexpr float DefaultEnergyPowerDurationScale = 1.f;
	inline constexpr float DefaultActiveDuration = 7.f;
	inline constexpr float DefaultCooldown = 12.f;
}
