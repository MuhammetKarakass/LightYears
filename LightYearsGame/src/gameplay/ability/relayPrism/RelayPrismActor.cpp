#include "gameplay/ability/relayPrism/RelayPrismActor.h"

#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <utility>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> RelayPrismCommonAttributes{
			CommonAttributeIds::Duration,
			CommonAttributeIds::Radius,
			CommonAttributeIds::Range
		};

		const List<sas::AttributeId> RelayPrismAttributeRoots{
			AbilityData::RelayPrism::Actor::Relay::Root
		};

		class RelayPrismActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::RelayPrism;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return RelayPrismAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return RelayPrismCommonAttributes;
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

				const sas::GameplayAttribute* radius = sas::FindAttribute(
					definition.attributes,
					CommonAttributeIds::Radius
				);
				const sas::GameplayAttribute* range = sas::FindAttribute(
					definition.attributes,
					CommonAttributeIds::Range
				);
				const sas::GameplayAttribute* projectileSpeed = sas::FindAttribute(
					definition.attributes,
					AbilityData::RelayPrism::Actor::Relay::ProjectileSpeed
				);
				if (!radius || radius->baseValue <= 0.f ||
					!range || range->baseValue <= 0.f ||
					!projectileSpeed || projectileSpeed->baseValue <= 0.f ||
					definition.lifeTime <= 0.f ||
					definition.lifeTime < range->baseValue / projectileSpeed->baseValue)
				{
					return {
						false,
						"Relay Prism projectile requires positive radius, range, speed, and enough lifetime to reach range."
					};
				}

				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<RelayPrismPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return {
						false,
						"Relay Prism actor requires a valid typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const RelayPrismPresentationProfile* profile =
					PresentationProfileRegistry<RelayPrismPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<RelayPrismActor>(
						&context.owner,
						*profile,
						context.targetLocation
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};

		std::uint64_t NextCaptureVolumeId()
		{
			static std::uint64_t nextId = 0;
			return ++nextId;
		}

		bool IsInsideCaptureRadius(
			const sf::Vector2f& sourceLocation,
			const sf::Vector2f& relayLocation,
			float radius
		)
		{
			const sf::Vector2f delta = sourceLocation - relayLocation;
			return delta.x * delta.x + delta.y * delta.y <= radius * radius;
		}
	}

	RelayPrismActor::RelayPrismActor(
		World* world,
		Actor* owner,
		const RelayPrismPresentationProfile& presentationProfile,
		std::optional<sf::Vector2f> targetLocation
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile(presentationProfile),
		mGlow(std::max(1.f, presentationProfile.projectile.glowRadius), 28),
		mCore(std::max(1.f, presentationProfile.projectile.coreRadius), 8),
		mTrail(4),
		mTargetLocation(std::move(targetLocation)),
		mCaptureVolumeId(NextCaptureVolumeId())
	{
		// The Prism is a travelling projectile and a capture query volume. Movement
		// is explicit, while collision physics stays disabled so ordinary ship and
		// weapon overlaps cannot consume the Prism itself.
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetRenderLayer(RenderLayer::Projectile);
		mGlow.setOrigin({ mGlow.getRadius(), mGlow.getRadius() });
		mCore.setOrigin({ mCore.getRadius(), mCore.getRadius() });
		ConfigureVisualGeometry();
	}

	void RelayPrismActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		// A targeted Prism is still only a projectile during its flight. The
		// capture volume and its telegraph are created only after the projectile
		// reaches the resolved endpoint, matching Gravity Anomaly's projectile ->
		// field transition. Target-less actors are used by direct capture tests and
		// represent an already-open Prism.
		if (!mTargetLocation || mHasReachedTarget)
		{
			OpenCaptureVolume();
		}
	}

	void RelayPrismActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}
		Move(deltaTime);
		AbilityWorldActor::Tick(deltaTime);
		if (GetIsPendingDestroy())
		{
			return;
		}

		if (mCaptureOpened)
		{
			CaptureNearbyProjectiles();
		}
	}

	void RelayPrismActor::BeginPortalTransit()
	{
		mPortalRemainingTargetDistance = mTargetLocation && !mHasReachedTarget
			? std::max(0.f, GetVectorLength(*mTargetLocation - GetActorLocation()))
			: 0.f;
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->SetRenderEnabled(false);
		}
		AbilityWorldActor::BeginPortalTransit();
	}

	void RelayPrismActor::RebasePortalDestination(
		const sf::Vector2f& exitLocation
	)
	{
		if (mTargetLocation && !mHasReachedTarget)
		{
			// Keep the destination relative to the portal exit so a flying Prism
			// opens where it actually finishes its post-portal flight.
			mTargetLocation = portal::RebaseForwardDestination(
				exitLocation, GetActorForwardDirection(), mPortalRemainingTargetDistance
			);
		}
		mPortalRemainingTargetDistance = 0.f;
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->SetActorLocation(exitLocation);
			telegraph->SetRenderEnabled(true);
		}
	}

	void RelayPrismActor::Destroy()
	{
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->Destroy();
		}
		mTelegraph.reset();
		AbilityWorldActor::Destroy();
	}

	void RelayPrismActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		Actor::Render(window);

		const RelayPrismProjectileVisualDefinition& visual =
			mPresentationProfile.projectile;
		const float pulse = 0.86f + 0.14f * std::sin(GetAge() * visual.pulseSpeed);
		const float openedScale = mCaptureOpened ? 1.12f : 1.f;
		const sf::Vector2f location = GetActorLocation();

		mTrail.setPosition(location);
		mTrail.setRotation(sf::degrees(GetActorRotation() + 180.f));
		mTrail.setFillColor(visual.trailColor);
		mGlow.setPosition(location);
		mGlow.setFillColor(visual.glowColor);
		mGlow.setScale({ pulse * openedScale, pulse * openedScale });
		mCore.setPosition(location);
		mCore.setFillColor(visual.coreColor);
		mCore.setScale({ openedScale, openedScale });

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mTrail, additive);
		window.draw(mGlow, additive);
		window.draw(mCore, additive);
	}

	void RelayPrismActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mCaptureRadius = std::max(
			1.f,
			sas::FindAttributeValue(
				attributes,
				CommonAttributeIds::Radius,
				mCaptureRadius
			)
		);
		mProjectileSpeed = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::RelayPrism::Actor::Relay::ProjectileSpeed,
				mProjectileSpeed
			)
		);
		mMaximumRange = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				CommonAttributeIds::Range,
				mMaximumRange
			)
		);
		mTravelDistance = 0.f;
		mPortalRemainingTargetDistance = 0.f;
		mHasReachedTarget = !mTargetLocation.has_value();
		mCaptureOpened = false;
		mLaunchVelocity = {};
		if (mTargetLocation)
		{
			const sf::Vector2f offset = *mTargetLocation - GetActorLocation();
			const float distance = GetVectorLength(offset);
			if (distance > 0.001f)
			{
				const sf::Vector2f direction = offset / distance;
				const float travelDistance = std::min(distance, mMaximumRange);
				mTargetLocation = GetActorLocation() + direction * travelDistance;
				mHasReachedTarget = travelDistance <= 0.001f;
				mLaunchVelocity = direction * mProjectileSpeed;
				SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);
			}
			else
			{
				mTargetLocation = GetActorLocation();
				mHasReachedTarget = true;
			}
		}
		SetVelocity(mHasReachedTarget ? sf::Vector2f{} : mLaunchVelocity);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void RelayPrismActor::ConfigureFromAbilityValues(
		const sas::GameplayAttributeList& values
	)
	{
		mBaseProjectileCount = std::max(
			1,
			static_cast<int>(std::round(sas::FindAttributeValue(
				values,
				AbilityData::RelayPrism::Attribute::ProjectileCount,
				4.f
			)))
		);
		mDamageTransferRatio = std::clamp(
			sas::FindAttributeValue(
				values,
				AbilityData::RelayPrism::Attribute::DamageTransferRatio,
				0.15f
			),
			0.f,
			1.f
		);
		mAttackPowerCoefficient = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::RelayPrism::Attribute::AttackPowerCoefficient,
				0.25f
			)
		);
		mMinimumScatterAngle = std::clamp(
			sas::FindAttributeValue(
				values,
				AbilityData::RelayPrism::Attribute::MinimumScatterAngle,
				30.f
			),
			0.f,
			360.f
		);
		mMaximumScatterAngle = std::clamp(
			sas::FindAttributeValue(
				values,
				AbilityData::RelayPrism::Attribute::MaximumScatterAngle,
				90.f
			),
			mMinimumScatterAngle,
			360.f
		);
		mMaximumBonusProjectileCount = std::max(
			0.f,
			sas::FindAttributeValue(
				values,
				AbilityData::RelayPrism::Attribute::MaximumBonusProjectileCount,
				4.f
			)
		);
	}

	void RelayPrismActor::ConfigureVisualGeometry()
	{
		const RelayPrismProjectileVisualDefinition& visual =
			mPresentationProfile.projectile;
		mTrail.setPoint(0, { -visual.trailWidth * 0.5f, 0.f });
		mTrail.setPoint(1, { visual.trailWidth * 0.5f, 0.f });
		mTrail.setPoint(2, { visual.trailWidth * 0.20f, visual.trailLength });
		mTrail.setPoint(3, { -visual.trailWidth * 0.20f, visual.trailLength });
	}

	void RelayPrismActor::OpenCaptureVolume()
	{
		if (mCaptureOpened || GetIsPendingDestroy())
		{
			return;
		}

		mCaptureOpened = true;
		SetVelocity({});
		SpawnTelegraph();
	}

	void RelayPrismActor::SpawnTelegraph()
	{
		World* world = GetWorld();
		if (!world || GetIsPendingDestroy() || !mCaptureOpened || !mTelegraph.expired())
		{
			return;
		}

		mTelegraph = world->SpawnActor<AreaTelegraphActor>(
			AreaTelegraphActor::SpawnParams{
				GetActorLocation(),
				mCaptureRadius,
				GetLifeTime(),
				mPresentationProfile.relayTelegraph,
				AreaTelegraphAnchorMode::FixedLocation,
				AreaTelegraphProgressDriver::Timed,
				nullptr
			}
		);
	}

	void RelayPrismActor::Move(float deltaTime)
	{
		if (mHasReachedTarget || !mTargetLocation || mProjectileSpeed <= 0.f)
		{
			SetVelocity({});
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		const float remainingDistance = GetVectorLength(*mTargetLocation - GetActorLocation());
		const float travelDistance = std::min(
			mProjectileSpeed * safeDeltaTime,
			remainingDistance
		);
		if (travelDistance > 0.f)
		{
			AddActorLocationOffset(GetActorForwardDirection() * travelDistance);
			mTravelDistance += travelDistance;
		}
		SetVelocity(mLaunchVelocity);
		if (remainingDistance <= mProjectileSpeed * safeDeltaTime + 0.001f)
		{
			SetActorLocation(*mTargetLocation);
			mHasReachedTarget = true;
			OpenCaptureVolume();
		}
	}

	void RelayPrismActor::CaptureNearbyProjectiles()
	{
		World* world = GetWorld();
		if (!world)
		{
			return;
		}

		// GetActorsByType returns a snapshot of weak handles, so clones added by
		// this pass are not recursively visited during the same capture frame.
		for (const weak_ptr<AbilityWorldActor>& projectileWeak :
			world->GetActorsByType<AbilityWorldActor>())
		{
			const shared_ptr<AbilityWorldActor> projectile = projectileWeak.lock();
			if (!projectile || projectile.get() == this ||
				projectile->GetIsPendingDestroy() ||
				projectile->IsInPortalTransit() ||
				!IsInsideCaptureRadius(
					projectile->GetActorLocation(),
					GetActorLocation(),
					mCaptureRadius
				))
			{
				continue;
			}

			TryCaptureProjectile(*projectile);
		}
	}

	bool RelayPrismActor::TryCaptureProjectile(AbilityWorldActor& projectile)
	{
		if (GetIsPendingDestroy() || projectile.GetIsPendingDestroy() ||
			projectile.IsInPortalTransit() ||
			(mTargetLocation.has_value() && !mCaptureOpened) ||
			!projectile.CanBeCapturedByRelay() ||
			projectile.GetProjectileRelayLineage().HasVisited(mCaptureVolumeId) ||
			!IsInsideCaptureRadius(
				projectile.GetActorLocation(),
				GetActorLocation(),
				mCaptureRadius
			))
		{
			return false;
		}

		ProjectileRelaySnapshot snapshot;
		if (!projectile.BuildRelaySnapshot(snapshot))
		{
			return false;
		}

		float ownerAttackPower = 0.f;
		float luckFactor = 0.f;
		if (const Combatant* combatant = dynamic_cast<const Combatant*>(GetOwnerActor()))
		{
			ownerAttackPower = std::max(
				0.f,
				combatant->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
					OwnerAttributeIds::AttackPower
				)
			);
			luckFactor = std::clamp(
				combatant->GetCombatRuntime().GetCombatLuckFactor(),
				0.f,
				1.f
			);
		}

		const float transferredDamage = std::max(
			0.f,
			snapshot.damage * mDamageTransferRatio +
				ownerAttackPower * mAttackPowerCoefficient
		);
		const float bonusProjectiles = luckFactor * mMaximumBonusProjectileCount;
		const int guaranteedBonus = static_cast<int>(std::floor(bonusProjectiles));
		const float fractionalBonus = bonusProjectiles - static_cast<float>(guaranteedBonus);
		const int bonus = guaranteedBonus +
			(RandRange(0.f, 1.f) < fractionalBonus ? 1 : 0);
		const int cloneCount = std::max(1, mBaseProjectileCount + bonus);
		const ProjectileRelayLineage lineage = snapshot.lineage.Appended(mCaptureVolumeId);

		mHasScatterRotation = false;
		bool spawnedAnyClone = false;
		for (int projectileIndex = 0; projectileIndex < cloneCount; ++projectileIndex)
		{
			ProjectileRelayCloneRequest request;
			request.location = GetActorLocation();
			request.direction = MakeScatterDirection(projectileIndex);
			request.damage = transferredDamage;
			request.allowFriendlyFire = true;
			request.snapshot = snapshot;
			request.snapshot.lineage = lineage;
			const weak_ptr<AbilityWorldActor> clone = projectile.SpawnRelayClone(request);
			spawnedAnyClone = spawnedAnyClone || !clone.expired();
		}

		// Capture is transactional: a source is consumed only after at least one
		// replacement clone was created. This protects unsupported or failed
		// projectile families from disappearing inside the Prism.
		if (!spawnedAnyClone)
		{
			mHasScatterRotation = false;
			return false;
		}
		projectile.Destroy();

		if (Combatant* combatant = dynamic_cast<Combatant*>(GetOwnerActor()))
		{
			sas::AbilityEvent event;
			event.eventTag = AbilityData::RelayPrism::Event::Captured;
			event.sourceAbilityId = projectile.GetSourceAbilityId();
			event.sourceAbilityTags = projectile.GetSourceAbilityTags();
			event.SetSource(GetOwnerActor());
			event.SetTarget(&projectile);
			combatant->GetAbilitySystemComponent().HandleGameplayEvent(event);
		}
		return true;
	}

	sf::Vector2f RelayPrismActor::MakeScatterDirection(int projectileIndex)
	{
		float rotation = mLastScatterRotation;
		if (projectileIndex == 0 || !mHasScatterRotation)
		{
			rotation = RandRange(0.f, 360.f);
		}
		else
		{
			const float direction = RandRange(0.f, 1.f) < 0.5f ? -1.f : 1.f;
			rotation += direction * RandRange(
				mMinimumScatterAngle,
				mMaximumScatterAngle
			);
		}
		mLastScatterRotation = rotation;
		mHasScatterRotation = true;
		return RotationToVector(rotation);
	}

	bool RegisterRelayPrismActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<RelayPrismActorTypeHandler>()
		);
		return registered;
	}
}
