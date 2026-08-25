#include "gameplay/ability/crystalBarricade/CrystalBarricadeActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/crystalBarricade/CrystalBarricadeContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> WallCommonAttributes{
			CommonAttributeIds::Duration,
			AreaAttributeIds::Length,
			AreaAttributeIds::Width
		};
		const List<sas::AttributeId> WallAttributeRoots{
			AbilityData::CrystalBarricade::Actor::Wall::Root
		};

		class CrystalBarricadeWallActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::CrystalBarricadeWall;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return WallAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return WallCommonAttributes;
			}

			AbilityActorValidationResult ValidateDefinition(
				const AbilityActorDefinition& definition
			) const override
			{
				const AbilityActorValidationResult base = AbilityActorTypeHandler::ValidateDefinition(definition);
				if (!base.isValid)
				{
					return base;
				}
				for (const sas::AttributeId& required : {
					CommonAttributeIds::Duration,
					AreaAttributeIds::Length,
					AreaAttributeIds::Width,
					AbilityData::CrystalBarricade::Actor::Wall::BaseContactDamage,
					AbilityData::CrystalBarricade::Actor::Wall::EnergyMaxContactScale,
					AbilityData::CrystalBarricade::Actor::Wall::ContactInterval,
					AbilityData::CrystalBarricade::Actor::Wall::BaseRicochetMultiplier,
					AbilityData::CrystalBarricade::Actor::Wall::EnergyMaxRicochetReference,
					AbilityData::CrystalBarricade::Actor::Wall::EnergyMaxRicochetScale,
					AbilityData::CrystalBarricade::Actor::Wall::SameSurfaceLockDuration,
					AbilityData::CrystalBarricade::Actor::Wall::MaxHealthDurationReference,
					AbilityData::CrystalBarricade::Actor::Wall::MaxHealthDurationScale
				})
				{
					const sas::GameplayAttribute* value = sas::FindAttribute(definition.attributes, required);
					if (!value || !std::isfinite(value->baseValue) || value->baseValue < 0.f)
					{
						return { false, "Crystal Barricade wall requires finite non-negative physical and combat attributes." };
					}
				}
				if (!definition.presentationProfileId.IsValid() ||
					!PresentationProfileRegistry<CrystalBarricadeWallPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					))
				{
					return { false, "Crystal Barricade wall requires a registered typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(const AbilityActorSpawnContext& context) const override
			{
				World* world = context.owner.GetWorld();
				const auto* profile = PresentationProfileRegistry<CrystalBarricadeWallPresentationProfile>::Find(
					context.definition.presentationProfileId.ToString()
				);
				return world && profile
					? world->SpawnActor<CrystalBarricadeActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};

		float Value(const sas::GameplayAttributeList& values, const sas::AttributeId& id, float fallback)
		{
			return sas::FindAttributeValue(values, id, fallback);
		}
	}

	CrystalBarricadeActor::CrystalBarricadeActor(
		World* world,
		Actor* owner,
		const CrystalBarricadeWallPresentationProfile& profile
	)
		: AbilityWorldActor(world, owner)
		, mProfile(profile)
	{
		SetRenderLayer(RenderLayer::World);
		SetPhysicsBodyType(PhysicsBodyType::Static);
		SetAbilityPhysicsEnabled(true);
	}

	void CrystalBarricadeActor::BeginPlay()
	{
		SetCollisionLayer(CollisionLayer::Environment);
		SetCollisionMask(
			CollisionLayer::Player |
			CollisionLayer::Enemy |
			CollisionLayer::FriendlySummon |
			CollisionLayer::PlayerBullet |
			CollisionLayer::EnemyBullet
		);
		AbilityWorldActor::BeginPlay();
	}

	void CrystalBarricadeActor::ConfigureFromAttributes(const sas::GameplayAttributeList& attributes)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mLength = std::max(1.f, Value(attributes, AreaAttributeIds::Length, mLength));
		mThickness = std::max(1.f, Value(attributes, AreaAttributeIds::Width, mThickness));
		const float energyMax = ResolveOwnerAttribute(OwnerAttributeIds::EnergyMax);
		const float maxHealth = ResolveOwnerAttribute(OwnerAttributeIds::MaxHealth);
		mRemainingDuration = std::max(0.f,
			Value(attributes, CommonAttributeIds::Duration, mRemainingDuration) +
			std::max(0.f, maxHealth - Value(attributes,
				AbilityData::CrystalBarricade::Actor::Wall::MaxHealthDurationReference, 100.f)) *
			Value(attributes, AbilityData::CrystalBarricade::Actor::Wall::MaxHealthDurationScale, 0.005f)
		);
		mContactDamage = std::max(0.f,
			Value(attributes, AbilityData::CrystalBarricade::Actor::Wall::BaseContactDamage, 20.f) +
			energyMax * Value(attributes,
				AbilityData::CrystalBarricade::Actor::Wall::EnergyMaxContactScale, 0.15f)
		);
		mContactInterval = std::max(0.01f, Value(attributes,
			AbilityData::CrystalBarricade::Actor::Wall::ContactInterval, 0.5f));
		mRicochetMultiplier = std::max(0.f,
			Value(attributes, AbilityData::CrystalBarricade::Actor::Wall::BaseRicochetMultiplier, 0.8f) +
			std::max(0.f, energyMax - Value(attributes,
				AbilityData::CrystalBarricade::Actor::Wall::EnergyMaxRicochetReference, 50.f)) *
			Value(attributes, AbilityData::CrystalBarricade::Actor::Wall::EnergyMaxRicochetScale, 0.001f)
		);
		mSameSurfaceLockDuration = std::max(0.f, Value(attributes,
			AbilityData::CrystalBarricade::Actor::Wall::SameSurfaceLockDuration, 0.12f));
		// Lifetime is owned here so physics can be disabled before the brief break
		// visual plays; the generic actor lifetime would destroy both at once.
		SetLifeTime(0.f);
	}

	void CrystalBarricadeActor::Tick(float deltaTime)
	{
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (mRemainingDuration > 0.f)
		{
			mRemainingDuration = std::max(0.f, mRemainingDuration - safeDeltaTime);
			if (mRemainingDuration <= 0.f)
			{
				SetCollisionLayer(CollisionLayer::None);
				SetCollisionMask(CollisionLayer::None);
				SetEnablePhysics(false);
				mBreakTimeRemaining = mProfile.breakVisualDuration;
			}
		}
		else if (mBreakTimeRemaining > 0.f)
		{
			mBreakTimeRemaining = std::max(0.f, mBreakTimeRemaining - safeDeltaTime);
			if (mBreakTimeRemaining <= 0.f)
			{
				Destroy();
			}
		}

		for (auto iterator = mNextContactDamageTime.begin(); iterator != mNextContactDamageTime.end();)
		{
			iterator->second -= safeDeltaTime;
			if (iterator->second <= -mContactInterval)
			{
				iterator = mNextContactDamageTime.erase(iterator);
			}
			else
			{
				++iterator;
			}
		}
		if (mRemainingDuration > 0.f)
		{
			ApplyPeriodicContactDamage();
		}
		AbilityWorldActor::Tick(safeDeltaTime);
	}

	void CrystalBarricadeActor::Render(sf::RenderWindow& window)
	{
		const float duration = std::max(0.001f, mProfile.breakVisualDuration);
		const float breakAlpha = mBreakTimeRemaining > 0.f ? mBreakTimeRemaining / duration : 0.f;
		sf::RectangleShape wall({ mLength, mThickness });
		wall.setOrigin({ mLength * 0.5f, mThickness * 0.5f });
		wall.setPosition(GetActorLocation());
		wall.setRotation(sf::degrees(GetActorRotation()));
		wall.setFillColor(mBreakTimeRemaining > 0.f
			? sf::Color{ mProfile.breakColor.r, mProfile.breakColor.g, mProfile.breakColor.b,
				static_cast<uint8_t>(mProfile.breakColor.a * breakAlpha) }
			: mProfile.coreColor);
		wall.setOutlineThickness(2.f);
		wall.setOutlineColor(mProfile.edgeColor);
		window.draw(wall);
		if (mBreakTimeRemaining > 0.f)
		{
			for (int shard = 0; shard < 8; ++shard)
			{
				const float phase = static_cast<float>(shard) * 0.78539816339f;
				const sf::Vector2f offset{ std::cos(phase) * 42.f * (1.f - breakAlpha),
					std::sin(phase) * 42.f * (1.f - breakAlpha) };
				sf::CircleShape fragment(3.f + 3.f * breakAlpha, 4);
				fragment.setOrigin({ fragment.getRadius(), fragment.getRadius() });
				fragment.setPosition(GetActorLocation() + offset);
				fragment.setFillColor(mProfile.breakColor);
				window.draw(fragment);
			}
		}
	}

	void CrystalBarricadeActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (otherActor)
		{
			TryApplyContactDamage(*otherActor);
		}
	}

	void CrystalBarricadeActor::TryApplyContactDamage(Actor& otherActor)
	{
		if (mRemainingDuration <= 0.f || !CanDamageContactTarget(otherActor))
		{
			return;
		}
		float& remainingCooldown = mNextContactDamageTime[otherActor.GetUniqueID()];
		if (remainingCooldown > 0.f)
		{
			return;
		}
		ApplyCombatDamage(
			otherActor,
			mContactDamage,
			GetOwnerActor(),
			GetDamageTags(),
			GetDamagePayload(),
			GetSourceAbilityId(),
			GetSourceAbilityTags(),
			DamageDeliveryType::Area,
			this
		);
		remainingCooldown = mContactInterval;
	}

	void CrystalBarricadeActor::ApplyPeriodicContactDamage()
	{
		World* world = GetWorld();
		if (!world)
		{
			return;
		}
		const float broadphaseRadius = std::sqrt(
			mLength * mLength + mThickness * mThickness
		) * 0.5f + 64.f;
		const float angle = GetActorRotation() * 0.01745329251994329577f;
		const float cosine = std::cos(angle);
		const float sine = std::sin(angle);
		for (const weak_ptr<Actor>& targetWeak : world->GetActorsInBounds(
			targeting::swept::RadiusBounds(GetActorLocation(), broadphaseRadius)
		))
		{
			const shared_ptr<Actor> target = targetWeak.lock();
			if (!target || !CanDamageContactTarget(*target))
			{
				continue;
			}
			const sf::Vector2f offset = target->GetActorLocation() - GetActorLocation();
			const sf::Vector2f local{
				cosine * offset.x + sine * offset.y,
				-sine * offset.x + cosine * offset.y
			};
			const float targetRadius = std::max(0.f, target->GetPhysicsCollisionRadius());
			if (std::abs(local.x) <= mLength * 0.5f + targetRadius &&
				std::abs(local.y) <= mThickness * 0.5f + targetRadius)
			{
				TryApplyContactDamage(*target);
			}
		}
	}

	sf::Vector2f CrystalBarricadeActor::GetPhysicsCollisionBoxHalfExtents() const
	{
		return { mLength * 0.5f, mThickness * 0.5f };
	}

	bool CrystalBarricadeActor::BuildProjectileReflectionResponse(
		const Actor& incomingProjectile,
		ProjectileReflectionSurfaceResponse& outResponse
	) const
	{
		(void)incomingProjectile;
		if (mRemainingDuration <= 0.f || !GetOwnerActor())
		{
			return false;
		}
		outResponse.newOwner = GetOwnerActor();
		outResponse.damageMultiplier = mRicochetMultiplier;
		outResponse.sameSurfaceLockDuration = mSameSurfaceLockDuration;
		outResponse.allowSameOwnerReflection = true;
		return true;
	}

	bool CrystalBarricadeActor::CanDamageContactTarget(const Actor& actor) const
	{
		return actor.GetCollisionLayer() == CollisionLayer::Enemy &&
			dynamic_cast<const Combatant*>(&actor) != nullptr &&
			!actor.GetIsPendingDestroy();
	}

	float CrystalBarricadeActor::ResolveOwnerAttribute(const sas::AttributeId& id) const
	{
		const auto* owner = dynamic_cast<const Combatant*>(GetOwnerActor());
		return owner
			? owner->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(id)
			: 0.f;
	}

	bool RegisterCrystalBarricadeWallActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<CrystalBarricadeWallActorTypeHandler>()
		);
	}
}
