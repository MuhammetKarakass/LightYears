#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"

#include "framework/World.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/rocket/RocketVisualActor.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileSweep.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace ly
{
	namespace
	{
		// A relay clone starts from the Prism instead of the source projectile's
		// original position. It therefore needs enough lifetime for a complete
		// delivery to its own maximum range, plus a small scheduling margin so a
		// frame-boundary lifetime check cannot destroy it just before the range
		// check reaches the endpoint.
		constexpr float RelayDeliveryLifetimeMarginSeconds = 0.05f;

		sf::Vector2f NormalizeOrDefault(
			const sf::Vector2f& direction,
			const sf::Vector2f& fallback
		)
		{
			const float length = GetVectorLength(direction);
			return length > 0.001f ? direction / length : fallback;
		}

		const List<sas::AttributeId> RocketProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Radius,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius,
			CommonAttributeIds::Duration
		};

		const List<sas::AttributeId> RocketProjectileAttributeRoots{
			AbilityData::Rocket::Actor::Projectile::Root,
			DamageAttributeIds::Root
		};

		class RocketProjectileActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::RocketProjectile;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return RocketProjectileAttributeRoots;
			}

		const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return RocketProjectileCommonAttributes;
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
					CommonAttributeIds::Damage,
					CommonAttributeIds::Radius,
					AbilityData::Rocket::Actor::Projectile::ProjectileSpeed,
					CommonAttributeIds::Range,
					CollisionAttributeIds::Radius
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(definition.attributes, required);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return {
							false,
							"Rocket projectile requires a positive '" + std::string{ required.GetName() } + "' attribute."
						};
					}
				}

				const float speed = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::Rocket::Actor::Projectile::ProjectileSpeed
				);
				const float range = sas::FindAttributeValue(
					definition.attributes,
					CommonAttributeIds::Range
				);
				if (definition.lifeTime <= range / speed)
				{
					return { false, "Rocket projectile lifetime must provide cleanup time beyond maximum range." };
				}

				if (!definition.presentationProfileId.IsValid()
					|| PresentationProfileRegistry<RocketPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return { false, "Rocket projectile requires a valid presentation profile." };
				}

				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const RocketPresentationProfile* presentationProfile =
					PresentationProfileRegistry<RocketPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && presentationProfile
					? world->SpawnActor<RocketProjectileActor>(
						&context.owner,
						*presentationProfile,
						context.targetLocation
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	RocketProjectileActor::RocketProjectileActor(
		World* world,
		Actor* owner,
		const RocketPresentationProfile& presentationProfile,
		std::optional<sf::Vector2f> targetLocation
	)
		: AbilityWorldActor(world, owner, presentationProfile.texturePath),
		mPresentationProfile(presentationProfile),
		mTargetLocation(std::move(targetLocation))
	{
		SetRenderLayer(RenderLayer::Projectile);
	}

	void RocketProjectileActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		SpawnPresentation();
	}

	void RocketProjectileActor::ConfigureFromAttributes(const sas::GameplayAttributeList& attributes)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);

		mProjectileSpeed = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::Rocket::Actor::Projectile::ProjectileSpeed,
				mProjectileSpeed
			)
		);
		mMaximumRange = std::max(
			0.f,
			sas::FindAttributeValue(attributes, CommonAttributeIds::Range, mMaximumRange)
		);
		mTargetTravelDistance = mMaximumRange;
		if (mTargetLocation)
		{
			sf::Vector2f forward = GetActorForwardDirection();
			const float forwardLength = std::sqrt(
				forward.x * forward.x + forward.y * forward.y
			);
			if (forwardLength > 0.001f)
			{
				forward /= forwardLength;
				const sf::Vector2f targetOffset = *mTargetLocation - GetActorLocation();
				const float projectedTargetDistance =
					targetOffset.x * forward.x + targetOffset.y * forward.y;
				mTargetTravelDistance = std::clamp(
					projectedTargetDistance,
					0.f,
					mMaximumRange
				);
			}
		}
		mExplosionRadius = std::max(
			0.f,
			sas::FindAttributeValue(attributes, CommonAttributeIds::Radius, mExplosionRadius)
		);
		mTravelDistance = 0.f;
		mHasExploded = false;
		mLaunchVelocity = GetActorForwardDirection() * mProjectileSpeed;
		SetVelocity(mLaunchVelocity);
	}

	void RocketProjectileActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			if (const shared_ptr<RocketVisualActor> visual = mVisualActor.lock())
			{
				visual->SetFlightVisible(false);
			}
			AbilityWorldActor::Tick(deltaTime);
			return;
		}
		if (const shared_ptr<RocketVisualActor> visual = mVisualActor.lock())
		{
			visual->SetFlightVisible(true);
		}
		if (mHasExploded)
		{
			return;
		}

		Move(deltaTime);
		if (mMaximumRange > 0.f && mTravelDistance >= mTargetTravelDistance)
		{
			Explode();
			return;
		}

		SynchronizePresentation();
		AbilityWorldActor::Tick(deltaTime);
	}

	void RocketProjectileActor::Render(sf::RenderWindow& window)
	{
		if (!mHasExploded && !IsInPortalTransit())
		{
			AbilityWorldActor::Render(window);
		}
	}

	void RocketProjectileActor::Destroy()
	{
		DestroyTelegraph();
		if (!mHasExploded)
		{
			DestroyFlightVisual();
		}
		AbilityWorldActor::Destroy();
	}

	void RocketProjectileActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		if (TryReflectOnOverlap(otherActor))
		{
			return;
		}
		if (auto* captureVolume = dynamic_cast<ProjectileCaptureVolume*>(otherActor);
			captureVolume && captureVolume->TryCaptureProjectile(*this))
		{
			return;
		}
		AbilityWorldActor::OnActorBeginOverlap(otherActor);
		if (!mHasExploded && IsValidAbilityTarget(otherActor))
		{
			Explode();
		}
	}

	weak_ptr<AbilityWorldActor> RocketProjectileActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return {};
		}

		weak_ptr<RocketProjectileActor> clone =
			world->SpawnActor<RocketProjectileActor>(
				owner,
				mPresentationProfile,
				std::nullopt
			);
		if (const shared_ptr<RocketProjectileActor> spawned = clone.lock())
		{
			// Set the requested direction before the typed configuration so the
			// rocket's launch velocity and presentation target use that direction.
			spawned->ConfigureRelayClone(request);
			spawned->ConfigureFromAttributes(request.snapshot.damageAttributes);
			spawned->ConfigureRelayClone(request);

			// The source snapshot's remaining lifetime is valid for the original
			// projectile, but not necessarily for a clone that starts travelling
			// again from the Prism. Never let that inherited value expire before
			// this Rocket can cover its configured maximum range.
			const float requiredDeliveryLifetime = spawned->mProjectileSpeed > 0.f
				? spawned->mMaximumRange / spawned->mProjectileSpeed +
					RelayDeliveryLifetimeMarginSeconds
				: 0.f;
			spawned->SetLifeTime(std::max(
				spawned->GetLifeTime(),
				requiredDeliveryLifetime
			));
		}
		return clone;
	}

	bool RocketProjectileActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		if (!CanBeReflected() || mHasExploded ||
			(GetOwnerActor() == &request.newOwner && !request.allowSameOwnerReflection) ||
			GetVectorLength(request.returnDirection) <= 0.001f)
		{
			return false;
		}

		const sf::Vector2f direction = NormalizeOrDefault(
			request.returnDirection,
			GetActorForwardDirection()
		);
		ApplyReflectionOwnership(request.newOwner, request.damageMultiplier);
		// mTravelDistance intentionally remains unchanged: the rocket only gets
		// the range it had left when it reached the defender.
		mTargetLocation.reset();
		mTargetTravelDistance = mMaximumRange;
		mLaunchVelocity = direction * mProjectileSpeed;
		SetVelocity(mLaunchVelocity);
		SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);
		return true;
	}

	void RocketProjectileActor::Move(float deltaTime)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (mProjectileSpeed <= 0.f || safeDeltaTime <= 0.f)
		{
			return;
		}

		float travelDistance = mProjectileSpeed * safeDeltaTime;
		travelDistance = std::min(
			travelDistance,
			std::max(0.f, mTargetTravelDistance - mTravelDistance)
		);

		SetVelocity(mLaunchVelocity);
		const sf::Vector2f startLocation = GetActorLocation();
		const sf::Vector2f endLocation =
			startLocation + GetActorForwardDirection() * travelDistance;
		for (const projectile::SweptContact& contact :
			projectile::FindSweptContacts(
				*this,
				startLocation,
				endLocation,
				GetPhysicsCollisionRadius()
			))
		{
			SetActorLocation(
				startLocation + (endLocation - startLocation) * contact.fraction
			);
			if (contact.hasSurfaceNormal &&
				ProjectileReflectionService::TryReflectFromSurface(
					*this,
					*contact.actor,
					{ contact.impactLocation, contact.surfaceNormal }
				))
			{
				mTravelDistance += travelDistance * contact.fraction;
				return;
			}
			OnActorBeginOverlap(contact.actor);
			if (mHasExploded || GetIsPendingDestroy() || IsInPortalTransit())
			{
				mTravelDistance += travelDistance * contact.fraction;
				return;
			}
		}
		SetActorLocation(endLocation);
		mTravelDistance += travelDistance;
	}

	void RocketProjectileActor::Explode()
	{
		if (mHasExploded)
		{
			return;
		}

		mHasExploded = true;
		ApplyCombatDamageInRadius(GetActorLocation(), mExplosionRadius);
		DestroyTelegraph();
		if (const shared_ptr<RocketVisualActor> visual = mVisualActor.lock())
		{
			visual->SetFlightState(
				GetActorLocation(),
				GetActorForwardDirection(),
				mExplosionRadius,
				1.f
			);
			visual->BeginImpact(GetActorLocation(), mExplosionRadius);
		}
		mVisualActor.reset();
		Destroy();
	}

	void RocketProjectileActor::SpawnPresentation()
	{
		World* world = GetWorld();
		if (world == nullptr)
		{
			return;
		}

		mVisualActor = world->SpawnActor<RocketVisualActor>(mPresentationProfile);

		const float travelDuration = mProjectileSpeed > 0.f
			? mTargetTravelDistance / mProjectileSpeed
			: 0.f;
		if (mTargetTravelDistance > 0.f && travelDuration > 0.f)
		{
			mPredictedImpactLocation =
				GetActorLocation() + GetActorForwardDirection() * mTargetTravelDistance;
			mTelegraph = world->SpawnActor<AreaTelegraphActor>(
				AreaTelegraphActor::SpawnParams{
					mPredictedImpactLocation,
					mExplosionRadius,
					0.f,
					mPresentationProfile.telegraph,
					AreaTelegraphAnchorMode::FixedLocation,
					AreaTelegraphProgressDriver::External
				}
			);
		}

		SynchronizePresentation();
	}

	void RocketProjectileActor::SynchronizePresentation()
	{
		const float travelProgress = mTargetTravelDistance > 0.f
			? std::clamp(mTravelDistance / mTargetTravelDistance, 0.f, 1.f)
			: 0.f;

		if (const shared_ptr<RocketVisualActor> visual = mVisualActor.lock())
		{
			visual->SetFlightState(
				GetActorLocation(),
				GetActorForwardDirection(),
				mExplosionRadius,
				travelProgress
			);
		}
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->SetExternalProgress(travelProgress);
		}
	}

	void RocketProjectileActor::DestroyTelegraph()
	{
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->Destroy();
		}
		mTelegraph.reset();
	}

	void RocketProjectileActor::DestroyFlightVisual()
	{
		if (const shared_ptr<RocketVisualActor> visual = mVisualActor.lock())
		{
			visual->Destroy();
		}
		mVisualActor.reset();
	}

	bool RegisterRocketProjectileActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<RocketProjectileActorTypeHandler>()
		);
		return registered;
	}
}
