#pragma once
#include "gameplay/module/ModuleResolver.h"
#include "framework/GameplayTag.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "gameplay/module/generated/ModuleRicochetProjectileActor.h"
#include "framework/Delegate.h"
#include "abilities/AbilityEvent.h"
#include <optional>
#include <random>

namespace vega
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
		bool Force(std::uint32_t instanceId, float powerMultiplier);
		std::size_t GetActiveActorCount() const;
		std::size_t GetRuleCount() const;
		// Run-counter damage multiplier (Odometer-style stacks) that kill explosions apply; their base is not a hit.
		void SetRunCounterDamageMultiplier(float multiplier) { mRunCounterDamageMultiplier = multiplier; }
	private:
		template<typename Rule> struct State { std::uint32_t instanceId = 0; Rule rule; std::shared_ptr<ModuleGeneratedLease> lease; };
		struct MineState : State<ModuleTrailMineRule> { float window = 0.f; float carriedDistance = 0.f; float power = 1.f; };
		struct LanceWindow { std::shared_ptr<ModuleGeneratedLease> lease; ModuleSpeedLanceRule rule; float remaining = 0.f; float peakSpeed = 0.f; };
		struct LanceEcho { std::shared_ptr<ModuleGeneratedLease> lease; ModuleSpeedLanceRule rule; float remaining = 0.f; float damage = 0.f; sf::Vector2f direction{}; };
		struct Event
		{
			std::uint32_t forcedInstanceId = 0;
			float power = 1.f;
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
			std::optional<ModuleRicochetProjectileSnapshot> primaryProjectile;
		};
		void OnEvent(const sas::AbilityEvent& event);
		void Flush();
		void Reconcile(const ResolvedModuleGeneratedRules& rules);
		void Execute(const Event& event);
		void Ricochet(const Event& event);
		void SpawnSalvos(const Event& event);
		void SpawnExplosions(const Event& event);
		void DropMines(float deltaSeconds);
		void TickLances(float deltaSeconds);
		void SpawnLance(const std::shared_ptr<ModuleGeneratedLease>& lease, const ModuleSpeedLanceRule& rule, float damage, const sf::Vector2f& direction);
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
		List<State<ModuleAnnularBurstRule>> mAnnularBursts;
		List<State<ModuleSpeedLanceRule>> mLances;
		List<LanceWindow> mLanceWindows;
		List<LanceEcho> mLanceEchoes;
		List<Event> mEvents;
		std::optional<ResolvedModuleGeneratedRules> mPending;
		std::uint64_t mNextKey = 1;
		std::uint64_t mFiredSerial = 0;
		bool mBusy = false;
		bool mShuttingDown = false;
		float mRunCounterDamageMultiplier = 1.f;
		std::mt19937 mRandom{ std::random_device{}() };
	};
}
