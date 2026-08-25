#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h"

#include "framework/World.h"
#include "gameConfigs/ability/control/GravityAnomalyConfig.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileSweep.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace ly
{
	namespace
	{
		constexpr float RelayDeliveryLifetimeMarginSeconds = 0.05f;

		const List<sas::AttributeId> ProjectileCommonAttributes{
			CommonAttributeIds::Duration,
			CommonAttributeIds::Radius,
			CommonAttributeIds::Range
		};

		const List<sas::AttributeId> GravityAnomalyAttributeRoots{
			AbilityData::GravityAnomaly::Actor::Projectile::Root,
			// The projectile profile carries field tuning forwarded to the spawned
			// field actor, so both feature-local roots are authorized here.
			AbilityData::GravityAnomaly::Actor::Field::Root
		};

		bool IsFiniteVector(const sf::Vector2f& value)
		{
			return std::isfinite(value.x) && std::isfinite(value.y);
		}

		class GravityAnomalyProjectileActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::GravityAnomalyProjectile;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return GravityAnomalyAttributeRoots;
			}

		const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return ProjectileCommonAttributes;
			}

			AbilityActorValidationResult ValidateDefinition(
				const AbilityActorDefinition& definition
			) const override
			{
				const AbilityActorValidationResult baseResult =
					AbilityActorTypeHandler::ValidateDefinition(definition);
				if (!baseResult.isValid)
				{
					return baseResult;
				}

				for (const sas::AttributeId& required : {
					AbilityData::GravityAnomaly::Actor::Projectile::ProjectileSpeed,
					CommonAttributeIds::Range,
					CommonAttributeIds::Duration,
					CommonAttributeIds::Radius,
					AbilityData::GravityAnomaly::Actor::Field::PullStrength,
					AbilityData::GravityAnomaly::Actor::Field::SlowMagnitude,
					AbilityData::GravityAnomaly::Actor::Field::InsideEffectDuration
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(definition.attributes, required);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return { false, "Gravity Anomaly projectile requires a positive '" + std::string{ required.GetName() } + "' attribute." };
					}
				}

				if (definition.spawnDistance < 0.f || !definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<GravityAnomalyProjectilePresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return { false, "Gravity Anomaly projectile requires a valid typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const GravityAnomalyProjectilePresentationProfile* profile =
					PresentationProfileRegistry<GravityAnomalyProjectilePresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<GravityAnomalyProjectileActor>(
						&context.owner,
						*profile,
						context.targetLocation
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	GravityAnomalyProjectileActor::GravityAnomalyProjectileActor(
		World* world,
		Actor* owner,
		const GravityAnomalyProjectilePresentationProfile& presentationProfile,
		std::optional<sf::Vector2f> requestedTargetLocation
	)
		: AbilityWorldActor(world, owner, presentationProfile.texturePath)
		, mPresentationProfile(presentationProfile)
		, mRequestedTargetLocation(std::move(requestedTargetLocation))
		, mGlow(std::max(1.f, presentationProfile.visual.glowRadius), 28)
		, mCore(std::max(1.f, presentationProfile.visual.coreRadius), 24)
		, mTrail(4)
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		mGlow.setOrigin({ mGlow.getRadius(), mGlow.getRadius() });
		mCore.setOrigin({ mCore.getRadius(), mCore.getRadius() });
	}

	void GravityAnomalyProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mProjectileSpeed = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::Actor::Projectile::ProjectileSpeed,
			mProjectileSpeed
		));
		mCastRange = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Range,
			mCastRange
		));
		mFieldDuration = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Duration,
			mFieldDuration
		));
		mFieldRadius = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Radius,
			mFieldRadius
		));
		mPullStrength = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::Actor::Field::PullStrength,
			mPullStrength
		));
		mSlowMagnitude = std::clamp(sas::FindAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::Actor::Field::SlowMagnitude,
			mSlowMagnitude
		), 0.f, 0.95f);
		mInsideEffectDuration = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::Actor::Field::InsideEffectDuration,
			mInsideEffectDuration
		));
		mFieldAttributes = {
			sas::GameplayAttribute{ CommonAttributeIds::Duration, mFieldDuration, 0.01f },
			sas::GameplayAttribute{ CommonAttributeIds::Radius, mFieldRadius, 0.01f },
		sas::GameplayAttribute{ AbilityData::GravityAnomaly::Actor::Field::PullStrength, mPullStrength, 0.f },
		sas::GameplayAttribute{ AbilityData::GravityAnomaly::Actor::Field::SlowMagnitude, mSlowMagnitude, 0.f, 0.95f },
		sas::GameplayAttribute{ AbilityData::GravityAnomaly::Actor::Field::InsideEffectDuration, mInsideEffectDuration, 0.f }
		};
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		ResolveTargetLocation();
		mTravelDistance = 0.f;
		mHasSpawnedField = false;
		ConfigureVisualGeometry();
	}

	void GravityAnomalyProjectileActor::ResolveTargetLocation()
	{
		Actor* owner = GetOwnerActor();
		const sf::Vector2f sourceLocation = mRelayLaunchDirection
			? GetActorLocation()
			: (owner ? owner->GetActorLocation() : GetActorLocation());
		sf::Vector2f direction = owner ? owner->GetActorForwardDirection() : GetActorForwardDirection();
		if (!IsFiniteVector(direction) || direction.x * direction.x + direction.y * direction.y <= 0.000001f)
		{
			direction = { 0.f, -1.f };
		}

		if (mRelayLaunchDirection && IsFiniteVector(*mRelayLaunchDirection))
		{
			direction = *mRelayLaunchDirection;
			const float directionLength = std::sqrt(
				direction.x * direction.x + direction.y * direction.y
			);
			if (directionLength > 0.001f)
			{
				direction /= directionLength;
			}
			mResolvedTargetLocation = sourceLocation + direction * mCastRange;
		}
		else if (mRequestedTargetLocation && IsFiniteVector(*mRequestedTargetLocation))
		{
			const sf::Vector2f aimDelta = *mRequestedTargetLocation - sourceLocation;
			const float aimDistanceSquared = aimDelta.x * aimDelta.x + aimDelta.y * aimDelta.y;
			if (std::isfinite(aimDistanceSquared) && aimDistanceSquared > 0.000001f)
			{
				const float aimDistance = std::sqrt(aimDistanceSquared);
				direction = aimDelta / aimDistance;
				mResolvedTargetLocation = sourceLocation + direction * std::min(aimDistance, mCastRange);
			}
			else
			{
				mResolvedTargetLocation = sourceLocation + direction * mCastRange;
			}
		}
		else
		{
			mResolvedTargetLocation = sourceLocation + direction * mCastRange;
		}

		const sf::Vector2f flightDelta = mResolvedTargetLocation - GetActorLocation();
		const float flightDistanceSquared = flightDelta.x * flightDelta.x + flightDelta.y * flightDelta.y;
		if (std::isfinite(flightDistanceSquared) && flightDistanceSquared > 0.000001f)
		{
			mTargetTravelDistance = std::sqrt(flightDistanceSquared);
			mFlightDirection = flightDelta / mTargetTravelDistance;
		}
		else
		{
			mTargetTravelDistance = 0.f;
			mFlightDirection = direction;
		}

		const float rotation = std::atan2(mFlightDirection.y, mFlightDirection.x) * 57.2957795131f + 90.f;
		SetActorRotation(rotation);
		SetVelocity(mFlightDirection * mProjectileSpeed);
	}

	void GravityAnomalyProjectileActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}
		if (mHasSpawnedField)
		{
			return;
		}

		if (MoveTowardTarget(deltaTime))
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}
		if (mTravelDistance >= mTargetTravelDistance)
		{
			SpawnField();
			Destroy();
			return;
		}
		AbilityWorldActor::Tick(deltaTime);
	}

	void GravityAnomalyProjectileActor::RebasePortalDestination(
		const sf::Vector2f& exitLocation
	)
	{
		const float remainingDistance = std::max(
			0.f,
			mTargetTravelDistance - mTravelDistance
		);
		if (!mHasSpawnedField)
		{
			// A portal relocates the delivery projectile, not an already-selected
			// field. Preserve its remaining travel budget from the new exit point.
			mResolvedTargetLocation = portal::RebaseForwardDestination(
				exitLocation, mFlightDirection, remainingDistance
			);
		}
	}

	bool GravityAnomalyProjectileActor::MoveTowardTarget(float deltaTime)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (mProjectileSpeed <= 0.f || safeDeltaTime <= 0.f || mTargetTravelDistance <= 0.f)
		{
			return false;
		}
		const float remainingDistance = std::max(0.f, mTargetTravelDistance - mTravelDistance);
		const float stepDistance = std::min(mProjectileSpeed * safeDeltaTime, remainingDistance);
		const sf::Vector2f startLocation = GetActorLocation();
		const sf::Vector2f endLocation = startLocation + mFlightDirection * stepDistance;
		SetVelocity(mFlightDirection * mProjectileSpeed);
		for (const projectile::SweptContact& contact : projectile::FindSweptContacts(
			*this,
			startLocation,
			endLocation,
			GetPhysicsCollisionRadius()
		))
		{
			if (!contact.hasSurfaceNormal)
			{
				continue;
			}
			SetActorLocation(contact.impactLocation);
			mTravelDistance += stepDistance * contact.fraction;
			if (ProjectileReflectionService::TryReflectFromSurface(
				*this,
				*contact.actor,
				{ contact.impactLocation, contact.surfaceNormal }
			))
			{
				return true;
			}
			// A delivery projectile that cannot reflect from a static world object
			// must not pass through it and create its field on the far side.
			Destroy();
			return true;
		}
		SetActorLocation(endLocation);
		mTravelDistance += stepDistance;
		return ProjectileReflectionService::TryReflectProjectileAlongPath(
			*this,
			startLocation,
			GetActorLocation()
		);
	}

	void GravityAnomalyProjectileActor::SpawnField()
	{
		if (mHasSpawnedField)
		{
			return;
		}
		mHasSpawnedField = true;

		Actor* owner = GetOwnerActor();
		const AbilityActorDefinition* definition = AbilityData::FindAbilityActorDefinition(
			std::string{ AbilityData::GravityAnomaly::Actor::Field::BasicDefinitionId }
		);
		if (!owner || !definition)
		{
			return;
		}
		weak_ptr<AbilityWorldActor> spawnedField = AbilityActorRegistry::Spawn(
			AbilityActorSpawnContext{ *owner, *definition, mFieldAttributes, mResolvedTargetLocation }
		);
		if (const shared_ptr<AbilityWorldActor> field = spawnedField.lock())
		{
			field->SetActorLocation(mResolvedTargetLocation);
			field->SetActorRotation(GetActorRotation());
			field->SetLifeTime(mFieldDuration);
			field->SetDamage(0.f);
			field->SetDamageTags({});
			field->SetAbilityCollisionRadius(0.f);
			field->ConfigureFromAttributes(mFieldAttributes);
		}
	}

	weak_ptr<AbilityWorldActor> GravityAnomalyProjectileActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return {};
		}

		weak_ptr<GravityAnomalyProjectileActor> clone =
			world->SpawnActor<GravityAnomalyProjectileActor>(
				owner,
				mPresentationProfile,
				std::nullopt
			);
		if (const shared_ptr<GravityAnomalyProjectileActor> spawned = clone.lock())
		{
			spawned->SetRelayLaunchDirection(request.direction);
			spawned->ConfigureRelayClone(request);
			spawned->ConfigureFromAttributes(request.snapshot.damageAttributes);
			spawned->ConfigureRelayClone(request);

			// The source snapshot may have only a fraction of its original
			// lifetime left. A relay clone starts a new delivery, so it must be
			// allowed to reach the cast point and spawn its field first.
			const float requiredDeliveryLifetime = spawned->mProjectileSpeed > 0.f
				? spawned->mCastRange / spawned->mProjectileSpeed +
					RelayDeliveryLifetimeMarginSeconds
				: 0.f;
			spawned->SetLifeTime(std::max(
				spawned->GetLifeTime(),
				requiredDeliveryLifetime
			));
		}
		return clone;
	}

	bool GravityAnomalyProjectileActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		if (!CanBeReflected() || mHasSpawnedField ||
			(GetOwnerActor() == &request.newOwner && !request.allowSameOwnerReflection))
		{
			return false;
		}

		const float directionLength = GetVectorLength(request.returnDirection);
		if (directionLength <= 0.001f)
		{
			return false;
		}
		const sf::Vector2f direction = request.returnDirection / directionLength;
		const float remainingDistance = std::max(
			0.f,
			mTargetTravelDistance - mTravelDistance
		);
		ApplyReflectionOwnership(request.newOwner, request.damageMultiplier);
		// The delivery retains payload and remaining travel, but does not gain
		// homing: this selects one straight return direction at reflection time.
		mRequestedTargetLocation.reset();
		mRelayLaunchDirection = direction;
		mFlightDirection = direction;
		mResolvedTargetLocation = GetActorLocation() + direction * remainingDistance;
		mTargetTravelDistance = mTravelDistance + remainingDistance;
		SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);
		SetVelocity(direction * mProjectileSpeed);
		return true;
	}

	void GravityAnomalyProjectileActor::ConfigureVisualGeometry()
	{
		const GravityAnomalyProjectileVisualDefinition& visual = mPresentationProfile.visual;
		mTrail.setPoint(0, { -visual.trailWidth * 0.5f, 0.f });
		mTrail.setPoint(1, { visual.trailWidth * 0.5f, 0.f });
		mTrail.setPoint(2, { visual.trailWidth * 0.20f, visual.trailLength });
		mTrail.setPoint(3, { -visual.trailWidth * 0.20f, visual.trailLength });
	}

	void GravityAnomalyProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		Actor::Render(window);
		const GravityAnomalyProjectileVisualDefinition& visual = mPresentationProfile.visual;
		const float pulse = 0.85f + 0.15f * std::sin(GetAge() * visual.pulseSpeed);
		mTrail.setPosition(GetActorLocation());
		mTrail.setRotation(sf::degrees(GetActorRotation() + 180.f));
		mTrail.setFillColor(visual.trailColor);
		mGlow.setPosition(GetActorLocation());
		mGlow.setFillColor(visual.glowColor);
		mGlow.setScale({ pulse, pulse });
		mCore.setPosition(GetActorLocation());
		mCore.setFillColor(visual.coreColor);
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mTrail, additive);
		window.draw(mGlow, additive);
		window.draw(mCore, additive);
	}

	bool RegisterGravityAnomalyProjectileActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<GravityAnomalyProjectileActorTypeHandler>()
		);
		return registered;
	}
}
