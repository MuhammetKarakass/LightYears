#pragma once

#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/infernoSpray/InfernoSprayContracts.h"
#include "presentation/ability/infernoSpray/InfernoSprayPresentationIds.h"

namespace AbilityData
{
	namespace InfernoSpray
	{
		// Actor/behavior/presentation contract only. Numeric tuning lives in abilities.json.

		inline const ly::AbilityActorDefinition ActorFlameConeBasic{
			"Actor.Ability.InfernoSpray.FlameCone.Basic",
			ActorSchema::TypeId,
			"",
			0.f,
			0.f,
			{},
			ly::InfernoSprayPresentationIds::Basic
		};

		inline const ly::AbilityActorDefinition* FindActorDefinition(
			const std::string& actorDefinitionId)
		{
			return actorDefinitionId == ActorFlameConeBasic.actorDefinitionId
				? &ActorFlameConeBasic
				: nullptr;
		}
	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition InfernoSpray_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = "Ability.InfernoSpray.Basic";
			definition.slot = sas::AbilitySlot::Ability2;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Offense" },
				InfernoSpray::FamilyTag
			};
			definition.damageTags = {
				ly::DamageTypeSchema::Thermal
			};
			definition.displayName = "Inferno Spray";
			definition.iconPath = "SpaceShooterRedux/PNG/Effects/star2.png";
			definition.inputLabel = "E";
			definition.accentColor = sf::Color{ 255, 100, 20, 255 };

			definition.actions = {
				ly::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					ly::SpawnActorAction{
						InfernoSpray::ActorFlameConeBasic.actorDefinitionId,
						sas::AbilitySpawnPolicy::AtOwner
					},
					0.f,
					1
				}
			};
			definition.behaviorId = InfernoSpray::BehaviorId;
			return definition;
		}();
	}

	namespace AbilityActors
	{
		inline const ly::AbilityActorDefinition& Actor_InfernoSpray_Basic =
			InfernoSpray::ActorFlameConeBasic;
	}
}
