#include "gameplay/ability/orbitalDrones/OrbitingDroneActor.h"

#include "framework/World.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/portal/PortalTransferParticipant.h"

#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace ly
{
	namespace
	{
		constexpr float Pi = 3.14159265358979323846f;
		constexpr float TwoPi = Pi * 2.f;

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

		float NormalizeAngle(float radians)
		{
			if (!std::isfinite(radians))
			{
				return 0.f;
			}

			radians = std::fmod(radians, TwoPi);
			return radians < 0.f ? radians + TwoPi : radians;
		}

		float DistanceSquaredToBounds(
			const Actor& actor,
			const sf::Vector2f& point
		)
		{
			const sf::FloatRect bounds = actor.GetActorGlobalBounds();
			if (bounds.size.x <= 0.f || bounds.size.y <= 0.f)
			{
				const sf::Vector2f delta = actor.GetActorLocation() - point;
				return delta.x * delta.x + delta.y * delta.y;
			}

			const float closestX = std::clamp(
				point.x,
				bounds.position.x,
				bounds.position.x + bounds.size.x
			);
			const float closestY = std::clamp(
				point.y,
				bounds.position.y,
				bounds.position.y + bounds.size.y
			);
			const float deltaX = point.x - closestX;
			const float deltaY = point.y - closestY;
			return deltaX * deltaX + deltaY * deltaY;
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
	}

	OrbitingDroneActor::OrbitingDroneActor(
		World* world,
		Actor* owner,
		const OrbitalDronesPresentationProfile& presentationProfile,
		OrbitConfiguration orbit
	)
		: AbilityWorldActor(world, owner),
		mOwnerActor{ MakeWeakActor(owner) },
		mPresentationProfile{ presentationProfile },
		mOrbit{ orbit },
		mGlow{ std::max(1.f, presentationProfile.visual.glowRadius), 24 },
		mBody{ std::max(1.f, presentationProfile.visual.bodyRadius), 20 },
		mCore{ std::max(1.f, presentationProfile.visual.coreRadius), 16 },
		mContactRadius{ std::max(1.f, presentationProfile.visual.bodyRadius) }
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		mOrbit.radius = std::max(0.f, mOrbit.radius);
		mOrbit.angularSpeedRadiansPerSecond = std::isfinite(
			mOrbit.angularSpeedRadiansPerSecond
		) ? mOrbit.angularSpeedRadiansPerSecond : 0.f;
		mOrbit.phaseOffsetRadians = NormalizeAngle(mOrbit.phaseOffsetRadians);
		mOrbitAngleRadians = mOrbit.phaseOffsetRadians;
		UpdatePrimitiveGeometry();
	}

	void OrbitingDroneActor::BeginPlay()
	{
		// The drone remains a physical ability actor so Box2D can report contact
		// starts. Tick-time geometric checks below also cover continuous overlap.
		ConfigureCollisionFromOwner();
		AbilityWorldActor::BeginPlay();

		if (const shared_ptr<Actor> owner = mOwnerActor.lock())
		{
			SetActorLocation(owner->GetActorLocation() + sf::Vector2f{
				std::cos(mOrbitAngleRadians) * mOrbit.radius,
				std::sin(mOrbitAngleRadians) * mOrbit.radius
			});
		}
	}

	void OrbitingDroneActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		const shared_ptr<Actor> portalOwner = mOwnerActor.lock();
		const auto* portalParticipant = portalOwner
			? dynamic_cast<const PortalTransferParticipant*>(portalOwner.get())
			: nullptr;
		if (portalParticipant && portalParticipant->IsInPortalTransit())
		{
			SetRenderEnabled(false);
			return;
		}
		SetRenderEnabled(true);

		const float safeDeltaTime = std::max(0.f, deltaTime);
		mContactClock += safeDeltaTime;
		mVisualAge += safeDeltaTime;
		PruneHitCooldowns();

		// AbilityWorldActor owns age/lifetime cleanup. A drone never calls
		// Destroy() from contact handling; only its configured lifetime or owner
		// loss can end it.
		AbilityWorldActor::Tick(deltaTime);
		if (GetIsPendingDestroy() || !IsRenderEnabled())
		{
			return;
		}

		const shared_ptr<Actor> owner = mOwnerActor.lock();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		UpdateOrbit(safeDeltaTime);
		ProcessContactCandidates();
	}

	void OrbitingDroneActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		AbilityWorldActor::Render(window);

		const OrbitalDroneVisualDefinition& visual = mPresentationProfile.visual;
		const float pulse = 0.82f + 0.18f * std::sin(
			mVisualAge * std::max(0.f, visual.pulseSpeed)
		);
		const float fade = ResolveExpiryFade();
		const sf::Vector2f location = GetActorLocation();
		const shared_ptr<Actor> owner = mOwnerActor.lock();
		const sf::Vector2f center = owner ? owner->GetActorLocation() : location;

		const int trailSegments = std::clamp(visual.trailSegments, 2, 32);
		sf::VertexArray trail(sf::PrimitiveType::LineStrip, static_cast<std::size_t>(trailSegments));
		const float trailDuration = std::max(0.f, visual.trailDuration);
		for (int index = 0; index < trailSegments; ++index)
		{
			const float normalized = static_cast<float>(index) /
				static_cast<float>(trailSegments - 1);
			const float trailAngle = mOrbitAngleRadians -
				mOrbit.angularSpeedRadiansPerSecond * trailDuration * normalized;
			trail[static_cast<std::size_t>(index)].position = center + sf::Vector2f{
				std::cos(trailAngle) * mOrbit.radius,
				std::sin(trailAngle) * mOrbit.radius
			};
			trail[static_cast<std::size_t>(index)].color = WithAlpha(
				visual.trailColor,
				fade * (1.f - normalized) * pulse
			);
		}

		sf::VertexArray tether(sf::PrimitiveType::LineStrip, 2);
		tether[0].position = center;
		tether[1].position = location;
		tether[0].color = WithAlpha(visual.tetherColor, fade * 0.75f);
		tether[1].color = WithAlpha(visual.tetherColor, fade);

		mGlow.setPosition(location);
		mGlow.setScale({ pulse, pulse });
		mGlow.setFillColor(WithAlpha(visual.glowColor, fade * (0.75f + 0.25f * pulse)));
		mBody.setPosition(location);
		mBody.setFillColor(WithAlpha(visual.bodyColor, fade * pulse));
		mCore.setPosition(location);
		mCore.setFillColor(WithAlpha(visual.coreColor, fade));

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(trail, additive);
		window.draw(tether, additive);
		window.draw(mGlow, additive);
		window.draw(mBody, additive);
		window.draw(mCore, additive);
	}

	void OrbitingDroneActor::OnActorBeginOverlap(Actor* otherActor)
	{
		const shared_ptr<Actor> portalOwner = mOwnerActor.lock();
		const auto* portalParticipant = portalOwner
			? dynamic_cast<const PortalTransferParticipant*>(portalOwner.get())
			: nullptr;
		if (portalParticipant && portalParticipant->IsInPortalTransit())
		{
			return;
		}
		AbilityWorldActor::OnActorBeginOverlap(otherActor);
		TryDamageTarget(otherActor);
	}

	void OrbitingDroneActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);

		// Collision.Radius is the shared actor collision primitive. Orbital radius
		// and angular speed intentionally remain explicit runtime configuration so
		// this actor does not silently reinterpret Common.Radius from another
		// ability family.
		const float configuredContactRadius = sas::FindAttributeValue(
			attributes,
			CollisionAttributeIds::Radius,
			mContactRadius
		);
		if (configuredContactRadius > 0.f && std::isfinite(configuredContactRadius))
		{
			SetContactRadius(configuredContactRadius);
		}
	}

	void OrbitingDroneActor::SetOrbitConfiguration(
		const OrbitConfiguration& configuration
	)
	{
		SetOrbitConfiguration(
			configuration.radius,
			configuration.angularSpeedRadiansPerSecond,
			configuration.phaseOffsetRadians
		);
	}

	void OrbitingDroneActor::SetOrbitConfiguration(
		float radius,
		float angularSpeedRadiansPerSecond,
		float phaseOffsetRadians
	)
	{
		mOrbit.radius = std::max(0.f, std::isfinite(radius) ? radius : 0.f);
		mOrbit.angularSpeedRadiansPerSecond = std::isfinite(
			angularSpeedRadiansPerSecond
		) ? angularSpeedRadiansPerSecond : 0.f;
		mOrbit.phaseOffsetRadians = NormalizeAngle(phaseOffsetRadians);
		mOrbitAngleRadians = mOrbit.phaseOffsetRadians;
	}

	void OrbitingDroneActor::SetContactRadius(float radius)
	{
		mContactRadius = std::max(0.f, std::isfinite(radius) ? radius : 0.f);
		SetAbilityCollisionRadius(mContactRadius);
	}

	void OrbitingDroneActor::SetSameTargetHitCooldown(float cooldownSeconds)
	{
		mSameTargetHitCooldown = std::max(
			0.f,
			std::isfinite(cooldownSeconds) ? cooldownSeconds : 0.f
		);
	}

	void OrbitingDroneActor::SetDamagePayload(const DamagePayload& payload)
	{
		mDamagePayloadOverride = payload;
		mHasDamagePayloadOverride = true;
	}

	void OrbitingDroneActor::ClearDamagePayloadOverride()
	{
		mDamagePayloadOverride = {};
		mHasDamagePayloadOverride = false;
	}

	float OrbitingDroneActor::CalculateFormationPhase(
		std::size_t droneIndex,
		std::size_t droneCount,
		float formationPhaseRadians
	)
	{
		if (droneCount == 0)
		{
			return NormalizeAngle(formationPhaseRadians);
		}

		const float normalizedIndex = static_cast<float>(droneIndex % droneCount) /
			static_cast<float>(droneCount);
		return NormalizeAngle(
			formationPhaseRadians + normalizedIndex * TwoPi
		);
	}

	void OrbitingDroneActor::UpdateOrbit(float deltaTime)
	{
		mOrbitAngleRadians = NormalizeAngle(
			mOrbitAngleRadians +
				mOrbit.angularSpeedRadiansPerSecond * deltaTime
		);

		const shared_ptr<Actor> owner = mOwnerActor.lock();
		if (!owner)
		{
			return;
		}

		const sf::Vector2f offset{
			std::cos(mOrbitAngleRadians) * mOrbit.radius,
			std::sin(mOrbitAngleRadians) * mOrbit.radius
		};
		SetActorLocation(owner->GetActorLocation() + offset);
		SetActorRotation(mOrbitAngleRadians * 180.f / Pi + 90.f);
	}

	void OrbitingDroneActor::ProcessContactCandidates()
	{
		World* world = GetWorld();
		if (!world)
		{
			return;
		}

		for (const weak_ptr<Actor>& actorWeak : world->GetActorsInBounds(
			targeting::swept::RadiusBounds(GetActorLocation(), mContactRadius)
		))
		{
			const shared_ptr<Actor> target = actorWeak.lock();
			if (target)
			{
				TryDamageTarget(target.get());
			}
		}
	}

	void OrbitingDroneActor::TryDamageTarget(Actor* target)
	{
		if (!IsEligibleCombatant(target) || !IsTouchingTarget(*target))
		{
			return;
		}

		const unsigned int targetId = target->GetUniqueID();
		const auto found = mNextHitAllowedAt.find(targetId);
		if (found != mNextHitAllowedAt.end() &&
			mContactClock < found->second.nextAllowedAt)
		{
			return;
		}

		const shared_ptr<Actor> source = mOwnerActor.lock();
		const float damage = std::max(0.f, GetDamage());
		if (!source || damage <= 0.f)
		{
			return;
		}

		const DamagePayload payload = mHasDamagePayloadOverride
			? mDamagePayloadOverride
			: GetDamagePayload();
		ApplyCombatDamage(
			*target,
			damage,
			source.get(),
			GetDamageTags(),
			payload,
			GetSourceAbilityId(),
			GetSourceAbilityTags()
		);
		mNextHitAllowedAt[targetId] = {
			MakeWeakActor(target),
			mContactClock + mSameTargetHitCooldown
		};
	}

	bool OrbitingDroneActor::IsEligibleCombatant(const Actor* target) const
	{
		const shared_ptr<Actor> owner = mOwnerActor.lock();
		return target && target != this && target != owner.get() &&
			!target->GetIsPendingDestroy() &&
			dynamic_cast<const Combatant*>(target) != nullptr &&
			IsValidAbilityTarget(target);
	}

	bool OrbitingDroneActor::IsTouchingTarget(const Actor& target) const
	{
		if (mContactRadius <= 0.f)
		{
			return false;
		}

		const float distanceSquared = DistanceSquaredToBounds(
			target,
			GetActorLocation()
		);
		return std::isfinite(distanceSquared) &&
			distanceSquared <= mContactRadius * mContactRadius;
	}

	void OrbitingDroneActor::PruneHitCooldowns()
	{
		for (auto iter = mNextHitAllowedAt.begin(); iter != mNextHitAllowedAt.end();)
		{
			const shared_ptr<Actor> target = iter->second.target.lock();
			if (!target || target->GetIsPendingDestroy() ||
				iter->second.nextAllowedAt + 1.f < mContactClock)
			{
				iter = mNextHitAllowedAt.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}

	float OrbitingDroneActor::ResolveExpiryFade() const
	{
		const float fadeDuration = std::max(
			0.f,
			mPresentationProfile.visual.expiryFadeDuration
		);
		const float lifeTime = GetLifeTime();
		if (fadeDuration <= 0.f || lifeTime <= 0.f)
		{
			return 1.f;
		}

		const float remaining = std::max(0.f, lifeTime - GetAge());
		const float normalized = std::clamp(remaining / fadeDuration, 0.f, 1.f);
		return normalized * normalized * (3.f - 2.f * normalized);
	}

	void OrbitingDroneActor::UpdatePrimitiveGeometry()
	{
		const OrbitalDroneVisualDefinition& visual = mPresentationProfile.visual;
		mGlow.setOrigin({ std::max(1.f, visual.glowRadius), std::max(1.f, visual.glowRadius) });
		mBody.setOrigin({ std::max(1.f, visual.bodyRadius), std::max(1.f, visual.bodyRadius) });
		mCore.setOrigin({ std::max(1.f, visual.coreRadius), std::max(1.f, visual.coreRadius) });
		mGlow.setFillColor(sf::Color::Transparent);
		mBody.setFillColor(sf::Color::Transparent);
		mCore.setFillColor(sf::Color::Transparent);
	}
}
