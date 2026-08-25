#include "gameplay/ability/actors/AbilityActorSpawner.h"

#include "gameplay/ability/GameAbilityActionExecutor.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace ly
{
	namespace
	{
		sas::GameplayAttributeList BuildAbilityActorAttributes(
			const AbilityActorDefinition& actorDefinition
		)
		{
			sas::GameplayAttributeList attributes = actorDefinition.attributes;
			if (actorDefinition.lifeTime > 0.f &&
				!sas::HasAttribute(attributes, CommonAttributeIds::Duration))
			{
				attributes.push_back(sas::GameplayAttribute{
					CommonAttributeIds::Duration,
					actorDefinition.lifeTime,
					0.f
				});
			}
			return attributes;
		}

		float ResolveRotation(
			const sf::Vector2f& direction,
			float fallbackRotation
		)
		{
			if (GetVectorLength(direction) <= 0.001f)
			{
				return fallbackRotation;
			}

			constexpr float DegreesPerRadian = 57.2957795131f;
			return std::atan2(direction.y, direction.x) * DegreesPerRadian + 90.f;
		}

		weak_ptr<Actor> MakeWeakActor(Actor* actor)
		{
			if (!actor)
			{
				return {};
			}

			const shared_ptr<Object> object = actor->GetWeakPtr().lock();
			return object
				? std::dynamic_pointer_cast<Actor>(object)
				: weak_ptr<Actor>{};
		}

		sf::Vector2f ResolveSpawnLocation(
			const Actor& owner,
			const SpawnActorAction& actionData,
			const AbilityActorDefinition& actorDefinition,
			const AbilityExecutionContext& context,
			const sf::Vector2f& direction
		)
		{
			switch (actionData.spawnPolicy)
			{
			case sas::AbilitySpawnPolicy::AtEventTarget:
				if (context.event)
				{
					if (const Actor* target = context.event->GetTarget<Actor>())
					{
						return target->GetActorLocation();
					}
				}
				break;
			case sas::AbilitySpawnPolicy::MouseWorld:
				if (const World* world = owner.GetWorld())
				{
					return world->GetMouseWorldPosition();
				}
				break;
			case sas::AbilitySpawnPolicy::OwnerForward:
				return owner.GetActorLocation() + direction * actorDefinition.spawnDistance;
			case sas::AbilitySpawnPolicy::AtOwner:
			default:
				break;
			}

			return owner.GetActorLocation();
		}

		std::optional<sf::Vector2f> ResolveTargetLocation(
			const Actor& owner,
			const SpawnActorAction& actionData,
			const AbilityExecutionContext& context
		)
		{
			if (actionData.directionPolicy == sas::AbilityDirectionPolicy::MouseWorld ||
				actionData.spawnPolicy == sas::AbilitySpawnPolicy::MouseWorld)
			{
				if (const World* world = owner.GetWorld();
					world && world->GetApplication())
				{
					return world->GetMouseWorldPosition();
				}
			}

			if (actionData.spawnPolicy == sas::AbilitySpawnPolicy::AtEventTarget &&
				context.event)
			{
				if (const Actor* target = context.event->GetTarget<Actor>())
				{
					return target->GetActorLocation();
				}
			}

			return std::nullopt;
		}

		weak_ptr<AbilityWorldActor> SpawnResolved(
			const sas::ContentId& actorDefinitionId,
			AbilityExecutionContext& context,
			Actor& owner,
			const sf::Vector2f& direction,
			const sf::Vector2f& spawnLocation,
			const std::optional<sf::Vector2f>& targetLocation,
			float damageMultiplier,
			Actor* targetActor
		)
		{
			if (!owner.GetWorld() || !context.abilitySystem || !context.definition)
			{
				return {};
			}

			const AbilityActorDefinition* actorDefinition =
				AbilityData::FindAbilityActorDefinition(actorDefinitionId.ToString());
			if (!actorDefinition)
			{
				return {};
			}

			const sas::GameplayAttributeList attributes =
				BuildAbilityActorAttributes(*actorDefinition);
			const List<GameplayTag> damageTags =
				AbilityActionAttributeResolver::ResolveDamageTags(
					context,
					AttachmentHostKind::Ability
				);
			const sas::GameplayAttributeList values =
				AbilityActionAttributeResolver::ResolveAttributes(
					context,
					nullptr,
					attributes,
					damageTags
				);
			weak_ptr<AbilityWorldActor> spawnedActor = AbilityActorRegistry::Spawn(
				AbilityActorSpawnContext{
					owner,
					*actorDefinition,
					values,
					targetLocation,
					MakeWeakActor(targetActor),
					context.instance,
					context.abilitySystem,
					context.definition
				}
			);

			auto actor = spawnedActor.lock();
			if (!actor)
			{
				return {};
			}

			const float duration = sas::FindAttributeValue(
				values,
				CommonAttributeIds::Duration,
				actorDefinition->lifeTime
			);
		const float damage = sas::FindAttributeValue(
				values,
				CommonAttributeIds::Damage,
				0.f
			);
		const float collisionRadius = sas::FindAttributeValue(
				values,
				CollisionAttributeIds::Radius,
				sas::FindAttributeValue(values, CommonAttributeIds::Radius, 0.f)
			);

			actor->SetActorLocation(spawnLocation);
			actor->SetActorRotation(ResolveRotation(direction, owner.GetActorRotation()));
			actor->SetLifeTime(duration);
			actor->SetDamageTags(damageTags);
			actor->SetSourceAbility(
				sas::ContentId{ context.definition->abilityId },
				context.definition->abilityTags
			);
			// Preserve the exact runtime instance instead of looking up by content ID
			// later. This matters when the same family is equipped in a different
			// runtime slot or when a projectile survives the ability's instant phase.
			actor->SetSourceAbilityInstance(context.instance);
			actor->SetAbilityUpgradeIds(context.definition->unlockedUpgradeIds);
			actor->SetAbilityCollisionRadius(collisionRadius);
			actor->ConfigureCollisionFromOwner();
			actor->ConfigureFromAttributes(values);
			// ConfigureFromAttributes rebuilds the base damage payload, so the
			// per-projectile decay is applied after the common actor setup.
			actor->SetDamage(damage * std::max(0.f, damageMultiplier));
			return spawnedActor;
		}
	}

	sf::Vector2f AbilityActorSpawner::ResolveDirection(
		const Actor& owner,
		sas::AbilityDirectionPolicy directionPolicy
	)
	{
		switch (directionPolicy)
		{
		case sas::AbilityDirectionPolicy::OwnerVelocity:
		{
			sf::Vector2f velocityDirection = owner.GetVelocity();
			if (GetVectorLength(velocityDirection) > 0.f)
			{
				NormalizeVector(velocityDirection);
				return velocityDirection;
			}
			break;
		}
		case sas::AbilityDirectionPolicy::MouseWorld:
			if (const World* world = owner.GetWorld();
				world && world->GetApplication())
			{
				sf::Vector2f mouseDirection =
					world->GetMouseWorldPosition() - owner.GetActorLocation();
				if (GetVectorLength(mouseDirection) > 0.f)
				{
					NormalizeVector(mouseDirection);
					return mouseDirection;
				}
			}
			break;
		case sas::AbilityDirectionPolicy::OwnerForward:
		default:
			break;
		}

		return owner.GetActorForwardDirection();
	}

	weak_ptr<AbilityWorldActor> AbilityActorSpawner::Spawn(
		const SpawnActorAction& actionData,
		AbilityExecutionContext& context,
		Actor& owner
	)
	{
		const sf::Vector2f direction = ResolveDirection(
			owner,
			actionData.directionPolicy
		);
		const AbilityActorDefinition* actorDefinition =
			AbilityData::FindAbilityActorDefinition(actionData.actorDefinitionId.ToString());
		if (!actorDefinition)
		{
			return {};
		}
		return SpawnResolved(
			actionData.actorDefinitionId,
			context,
			owner,
			direction,
			ResolveSpawnLocation(
				owner,
				actionData,
				*actorDefinition,
				context,
				direction
			),
			ResolveTargetLocation(owner, actionData, context),
			1.f,
			nullptr
		);
	}

	weak_ptr<AbilityWorldActor> AbilityActorSpawner::SpawnToTarget(
		const sas::ContentId& actorDefinitionId,
		AbilityExecutionContext& context,
		Actor& owner,
		const sf::Vector2f& direction,
		const std::optional<sf::Vector2f>& targetLocation,
		float damageMultiplier,
		Actor* targetActor
	)
	{
		const AbilityActorDefinition* actorDefinition =
			AbilityData::FindAbilityActorDefinition(actorDefinitionId.ToString());
		return SpawnResolved(
			actorDefinitionId,
			context,
			owner,
			direction,
			owner.GetActorLocation() + direction * (
				actorDefinition ? actorDefinition->spawnDistance : 0.f
			),
			targetLocation,
			damageMultiplier,
			targetActor
		);
	}

	weak_ptr<AbilityWorldActor> AbilityActorSpawner::SpawnAtLocation(
		const sas::ContentId& actorDefinitionId,
		AbilityExecutionContext& context,
		Actor& owner,
		const sf::Vector2f& location,
		const sf::Vector2f& direction,
		float damageMultiplier
	)
	{
		return SpawnAtLocation(
			actorDefinitionId,
			context,
			owner,
			location,
			direction,
			damageMultiplier,
			std::nullopt,
			nullptr
		);
	}

	weak_ptr<AbilityWorldActor> AbilityActorSpawner::SpawnAtLocation(
		const sas::ContentId& actorDefinitionId,
		AbilityExecutionContext& context,
		Actor& owner,
		const sf::Vector2f& location,
		const sf::Vector2f& direction,
		float damageMultiplier,
		const std::optional<sf::Vector2f>& targetLocation,
		Actor* targetActor
	)
	{
		return SpawnResolved(
			actorDefinitionId,
			context,
			owner,
			direction,
			location,
			targetLocation,
			damageMultiplier,
			targetActor
		);
	}
}
