#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/voidGate/VoidGateContracts.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/voidGate/VoidGatePresentationIds.h"

namespace AbilityData::Definitions
{
	inline const ly::AbilityActorDefinition VoidGatePortalBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId =
			AbilityData::VoidGate::Actor::Portal::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::VoidGatePortal;
		definition.presentationProfileId = ly::VoidGatePresentationIds::PortalBasic;
		definition.attributes = {
			sas::GameplayAttribute{
				ly::CommonAttributeIds::Radius,
				AbilityData::VoidGate::DefaultPortalRadius,
				1.f
			}
		};
		return definition;
	}();

	inline const ly::GameAbilityDefinition VoidGate_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::VoidGate::AbilityId::Basic;
		// Ability1 is the default Q slot. Runtime loadout binding may still move
		// Void Gate to another Ability1-4 slot later.
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.cooldown = AbilityData::VoidGate::DefaultCooldown;
		definition.duration = AbilityData::VoidGate::DefaultActiveDuration;
		definition.maxCharges = 1;
		// Void Gate needs two positional inputs. Echo replay only has one stored
		// activation context, so this staged ability explicitly opts out of history.
		definition.recordInAbilityHistory = false;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Utility,
			ly::GameplayTags::Ability::Family::VoidGate
		};
		definition.displayName = "Void Gate";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue.png";
		definition.accentColor = sf::Color{ 145, 80, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{
				AbilityData::VoidGate::Attribute::PortalRadius,
				AbilityData::VoidGate::DefaultPortalRadius,
				1.f
			},
			sas::GameplayAttribute{
				AbilityData::VoidGate::Attribute::TransferDuration,
				AbilityData::VoidGate::DefaultTransferDuration,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::VoidGate::Attribute::ReentryCooldown,
				AbilityData::VoidGate::DefaultReentryCooldown,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::VoidGate::Attribute::PortalBPlacementTimeout,
				AbilityData::VoidGate::DefaultPortalBPlacementTimeout,
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::VoidGate::Attribute::EnergyPowerReference,
				AbilityData::VoidGate::DefaultEnergyPowerReference,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::VoidGate::Attribute::EnergyPowerDurationScale,
				AbilityData::VoidGate::DefaultEnergyPowerDurationScale,
				0.f
			}
		};
		definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
			14,
			ly::AbilityLevelStep{
				{
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
		definition.behaviorType = ly::AbilityBehaviorType::VoidGate;
		return definition;
	}();
}
