#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/vectorSync/VectorSyncContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition VectorSync_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::VectorSync::AbilityId::Basic;
		// Loadouts own real bindings. This only keeps the catalog definition
		// valid before a player equips Vector Sync into a runtime slot.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 10.f;
		definition.duration = 5.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Movement,
			ly::GameplayTags::Ability::Family::VectorSync
		};
		definition.displayName = "Vector Sync";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 90, 225, 255, 255 };
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Duration,
						sas::AttributeModifierOperation::Add,
						0.10f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.20f
					}
				}, {}, {}, {}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::VectorSync;
		return definition;
	}();
}
