#include "gameplay/module/generated/ModuleRicochetProjectileActor.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include <random>

namespace ly
{
	ModuleRicochetProjectileActor::ModuleRicochetProjectileActor(World* world, Actor* owner, const ModuleRicochetProjectileSnapshot& snapshot, const std::shared_ptr<ModuleGeneratedLease>& lease, const List<std::uint64_t>& visited, float range, float damage, const List<GameplayTag>& tags, float secondaryDamage, const std::optional<sas::GameplayEffectSpec>& status) : PrimaryWeaponProjectileActor{ world, owner, snapshot.presentation, snapshot.attributes }, mSnapshot{ snapshot }, mLease{ lease }, mVisited{ visited }, mRange{ range }, mSecondaryDamage{ secondaryDamage }, mStatus{ status }
	{
		SetDamage(damage);
		SetDamageTags(tags);
		SetDamagePayload(MakeModuleGeneratedDamagePayload());
		SetLifeTime(range / std::max(.001f, snapshot.speed));
	}
	shared_ptr<ModuleRicochetProjectileActor> ModuleRicochetProjectileActor::Spawn(Actor& source, const ModuleRicochetProjectileSnapshot& snapshot, const sf::Vector2f& origin, const std::shared_ptr<ModuleGeneratedLease>& lease, const List<std::uint64_t>& visited, float range, float damage, const List<GameplayTag>& tags, float secondaryDamage, const std::optional<sas::GameplayEffectSpec>& status)
	{
		if (!source.GetWorld() || !lease || !lease->active || snapshot.speed <= 0.f) return {};
		const auto live = std::count_if(lease->actors.begin(), lease->actors.end(), [](const auto& weak) { const auto actor = weak.lock(); return actor && !actor->GetIsPendingDestroy(); });
		if (live >= 64) return {};
		auto targets = targeting::FindOpposingCombatants(*source.GetWorld(), source, origin, range, false);
		targets.erase(std::remove_if(targets.begin(), targets.end(), [&visited](const auto& actor) { return std::find(visited.begin(), visited.end(), actor->GetUniqueID()) != visited.end(); }), targets.end());
		std::sort(targets.begin(), targets.end(), [origin](const auto& a, const auto& b)
		{
			const auto da = a->GetActorLocation() - origin; const auto db = b->GetActorLocation() - origin;
			const float left = da.x * da.x + da.y * da.y; const float right = db.x * db.x + db.y * db.y;
			return left == right ? a->GetUniqueID() < b->GetUniqueID() : left < right;
		});
		static thread_local std::mt19937 random{ std::random_device{}() };
		const float angle = std::uniform_real_distribution<float>{ 0.f, 6.283185307f }(random);
		const auto offset = targets.empty() ? sf::Vector2f{ std::cos(angle), std::sin(angle) } : targets.front()->GetActorLocation() - origin;
		const float length = GetVectorLength(offset);
		const sf::Vector2f direction = length > .001f ? offset / length : sf::Vector2f{ 1.f, 0.f };
		const auto actor = source.GetWorld()->SpawnActor<ModuleRicochetProjectileActor>(&source, snapshot, lease, visited, range, damage, tags, secondaryDamage, status).lock();
		if (!actor) return {};
		actor->SetActorLocation(origin);
		actor->SetActorRotation(std::atan2(direction.y, direction.x) * 57.2957795131f + 90.f);
		actor->SetLaunchVelocity(direction * snapshot.speed);
		if (!targets.empty()) actor->mTarget = targets.front();
		lease->actors.push_back(actor);
		return actor;
	}
	void ModuleRicochetProjectileActor::Tick(float deltaTime)
	{
		const auto lease = mLease.lock(); const auto owner = LockOwnerActor();
		if (!lease || !lease->active || !owner || owner->GetIsPendingDestroy()) { Destroy(); return; }
		const auto start = GetActorLocation();
		PrimaryWeaponProjectileActor::Tick(std::min(std::max(0.f, deltaTime), std::max(0.f, GetLifeTime() - GetAge())));
		if (!GetIsPendingDestroy() && !IsInPortalTransit())
			if (const auto target = mTarget.lock(); target && !target->GetIsPendingDestroy())
			{
				const float radius = GetPhysicsCollisionRadius() + target->GetPhysicsCollisionRadius();
				if (targeting::swept::DistanceSquaredToSegment(target->GetActorLocation(), start, GetActorLocation()) <= radius * radius) OnActorBeginOverlap(target.get());
			}
		if (GetAge() >= GetLifeTime()) Destroy();
	}
	void ModuleRicochetProjectileActor::OnActorBeginOverlap(Actor* target)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit() || !target || std::find(mVisited.begin(), mVisited.end(), target->GetUniqueID()) != mVisited.end()) return;
		AbilityWorldActor::OnActorBeginOverlap(target);
		if (GetIsPendingDestroy() || !IsValidAbilityTarget(target)) return;
		const auto lease = mLease.lock(); const auto owner = LockOwnerActor();
		if (!lease || !lease->active || !owner || owner->GetIsPendingDestroy()) { Destroy(); return; }
		const auto targetLifetime = target->GetWeakActor().lock();
		const auto origin = target->GetActorLocation();
		mVisited.push_back(target->GetUniqueID());
		ApplyCombatDamage(*target, GetDamage(), owner.get(), GetDamageTags(), GetDamagePayload(), {}, {}, DamageDeliveryType::Projectile, this);
		if (lease->active && mStatus && !target->GetIsPendingDestroy())
			if (auto* combatant = dynamic_cast<Combatant*>(target))
			{
				auto status = *mStatus; status.attributes.emplace_back(DamageAttributeIds::SuppressModuleTriggers, 1.f, 1.f);
				auto& abilities = combatant->GetAbilitySystemComponent(); const auto* existing = abilities.FindGameplayEffectById(status.definition.effectId);
				if (!existing || existing->stackCount < status.maxStacks) abilities.ApplyGameplayEffect(status, owner.get());
			}
		if (lease->active && !owner->GetIsPendingDestroy() && mSecondaryDamage > 0.f) Spawn(*owner, mSnapshot, origin, lease, mVisited, mRange, mSecondaryDamage, GetDamageTags(), 0.f, mStatus);
		Destroy();
	}
}
