#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/ability/content/GameAbilityProgression.h"
#include "gameplay/ability/foldspaceArena/FoldspaceArenaContracts.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/foldspaceArena/FoldspaceArenaPresentationIds.h"

namespace AbilityData::FoldspaceArena
{
	inline const ly::AbilityActorDefinition ActorArenaBasic = []
	{
		ly::AbilityActorDefinition definition;
		definition.actorDefinitionId = Actor::Arena::BasicDefinitionId;
		definition.actorType = ly::AbilityActorType::FoldspaceArena;
		// Runtime duration is resolved from EnergyPower and extended through travel.
		// This is only the fallback before ability values are applied.
		definition.lifeTime = 10.f;
		definition.spawnDistance = 0.f;
		definition.presentationProfileId = ly::FoldspaceArenaPresentationIds::ArenaBasic;
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Duration, 10.f, 0.01f },
			sas::GameplayAttribute{ Actor::Arena::Width, 1500.f, 1.f },
			sas::GameplayAttribute{ Actor::Arena::Height, 1000.f, 1.f },
			sas::GameplayAttribute{ Actor::Arena::ProjectileSpeed, 1200.f, 1.f },
			sas::GameplayAttribute{ Actor::Arena::CornerRadius, 250.f, 0.f },
			sas::GameplayAttribute{ Actor::Arena::WrapInwardOffset, 20.f, 0.f }
		};
		return definition;
	}();
}

namespace AbilityData::Definitions
{
	inline const ly::GameAbilityDefinition FoldspaceArena_Basic = []
	{
		ly::GameAbilityDefinition definition;
		definition.abilityId = AbilityData::FoldspaceArena::AbilityId::Basic;
		definition.slot = sas::AbilitySlot::Ability1;
		definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
		definition.lifetimePolicy = sas::AbilityLifetimePolicy::Duration;
		// The behavior extends this lifecycle through the resolved arena duration
		// and projectile travel time; this is the safe fallback duration.
		definition.duration = 10.f;
		definition.cooldown = 17.f;
		definition.maxCharges = 1;
		definition.abilityTags = {
			ly::GameplayTags::Ability::Utility,
			ly::GameplayTags::Ability::Family::FoldspaceArena
		};
		definition.damageTags = { ly::DamageTypeSchema::Photonic };
		definition.displayName = "Foldspace Arena";
		definition.iconPath = "SpaceShooterRedux/PNG/Power-ups/powerupBlue_bolt.png";
		definition.accentColor = sf::Color{ 145, 215, 255, 255 };
		definition.attributes = {
			sas::GameplayAttribute{ ly::CommonAttributeIds::Range, 300.f, 1.f },
			sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, 30.f, 0.f },
			sas::GameplayAttribute{
				AbilityData::FoldspaceArena::Attribute::MinimumArenaDuration, 1.f, 0.f
			},
			sas::GameplayAttribute{
				AbilityData::FoldspaceArena::Attribute::BaseArenaDuration, 7.f, 0.01f
			},
			sas::GameplayAttribute{
				AbilityData::FoldspaceArena::Attribute::EnergyPowerDamageScale,
				0.20f,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::FoldspaceArena::Attribute::EnergyPowerDurationScale,
				0.75f,
				0.f
			}
		};
		const float cooldownDeltas[] = {
			-0.600f, -0.600f, -0.600f, -0.600f,
			-0.500f, -0.500f, -0.500f, -0.500f,
			-0.400f, -0.400f, -0.400f, -0.400f,
			-0.300f, -0.300f
		};
		definition.levelProgression.reserve(14);
		for (const float cooldownDelta : cooldownDeltas)
		{
			definition.levelProgression.push_back(ly::AbilityLevelStep{
				{
					sas::AttributeModifier{
						ly::CommonAttributeIds::Damage,
						sas::AttributeModifierOperation::Add,
						5.f
					},
					sas::AttributeModifier{
						AbilityData::FoldspaceArena::Attribute::EnergyPowerDamageScale,
						sas::AttributeModifierOperation::Add,
						0.02f
					},
					sas::AttributeModifier{
						AbilityData::FoldspaceArena::Attribute::BaseArenaDuration,
						sas::AttributeModifierOperation::Add,
						0.15f
					},
					sas::AttributeModifier{
						ly::CommonAttributeIds::Cooldown,
						sas::AttributeModifierOperation::Add,
						cooldownDelta
					}
				}, {}, {}, {}
			});
		}
		definition.levelUpgradeScrapCosts = {
			60, 60, 60, 60, 60, 60, 60,
			60, 60, 60, 60, 60, 60, 60
		};
		definition.behaviorType = ly::AbilityBehaviorType::FoldspaceArena;
		return definition;
	}();
}
