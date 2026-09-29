#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/orbitalDrones/OrbitalDronesContracts.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition OrbitalDrones_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::OrbitalDrones::AbilityId::Basic;
		// Numeric balance, progression and costs are authored in abilities.json.
		// The content default remains Ability4/R. The runtime loadout may later
		// rebind the acquired ability to another player ability slot.
		definition.slot = sas::AbilitySlot::Ability4;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::OrbitalDrones
		};
		definition.displayName = "Orbital Drones";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupRed_bolt.png";
		definition.accentColor = sf::Color{ 255, 145, 45, 255 };
		// Physical drone contact uses the established Kinetic damage domain.
		definition.attachmentCapabilities = {
			ly::AttachmentSchema::Capability::Damage
		};
		definition.behaviorType = ly::AbilityBehaviorType::OrbitalDrones;
		return definition;
	}();
}
