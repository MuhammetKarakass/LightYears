#include "gameplay/ability/overdriveCore/OverdriveCoreProjectileActor.h"

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreContracts.h"
#include "gameplay/ability/overdriveCore/OverdriveCoreVisualActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/projectile/ProjectileSweep.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/content/ContentIdSchema.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ly
{
	namespace
	{
		constexpr float RelayDeliveryLifetimeMarginSeconds = 0.05f;

		const List<sas::AttributeId> OverdriveCoreProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Radius,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius,
			CommonAttributeIds::Duration
		};

		const List<sas::AttributeId> OverdriveCoreProjectileAttributeRoots{
			AbilityData::OverdriveCore::Actor::Projectile::Root,
			DamageAttributeIds::Root
		};

		class OverdriveCoreProjectileActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::OverdriveCoreProjectile;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return OverdriveCoreProjectileAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return OverdriveCoreProjectileCommonAttributes;
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
					AbilityData::OverdriveCore::Actor::Projectile::ProjectileSpeed,
					CommonAttributeIds::Range,
					CollisionAttributeIds::Radius
				})
				{
					const sas::GameplayAttribute* attribute =
						sas::FindAttribute(definition.attributes, required);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return {
							false,
							"Overdrive Core projectile requires a positive '" +
								std::string{ required.GetName() } + "' attribute."
						};
					}
				}

				const float speed = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::OverdriveCore::Actor::Projectile::ProjectileSpeed
				);
				const float range = sas::FindAttributeValue(
					definition.attributes,
					CommonAttributeIds::Range
				);
				if (definition.lifeTime <= range / speed)
				{
					return {
						false,
						"Overdrive Core projectile lifetime must exceed maximum travel time."
					};
				}

				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<OverdriveCorePresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return {
						false,
						"Overdrive Core projectile requires its typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const OverdriveCorePresentationProfile* profile =
					PresentationProfileRegistry<OverdriveCorePresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<OverdriveCoreProjectileActor>(
						&context.owner,
						*profile,
						context.targetLocation,
						context.targetActor
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	OverdriveCoreProjectileActor::OverdriveCoreProjectileActor(
		World* world,
		Actor* owner,
		const OverdriveCorePresentationProfile& presentationProfile,
		std::optional<sf::Vector2f> targetLocation,
		weak_ptr<Actor> targetActor
	)
		: AbilityWorldActor(world, owner, presentationProfile.texturePath),
		mPresentationProfile(presentationProfile),
		mTargetLocation(std::move(targetLocation)),
		mTargetActor(std::move(targetActor))
	{
		SetRenderLayer(RenderLayer::Projectile);
	}

	void OverdriveCoreProjectileActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		SpawnPresentation();
	}

	void OverdriveCoreProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mProjectileSpeed = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::OverdriveCore::Actor::Projectile::ProjectileSpeed,
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
			const sf::Vector2f forward = GetActorForwardDirection();
			const float length = std::sqrt(forward.x * forward.x + forward.y * forward.y);
			if (length > 0.001f)
			{
				const sf::Vector2f normalized = forward / length;
				const sf::Vector2f offset = *mTargetLocation - GetActorLocation();
				mTargetTravelDistance = std::clamp(
					offset.x * normalized.x + offset.y * normalized.y,
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

	void OverdriveCoreProjectileActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			if (const shared_ptr<OverdriveCoreVisualActor> visual = mVisualActor.lock())
			{
				visual->SetFlightVisible(false);
			}
			AbilityWorldActor::Tick(deltaTime);
			return;
		}
		if (const shared_ptr<OverdriveCoreVisualActor> visual = mVisualActor.lock())
		{
			visual->SetFlightVisible(true);
		}
		if (mHasExploded)
		{
			return;
		}

		// Auto-targeting selects an actor, not just a coordinate. Follow that
		// actor so a moving enemy cannot leave the explosion point before impact.
		if (const shared_ptr<Actor> target = mTargetActor.lock();
			target && !target->GetIsPendingDestroy())
		{
			const sf::Vector2f offset = target->GetActorLocation() - GetActorLocation();
			const float distanceToTarget = std::sqrt(
				offset.x * offset.x + offset.y * offset.y
			);
			const float travelStep = mProjectileSpeed * std::max(0.f, deltaTime);
			if (distanceToTarget <= travelStep)
			{
				SetActorLocation(target->GetActorLocation());
				mTravelDistance += distanceToTarget;
				Explode();
				return;
			}

			if (distanceToTarget > 0.001f)
			{
				const sf::Vector2f direction = offset / distanceToTarget;
				mLaunchVelocity = direction * mProjectileSpeed;
				SetActorRotation(
					std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f
				);
				SetVelocity(mLaunchVelocity);
			}
		}
		else if (!mTargetActor.expired())
		{
			// Keep the original target location as a safe fallback when the
			// selected actor is destroyed during the projectile's flight.
			mTargetActor.reset();
		}

		Move(deltaTime);
		if (mMaximumRange > 0.f &&
			mTravelDistance >= (mTargetActor.expired() ? mTargetTravelDistance : mMaximumRange))
		{
			Explode();
			return;
		}
		SynchronizePresentation();
		AbilityWorldActor::Tick(deltaTime);
	}

	void OverdriveCoreProjectileActor::Render(sf::RenderWindow& window)
	{
		if (!mHasExploded && !IsInPortalTransit())
		{
			AbilityWorldActor::Render(window);
		}
	}

	void OverdriveCoreProjectileActor::Destroy()
	{
		DestroyTelegraph();
		if (!mHasExploded)
		{
			DestroyFlightVisual();
		}
		AbilityWorldActor::Destroy();
	}

	void OverdriveCoreProjectileActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (GetIsPendingDestroy())
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

	weak_ptr<AbilityWorldActor> OverdriveCoreProjectileActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return {};
		}

		weak_ptr<OverdriveCoreProjectileActor> clone =
			world->SpawnActor<OverdriveCoreProjectileActor>(
				owner,
				mPresentationProfile,
				std::nullopt,
				request.snapshot.homingTarget
			);
		if (const shared_ptr<OverdriveCoreProjectileActor> spawned = clone.lock())
		{
			spawned->ConfigureRelayClone(request);
			spawned->ConfigureFromAttributes(request.snapshot.damageAttributes);
			spawned->ConfigureRelayClone(request);

			// Echo/Prism can recast an Overdrive projectile after its source has
			// already aged. The clone needs a fresh delivery window instead of
			// inheriting a lifetime that expires before maximum range.
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

	bool OverdriveCoreProjectileActor::BuildRelaySnapshot(
		ProjectileRelaySnapshot& snapshot
	) const
	{
		if (!AbilityWorldActor::BuildRelaySnapshot(snapshot))
		{
			return false;
		}
		snapshot.homingTarget = mTargetActor;
		return true;
	}

	void OverdriveCoreProjectileActor::Move(float deltaTime)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (mProjectileSpeed <= 0.f || safeDeltaTime <= 0.f)
		{
			return;
		}

		// A homing rocket must remain free to travel toward its live target.
		// mTargetTravelDistance is only the distance to the original snapshot
		// point; using it for homing movement makes a target that spawned at the
		// owner's position produce a zero-length path and leaves the rocket
		// permanently frozen. The live target is instead bounded by the actor's
		// configured maximum range.
		const bool hasLiveTarget = !mTargetActor.expired();
		const float travelLimit = hasLiveTarget ? mMaximumRange : mTargetTravelDistance;
		const float remainingTravel = travelLimit > 0.f
			? std::max(0.f, travelLimit - mTravelDistance)
			: mProjectileSpeed * safeDeltaTime;
		const float travelDistance = std::min(
			mProjectileSpeed * safeDeltaTime,
			remainingTravel
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

	void OverdriveCoreProjectileActor::Explode()
	{
		if (mHasExploded)
		{
			return;
		}
		mHasExploded = true;
		ApplyCombatDamageInRadius(GetActorLocation(), mExplosionRadius);
		DestroyTelegraph();
		if (const shared_ptr<OverdriveCoreVisualActor> visual = mVisualActor.lock())
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

	void OverdriveCoreProjectileActor::SpawnPresentation()
	{
		World* world = GetWorld();
		if (!world)
		{
			return;
		}
		mVisualActor = world->SpawnActor<OverdriveCoreVisualActor>(mPresentationProfile);
		const float travelDuration = mProjectileSpeed > 0.f
			? mTargetTravelDistance / mProjectileSpeed
			: 0.f;
		if (mTargetTravelDistance > 0.f && travelDuration > 0.f)
		{
			mTelegraph = world->SpawnActor<AreaTelegraphActor>(
				AreaTelegraphActor::SpawnParams{
					GetActorLocation() + GetActorForwardDirection() * mTargetTravelDistance,
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

	void OverdriveCoreProjectileActor::SynchronizePresentation()
	{
		const float progress = mTargetTravelDistance > 0.f
			? std::clamp(mTravelDistance / mTargetTravelDistance, 0.f, 1.f)
			: 0.f;
		if (const shared_ptr<OverdriveCoreVisualActor> visual = mVisualActor.lock())
		{
			visual->SetFlightState(
				GetActorLocation(),
				GetActorForwardDirection(),
				mExplosionRadius,
				progress
			);
		}
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->SetExternalProgress(progress);
		}
	}

	void OverdriveCoreProjectileActor::DestroyTelegraph()
	{
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->Destroy();
		}
		mTelegraph.reset();
	}

	void OverdriveCoreProjectileActor::DestroyFlightVisual()
	{
		if (const shared_ptr<OverdriveCoreVisualActor> visual = mVisualActor.lock())
		{
			visual->Destroy();
		}
		mVisualActor.reset();
	}

	bool RegisterOverdriveCoreProjectileActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<OverdriveCoreProjectileActorTypeHandler>()
		);
		return registered;
	}
}
