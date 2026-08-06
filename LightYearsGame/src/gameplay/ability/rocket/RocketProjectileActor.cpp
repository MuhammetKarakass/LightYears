#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/rocket/RocketProjectileActor.h"

#include "framework/World.h"
#include "gameConfigs/ability/offensive/RocketConfig.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/rocket/RocketVisualActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace ly
{
	namespace
	{
		const List<GameplayTag> RocketProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Radius,
			CommonAttributeIds::Range,
			CommonAttributeIds::CollisionRadius,
			CommonAttributeIds::Duration
		};

		const List<GameplayTag> RocketProjectileAttributeRoots{
			AbilityData::Rocket::ActorSchema::AttributeRoot,
			DamageAttributeIds::AttributeRoot
		};

		class RocketProjectileActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			const GameplayTag& GetActorTypeTag() const override
			{
				return AbilityData::Rocket::ActorSchema::TypeId;
			}

			const List<GameplayTag>& GetOwnedAttributeRoots() const override
			{
				return RocketProjectileAttributeRoots;
			}

			const List<GameplayTag>& GetAllowedCommonAttributeIds() const override
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

				for (const GameplayTag& required : {
					CommonAttributeIds::Damage,
					CommonAttributeIds::Radius,
					AbilityData::Rocket::ActorSchema::ProjectileSpeed,
					CommonAttributeIds::Range,
					CommonAttributeIds::CollisionRadius
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindGameplayAttribute(definition.attributes, required);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return {
							false,
							"Rocket projectile requires a positive '" + required.ToString() + "' attribute."
						};
					}
				}

				const float speed = sas::FindGameplayAttributeValue(
					definition.attributes,
					AbilityData::Rocket::ActorSchema::ProjectileSpeed
				);
				const float range = sas::FindGameplayAttributeValue(
					definition.attributes,
					CommonAttributeIds::Range
				);
				if (definition.lifeTime <= range / speed)
				{
					return { false, "Rocket projectile lifetime must provide cleanup time beyond maximum range." };
				}

				if (definition.presentationProfileId.empty()
					|| PresentationProfileRegistry<RocketPresentationProfile>::Find(
						definition.presentationProfileId
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
						context.definition.presentationProfileId
					);
				return world && presentationProfile
					? world->SpawnActor<RocketProjectileActor>(
						&context.owner,
						context.definition.texturePath,
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
		const std::string& texturePath,
		const RocketPresentationProfile& presentationProfile,
		std::optional<sf::Vector2f> targetLocation
	)
		: AbilityWorldActor(world, owner, texturePath),
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
			sas::FindGameplayAttributeValue(
				attributes,
				AbilityData::Rocket::ActorSchema::ProjectileSpeed,
				mProjectileSpeed
			)
		);
		mMaximumRange = std::max(
			0.f,
			sas::FindGameplayAttributeValue(attributes, CommonAttributeIds::Range, mMaximumRange)
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
			sas::FindGameplayAttributeValue(attributes, CommonAttributeIds::Radius, mExplosionRadius)
		);
		mTravelDistance = 0.f;
		mHasExploded = false;
		mLaunchVelocity = GetActorForwardDirection() * mProjectileSpeed;
		SetVelocity(mLaunchVelocity);
	}

	void RocketProjectileActor::Tick(float deltaTime)
	{
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
		if (!mHasExploded)
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
		AbilityWorldActor::OnActorBeginOverlap(otherActor);
		if (!mHasExploded && IsValidAbilityTarget(otherActor))
		{
			Explode();
		}
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
		AddActorLocationOffset(GetActorForwardDirection() * travelDistance);
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
				mPredictedImpactLocation,
				mExplosionRadius,
				travelDuration + 0.25f,
				mPresentationProfile.telegraph
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
			telegraph->SetCountdownProgress(travelProgress);
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
