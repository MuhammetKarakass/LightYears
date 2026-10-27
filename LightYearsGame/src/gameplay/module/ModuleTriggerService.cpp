#include "gameplay/module/ModuleTriggerService.h"
#include <algorithm>
#include <cmath>

namespace vega
{
	struct ModuleTriggerService::State
	{
		std::function<bool(std::uint32_t, float)> dispatch;
		std::function<void(const ModuleRuleIdentity&)> observer;
		std::uint64_t dispatchGeneration = 0, observerGeneration = 0;
	};
	ModuleTriggerService::ModuleTriggerService() : mState{ std::make_shared<State>() } {}
	ModuleTriggerService::Connection ModuleTriggerService::RegisterDispatcher(std::function<bool(std::uint32_t, float)> dispatch)
	{
		const auto generation = ++mState->dispatchGeneration;
		mState->dispatch = std::move(dispatch);
		return Connection{ [weak = std::weak_ptr<State>{ mState }, generation] { if (auto state = weak.lock(); state && state->dispatchGeneration == generation) state->dispatch = {}; } };
	}
	ModuleTriggerService::Connection ModuleTriggerService::ObserveNatural(std::function<void(const ModuleRuleIdentity&)> observer)
	{
		const auto generation = ++mState->observerGeneration;
		mState->observer = std::move(observer);
		return Connection{ [weak = std::weak_ptr<State>{ mState }, generation] { if (auto state = weak.lock(); state && state->observerGeneration == generation) state->observer = {}; } };
	}
	void ModuleTriggerService::BeginEvent()
	{
		if (mDepth++ == 0) { mVisited.clear(); mReported.clear(); mPending.clear(); }
	}
	void ModuleTriggerService::EndEvent()
	{
		if (mDepth > 1) { --mDepth; return; }
		// Keep the event open while observers force neighbours. Natural reports are fully collected first.
		try
		{
			while (!mPending.empty())
			{
				const auto rule = mPending.front(); mPending.erase(mPending.begin());
				if (auto observer = mState->observer) observer(rule);
			}
		}
		catch (...) { mDepth = 0; mPending.clear(); throw; }
		mDepth = 0;
	}
	void ModuleTriggerService::Notify(const ModuleRuleIdentity& rule, bool natural)
	{
		EventScope scope{ *this };
		natural = natural && !mForcing;
		mVisited.insert(rule.instanceId);
		if (natural && mReported.insert(rule.instanceId).second)
		{
			++mNaturalCounts[rule.instanceId];
			mPending.push_back(rule);
		}
	}
	bool ModuleTriggerService::Force(std::uint32_t instanceId, float powerMultiplier)
	{
		if (!std::isfinite(powerMultiplier) || powerMultiplier <= 0.f || !mState->dispatch) return false;
		EventScope scope{ *this };
		if (!mVisited.insert(instanceId).second) return false;
		const bool previous = std::exchange(mForcing, true);
		bool fired = false;
		try { auto dispatch = mState->dispatch; fired = dispatch(instanceId, powerMultiplier); }
		catch (...) { mForcing = previous; throw; }
		mForcing = previous;
		if (fired) ++mForcedCounts[instanceId];
		return fired;
	}
	List<GameplayTag> ModuleTriggerService::AbilityEventTags(std::uint32_t instanceId, const List<GameplayTag>& tags) const
	{
		List<GameplayTag> result = tags;
		const auto found = mAliases.find(instanceId);
		if (found != mAliases.end())
			for (const auto& [required, alternate] : found->second)
				if (std::any_of(tags.begin(), tags.end(), [&alternate](const GameplayTag& tag) { return tag.MatchesTag(alternate); })) result.push_back(required);
		return result;
	}
	bool ModuleTriggerService::MatchesAbilityEventTags(std::uint32_t instanceId, const List<GameplayTag>& required, const List<GameplayTag>& tags) const
	{
		return ModuleTriggerEventTagsMatch(required, AbilityEventTags(instanceId, tags));
	}
	std::uint64_t ModuleTriggerService::GetNaturalCount(std::uint32_t id) const { const auto found = mNaturalCounts.find(id); return found == mNaturalCounts.end() ? 0 : found->second; }
	std::uint64_t ModuleTriggerService::GetForcedCount(std::uint32_t id) const { const auto found = mForcedCounts.find(id); return found == mForcedCounts.end() ? 0 : found->second; }
}
