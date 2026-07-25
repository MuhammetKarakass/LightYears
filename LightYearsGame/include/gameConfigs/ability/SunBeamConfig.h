#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/ability/AbilityStructs.h"
#include "presentation/ability/sunBeam/SunBeamPresentationIds.h"

namespace AbilityData
{
	namespace SunBeam
	{
		inline const ly::GameplayTag BehaviorId{ "AbilityBehavior.SunBeam" };
		inline const ly::GameplayTag FamilyTag{ "Ability.Offense.SunBeam" };

		struct ActorSchema
		{
			inline static const ly::GameplayTag SharedAttributeRoot{
				"Attribute.AbilityActor.SunBeam.Shared"
			};
			inline static const ly::GameplayTag Width{
				"Attribute.AbilityActor.SunBeam.Shared.Width"
			};
			inline static const ly::GameplayTag Length{
				"Attribute.AbilityActor.SunBeam.Shared.Length"
			};

			struct Strike
			{
				inline static const ly::GameplayTag TypeId{ "AbilityActor.SunBeam.Strike" };
				inline static const ly::GameplayTag AttributeRoot{
					"Attribute.AbilityActor.SunBeam.Strike"
				};
				inline static const ly::GameplayTag TelegraphDuration{
					"Attribute.AbilityActor.SunBeam.Strike.TelegraphDuration"
				};
				inline static const ly::GameplayTag ArrivalDuration{
					"Attribute.AbilityActor.SunBeam.Strike.ArrivalDuration"
				};
				inline static const ly::GameplayTag ImpactDelay{
					"Attribute.AbilityActor.SunBeam.Strike.ImpactDelay"
				};
				inline static const ly::GameplayTag ImpactVisualDuration{
					"Attribute.AbilityActor.SunBeam.Strike.ImpactVisualDuration"
				};
			};
		};

		inline const ly::AbilityActorDefinition ActorStrikeBasic{
			"Actor.Ability.SunBeam.Strike.Basic",
			ActorSchema::Strike::TypeId,
			"",
			0.f,
			0.f,
			{
				ly::GameplayAttribute{ ly::CommonAttributeIds::Damage, 40.f, 0.f },
				ly::GameplayAttribute{ ly::CommonAttributeIds::Radius, 96.f, 1.f },
				ly::GameplayAttribute{ ActorSchema::Width, 72.f, 1.f },
				ly::GameplayAttribute{ ActorSchema::Length, 720.f, 1.f },
				ly::GameplayAttribute{ ActorSchema::Strike::TelegraphDuration, 0.5f, 0.f },
				ly::GameplayAttribute{ ActorSchema::Strike::ArrivalDuration, 0.2f, 0.f },
				ly::GameplayAttribute{ ActorSchema::Strike::ImpactDelay, 0.05f, 0.f },
				ly::GameplayAttribute{ ActorSchema::Strike::ImpactVisualDuration, 0.22f, 0.f }
			},
			ly::SunBeamPresentationIds::StrikeBasic
		};

		inline const ly::AbilityActorDefinition* FindActorDefinition(
			const std::string& actorDefinitionId)
		{
			return actorDefinitionId == ActorStrikeBasic.actorDefinitionId
				? &ActorStrikeBasic
				: nullptr;
		}
	}

	namespace Definitions
	{
		inline const ly::AbilityDefinition SunBeam_Strike_Basic = []
		{
			ly::AbilityDefinition definition;
			definition.abilityId = "Ability.SunBeam.Strike.Basic";
			definition.slot = ly::AbilitySlot::Ability2;
			definition.activationPolicy = ly::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = ly::AbilityLifetimePolicy::Instant;
			definition.cooldown = 1.f;
			definition.duration = 0.f;
			definition.maxCharges = 1;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Offense" },
				SunBeam::FamilyTag
			};
			definition.displayName = "Sun Beam";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue01.png";
			definition.inputLabel = "E";
			definition.accentColor = sf::Color{ 255, 190, 70, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					ly::AbilityActionPhase::OnActivate,
					ly::SpawnActorAction{
						SunBeam::ActorStrikeBasic.actorDefinitionId,
						ly::AbilitySpawnPolicy::MouseWorld
					},
					0.f,
					1
				}
			};
			definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
				4,
				ly::AbilityLevelStep{
					{
						ly::AttributeModifier{
							ly::CommonAttributeIds::Damage,
							ly::AttributeModifierOperation::Add,
							8.f
						},
						ly::AttributeModifier{
							ly::CommonAttributeIds::Radius,
							ly::AttributeModifierOperation::Add,
							8.f
						}
					}
				}
			);
			definition.levelUpgradeScrapCosts = { 40u, 50u, 65u, 80u };
			definition.behaviorId = SunBeam::BehaviorId;
			return definition;
		}();
	}

	namespace AbilityActors
	{
		inline const ly::AbilityActorDefinition& Actor_SunBeam_Basic =
			SunBeam::ActorStrikeBasic;
	}
}
