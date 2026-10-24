#pragma once

#include "abilities/AbilityEvent.h"
#include "framework/Core.h"
#include "framework/Delegate.h"
#include "gameplay/ShieldComponent.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"

#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class SpaceShip;

	// Shield Battery: folds the placed battery rules into the ship's shield behaviour. The largest reserve cap and the largest
	// damaged fill win, so several batteries share one reserve rule instead of stacking.
	void ApplyShieldBatteryBehavior(const List<ResolvedModuleShieldBatteryRule>& rules, ShieldModuleBehavior& behavior);

	// Shield Rupture: owner-caused shield losses are remembered per enemy for historySeconds. A hit that breaks an enemy's shield
	// spawns an Energy explosion there sized by the shield the owner removed from it in that window. Keyed by unique id; the
	// enemy is reached only through its ShieldComponent during the hit, never stored as a pointer.
	class ModuleShieldRuptureRuntime final
	{
	public:
		explicit ModuleShieldRuptureRuntime(LightYearsAbilitySystemComponent& abilities);
		~ModuleShieldRuptureRuntime();
		ModuleShieldRuptureRuntime(const ModuleShieldRuptureRuntime&) = delete;
		ModuleShieldRuptureRuntime& operator=(const ModuleShieldRuptureRuntime&) = delete;
		void Replace(const List<ResolvedModuleShieldRuptureRule>& rules);
		void Tick(float deltaSeconds);
		std::size_t GetActiveActorCount() const;

	private:
		struct State
		{
			std::uint32_t instanceId = 0;
			std::size_t ruleIndex = 0;
			ModuleShieldRuptureRule rule;
			std::shared_ptr<ModuleGeneratedLease> lease;
		};
		struct ShieldLoss
		{
			std::uint64_t targetId = 0;
			float amount = 0.f;
			float remainingSeconds = 0.f;
		};
		void OnEvent(const sas::AbilityEvent& event);
		void Detonate(const State& state, const sf::Vector2f& origin, float lostShield);
		static void Cancel(const std::shared_ptr<ModuleGeneratedLease>& lease) noexcept;
		static void Prune(const std::shared_ptr<ModuleGeneratedLease>& lease);

		LightYearsAbilitySystemComponent& mAbilities;
		SpaceShip* mShip = nullptr; // Same owner's component lifetime; no registry identity.
		DelegateHandle mEventHandle;
		List<State> mStates;
		List<ShieldLoss> mLosses;
		std::uint64_t mNextKey = 1;
		bool mShuttingDown = false;
	};
}
