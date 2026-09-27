#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/orbitalDrones/OrbitalDronesContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

#include <algorithm>

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition OrbitalDrones_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::OrbitalDrones::AbilityId::Basic;
		// The content default remains Ability4/R. The runtime loadout may later
		// rebind the acquired ability to another player ability slot.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 12.f;
		definition.duration = 6.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::OrbitalDrones
		};
		definition.displayName = "Orbital Drones";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupRed_bolt.png";
		definition.accentColor = sf::Color{ 255, 145, 45, 255 };
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 500.f, 1.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 25.f, 0.f },
			sas::GameplayAttribute{
				AbilityData::OrbitalDrones::Attribute::DroneCount,
				4.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::OrbitalDrones::Attribute::SameTargetHitCooldown,
				0.5f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::OrbitalDrones::Attribute::BaseAngularSpeedRadiansPerSecond,
				2.5f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::OrbitalDrones::Attribute::ContactRadius,
				12.f,
				0.1f
			},
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				0.40f
			}
		};
		float resolvedCooldown = definition.cooldown;
		for (int targetLevel = 2; targetLevel <= 25; ++targetLevel)
		{
			const float cooldownReduction = targetLevel <= 5 ? 0.50f :
				targetLevel <= 9 ? 0.40f :
				targetLevel <= 13 ? 0.30f :
				targetLevel <= 17 ? 0.20f :
				targetLevel <= 21 ? 0.10f : 0.08f;
			const float nextCooldown = std::max(7.f, resolvedCooldown - cooldownReduction);
			definition.levelProgression.push_back(ly::AbilityLevelStep{
				{
					{ ly::CommonAttributeIds::Damage, sas::AttributeModifierOperation::Add, 4.f },
					{ ly::CommonAttributeIds::Cooldown, sas::AttributeModifierOperation::Add, nextCooldown - resolvedCooldown }
				},
				{}, {}, {},
				{ { ly::CommonAttributeIds::Damage, ly::OwnerAttributeIds::AttackPower, sas::AttributeModifierOperation::Add, 0.04f } }
			});
			resolvedCooldown = nextCooldown;
		}
		definition.levelUpgradeScrapCosts.assign(24, 60);
		// Physical drone contact uses the established Kinetic damage domain.
		definition.damageTags = { ly::DamageTypeSchema::Kinetic };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage
		};
		definition.behaviorType = ly::AbilityBehaviorType::OrbitalDrones;
		return definition;
	}();
}
