#include "gameplay/ability/ionStorm/IonStormProjectileActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/ionStorm/IonStormContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace ly
{
	namespace
	{
		constexpr float DirectionEpsilonSquared = 0.000001f;
		constexpr float DegreesPerRadian = 57.2957795131f;

		const List<sas::AttributeId> ProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Duration,
			CommonAttributeIds::Radius,
			CommonAttributeIds::Range
		};

		const List<sas::AttributeId> ProjectileAttributeRoots{
			AbilityData::IonStorm::Actor::Projectile::Root,
			AbilityData::IonStorm::Actor::Field::Root
		};

		bool IsFiniteVector(const sf::Vector2f& value)
		{
			return std::isfinite(value.x) && std::isfinite(value.y);
		}

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			return sf::Color{
				color.r,
				color.g,
				color.b,
				static_cast<std::uint8_t>(std::clamp(
					static_cast<float>(color.a) * std::max(0.f, multiplier),
					0.f,
					255.f
				))
			};
		}

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

		class IonStormProjectileActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::IonStormProjectile;
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
					CommonAttributeIds::Duration,
					CommonAttributeIds::Radius,
					CommonAttributeIds::Range,
					AbilityData::IonStorm::Attribute::ProjectileSpeed,
					AbilityData::IonStorm::Attribute::TickInterval,
					AbilityData::IonStorm::Attribute::InnerCoreRadius,
					AbilityData::IonStorm::Attribute::OuterMinRadius,
					AbilityData::IonStorm::Attribute::OuterMaxRadius,
					AbilityData::IonStorm::Attribute::BoundaryPointCount
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return {
							false,
							"Ion Storm projectile requires positive delivery and field attributes."
						};
					}
				}
				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<
						IonStormProjectilePresentationProfile
					>::Find(definition.presentationProfileId.ToString()) == nullptr)
				{
					return {
						false,
						"Ion Storm projectile requires a valid typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const IonStormProjectilePresentationProfile* profile =
					PresentationProfileRegistry<
						IonStormProjectilePresentationProfile
					>::Find(context.definition.presentationProfileId.ToString());
				return world && profile
					? world->SpawnActor<IonStormProjectileActor>(
						&context.owner,
						*profile,
						context.targetLocation
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	IonStormProjectileActor::IonStormProjectileActor(
		World* world,
		Actor* owner,
		const IonStormProjectilePresentationProfile& presentationProfile,
		std::optional<sf::Vector2f> requestedTargetLocation
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
		, mRequestedTargetLocation(std::move(requestedTargetLocation))
		, mOwnerReference(MakeWeakActor(owner))
		, mGlow(std::max(1.f, presentationProfile.visual.radius * 1.8f), 28)
		, mCore(std::max(1.f, presentationProfile.visual.radius), 24)
		, mTrail(sf::PrimitiveType::TriangleStrip, 4)
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		mGlow.setOrigin({ mGlow.getRadius(), mGlow.getRadius() });
		mCore.setOrigin({ mCore.getRadius(), mCore.getRadius() });
	}

	void IonStormProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mProjectileSpeed = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::IonStorm::Attribute::ProjectileSpeed,
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
		mFieldDamage = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Damage,
			mFieldDamage
		));
		mFieldAttributes = {
			sas::GameplayAttribute{
				CommonAttributeIds::Duration,
				mFieldDuration,
				0.01f
			},
			sas::GameplayAttribute{
				CommonAttributeIds::Radius,
				sas::FindAttributeValue(attributes, CommonAttributeIds::Radius, 335.f),
				0.01f
			},
			sas::GameplayAttribute{
				CommonAttributeIds::Damage,
				mFieldDamage,
				0.f
			},
			sas::GameplayAttribute{
				AbilityData::IonStorm::Attribute::TickInterval,
				sas::FindAttributeValue(attributes, AbilityData::IonStorm::Attribute::TickInterval, 0.25f),
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::IonStorm::Attribute::InnerCoreRadius,
				sas::FindAttributeValue(attributes, AbilityData::IonStorm::Attribute::InnerCoreRadius, 250.f),
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::IonStorm::Attribute::OuterMinRadius,
				sas::FindAttributeValue(attributes, AbilityData::IonStorm::Attribute::OuterMinRadius, 250.f),
				0.01f
			},
			sas::GameplayAttribute{
				AbilityData::IonStorm::Attribute::OuterMaxRadius,
				sas::FindAttributeValue(attributes, AbilityData::IonStorm::Attribute::OuterMaxRadius, 335.f),
				0.01f
		},
			sas::GameplayAttribute{
				AbilityData::IonStorm::Attribute::BoundaryPointCount,
				sas::FindAttributeValue(attributes, AbilityData::IonStorm::Attribute::BoundaryPointCount, 20.f),
				3.f
		}
		};
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		ResolveTargetLocation();
		mTravelDistance = 0.f;
		mHasSpawnedField = false;
		ConfigureVisualGeometry();
	}

	void IonStormProjectileActor::ResolveTargetLocation()
	{
		const shared_ptr<Actor> owner = mOwnerReference.lock();
		const sf::Vector2f sourceLocation = owner
			? owner->GetActorLocation()
			: GetActorLocation();
		sf::Vector2f direction = owner
			? owner->GetActorForwardDirection()
			: GetActorForwardDirection();
		if (!IsFiniteVector(direction) ||
			direction.x * direction.x + direction.y * direction.y <= DirectionEpsilonSquared)
		{
			direction = { 0.f, -1.f };
		}

		if (mRequestedTargetLocation && IsFiniteVector(*mRequestedTargetLocation))
		{
			const sf::Vector2f aimDelta = *mRequestedTargetLocation - sourceLocation;
			const float aimDistanceSquared = aimDelta.x * aimDelta.x + aimDelta.y * aimDelta.y;
			if (std::isfinite(aimDistanceSquared) && aimDistanceSquared > DirectionEpsilonSquared)
			{
				const float aimDistance = std::sqrt(aimDistanceSquared);
				direction = aimDelta / aimDistance;
				mResolvedTargetLocation = sourceLocation + direction *
					std::min(aimDistance, mCastRange);
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

		const sf::Vector2f flightDelta = mResolvedTargetLocation - GetActorLocation();
		const float flightDistanceSquared = flightDelta.x * flightDelta.x + flightDelta.y * flightDelta.y;
		if (std::isfinite(flightDistanceSquared) && flightDistanceSquared > DirectionEpsilonSquared)
		{
			mTargetTravelDistance = std::sqrt(flightDistanceSquared);
			mFlightDirection = flightDelta / mTargetTravelDistance;
		}
		else
		{
			mTargetTravelDistance = 0.f;
			mFlightDirection = direction;
		}

		SetActorRotation(std::atan2(
			mFlightDirection.y,
			mFlightDirection.x
		) * DegreesPerRadian + 90.f);
		SetVelocity(mFlightDirection * mProjectileSpeed);
	}

	void IonStormProjectileActor::MoveTowardTarget(float deltaTime)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (mProjectileSpeed <= 0.f || safeDeltaTime <= 0.f || mTargetTravelDistance <= 0.f)
		{
			return;
		}

		const float remainingDistance = std::max(
			0.f,
			mTargetTravelDistance - mTravelDistance
		);
		const float stepDistance = std::min(
			mProjectileSpeed * safeDeltaTime,
			remainingDistance
		);
		SetVelocity(mFlightDirection * mProjectileSpeed);
		AddActorLocationOffset(mFlightDirection * stepDistance);
		mTravelDistance += stepDistance;
	}

	void IonStormProjectileActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}
		if (GetIsPendingDestroy() || mHasSpawnedField)
		{
			return;
		}

		const shared_ptr<Actor> owner = mOwnerReference.lock();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		mVisualAge += std::max(0.f, deltaTime);
		MoveTowardTarget(deltaTime);
		if (mTravelDistance >= mTargetTravelDistance)
		{
			SpawnField();
			Destroy();
			return;
		}
		AbilityWorldActor::Tick(deltaTime);
	}

	void IonStormProjectileActor::RebasePortalDestination(
		const sf::Vector2f& exitLocation
	)
	{
		const float remainingDistance = std::max(
			0.f,
			mTargetTravelDistance - mTravelDistance
		);
		if (!mHasSpawnedField)
		{
			// Ion Storm's field must be delivered after the portal exit, rather
			// than appearing at the pre-transfer cursor target.
			mResolvedTargetLocation = portal::RebaseForwardDestination(
				exitLocation, mFlightDirection, remainingDistance
			);
		}
	}

	void IonStormProjectileActor::SpawnField()
	{
		if (mHasSpawnedField)
		{
			return;
		}
		mHasSpawnedField = true;

		const shared_ptr<Actor> owner = mOwnerReference.lock();
		const AbilityActorDefinition* definition = AbilityData::FindAbilityActorDefinition(
			AbilityData::IonStorm::Actor::Field::BasicDefinitionId
		);
		if (!owner || owner->GetIsPendingDestroy() || !definition)
		{
			return;
		}

		const weak_ptr<AbilityWorldActor> spawned = AbilityActorRegistry::Spawn(
			AbilityActorSpawnContext{
				*owner,
				*definition,
				mFieldAttributes,
				mResolvedTargetLocation,
				{},
				GetSourceAbilityInstance()
			}
		);
		const shared_ptr<AbilityWorldActor> field = spawned.lock();
		if (!field)
		{
			return;
		}

		field->SetActorLocation(mResolvedTargetLocation);
		field->SetActorRotation(GetActorRotation());
		field->SetLifeTime(mFieldDuration);
		field->SetDamageTags(GetDamageTags());
		field->SetDamageAttributes(mFieldAttributes);
		field->SetSourceAbility(GetSourceAbilityId(), GetSourceAbilityTags());
		field->SetSourceAbilityInstance(GetSourceAbilityInstance());
		field->SetAbilityUpgradeIds(GetAbilityUpgradeIds());
		field->SetAbilityCollisionRadius(0.f);
		field->ConfigureFromAttributes(mFieldAttributes);
		field->SetDamage(mFieldDamage);
	}

	void IonStormProjectileActor::ConfigureVisualGeometry()
	{
		mGlow.setRadius(std::max(1.f, mPresentationProfile.visual.radius * 1.8f));
		mCore.setRadius(std::max(1.f, mPresentationProfile.visual.radius));
		mGlow.setOrigin({ mGlow.getRadius(), mGlow.getRadius() });
		mCore.setOrigin({ mCore.getRadius(), mCore.getRadius() });
	}

	void IonStormProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}

		const float pulse = 0.88f + 0.12f * std::sin(
			mVisualAge * mPresentationProfile.visual.pulseSpeed
		);
		const sf::Vector2f location = GetActorLocation();
		const sf::Vector2f direction = GetVectorLength(mFlightDirection) > 0.001f
			? mFlightDirection
			: sf::Vector2f{ 0.f, -1.f };
		const sf::Vector2f right{ -direction.y, direction.x };
		const float radius = std::max(1.f, mPresentationProfile.visual.radius);
		const sf::Vector2f trailStart = location - direction *
			mPresentationProfile.visual.trailLength;
		const sf::Vector2f trailEnd = location - direction * radius * 0.4f;
		mTrail[0].position = trailStart - right * radius * 0.25f;
		mTrail[1].position = trailStart + right * radius * 0.25f;
		mTrail[2].position = trailEnd + right * radius * 0.6f;
		mTrail[3].position = trailEnd - right * radius * 0.6f;
		const sf::Color trailColor = WithAlpha(
			mPresentationProfile.visual.trailColor,
			pulse
		);
		for (std::size_t index = 0; index < 4; ++index)
		{
			mTrail[index].color = trailColor;
		}

		mGlow.setPosition(location);
		mCore.setPosition(location);
		mGlow.setFillColor(WithAlpha(
			mPresentationProfile.visual.outerColor,
			pulse
		));
		mCore.setFillColor(WithAlpha(
			mPresentationProfile.visual.coreColor,
			pulse
		));
		window.draw(mTrail);
		window.draw(mGlow);
		window.draw(mCore);
	}

	bool RegisterIonStormProjectileActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<IonStormProjectileActorTypeHandler>()
		);
	}
}
