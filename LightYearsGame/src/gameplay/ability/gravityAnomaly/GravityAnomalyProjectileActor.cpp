#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/gravityAnomaly/GravityAnomalyProjectileActor.h"

#include "framework/World.h"
#include "gameConfigs/ability/GravityAnomalyConfig.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace ly
{
	namespace
	{
		const List<GameplayTag> ProjectileCommonAttributes{
			CommonAttributeIds::Duration,
			CommonAttributeIds::Radius
		};

		const List<GameplayTag> GravityAnomalyAttributeRoots{
			AbilityData::GravityAnomaly::ActorSchema::AttributeRoot
		};

		bool IsFiniteVector(const sf::Vector2f& value)
		{
			return std::isfinite(value.x) && std::isfinite(value.y);
		}

		class GravityAnomalyProjectileActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			const GameplayTag& GetActorTypeTag() const override
			{
				return AbilityData::GravityAnomaly::ActorSchema::ProjectileTypeId;
			}

			const List<GameplayTag>& GetOwnedAttributeRoots() const override
			{
				return GravityAnomalyAttributeRoots;
			}

			const List<GameplayTag>& GetAllowedCommonAttributeIds() const override
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

				for (const GameplayTag& required : {
					AbilityData::GravityAnomaly::ActorSchema::ProjectileSpeed,
					AbilityData::GravityAnomaly::ActorSchema::CastRange,
					CommonAttributeIds::Duration,
					CommonAttributeIds::Radius,
					AbilityData::GravityAnomaly::ActorSchema::PullStrength,
					AbilityData::GravityAnomaly::ActorSchema::SlowMagnitude
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindGameplayAttribute(definition.attributes, required);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return { false, "Gravity Anomaly projectile requires a positive '" + required.ToString() + "' attribute." };
					}
				}

				if (definition.spawnDistance < 0.f || definition.presentationProfileId.empty() ||
					PresentationProfileRegistry<GravityAnomalyProjectilePresentationProfile>::Find(
						definition.presentationProfileId
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
						context.definition.presentationProfileId
					);
				return world && profile
					? world->SpawnActor<GravityAnomalyProjectileActor>(
						&context.owner,
						context.definition.texturePath,
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
		const std::string& texturePath,
		const GravityAnomalyProjectilePresentationProfile& presentationProfile,
		std::optional<sf::Vector2f> requestedTargetLocation
	)
		: AbilityWorldActor(world, owner, texturePath)
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
		mProjectileSpeed = std::max(0.f, sas::FindGameplayAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::ActorSchema::ProjectileSpeed,
			mProjectileSpeed
		));
		mCastRange = std::max(0.f, sas::FindGameplayAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::ActorSchema::CastRange,
			mCastRange
		));
		mFieldDuration = std::max(0.f, sas::FindGameplayAttributeValue(
			attributes,
			CommonAttributeIds::Duration,
			mFieldDuration
		));
		mFieldRadius = std::max(0.f, sas::FindGameplayAttributeValue(
			attributes,
			CommonAttributeIds::Radius,
			mFieldRadius
		));
		mPullStrength = std::max(0.f, sas::FindGameplayAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::ActorSchema::PullStrength,
			mPullStrength
		));
		mSlowMagnitude = std::clamp(sas::FindGameplayAttributeValue(
			attributes,
			AbilityData::GravityAnomaly::ActorSchema::SlowMagnitude,
			mSlowMagnitude
		), 0.f, 0.95f);
		mFieldAttributes = {
			sas::GameplayAttribute{ CommonAttributeIds::Duration, mFieldDuration, 0.01f },
			sas::GameplayAttribute{ CommonAttributeIds::Radius, mFieldRadius, 0.01f },
			sas::GameplayAttribute{ AbilityData::GravityAnomaly::ActorSchema::PullStrength, mPullStrength, 0.f },
			sas::GameplayAttribute{ AbilityData::GravityAnomaly::ActorSchema::SlowMagnitude, mSlowMagnitude, 0.f, 0.95f }
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
		const sf::Vector2f sourceLocation = owner ? owner->GetActorLocation() : GetActorLocation();
		sf::Vector2f direction = owner ? owner->GetActorForwardDirection() : GetActorForwardDirection();
		if (!IsFiniteVector(direction) || direction.x * direction.x + direction.y * direction.y <= 0.000001f)
		{
			direction = { 0.f, -1.f };
		}

		if (mRequestedTargetLocation && IsFiniteVector(*mRequestedTargetLocation))
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
		if (mHasSpawnedField)
		{
			return;
		}

		MoveTowardTarget(deltaTime);
		if (mTravelDistance >= mTargetTravelDistance)
		{
			SpawnField();
			Destroy();
			return;
		}
		AbilityWorldActor::Tick(deltaTime);
	}

	void GravityAnomalyProjectileActor::MoveTowardTarget(float deltaTime)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (mProjectileSpeed <= 0.f || safeDeltaTime <= 0.f || mTargetTravelDistance <= 0.f)
		{
			return;
		}
		const float remainingDistance = std::max(0.f, mTargetTravelDistance - mTravelDistance);
		const float stepDistance = std::min(mProjectileSpeed * safeDeltaTime, remainingDistance);
		SetVelocity(mFlightDirection * mProjectileSpeed);
		AddActorLocationOffset(mFlightDirection * stepDistance);
		mTravelDistance += stepDistance;
	}

	void GravityAnomalyProjectileActor::SpawnField()
	{
		if (mHasSpawnedField)
		{
			return;
		}
		mHasSpawnedField = true;

		Actor* owner = GetOwnerActor();
		const AbilityActorDefinition* definition = AbilityData::GravityAnomaly::FindActorDefinition(
			"Actor.Ability.GravityAnomaly.Field.Basic"
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
		Actor::Render(window);
		if (GetIsPendingDestroy())
		{
			return;
		}
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
