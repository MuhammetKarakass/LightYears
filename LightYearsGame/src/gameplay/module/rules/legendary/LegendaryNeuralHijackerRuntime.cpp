#include "gameplay/module/rules/legendary/LegendaryNeuralHijackerRuntime.h"
#include "enemy/EnemyActor.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/CombatantTakeover.h"

#include <algorithm>

namespace vega
{
	LegendaryNeuralHijackerRuntime::LegendaryNeuralHijackerRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime)
		: mAbilities{ abilities }, mCombatRuntime{ combatRuntime }, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) } {}

	LegendaryNeuralHijackerRuntime::~LegendaryNeuralHijackerRuntime() { Clear(); }

	void LegendaryNeuralHijackerRuntime::Replace(const List<ResolvedModuleLegendaryNeuralHijackerRule>& rules)
	{
		if (rules.empty()) { Clear(); return; }
		mRules = rules;
		if (!mInterceptor.IsValid())
			mInterceptor = mCombatRuntime.GetLethalHitInterceptors().Register([this](SpaceShip& target, const DamageContext& context) { return Intercept(target, context); });
	}

	void LegendaryNeuralHijackerRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
		mAllies.erase(std::remove_if(mAllies.begin(), mAllies.end(), [](const weak_ptr<EnemyActor>& ally) { const auto actor = ally.lock(); return !actor || actor->GetIsPendingDestroy(); }), mAllies.end());
	}

	void LegendaryNeuralHijackerRuntime::Clear()
	{
		mInterceptor.Reset();
		mRules.clear();
		EndAllies();
	}

	std::size_t LegendaryNeuralHijackerRuntime::GetAllyCount() const
	{
		return static_cast<std::size_t>(std::count_if(mAllies.begin(), mAllies.end(), [](const weak_ptr<EnemyActor>& ally) { const auto actor = ally.lock(); return actor && !actor->GetIsPendingDestroy() && IsCombatantTakenOver(*actor); }));
	}

	LethalDamageResolution LegendaryNeuralHijackerRuntime::Intercept(SpaceShip& target, const DamageContext& context)
	{
		// Only the owner's own hits hijack; allies' kills are ordinary kills, so there is no chain.
		if (mRules.empty() || !mShip || context.source != mShip) return LethalDamageResolution::None;
		auto* enemy = dynamic_cast<EnemyActor*>(&target);
		if (!enemy || !CanTakeOverCombatant(*enemy)) return LethalDamageResolution::None;
		const auto strongest = std::max_element(mRules.begin(), mRules.end(), [](const auto& left, const auto& right) { return left.allySeconds < right.allySeconds; });
		CombatantTakeoverRequest request;
		request.taker = mShip->GetWeakActor();
		request.seconds = strongest->allySeconds;
		request.healthFractionOfMax = strongest->allyHealthFraction;
		request.damageMultiplier = strongest->allyDamageMultiplier;
		if (BeginCombatantTakeover(*enemy, request).expired()) return LethalDamageResolution::None;
		mAllies.push_back(std::static_pointer_cast<EnemyActor>(enemy->GetWeakActor().lock()));
		return LethalDamageResolution::ConvertedKill;
	}

	void LegendaryNeuralHijackerRuntime::EndAllies()
	{
		const List<weak_ptr<EnemyActor>> allies = std::move(mAllies);
		mAllies.clear();
		for (const auto& ally : allies)
			if (const auto actor = ally.lock(); actor && !actor->GetIsPendingDestroy()) EndCombatantTakeover(*actor);
	}
}
