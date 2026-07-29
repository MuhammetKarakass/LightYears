#pragma once

#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameConfigs/ability/AbilityStructs.h"
#include "presentation/ability/gravityAnomaly/GravityAnomalyPresentationIds.h"

namespace AbilityData
{
	namespace GravityAnomaly
	{
		inline const ly::GameplayTag BehaviorId{ "AbilityBehavior.GravityAnomaly" };
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
				ly::GameplayAttribute{ ActorSchema::ProjectileSpeed, BasicSettings.projectileSpeed, 0.01f },
				ly::GameplayAttribute{ ActorSchema::CastRange, BasicSettings.castRange, 0.01f },
				ly::GameplayAttribute{ ly::CommonAttributeIds::Duration, BasicSettings.baseDuration, 0.01f },
				ly::GameplayAttribute{ ly::CommonAttributeIds::Radius, BasicSettings.baseRadius, 0.01f },
				ly::GameplayAttribute{ ActorSchema::PullStrength, BasicSettings.pullStrength, 0.f },
				ly::GameplayAttribute{ ActorSchema::SlowMagnitude, BasicSettings.slowMagnitude, 0.f, 0.95f }
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
				ly::GameplayAttribute{ ly::CommonAttributeIds::Duration, BasicSettings.baseDuration, 0.01f },
				ly::GameplayAttribute{ ly::CommonAttributeIds::Radius, BasicSettings.baseRadius, 0.01f },
				ly::GameplayAttribute{ ActorSchema::PullStrength, BasicSettings.pullStrength, 0.f },
				ly::GameplayAttribute{ ActorSchema::SlowMagnitude, BasicSettings.slowMagnitude, 0.f, 0.95f }
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
		inline const ly::AbilityDefinition GravityAnomaly_Basic = []
		{
			ly::AbilityDefinition definition;
			definition.abilityId = "Ability.GravityAnomaly.Basic";
			definition.slot = ly::AbilitySlot::Ability1;
			definition.activationPolicy = ly::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = ly::AbilityLifetimePolicy::Instant;
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
					ly::AbilityActionPhase::OnActivate,
					ly::SpawnActorAction{
						GravityAnomaly::ActorProjectileBasic.actorDefinitionId,
						ly::AbilitySpawnPolicy::OwnerForward,
						ly::AbilityDirectionPolicy::MouseWorld
					},
					0.f,
					1
				}
			};
			definition.levelProgression = ly::MakeRepeatedAbilityLevelProgression(
				14,
				ly::AbilityLevelStep{
					{
						ly::AttributeModifier{ ly::CommonAttributeIds::Cooldown, ly::AttributeModifierOperation::Add, -GravityAnomaly::BasicSettings.cooldownReductionPerLevel },
						ly::AttributeModifier{ ly::CommonAttributeIds::Duration, ly::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.durationPerLevel },
						ly::AttributeModifier{ ly::CommonAttributeIds::Radius, ly::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.radiusPerLevel },
						ly::AttributeModifier{ GravityAnomaly::ActorSchema::PullStrength, ly::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.pullStrengthPerLevel },
						ly::AttributeModifier{ GravityAnomaly::ActorSchema::SlowMagnitude, ly::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.slowMagnitudePerLevel },
						ly::AttributeModifier{ GravityAnomaly::ActorSchema::ProjectileSpeed, ly::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.projectileSpeedPerLevel },
						ly::AttributeModifier{ GravityAnomaly::ActorSchema::CastRange, ly::AttributeModifierOperation::Add, GravityAnomaly::BasicSettings.castRangePerLevel }
					}
				}
			);
			definition.levelUpgradeScrapCosts = ly::List<unsigned int>(14, 60u);
			definition.scalingRules = {
				ly::AttributeScalingRule{
					ly::CommonAttributeIds::Radius,
					ly::OwnerAttributeIds::MaxHealth,
					ly::AttributeModifierOperation::Add,
					GravityAnomaly::BasicSettings.radiusPerMaxHealth
				},
				ly::AttributeScalingRule{
					ly::CommonAttributeIds::Duration,
					ly::OwnerAttributeIds::MaxHealth,
					ly::AttributeModifierOperation::Add,
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
