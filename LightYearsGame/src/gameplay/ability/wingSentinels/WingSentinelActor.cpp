#include "gameplay/ability/wingSentinels/WingSentinelActor.h"

#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/wingSentinels/WingSentinelsContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/AutoTargeting.h"
#include "gameplay/targeting/TargetRelation.h"
#include "attributes/AttributeMath.h"
#include "framework/World.h"

#include <SFML/Graphics/CircleShape.hpp>

#include <algorithm>
#include <cmath>

namespace ly
{
	WingSentinelActor::WingSentinelActor(World* world, Actor* owner, const WingSentinelsPresentationProfile& profile, Configuration configuration)
		: AbilityWorldActor(world, owner), mProfile(profile), mConfiguration(configuration)
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetAbilityPhysicsEnabled(false);
	}

	void WingSentinelActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		UpdateFormation();
	}

	void WingSentinelActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy()) return;
		mVisualAge += std::max(0.f, deltaTime);
		UpdateFormation();
		TryFire(std::max(0.f, deltaTime));
		AbilityWorldActor::Tick(std::max(0.f, deltaTime));
	}

	void WingSentinelActor::UpdateFormation()
	{
		Actor* owner = GetOwnerActor();
		if (!owner) { Destroy(); return; }
		const sf::Vector2f forward = owner->GetActorForwardDirection();
		const sf::Vector2f right{ -forward.y, forward.x };
		const float sign = static_cast<float>(mConfiguration.side == Side::Left ? -1 : 1);
		SetActorLocation(owner->GetActorLocation() + right * (sign * mConfiguration.sideOffset));
	}

	shared_ptr<Actor> WingSentinelActor::FindTarget() const
	{
		Actor* owner = GetOwnerActor();
		World* world = GetWorld();
		if (!owner || !world) return {};
		const sf::Vector2f forward = owner->GetActorForwardDirection();
		const sf::Vector2f right{ -forward.y, forward.x };
		const float expectedSign = static_cast<float>(mConfiguration.side == Side::Left ? -1 : 1);
		targeting::TargetingQuery query;
		query.source = owner;
		query.origin = owner->GetActorLocation();
		query.range = mConfiguration.targetingRange;
		query.requiredTargetLayers = targeting::ResolveOpposingLayer(*owner);
		query.requireCollisionCompatibility = true;
		// AutoTargeting's filter has no origin argument, so capture the ship center
		// explicitly for the side-dot test.
		const sf::Vector2f center = owner->GetActorLocation();
		query.filter = [center, right, expectedSign](const Actor*, const Actor& candidate, const targeting::TargetingCandidate&)
		{
			if (!dynamic_cast<const Combatant*>(&candidate)) return false;
			const sf::Vector2f delta = candidate.GetActorLocation() - center;
			return (delta.x * right.x + delta.y * right.y) * expectedSign > 0.f;
		};
		return targeting::AutoTargeting::FindTarget(*world, query).lock();
	}

	float WingSentinelActor::ResolveAttackRate() const
	{
		Actor* owner = GetOwnerActor();
		const Combatant* combatant = owner ? dynamic_cast<const Combatant*>(owner) : nullptr;
		if (!combatant) return mConfiguration.baseAttackRate;
		const float rating = combatant->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed);
		// AttackSpeed is a rating across the game. Reuse its established percentage
		// scale rather than treating a zero rating as a zero attack multiplier.
		return mConfiguration.baseAttackRate * (1.f + std::max(0.f, rating) / sas::AttributeMath::PercentageRatingScale);
	}

	void WingSentinelActor::TryFire(float deltaTime)
	{
		mFireCooldown = std::max(0.f, mFireCooldown - deltaTime);
		if (mFireCooldown > 0.f || !mConfiguration.abilitySystem || !mConfiguration.abilityDefinition || !mConfiguration.abilityInstance) return;
		const shared_ptr<Actor> target = FindTarget();
		if (!target) return;
		sf::Vector2f direction = target->GetActorLocation() - GetActorLocation();
		if (GetVectorLength(direction) <= 0.001f) return;
		NormalizeVector(direction);
		SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);
		AbilityExecutionContext context{ mConfiguration.abilitySystem, mConfiguration.abilityDefinition, nullptr, mConfiguration.abilityInstance };
		AbilityActorSpawner::SpawnAtLocation(AbilityData::WingSentinels::Actor::Projectile::BasicDefinitionId, context, *GetOwnerActor(), GetActorLocation(), direction);
		mFireCooldown = 1.f / std::max(0.01f, ResolveAttackRate());
	}

	void WingSentinelActor::Render(sf::RenderWindow& window)
	{
		const float pulse = 0.80f + 0.20f * std::sin(mVisualAge * 9.f);
		sf::RenderStates additive; additive.blendMode = sf::BlendAdd;
		sf::CircleShape glow(mProfile.droneRadius * 1.9f, 20); glow.setOrigin({ mProfile.droneRadius * 1.9f, mProfile.droneRadius * 1.9f }); glow.setPosition(GetActorLocation()); glow.setFillColor(mProfile.droneGlowColor);
		sf::CircleShape body(mProfile.droneRadius, 6); body.setOrigin({ mProfile.droneRadius, mProfile.droneRadius }); body.setPosition(GetActorLocation()); body.setRotation(sf::degrees(GetActorRotation())); body.setFillColor(mProfile.droneBodyColor);
		sf::CircleShape core(mProfile.droneRadius * 0.34f * pulse, 12); core.setOrigin({ mProfile.droneRadius * 0.34f * pulse, mProfile.droneRadius * 0.34f * pulse }); core.setPosition(GetActorLocation()); core.setFillColor(mProfile.droneCoreColor);
		window.draw(glow, additive); window.draw(body, additive); window.draw(core, additive);
	}
}
