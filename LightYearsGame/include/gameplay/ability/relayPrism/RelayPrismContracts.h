#pragma once

#include "attributes/AttributeId.h"
#include "framework/Core.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::RelayPrism
{
	struct AbilityId
	{
		inline static constexpr char Basic[] = "Ability.Utility.RelayPrism.Basic";
	};

	inline const ly::GameplayTag CategoryTag{ ly::GameplayTagSchema::AbilityUtility };
	inline const ly::GameplayTag BehaviorTag{ "GameAbilityBehavior.RelayPrism" };
	inline const ly::GameplayTag FamilyTag{ "Ability.Utility.RelayPrism" };

	struct Attribute
	{
		// ProjectileCount is shared because it is useful to other projectile
		// transforming mechanics too; Relay-specific policy remains family-local.
		inline static const sas::AttributeId ProjectileCount =
			ly::CommonAttributeIds::ProjectileCount;
		inline static const sas::AttributeId DamageTransferRatio{
			"Ability.Utility.RelayPrism.DamageTransferRatio"
		};
		inline static const sas::AttributeId AttackPowerCoefficient{
			"Ability.Utility.RelayPrism.AttackPowerCoefficient"
		};
		inline static const sas::AttributeId MinimumScatterAngle{
			"Ability.Utility.RelayPrism.MinimumScatterAngle"
		};
		inline static const sas::AttributeId MaximumScatterAngle{
			"Ability.Utility.RelayPrism.MaximumScatterAngle"
		};
		inline static const sas::AttributeId MaximumBonusProjectileCount{
			"Ability.Utility.RelayPrism.MaximumBonusProjectileCount"
		};
	};

	struct Actor
	{
		struct Relay
		{
			inline static constexpr char BasicDefinitionId[] =
				"Actor.Ability.RelayPrism.Relay.Basic";
			inline static const sas::AttributeId Root{
				"AbilityActor.RelayPrism.Relay"
			};
			// Relay Prism is a travelling ability actor; delivery speed belongs to
			// this actor role rather than being hidden in the ability behavior.
			inline static const sas::AttributeId ProjectileSpeed{
				"AbilityActor.RelayPrism.Relay.ProjectileSpeed"
			};
		};
	};

	struct State
	{
		inline static const ly::GameplayTag Active =
			ly::GameplayTags::State::Ability::RelayPrism::Active;
	};

	struct Event
	{
		inline static const ly::GameplayTag Started =
			ly::GameplayTags::Event::Ability::RelayPrism::Started;
		inline static const ly::GameplayTag Captured =
			ly::GameplayTags::Event::Ability::RelayPrism::Captured;
		inline static const ly::GameplayTag Ended =
			ly::GameplayTags::Event::Ability::RelayPrism::Ended;
	};
}
