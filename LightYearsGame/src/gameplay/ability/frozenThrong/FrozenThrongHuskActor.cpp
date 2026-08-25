#include "gameplay/ability/frozenThrong/FrozenThrongHuskActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/frozenThrong/FrozenThrongContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> HuskCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Duration,
			CommonAttributeIds::Radius,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius
		};

		const List<sas::AttributeId> HuskAttributeRoots{
			AbilityData::FrozenThrong::Actor::Husk::Root,
			DamageAttributeIds::Root
		};

		class FrozenThrongHuskActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::FrozenThrongHusk;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return HuskAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return HuskCommonAttributes;
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
					CommonAttributeIds::Duration,
					CommonAttributeIds::Radius,
					CommonAttributeIds::Range,
					AbilityData::FrozenThrong::Actor::Husk::ProjectileSpeed,
					DamageAttributeIds::CryoBuildupPerHit,
					DamageAttributeIds::CryoBuildupRequired,
					DamageAttributeIds::CryoBuildupDuration,
					DamageAttributeIds::CryoSlowPercent,
					DamageAttributeIds::CryoSlowDuration
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute || !std::isfinite(attribute->baseValue) ||
						attribute->baseValue <= 0.f)
					{
						return {
							false,
							"Frozen Throng Husk requires positive movement, damage, and Cryo attributes."
						};
					}
				}

				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<FrozenThrongPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return {
						false,
						"Frozen Throng Husk requires a registered typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const FrozenThrongPresentationProfile* profile =
					PresentationProfileRegistry<FrozenThrongPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<FrozenThrongHuskActor>(
						&context.owner,
						*profile,
						context.targetLocation,
						context.targetActor
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	FrozenThrongHuskActor::FrozenThrongHuskActor(
		World* world,
		Actor* owner,
		const FrozenThrongPresentationProfile& presentationProfile,
		std::optional<sf::Vector2f> targetLocation,
		weak_ptr<Actor> targetActor
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
		, mTargetActor(std::move(targetActor))
		, mTargetLocation(std::move(targetLocation))
	{
		SetRenderLayer(RenderLayer::Projectile);
	}

	void FrozenThrongHuskActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		mLaunchDirection = GetActorForwardDirection();
		if (mTargetLocation)
		{
			const sf::Vector2f offset = *mTargetLocation - GetActorLocation();
			const float distance = GetVectorLength(offset);
			if (distance > 0.001f)
			{
				mLaunchDirection = offset / distance;
			}
			mTargetTravelDistance = distance;
		}
	}

	void FrozenThrongHuskActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mProjectileSpeed = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::FrozenThrong::Actor::Husk::ProjectileSpeed,
			mProjectileSpeed
		));
		mMaximumRange = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Range,
			mMaximumRange
		));
		mExplosionRadius = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Radius,
			mExplosionRadius
		));
		mTravelDistance = 0.f;
		mExplosionAge = 0.f;
		mHasExploded = false;
		SetVelocity(mLaunchDirection * mProjectileSpeed);
	}

	void FrozenThrongHuskActor::SetLaunchDelay(float delaySeconds)
	{
		mLaunchDelayRemaining = std::max(0.f, delaySeconds);
		// The delay is part of the delivery timeline, not a silent reduction of
		// the projectile's configured lifetime.
		SetLifeTime(GetLifeTime() + mLaunchDelayRemaining);
		if (mLaunchDelayRemaining > 0.f)
		{
			// ConfigureFromAttributes runs before this method and gives the actor
			// its normal projectile velocity. Freeze both movement and collision
			// until the delayed launch actually starts.
			SetVelocity({});
			SetEnablePhysics(false);
		}
	}

	sf::Vector2f FrozenThrongHuskActor::ResolveTargetPosition() const
	{
		if (const shared_ptr<Actor> target = mTargetActor.lock();
			target && !target->GetIsPendingDestroy())
		{
			return target->GetActorLocation();
		}
		return mTargetLocation.value_or(
			GetActorLocation() + mLaunchDirection * mMaximumRange
		);
	}

	void FrozenThrongHuskActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		if (mHasExploded)
		{
			mExplosionAge += std::max(0.f, deltaTime);
			if (mExplosionAge >= 0.18f)
			{
				Destroy();
			}
			return;
		}
		if (mLaunchDelayRemaining > 0.f)
		{
			mLaunchDelayRemaining = std::max(
				0.f,
				mLaunchDelayRemaining - std::max(0.f, deltaTime)
			);
			if (mLaunchDelayRemaining <= 0.f)
			{
				SetEnablePhysics(true);
				SetVelocity(mLaunchDirection * mProjectileSpeed);
			}
			AbilityWorldActor::Tick(deltaTime);
			if (mLaunchDelayRemaining > 0.f)
			{
				return;
			}
		}

		const sf::Vector2f targetPosition = ResolveTargetPosition();
		const sf::Vector2f offset = targetPosition - GetActorLocation();
		const float distance = GetVectorLength(offset);
		const float step = mProjectileSpeed * std::max(0.f, deltaTime);
		if (distance <= std::max(0.01f, step))
		{
			SetActorLocation(targetPosition);
			Explode();
			return;
		}
		if (distance > 0.001f)
		{
			mLaunchDirection = offset / distance;
			SetActorRotation(
				std::atan2(mLaunchDirection.y, mLaunchDirection.x) * 57.2957795131f + 90.f
			);
		}
		Move(deltaTime);
		if (!mTargetActor.lock() && mMaximumRange > 0.f &&
			mTravelDistance >= mMaximumRange)
		{
			Explode();
			return;
		}
		AbilityWorldActor::Tick(deltaTime);
	}

	void FrozenThrongHuskActor::Move(float deltaTime)
	{
		const float step = mProjectileSpeed * std::max(0.f, deltaTime);
		SetActorLocation(GetActorLocation() + mLaunchDirection * step);
		mTravelDistance += step;
		SetVelocity(mLaunchDirection * mProjectileSpeed);
	}

	void FrozenThrongHuskActor::Explode()
	{
		if (mHasExploded)
		{
			return;
		}
		mHasExploded = true;
		mExplosionAge = 0.f;
		SetVelocity({});
		SetEnablePhysics(false);
		ApplyCombatDamageInRadius(GetActorLocation(), mExplosionRadius);
	}

	void FrozenThrongHuskActor::OnActorBeginOverlap(Actor* otherActor)
	{
		// A Husk is intentionally delayed after the kill. It must not consume
		// that delay by colliding with the victim or another actor at its spawn
		// point before the projectile is released.
		if (GetIsPendingDestroy() || mHasExploded || mLaunchDelayRemaining > 0.f)
		{
			return;
		}
		if (auto* captureVolume = dynamic_cast<ProjectileCaptureVolume*>(otherActor);
			captureVolume && captureVolume->TryCaptureProjectile(*this))
		{
			return;
		}
		AbilityWorldActor::OnActorBeginOverlap(otherActor);
		if (IsValidAbilityTarget(otherActor))
		{
			Explode();
		}
	}

	void FrozenThrongHuskActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		// The launch delay is gameplay timing only. Keep the Husk rendered at
		// the recorded death location so the player can see where the proc was
		// created instead of seeing a projectile appear from empty space.
		if (mHasExploded)
		{
			const float progress = std::clamp(mExplosionAge / 0.18f, 0.f, 1.f);
			sf::CircleShape explosion(mExplosionRadius * progress);
			explosion.setOrigin({ mExplosionRadius * progress, mExplosionRadius * progress });
			explosion.setPosition(GetActorLocation());
			explosion.setFillColor(sf::Color::Transparent);
			explosion.setOutlineColor(mPresentationProfile.glowColor);
			explosion.setOutlineThickness(4.f * (1.f - progress));
			window.draw(explosion);
			return;
		}

		const float pulse = 1.f + 0.12f * std::sin(GetAge() * mPresentationProfile.pulseSpeed);
		sf::CircleShape glow(mPresentationProfile.glowRadius * pulse);
		glow.setOrigin({ mPresentationProfile.glowRadius * pulse, mPresentationProfile.glowRadius * pulse });
		glow.setPosition(GetActorLocation());
		glow.setFillColor(mPresentationProfile.glowColor);
		window.draw(glow);

		sf::CircleShape core(mPresentationProfile.coreRadius);
		core.setOrigin({ mPresentationProfile.coreRadius, mPresentationProfile.coreRadius });
		core.setPosition(GetActorLocation());
		core.setFillColor(mPresentationProfile.coreColor);
		window.draw(core);
	}

	bool RegisterFrozenThrongHuskActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<FrozenThrongHuskActorTypeHandler>()
		);
	}
}
