#pragma once

#include "gameConfigs/AbilityActorStructs.h"
#include "gameConfigs/AbilityStructs.h"
#include "gameConfigs/VisualConfig.h"
#include "gameConfigs/EffectConfig.h"
#include "gameConfigs/WeaponConfig.h"

namespace AbilityData
{
	namespace Definitions
	{
		static const ly::AbilityDefinition Shield_Basic{
			"Ability.Shield.Basic",
			ly::AbilitySlot::Ability1,
			ly::AbilityActivationPolicy::OnPressed,
			ly::AbilityLifetimePolicy::Duration,
			8.f,
			5.f,
			1,
			{
				ly::GameplayTag{ "Ability.Defense" },
				ly::AbilitySchema::Shield::FamilyTag
			},
			{},
			{},
			"Shield",
			"SpaceShooterRedux/PNG/Power-ups/shield_gold.png",
			"Q",
			sf::Color{ 80, 200, 255, 255 },
			{
				ly::AbilityActionSpec{
					ly::AbilityActionPhase::OnActivate,
					ly::ApplyEffectAction{ "Effect.Barrier.Basic", ly::AbilityTargetPolicy::Self },
					0.f,
					1
				}
			},
			{
				ly::AbilityTriggerSpec{
					ly::GameplayTag{ "Event.Owner.BarrierBroken" },
					0.f,
					{},
					{},
					{
						ly::AbilityActionSpec{
							ly::AbilityActionPhase::OnActivate,
							ly::ApplyEffectAction{ "Effect.Test.BarrierBreak.ThrustBoost", ly::AbilityTargetPolicy::Self },
							0.f,
							1
						}
					}
				}
			},
			{},
			{},
			{
				ly::AttributeScalingRule{
					BarrierEffectSchema::Capacity,
					ly::OwnerAttributeIds::MaxHealth,
					ly::AttributeModifierOperation::Add,
					0.2f
				},
				ly::AttributeScalingRule{
					BarrierEffectSchema::Capacity,
					ly::OwnerAttributeIds::Armor,
					ly::AttributeModifierOperation::Add,
					50.f
				}
			}
		};

		static const ly::AbilityDefinition SunBeam_Strike_Basic{
			"Ability.SunBeam.Strike.Basic",
			ly::AbilitySlot::Ability2,
			ly::AbilityActivationPolicy::OnPressed,
			ly::AbilityLifetimePolicy::Instant,
			10.f,
			0.f,
			1,
			{
				ly::GameplayTag{ "Ability.Offense" },
				ly::AbilitySchema::SunBeam::FamilyTag
			},
			{},
			{},
			"Sun Beam",
			"SpaceShooterRedux/PNG/Lasers/laserBlue01.png",
			"E",
			sf::Color{ 255, 190, 70, 255 },
			{
				ly::AbilityActionSpec{
					ly::AbilityActionPhase::OnActivate,
					ly::SpawnActorAction{
						"Actor.Ability.SunBeam.Strike.Basic",
						ly::AbilitySpawnPolicy::MouseWorld
					},
					0.f,
					1
				}
			},
			{},
			{
				ly::AbilityLevelStep{
					{
						ly::AttributeModifier{ ly::CommonAttributeIds::Damage, ly::AttributeModifierOperation::Add, 8.f },
						ly::AttributeModifier{ ly::CommonAttributeIds::Radius, ly::AttributeModifierOperation::Add, 8.f }
					}
				},
				ly::AbilityLevelStep{
					{
						ly::AttributeModifier{ ly::CommonAttributeIds::Damage, ly::AttributeModifierOperation::Add, 8.f },
						ly::AttributeModifier{ ly::CommonAttributeIds::Radius, ly::AttributeModifierOperation::Add, 8.f }
					}
				},
				ly::AbilityLevelStep{
					{
						ly::AttributeModifier{ ly::CommonAttributeIds::Damage, ly::AttributeModifierOperation::Add, 8.f },
						ly::AttributeModifier{ ly::CommonAttributeIds::Radius, ly::AttributeModifierOperation::Add, 8.f }
					}
				},
				ly::AbilityLevelStep{
					{
						ly::AttributeModifier{ ly::CommonAttributeIds::Damage, ly::AttributeModifierOperation::Add, 8.f },
						ly::AttributeModifier{ ly::CommonAttributeIds::Radius, ly::AttributeModifierOperation::Add, 8.f }
					}
				}
			},
			{},
			{}
		};

