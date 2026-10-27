#pragma once
#include "framework/Core.h"
#include "gameplay/module/ModuleRulePredicates.h"
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <utility>

namespace vega
{
	enum class ModuleRuleKind { TriggerBuff, EventAction, OwnerBurst, DamageWindow, Generated, ShieldReflection, ShieldRupture, Epic, Legendary, Status, RunCounter };
	struct ModuleRuleIdentity { std::uint32_t instanceId = 0; ModuleRuleKind kind = ModuleRuleKind::TriggerBuff; std::size_t ruleIndex = 0; };

	// Ship-owned dispatch boundary. No ability activation and no retained actors or event context pointers.
	class ModuleTriggerService final
	{
		struct State;
	public:
		class Connection final
		{
		public:
			Connection() = default;
			explicit Connection(std::function<void()> remove) : mRemove{ std::move(remove) } {}
			~Connection() { Reset(); }
			Connection(Connection&& other) noexcept : mRemove{ std::exchange(other.mRemove, {}) } {}
			Connection& operator=(Connection&& other) noexcept { if (this != &other) { Reset(); mRemove = std::exchange(other.mRemove, {}); } return *this; }
			Connection(const Connection&) = delete;
			Connection& operator=(const Connection&) = delete;
			void Reset() { if (auto remove = std::exchange(mRemove, {})) remove(); }
		private:
			std::function<void()> mRemove;
		};
		class EventScope final
		{
		public:
			explicit EventScope(ModuleTriggerService& service) : mService{ service } { mService.BeginEvent(); }
			~EventScope() noexcept(false) { mService.EndEvent(); }
			EventScope(const EventScope&) = delete;
			EventScope& operator=(const EventScope&) = delete;
		private:
			ModuleTriggerService& mService;
		};
		ModuleTriggerService();
		Connection RegisterDispatcher(std::function<bool(std::uint32_t, float)> dispatch);
		Connection ObserveNatural(std::function<void(const ModuleRuleIdentity&)> observer);
		void Notify(const ModuleRuleIdentity& rule, bool natural = true);
		bool Force(std::uint32_t instanceId, float powerMultiplier = 1.f);
		bool IsForcing() const { return mForcing; }
		using FamilyAlias = std::pair<GameplayTag, GameplayTag>;
		void SetAbilityFamilyAliases(std::map<std::uint32_t, List<FamilyAlias>> aliases) { mAliases = std::move(aliases); }
		List<GameplayTag> AbilityEventTags(std::uint32_t instanceId, const List<GameplayTag>& tags) const;
		bool MatchesAbilityEventTags(std::uint32_t instanceId, const List<GameplayTag>& required, const List<GameplayTag>& tags) const;
		std::uint64_t GetNaturalCount(std::uint32_t id) const;
		std::uint64_t GetForcedCount(std::uint32_t id) const;
	private:
		void BeginEvent();
		void EndEvent();
		std::shared_ptr<State> mState;
		std::map<std::uint32_t, List<FamilyAlias>> mAliases;
		std::map<std::uint32_t, std::uint64_t> mNaturalCounts, mForcedCounts;
		List<ModuleRuleIdentity> mPending;
		std::set<std::uint32_t> mVisited, mReported;
		std::size_t mDepth = 0;
		bool mForcing = false;
	};
}
