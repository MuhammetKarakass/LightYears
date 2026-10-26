#include "gameplay/module/rules/legendary/LegendaryTwinRackRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/tags/GameplayTags.h"
#include <algorithm>

namespace vega
{
	LegendaryTwinRackRuntime::LegendaryTwinRackRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime }
	{}

	LegendaryTwinRackRuntime::~LegendaryTwinRackRuntime()
	{
		try { Clear(); } catch (...) {}
	}

	void LegendaryTwinRackRuntime::Replace(const List<ResolvedModuleLegendaryTwinRackRule>& rules)
	{
		mAbilities.UnregisterAbilityLifecycleObserver(mUseObserver);
		mUseObserver = {};
		mRules = rules;
		// Twin Rack is a capability, so duplicate placements do not multiply
		// the penalty or chain refunds. The strongest resolved refund wins.
		const auto strongest = std::max_element(mRules.begin(), mRules.end(), [](const auto& a, const auto& b) { return a.pairedRefundFraction < b.pairedRefundFraction; });
		mAbilities.ConfigureTwinRack(strongest != mRules.end(), strongest != mRules.end() ? strongest->cooldownMultiplier : 1.f);
		if (!mRules.empty() && !mUseObserver.IsValid())
		{
			AbilityLifecycleObserverFilter filter;
			filter.eventTag = GameplayTags::Event::Ability::Activated;
			filter.activationOrigin = sas::AbilityActivationOrigin::NormalInput;
			mUseObserver = mAbilities.RegisterAbilityLifecycleObserver(filter, [this](const auto& event) { OnAbilityUse(event); });
		}
	}

	void LegendaryTwinRackRuntime::Tick(float deltaSeconds)
	{
		(void)deltaSeconds;
	}

	void LegendaryTwinRackRuntime::Clear()
	{
		mAbilities.UnregisterAbilityLifecycleObserver(mUseObserver);
		mUseObserver = {};
		mRules.clear();
		mAbilities.ConfigureTwinRack(false);
	}

	void LegendaryTwinRackRuntime::OnAbilityUse(const sas::AbilityLifecycleEvent& event)
	{
		if (mRules.empty() || !mAbilities.IsTwinRackEnabled() || !sas::IsLoadoutAbilitySlot(event.slot)) return;
		auto* used = mAbilities.GetAbility(event.abilityHandle);
		if (!used || !used->IsActive() || used->GetDefinition().abilityId != event.abilityId.ToString()) return;
		auto* first = mAbilities.GetAbility(event.slot);
		auto* second = mAbilities.GetSecondAbility(event.slot);
		auto* pair = used == first ? second : used == second ? first : nullptr;
		if (!pair || !pair->IsOnCooldown()) return;
		float refund = 0.f;
		for (const auto& rule : mRules) refund = std::max(refund, rule.pairedRefundFraction);
		// Only the running recharge is reduced. A ready pair stores no credit,
		// and changing a timer never publishes a successful-use lifecycle event.
		pair->ReduceCooldownRemaining(pair->GetCooldownRemaining() * std::clamp(refund, 0.f, 1.f));
	}
}
