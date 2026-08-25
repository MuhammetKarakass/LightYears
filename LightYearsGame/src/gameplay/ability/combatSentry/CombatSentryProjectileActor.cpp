#include "gameplay/ability/combatSentry/CombatSentryProjectileActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/combatSentry/CombatSentryContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileSweep.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> ProjectileCommonAttributes{
			CommonAttributeIds::Duration,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius
		};

		const List<sas::AttributeId> ProjectileAttributeRoots{
			AbilityData::CombatSentry::Actor::Projectile::Root
		};

		class CombatSentryProjectileActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::CombatSentryProjectile;
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
					CommonAttributeIds::Duration,
					CommonAttributeIds::Range,
					CollisionAttributeIds::Radius,
					AbilityData::CombatSentry::Actor::Projectile::ProjectileSpeed
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute || !std::isfinite(attribute->baseValue) ||
						attribute->baseValue <= 0.f)
					{
						return { false, "Combat Sentry projectile requires positive delivery attributes." };
					}
				}
				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<CombatSentryProjectilePresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return { false, "Combat Sentry projectile requires a typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const auto* profile =
					PresentationProfileRegistry<CombatSentryProjectilePresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<CombatSentryProjectileActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	CombatSentryProjectileActor::CombatSentryProjectileActor(
		World* world,
		Actor* owner,
		const CombatSentryProjectilePresentationProfile& profile
	)
		: AbilityWorldActor(world, owner)
		, mProfile(profile)
	{
		SetRenderLayer(RenderLayer::Projectile);
	}

	void CombatSentryProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mSpeed = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::CombatSentry::Actor::Projectile::ProjectileSpeed,
			mSpeed
		));
		mMaximumRange = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Range,
			mMaximumRange
		));
		mTravelDistance = 0.f;
		SetVelocity(GetActorForwardDirection() * mSpeed);
	}

	void CombatSentryProjectileActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		Move(std::max(0.f, deltaTime));
		if (mMaximumRange > 0.f && mTravelDistance >= mMaximumRange)
		{
			Destroy();
			return;
		}
		AbilityWorldActor::Tick(std::max(0.f, deltaTime));
	}

	void CombatSentryProjectileActor::Move(float deltaTime)
	{
		const sf::Vector2f startLocation = GetActorLocation();
		const sf::Vector2f movement = GetActorForwardDirection() * mSpeed * deltaTime;
		const sf::Vector2f endLocation = startLocation + movement;
		for (const projectile::SweptContact& contact : projectile::FindSweptContacts(
			*this,
			startLocation,
			endLocation,
			std::max(0.f, GetPhysicsCollisionRadius())
		))
		{
			if (GetIsPendingDestroy())
			{
				return;
			}
			SetActorLocation(startLocation + movement * contact.fraction);
			if (contact.hasSurfaceNormal &&
				ProjectileReflectionService::TryReflectFromSurface(
					*this,
					*contact.actor,
					{ contact.impactLocation, contact.surfaceNormal }
				))
			{
				mTravelDistance += GetVectorLength(movement) * contact.fraction;
				return;
			}
			OnActorBeginOverlap(contact.actor.get());
		}
		if (!GetIsPendingDestroy())
		{
			SetActorLocation(endLocation);
			mTravelDistance += GetVectorLength(movement);
			SetVelocity(GetActorForwardDirection() * mSpeed);
		}
	}

	void CombatSentryProjectileActor::OnActorBeginOverlap(Actor* otherActor)
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
		if (!IsValidAbilityTarget(otherActor))
		{
			return;
		}
		ApplyCombatDamage(
			*otherActor,
			GetDamage(),
			GetOwnerActor(),
			GetDamageTags(),
			GetDamagePayload(),
			GetSourceAbilityId(),
			GetSourceAbilityTags(),
			DamageDeliveryType::Projectile,
			this
		);
		Destroy();
	}

	bool CombatSentryProjectileActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		sf::Vector2f trajectory = GetActorForwardDirection() * mSpeed;
		return ApplyBallisticReflection(request, mSpeed, trajectory);
	}

	void CombatSentryProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		const sf::Vector2f direction = GetActorForwardDirection();
		const float rotation = std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f;
		sf::RectangleShape trail({ 3.f, mProfile.trailLength });
		trail.setOrigin({ 1.5f, mProfile.trailLength });
		trail.setPosition(GetActorLocation());
		trail.setRotation(sf::degrees(rotation));
		trail.setFillColor(mProfile.trailColor);
		window.draw(trail);

		sf::CircleShape core(mProfile.coreRadius, 14);
		core.setOrigin({ mProfile.coreRadius, mProfile.coreRadius });
		core.setPosition(GetActorLocation());
		core.setFillColor(mProfile.coreColor);
		window.draw(core);
	}

	bool RegisterCombatSentryProjectileActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<CombatSentryProjectileActorTypeHandler>()
		);
	}
}
