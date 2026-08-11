#pragma once

#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
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

		inline const ly::AbilityActorDefinition ActorFlameConeBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::FlameCone::BasicDefinitionId;
			definition.actorType = ly::AbilityActorType::InfernoSprayFlameCone;
			definition.presentationProfileId =
				ly::InfernoSprayPresentationIds::FlameConeBasic;
			return definition;
		}();

	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition InfernoSpray_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = InfernoSpray::AbilityId::Basic;
			definition.slot = sas::AbilitySlot::Ability2;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTags::Ability::Offense,
				ly::GameplayTags::Ability::Family::InfernoSpray
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
			definition.behaviorType = ly::AbilityBehaviorType::InfernoSpray;
			return definition;
		}();
	}

}
