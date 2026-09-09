#include "gameplay/ability/seismicCharge/SeismicChargeBombActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/seismicCharge/SeismicChargeContracts.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace ly
{
	namespace
	{
		constexpr float Epsilon = 0.001f;

		const List<sas::AttributeId> BombCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Range
		};
		const List<sas::AttributeId> BombAttributeRoots{
			AbilityData::SeismicCharge::Actor::Bomb::Root
		};

		float DistanceSquared(const sf::Vector2f& left, const sf::Vector2f& right)
		{
			const sf::Vector2f delta = left - right;
			return delta.x * delta.x + delta.y * delta.y;
		}

		float MinimumDistanceToBoundsSquared(
			const sf::FloatRect& bounds,
			const sf::Vector2f& point
		)
		{
			if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
			{
				return 0.f;
			}
			const float closestX = std::clamp(point.x, bounds.position.x, bounds.position.x + bounds.size.x);
			const float closestY = std::clamp(point.y, bounds.position.y, bounds.position.y + bounds.size.y);
			return DistanceSquared(point, { closestX, closestY });
		}

		float MaximumDistanceToBoundsSquared(
			const sf::FloatRect& bounds,
			const sf::Vector2f& point
		)
		{
			if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
			{
				return 0.f;
			}
			float maximum = 0.f;
			for (const sf::Vector2f corner : {
				sf::Vector2f{ bounds.position.x, bounds.position.y },
				sf::Vector2f{ bounds.position.x + bounds.size.x, bounds.position.y },
				sf::Vector2f{ bounds.position.x, bounds.position.y + bounds.size.y },
				sf::Vector2f{ bounds.position.x + bounds.size.x, bounds.position.y + bounds.size.y }
			})
			{
				maximum = std::max(maximum, DistanceSquared(point, corner));
			}
			return maximum;
		}

		bool IntersectsShockwaveBand(
			const Actor& target,
			const sf::Vector2f& center,
			float innerRadius,
			float outerRadius
		)
		{
			const sf::FloatRect bounds = target.GetActorGlobalBounds();
			if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
			{
				const float distanceSquared = DistanceSquared(target.GetActorLocation(), center);
				return distanceSquared >= innerRadius * innerRadius &&
					distanceSquared <= outerRadius * outerRadius;
			}
			return MaximumDistanceToBoundsSquared(bounds, center) >= innerRadius * innerRadius &&
				MinimumDistanceToBoundsSquared(bounds, center) <= outerRadius * outerRadius;
		}

		sf::Color WithAlpha(const sf::Color& color, float alpha)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
			return result;
		}

		class SeismicChargeBombActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::SeismicChargeBomb;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return BombAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return BombCommonAttributes;
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
					AbilityData::SeismicCharge::Attribute::Damage,
					AbilityData::SeismicCharge::Attribute::MaximumRadius,
					AbilityData::SeismicCharge::Attribute::DropOffset,
					AbilityData::SeismicCharge::Attribute::DeploymentDuration,
					AbilityData::SeismicCharge::Attribute::FuseDuration,
					AbilityData::SeismicCharge::Attribute::ShockwaveDuration,
					AbilityData::SeismicCharge::Attribute::ShockwaveThickness
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes, required
					);
					if (!attribute || !std::isfinite(attribute->baseValue) ||
						attribute->baseValue <= 0.f)
					{
						return { false, "Seismic Charge bomb is missing a positive required runtime attribute." };
					}
				}
				const float totalDuration =
					sas::FindAttributeValue(definition.attributes, AbilityData::SeismicCharge::Attribute::DeploymentDuration, 0.f) +
					sas::FindAttributeValue(definition.attributes, AbilityData::SeismicCharge::Attribute::FuseDuration, 0.f) +
					sas::FindAttributeValue(definition.attributes, AbilityData::SeismicCharge::Attribute::ShockwaveDuration, 0.f);
				if (definition.lifeTime < totalDuration || !definition.presentationProfileId.IsValid() ||
					!PresentationProfileRegistry<SeismicChargePresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					))
				{
					return { false, "Seismic Charge bomb requires sufficient lifetime and a registered typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const SeismicChargePresentationProfile* profile =
					PresentationProfileRegistry<SeismicChargePresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<SeismicChargeBombActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	SeismicChargeBombActor::SeismicChargeBombActor(
		World* world,
		Actor* owner,
		const SeismicChargePresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile(presentationProfile),
		mBombGlow(1.f, 32),
		mBombCore(1.f, 32),
		mMaximumRangeTelegraph(1.f, 96),
		mShockwave(1.f, 128)
	{
		SetRenderLayer(RenderLayer::Projectile);
		// The bomb is a deterministic delivery marker, never a physical obstacle
		// or a targetable projectile. Its damage comes only from the shockwave band.
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void SeismicChargeBombActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		const auto value = [&](const sas::AttributeId& id, float fallback)
		{
			return sas::FindAttributeValue(attributes, id, fallback);
		};
		mDropOffset = std::max(0.f, value(AbilityData::SeismicCharge::Attribute::DropOffset, 0.f));
		mDeploymentDuration = std::max(0.01f, value(AbilityData::SeismicCharge::Attribute::DeploymentDuration, 0.5f));
		mFuseDuration = std::max(0.01f, value(AbilityData::SeismicCharge::Attribute::FuseDuration, 3.f));
		mShockwaveDuration = std::max(0.01f, value(AbilityData::SeismicCharge::Attribute::ShockwaveDuration, 2.f));
		mShockwaveThickness = std::max(1.f, value(AbilityData::SeismicCharge::Attribute::ShockwaveThickness, 120.f));
		mMaximumRadius = std::max(1.f, value(AbilityData::SeismicCharge::Attribute::MaximumRadius, 1200.f));
		mDeploymentStart = GetActorLocation();
		const sf::Vector2f ownerForward = GetActorForwardDirection();
		mDeploymentDirection = { -ownerForward.x, -ownerForward.y };
		if (GetVectorLength(mDeploymentDirection) <= Epsilon)
		{
			mDeploymentDirection = { 0.f, 1.f };
		}
		else
		{
			NormalizeVector(mDeploymentDirection);
		}
		mDropLocation = mDeploymentStart + mDeploymentDirection * mDropOffset;
		mPhase = Phase::Deployment;
		mPhaseAge = 0.f;
		mCurrentRadius = 0.f;
		mHitActors.clear();
		ConfigureCircle(mBombGlow, mPresentationProfile.bombRadius * 1.8f);
		ConfigureCircle(mBombCore, mPresentationProfile.bombRadius);
		ConfigureCircle(mMaximumRangeTelegraph, mMaximumRadius);
		ConfigureCircle(mShockwave, 1.f);
	}

	void SeismicChargeBombActor::Tick(float deltaTime)
	{
		AbilityWorldActor::Tick(deltaTime);
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mPhaseAge += safeDeltaTime;
		if (mPhase == Phase::Deployment)
		{
			const float progress = std::clamp(mPhaseAge / mDeploymentDuration, 0.f, 1.f);
			SetActorLocation(mDeploymentStart + (mDropLocation - mDeploymentStart) * progress);
			if (progress >= 1.f)
			{
				mPhase = Phase::Fuse;
				mPhaseAge = 0.f;
				SetActorLocation(mDropLocation);
			}
			return;
		}

		if (mPhase == Phase::Fuse)
		{
			if (mPhaseAge >= mFuseDuration)
			{
				BeginShockwave();
			}
			return;
		}

		const float previousRadius = mCurrentRadius;
		mCurrentRadius = mMaximumRadius * std::clamp(
			mPhaseAge / mShockwaveDuration, 0.f, 1.f
		);
		ApplyShockwaveHits(previousRadius, mCurrentRadius);
		if (mPhaseAge >= mShockwaveDuration)
		{
			Destroy();
		}
	}

	void SeismicChargeBombActor::BeginShockwave()
	{
		mPhase = Phase::Shockwave;
		mPhaseAge = 0.f;
		mCurrentRadius = 0.f;
	}

	void SeismicChargeBombActor::ApplyShockwaveHits(
		float previousRadius,
		float currentRadius
	)
	{
		World* world = GetWorld();
		if (!world || currentRadius <= 0.f || GetDamage() <= 0.f)
		{
			return;
		}
		const float halfThickness = mShockwaveThickness * 0.5f;
		const float innerRadius = std::max(0.f, std::min(previousRadius, currentRadius) - halfThickness);
		const float outerRadius = std::min(mMaximumRadius + halfThickness, currentRadius + halfThickness);
		for (const weak_ptr<Actor>& candidate : world->GetActorsInBounds(
			targeting::swept::RadiusBounds(GetActorLocation(), outerRadius)
		))
		{
			const shared_ptr<Actor> target = candidate.lock();
			if (!target || target.get() == this || target->GetIsPendingDestroy() ||
				mHitActors.find(target.get()) != mHitActors.end() ||
				!dynamic_cast<Combatant*>(target.get()) ||
				!IntersectsShockwaveBand(*target, GetActorLocation(), innerRadius, outerRadius))
			{
				continue;
			}

			// No team filter is applied: the owner, friendly summons and enemies all
			// receive the same Energy hit when the expanding ring reaches them.
			mHitActors.insert(target.get());
			ApplyCombatDamage(
				*target,
				GetDamage(),
				GetOwnerActor(),
				GetDamageTags(),
				GetDamagePayload(),
				GetSourceAbilityId(),
				GetSourceAbilityTags(),
				DamageDeliveryType::Area,
				this
			);
		}
	}

	void SeismicChargeBombActor::ConfigureCircle(sf::CircleShape& shape, float radius) const
	{
		const float safeRadius = std::max(1.f, radius);
		shape.setRadius(safeRadius);
		shape.setOrigin({ safeRadius, safeRadius });
	}

	void SeismicChargeBombActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		const sf::Vector2f location = GetActorLocation();
		if (mPhase != Phase::Shockwave)
		{
			const float fuseProgress = mPhase == Phase::Fuse
				? std::clamp(mPhaseAge / mFuseDuration, 0.f, 1.f)
				: 0.f;
			const float pulse = 0.78f + 0.22f * std::sin(
				GetAge() * (5.f + fuseProgress * 15.f)
			);
			mBombGlow.setPosition(location);
			mBombCore.setPosition(location);
			mBombGlow.setFillColor(WithAlpha(mPresentationProfile.bombOuterColor, pulse));
			mBombCore.setFillColor(WithAlpha(mPresentationProfile.bombCoreColor, 0.8f + fuseProgress * 0.2f));
			sf::RenderStates additive;
			additive.blendMode = sf::BlendAdd;
			window.draw(mBombGlow, additive);
			window.draw(mBombCore, additive);

			if (mPhase == Phase::Fuse)
			{
				mMaximumRangeTelegraph.setPosition(location);
				mMaximumRangeTelegraph.setFillColor(sf::Color::Transparent);
				mMaximumRangeTelegraph.setOutlineColor(WithAlpha(
					mPresentationProfile.telegraphColor, 0.35f + fuseProgress * 0.55f
				));
				mMaximumRangeTelegraph.setOutlineThickness(
					mPresentationProfile.maximumRangeOutlineThickness
				);
				window.draw(mMaximumRangeTelegraph, additive);
			}
			return;
		}

		ConfigureCircle(mShockwave, std::max(1.f, mCurrentRadius));
		mShockwave.setPosition(location);
		mShockwave.setFillColor(sf::Color::Transparent);
		mShockwave.setOutlineColor(WithAlpha(
			mPresentationProfile.shockwaveColor,
			1.f - std::clamp(mPhaseAge / mShockwaveDuration, 0.f, 1.f) * 0.45f
		));
		mShockwave.setOutlineThickness(mPresentationProfile.shockwaveOutlineThickness);
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mShockwave, additive);
	}

	bool RegisterSeismicChargeBombActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<SeismicChargeBombActorTypeHandler>()
		);
	}
}
