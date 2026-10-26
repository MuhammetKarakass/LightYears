#include "gameplay/combat/CombatantTakeover.h"
#include "enemy/EnemyActor.h"
#include "framework/World.h"
#include "framework/WorldServices.h"
#include "gameConfigs/CollisionLayers.h"
#include "gameplay/attributes/AttributeIds.h"

#include <algorithm>

namespace vega
{
	namespace
	{
		constexpr char kDamageModifierSource[] = "CombatantTakeover";

		bool IsAlive(const SpaceShip& ship)
		{
			return !ship.GetIsPendingDestroy() && ship.GetHealthComponent().GetHealth() > 0.f;
		}
	}

	CombatantTakeoverRegistry* CombatantTakeoverRegistry::Find(const World& world)
	{
		return world.Services().Find<CombatantTakeoverRegistry>();
	}

	CombatantTakeoverRegistry& CombatantTakeoverRegistry::GetOrCreate(World& world)
	{
		return world.Services().GetOrCreate<CombatantTakeoverRegistry>();
	}

	void CombatantTakeoverRegistry::Add(std::uint64_t targetId, const weak_ptr<CombatantTakeoverActor>& controller)
	{
		// One takeover per target: a newer controller replaces the stale entry explicitly.
		mControllers[targetId] = controller;
	}

	shared_ptr<CombatantTakeoverActor> CombatantTakeoverRegistry::FindController(std::uint64_t targetId)
	{
		const auto found = mControllers.find(targetId);
		if (found == mControllers.end()) return {};
		auto controller = found->second.lock();
		if (!controller || controller->GetIsPendingDestroy())
		{
			mControllers.erase(found);
			return {};
		}
		return controller;
	}

	void CombatantTakeoverRegistry::Remove(std::uint64_t targetId, const CombatantTakeoverActor* controller)
	{
		const auto found = mControllers.find(targetId);
		if (found == mControllers.end()) return;
		const auto current = found->second.lock();
		// Only the controller that registered the entry may remove it.
		if (!current || current.get() == controller) mControllers.erase(found);
	}

	bool IsCombatantTakenOver(const Actor& target)
	{
		const World* world = target.GetWorld();
		CombatantTakeoverRegistry* registry = world ? CombatantTakeoverRegistry::Find(*world) : nullptr;
		return registry && registry->FindController(target.GetUniqueID()) != nullptr;
	}

	bool CanTakeOverCombatant(const Actor& target)
	{
		const auto* enemy = dynamic_cast<const EnemyActor*>(&target);
		if (!enemy || !IsAlive(*enemy) || !enemy->GetEnemyRuntime().IsReady() || enemy->GetCollisionLayer() != GameCollision::Enemy) return false;
		const ControlTargetClass targetClass = enemy->GetControlTargetClass();
		if (targetClass != ControlTargetClass::Normal && targetClass != ControlTargetClass::Elite) return false;
		return !IsCombatantTakenOver(target);
	}

	weak_ptr<CombatantTakeoverActor> BeginCombatantTakeover(EnemyActor& target, const CombatantTakeoverRequest& request)
	{
		World* world = target.GetWorld();
		if (!world || !CanTakeOverCombatant(target) || !(request.seconds > 0.f) || request.taker.expired()) return {};
		const auto targetWeak = std::static_pointer_cast<EnemyActor>(target.GetWeakActor().lock());
		auto controller = world->SpawnActor<CombatantTakeoverActor>(targetWeak, request);
		const auto spawned = controller.lock();
		if (!spawned) return {};
		CombatantTakeoverRegistry::GetOrCreate(*world).Add(target.GetUniqueID(), controller);
		spawned->Start();
		return controller;
	}

	void EndCombatantTakeover(Actor& target)
	{
		const World* world = target.GetWorld();
		CombatantTakeoverRegistry* registry = world ? CombatantTakeoverRegistry::Find(*world) : nullptr;
		if (const auto controller = registry ? registry->FindController(target.GetUniqueID()) : nullptr) controller->Finish();
	}

	CombatantTakeoverActor::CombatantTakeoverActor(World* world, const weak_ptr<EnemyActor>& target, const CombatantTakeoverRequest& request)
		: Actor{ world }, mTarget{ target }, mRequest{ request }, mRemaining{ std::max(0.f, request.seconds) }
	{
		SetEnablePhysics(false);
	}

	void CombatantTakeoverActor::Start()
	{
		const auto target = mTarget.lock();
		if (!target) return;
		// Its reward is earned now; its own death later grants nothing.
		target->GrantKillRewards();
		target->SetCollisionLayer(GameCollision::FriendlySummon);
		target->SetCollisionMask(GameCollision::Enemy | GameCollision::EnemyBullet | GameCollision::Environment);
		HealthComponent& health = target->GetHealthComponent();
		if (mRequest.healthMultiplier > 0.f && mRequest.healthMultiplier != 1.f)
		{
			const float boostedHealth = health.GetHealth() * mRequest.healthMultiplier;
			target->GetAbilitySystemComponent().GetAttributes().AddModifier({ OwnerAttributeIds::MaxHealth, sas::AttributeModifierOperation::Multiply, mRequest.healthMultiplier });
			target->RefreshMovementAttributesFromRuntime();
			health.ChangeHealth(boostedHealth - health.GetHealth());
		}
		if (mRequest.healthFractionOfMax) health.ChangeHealth(std::max(1.f, health.GetMaxHealth() * *mRequest.healthFractionOfMax) - health.GetHealth());
		CombatRuntimeModifier modifier;
		modifier.outgoingDamageMultiplier = std::max(0.f, mRequest.damageMultiplier);
		target->GetCombatRuntime().SetRuntimeModifier(kDamageModifierSource, modifier);
	}

	void CombatantTakeoverActor::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);
		if (mFinished) return;
		const auto target = mTarget.lock();
		if (!target || !IsAlive(*target))
		{
			mFinished = true;
			if (World* world = GetWorld()) if (auto* registry = CombatantTakeoverRegistry::Find(*world)) registry->Remove(target ? target->GetUniqueID() : 0, this);
			Destroy();
			return;
		}
		mRemaining -= std::max(0.f, deltaTime);
		const auto taker = mRequest.taker.lock();
		if (mRemaining <= 0.f || !taker || taker->GetIsPendingDestroy()) Finish();
	}

	void CombatantTakeoverActor::Finish()
	{
		if (mFinished) return;
		mFinished = true;
		const auto target = mTarget.lock();
		if (World* world = GetWorld(); world && target) if (auto* registry = CombatantTakeoverRegistry::Find(*world)) registry->Remove(target->GetUniqueID(), this);
		if (target && IsAlive(*target))
		{
			target->ClearExternalControl();
			// No source: the expiry is not anyone's kill and grants nothing (rewards were granted at the start).
			target->GetHealthComponent().ChangeHealth(-target->GetHealthComponent().GetHealth());
		}
		Destroy();
	}
}
