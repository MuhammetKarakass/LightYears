#include "gameplay/module/rules/legendary/LegendaryParasiteHullRuntime.h"
#include "enemy/EnemyActor.h"
#include "framework/World.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/combat/CombatantTakeover.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "player/PlayerSpaceShip.h"

#include <algorithm>
#include <limits>

namespace vega
{
	namespace
	{
		bool IsAlive(const SpaceShip& ship)
		{
			return !ship.GetIsPendingDestroy() && ship.GetHealthComponent().GetHealth() > 0.f;
		}
	}

	LegendaryParasiteHullRuntime::LegendaryParasiteHullRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime)
		: mAbilities{ abilities }, mCombatRuntime{ combatRuntime }, mOwner{ dynamic_cast<PlayerSpaceShip*>(&abilities.GetOwner()) } {}

	LegendaryParasiteHullRuntime::~LegendaryParasiteHullRuntime() { Clear(); }

	void LegendaryParasiteHullRuntime::Replace(const List<ResolvedModuleLegendaryParasiteHullRule>& rules)
	{
		if (rules.empty())
		{
			if (mPossessing) Return();
			Clear();
			return;
		}
		// A rebuild with the module still installed keeps the cooldown and a running possession.
		mRules = rules;
	}

	void LegendaryParasiteHullRuntime::Clear()
	{
		mRules.clear();
		mHost.reset();
		mPossessing = false;
		mElapsed = 0.f;
		mCooldownRemaining = 0.f;
	}

	bool LegendaryParasiteHullRuntime::TryPossess(const DamageContext& context)
	{
		(void)context;
		if (mRules.empty() || !mOwner || mPossessing || mCooldownRemaining > 0.f || !mOwner->GetWorld()) return false;
		const ResolvedModuleLegendaryParasiteHullRule rule = *std::max_element(mRules.begin(), mRules.end(), [](const auto& left, const auto& right) { return left.borrowedHealthBonus < right.borrowedHealthBonus; });
		const sf::Vector2f origin = mOwner->GetActorLocation();
		EnemyActor* host = nullptr;
		float nearest = std::numeric_limits<float>::max();
		for (const auto& candidate : targeting::FindOpposingCombatants(*mOwner->GetWorld(), *mOwner, origin, rule.possessionRange, false))
		{
			auto* enemy = dynamic_cast<EnemyActor*>(candidate.get());
			if (!enemy || !CanTakeOverCombatant(*enemy)) continue;
			const sf::Vector2f offset = enemy->GetActorLocation() - origin;
			const float distance = offset.x * offset.x + offset.y * offset.y;
			if (distance < nearest) { nearest = distance; host = enemy; }
		}
		if (!host) return false;
		CombatantTakeoverRequest request;
		request.taker = mOwner->GetWeakActor();
		request.seconds = rule.maximumSeconds;
		request.healthMultiplier = 1.f + rule.borrowedHealthBonus;
		request.damageMultiplier = 1.f + rule.borrowedDamageBonus;
		if (BeginCombatantTakeover(*host, request).expired()) return false;
		if (!mOwner->BeginPossession(*host))
		{
			EndCombatantTakeover(*host);
			return false;
		}
		mActiveRule = rule;
		mHost = std::static_pointer_cast<EnemyActor>(host->GetWeakActor().lock());
		mLastHostLocation = host->GetActorLocation();
		mElapsed = 0.f;
		mPossessing = true;
		mCooldownRemaining = rule.cooldownSeconds;
		return true;
	}

	void LegendaryParasiteHullRuntime::Tick(float deltaSeconds)
	{
		const float step = std::max(0.f, deltaSeconds);
		mCooldownRemaining = std::max(0.f, mCooldownRemaining - step);
		if (!mPossessing) return;
		const auto host = mHost.lock();
		const bool hostAlive = host && IsAlive(*host);
		if (hostAlive)
		{
			mLastHostLocation = host->GetActorLocation();
			mElapsed = std::min(mElapsed + step, mActiveRule.maximumSeconds);
		}
		if (!hostAlive || mElapsed >= mActiveRule.maximumSeconds) Return();
	}

	void LegendaryParasiteHullRuntime::Return()
	{
		mPossessing = false;
		if (const auto host = mHost.lock(); host && IsAlive(*host))
		{
			mLastHostLocation = host->GetActorLocation();
			EndCombatantTakeover(*host);
		}
		mHost.reset();
		if (!mOwner || mOwner->GetIsPendingDestroy()) return;
		mOwner->EndPossession(mLastHostLocation);
		HealthComponent& health = mOwner->GetHealthComponent();
		const float fraction = std::clamp(std::max(mActiveRule.minimumReturnHealthFraction, mActiveRule.repairMaxHealthFractionPerSecond * mElapsed), 0.f, 1.f);
		health.ChangeHealth(health.GetMaxHealth() * fraction - health.GetHealth());
	}
}
