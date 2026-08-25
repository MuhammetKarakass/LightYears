#pragma once

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/nanoPlague/NanoPlagueContracts.h"
#include "gameplay/tags/GameplayTags.h"

namespace AbilityData::Definitions
{
	// JSON is authoritative for balance. This fallback supplies only the stable
	// content identity used by behavior registration and content-loader tests.
	inline const ly::GameAbilityDefinition NanoPlague_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::NanoPlague::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
		definition.cooldown = AbilityData::NanoPlague::DefaultCooldown;
		definition.duration = 0.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Offense,
			ly::GameplayTags::Ability::Family::NanoPlague
		};
		definition.displayName = "Nano Plague";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupGreen_bolt.png";
		definition.accentColor = sf::Color{ 95, 255, 140, 255 };
		definition.damageTags = { ly::DamageTypeSchema::Energy };
		definition.behaviorType = ly::AbilityBehaviorType::NanoPlague;
		return definition;
	}();
}
