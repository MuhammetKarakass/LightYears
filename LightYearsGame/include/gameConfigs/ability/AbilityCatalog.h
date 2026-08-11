#pragma once

#include "gameConfigs/ability/functional/DashConfig.h"
#include "gameConfigs/ability/offensive/GravityAnomalyConfig.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/ability/offensive/OverdriveCoreConfig.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameConfigs/ability/defensive/ShieldConfig.h"
#include "gameConfigs/ability/defensive/PhaseDriftConfig.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTagSchema.h"

namespace AbilityData
{
	namespace Definitions
	{
		inline ly::GameAbilityDefinition MakePrimaryFireAbilityDefinition(
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
				for (const PrimaryWeaponFeatureType featureType : weaponLevelStep.unlockedFeatureTypes)
				{
					abilityLevelStep.unlockedUpgradeIds.emplace_back(
						PrimaryWeaponFeatureUpgradeId(featureType)
					);
				}
				primaryLevelProgression.push_back(abilityLevelStep);
			}

			ly::GameAbilityDefinition definition;
			definition.abilityId = weaponDefinition.weaponId;
			definition.slot = sas::AbilitySlot::PrimaryFire;
			definition.activationPolicy = automaticFire
				? sas::AbilityActivationPolicy::WhileHeld
				: sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = automaticFire
				? sas::AbilityLifetimePolicy::WhileInputHeld
				: sas::AbilityLifetimePolicy::Instant;
			definition.cooldown = 0.f;
			definition.duration = 0.f;
			definition.maxCharges = 0;
			definition.abilityTags = {
				ly::GameplayTagSchema::AbilityPrimary,
				ly::GameplayTagSchema::AbilityOffense
			};
			definition.displayName = weaponDefinition.weaponId;
			definition.iconPath = weaponDefinition.presentationDefinition.texturePath;
			definition.inputLabel = "Space";
			definition.accentColor = sf::Color{ 100, 220, 255, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					automaticFire
						? sas::AbilityActionPhase::WhileActive
						: sas::AbilityActionPhase::OnActivate,
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

	inline ly::GameAbilityDefinition MakePrimaryFireAbilityDefinition(
		const PrimaryWeaponDefinition& weaponDefinition)
	{
		return Definitions::MakePrimaryFireAbilityDefinition(weaponDefinition);
	}

	const ly::List<const ly::GameAbilityDefinition*>& GetBuiltinShippedAbilityDefinitions();
	const ly::List<const ly::AbilityActorDefinition*>& GetBuiltinAbilityActorDefinitions();
	const ly::List<const ly::GameAbilityDefinition*>& GetShippedAbilityDefinitions();
	const ly::GameAbilityDefinition* FindShippedAbilityDefinition(
		const std::string& abilityId
	);
}
