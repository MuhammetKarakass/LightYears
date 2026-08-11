#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/sunBeam/SunBeamContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/sunBeam/SunBeamPresentationIds.h"

namespace AbilityData
{
	namespace SunBeam
	{
		// Actor/presentation schema only. Numeric tuning lives in abilities.json.

		inline const ly::AbilityActorDefinition ActorStrikeBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::Strike::BasicDefinitionId;
			definition.actorType = ly::AbilityActorType::SunBeamStrike;
			definition.presentationProfileId = ly::SunBeamPresentationIds::StrikeBasic;
			return definition;
		}();

	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition SunBeam_Strike_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = SunBeam::AbilityId::Strike::Basic;
			definition.slot = sas::AbilitySlot::Ability2;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTags::Ability::Offense,
				ly::GameplayTags::Ability::Family::SunBeam
			};
			definition.displayName = "Sun Beam";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
			definition.inputLabel = "E";
			definition.accentColor = sf::Color{ 255, 190, 70, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					ly::SpawnActorAction{
						SunBeam::ActorStrikeBasic.actorDefinitionId,
						sas::AbilitySpawnPolicy::MouseWorld
					},
					0.f,
					1
				}
			};
			definition.behaviorType = ly::AbilityBehaviorType::SunBeam;
			return definition;
		}();
	}

}
