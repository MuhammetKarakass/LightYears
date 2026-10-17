#pragma once

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

		inline const vega::AbilityActorDefinition ActorStrikeBasic = []
		{
			vega::AbilityActorDefinition definition;
			definition.actorDefinitionId = Actor::Strike::BasicDefinitionId;
			definition.actorType = vega::AbilityActorType::SunBeamStrike;
			definition.presentationProfileId = vega::SunBeamPresentationIds::StrikeBasic;
			return definition;
		}();

	}

	namespace Definitions
	{
		inline const vega::GameAbilityDefinition SunBeam_Strike_Basic = []
		{
			vega::GameAbilityDefinition definition;
			definition.abilityId = SunBeam::AbilityId::Strike::Basic;
			// Numeric balance, progression and costs are authored in abilities.json.
			definition.slot = sas::AbilitySlot::Ability2;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.abilityTags = {
				vega::GameplayTags::Ability::Offense,
				vega::GameplayTags::Ability::Family::SunBeam
			};
			definition.displayName = "Sun Beam";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
			definition.accentColor = sf::Color{ 255, 190, 70, 255 };
			definition.actions = {
				vega::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					vega::SpawnActorAction{
						SunBeam::ActorStrikeBasic.actorDefinitionId,
						sas::AbilitySpawnPolicy::MouseWorld
					},
					0.f,
					1
				}
			};
			definition.behaviorType = vega::AbilityBehaviorType::SunBeam;
			return definition;
		}();
	}

}
