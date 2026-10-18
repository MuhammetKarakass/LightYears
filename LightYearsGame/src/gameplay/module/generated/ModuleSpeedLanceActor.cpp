#include "gameplay/module/generated/ModuleSpeedLanceActor.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/projectile/ProjectileSweep.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>
#include <cmath>

namespace vega
{
	ModuleSpeedLanceActor::ModuleSpeedLanceActor(World* world, Actor* source, const ModuleSpeedLancePresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& direction, float speed, float range, float damage, int firstTargetElectricStacks) : AbilityWorldActor{ world, source }, mProfile{ profile }, mLease{ lease }, mDirection{ direction }, mSpeed{ speed }, mFirstTargetElectricStacks{ firstTargetElectricStacks }
	{
		const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
		mDirection = length > 0.f ? direction / length : sf::Vector2f{ 0.f, -1.f };
		SetRenderLayer(RenderLayer::WorldFront); SetAbilityPhysicsEnabled(false); SetAbilityCollisionRadius(profile.collisionRadius); ConfigureCollisionFromOwner();
		SetDamage(damage); SetDamageTags({ DamageTypeSchema::Energy }); SetDamagePayload(MakeModuleGeneratedDamagePayload(0, DamageSourceScalingPolicy::AlreadyResolved)); SetLifeTime(range / speed);
		SetActorRotation(std::atan2(mDirection.y, mDirection.x) * 57.2957795131f + 90.f);
	}
	void ModuleSpeedLanceActor::Tick(float deltaTime)
	{
		const auto lease = mLease.lock(); const auto owner = LockOwnerActor();
		const auto* health = owner ? owner->FindComponent<HealthComponent>() : nullptr;
		if (!lease || !lease->active || !owner || owner->GetIsPendingDestroy() || (health && health->GetHealth() <= 0.f)) { Destroy(); return; }
		if (IsInPortalTransit()) { AbilityWorldActor::Tick(deltaTime); return; }
		const auto start = GetActorLocation(); const float step = std::min(std::max(0.f, deltaTime), std::max(0.f, GetLifeTime() - GetAge())); const auto end = start + mDirection * mSpeed * step;
		for (const auto& contact : projectile::FindSweptContacts(*this, start, end, GetPhysicsCollisionRadius()))
		{
			SetActorLocation(start + (end - start) * contact.fraction); OnActorBeginOverlap(contact.actor);
			if (GetIsPendingDestroy() || !lease->active) return;
		}
		SetActorLocation(end); AbilityWorldActor::Tick(deltaTime);
	}
	void ModuleSpeedLanceActor::OnActorBeginOverlap(Actor* target)
	{
		AbilityWorldActor::OnActorBeginOverlap(target);
		if (GetIsPendingDestroy() || IsInPortalTransit() || !IsValidAbilityTarget(target)) return;
		const auto owner = LockOwnerActor(); const auto lease = mLease.lock();
		const auto* health = owner ? owner->FindComponent<HealthComponent>() : nullptr;
		if (!owner || owner->GetIsPendingDestroy() || (health && health->GetHealth() <= 0.f) || !lease || !lease->active || !mHitTargets.insert(target->GetUniqueID()).second) return;
		const bool first = mHitTargets.size() == 1;
		const auto targetLifetime = target->GetWeakActor().lock();
		ApplyCombatDamage(*target, GetDamage(), owner.get(), GetDamageTags(), GetDamagePayload(), {}, {}, DamageDeliveryType::Projectile, this);
		if (first && mFirstTargetElectricStacks > 0 && targetLifetime && !target->GetIsPendingDestroy() && lease->active)
			if (auto* combatant = dynamic_cast<Combatant*>(target); combatant && target->FindComponent<HealthComponent>() && target->FindComponent<HealthComponent>()->GetHealth() > 0.f)
			{
				DamageContext context; context.source = owner.get(); context.target = target; context.remainingDamage = 1.f; context.damageTags = { DamageTypeSchema::Electric };
				context.payload = MakeModuleGeneratedDamagePayload(); context.payload.electricStacks = mFirstTargetElectricStacks;
				DamageTypeSystem::ApplyStatusEffects(combatant->GetAbilitySystemComponent(), context);
			}
	}
	void ModuleSpeedLanceActor::Render(sf::RenderTarget& target)
	{
		sf::RectangleShape body{ { mProfile.length, mProfile.width } }; body.setOrigin({ mProfile.length * .5f, mProfile.width * .5f }); body.setPosition(GetActorLocation());
		body.setRotation(sf::degrees(GetActorRotation() - 90.f)); body.setFillColor(mProfile.body); target.draw(body);
	}
}
