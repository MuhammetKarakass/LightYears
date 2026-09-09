#include "gameplay/ability/emberSwarm/EmberDroneActor.h"

#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/portal/PortalTransferParticipant.h"
#include "spaceShip/SpaceShip.h"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float Pi = 3.14159265358979323846f;
		constexpr float TwoPi = Pi * 2.f;
		constexpr float PulsePhaseOffsets[3] = { 0.0f, 0.0833f, 0.1667f };

		float NormalizeAngle(float radians)
		{
			if (!std::isfinite(radians))
			{
				return 0.f;
			}
			radians = std::fmod(radians, TwoPi);
			return radians < 0.f ? radians + TwoPi : radians;
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

	EmberDroneActor::EmberDroneActor(
		World* world,
		Actor* owner,
		const EmberSwarmPresentationProfile& presentationProfile,
		const Configuration& configuration
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile(presentationProfile),
		mConfiguration(configuration),
		mGlow(std::max(1.f, presentationProfile.visual.glowRadius), 20),
		mBody(std::max(1.f, presentationProfile.visual.bodyRadius), 16),
		mCore(std::max(1.f, presentationProfile.visual.coreRadius), 12)
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetAbilityPhysicsEnabled(false);

		// Distinct initial formation phase offset for 3 drones: 0, 2pi/3, 4pi/3
		const float basePhase = (static_cast<float>(mConfiguration.droneIndex % 3) / 3.f) * TwoPi;
		mOrbitAngleRadians = NormalizeAngle(basePhase);

		if (mConfiguration.pulsePhaseOffset <= 0.f && mConfiguration.droneIndex % 3 != 0)
		{
			mConfiguration.pulsePhaseOffset = PulsePhaseOffsets[mConfiguration.droneIndex % 3];
		}
		mNextPulseTime = mConfiguration.pulsePhaseOffset;

		UpdatePrimitiveGeometry();
	}

	void EmberDroneActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();

		Actor* owner = GetOwnerActor();
		if (owner)
		{
			const sf::Vector2f offset{
				std::cos(mOrbitAngleRadians) * mConfiguration.orbitRadius,
				std::sin(mOrbitAngleRadians) * mConfiguration.orbitRadius
			};
			SetActorLocation(owner->GetActorLocation() + offset);
			SetActorRotation(mOrbitAngleRadians * 180.f / Pi + 90.f);
		}
	}

	void EmberDroneActor::SetTarget(const shared_ptr<Actor>& target)
	{
		mTarget = target;
		if (target && !target->GetIsPendingDestroy())
		{
			mState = State::TravelingToTarget;
		}
		else
		{
			mTarget.reset();
			if (mState == State::OrbitingTarget)
			{
				mState = State::TravelingToTarget;
			}
		}
	}

	void EmberDroneActor::ClearTarget()
	{
		mTarget.reset();
		if (mState == State::OrbitingTarget)
		{
			mState = State::TravelingToTarget;
		}
	}

	Actor* EmberDroneActor::GetTarget() const
	{
		return mTarget.lock().get();
	}

	void EmberDroneActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		Actor* owner = GetOwnerActor();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		const auto* portalParticipant = dynamic_cast<const PortalTransferParticipant*>(owner);
		if (portalParticipant && portalParticipant->IsInPortalTransit())
		{
			SetRenderEnabled(false);
			return;
		}
		SetRenderEnabled(true);

		const float safeDeltaTime = std::max(0.f, deltaTime);

		AbilityWorldActor::Tick(safeDeltaTime);
		if (GetIsPendingDestroy() || !IsRenderEnabled())
		{
			return;
		}

		// Update pulse visual timer
		if (mHasActivePulseVisual)
		{
			mPulseVisualTimer += safeDeltaTime;
			if (mPulseVisualTimer >= mPresentationProfile.visual.pulseDuration)
			{
				mHasActivePulseVisual = false;
				mHasImpactVisual = false;
			}
		}

		// Update orbit angle continuously (stable phase)
		mOrbitAngleRadians = NormalizeAngle(
			mOrbitAngleRadians + mConfiguration.angularSpeed * safeDeltaTime
		);

		// Check if target is still valid and within leash (1000 from owner)
		shared_ptr<Actor> target = mTarget.lock();
		if (target)
		{
			bool targetValid = !target->GetIsPendingDestroy();
			if (targetValid)
			{
				if (const auto* ship = dynamic_cast<const SpaceShip*>(target.get()))
				{
					targetValid = ship->GetHealthComponent().GetHealth() > 0.f;
				}
			}
			if (targetValid)
			{
				const sf::Vector2f delta = target->GetActorLocation() - owner->GetActorLocation();
				if (delta.x * delta.x + delta.y * delta.y > 1000.f * 1000.f)
				{
					targetValid = false;
				}
			}

			if (!targetValid)
			{
				ClearTarget();
				target = nullptr;
			}
		}

		const sf::Vector2f currentLoc = GetActorLocation();

		if (target)
		{
			const sf::Vector2f targetLoc = target->GetActorLocation();
			const sf::Vector2f orbitOffset{
				std::cos(mOrbitAngleRadians) * mConfiguration.targetOrbitRadius,
				std::sin(mOrbitAngleRadians) * mConfiguration.targetOrbitRadius
			};
			const sf::Vector2f dest = targetLoc + orbitOffset;
			const sf::Vector2f toDest = dest - currentLoc;
			const float dist = GetVectorLength(toDest);
			const float travelStep = mConfiguration.travelSpeed * safeDeltaTime;

			if (mState == State::OrbitingTarget)
			{
				SetActorLocation(dest);
				SetActorRotation(mOrbitAngleRadians * 180.f / Pi + 90.f);
			}
			else
			{
				if (safeDeltaTime > 0.f && (dist <= travelStep || dist <= 5.f))
				{
					SetActorLocation(dest);
					SetActorRotation(mOrbitAngleRadians * 180.f / Pi + 90.f);
					mState = State::OrbitingTarget;
				}
				else
				{
					mState = State::TravelingToTarget;
					const sf::Vector2f dir = dist > 0.0001f ? (toDest / dist) : sf::Vector2f{ 0.f, 0.f };
					SetActorLocation(currentLoc + dir * travelStep);
					SetActorRotation(std::atan2(toDest.y, toDest.x) * 180.f / Pi + 90.f);
				}
			}
		}
		else
		{
			// Targetless: idle orbit owner
			const sf::Vector2f ownerOrbitOffset{
				std::cos(mOrbitAngleRadians) * mConfiguration.orbitRadius,
				std::sin(mOrbitAngleRadians) * mConfiguration.orbitRadius
			};
			const sf::Vector2f dest = owner->GetActorLocation() + ownerOrbitOffset;
			const sf::Vector2f toDest = dest - currentLoc;
			const float dist = GetVectorLength(toDest);
			const float travelStep = mConfiguration.travelSpeed * safeDeltaTime;

			if (mState == State::IdleOrbitOwner)
			{
				SetActorLocation(dest);
				SetActorRotation(mOrbitAngleRadians * 180.f / Pi + 90.f);
			}
			else
			{
				if (safeDeltaTime > 0.f && (dist <= travelStep || dist <= 5.f))
				{
					SetActorLocation(dest);
					SetActorRotation(mOrbitAngleRadians * 180.f / Pi + 90.f);
					mState = State::IdleOrbitOwner;
				}
				else
				{
					const sf::Vector2f dir = dist > 0.0001f ? (toDest / dist) : sf::Vector2f{ 0.f, 0.f };
					SetActorLocation(currentLoc + dir * travelStep);
					SetActorRotation(std::atan2(toDest.y, toDest.x) * 180.f / Pi + 90.f);
				}
			}
		}

		// Pulse logic: Orbiting drones pulse on separate 0.0/0.0833/0.1667 phase offsets every .25
		// No travel damage/collision: pulses only fire when in State::OrbitingTarget
		if (safeDeltaTime > 0.f)
		{
			const float age = GetAge();
			while (age >= mNextPulseTime)
			{
				if (mState == State::OrbitingTarget && target && !target->GetIsPendingDestroy())
				{
					PerformPulse(*owner, *target);
				}
				mNextPulseTime += mConfiguration.pulseInterval;
			}
		}
	}

	void EmberDroneActor::PerformPulse(Actor& owner, Actor& target)
	{
		float attackPower = 0.f;
		if (auto* combatantOwner = dynamic_cast<Combatant*>(&owner))
		{
			const sas::AttributeSystem& attributes =
				combatantOwner->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			if (attributes.HasAttribute(OwnerAttributeIds::AttackPower))
			{
				attackPower = std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::AttackPower));
			}
		}

		const int level = std::max(1, mAbilityLevel);
		const float baseDamage = 4.0f + 0.4f * static_cast<float>(level - 1);
		const float pulseDamage = baseDamage + attackPower * 0.08f;

		DamagePayload payload;
		payload.canCrit = true;
		payload.igniteStacks = 1;
		payload.burnDamagePerSecond = 1.f;
		payload.burnDuration = 3.f;
		payload.burnMaxStacks = 4;

		ApplyCombatDamage(
			target,
			pulseDamage,
			&owner,
			{ DamageTypeSchema::Thermal },
			payload,
			GetSourceAbilityId(),
			GetSourceAbilityTags(),
			DamageDeliveryType::Direct,
			this
		);

		mPulseOrigin = GetActorLocation();
		mImpactLocation = target.GetActorLocation();
		mPulseVisualTimer = 0.f;
		mHasActivePulseVisual = true;
		mHasImpactVisual = true;
	}

	void EmberDroneActor::Render(sf::RenderWindow& window)
	{
		if (!IsRenderEnabled())
		{
			return;
		}

		const float fade = ResolveExpiryFade();
		const sf::Vector2f loc = GetActorLocation();

		const EmberDroneVisualDefinition& visual = mPresentationProfile.visual;

		mGlow.setPosition(loc);
		mGlow.setFillColor(WithAlpha(visual.glowColor, fade));

		mBody.setPosition(loc);
		mBody.setFillColor(WithAlpha(visual.bodyColor, fade));

		mCore.setPosition(loc);
		mCore.setFillColor(WithAlpha(visual.coreColor, fade));

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mGlow, additive);
		window.draw(mBody, additive);
		window.draw(mCore, additive);

		// Render pulse and impact visual feedback
		if (mHasActivePulseVisual && visual.pulseDuration > 0.f)
		{
			const float pulseProgress = std::clamp(mPulseVisualTimer / visual.pulseDuration, 0.f, 1.f);
			const float pulseAlpha = (1.f - pulseProgress) * fade;
			const float currentRadius = visual.pulseRadius * pulseProgress;

			sf::CircleShape pulseCircle(std::max(1.f, currentRadius), 24);
			pulseCircle.setOrigin({ currentRadius, currentRadius });
			pulseCircle.setPosition(mPulseOrigin);
			pulseCircle.setFillColor(sf::Color::Transparent);
			pulseCircle.setOutlineColor(WithAlpha(visual.pulseColor, pulseAlpha));
			pulseCircle.setOutlineThickness(visual.pulseOutlineThickness);

			window.draw(pulseCircle, additive);

			if (mHasImpactVisual)
			{
				const float impactRadius = visual.impactRadius * pulseProgress;
				sf::CircleShape impactCircle(std::max(1.f, impactRadius), 16);
				impactCircle.setOrigin({ impactRadius, impactRadius });
				impactCircle.setPosition(mImpactLocation);
				impactCircle.setFillColor(WithAlpha(visual.impactColor, pulseAlpha * 0.4f));
				impactCircle.setOutlineColor(WithAlpha(visual.impactColor, pulseAlpha));
				impactCircle.setOutlineThickness(visual.pulseOutlineThickness);

				window.draw(impactCircle, additive);
			}
		}
	}

	float EmberDroneActor::ResolveExpiryFade() const
	{
		const float remainingLifetime = GetLifeTime() - GetAge();
		const float fadeDuration = mPresentationProfile.visual.expiryFadeDuration;
		if (fadeDuration <= 0.f || remainingLifetime >= fadeDuration)
		{
			return 1.f;
		}
		return std::clamp(remainingLifetime / fadeDuration, 0.f, 1.f);
	}

	void EmberDroneActor::UpdatePrimitiveGeometry()
	{
		const auto centerOrigin = [](sf::CircleShape& shape)
		{
			const float r = shape.getRadius();
			shape.setOrigin({ r, r });
		};

		mGlow.setRadius(std::max(1.f, mPresentationProfile.visual.glowRadius));
		centerOrigin(mGlow);

		mBody.setRadius(std::max(1.f, mPresentationProfile.visual.bodyRadius));
		centerOrigin(mBody);

		mCore.setRadius(std::max(1.f, mPresentationProfile.visual.coreRadius));
		centerOrigin(mCore);
	}
}
