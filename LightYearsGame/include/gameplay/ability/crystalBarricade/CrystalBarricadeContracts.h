#pragma once

#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::CrystalBarricade
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Defense.CrystalBarricade.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTags::Ability::Defense };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.CrystalBarricade" };
	inline const ly::GameplayTag FamilyTag{ ly::GameplayTags::Ability::Family::CrystalBarricade };

	struct Event
	{
		inline static const ly::GameplayTag Placed{ ly::GameplayTags::Event::Ability::CrystalBarricade::Placed };
		inline static const ly::GameplayTag Ended{ ly::GameplayTags::Event::Ability::CrystalBarricade::Ended };
	};

	struct Actor
	{
		struct Wall
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.CrystalBarricade.Wall.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.CrystalBarricade.Wall"
			};
			inline static const sas::AttributeId BaseContactDamage{
				"AbilityActor.CrystalBarricade.Wall.BaseContactDamage"
			};
			inline static const sas::AttributeId EnergyMaxContactScale{
				"AbilityActor.CrystalBarricade.Wall.EnergyMaxContactScale"
			};
			inline static const sas::AttributeId ContactInterval{
				"AbilityActor.CrystalBarricade.Wall.ContactInterval"
			};
			inline static const sas::AttributeId BaseRicochetMultiplier{
				"AbilityActor.CrystalBarricade.Wall.BaseRicochetMultiplier"
			};
			inline static const sas::AttributeId EnergyMaxRicochetReference{
				"AbilityActor.CrystalBarricade.Wall.EnergyMaxRicochetReference"
			};
			inline static const sas::AttributeId EnergyMaxRicochetScale{
				"AbilityActor.CrystalBarricade.Wall.EnergyMaxRicochetScale"
			};
			inline static const sas::AttributeId SameSurfaceLockDuration{
				"AbilityActor.CrystalBarricade.Wall.SameSurfaceLockDuration"
			};
			inline static const sas::AttributeId MaxHealthDurationReference{
				"AbilityActor.CrystalBarricade.Wall.MaxHealthDurationReference"
			};
			inline static const sas::AttributeId MaxHealthDurationScale{
				"AbilityActor.CrystalBarricade.Wall.MaxHealthDurationScale"
			};
		};
	};
}
