#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/infernoSpray/InfernoSprayContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationIds.h"

namespace AbilityData
{
	namespace InfernoSpray
	{
		// Actor/presentation schema only. Numeric tuning lives in abilities.json.

		inline const vega::AbilityActorDefinition ActorFlameConeBasic = []
		{
			vega::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::FlameCone::BasicDefinitionId;
			definition.actorType = vega::AbilityActorType::InfernoSprayFlameCone;
			definition.presentationProfileId =
				vega::InfernoSprayPresentationIds::FlameConeBasic;
			return definition;
		}();

	}

	namespace Definitions
	{
		inline const vega::GameAbilityDefinition InfernoSpray_Basic = []
		{
			vega::GameAbilityDefinition definition;
			definition.abilityId = InfernoSpray::AbilityId::Basic;
			// Numeric balance, progression and costs are authored in abilities.json.
			definition.slot = sas::AbilitySlot::Ability2;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
			definition.abilityTags = {
				vega::GameplayTags::Ability::Offense,
				vega::GameplayTags::Ability::Family::InfernoSpray
			};
			definition.damageTags = {
				vega::DamageTypeSchema::Thermal
			};
			definition.displayName = "Inferno Spray";
			definition.iconPath = "SpaceShooterRedux/PNG/Effects/star2.png";
			definition.accentColor = sf::Color{ 255, 100, 20, 255 };

			definition.actions = {
				vega::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					vega::SpawnActorAction{
						InfernoSpray::ActorFlameConeBasic.actorDefinitionId,
						sas::AbilitySpawnPolicy::AtOwner
					},
					0.f,
					1
				}
			};
			definition.behaviorType = vega::AbilityBehaviorType::InfernoSpray;
			return definition;
		}();
	}

}
