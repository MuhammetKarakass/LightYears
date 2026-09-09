#include "gameplay/ability/solarBombardment/SolarBombardmentProjectileActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/solarBombardment/SolarBombardmentContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>

namespace ly
{
	namespace
	{
		constexpr float DirectionEpsilonSquared = 0.000001f;
		constexpr float FirstTravelDistance = 400.f;
		constexpr float DegreesPerRadian = 57.2957795131f;

		const List<sas::AttributeId> ProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Radius,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius
		};

		const List<sas::AttributeId> ProjectileAttributeRoots{
			AbilityData::SolarBombardment::Actor::Projectile::Root,
			DamageAttributeIds::Root
		};

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

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * std::clamp(multiplier, 0.f, 1.f),
				0.f,
				255.f
			));
			return result;
		}

		sf::Vector2f NormalizeOrDefault(
			const sf::Vector2f& value,
			const sf::Vector2f& fallback
		)
		{
			const float length = GetVectorLength(value);
			return length > 0.001f ? value / length : fallback;
		}

		void ConfigureCircle(sf::CircleShape& shape, float radius)
		{
			const float safeRadius = std::max(1.f, radius);
			shape.setRadius(safeRadius);
			shape.setOrigin({ safeRadius, safeRadius });
		}

		class SolarBombardmentProjectileActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::SolarBombardmentProjectile;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return ProjectileAttributeRoots;
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
					CommonAttributeIds::Damage,
					CommonAttributeIds::Radius,
					CommonAttributeIds::Range,
					CollisionAttributeIds::Radius,
					AbilityData::SolarBombardment::Attribute::InnerRadius,
					AbilityData::SolarBombardment::Attribute::InnerDamageMultiplier,
					AbilityData::SolarBombardment::Attribute::InnerIgniteStacks,
					AbilityData::SolarBombardment::Attribute::OuterIgniteStacks,
					AbilityData::SolarBombardment::Attribute::MinTravelTime,
					AbilityData::SolarBombardment::Attribute::MaxTravelTime,
					DamageAttributeIds::BurnDamagePerSecond,
					DamageAttributeIds::BurnDuration,
					DamageAttributeIds::BurnMaxStacks
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute || !std::isfinite(attribute->baseValue))
					{
						return {
							false,
							"Solar Bombardment projectile is missing required attribute '" +
								std::string{ required.GetName() } + "'."
						};
					}
				}

				const float outerRadius = sas::FindAttributeValue(
					definition.attributes, CommonAttributeIds::Radius, 0.f
				);
				const float innerRadius = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::SolarBombardment::Attribute::InnerRadius,
					0.f
				);
				const float minTravel = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::SolarBombardment::Attribute::MinTravelTime,
					0.f
				);
				const float maxTravel = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::SolarBombardment::Attribute::MaxTravelTime,
					0.f
				);
				if (outerRadius <= innerRadius || innerRadius <= 0.f ||
					minTravel <= 0.f || maxTravel < minTravel ||
					definition.lifeTime <= maxTravel ||
					!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<
						SolarBombardmentPresentationProfile
					>::Find(definition.presentationProfileId.ToString()) == nullptr)
				{
					return {
						false,
						"Solar Bombardment projectile requires valid two-zone travel, lifetime, and presentation values."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const SolarBombardmentPresentationProfile* profile =
					PresentationProfileRegistry<
						SolarBombardmentPresentationProfile
					>::Find(context.definition.presentationProfileId.ToString());
				return world && profile
					? world->SpawnActor<SolarBombardmentProjectileActor>(
						&context.owner,
						*profile,
						context.targetLocation
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	SolarBombardmentProjectileActor::SolarBombardmentProjectileActor(
		World* world,
		Actor* owner,
		const SolarBombardmentPresentationProfile& presentationProfile,
		std::optional<sf::Vector2f> requestedTargetLocation
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile(presentationProfile),
		mRequestedTargetLocation(std::move(requestedTargetLocation)),
		mOwnerReference(MakeWeakActor(owner)),
		mProjectileGlow(1.f, 32),
		mProjectileCore(1.f, 24),
		mExplosionOuter(1.f, 64),
		mExplosionInner(1.f, 64),
		mExplosionShockwave(1.f, 64),
		mTrail(sf::PrimitiveType::TriangleStrip, 4)
	{
		SetRenderLayer(RenderLayer::Projectile);
		// The projectile is a delivery marker; it must never damage or block via
		// contact while travelling. Damage is resolved once at the target point.
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		ConfigureVisualGeometry();
	}

	void SolarBombardmentProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mCastRange = std::max(0.f, sas::FindAttributeValue(
			attributes, CommonAttributeIds::Range, mCastRange
		));
		mOuterRadius = std::max(0.f, sas::FindAttributeValue(
			attributes, CommonAttributeIds::Radius, mOuterRadius
		));
		mInnerRadius = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::SolarBombardment::Attribute::InnerRadius,
			mInnerRadius
		));
		mInnerDamageMultiplier = std::max(1.f, sas::FindAttributeValue(
			attributes,
			AbilityData::SolarBombardment::Attribute::InnerDamageMultiplier,
			mInnerDamageMultiplier
		));
		mInnerIgniteStacks = std::max(0, static_cast<int>(std::lround(
			sas::FindAttributeValue(
				attributes,
				AbilityData::SolarBombardment::Attribute::InnerIgniteStacks,
				static_cast<float>(mInnerIgniteStacks)
			)
		)));
		mOuterIgniteStacks = std::max(0, static_cast<int>(std::lround(
			sas::FindAttributeValue(
				attributes,
				AbilityData::SolarBombardment::Attribute::OuterIgniteStacks,
				static_cast<float>(mOuterIgniteStacks)
			)
		)));
		mMinTravelTime = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::SolarBombardment::Attribute::MinTravelTime,
			mMinTravelTime
		));
		mMaxTravelTime = std::max(mMinTravelTime, sas::FindAttributeValue(
			attributes,
			AbilityData::SolarBombardment::Attribute::MaxTravelTime,
			mMaxTravelTime
		));

		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		ResolveTargetLocation();
		mStartLocation = GetActorLocation();
		mTargetDistance = GetVectorLength(mResolvedTargetLocation - mStartLocation);
		const float normalizedDistance = std::clamp(
			(mTargetDistance - FirstTravelDistance) /
				std::max(1.f, mCastRange - FirstTravelDistance),
			0.f,
			1.f
		);
		mTravelDuration = mMinTravelTime +
			(mMaxTravelTime - mMinTravelTime) * normalizedDistance;
		mTravelDuration = std::max(0.01f, mTravelDuration);
		mTravelAge = 0.f;
		mExplosionAge = 0.f;
		mVisualAge = 0.f;
		mDetonated = false;
		mDamageApplied = false;
		SpawnTelegraphs();
	}

	void SolarBombardmentProjectileActor::ResolveTargetLocation()
	{
		const shared_ptr<Actor> owner = mOwnerReference.lock();
		const sf::Vector2f sourceLocation = GetActorLocation();
		sf::Vector2f direction = owner
			? owner->GetActorForwardDirection()
			: GetActorForwardDirection();
		if (!std::isfinite(direction.x) || !std::isfinite(direction.y) ||
			direction.x * direction.x + direction.y * direction.y <= DirectionEpsilonSquared)
		{
			direction = { 0.f, -1.f };
		}

		if (mRequestedTargetLocation)
		{
			const sf::Vector2f delta = *mRequestedTargetLocation - sourceLocation;
			const float distance = GetVectorLength(delta);
			if (std::isfinite(distance) && distance > 0.001f)
			{
				direction = delta / distance;
				mResolvedTargetLocation = sourceLocation + direction *
					std::min(distance, mCastRange);
			}
			else
			{
				mResolvedTargetLocation = sourceLocation;
			}
		}
		else
		{
			NormalizeVector(direction);
			mResolvedTargetLocation = sourceLocation + direction * mCastRange;
		}

		mFlightDirection = NormalizeOrDefault(
			mResolvedTargetLocation - sourceLocation,
			direction
		);
		SetActorRotation(std::atan2(
			mFlightDirection.y,
			mFlightDirection.x
		) * DegreesPerRadian + 90.f);
	}

	void SolarBombardmentProjectileActor::SpawnTelegraphs()
	{
		World* world = GetWorld();
		if (!world || mOuterRadius <= 0.f || mInnerRadius <= 0.f)
		{
			return;
		}

		mOuterTelegraph = world->SpawnActor<AreaTelegraphActor>(
			AreaTelegraphActor::SpawnParams{
				mResolvedTargetLocation,
				mOuterRadius,
				mTravelDuration,
				mPresentationProfile.outerTelegraph,
				AreaTelegraphAnchorMode::FixedLocation,
				AreaTelegraphProgressDriver::External,
				nullptr
			}
		);
		mInnerTelegraph = world->SpawnActor<AreaTelegraphActor>(
			AreaTelegraphActor::SpawnParams{
				mResolvedTargetLocation,
				mInnerRadius,
				mTravelDuration,
				mPresentationProfile.innerTelegraph,
				AreaTelegraphAnchorMode::FixedLocation,
				AreaTelegraphProgressDriver::External,
				nullptr
			}
		);
		UpdateTelegraphProgress(0.f);
	}

	void SolarBombardmentProjectileActor::UpdateTelegraphProgress(
		float normalizedProgress
	)
	{
		if (const shared_ptr<AreaTelegraphActor> telegraph = mOuterTelegraph.lock())
		{
			telegraph->SetExternalProgress(normalizedProgress);
		}
		if (const shared_ptr<AreaTelegraphActor> telegraph = mInnerTelegraph.lock())
		{
			telegraph->SetExternalProgress(normalizedProgress);
		}
	}

	void SolarBombardmentProjectileActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mVisualAge += safeDeltaTime;
		if (mDetonated)
		{
			mExplosionAge += safeDeltaTime;
			if (mExplosionAge >= mPresentationProfile.explosion.duration)
			{
				Destroy();
			}
			return;
		}

		mTravelAge += safeDeltaTime;
		const float progress = std::clamp(
			mTravelAge / std::max(0.01f, mTravelDuration),
			0.f,
			1.f
		);
		SetActorLocation(mStartLocation +
			(mResolvedTargetLocation - mStartLocation) * progress);
		UpdateTelegraphProgress(progress);
		if (progress >= 1.f)
		{
			Detonate();
			return;
		}

		AbilityWorldActor::Tick(deltaTime);
	}

	void SolarBombardmentProjectileActor::Detonate()
	{
		if (mDetonated)
		{
			return;
		}
		mDetonated = true;
		mExplosionAge = 0.f;
		SetActorLocation(mResolvedTargetLocation);
		if (const shared_ptr<AreaTelegraphActor> telegraph = mOuterTelegraph.lock())
		{
			telegraph->Complete(1.f);
		}
		if (const shared_ptr<AreaTelegraphActor> telegraph = mInnerTelegraph.lock())
		{
			telegraph->Complete(1.f);
		}
		ApplyExplosionDamage();
	}

	void SolarBombardmentProjectileActor::ApplyExplosionDamage()
	{
		if (mDamageApplied)
		{
			return;
		}
		mDamageApplied = true;
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner || owner->GetIsPendingDestroy() || mOuterRadius <= 0.f)
		{
			return;
		}

		const float outerRadiusSquared = mOuterRadius * mOuterRadius;
		const float innerRadiusSquared = mInnerRadius * mInnerRadius;
		for (const shared_ptr<Actor>& target : targeting::FindOpposingCombatants(
			*world,
			*owner,
			mResolvedTargetLocation,
			mOuterRadius
		))
		{
			if (!target || target->GetIsPendingDestroy())
			{
				continue;
			}
			const sf::Vector2f delta = target->GetActorLocation() -
				mResolvedTargetLocation;
			const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
			if (distanceSquared > outerRadiusSquared)
			{
				continue;
			}

			const bool inInnerZone = distanceSquared <= innerRadiusSquared;
			DamagePayload payload = GetDamagePayload();
			payload.igniteStacks = inInnerZone
				? mInnerIgniteStacks
				: mOuterIgniteStacks;
			const float damageMultiplier = inInnerZone
				? mInnerDamageMultiplier
				: 1.f;
			ApplyCombatDamage(
				*target,
				GetDamage() * damageMultiplier,
				owner,
				GetDamageTags(),
				payload,
				GetSourceAbilityId(),
				GetSourceAbilityTags(),
				DamageDeliveryType::Area,
				this
			);
		}
	}

	void SolarBombardmentProjectileActor::ConfigureVisualGeometry()
	{
		const float projectileRadius = std::max(
			1.f,
			mPresentationProfile.projectile.radius
		);
		ConfigureCircle(mProjectileGlow, projectileRadius * 1.9f);
		ConfigureCircle(mProjectileCore, projectileRadius);
		ConfigureCircle(mExplosionOuter, mPresentationProfile.explosion.outerRadius);
		ConfigureCircle(mExplosionInner, mPresentationProfile.explosion.innerRadius);
		ConfigureCircle(mExplosionShockwave, 1.f);
	}

	void SolarBombardmentProjectileActor::DrawProjectile(sf::RenderWindow& window)
	{
		const float pulse = 0.88f + 0.12f * std::sin(
			mVisualAge * mPresentationProfile.projectile.pulseSpeed
		);
		const sf::Vector2f location = GetActorLocation();
		const sf::Vector2f direction = GetVectorLength(mFlightDirection) > 0.001f
			? mFlightDirection
			: sf::Vector2f{ 0.f, -1.f };
		const sf::Vector2f right{ -direction.y, direction.x };
		const float radius = std::max(1.f, mPresentationProfile.projectile.radius);
		const sf::Vector2f trailStart = location - direction *
			mPresentationProfile.projectile.trailLength;
		const sf::Vector2f trailEnd = location - direction * radius * 0.35f;
		mTrail[0].position = trailStart - right * radius * 0.20f;
		mTrail[1].position = trailStart + right * radius * 0.20f;
		mTrail[2].position = trailEnd + right * radius * 0.70f;
		mTrail[3].position = trailEnd - right * radius * 0.70f;
		for (std::size_t index = 0; index < 4; ++index)
		{
			mTrail[index].color = WithAlpha(
				mPresentationProfile.projectile.trailColor,
				pulse
			);
		}

		mProjectileGlow.setPosition(location);
		mProjectileCore.setPosition(location);
		mProjectileGlow.setFillColor(WithAlpha(
			mPresentationProfile.projectile.outerColor, pulse
		));
		mProjectileCore.setFillColor(WithAlpha(
			mPresentationProfile.projectile.coreColor, pulse
		));
		window.draw(mTrail);
		window.draw(mProjectileGlow);
		window.draw(mProjectileCore);
	}

	void SolarBombardmentProjectileActor::DrawExplosion(sf::RenderWindow& window)
	{
		const float duration = std::max(
			0.01f,
			mPresentationProfile.explosion.duration
		);
		const float progress = std::clamp(mExplosionAge / duration, 0.f, 1.f);
		const float eased = 1.f - (1.f - progress) * (1.f - progress);
		const sf::Vector2f location = GetActorLocation();
		mExplosionOuter.setPosition(location);
		mExplosionInner.setPosition(location);
		mExplosionShockwave.setPosition(location);
		ConfigureCircle(mExplosionOuter, mPresentationProfile.explosion.outerRadius * eased);
		ConfigureCircle(mExplosionInner, mPresentationProfile.explosion.innerRadius * eased);
		ConfigureCircle(mExplosionShockwave, std::max(1.f,
			mPresentationProfile.explosion.outerRadius * progress));
		mExplosionOuter.setFillColor(WithAlpha(
			mPresentationProfile.explosion.outerFillColor,
			1.f - progress
		));
		mExplosionOuter.setOutlineColor(WithAlpha(
			mPresentationProfile.explosion.outerOutlineColor,
			1.f - progress * 0.65f
		));
		mExplosionOuter.setOutlineThickness(
			mPresentationProfile.explosion.outlineThickness
		);
		mExplosionInner.setFillColor(WithAlpha(
			mPresentationProfile.explosion.innerFillColor,
			1.f - progress
		));
		mExplosionInner.setOutlineColor(WithAlpha(
			mPresentationProfile.explosion.innerOutlineColor,
			1.f - progress * 0.50f
		));
		mExplosionInner.setOutlineThickness(
			mPresentationProfile.explosion.outlineThickness
		);
		mExplosionShockwave.setFillColor(sf::Color::Transparent);
		mExplosionShockwave.setOutlineColor(WithAlpha(
			mPresentationProfile.explosion.shockwaveColor,
			1.f - progress
		));
		mExplosionShockwave.setOutlineThickness(
			mPresentationProfile.explosion.shockwaveThickness
		);
		window.draw(mExplosionOuter);
		window.draw(mExplosionInner);
		window.draw(mExplosionShockwave);
	}

	void SolarBombardmentProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		if (mDetonated)
		{
			DrawExplosion(window);
		}
		else
		{
			DrawProjectile(window);
		}
	}

	void SolarBombardmentProjectileActor::OnActorBeginOverlap(Actor*)
	{
		// Physics is disabled for this delivery actor. It must travel to its
		// target point and detonate there, regardless of incidental overlaps.
	}

	bool RegisterSolarBombardmentProjectileActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<SolarBombardmentProjectileActorTypeHandler>()
		);
	}
}
