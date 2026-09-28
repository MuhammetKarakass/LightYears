#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/relayPrism/RelayPrismContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/relayPrism/RelayPrismPresentationIds.h"

namespace AbilityData::RelayPrism
{
	inline const ly::AbilityActorDefinition ActorRelayBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Relay::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::RelayPrism;
		definition.presentationProfileId = ly::RelayPrismPresentationIds::RelayBasic;
		definition.lifeTime = 4.f;
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, 100.f, 1.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Range, 600.f, 1.f },
			sas::GameplayAttribute{
				Actor::Relay::ProjectileSpeed,
				450.f,
				1.f
			}
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	// C++ owns identity, executable behavior, actor type and presentation type.
	// JSON owns the shipped balance values and progression.
	inline const ly::GameAbilityDefinition RelayPrism_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::RelayPrism::AbilityId::Basic;
		// Ability2 is the default E binding. The runtime loadout can still
		// rebind this active ability to another player slot later.
		definition.slot = sas::AbilitySlot::Ability2;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = 10.f;
		definition.duration = 4.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Utility,
			ly::GameplayTags::Ability::Family::RelayPrism
		};
		definition.displayName = "Relay Prism";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 220, 100, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::ProjectileCount,
				4.f,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::BaseTransfer,
				0.20f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::EnergyPowerScale,
				0.04f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::MinimumScatterAngle,
				30.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::MaximumScatterAngle,
				90.f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::RelayPrism::Attribute::MaximumBonusProjectileCount,
				4.f,
				0.f
			}
		};
		definition.levelProgression.reserve(14);
		for (int step = 0; step < 14; ++step)
		{
			const float cooldownDelta = step < 4
				? -0.425f
				: step < 8 ? -0.325f : step < 12 ? -0.225f : -0.125f;
			definition.levelProgression.push_back(ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						AbilityData::RelayPrism::Attribute::BaseTransfer,
						sas::AttributeModifierOperation::Add,
						0.02f
					},
					sas::AttributeModifier{
						AbilityData::RelayPrism::Attribute::EnergyPowerScale,
						sas::AttributeModifierOperation::Add,
						0.02f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						cooldownDelta
					}
				},
				{},
				{},
				{}
			});
		}
		definition.levelUpgradeScrapCosts = {
			40, 50, 65, 80, 100, 125, 155,
			190, 230, 275, 325, 380, 440, 505
		};
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Projectile,
			ly::AttachmentSchema::Capability::Damage
		};
		definition.behaviorType = ly::AbilityBehaviorType::RelayPrism;
		return definition;
	}();
}
