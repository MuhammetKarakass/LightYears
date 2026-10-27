#pragma once
#include "gameplay/module/ModulePlacementRules.h"
#include "gameplay/module/ModuleTriggerService.h"
#include "gameplay/progression/RunGold.h"
#include "abilities/AbilityEvent.h"
#include <map>
#include <memory>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	// Placement policy only; firing, power scaling and ability levels belong to their reusable owners.
	class UniqueModuleRuntime final
	{
	public:
		explicit UniqueModuleRuntime(LightYearsAbilitySystemComponent& abilities);
		~UniqueModuleRuntime() = default;
		void Replace(const ModuleRulesContext& rules, const List<ModulePlacement>& placements);
		void Tick(float deltaSeconds, bool aliveInPlay, bool inCombat);
		void SetGoldWallet(std::weak_ptr<RunGold> wallet) { mWallet = std::move(wallet); }
	private:
		struct Entry { std::uint32_t id = 0; UniqueModuleRule rule; List<std::uint32_t> targets; List<sas::AbilitySlot> slots; float elapsed = 0.f; };
		struct Echo { std::uint32_t source = 0, target = 0; float remaining = 0.f, power = 1.f; };
		void OnNatural(const ModuleRuleIdentity& rule);
		void OnAbility(const sas::AbilityLifecycleEvent& event);
		LightYearsAbilitySystemComponent& mAbilities;
		ModuleTriggerService& mTriggers;
		List<Entry> mEntries;
		List<Echo> mEchoes;
		std::map<std::uint32_t, float> mProfitReady;
		std::weak_ptr<RunGold> mWallet;
		float mTime = 0.f;
		ModuleTriggerService::Connection mNaturalConnection, mAbilityConnection;
	};
}
