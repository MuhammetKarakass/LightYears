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

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Utility };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.FoldspaceArena" };
	inline const ly::GameplayTag FamilyTag{
		ly::GameplayTags::Ability::Family::FoldspaceArena
	};

	struct State
	{
		inline static const ly::GameplayTag Active{
			ly::GameplayTags::State::Ability::FoldspaceArena::Active
		};
		inline static const ly::GameplayTag CancelAvailable{
			ly::GameplayTags::State::Ability::FoldspaceArena::CancelAvailable
		};
	};

	struct Event
	{
		inline static const ly::GameplayTag Started{
			ly::GameplayTags::Event::Ability::FoldspaceArena::Started
		};
		inline static const ly::GameplayTag CancelAvailable{
			ly::GameplayTags::Event::Ability::FoldspaceArena::CancelAvailable
		};
		inline static const ly::GameplayTag Ended{
			ly::GameplayTags::Event::Ability::FoldspaceArena::Ended
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
			inline static const sas::AttributeId Width = ly::AreaAttributeIds::Width;
			inline static const sas::AttributeId Height = ly::AreaAttributeIds::Length;
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
		inline static const sas::AttributeId EnergyPowerDurationReference{
			"Ability.Utility.FoldspaceArena.EnergyPowerDurationReference"
		};
		inline static const sas::AttributeId EnergyPowerDurationPerPoint{
			"Ability.Utility.FoldspaceArena.EnergyPowerDurationPerPoint"
		};
	};
}
