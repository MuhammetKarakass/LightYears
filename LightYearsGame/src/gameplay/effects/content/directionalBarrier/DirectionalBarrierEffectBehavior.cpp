#include "gameplay/effects/content/directionalBarrier/DirectionalBarrierEffectBehavior.h"

#include "framework/Actor.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/directionalBarrier/DirectionalBarrierContracts.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/effects/LightYearsEffectBehaviorRuntime.h"
#include "gameConfigs/combat/EffectConfig.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "spaceShip/SpaceShip.h"
#include "gameplay/targeting/SweptGeometry.h"

#include <algorithm>
#include <cmath>

namespace vega::DirectionalBarrierEffectBehavior
{
	namespace
	{
		float Dot(const sf::Vector2f& left, const sf::Vector2f& right)
		{
			return left.x * right.x + left.y * right.y;
		}

		// Front arc is +/-60 degrees around facing: cos(60 deg) = 0.5.
		constexpr float FrontArcHalfAngleCosine = 0.5f;

		bool IsFrontApproach(const Actor& owner, const Actor& projectile)
		{
			sf::Vector2f approachDirection = -projectile.GetVelocity();
			if (GetVectorLength(approachDirection) <= 0.001f)
			{
				approachDirection = projectile.GetActorLocation() - owner.GetActorLocation();
			}
			if (GetVectorLength(approachDirection) <= 0.001f)
			{
				return false;
			}

			NormalizeVector(approachDirection);
			return Dot(owner.GetActorForwardDirection(), approachDirection) > FrontArcHalfAngleCosine;
		}

		sas::GameplayEffectBehaviorResult ProcessIncomingDamage(
			sas::ActiveGameplayEffect&,
			DamageContext& context
		)
		{
			sas::GameplayEffectBehaviorResult result;
			if (context.remainingDamage <= 0.f ||
				context.deliveryType != DamageDeliveryType::Projectile ||
				!context.deliveryActor ||
				!context.target ||
				!IsFrontApproach(*context.target, *context.deliveryActor))
			{
				return result;
			}

			// A directional barrier has no capacity and never partially absorbs a
			// hit. It rejects the entire projectile delivery, which also prevents
			// DamageTypeSystem from applying projectile-borne statuses because the
			// shared pipeline sees zero remaining damage afterward.
			const float blockedDamage = context.remainingDamage;
			context.remainingDamage = 0.f;
			context.absorbedDamage += blockedDamage;
			context.modifiedDamage = 0.f;
			result.changed = true;
			return result;
		}
	}

	bool TryInterceptProjectile(
		Actor& projectile,
		const sf::Vector2f& previousLocation
	)
	{
		if (projectile.GetIsPendingDestroy() || !projectile.GetWorld() ||
			!dynamic_cast<AbilityWorldActor*>(&projectile))
		{
			return false;
		}

		const sf::Vector2f currentLocation = projectile.GetActorLocation();
		const float projectileRadius = std::max(
			0.f,
			projectile.GetPhysicsCollisionRadius()
		);
		const float expandedBarrierRadius =
			AbilityData::DirectionalBarrier::BarrierRadius + projectileRadius;
		const float radiusSquared = expandedBarrierRadius * expandedBarrierRadius;

		// Runs for every projectile every frame: the callback query neither copies weak pointers nor allocates.
		bool intercepted = false;
		projectile.GetWorld()->ForEachActorInBounds(
			targeting::swept::SegmentBounds(
				previousLocation,
				currentLocation,
				expandedBarrierRadius
			),
			[&](Actor& actor)
			{
				if (intercepted)
				{
					return;
				}
				auto* ship = dynamic_cast<SpaceShip*>(&actor);
				if (!ship || ship->GetIsPendingDestroy() || ship->IsInPortalTransit() ||
					!ship->GetAbilitySystemComponent().HasOwnedTag(
						AbilityData::DirectionalBarrier::State::Active
					) ||
					!projectile.CanCollideWith(ship) ||
					!ship->CanCollideWith(&projectile) ||
					!IsFrontApproach(*ship, projectile))
				{
					return;
				}

				const sf::Vector2f center = ship->GetActorLocation();
				const sf::Vector2f previousOffset = previousLocation - center;
				const sf::Vector2f currentOffset = currentLocation - center;
				const float previousDistanceSquared = Dot(previousOffset, previousOffset);
				const float currentDistanceSquared = Dot(currentOffset, currentOffset);
				const bool crossedOuterEdge =
					previousDistanceSquared > radiusSquared &&
					targeting::swept::DistanceSquaredToSegment(
						center,
						previousLocation,
						currentLocation
					) <= radiusSquared;
				if (crossedOuterEdge ||
					(currentDistanceSquared <= radiusSquared &&
					 previousDistanceSquared > radiusSquared))
				{
					intercepted = true;
				}
			}
		);
		return intercepted;
	}

	bool RegisterDirectionalBarrierEffectBehavior()
	{
		static const bool registered = []
		{
			LightYearsEffectBehaviorRuntime::Hooks hooks;
			hooks.eventPhase = IncomingDamagePhase::PreMitigation;
			hooks.processEvent = &ProcessIncomingDamage;
			return GetEffectBehaviorRuntime().Register(
				EffectData::DirectionalBarrierBehaviorKey,
				hooks
			);
		}();
		return registered;
	}
}
