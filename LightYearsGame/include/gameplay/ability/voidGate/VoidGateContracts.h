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

	inline const ly::GameplayTag CategoryTag = ly::GameplayTags::Ability::Utility;
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.VoidGate" };
	inline const ly::GameplayTag FamilyTag =
		ly::GameplayTags::Ability::Family::VoidGate;

	struct State
	{
		inline static const ly::GameplayTag WaitingForPortalB =
			ly::GameplayTags::State::Ability::VoidGate::WaitingForPortalB;
		inline static const ly::GameplayTag Active =
			ly::GameplayTags::State::Ability::VoidGate::Active;
	};

	struct Event
	{
		inline static const ly::GameplayTag Started =
			ly::GameplayTags::Event::Ability::VoidGate::Started;
		inline static const ly::GameplayTag PortalAPlaced =
			ly::GameplayTags::Event::Ability::VoidGate::PortalAPlaced;
		inline static const ly::GameplayTag PortalBPlaced =
			ly::GameplayTags::Event::Ability::VoidGate::PortalBPlaced;
		inline static const ly::GameplayTag TransferStarted =
			ly::GameplayTags::Event::Ability::VoidGate::TransferStarted;
		inline static const ly::GameplayTag TransferCompleted =
			ly::GameplayTags::Event::Ability::VoidGate::TransferCompleted;
		inline static const ly::GameplayTag Ended =
			ly::GameplayTags::Event::Ability::VoidGate::Ended;
	};

	struct Attribute
	{
		inline static const sas::AttributeId PortalRadius =
			ly::CommonAttributeIds::Radius;
		inline static const sas::AttributeId TransferDuration{
			"Ability.Utility.VoidGate.TransferDuration"
		};
		inline static const sas::AttributeId ReentryCooldown{
			"Ability.Utility.VoidGate.ReentryCooldown"
		};
		inline static const sas::AttributeId PortalBPlacementTimeout{
			"Ability.Utility.VoidGate.PortalBPlacementTimeout"
		};
		inline static const sas::AttributeId EnergyPowerReference{
			"Ability.Utility.VoidGate.EnergyPowerReference"
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
	inline constexpr float DefaultPortalBPlacementTimeout = 4.f;
	inline constexpr float DefaultEnergyPowerReference = 50.f;
	inline constexpr float DefaultEnergyPowerDurationScale = 0.002f;
	inline constexpr float DefaultActiveDuration = 6.f;
	inline constexpr float DefaultCooldown = 12.f;
}
