#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/orbitalDrones/OrbitalDronesContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

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
			sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 200.f, 1.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 18.f, 0.f },
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
			sas::GameplayAttribute{
				AbilityData::OrbitalDrones::Attribute::EnergyPowerReference,
				50.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::OrbitalDrones::Attribute::EnergyPowerDurationScale,
				0.02f,
				0.f
			}
		};
		definition.scalingRules = {
			sas::AttributeScalingRule{
				ly::CommonAttributeIds::Damage,
				ly::OwnerAttributeIds::AttackPower,
				sas::AttributeModifierOperation::Add,
				0.50f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Add,
						2.f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						-0.25f
					}
				},
				{},
				{},
				{}
			}
		);
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		// Physical drone contact uses the established Kinetic damage domain.
		definition.damageTags = { ly::DamageTypeSchema::Kinetic };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage
		};
		definition.behaviorType = ly::AbilityBehaviorType::OrbitalDrones;
		return definition;
	}();
}
