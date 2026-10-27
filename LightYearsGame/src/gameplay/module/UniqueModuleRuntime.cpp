#include "gameplay/module/UniqueModuleRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/tags/GameplayTags.h"
#include <algorithm>
#include <set>

namespace vega
{
	UniqueModuleRuntime::UniqueModuleRuntime(LightYearsAbilitySystemComponent& abilities) : mAbilities{ abilities }, mTriggers{ abilities.GetModuleTriggerService() }
	{
		mNaturalConnection = mTriggers.ObserveNatural([this](const ModuleRuleIdentity& rule) { OnNatural(rule); });
		AbilityLifecycleObserverFilter filter;
		filter.eventTag = GameplayTags::Event::Ability::Activated;
		filter.activationOrigin = sas::AbilityActivationOrigin::NormalInput;
		const auto handle = mAbilities.RegisterAbilityLifecycleObserver(filter, [this](const sas::AbilityLifecycleEvent& event) { OnAbility(event); });
		mAbilityConnection = ModuleTriggerService::Connection{ [&abilities, handle] { abilities.UnregisterAbilityLifecycleObserver(handle); } };
	}
	void UniqueModuleRuntime::Replace(const ModuleRulesContext& rules, const List<ModulePlacement>& placements)
	{
		List<Entry> desired;
		std::map<std::uint32_t, List<ModuleTriggerService::FamilyAlias>> aliases;
		for (const auto& placement : placements)
		{
			const auto* module = rules.lookup(placement.moduleId);
			if (!module || !module->unique) continue;
			Entry entry; entry.id = placement.instanceId; entry.rule = *module->unique;
			for (const auto& old : mEntries) if (old.id == entry.id || (entry.rule.kind == UniqueModuleKind::Pulsar && old.rule.kind == UniqueModuleKind::Pulsar)) { entry.elapsed = old.elapsed; break; }
			const auto adjacent = ModulePlacementRules::GetAdjacentPlacements(rules, placements, placement);
			const auto front = placement.origin + ModulePlacementRules::FacingOffset(placement.rotationQuarterTurns);
			for (const auto& target : placements)
			{
				if (std::find(adjacent.begin(), adjacent.end(), target.instanceId) == adjacent.end()) continue;
				const auto* definition = rules.lookup(target.moduleId);
				if (!definition || definition->unique) continue;
				entry.targets.push_back(target.instanceId);
				const auto slot = AbilitySlotForModuleSystem(ModuleSystemForPlacement(rules, *definition, target));
				if (slot != sas::AbilitySlot::None) entry.slots.push_back(slot);
				if (entry.rule.kind == UniqueModuleKind::Mirror)
				{
					const auto cells = ModulePlacementRules::GetCoveredCells(*definition, target);
					if (std::find(cells.begin(), cells.end(), front) != cells.end())
						aliases[target.instanceId] = { { GameplayTags::Ability::Offense, GameplayTags::Ability::Defense }, { GameplayTags::Ability::Defense, GameplayTags::Ability::Offense }, { GameplayTags::Ability::Movement, GameplayTags::Ability::Utility }, { GameplayTags::Ability::Utility, GameplayTags::Ability::Movement } };
				}
			}
			desired.push_back(std::move(entry));
		}
		mEntries = std::move(desired);
		mTriggers.SetAbilityFamilyAliases(std::move(aliases));
		mEchoes.erase(std::remove_if(mEchoes.begin(), mEchoes.end(), [this](const Echo& echo)
		{
			return std::none_of(mEntries.begin(), mEntries.end(), [&echo](const Entry& entry) { return entry.id == echo.source && std::find(entry.targets.begin(), entry.targets.end(), echo.target) != entry.targets.end(); });
		}), mEchoes.end());
	}
	void UniqueModuleRuntime::OnNatural(const ModuleRuleIdentity& fired)
	{
		std::set<UniqueModuleKind> handled;
		const auto entries = mEntries;
		for (const auto& entry : entries)
		{
			if (std::find(entry.targets.begin(), entry.targets.end(), fired.instanceId) == entry.targets.end()) continue;
			const bool firstKind = handled.insert(entry.rule.kind).second;
			if (entry.rule.kind == UniqueModuleKind::Relay)
				for (const auto target : entry.targets) if (target != fired.instanceId) mTriggers.Force(target);
			if (entry.rule.kind == UniqueModuleKind::Echo && firstKind) mEchoes.push_back({ entry.id, fired.instanceId, entry.rule.echoDelaySeconds, entry.rule.echoPowerFraction });
			if (entry.rule.kind == UniqueModuleKind::ProfitShare && firstKind && mTime >= mProfitReady[fired.instanceId])
			{
				if (const auto wallet = mWallet.lock()) wallet->Award(entry.rule.goldPerTrigger);
				mProfitReady[fired.instanceId] = mTime + entry.rule.cooldownSeconds;
			}
		}
	}
	void UniqueModuleRuntime::OnAbility(const sas::AbilityLifecycleEvent& event)
	{
		if (!sas::IsLoadoutAbilitySlot(event.slot)) return;
		const auto entries = mEntries;
		for (const auto& entry : entries)
			if (entry.rule.kind == UniqueModuleKind::Spark && std::find(entry.slots.begin(), entry.slots.end(), event.slot) != entry.slots.end())
				for (const auto target : entry.targets) mTriggers.Force(target);
	}
	void UniqueModuleRuntime::Tick(float deltaSeconds, bool aliveInPlay, bool inCombat)
	{
		if (deltaSeconds <= 0.f || !aliveInPlay) return;
		mTime += deltaSeconds;
		List<Echo> due;
		for (auto& echo : mEchoes) { echo.remaining -= deltaSeconds; if (echo.remaining <= 0.00001f) due.push_back(echo); }
		mEchoes.erase(std::remove_if(mEchoes.begin(), mEchoes.end(), [](const Echo& echo) { return echo.remaining <= 0.00001f; }), mEchoes.end());
		for (const auto& echo : due) { ModuleTriggerService::EventScope event{ mTriggers }; mTriggers.Force(echo.target, echo.power); }
		if (!inCombat) return;
		// One pulse event covers every Pulsar, so overlapping copies cannot stack.
		float elapsed = deltaSeconds;
		while (elapsed > 0.f)
		{
			float step = elapsed;
			for (const auto& entry : mEntries) if (entry.rule.kind == UniqueModuleKind::Pulsar) step = std::min(step, std::max(0.f, entry.rule.pulsarIntervalSeconds - entry.elapsed));
			elapsed -= step;
			List<std::uint32_t> targets;
			for (auto& entry : mEntries)
			{
				if (entry.rule.kind != UniqueModuleKind::Pulsar) continue;
				entry.elapsed += step;
				if (entry.elapsed + 0.00001f < entry.rule.pulsarIntervalSeconds) continue;
				entry.elapsed = 0.f;
				targets.insert(targets.end(), entry.targets.begin(), entry.targets.end());
			}
			ModuleTriggerService::EventScope event{ mTriggers };
			for (const auto target : targets) mTriggers.Force(target);
			if (step == 0.f && elapsed <= 0.f) break;
		}
	}
}
