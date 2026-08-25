#include "gameplay/ability/railBurst/RailBurstProjectileActor.h"

#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/railBurst/RailBurstContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/projectile/ProjectileCaptureVolume.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "framework/World.h"
#include "gameConfigs/combat/DamageTypeConfig.h"

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
		constexpr float RelayDeliveryLifetimeMarginSeconds = 0.05f;

		const List<sas::AttributeId> RailBurstProjectileCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Range,
			CollisionAttributeIds::Radius
		};

		const List<sas::AttributeId> RailBurstProjectileAttributeRoots{
			AbilityData::RailBurst::Actor::Projectile::Root,
			DamageAttributeIds::Root
		};

		sf::Vector2f NormalizeOrDefault(const sf::Vector2f& direction)
		{
			const float length = std::sqrt(
				direction.x * direction.x + direction.y * direction.y
			);
			return length > 0.001f
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

		class RailBurstProjectileActorType final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::RailBurstProjectile;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return RailBurstProjectileAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return RailBurstProjectileCommonAttributes;
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
					AbilityData::RailBurst::Actor::Projectile::ProjectileSpeed
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute)
					{
						return {
							false,
							"Rail Burst projectile is missing required attribute '" +
								std::string{ required.GetName() } + "'."
						};
					}
				}

			const float speed = sas::FindAttributeValue(
				definition.attributes,
				AbilityData::RailBurst::Actor::Projectile::ProjectileSpeed
			);
			const float range = sas::FindAttributeValue(
				definition.attributes,
				CommonAttributeIds::Range
			);
			if (speed <= 0.f || range <= 0.f)
			{
				return {
					false,
					"Rail Burst projectile requires positive speed and range."
				};
			}
			if (definition.lifeTime <= range / speed)
			{
				return {
					false,
					"Rail Burst projectile lifetime must exceed range divided by speed."
				};
			}
			if (!definition.presentationProfileId.IsValid() ||
				PresentationProfileRegistry<RailBurstPresentationProfile>::Find(
					definition.presentationProfileId.ToString()
				) == nullptr)
			{
				return { false, "Rail Burst projectile requires a registered presentation profile." };
			}
			return { true, {} };
		}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const RailBurstPresentationProfile* profile =
					PresentationProfileRegistry<RailBurstPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<RailBurstProjectileActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	RailBurstProjectileActor::RailBurstProjectileActor(
		World* world,
		Actor* owner,
		const RailBurstPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile{ presentationProfile }
	{
		SetRenderLayer(RenderLayer::Projectile);
	}

	void RailBurstProjectileActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		SetVelocity(mLaunchVelocity);
	}

	void RailBurstProjectileActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mProjectileSpeed = std::max(
			0.f,
			sas::FindAttributeValue(
				attributes,
				AbilityData::RailBurst::Actor::Projectile::ProjectileSpeed,
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
	}

	void RailBurstProjectileActor::Tick(float deltaTime)
	{
		if (IsInPortalTransit())
		{
			AbilityWorldActor::Tick(deltaTime);
			return;
		}
		const float safeDeltaTime = std::max(0.f, deltaTime);
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

		if (!GetIsPendingDestroy())
		{
			Move(safeDeltaTime);
		}
		AbilityWorldActor::Tick(safeDeltaTime);
	}

	void RailBurstProjectileActor::Move(float deltaTime)
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

		// High-speed projectiles use bounded substeps so a frame cannot move the
		// rail shot through a narrow enemy collision volume in one transform.
		const float distance = mProjectileSpeed * deltaTime;
		const int substeps = std::clamp(
			static_cast<int>(std::ceil(distance / MaximumMovementSubstep)),
			1,
			32
		);
		const float stepTime = deltaTime / static_cast<float>(substeps);
		for (int step = 0; step < substeps && !GetIsPendingDestroy(); ++step)
		{
			const sf::Vector2f previousLocation = GetActorLocation();
			AddActorLocationOffset(mLaunchVelocity * stepTime);
			ApplySweptHits(previousLocation, GetActorLocation());
			mTravelDistance += mProjectileSpeed * stepTime;
			if (mMaximumRange > 0.f && mTravelDistance >= mMaximumRange)
			{
				Destroy();
			}
		}
	}

	void RailBurstProjectileActor::ApplySweptHits(
		const sf::Vector2f& startLocation,
		const sf::Vector2f& endLocation
	)
	{
		World* world = GetWorld();
		if (!world || GetIsPendingDestroy())
		{
			return;
		}

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
			if (!candidate || candidate.get() == this)
			{
				continue;
			}

			if (auto* captureVolume = dynamic_cast<ProjectileCaptureVolume*>(candidate.get());
				captureVolume &&
				targeting::swept::SegmentIntersectsExpandedBounds(
					startLocation,
					endLocation,
					candidate->GetActorGlobalBounds(),
					collisionRadius
				) && captureVolume->TryCaptureProjectile(*this))
			{
				return;
			}

			if (!IsValidAbilityTarget(candidate.get()) ||
				!targeting::swept::SegmentIntersectsExpandedBounds(
					startLocation,
					endLocation,
					candidate->GetActorGlobalBounds(),
					collisionRadius
				))
			{
				continue;
			}
			TryHitTarget(candidate.get());
		}
	}

	void RailBurstProjectileActor::ApplyPiercingHit(Actor& target)
	{
		const float damage = GetDamage();
		ApplyCombatDamage(
			target,
			damage,
			GetOwnerActor(),
			GetDamageTags(),
			GetDamagePayload(),
			GetSourceAbilityId(),
			GetSourceAbilityTags(),
			DamageDeliveryType::Projectile,
			this
		);
		mImpactPulses.push_back(ImpactPulse{ target.GetActorLocation(), 0.f });
	}

	void RailBurstProjectileActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
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
		TryHitTarget(otherActor);
	}

	bool RailBurstProjectileActor::TryReflectProjectile(
		const ProjectileReflectionRequest& request
	)
	{
		return ApplyBallisticReflection(request, mProjectileSpeed, mLaunchVelocity);
	}

	bool RailBurstProjectileActor::TryHitTarget(Actor* otherActor)
	{
		if (GetIsPendingDestroy() || !otherActor || !IsValidAbilityTarget(otherActor) ||
			!mHitTargets.insert(otherActor).second)
		{
			return false;
		}
		ApplyPiercingHit(*otherActor);
		return true;
	}

	void RailBurstProjectileActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		AbilityWorldActor::Render(window);
		DrawFlight(window);
		DrawImpacts(window);
	}

	void RailBurstProjectileActor::DrawFlight(sf::RenderWindow& window) const
	{
		const RailBurstVisualDefinition& visual = mPresentationProfile.visual;
		const sf::Vector2f location = GetActorLocation();
		const sf::Vector2f forward = NormalizeOrDefault(GetActorForwardDirection());
		const sf::Vector2f perpendicular{ -forward.y, forward.x };
		const float pulse = 0.80f + 0.20f * std::sin(
			GetAge() * std::max(0.f, visual.pulseSpeed)
		);
		const float shaftLength = std::max(1.f, visual.bodyLength);
		const float shaftWidth = std::max(1.f, visual.bodyWidth);
		// The projectile has no sprite and is rendered procedurally. Keep every
		// trail element behind the distance it has actually travelled; otherwise
		// the configured visual trail appears through the owner's hull on frame 1.
		const float configuredTrailLength = std::max(0.f, visual.trailLength);
		const float tailLength = std::min(
			configuredTrailLength,
			std::max(0.f, mTravelDistance)
		);
		const float headLength = std::clamp(
			visual.headLength,
			2.f,
			shaftLength * 0.45f
		);

		// A rail shot is represented by a tapered energy shaft instead of a
		// centered rectangle: the sharp nose and long fading tail communicate
		// direction and speed even when the projectile crosses the screen in one
		// or two frames.
		auto MakeStreak = [&](const sf::Vector2f& streakLocation,
			float streakTailLength,
			float streakNoseLength,
			float frontWidth,
			float tailWidth)
		{
			sf::ConvexShape streak;
			streak.setPointCount(5);
			const sf::Vector2f tail = streakLocation - forward * streakTailLength;
			const sf::Vector2f shoulder = streakLocation - forward *
				std::max(0.f, streakNoseLength * 0.20f);
			const sf::Vector2f nose = streakLocation + forward * streakNoseLength;
			streak.setPoint(0, tail - perpendicular * (tailWidth * 0.10f));
			streak.setPoint(1, shoulder - perpendicular * (frontWidth * 0.50f));
			streak.setPoint(2, nose);
			streak.setPoint(3, shoulder + perpendicular * (frontWidth * 0.50f));
			streak.setPoint(4, tail + perpendicular * (tailWidth * 0.10f));
			return streak;
		};

		sf::RenderStates additiveStates;
		additiveStates.blendMode = sf::BlendAdd;

		const int afterimageCount = std::clamp(visual.afterimageCount, 0, 8);
		const float afterimageDecay = std::clamp(visual.afterimageDecay, 0.05f, 0.95f);
		for (int index = afterimageCount; index >= 1; --index)
		{
			const float offset = std::max(0.f, visual.afterimageSpacing) *
				static_cast<float>(index);
			if (offset >= mTravelDistance)
			{
				continue;
			}
			const float alpha = pulse * std::pow(afterimageDecay, static_cast<float>(index));
			const sf::Vector2f ghostLocation = location - forward * offset;
			sf::ConvexShape afterimage = MakeStreak(
				ghostLocation,
				std::min(
					configuredTrailLength * 0.55f,
					std::max(0.f, mTravelDistance - offset)
				),
				headLength * 0.60f,
				shaftWidth * 0.65f,
				visual.trailWidth * 0.45f
			);
			afterimage.setFillColor(WithAlpha(visual.glowColor, alpha));
			window.draw(afterimage, additiveStates);
		}

		sf::ConvexShape outerStreak = MakeStreak(
			location,
			tailLength,
			headLength,
			shaftWidth,
			visual.trailWidth
		);
		outerStreak.setFillColor(WithAlpha(visual.outerColor, pulse));
		window.draw(outerStreak, additiveStates);

		// Two thin side rails make the projectile read as a railgun discharge,
		// while the white center remains the actual high-energy penetrator.
		const float railLength = shaftLength * 0.82f;
		for (const float side : { -1.f, 1.f })
		{
			sf::RectangleShape rail({
				railLength,
				std::max(0.5f, visual.railThickness)
			});
			// The rail starts at the projectile nose and extends forward. Centering
			// it would create a second artificial trail behind the launch point.
			rail.setOrigin({ 0.f, visual.railThickness * 0.50f });
			rail.setPosition(
				location + forward * (headLength * 0.10f) +
					perpendicular * (side * std::max(0.f, visual.railSeparation) * 0.50f)
			);
			rail.setRotation(sf::degrees(std::atan2(forward.y, forward.x) * 57.2957795131f));
			rail.setFillColor(WithAlpha(visual.coreColor, pulse * 0.90f));
			window.draw(rail, additiveStates);
		}

		sf::ConvexShape coreStreak = MakeStreak(
			location + forward * (headLength * 0.08f),
			tailLength * 0.72f,
			headLength * 0.90f,
			std::max(0.5f, visual.coreWidth),
			std::max(0.5f, visual.coreWidth * 0.20f)
		);
		coreStreak.setFillColor(visual.coreColor);
		window.draw(coreStreak, additiveStates);

		sf::CircleShape glow(std::max(1.f, visual.glowRadius), 16);
		glow.setOrigin({ visual.glowRadius, visual.glowRadius });
		glow.setPosition(location);
		glow.setFillColor(WithAlpha(visual.glowColor, pulse * 0.22f));
		window.draw(glow, additiveStates);
	}

	void RailBurstProjectileActor::DrawImpacts(sf::RenderWindow& window) const
	{
		const RailBurstVisualDefinition& visual = mPresentationProfile.visual;
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
			flash.setScale({ 0.45f + progress * 0.55f, 0.45f + progress * 0.55f });
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

	weak_ptr<AbilityWorldActor> RailBurstProjectileActor::SpawnRelayClone(
		const ProjectileRelayCloneRequest& request
	) const
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return {};
		}

		weak_ptr<RailBurstProjectileActor> clone =
			world->SpawnActor<RailBurstProjectileActor>(
				owner,
				mPresentationProfile
			);
		if (const shared_ptr<RailBurstProjectileActor> spawned = clone.lock())
		{
			spawned->ConfigureFromAttributes(request.snapshot.damageAttributes);
			spawned->ConfigureRelayClone(request);
			spawned->SetDamage(request.damage);
			spawned->SetLifeTime(std::max(
				request.snapshot.remainingLifetime,
				spawned->mProjectileSpeed > 0.f
					? spawned->mMaximumRange / spawned->mProjectileSpeed +
						RelayDeliveryLifetimeMarginSeconds
					: request.snapshot.remainingLifetime
			));
			spawned->mLaunchVelocity = NormalizeOrDefault(request.direction) *
				spawned->mProjectileSpeed;
			spawned->SetVelocity(spawned->mLaunchVelocity);
		}
		return clone;
	}

	bool RegisterRailBurstProjectileActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<RailBurstProjectileActorType>()
		);
	}
}
