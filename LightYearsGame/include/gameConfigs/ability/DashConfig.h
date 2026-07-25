#pragma once

#include "gameConfigs/ability/AbilityStructs.h"

namespace AbilityData
{
	namespace Dash
	{
		inline const ly::GameplayTag BehaviorId{ "AbilityBehavior.Dash" };
		inline const ly::GameplayTag FamilyTag{ "Ability.Movement.Dash" };
		inline const ly::GameplayTag StartEvent{ "Event.Ability.Dash.Start" };
		inline const ly::GameplayTag EndEvent{ "Event.Ability.Dash.End" };
		inline const ly::GameplayTag StateTag{ "State.Ability.Dashing" };

		enum class DirectionPolicy
		{
			MovementInputOrMouseWorld
		};

		struct Settings
		{
			float baseDistance = 0.f;
			float duration = 0.f;
			float cameraZoomOutRatio = 0.f;              // 0.15 = current camera target +15%.
			float cooldownReductionPerLevelRatio = 0.f; // Portion of base cooldown removed per level.
			DirectionPolicy directionPolicy = DirectionPolicy::MovementInputOrMouseWorld;
		};

		inline const Settings BasicSettings{
			260.f, // baseDistance
			0.24f, // duration
			0.15f, // cameraZoomOutRatio
			0.06f, // cooldownReductionPerLevelRatio
			DirectionPolicy::MovementInputOrMouseWorld
		};

		inline const Settings* FindSettings(const std::string& abilityId)
		{
			return abilityId == "Ability.Dash.Basic" ? &BasicSettings : nullptr;
		}
	}

	namespace Definitions
	{
		inline const ly::AbilityDefinition Dash_Basic = []
		{
			ly::AbilityDefinition definition;
			definition.abilityId = "Ability.Dash.Basic";
			definition.slot = ly::AbilitySlot::Ability3;
			definition.activationPolicy = ly::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = ly::AbilityLifetimePolicy::Duration;
			definition.cooldown = 2.f; // Base cooldown before level progression and Ability Haste.
			definition.duration = Dash::BasicSettings.duration;
			definition.maxCharges = 1;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Movement" },
				Dash::FamilyTag
			};
			definition.displayName = "Dash";
			definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/star_gold.png";
			definition.inputLabel = "F";
			definition.accentColor = sf::Color{ 120, 220, 255, 255 };
			definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
				4,
				ly::AbilityLevelStep{
					{
						ly::AttributeModifier{
							ly::CommonAttributeIds::Cooldown,
							ly::AttributeModifierOperation::Add,
							-definition.cooldown * Dash::BasicSettings.cooldownReductionPerLevelRatio
						}
					}
				}
			);
			definition.levelUpgradeScrapCosts = { 40u, 50u, 65u, 80u };
			definition.behaviorId = Dash::BehaviorId;
			return definition;
		}();
	}
}
