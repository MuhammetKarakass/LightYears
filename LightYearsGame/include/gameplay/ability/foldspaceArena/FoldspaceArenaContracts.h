#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::FoldspaceArena
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Utility.FoldspaceArena.Basic";
	};

	inline const vega::GameplayTag CategoryTag{ vega::GameplayTags::Ability::Utility };
	inline const vega::GameplayTag FamilyTag{
		vega::GameplayTags::Ability::Family::FoldspaceArena
	};

	struct State
	{
		inline static const vega::GameplayTag Active{
			vega::GameplayTags::State::Ability::FoldspaceArena::Active
		};
		inline static const vega::GameplayTag CancelAvailable{
			vega::GameplayTags::State::Ability::FoldspaceArena::CancelAvailable
		};
	};

	struct Event
	{
		inline static const vega::GameplayTag Started{
			vega::GameplayTags::Event::Ability::FoldspaceArena::Started
		};
		inline static const vega::GameplayTag CancelAvailable{
			vega::GameplayTags::Event::Ability::FoldspaceArena::CancelAvailable
		};
		inline static const vega::GameplayTag Ended{
			vega::GameplayTags::Event::Ability::FoldspaceArena::Ended
		};
	};

	struct Actor
	{
		struct Arena
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.FoldspaceArena.Arena.Basic";
			inline static const sas::AttributeId AttributeRoot{
				"AbilityActor.FoldspaceArena.Arena"
			};
			inline static const sas::AttributeId Width = vega::AreaAttributeIds::Width;
			inline static const sas::AttributeId Height = vega::AreaAttributeIds::Length;
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.FoldspaceArena.Arena.ProjectileSpeed"
			};
			inline static const sas::AttributeId CornerRadius{
				"AbilityActor.FoldspaceArena.Arena.CornerRadius"
			};
			inline static const sas::AttributeId WrapInwardOffset{
				"AbilityActor.FoldspaceArena.Arena.WrapInwardOffset"
			};
		};
	};

	struct Attribute
	{
		// These values control the ability lifecycle. Fixed delivery geometry lives
		// on Actor::Arena, just like other fixed projectile/field geometry.
		inline static const sas::AttributeId MinimumArenaDuration{
			"Ability.Utility.FoldspaceArena.MinimumArenaDuration"
		};
		inline static const sas::AttributeId BaseArenaDuration{
			"Ability.Utility.FoldspaceArena.BaseArenaDuration"
		};
		inline static const sas::AttributeId EnergyPowerDamageScale{
			"Ability.Utility.FoldspaceArena.EnergyPowerDamageScale"
		};
		inline static const sas::AttributeId EnergyPowerDurationScale{
			"Ability.Utility.FoldspaceArena.EnergyPowerDurationScale"
		};
	};
}
