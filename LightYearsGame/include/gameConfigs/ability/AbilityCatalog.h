#pragma once

#include "gameConfigs/ability/DashConfig.h"
#include "gameConfigs/ability/RocketConfig.h"
#include "gameConfigs/ability/ShieldConfig.h"
#include "gameConfigs/ability/SunBeamConfig.h"
#include "gameConfigs/combat/WeaponConfig.h"
#include "gameplay/attachment/AttachmentDefinition.h"

namespace AbilityData
{
	namespace Definitions
	{
		inline ly::AbilityDefinition MakePrimaryFireAbilityDefinition(
			const PrimaryWeaponDefinition& weaponDefinition)
		{
			const bool automaticFire = weaponDefinition.automaticFire;
			ly::List<ly::AbilityLevelStep> primaryLevelProgression;
			for (const PrimaryWeaponLevelStep& weaponLevelStep :
				weaponDefinition.progressionProfile.ResolveLevelSteps())
			{
				ly::AbilityLevelStep abilityLevelStep;
				abilityLevelStep.attributeModifiers = weaponLevelStep.attributeModifiers;
				abilityLevelStep.unlockedUpgradeIds = weaponLevelStep.unlockedUpgradeIds;
				abilityLevelStep.unlockedUpgradeIds.insert(
					abilityLevelStep.unlockedUpgradeIds.end(),
					weaponLevelStep.unlockedFeatureTags.begin(),
					weaponLevelStep.unlockedFeatureTags.end()
				);
				primaryLevelProgression.push_back(abilityLevelStep);
			}

			ly::AbilityDefinition definition;
			definition.abilityId = weaponDefinition.weaponId;
			definition.slot = ly::AbilitySlot::PrimaryFire;
			definition.activationPolicy = automaticFire
				? ly::AbilityActivationPolicy::WhileHeld
				: ly::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = automaticFire
				? ly::AbilityLifetimePolicy::WhileInputHeld
				: ly::AbilityLifetimePolicy::Instant;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Primary" },
				ly::GameplayTag{ "Ability.Offense" },
				weaponDefinition.weaponTypeTag
			};
			definition.displayName = weaponDefinition.weaponId;
			definition.iconPath = weaponDefinition.presentationDefinition.texturePath;
			definition.inputLabel = "Space";
			definition.accentColor = sf::Color{ 100, 220, 255, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					automaticFire
						? ly::AbilityActionPhase::WhileActive
						: ly::AbilityActionPhase::OnActivate,
					ly::FireWeaponAction{ weaponDefinition },
					0.f,
					automaticFire ? 0 : 1
				}
			};
			definition.levelProgression = std::move(primaryLevelProgression);
			definition.levelUpgradeScrapCosts =
				weaponDefinition.progressionProfile.levelUpgradeScrapCosts;
			definition.damageTags = weaponDefinition.damageTags.empty()
				? ly::List<ly::GameplayTag>{ ly::DamageTypeSchema::Photonic }
				: weaponDefinition.damageTags;
			definition.attachmentCapabilities = {
				ly::AttachmentSchema::Capability::Damage
			};
			return definition;
		}
	}

	inline ly::AbilityDefinition MakePrimaryFireAbilityDefinition(
		const PrimaryWeaponDefinition& weaponDefinition)
	{
		return Definitions::MakePrimaryFireAbilityDefinition(weaponDefinition);
	}

	inline const ly::List<const ly::AbilityDefinition*>& GetShippedAbilityDefinitions()
	{
		static const ly::List<const ly::AbilityDefinition*> definitions{
			&Definitions::Shield_Basic,
			&Definitions::SunBeam_Strike_Basic,
			&Definitions::Dash_Basic,
			&Definitions::Rocket_Basic
		};
		return definitions;
	}

	inline const ly::AbilityDefinition* FindShippedAbilityDefinition(
		const std::string& abilityId)
	{
		for (const ly::AbilityDefinition* definition : GetShippedAbilityDefinitions())
		{
			if (definition && definition->abilityId == abilityId)
			{
				return definition;
			}
		}
		return nullptr;
	}
}
