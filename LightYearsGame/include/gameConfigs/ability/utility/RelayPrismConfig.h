#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/actors/AbilityActorType.h"
#include "gameplay/ability/relayPrism/RelayPrismContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/relayPrism/RelayPrismPresentationIds.h"

namespace AbilityData::RelayPrism
{
	inline const ly::AbilityActorDefinition ActorRelayBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Relay::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::RelayPrism;
		definition.presentationProfileId = ly::RelayPrismPresentationIds::RelayBasic;
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
		// Numeric balance, progression and costs are authored in abilities.json.
		// Ability2 is the default E binding. The runtime loadout can still
		// rebind this active ability to another player slot later.
		definition.slot = sas::AbilitySlot::Ability2;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Utility,
			ly::GameplayTags::Ability::Family::RelayPrism
		};
		definition.displayName = "Relay Prism";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_star.png";
		definition.accentColor = sf::Color{ 220, 100, 255, 255 };
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Projectile,
			ly::AttachmentSchema::Capability::Damage
		};
		definition.behaviorType = ly::AbilityBehaviorType::RelayPrism;
		return definition;
	}();
}
