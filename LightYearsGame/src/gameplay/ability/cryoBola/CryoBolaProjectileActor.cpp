#include "gameplay/ability/cryoBola/CryoBolaProjectileActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/cryoBola/CryoBolaContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

namespace ly
{
	namespace
	{
		constexpr float MaximumMovementSubstep = 12.f;
		constexpr float DegreesPerRadian = 57.2957795131f;
		constexpr float RelayLifetimeMarginSeconds = 0.35f;

		const List<sas::AttributeId> ProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius
		};
		const List<sas::AttributeId> ProjectileAttributeRoots{
			AbilityData::CryoBola::Actor::Projectile::Root,
			DamageAttributeIds::Root
		};

		sf::Vector2f NormalizeOrDefault(const sf::Vector2f& direction)
		{
			const float length = GetVectorLength(direction);
			return length > 0.001f ? direction / length : sf::Vector2f{ 0.f, -1.f };
		}

		sf::Color WithAlpha(const sf::Color& color, float alphaMultiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * std::clamp(alphaMultiplier, 0.f, 1.f),
				0.f,
				255.f
			));
			return result;
		}

		class CryoBolaProjectileActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::CryoBolaProjectile;
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
					CommonAttributeIds::Range,
					CollisionAttributeIds::Radius,
					AbilityData::CryoBola::Actor::Projectile::ProjectileSpeed,
					AbilityData::CryoBola::Actor::Projectile::RuptureRadius,
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
					if (!attribute || !std::isfinite(attribute->baseValue))
					{
						return { false, "Cryo Bola projectile is missing required attribute '" +
							std::string{ required.GetName() } + "'." };
					}
				}

				const float speed = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::CryoBola::Actor::Projectile::ProjectileSpeed,
					0.f
				);
				const float range = sas::FindAttributeValue(
					definition.attributes,
					CommonAttributeIds::Range,
					0.f
				);
				const float ruptureRadius = sas::FindAttributeValue(
					definition.attributes,
					AbilityData::CryoBola::Actor::Projectile::RuptureRadius,
					0.f
				);
				const int requiredStacks = static_cast<int>(std::lround(
					sas::FindAttributeValue(
						definition.attributes,
						DamageAttributeIds::CryoBuildupRequired,
						0.f
					)
				));
				const int appliedStacks = static_cast<int>(std::lround(
					sas::FindAttributeValue(
						definition.attributes,
						DamageAttributeIds::CryoBuildupPerHit,
						0.f
					)
				));
				if (speed <= 0.f || range <= 0.f || ruptureRadius <= 0.f ||
					requiredStacks <= 0 || appliedStacks != requiredStacks ||
					definition.lifeTime <= range / speed ||
					!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<CryoBolaPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return {
						false,
						"Cryo Bola projectile requires positive flight values, a full Cryo stack payload, cleanup time, and a registered presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const CryoBolaPresentationProfile* profile =
					PresentationProfileRegistry<CryoBolaPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<CryoBolaProjectileActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	CryoBolaProjectileActor::CryoBolaProjectileActor(
		World* world,
		Actor* owner,
		const CryoBolaPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile(presentationProfile)
	{
		SetRenderLayer(RenderLayer::Projectile);
	}

	void CryoBolaProjectileActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		mLaunchVelocity = NormalizeOrDefault(GetActorForwardDirection()) * mProjectileSpeed;
		SetVelocity(mLaunchVelocity);
	}

	void CryoBolaProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mProjectileSpeed = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::CryoBola::Actor::Projectile::ProjectileSpeed,
			mProjectileSpeed
		));
		mMaximumRange = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Range,
			mMaximumRange
		));
		mRuptureRadius = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::CryoBola::Actor::Projectile::RuptureRadius,
			mRuptureRadius
		));
	}

	void CryoBolaProjectileActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (mHitResolved)
		{
			mRuptureAge += safeDeltaTime;
			if (mRuptureAge >= mPresentationProfile.visual.ruptureDuration)
			{
				Destroy();
			}
		}
		else if (!mResolved && !GetIsPendingDestroy())
		{
			Move(safeDeltaTime);
		}
		AbilityWorldActor::Tick(safeDeltaTime);
	}

	void CryoBolaProjectileActor::Move(float deltaTime)
	{
		if (mProjectileSpeed <= 0.f || deltaTime <= 0.f)
		{
			return;
		}
		if (GetVectorLength(mLaunchVelocity) <= 0.001f)
		{
			mLaunchVelocity = NormalizeOrDefault(GetActorForwardDirection()) * mProjectileSpeed;
		}
		SetVelocity(mLaunchVelocity);

		const float distance = mProjectileSpeed * deltaTime;
		const int substeps = std::clamp(
			static_cast<int>(std::ceil(distance / MaximumMovementSubstep)),
			1,
			32
		);
		const float stepTime = deltaTime / static_cast<float>(substeps);
		for (int step = 0; step < substeps && !mResolved && !GetIsPendingDestroy(); ++step)
		{
			const sf::Vector2f startLocation = GetActorLocation();
			AddActorLocationOffset(mLaunchVelocity * stepTime);
			ApplySweptHit(startLocation, GetActorLocation());
			mTravelDistance += mProjectileSpeed * stepTime;
			if (!mResolved && mMaximumRange > 0.f && mTravelDistance >= mMaximumRange)
			{
				ResolveMiss();
			}
		}
	}

	void CryoBolaProjectileActor::ApplySweptHit(
		const sf::Vector2f& startLocation,
		const sf::Vector2f& endLocation
	)
	{
		World* world = GetWorld();
		if (!world || mResolved || GetIsPendingDestroy())
		{
			return;
		}

		const float collisionRadius = std::max(0.f, GetPhysicsCollisionRadius());
		for (const weak_ptr<Actor>& actorWeak : world->GetActorsInBounds(
			targeting::swept::SegmentBounds(startLocation, endLocation, collisionRadius)
		))
		{
			const shared_ptr<Actor> candidate = actorWeak.lock();
			if (!candidate || candidate.get() == this ||
				!targeting::swept::SegmentIntersectsExpandedBounds(
					startLocation,
					endLocation,
					candidate->GetActorGlobalBounds(),
					collisionRadius
				))
			{
				continue;
			}
			if (auto* capture = dynamic_cast<ProjectileCaptureVolume*>(candidate.get());
				capture && capture->TryCaptureProjectile(*this))
			{
				return;
			}
			if (IsValidAbilityTarget(candidate.get()))
			{
				ResolveHit(*candidate);
				return;
			}
		}
	}

	void CryoBolaProjectileActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (mResolved || GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		if (TryReflectOnOverlap(otherActor))
		{
			return;
		}
		if (auto* capture = dynamic_cast<ProjectileCaptureVolume*>(otherActor);
			capture && capture->TryCaptureProjectile(*this))
		{
			return;
		}
		if (IsValidAbilityTarget(otherActor))
		{
			ResolveHit(*otherActor);
			return;
		}
		if (otherActor && otherActor->GetPhysicsBodyType() == PhysicsBodyType::Static)
		{
			// Geometry is a miss: unlike an enemy impact it must not create a rupture.
			ResolveMiss();
		}
	}

	bool CryoBolaProjectileActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		return !mResolved && ApplyBallisticReflection(
			request,
			mProjectileSpeed,
			mLaunchVelocity
		);
	}

	void CryoBolaProjectileActor::ResolveHit(Actor& primaryTarget)
	{
		if (mResolved)
		{
			return;
		}
		mResolved = true;
		mHitResolved = true;
		mRuptureAge = 0.f;
		SetVelocity({});
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetActorLocation(primaryTarget.GetActorLocation());
		ApplyRupture(primaryTarget);
	}

	void CryoBolaProjectileActor::ResolveMiss()
	{
		if (mResolved)
		{
			return;
		}
		mResolved = true;
		Destroy();
	}

	void CryoBolaProjectileActor::ApplyRupture(Actor& primaryTarget)
	{
		Actor* owner = GetOwnerActor();
		World* world = GetWorld();
		if (!owner || !world || owner->GetIsPendingDestroy())
		{
			return;
		}

		// Direct target is resolved exactly once. The target query below explicitly
		// excludes it so the rupture cannot double damage or double-apply Cryo.
		ApplyCombatDamage(
			primaryTarget,
			GetDamage(),
			owner,
			GetDamageTags(),
			GetDamagePayload(),
			GetSourceAbilityId(),
			GetSourceAbilityTags(),
			DamageDeliveryType::Projectile,
			this
		);

		for (const shared_ptr<Actor>& target : targeting::FindOpposingCombatants(
			*world,
			*owner,
			GetActorLocation(),
			mRuptureRadius
		))
		{
			if (!target || target.get() == &primaryTarget ||
				target->GetIsPendingDestroy())
			{
				continue;
			}
			ApplyCombatDamage(
				*target,
				GetDamage(),
				owner,
				GetDamageTags(),
				GetDamagePayload(),
				GetSourceAbilityId(),
				GetSourceAbilityTags(),
				DamageDeliveryType::Area,
				this
			);
		}
	}

	void CryoBolaProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		if (mHitResolved)
		{
			DrawRupture(window);
		}
		else
		{
			DrawProjectile(window);
		}
	}

	void CryoBolaProjectileActor::DrawProjectile(sf::RenderWindow& window) const
	{
		const CryoBolaVisualDefinition& visual = mPresentationProfile.visual;
		const sf::Vector2f location = GetActorLocation();
		const sf::Vector2f forward = NormalizeOrDefault(mLaunchVelocity);
		const sf::Vector2f right{ -forward.y, forward.x };
		const float angle = GetAge() * visual.spinRadiansPerSecond;
		const sf::Vector2f orbitOffset = right * std::sin(angle) * visual.orbOrbitRadius +
			forward * std::cos(angle) * visual.orbOrbitRadius;
		const float pulse = 0.88f + 0.12f * std::sin(GetAge() * 9.f);

		sf::VertexArray tether(sf::PrimitiveType::Lines, 2);
		tether[0].position = location - orbitOffset;
		tether[1].position = location + orbitOffset;
		tether[0].color = WithAlpha(visual.tetherColor, pulse);
		tether[1].color = WithAlpha(visual.tetherColor, pulse);

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(tether, additive);
		for (const float direction : { -1.f, 1.f })
		{
			const sf::Vector2f orbLocation = location + orbitOffset * direction;
			sf::CircleShape glow(visual.orbRadius * 1.9f, 24);
			glow.setOrigin({ visual.orbRadius * 1.9f, visual.orbRadius * 1.9f });
			glow.setPosition(orbLocation);
			glow.setFillColor(WithAlpha(visual.orbGlowColor, pulse));
			sf::CircleShape orb(visual.orbRadius, 24);
			orb.setOrigin({ visual.orbRadius, visual.orbRadius });
			orb.setPosition(orbLocation);
			orb.setFillColor(WithAlpha(visual.orbColor, pulse));
			window.draw(glow, additive);
			window.draw(orb, additive);
		}
	}

	void CryoBolaProjectileActor::DrawRupture(sf::RenderWindow& window) const
	{
		const CryoBolaVisualDefinition& visual = mPresentationProfile.visual;
		const float duration = std::max(0.01f, visual.ruptureDuration);
		const float progress = std::clamp(mRuptureAge / duration, 0.f, 1.f);
		const float radius = visual.ruptureVisualRadius *
			(0.25f + 0.75f * (1.f - (1.f - progress) * (1.f - progress)));
		const sf::Vector2f center = GetActorLocation();
		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;

		sf::CircleShape ring(std::max(1.f, radius), 64);
		ring.setOrigin({ std::max(1.f, radius), std::max(1.f, radius) });
		ring.setPosition(center);
		ring.setFillColor(sf::Color::Transparent);
		ring.setOutlineThickness(visual.ruptureOutlineThickness);
		ring.setOutlineColor(WithAlpha(visual.ruptureColor, 1.f - progress));
		window.draw(ring, additive);

		const int lineCount = std::max(1, visual.ruptureLineCount);
		sf::VertexArray shards(sf::PrimitiveType::Lines,
			static_cast<std::size_t>(lineCount) * 2);
		for (int index = 0; index < lineCount; ++index)
		{
			const float angle = static_cast<float>(index) * 6.28318530718f /
				static_cast<float>(lineCount) + GetAge() * 1.7f;
			const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
			shards[static_cast<std::size_t>(index) * 2].position = center +
				direction * radius * 0.18f;
			shards[static_cast<std::size_t>(index) * 2 + 1].position = center +
				direction * radius;
			shards[static_cast<std::size_t>(index) * 2].color =
				WithAlpha(visual.ruptureLineColor, 1.f - progress);
			shards[static_cast<std::size_t>(index) * 2 + 1].color =
				WithAlpha(visual.ruptureColor, 1.f - progress);
		}
		window.draw(shards, additive);
	}

	weak_ptr<AbilityWorldActor> CryoBolaProjectileActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return {};
		}
		weak_ptr<CryoBolaProjectileActor> clone =
			world->SpawnActor<CryoBolaProjectileActor>(owner, mPresentationProfile);
		if (const shared_ptr<CryoBolaProjectileActor> spawned = clone.lock())
		{
			spawned->ConfigureFromAttributes(request.snapshot.damageAttributes);
			spawned->ConfigureRelayClone(request);
			spawned->SetDamage(request.damage);
			spawned->mTravelDistance = 0.f;
			spawned->mLaunchVelocity = NormalizeOrDefault(request.direction) *
				spawned->mProjectileSpeed;
			spawned->SetVelocity(spawned->mLaunchVelocity);
			// A relay emits a fresh physical bola. Its range starts again here rather
			// than inheriting distance already travelled before the prism.
			spawned->SetLifeTime(std::max(
				request.snapshot.remainingLifetime,
				spawned->mProjectileSpeed > 0.f
					? spawned->mMaximumRange / spawned->mProjectileSpeed +
						RelayLifetimeMarginSeconds
					: request.snapshot.remainingLifetime
			));
		}
		return clone;
	}

	bool RegisterCryoBolaProjectileActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<CryoBolaProjectileActorTypeHandler>()
		);
	}
}
