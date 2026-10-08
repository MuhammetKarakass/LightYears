#pragma once
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "framework/Delegate.h"
#include "abilities/AbilityEvent.h"
#include <optional>

namespace ly
{
	class LightYearsAbilitySystemComponent;
	class SpaceShip;
	class ModuleGeneratedRuntime final
	{
	public:
		explicit ModuleGeneratedRuntime(LightYearsAbilitySystemComponent& abilities);
		~ModuleGeneratedRuntime();
		ModuleGeneratedRuntime(const ModuleGeneratedRuntime&) = delete;
		ModuleGeneratedRuntime& operator=(const ModuleGeneratedRuntime&) = delete;
		void Replace(const ResolvedModuleGeneratedRules& rules);
		void Tick(float deltaSeconds);
		std::size_t GetActiveActorCount() const;
		std::size_t GetRuleCount() const;
	private:
		template<typename Rule> struct State { std::uint32_t instanceId = 0; Rule rule; std::shared_ptr<ModuleGeneratedLease> lease; };
		struct MineState : State<ModuleTrailMineRule> { float window = 0.f; float carriedDistance = 0.f; };
		struct Event
		{
			GameplayTag tag;
			sas::ContentId abilityId;
			List<GameplayTag> abilityTags;
			List<GameplayTag> damageTags;
			std::shared_ptr<DamageHitGroup> hitGroup;
			weak_ptr<Actor> target;
			sf::Vector2f origin{};
			float damage = 0.f;
			float targetMaxHealth = 0.f;
			List<GameplayTag> targetStatuses;
			List<sas::GameplayEffectSpec> targetStatusEffects;
		};
		void OnEvent(const sas::AbilityEvent& event);
		void Flush();
		void Reconcile(const ResolvedModuleGeneratedRules& rules);
		void Execute(const Event& event);
		void Ricochet(const Event& event);
		void SpawnSalvos(const Event& event);
		void SpawnExplosions(const Event& event);
		void DropMines(float deltaSeconds);
		static void Cancel(const std::shared_ptr<ModuleGeneratedLease>& lease) noexcept;
		static void Prune(const std::shared_ptr<ModuleGeneratedLease>& lease);
		LightYearsAbilitySystemComponent& mAbilities;
		SpaceShip* mShip = nullptr;
		DelegateHandle mEventHandle;
		List<State<ModuleRicochetRule>> mRicochets;
		List<State<ModuleGuidedSalvoRule>> mSalvos;
		List<State<ModuleDelayedExplosionRule>> mAftershocks;
		List<MineState> mMines;
		List<State<ModuleKillExplosionRule>> mKillExplosions;
		List<Event> mEvents;
		std::optional<ResolvedModuleGeneratedRules> mPending;
		std::uint64_t mNextKey = 1;
		bool mBusy = false;
		bool mShuttingDown = false;
	};
}
