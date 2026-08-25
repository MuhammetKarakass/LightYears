#include "gameplay/ability/crescentReaver/CrescentReaverProjectileActor.h"

#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/crescentReaver/CrescentReaverContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "framework/World.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace ly
{
	namespace
	{
		constexpr float MaximumMovementSubstep = 12.f;
		constexpr float CollisionSeparation = 2.f;
		constexpr float MinimumDirectionLength = 0.001f;

		const List<sas::AttributeId> CrescentReaverProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CollisionAttributeIds::Radius
		};

		const List<sas::AttributeId> CrescentReaverProjectileAttributeRoots{
			AbilityData::CrescentReaver::Actor::Projectile::Root,
			DamageAttributeIds::Root
		};

		sf::Vector2f NormalizeOrDefault(const sf::Vector2f& direction)
		{
			const float length = std::sqrt(
				direction.x * direction.x + direction.y * direction.y
			);
			return length > MinimumDirectionLength
				? direction / length
				: sf::Vector2f{ 0.f, -1.f };
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

		bool IsOutsideActiveWorldView(
			const Actor& actor,
			float allowance
		)
		{
			const World* world = actor.GetWorld();
			if (!world || !world->GetApplication())
			{
				// Headless tests have no camera. Leaving cleanup disabled there keeps
				// this presentation-only policy out of gameplay simulation tests.
				return false;
			}

			const sf::View view = world->GetWorldView();
			const sf::Vector2f halfViewSize = view.getSize() * 0.5f;
			const sf::Vector2f minimum = view.getCenter() - halfViewSize;
			const sf::Vector2f maximum = view.getCenter() + halfViewSize;
			const sf::Vector2f location = actor.GetActorLocation();
			const float margin = std::max(0.f, allowance);
			return location.x < minimum.x - margin ||
				location.x > maximum.x + margin ||
				location.y < minimum.y - margin ||
				location.y > maximum.y + margin;
		}

		bool IsPointInsideExpandedBounds(
			const sf::Vector2f& point,
			const sf::FloatRect& bounds,
			float expansion
		)
		{
			const float left = bounds.position.x - expansion;
			const float right = bounds.position.x + bounds.size.x + expansion;
			const float top = bounds.position.y - expansion;
			const float bottom = bounds.position.y + bounds.size.y + expansion;
			return point.x >= left && point.x <= right &&
				point.y >= top && point.y <= bottom;
		}

		bool SweepExpandedBounds(
			const sf::Vector2f& start,
			const sf::Vector2f& end,
			const sf::FloatRect& bounds,
			float expansion,
			float& normalizedTime,
			sf::Vector2f& normal
		)
		{
			const float left = bounds.position.x - expansion;
			const float right = bounds.position.x + bounds.size.x + expansion;
			const float top = bounds.position.y - expansion;
			const float bottom = bounds.position.y + bounds.size.y + expansion;
			const sf::Vector2f delta = end - start;
			float enter = 0.f;
			float exit = 1.f;
			sf::Vector2f enterNormal{};

			const auto Clip = [&](float origin,
				float direction,
				float minimum,
				float maximum,
				sf::Vector2f minimumNormal,
				sf::Vector2f maximumNormal)
			{
				if (std::abs(direction) <= MinimumDirectionLength)
				{
					return origin >= minimum && origin <= maximum;
				}

				float nearTime = (minimum - origin) / direction;
				float farTime = (maximum - origin) / direction;
				sf::Vector2f nearNormal = minimumNormal;
				if (nearTime > farTime)
				{
					std::swap(nearTime, farTime);
					nearNormal = maximumNormal;
				}
				if (nearTime > enter)
				{
					enter = nearTime;
					enterNormal = nearNormal;
				}
				exit = std::min(exit, farTime);
				return enter <= exit;
			};

			if (!Clip(
				start.x,
				delta.x,
				left,
				right,
				sf::Vector2f{ -1.f, 0.f },
				sf::Vector2f{ 1.f, 0.f }
			) || !Clip(
				start.y,
				delta.y,
				top,
				bottom,
				sf::Vector2f{ 0.f, -1.f },
				sf::Vector2f{ 0.f, 1.f }
			))
			{
				return false;
			}

			if (enter < 0.f || enter > 1.f || exit < 0.f)
			{
				return false;
			}
			normalizedTime = std::clamp(enter, 0.f, 1.f);
			normal = GetVectorLength(enterNormal) > MinimumDirectionLength
				? enterNormal
				: -NormalizeOrDefault(delta);
			return true;
		}

		class CrescentReaverProjectileActorType final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::CrescentReaverProjectile;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return CrescentReaverProjectileAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return CrescentReaverProjectileCommonAttributes;
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
					CollisionAttributeIds::Radius,
					AbilityData::CrescentReaver::Actor::Projectile::ProjectileSpeed,
					AbilityData::CrescentReaver::Actor::Projectile::BounceCount,
					AbilityData::CrescentReaver::Actor::Projectile::BounceDamageGrowth,
					AbilityData::CrescentReaver::Actor::Projectile::BounceCooldownReduction
				})
				{
					if (!sas::FindAttribute(definition.attributes, required))
					{
						return {
							false,
							"Crescent Reaver projectile is missing required attribute '" +
								std::string{ required.GetName() } + "'."
						};
					}
				}

			const float speed = sas::FindAttributeValue(
				definition.attributes,
				AbilityData::CrescentReaver::Actor::Projectile::ProjectileSpeed
			);
			const float bounceCount = sas::FindAttributeValue(
				definition.attributes,
				AbilityData::CrescentReaver::Actor::Projectile::BounceCount
			);
			const float damageGrowth = sas::FindAttributeValue(
				definition.attributes,
				AbilityData::CrescentReaver::Actor::Projectile::BounceDamageGrowth
			);
			const float cooldownReduction = sas::FindAttributeValue(
				definition.attributes,
				AbilityData::CrescentReaver::Actor::Projectile::BounceCooldownReduction
			);
			const float radius = sas::FindAttributeValue(
				definition.attributes,
				CollisionAttributeIds::Radius
			);
			if (speed <= 0.f || bounceCount < 0.f ||
				std::round(bounceCount) != bounceCount || damageGrowth < 0.f ||
				cooldownReduction < 0.f || radius <= 0.f ||
				!definition.presentationProfileId.IsValid() ||
				PresentationProfileRegistry<CrescentReaverPresentationProfile>::Find(
					definition.presentationProfileId.ToString()
				) == nullptr)
			{
				return {
					false,
					"Crescent Reaver projectile requires valid speed, integer bounce count, non-negative bounce values, collision radius, and a registered presentation profile."
				};
			}
			return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const CrescentReaverPresentationProfile* profile =
					PresentationProfileRegistry<CrescentReaverPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<CrescentReaverProjectileActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	CrescentReaverProjectileActor::CrescentReaverProjectileActor(
		World* world,
		Actor* owner,
		const CrescentReaverPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile{ presentationProfile }
	{
		SetRenderLayer(RenderLayer::Projectile);
	}

	void CrescentReaverProjectileActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		if (GetVectorLength(mLaunchVelocity) <= MinimumDirectionLength)
		{
			mLaunchVelocity = NormalizeOrDefault(GetActorForwardDirection()) *
				mProjectileSpeed;
		}
		SetVelocity(mLaunchVelocity);
	}

	void CrescentReaverProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mInitialDamage = GetDamage();
		mProjectileSpeed = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::CrescentReaver::Actor::Projectile::ProjectileSpeed,
				mProjectileSpeed
			)
		);
		mBounceCountRemaining = std::max(
			0,
			static_cast<int>(std::round(sas::FindAttributeValue(
				attributes,
				AbilityData::CrescentReaver::Actor::Projectile::BounceCount,
				static_cast<float>(mBounceCountRemaining)
			))))
		;
		mCompletedBounceCount = 0;
		mBounceDamageGrowth = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::CrescentReaver::Actor::Projectile::BounceDamageGrowth,
				mBounceDamageGrowth
			)
		);
		mBounceCooldownReduction = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::CrescentReaver::Actor::Projectile::BounceCooldownReduction,
				mBounceCooldownReduction
			)
		);
		mLaunchVelocity = NormalizeOrDefault(GetActorForwardDirection()) *
			mProjectileSpeed;
	}

	void CrescentReaverProjectileActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}
		const float safeDeltaTime = std::max(0.f, deltaTime);
		mSpinDegrees = std::fmod(
			mSpinDegrees + mPresentationProfile.visual.spinDegreesPerSecond * safeDeltaTime,
			360.f
		);
		for (ImpactPulse& pulse : mImpactPulses)
		{
			pulse.age += safeDeltaTime;
		}
		mImpactPulses.erase(
			std::remove_if(
				mImpactPulses.begin(),
				mImpactPulses.end(),
				[&](const ImpactPulse& pulse)
				{
					return pulse.age >= mPresentationProfile.visual.impactDuration;
				}
			),
			mImpactPulses.end()
		);

		if (mIsDissipating)
		{
			mDissipationAge += safeDeltaTime;
			if (mDissipationAge >= std::max(
				0.001f,
				mPresentationProfile.visual.dissipationDuration
			))
			{
				Destroy();
			}
		}
		else if (!GetIsPendingDestroy())
		{
			Move(safeDeltaTime);
		}
		AbilityWorldActor::Tick(safeDeltaTime);
	}

	void CrescentReaverProjectileActor::Move(float deltaTime)
	{
		if (mProjectileSpeed <= 0.f || deltaTime <= 0.f)
		{
			return;
		}

		if (IsOutsideActiveWorldView(*this, 256.f))
		{
			// Arena levels use world coordinates larger than the physical window.
			// Actor::IsActorOutOfWindow compares against screen coordinates, which
			// would immediately remove a projectile spawned near the arena centre.
			// This remains a technical cleanup, but measures from the active camera.
			Destroy();
			return;
		}

		const float distance = mProjectileSpeed * deltaTime;
		const int substeps = std::clamp(
			static_cast<int>(std::ceil(distance / MaximumMovementSubstep)),
			1,
			32
		);
		const float stepTime = deltaTime / static_cast<float>(substeps);
		for (int step = 0; step < substeps && !GetIsPendingDestroy(); ++step)
		{
			AdvanceWithBounce(stepTime);
		}
	}

	bool CrescentReaverProjectileActor::AdvanceWithBounce(float deltaTime)
	{
		float remainingTime = deltaTime;
		for (int collisionGuard = 0;
			collisionGuard < 8 && remainingTime > 0.00001f && !GetIsPendingDestroy();
			++collisionGuard)
		{
			if (mIgnoredCollisionActor &&
				!IsPointInsideExpandedBounds(
					GetActorLocation(),
					mIgnoredCollisionActor->GetActorGlobalBounds(),
					std::max(0.f, GetPhysicsCollisionRadius())
				))
			{
				mIgnoredCollisionActor = nullptr;
			}

			const sf::Vector2f startLocation = GetActorLocation();
			const sf::Vector2f endLocation = startLocation + mLaunchVelocity * remainingTime;
			const std::optional<SweepHit> hit = FindNearestCollision(
				startLocation,
				endLocation
			);
			if (!hit)
			{
				AddActorLocationOffset(endLocation - startLocation);
				return true;
			}

			const sf::Vector2f hitLocation = startLocation +
				(endLocation - startLocation) * hit->normalizedTime;
			SetActorLocation(hitLocation);
			remainingTime *= 1.f - hit->normalizedTime;
			if (!TryProcessCollision(*hit))
			{
				return false;
			}

			// Move a tiny distance away from the expanded hit bounds so the same
			// contact cannot be reported repeatedly on the next swept segment.
			AddActorLocationOffset(
				NormalizeOrDefault(mLaunchVelocity) * CollisionSeparation
			);
			remainingTime = std::max(
				0.f,
				remainingTime - CollisionSeparation / mProjectileSpeed
			);
		}
		return !GetIsPendingDestroy();
	}

	std::optional<CrescentReaverProjectileActor::SweepHit>
		CrescentReaverProjectileActor::FindNearestCollision(
			const sf::Vector2f& startLocation,
			const sf::Vector2f& endLocation
		) const
	{
		World* world = GetWorld();
		if (!world)
		{
			return std::nullopt;
		}

		std::optional<SweepHit> nearest;
		const float collisionRadius = std::max(0.f, GetPhysicsCollisionRadius());
		for (const weak_ptr<Actor>& actorWeak : world->GetActorsInBounds(
			targeting::swept::SegmentBounds(
				startLocation,
				endLocation,
				collisionRadius
			)
		))
		{
			const shared_ptr<Actor> candidate = actorWeak.lock();
			if (!candidate || candidate.get() == this ||
				candidate.get() == mIgnoredCollisionActor ||
				candidate->GetIsPendingDestroy())
			{
				continue;
			}

			const bool isEnemy = candidate->GetCollisionLayer() == CollisionLayer::Enemy &&
				IsValidAbilityTarget(candidate.get());
			const bool isSurface = candidate->GetCollisionLayer() == CollisionLayer::Environment;
			if (!isEnemy && !isSurface)
			{
				continue;
			}

			float normalizedTime = 0.f;
			sf::Vector2f normal{};
			if (!SweepExpandedBounds(
				startLocation,
				endLocation,
				candidate->GetActorGlobalBounds(),
				collisionRadius,
				normalizedTime,
				normal
			))
			{
				continue;
			}

			if (!nearest || normalizedTime < nearest->normalizedTime)
			{
				nearest = SweepHit{ candidate.get(), normalizedTime, normal, isSurface };
			}
		}
		return nearest;
	}

	bool CrescentReaverProjectileActor::TryProcessCollision(const SweepHit& hit)
	{
		if (!hit.actor || GetIsPendingDestroy())
		{
			return false;
		}

		if (mIsDissipating || !hit.actor || GetIsPendingDestroy())
		{
			return false;
		}

		if (!hit.isSurface)
		{
			ApplyCombatDamage(
				*hit.actor,
				GetDamage(),
				GetOwnerActor(),
				GetDamageTags(),
				GetDamagePayload(),
				GetSourceAbilityId(),
				GetSourceAbilityTags(),
				DamageDeliveryType::Projectile,
				this
			);
		}
		mImpactPulses.push_back(ImpactPulse{ GetActorLocation(), 0.f });

		if (mBounceCountRemaining <= 0)
		{
			BeginDissipation();
			return false;
		}

		--mBounceCountRemaining;
		++mCompletedBounceCount;
		ReduceSourceAbilityCooldown();
		SetDamage(
			mInitialDamage * (1.f +
				static_cast<float>(mCompletedBounceCount) * mBounceDamageGrowth)
		);

		const sf::Vector2f incoming = NormalizeOrDefault(mLaunchVelocity);
		const sf::Vector2f surfaceNormal = NormalizeOrDefault(hit.normal);
		const sf::Vector2f reflected = incoming -
			2.f * (incoming.x * surfaceNormal.x + incoming.y * surfaceNormal.y) *
			surfaceNormal;
		mLaunchVelocity = NormalizeOrDefault(reflected) * mProjectileSpeed;
		SetVelocity(mLaunchVelocity);
		SetActorRotation(std::atan2(mLaunchVelocity.y, mLaunchVelocity.x) *
			57.2957795131f + 90.f);
		mIgnoredCollisionActor = hit.actor;
		return true;
	}

	void CrescentReaverProjectileActor::ReduceSourceAbilityCooldown()
	{
		if (GameAbility* ability = GetSourceAbilityInstance())
		{
			ability->ReduceCooldownRemaining(mBounceCooldownReduction);
		}
	}

	void CrescentReaverProjectileActor::BeginDissipation()
	{
		if (mIsDissipating || GetIsPendingDestroy())
		{
			return;
		}

		mIsDissipating = true;
		mDissipationAge = 0.f;
		mLaunchVelocity = {};
		SetVelocity({});
		// The actor remains alive only for presentation. Removing its collision
		// participation immediately prevents the final-hit effect from dealing
		// damage or being captured by Relay Prism a second time.
		SetAbilityPhysicsEnabled(false);
		SetAbilityCollisionRadius(0.f);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void CrescentReaverProjectileActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (mIsDissipating)
		{
			return;
		}

		// Damage and reflection are resolved by the swept path above. The physics
		// callback is intentionally limited to relay capture so a fast projectile
		// cannot apply the same hit twice in one frame.
		if (auto* captureVolume = dynamic_cast<ProjectileCaptureVolume*>(otherActor);
			captureVolume)
		{
			captureVolume->TryCaptureProjectile(*this);
		}
	}

	void CrescentReaverProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}
		if (mIsDissipating)
		{
			DrawDissipation(window);
			return;
		}

		AbilityWorldActor::Render(window);
		DrawCrescent(window);
		DrawImpacts(window);
	}

	void CrescentReaverProjectileActor::DrawCrescent(
		sf::RenderWindow& window,
		float opacity,
		float scale
	) const
	{
		const CrescentReaverVisualDefinition& visual = mPresentationProfile.visual;
		const float safeScale = std::max(0.f, scale);
		const float safeOpacity = std::clamp(opacity, 0.f, 1.f);
		const sf::Vector2f direction = NormalizeOrDefault(mLaunchVelocity);
		const sf::Vector2f tail = GetActorLocation() - direction *
			std::min(visual.trailLength * safeScale,
				GetVectorLength(mLaunchVelocity) * GetAge());
		const sf::Vector2f perpendicular{ -direction.y, direction.x };

		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;
		sf::ConvexShape trail;
		trail.setPointCount(4);
		trail.setPoint(0, tail - perpendicular * (visual.trailWidth * safeScale * 0.15f));
		trail.setPoint(1, GetActorLocation() - perpendicular * (visual.trailWidth * safeScale * 0.50f));
		trail.setPoint(2, GetActorLocation() + perpendicular * (visual.trailWidth * safeScale * 0.50f));
		trail.setPoint(3, tail + perpendicular * (visual.trailWidth * safeScale * 0.15f));
		trail.setFillColor(WithAlpha(visual.glowColor, 0.70f * safeOpacity));
		window.draw(trail, additiveStates);

		const float pulse = 0.88f + 0.12f * std::sin(GetAge() * 18.f);
		const float glowRadius = std::max(1.f, visual.glowRadius * safeScale);
		sf::CircleShape glow(glowRadius, 32);
		glow.setOrigin({ glowRadius, glowRadius });
		glow.setPosition(GetActorLocation());
		glow.setFillColor(WithAlpha(visual.glowColor, pulse * 0.55f * safeOpacity));
		window.draw(glow, additiveStates);

		constexpr int ArcSegments = 18;
		constexpr float StartAngle = -50.f;
		constexpr float ArcDegrees = 300.f;
		const float innerRadius = std::max(
			1.f,
			visual.radius - std::max(1.f, visual.bladeThickness)
		);
		sf::ConvexShape edge;
		edge.setPointCount((ArcSegments + 1) * 2);
		for (int index = 0; index <= ArcSegments; ++index)
		{
			const float angle = (StartAngle + ArcDegrees *
				static_cast<float>(index) / static_cast<float>(ArcSegments)) *
				0.0174532925199433f;
				edge.setPoint(static_cast<std::size_t>(index), {
					std::cos(angle) * (visual.radius * safeScale + 2.f * safeScale),
					std::sin(angle) * (visual.radius * safeScale + 2.f * safeScale)
				});
		}
		for (int index = 0; index <= ArcSegments; ++index)
		{
			const float angle = (StartAngle + ArcDegrees - ArcDegrees *
				static_cast<float>(index) / static_cast<float>(ArcSegments)) *
				0.0174532925199433f;
				edge.setPoint(static_cast<std::size_t>(ArcSegments + 1 + index), {
					std::cos(angle) * std::max(1.f, (innerRadius - 1.f) * safeScale),
					std::sin(angle) * std::max(1.f, (innerRadius - 1.f) * safeScale)
				});
		}
		edge.setPosition(GetActorLocation());
		edge.setRotation(sf::degrees(mSpinDegrees));
		edge.setFillColor(WithAlpha(visual.edgeColor, pulse * safeOpacity));
		window.draw(edge, additiveStates);

		sf::ConvexShape blade = edge;
		blade.setPosition(GetActorLocation());
		blade.setFillColor(WithAlpha(visual.bladeColor, pulse * safeOpacity));
		window.draw(blade, additiveStates);
	}

	void CrescentReaverProjectileActor::DrawDissipation(sf::RenderWindow& window) const
	{
		const CrescentReaverVisualDefinition& visual = mPresentationProfile.visual;
		const float duration = std::max(0.001f, visual.dissipationDuration);
		const float progress = std::clamp(mDissipationAge / duration, 0.f, 1.f);
		const float fade = 1.f - progress;
		const float easedProgress = progress * progress * (3.f - 2.f * progress);

		// The crescent expands slightly and fades while a ring flashes outward,
		// making the final bounce visually distinct from an ordinary hit.
		DrawCrescent(window, fade, 1.f + easedProgress * 0.35f);

		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;
		const float ringRadius = std::max(
			1.f,
			visual.impactRadius * (0.75f + easedProgress * 1.75f)
		);
		sf::CircleShape ring(ringRadius, 48);
		ring.setOrigin({ ringRadius, ringRadius });
		ring.setPosition(GetActorLocation());
		ring.setFillColor(sf::Color::Transparent);
		ring.setOutlineThickness(std::max(1.f, visual.impactRingThickness));
		ring.setOutlineColor(WithAlpha(visual.impactRingColor, fade));
		window.draw(ring, additiveStates);

		const float flashRadius = std::max(1.f, visual.impactRadius *
			(0.55f + easedProgress * 0.80f));
		sf::CircleShape flash(flashRadius, 32);
		flash.setOrigin({ flashRadius, flashRadius });
		flash.setPosition(GetActorLocation());
		flash.setFillColor(WithAlpha(visual.impactColor, fade * 0.35f));
		window.draw(flash, additiveStates);
	}

	void CrescentReaverProjectileActor::DrawImpacts(sf::RenderWindow& window) const
	{
		const CrescentReaverVisualDefinition& visual = mPresentationProfile.visual;
		const float duration = std::max(0.001f, visual.impactDuration);
		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;
		for (const ImpactPulse& pulse : mImpactPulses)
		{
			const float progress = std::clamp(pulse.age / duration, 0.f, 1.f);
			const float fade = 1.f - progress;
			const float radius = std::max(1.f, visual.impactRadius);

			sf::CircleShape flash(radius, 32);
			flash.setOrigin({ radius, radius });
			flash.setPosition(pulse.location);
			flash.setScale({ 0.35f + progress * 0.65f, 0.35f + progress * 0.65f });
			flash.setFillColor(WithAlpha(visual.impactColor, fade));

			sf::CircleShape ring(radius, 48);
			ring.setOrigin({ radius, radius });
			ring.setPosition(pulse.location);
			ring.setScale({ 0.5f + progress, 0.5f + progress });
			ring.setFillColor(sf::Color::Transparent);
			ring.setOutlineThickness(std::max(0.f, visual.impactRingThickness));
			ring.setOutlineColor(WithAlpha(visual.impactRingColor, fade));

			window.draw(flash, additiveStates);
			window.draw(ring, additiveStates);
		}
	}

	weak_ptr<AbilityWorldActor> CrescentReaverProjectileActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return {};
		}

		weak_ptr<CrescentReaverProjectileActor> clone =
			world->SpawnActor<CrescentReaverProjectileActor>(
				owner,
				mPresentationProfile
			);
		if (const shared_ptr<CrescentReaverProjectileActor> spawned = clone.lock())
		{
			spawned->ConfigureFromAttributes(request.snapshot.damageAttributes);
			spawned->ConfigureRelayClone(request);
			spawned->SetDamage(request.damage);
			// ConfigureRelayClone owns the common position/rotation/collision state,
			// but Crescent movement is driven by this family's cached velocity. Keep
			// it in sync with the Prism scatter direction so each relay clone leaves
			// on its own trajectory instead of all four overlapping visually.
			spawned->mLaunchVelocity = NormalizeOrDefault(request.direction) *
				spawned->mProjectileSpeed;
			spawned->SetVelocity(spawned->mLaunchVelocity);
			// A relay changes the initial hit value. Subsequent bounce growth must
			// scale from that converted damage rather than reverting to the source.
			spawned->mInitialDamage = request.damage;
			spawned->SetSourceAbilityInstance(GetSourceAbilityInstance());
			spawned->SetLifeTime(0.f);
		}
		return clone;
	}

	bool RegisterCrescentReaverProjectileActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<CrescentReaverProjectileActorType>()
		);
	}
}