		inline ly::AbilityDefinition MakePrimaryFireAbilityDefinition(const PrimaryWeaponDefinition& weaponDefinition)
		{
			const bool automaticFire = weaponDefinition.automaticFire;
			ly::List<ly::AbilityLevelStep> primaryLevelProgression;
			for (const PrimaryWeaponLevelStep& weaponLevelStep : weaponDefinition.levelProgression)
			{
				primaryLevelProgression.push_back(ly::AbilityLevelStep{ weaponLevelStep.modifiers });
			}

			return ly::AbilityDefinition{
				weaponDefinition.weaponId,
				ly::AbilitySlot::PrimaryFire,
				automaticFire ? ly::AbilityActivationPolicy::WhileHeld : ly::AbilityActivationPolicy::OnPressed,
				automaticFire ? ly::AbilityLifetimePolicy::WhileInputHeld : ly::AbilityLifetimePolicy::Instant,
				0.f,
				0.f,
				0,
				{
					ly::GameplayTag{ "Ability.Primary" },
					ly::GameplayTag{ "Ability.Offense" },
					weaponDefinition.weaponTypeTag
				},
				{},
				{},
				weaponDefinition.weaponId,
				weaponDefinition.presentationDefinition.texturePath,
				"Space",
				sf::Color{ 100, 220, 255, 255 },
				{
					ly::AbilityActionSpec{
						automaticFire ? ly::AbilityActionPhase::WhileActive : ly::AbilityActionPhase::OnActivate,
						ly::FireWeaponAction{ weaponDefinition },
						0.f,
						automaticFire ? 0 : 1
					}
				},
				{},
				primaryLevelProgression,
				{},
				{}
			};
		}
	}

	namespace AbilityActors
	{
		static const ly::AbilityActorDefinition Actor_SunBeam_Basic{
			"Actor.Ability.SunBeam.Strike.Basic",
			ly::AbilityActorSchema::SunBeam::Strike::TypeId,
			"",
			0.f,
			0.f,
			{
				ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 40.f, 0.f },
				ly::GameplayAttribute{ ly::CommonAttributeIds::Radius, 96.f, 1.f },
				ly::GameplayAttribute{ ly::AbilityActorSchema::SunBeam::Width, 72.f, 1.f },
				ly::GameplayAttribute{ ly::AbilityActorSchema::SunBeam::Length, 720.f, 1.f },
				ly::GameplayAttribute{ ly::AbilityActorSchema::SunBeam::Strike::TelegraphDuration, 0.5f, 0.f },
				ly::GameplayAttribute{ ly::AbilityActorSchema::SunBeam::Strike::ArrivalDuration, 0.2f, 0.f },
				ly::GameplayAttribute{ ly::AbilityActorSchema::SunBeam::Strike::ImpactVisualDuration, 0.14f, 0.f }
			},
			"Visual.Telegraph.SunBeam.Strike",
			"Visual.SunBeam.Basic"
		};

		inline const ly::AbilityActorDefinition* FindAbilityActorDefinition(const std::string& actorDefinitionId)
		{
			if (actorDefinitionId == Actor_SunBeam_Basic.actorDefinitionId)
			{
				return &Actor_SunBeam_Basic;
			}

			return nullptr;
		}
	}

	inline ly::AbilityDefinition MakePrimaryFireAbilityDefinition(const PrimaryWeaponDefinition& weaponDefinition)
	{
		return Definitions::MakePrimaryFireAbilityDefinition(weaponDefinition);
	}

	inline const ly::AbilityActorDefinition* FindAbilityActorDefinition(const std::string& actorDefinitionId)
	{
		return AbilityActors::FindAbilityActorDefinition(actorDefinitionId);
	}
}
