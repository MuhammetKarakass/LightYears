#include "gameplay/module/generated/ModuleGuidedProjectileActor.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/projectile/ProjectileSweep.h"
#include "gameplay/combat/Combatant.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "framework/MathUtility.h"
#include <algorithm>
#include <cmath>

namespace vega
{
	ModuleGuidedProjectileActor::ModuleGuidedProjectileActor(World* world, Actor* source, const ModuleGuidedPresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, const weak_ptr<Actor>& target, float speed, float lifetime, float damage, const GameplayTag& damageType) : AbilityWorldActor{ world, source }, mProfile{ profile }, mLease{ lease }, mTarget{ target }, mSpeed{ speed }
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetAbilityPhysicsEnabled(false);
		SetAbilityCollisionRadius(profile.radius);
		ConfigureCollisionFromOwner();
		SetDamage(damage);
		SetDamageTags({ damageType });
		SetDamagePayload(MakeModuleGeneratedDamagePayload());
		SetLifeTime(lifetime);
	}
	void ModuleGuidedProjectileActor::Tick(float deltaTime)
	{
		const auto lease = mLease.lock();
		const auto owner = LockOwnerActor();
		const auto target = mTarget.lock();
		if (!lease || !lease->active || !owner || owner->GetIsPendingDestroy() || (!mUnguided && (!target || target->GetIsPendingDestroy()))) { Destroy(); return; }
		if (IsInPortalTransit()) { AbilityWorldActor::Tick(deltaTime); return; }
		const auto start = GetActorLocation();
		const auto offset = mUnguided ? mUnguidedDirection : target->GetActorLocation() - start;
		const float distance = GetVectorLength(offset);
		const float step = std::min(std::max(0.f, deltaTime), std::max(0.f, GetLifeTime() - GetAge()));
		const auto end = mUnguided ? start + mUnguidedDirection * mSpeed * step : (distance > 0.f ? start + offset / distance * std::min(distance, mSpeed * step) : start);
		for (const auto& contact : projectile::FindSweptContacts(*this, start, end, GetPhysicsCollisionRadius()))
		{
			SetActorLocation(start + (end - start) * contact.fraction);
			AbilityWorldActor::Tick(0.f);
			if (GetIsPendingDestroy()) return;
			OnActorBeginOverlap(contact.actor);
			if (GetIsPendingDestroy()) return;
		}
		SetActorLocation(end);
		AbilityWorldActor::Tick(deltaTime);
		// A body-less combatant can still be the selected guided target.
		if (!mUnguided && distance <= mSpeed * step && !GetIsPendingDestroy())
		{
			ApplyCombatDamage(*target, GetDamage(), owner.get(), GetDamageTags(), GetDamagePayload(), {}, {}, DamageDeliveryType::Projectile, this);
			Destroy();
			return;
		}
	}
	void ModuleGuidedProjectileActor::OnActorBeginOverlap(Actor* target)
	{
		if (target && std::find(mExcludedTargets.begin(), mExcludedTargets.end(), target->GetUniqueID()) != mExcludedTargets.end()) return;
		AbilityWorldActor::OnActorBeginOverlap(target);
		if (GetIsPendingDestroy() || !IsValidAbilityTarget(target)) return;
		const auto owner = LockOwnerActor();
		const auto lease = mLease.lock();
		if (!owner || !lease || !lease->active) return;
		ApplyCombatDamage(*target, GetDamage(), owner.get(), GetDamageTags(), GetDamagePayload(), {}, {}, DamageDeliveryType::Projectile, this);
		Destroy();
	}
	void ModuleGuidedProjectileActor::Render(sf::RenderTarget& window)
	{
		sf::CircleShape body{ mProfile.radius };
		body.setOrigin({ mProfile.radius, mProfile.radius });
		body.setPosition(GetActorLocation());
		body.setFillColor(mProfile.body);
		window.draw(body);
	}
}
