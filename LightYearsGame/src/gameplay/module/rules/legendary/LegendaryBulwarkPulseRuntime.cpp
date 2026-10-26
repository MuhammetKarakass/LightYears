#include "gameplay/module/rules/legendary/LegendaryBulwarkPulseRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/module/ModuleRulePredicates.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameplay/tags/GameplayTagSchema.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <utility>

namespace vega
{
	namespace
	{
		void Deactivate(const shared_ptr<ModuleGeneratedLease>& lease) noexcept
		{
			if (lease) lease->active = false;
		}
	}

	LegendaryBulwarkPulseRuntime::LegendaryBulwarkPulseRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime)
		: mAbilities{ abilities }, mCombatRuntime{ combatRuntime }, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) }
	{
		AbilityLifecycleObserverFilter filter;
		filter.eventTag = GameplayTags::Event::Ability::Activated;
		filter.activationOrigin = sas::AbilityActivationOrigin::NormalInput;
		mObserverHandle = mAbilities.RegisterAbilityLifecycleObserver(std::move(filter), [this](const sas::AbilityLifecycleEvent& event) { OnAbilityActivated(event); });
	}

	LegendaryBulwarkPulseRuntime::~LegendaryBulwarkPulseRuntime() { Clear(); }

	void LegendaryBulwarkPulseRuntime::Replace(const List<ResolvedModuleLegendaryBulwarkPulseRule>& rules)
	{
		List<State> next;
		for (const ResolvedModuleLegendaryBulwarkPulseRule& rule : rules)
		{
			State state{ rule };
			const auto previous = std::find_if(mStates.begin(), mStates.end(), [&rule](const State& candidate) { return candidate.rule.instanceId == rule.instanceId; });
			if (previous != mStates.end())
			{
				state.cooldownRemaining = previous->cooldownRemaining;
				state.lease = previous->lease;
				mStates.erase(previous);
			}
			if (!state.lease) state.lease = std::make_shared<ModuleGeneratedLease>();
			next.push_back(std::move(state));
		}
		for (State& previous : mStates) Deactivate(previous.lease);
		mStates = std::move(next);
		mRules = rules;
	}

	void LegendaryBulwarkPulseRuntime::Tick(float deltaSeconds)
	{
		if (deltaSeconds <= 0.f) return;
		for (State& state : mStates) state.cooldownRemaining = std::max(0.f, state.cooldownRemaining - deltaSeconds);
	}

	void LegendaryBulwarkPulseRuntime::OnAbilityActivated(const sas::AbilityLifecycleEvent& event)
	{
		if (mBusy || !mShip || mShip->GetIsPendingDestroy() || !mShip->GetWorld()) return;
		if (!sas::IsLoadoutAbilitySlot(event.slot) || !ModuleTriggerEventTagsMatch({ GameplayTags::Ability::Defense }, event.abilityTags)) return;

		List<std::pair<ResolvedModuleLegendaryBulwarkPulseRule, shared_ptr<ModuleGeneratedLease>>> waves;
		for (State& state : mStates)
		{
			if (state.cooldownRemaining > 0.f) continue;
			state.cooldownRemaining = state.rule.cooldownSeconds;
			waves.emplace_back(state.rule, state.lease);
		}

		mBusy = true;
		try
		{
			for (const auto& wave : waves) Detonate(wave.first, wave.second);
		}
		catch (...)
		{
			mBusy = false;
			throw;
		}
		mBusy = false;
	}

	void LegendaryBulwarkPulseRuntime::Detonate(const ResolvedModuleLegendaryBulwarkPulseRule& rule, const shared_ptr<ModuleGeneratedLease>& lease)
	{
		if (!lease || !lease->active || !mShip || mShip->GetIsPendingDestroy() || !mShip->GetWorld()) return;
		const float maxHealth = mShip->GetHealthComponent().GetMaxHealth();
		const float damage = rule.baseDamage + rule.healthCoefficient * std::max(0.f, maxHealth - rule.healthThreshold);
		if (!(damage > 0.f) || !(rule.radius > 0.f)) return;
		ApplyModuleGeneratedAreaDamage(*mShip, *mShip, lease, mShip->GetActorLocation(), rule.radius, damage, { DamageTypeSchema::Energy });
	}

	void LegendaryBulwarkPulseRuntime::Clear()
	{
		for (State& state : mStates) Deactivate(state.lease);
		mStates.clear();
		mRules.clear();
		if (mObserverHandle.IsValid())
		{
			mAbilities.UnregisterAbilityLifecycleObserver(mObserverHandle);
			mObserverHandle = {};
		}
	}
}
