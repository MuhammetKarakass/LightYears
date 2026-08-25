#include "gameplay/ability/energySpear/EnergySpearTraversalActor.h"

#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/ability/energySpear/EnergySpearContracts.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/targeting/SweptGeometry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
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

		sf::Vector2f NormalizeOrDefault(sf::Vector2f direction)
		{
			if (GetVectorLength(direction) <= 0.001f)
			{
				return { 0.f, -1.f };
			}
			NormalizeVector(direction);
			return direction;
		}

	}

	EnergySpearTraversalActor::EnergySpearTraversalActor(
		World* world,
		const EnergySpearTraversalRequest& request
	)
		: Actor(world),
		mOwner(MakeWeakActor(request.owner)),
		mVisual(request.visual),
		mStartLocation(request.startLocation),
		mDirection(NormalizeOrDefault(request.direction)),
		mTravelDistance(std::max(0.f, request.travelDistance)),
		mTravelSpeed(std::max(1.f, request.travelSpeed)),
		mCollisionRadius(std::max(0.1f, request.collisionRadius)),
		mMinimumDamageDistance(std::max(0.f, request.minimumDamageDistance)),
		mEndpointDamageMultiplier(std::max(1.f, request.endpointDamageMultiplier)),
		mChargeDamageMultiplier(std::max(0.f, request.chargeDamageMultiplier)),
		mDamage(std::max(0.f, request.damage)),
		mDamageTags(request.damageTags),
		mDamagePayload(request.damagePayload),
		mSourceAbilityId(request.sourceAbilityId),
		mSourceAbilityTags(request.sourceAbilityTags)
	{
		SetActorLocation(mStartLocation);
		SetActorRotation(RadiansToDegrees(std::atan2(mDirection.y, mDirection.x)) + 90.f);
		SetRenderLayer(RenderLayer::Projectile);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void EnergySpearTraversalActor::BeginPlay()
	{
		Actor::BeginPlay();
		if (const shared_ptr<Actor> owner = mOwner.lock())
		{
			mPreservedVelocity = owner->GetVelocity();
			owner->SetVelocity(mDirection * mTravelSpeed);
		}
	}

	void EnergySpearTraversalActor::Tick(float deltaTime)
	{
		if (mFinished)
		{
			return;
		}

		for (ImpactPulse& pulse : mImpactPulses)
		{
			pulse.age += std::max(0.f, deltaTime);
		}
		mImpactPulses.erase(
			std::remove_if(
				mImpactPulses.begin(),
				mImpactPulses.end(),
				[&](const ImpactPulse& pulse)
				{
					return pulse.age >= std::max(0.001f, mVisual.impactDuration);
				}
			),
			mImpactPulses.end()
		);

		const shared_ptr<Actor> owner = mOwner.lock();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Finish();
			return;
		}
		if (const auto* combatant = dynamic_cast<const Combatant*>(owner.get()))
		{
			const auto& ownedTags = combatant->GetAbilitySystemComponent().GetOwnedTags();
			if (ownedTags.HasTag(GameplayTags::State::Effect::Control::Stunned) ||
				ownedTags.HasTag(GameplayTags::State::Effect::Control::Staggered) ||
				ownedTags.HasTag(GameplayTags::State::ActionLock::ExternalMovement))
			{
				Finish();
				return;
			}
		}

		Move(deltaTime);
		Actor::Tick(deltaTime);
	}

	void EnergySpearTraversalActor::Move(float deltaTime)
	{
		const shared_ptr<Actor> owner = mOwner.lock();
		if (!owner)
		{
			Finish();
			return;
		}

		float remainingDistance = mTravelSpeed * std::max(0.f, deltaTime);
		constexpr float MaximumMovementSubstep = 10.f;
		while (remainingDistance > 0.f && !mFinished)
		{
			const float step = std::min({
				remainingDistance,
				MaximumMovementSubstep,
				mTravelDistance - mDistanceTravelled
			});
			if (step <= 0.f)
			{
				Finish();
				break;
			}

			const sf::Vector2f segmentStart = owner->GetActorLocation();
			owner->SetVelocity(mDirection * mTravelSpeed);
			owner->AddActorLocationOffset(mDirection * step);
			const sf::Vector2f segmentEnd = owner->GetActorLocation();
			ApplyPiercingHits(segmentStart, segmentEnd);

			mDistanceTravelled += step;
			remainingDistance -= step;
			if (mDistanceTravelled >= mTravelDistance - 0.001f)
			{
				owner->SetActorLocation(mStartLocation + mDirection * mTravelDistance);
				Finish();
			}
		}
	}

	void EnergySpearTraversalActor::ApplyPiercingHits(
		const sf::Vector2f& segmentStart,
		const sf::Vector2f& segmentEnd
	)
	{
		World* world = GetWorld();
		const shared_ptr<Actor> owner = mOwner.lock();
		if (!world || !owner || mDamage <= 0.f)
		{
			return;
		}

		for (const weak_ptr<Actor>& actorWeak : world->GetActorsInBounds(
			targeting::swept::SegmentBounds(
				segmentStart,
				segmentEnd,
				mCollisionRadius
			)
		))
		{
			const shared_ptr<Actor> target = actorWeak.lock();
			if (!target || target.get() == owner.get() || target->GetIsPendingDestroy() ||
				!HasCollisionLayer(target->GetCollisionLayer(), CollisionLayer::Enemy) ||
				!dynamic_cast<Combatant*>(target.get()) ||
				mHitTargets.find(target.get()) != mHitTargets.end() ||
				!targeting::swept::SegmentIntersectsExpandedBounds(
					segmentStart,
					segmentEnd,
					target->GetActorGlobalBounds(),
					mCollisionRadius
				))
			{
				continue;
			}

			const sf::Vector2f targetOffset = target->GetActorLocation() - mStartLocation;
			const float hitDistance = std::clamp(
				targetOffset.x * mDirection.x + targetOffset.y * mDirection.y,
				0.f,
				mTravelDistance
			);
			const float distanceDenominator = mTravelDistance - mMinimumDamageDistance;
			const float distanceRatio = distanceDenominator > 0.001f
				? std::clamp(
					(hitDistance - mMinimumDamageDistance) / distanceDenominator,
					0.f,
					1.f
				)
				: 0.f;
			const float distanceMultiplier = 1.f +
				(mEndpointDamageMultiplier - 1.f) * distanceRatio;
			const float finalDamage = mDamage * mChargeDamageMultiplier * distanceMultiplier;

			mHitTargets.insert(target.get());
			mImpactPulses.push_back({ target->GetActorLocation(), 0.f });
			ApplyCombatDamage(
				*target,
				finalDamage,
				owner.get(),
				mDamageTags,
				mDamagePayload,
				mSourceAbilityId,
				mSourceAbilityTags,
				DamageDeliveryType::Projectile,
				this
			);
		}
	}

	void EnergySpearTraversalActor::Finish()
	{
		if (mFinished)
		{
			return;
		}
		mFinished = true;

		if (const shared_ptr<Actor> owner = mOwner.lock())
		{
			owner->SetVelocity(mPreservedVelocity);
			if (auto* combatant = dynamic_cast<Combatant*>(owner.get()))
			{
				LightYearsAbilitySystemComponent& abilitySystem =
					combatant->GetAbilitySystemComponent();
				abilitySystem.RemoveOwnedTag(
					GameplayTags::State::ActionLock::AbilityActivation
				);
				abilitySystem.RemoveOwnedTag(
					GameplayTags::State::ActionLock::PrimaryWeaponFire
				);
				abilitySystem.RemoveOwnedTag(
					GameplayTags::State::ActionLock::MovementInput
				);
				abilitySystem.RemoveOwnedTag(AbilityData::EnergySpear::State::Traversing);

				sas::AbilityEvent event;
				event.eventTag = AbilityData::EnergySpear::Event::Ended;
				event.sourceAbilityId = mSourceAbilityId;
				event.sourceAbilityTags = mSourceAbilityTags;
				event.SetSource(owner.get());
				event.SetTarget(owner.get());
				abilitySystem.HandleGameplayEvent(event);
			}
		}
		Actor::Destroy();
	}

	void EnergySpearTraversalActor::Destroy()
	{
		Finish();
	}

	void EnergySpearTraversalActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		DrawImpacts(window);
	}

	void EnergySpearTraversalActor::DrawImpacts(sf::RenderWindow& window) const
	{
		for (const ImpactPulse& pulse : mImpactPulses)
		{
			const float duration = std::max(0.001f, mVisual.impactDuration);
			const float progress = std::clamp(pulse.age / duration, 0.f, 1.f);
			const float fade = 1.f - progress;
			const float radius = mVisual.impactRadius * (0.65f + 0.35f * progress);
			sf::CircleShape flash(radius, 32);
			flash.setOrigin({ radius, radius });
			flash.setPosition(pulse.location);
			flash.setFillColor(WithAlpha(mVisual.impactColor, fade));
			window.draw(flash, sf::RenderStates{ sf::BlendAdd });

			sf::CircleShape ring(radius, 32);
			ring.setOrigin({ radius, radius });
			ring.setPosition(pulse.location);
			ring.setFillColor(sf::Color::Transparent);
			ring.setOutlineColor(WithAlpha(mVisual.impactRingColor, fade));
			ring.setOutlineThickness(mVisual.impactRingThickness);
			window.draw(ring, sf::RenderStates{ sf::BlendAdd });
		}
	}
}
