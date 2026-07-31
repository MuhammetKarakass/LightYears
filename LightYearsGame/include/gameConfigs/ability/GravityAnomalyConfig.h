#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationIds.h"

namespace AbilityData
{
	namespace GravityAnomaly
	{
		inline const ly::GameplayTag BehaviorId{ "GameAbilityBehavior.GravityAnomaly" };
		inline const ly::GameplayTag FamilyTag{ "Ability.Control.GravityAnomaly" };

		struct EffectSchema
		{
			inline static const ly::GameplayTag BehaviorId{
				"EffectBehavior.GravityAnomaly"
			};
			inline static constexpr char InsideEffectId[] =
				"Effect.GravityAnomaly.Inside";
			inline static const ly::GameplayTag InsideTag{
				"Effect.GravityAnomaly.Inside"
			};
			inline static constexpr float InsideEffectDurationSeconds = 2.f;
		};

		struct ActorSchema
		{
			inline static const ly::GameplayTag AttributeRoot{
				"Attribute.AbilityActor.GravityAnomaly"
			};
			inline static const ly::GameplayTag ProjectileTypeId{
				"AbilityActor.GravityAnomaly.Projectile"
			};
			inline static const ly::GameplayTag FieldTypeId{
				"AbilityActor.GravityAnomaly.Field"
			};
			inline static const ly::GameplayTag ProjectileSpeed{
				"Attribute.AbilityActor.GravityAnomaly.ProjectileSpeed"
			};
			inline static const ly::GameplayTag CastRange{
				"Attribute.AbilityActor.GravityAnomaly.CastRange"
			};
			inline static const ly::GameplayTag PullStrength{
				"Attribute.AbilityActor.GravityAnomaly.PullStrength"
			};
			inline static const ly::GameplayTag SlowMagnitude{
				"Attribute.AbilityActor.GravityAnomaly.SlowMagnitude"
			};
		};

		struct Settings
		{
			float cooldown = 8.f;
			int chargeCount = 1;
			float castRange = 900.f;
			float projectileSpeed = 2000.f;
			float baseDuration = 2.5f;
			float baseRadius = 220.f;
			float pullStrength = 500.f;
			float slowMagnitude = 0.20f;
			float spawnDistance = 42.f;
			float cooldownReductionPerLevel = 0.10f;
			float durationPerLevel = 0.03f;
			float radiusPerLevel = 2.f;
			float pullStrengthPerLevel = 10.f;
			float slowMagnitudePerLevel = 0.005f;
			float projectileSpeedPerLevel = 25.f;
			float castRangePerLevel = 5.f;
			float radiusPerMaxHealth = 0.20f;
			float durationPerMaxHealth = 0.0025f;
		};

		inline const Settings BasicSettings{};

		inline const Settings* FindSettings(const std::string& abilityId)
		{
			return abilityId == "Ability.GravityAnomaly.Basic" ? &BasicSettings : nullptr;
		}

