#include "gameplay/ability/wingSentinels/WingSentinelProjectileActor.h"

#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/wingSentinels/WingSentinelsContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/projectile/ProjectileReflectionService.h"
#include "gameplay/projectile/ProjectileSweep.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "framework/World.h"

#include <SFML/Graphics/CircleShape.hpp>

#include <algorithm>
#include <memory>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> CommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius
		};
		const List<sas::AttributeId> AttributeRoots{
			AbilityData::WingSentinels::Actor::Projectile::Root,
			DamageAttributeIds::Root
		};

		class TypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override { return AbilityActorType::WingSentinelProjectile; }
			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override { return AttributeRoots; }
			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override { return CommonAttributes; }
			AbilityActorValidationResult ValidateDefinition(const AbilityActorDefinition& definition) const override
			{
				const AbilityActorValidationResult base = AbilityActorTypeHandler::ValidateDefinition(definition);
				if (!base.isValid) return base;
				for (const sas::AttributeId& id : { CommonAttributeIds::Damage, CommonAttributeIds::Range, CollisionAttributeIds::Radius, AbilityData::WingSentinels::Actor::Projectile::ProjectileSpeed })
				{
					if (!sas::FindAttribute(definition.attributes, id) || sas::FindAttributeValue(definition.attributes, id) <= 0.f)
						return { false, "Wing Sentinel projectile requires a positive '" + std::string{ id.GetName() } + "' attribute." };
				}
				if (!definition.presentationProfileId.IsValid() || !PresentationProfileRegistry<WingSentinelsPresentationProfile>::Find(definition.presentationProfileId.ToString()))
					return { false, "Wing Sentinel projectile requires a registered presentation profile." };
				return { true, {} };
			}
			weak_ptr<AbilityWorldActor> Spawn(const AbilityActorSpawnContext& context) const override
			{
				World* world = context.owner.GetWorld();
				const WingSentinelsPresentationProfile* profile = PresentationProfileRegistry<WingSentinelsPresentationProfile>::Find(context.definition.presentationProfileId.ToString());
				return world && profile ? world->SpawnActor<WingSentinelProjectileActor>(&context.owner, *profile) : weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	WingSentinelProjectileActor::WingSentinelProjectileActor(World* world, Actor* owner, const WingSentinelsPresentationProfile& profile)
		: AbilityWorldActor(world, owner), mProfile(profile)
	{
		SetRenderLayer(RenderLayer::Projectile);
	}

	void WingSentinelProjectileActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		SetVelocity(GetActorForwardDirection() * mSpeed);
	}

	void WingSentinelProjectileActor::ConfigureFromAttributes(const sas::GameplayAttributeList& attributes)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mSpeed = std::max(0.f, sas::FindAttributeValue(attributes, AbilityData::WingSentinels::Actor::Projectile::ProjectileSpeed));
		mRange = std::max(0.f, sas::FindAttributeValue(attributes, CommonAttributeIds::Range));
		mTravelDistance = 0.f;
	}

	void WingSentinelProjectileActor::Tick(float deltaTime)
	{
		if (!IsInPortalTransit() && !GetIsPendingDestroy()) Move(std::max(0.f, deltaTime));
		AbilityWorldActor::Tick(std::max(0.f, deltaTime));
	}

	void WingSentinelProjectileActor::Move(float deltaTime)
	{
		if (mSpeed <= 0.f || deltaTime <= 0.f) return;
		const float distance = std::min(mSpeed * deltaTime, std::max(0.f, mRange - mTravelDistance));
		const sf::Vector2f start = GetActorLocation();
		const sf::Vector2f end = start + GetActorForwardDirection() * distance;
		SetVelocity(GetActorForwardDirection() * mSpeed);
		for (const projectile::SweptContact& contact : projectile::FindSweptContacts(*this, start, end, GetPhysicsCollisionRadius()))
		{
			SetActorLocation(start + (end - start) * contact.fraction);
			if (contact.hasSurfaceNormal && ProjectileReflectionService::TryReflectFromSurface(
				*this, *contact.actor, { contact.impactLocation, contact.surfaceNormal }))
			{
				mTravelDistance += distance * contact.fraction;
				return;
			}
			OnActorBeginOverlap(contact.actor.get());
			if (GetIsPendingDestroy() || IsInPortalTransit()) return;
		}
		SetActorLocation(end);
		mTravelDistance += distance;
		if (mTravelDistance >= mRange) Destroy();
	}

	void WingSentinelProjectileActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (GetIsPendingDestroy()) return;
		if (TryReflectOnOverlap(otherActor)) return;
		if (auto* capture = dynamic_cast<ProjectileCaptureVolume*>(otherActor); capture && capture->TryCaptureProjectile(*this)) return;
		AbilityWorldActor::OnActorBeginOverlap(otherActor);
		if (!IsValidAbilityTarget(otherActor)) return;
		ApplyCombatDamage(*otherActor, GetDamage(), GetOwnerActor(), GetDamageTags(), GetDamagePayload(), GetSourceAbilityId(), GetSourceAbilityTags(), DamageDeliveryType::Projectile, this);
		Destroy();
	}

	bool WingSentinelProjectileActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		sf::Vector2f trajectory = GetActorForwardDirection() * mSpeed;
		return ApplyBallisticReflection(request, mSpeed, trajectory);
	}

	void WingSentinelProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy()) return;
		sf::RenderStates additive; additive.blendMode = sf::BlendAdd;
		const float radius = mProfile.projectileRadius;
		sf::CircleShape glow(radius * 2.2f, 18); glow.setOrigin({ radius * 2.2f, radius * 2.2f }); glow.setPosition(GetActorLocation()); glow.setFillColor(mProfile.projectileGlowColor);
		sf::CircleShape body(radius, 14); body.setOrigin({ radius, radius }); body.setPosition(GetActorLocation()); body.setFillColor(mProfile.projectileColor);
		window.draw(glow, additive); window.draw(body, additive);
	}

	bool RegisterWingSentinelProjectileActorType()
	{
		return AbilityActorRegistry::RegisterHandler(std::make_unique<TypeHandler>());
	}
}
