#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/attributes/AttributeIds.h"

#include "gameplay/ability/content/GameAbilityDefinition.h"
#include "gameplay/attachment/AttachmentDefinition.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "presentation/ability/rocket/RocketPresentationIds.h"

namespace AbilityData
{
	namespace Rocket
	{
		inline const ly::GameplayTag BehaviorId{ "GameAbilityBehavior.Rocket" };
		inline const ly::GameplayTag FamilyTag{ "Ability.Offense.Rocket" };

		struct ActorSchema
		{
			inline static const ly::GameplayTag TypeId{ "AbilityActor.Rocket.Projectile" };
			inline static const ly::GameplayTag AttributeRoot{ "Attribute.AbilityActor.Rocket" };
			inline static const ly::GameplayTag ProjectileSpeed{
				"Attribute.AbilityActor.Rocket.ProjectileSpeed"
			};
		};

		struct Settings
		{
			float baseDamage = 0.f;
			float cooldown = 0.f;
			float projectileSpeed = 0.f;
			float range = 0.f;
			float explosionRadius = 0.f;
			int projectileCount = 0;
			float collisionRadius = 0.f;
			float spawnDistance = 0.f;
			float cleanupGraceDuration = 0.f;
			float damagePerLevel = 0.f;
			float cooldownReductionPerLevel = 0.f;
			float explosionRadiusPerLevel = 0.f;
		};

		// Projectile speed and range belong to delivery feel, not normal skill
		// progression. They intentionally remain fixed at every Rocket level.
		inline const Settings BasicSettings{
			55.f,   // baseDamage
			3.f,    // cooldown
			1000.f, // projectileSpeed
			1100.f, // range
			55.f,   // explosionRadius
			1,      // projectileCount
			8.f,    // collisionRadius
			42.f,   // spawnDistance
			0.25f,  // cleanupGraceDuration
			4.f,    // damagePerLevel
			0.12f,  // cooldownReductionPerLevel
			1.f     // explosionRadiusPerLevel
		};

		inline const Settings* FindSettings(const std::string& abilityId)
		{
			return abilityId == "Ability.Rocket.Basic" ? &BasicSettings : nullptr;
		}

		inline const ly::AbilityActorDefinition ActorProjectileBasic = []
		{
			ly::AbilityActorDefinition definition;
			definition.actorDefinitionId = "Actor.Ability.Rocket.Projectile.Basic";
			definition.actorTypeTag = ActorSchema::TypeId;
			definition.texturePath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
			definition.lifeTime =
				BasicSettings.range / BasicSettings.projectileSpeed + BasicSettings.cleanupGraceDuration;
			definition.spawnDistance = BasicSettings.spawnDistance;
			definition.attributes = {
				sas::GameplayAttribute{ ly::CommonAttributeIds::Damage, BasicSettings.baseDamage, 0.f },
				sas::GameplayAttribute{ ly::CommonAttributeIds::Radius, BasicSettings.explosionRadius, 0.f },
				sas::GameplayAttribute{ ActorSchema::ProjectileSpeed, BasicSettings.projectileSpeed, 0.f },
				sas::GameplayAttribute{ ly::CommonAttributeIds::Range, BasicSettings.range, 0.f },
				sas::GameplayAttribute{ ly::CommonAttributeIds::CollisionRadius, BasicSettings.collisionRadius, 0.1f }
			};
			definition.presentationProfileId = ly::RocketPresentationIds::Basic;
			return definition;
		}();

		inline const ly::AbilityActorDefinition* FindActorDefinition(
			const std::string& actorDefinitionId)
		{
			return actorDefinitionId == ActorProjectileBasic.actorDefinitionId
				? &ActorProjectileBasic
				: nullptr;
		}
	}

	namespace Definitions
	{
		inline const ly::GameAbilityDefinition Rocket_Basic = []
		{
			ly::GameAbilityDefinition definition;
			definition.abilityId = "Ability.Rocket.Basic";
			definition.slot = sas::AbilitySlot::Ability4;
			definition.activationPolicy = sas::AbilityActivationPolicy::OnPressed;
			definition.lifetimePolicy = sas::AbilityLifetimePolicy::Instant;
			definition.cooldown = Rocket::BasicSettings.cooldown;
			definition.duration = 0.f;
			definition.maxCharges = 1;
			definition.abilityTags = {
				ly::GameplayTag{ "Ability.Offense" },
				Rocket::FamilyTag
			};
			definition.displayName = "Rocket";
			definition.iconPath = "SpaceShooterRedux/PNG/Lasers/laserRed04.png";
			definition.inputLabel = "R";
			definition.accentColor = sf::Color{ 255, 115, 75, 255 };
			definition.actions = {
				ly::AbilityActionSpec{
					sas::AbilityActionPhase::OnActivate,
					ly::SpawnActorAction{
						Rocket::ActorProjectileBasic.actorDefinitionId,
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
						sas::AttributeModifier{
							ly::CommonAttributeIds::Damage,
							sas::AttributeModifierOperation::Add,
							Rocket::BasicSettings.damagePerLevel
						},
						sas::AttributeModifier{
							ly::CommonAttributeIds::Cooldown,
							sas::AttributeModifierOperation::Add,
							-Rocket::BasicSettings.cooldownReductionPerLevel
						},
						sas::AttributeModifier{
							ly::CommonAttributeIds::Radius,
							sas::AttributeModifierOperation::Add,
							Rocket::BasicSettings.explosionRadiusPerLevel
						}
					}
				}
			);
			definition.levelUpgradeScrapCosts = ly::List<unsigned int>(14, 60u);
			definition.scalingRules = {
				sas::AttributeScalingRule{
					ly::CommonAttributeIds::Damage,
					ly::OwnerAttributeIds::AttackPower,
					sas::AttributeModifierOperation::Add,
					1.25f
				}
			};
			definition.damageTags = { ly::DamageTypeSchema::Kinetic };
			definition.attachmentCapabilities = { ly::AttachmentSchema::Capability::Damage };
			definition.behaviorId = Rocket::BehaviorId;
			return definition;
		}();
	}

	namespace AbilityActors
	{
		inline const ly::AbilityActorDefinition& Actor_Rocket_Basic =
			Rocket::ActorProjectileBasic;
	}
}