		inline const ly::AbilityActorDefinition ActorProjectileBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = "Actor.Ability.GravityAnomaly.Projectile.Basic";
			definition.actorTypeTag = ActorSchema::ProjectileTypeId;
			definition.texturePath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
			definition.lifeTime = BasicSettings.baseDuration;
			definition.spawnDistance = BasicSettings.spawnDistance;
			definition.attributes = {
				sas::GameplayAttribute{ ActorSchema::ProjectileSpeed, BasicSettings.projectileSpeed, 0.01f },
				sas::GameplayAttribute{ ActorSchema::CastRange, BasicSettings.castRange, 0.01f },
				sas::GameplayAttribute{ ly::CommonAttributeIds::Duration, BasicSettings.baseDuration, 0.01f },
				sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, BasicSettings.baseRadius, 0.01f },
				sas::GameplayAttribute{ ActorSchema::PullStrength, BasicSettings.pullStrength, 0.f },
				sas::GameplayAttribute{ ActorSchema::SlowMagnitude, BasicSettings.slowMagnitude, 0.f, 0.95f }
			};
			definition.presentationProfileId = ly::GravityAnomalyPresentationIds::ProjectileBasic;
			return definition;
		}();

		inline const ly::AbilityActorDefinition ActorFieldBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = "Actor.Ability.GravityAnomaly.Field.Basic";
			definition.actorTypeTag = ActorSchema::FieldTypeId;
			definition.lifeTime = BasicSettings.baseDuration;
			definition.attributes = {
				sas::GameplayAttribute{ ly::CommonAttributeIds::Duration, BasicSettings.baseDuration, 0.01f },
				sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, BasicSettings.baseRadius, 0.01f },
				sas::GameplayAttribute{ ActorSchema::PullStrength, BasicSettings.pullStrength, 0.f },
				sas::GameplayAttribute{ ActorSchema::SlowMagnitude, BasicSettings.slowMagnitude, 0.f, 0.95f }
			};
			definition.presentationProfileId = ly::GravityAnomalyPresentationIds::FieldBasic;
			return definition;
		}();

		inline const ly::AbilityActorDefinition* FindActorDefinition(
			const std::string& actorDefinitionId
		)
		{
			if (actorDefinitionId == ActorProjectileBasic.actorDefinitionId)
			{
				return &ActorProjectileBasic;
			}
			return actorDefinitionId == ActorFieldBasic.actorDefinitionId
				? &ActorFieldBasic
				: nullptr;
		}
	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition GravityAnomaly_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = "Ability.GravityAnomaly.Basic";
			definition.slot = sas::AbilitySlot::Ability1;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.cooldown = GravityAnomaly::BasicSettings.cooldown;
			definition.maxCharges = GravityAnomaly::BasicSettings.chargeCount;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Control" },
				GravityAnomaly::FamilyTag
			};
			definition.displayName = "Gravity Anomaly";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserBlue04.png";
			definition.inputLabel = "Q";
			definition.accentColor = sf::Color{ 135, 95, 255, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					ly::SpawnActorAction{
						GravityAnomaly::ActorProjectileBasic.actorDefinitionId,
						sas::AbilitySpawnPolicy::OwnerForward,
						sas::AbilityDirectionPolicy::MouseWorld
					},
					0.f,
					1
				}
			};
			definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
				14,
				ly::AbilityLevelStep{
					{
						sas::AttributeModifier{ ly::CommonAttributeIds::Cooldown, sas::AttributeModifierOperation::Add, -GravityAnomaly::BasicSettings.cooldownReductionPerLevel },
						sas::AttributeModifier{ ly::CommonAttributeIds::Duration, sas::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.durationPerLevel },
						sas::AttributeModifier{ ly::CommonAttributeIds::Radius, sas::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.radiusPerLevel },
						sas::AttributeModifier{ GravityAnomaly::ActorSchema::PullStrength, sas::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.pullStrengthPerLevel },
						sas::AttributeModifier{ GravityAnomaly::ActorSchema::SlowMagnitude, sas::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.slowMagnitudePerLevel },
						sas::AttributeModifier{ GravityAnomaly::ActorSchema::ProjectileSpeed, sas::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.projectileSpeedPerLevel },
						sas::AttributeModifier{ GravityAnomaly::ActorSchema::CastRange, sas::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.castRangePerLevel }
					}
				}
			);
			definition.levelUpgradeScrapCosts = ly::List<unsigned int>(14, 60u);
			definition.scalingRules = {
				sas::AttributeScalingRule{
					ly::CommonAttributeIds::Radius,
					ly::OwnerAttributeIds::MaxHealth,
					sas::AttributeModifierOperation::Add,
					GravityAnomaly::BasicSettings.radiusPerMaxHealth
				},
				sas::AttributeScalingRule{
					ly::CommonAttributeIds::Duration,
					ly::OwnerAttributeIds::MaxHealth,
					sas::AttributeModifierOperation::Add,
					GravityAnomaly::BasicSettings.durationPerMaxHealth
				}
			};
			definition.behaviorId = GravityAnomaly::BehaviorId;
			return definition;
		}();
	}

	namespace AbilityActors
	{
		inline const ly::AbilityActorDefinition& Actor_GravityAnomaly_Projectile_Basic =
			GravityAnomaly::ActorProjectileBasic;
		inline const ly::AbilityActorDefinition& Actor_GravityAnomaly_Field_Basic =
			GravityAnomaly::ActorFieldBasic;
	}
}
