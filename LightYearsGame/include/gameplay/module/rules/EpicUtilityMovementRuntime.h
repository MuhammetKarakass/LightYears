#pragma once

#include "abilities/AbilityEvent.h"
#include "attributes/AttributeSystem.h"
#include "framework/Core.h"
#include "framework/Delegate.h"
#include "framework/GameplayTag.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/rules/EpicUtilityMovementRules.h"

#include <SFML/System/Vector2.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;
	class SpaceShip;
	class PlayerSpaceShip;

	// Owner-scoped runtime for the Epic Utility/Movement modules (Cascade Reactor, Field Mastery, Assault Vector, Phase Slip).
	// ShipModuleApplier owns one instance, replaces its rules on every rebuild and ticks it each frame. Every handle it creates
	// (outgoing damage modifiers, Armor modifier, damage protection) is released by Replace, Clear or destruction.
	class ModuleEpicUtilityMovementRuntime final
	{
	public:
		ModuleEpicUtilityMovementRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~ModuleEpicUtilityMovementRuntime();
		ModuleEpicUtilityMovementRuntime(const ModuleEpicUtilityMovementRuntime&) = delete;
		ModuleEpicUtilityMovementRuntime& operator=(const ModuleEpicUtilityMovementRuntime&) = delete;

		void Replace(const ResolvedModuleEpicRules& rules);
		void Tick(float deltaSeconds);
		// Called for each loadout ability the owner activates. Cascade chains advance here; Utility uses count toward Field Mastery.
		void OnAbilityUse(const std::string& abilityId, const List<GameplayTag>& tags);
		// Receives every Field Mastery level gain (ability id, new level) so the owning Player keeps it for the run.
		void SetAbilityLevelSink(std::function<void(const std::string&, int)> sink) { mAbilityLevelSink = std::move(sink); }

		// Phase Slip lock: Primary and loadout ability systems report disabled while a phase is open.
		bool IsSystemLocked(const GameplayTag& systemTag) const;
		// Field Mastery: Utility abilities cannot be upgraded with Scrap while the module is installed.
		bool IsScrapUpgradeLocked(const List<GameplayTag>& abilityTags) const;

		int GetCascadeStep(std::uint32_t instanceId) const;
		float GetUseCount(const std::string& abilityId) const;
		bool IsPhaseActive() const { return mPhaseRemaining > 0.f; }
		bool IsPhaseArmed() const { return mPhaseArmed; }
		float GetPhaseCooldown() const { return mPhaseCooldown; }
		float GetAssaultDamageBonus() const { return mAssaultDamageBonus; }
		float GetAssaultArmorBonus() const { return mAssaultArmorBonus; }

	private:
		struct CascadeState
		{
			std::uint32_t instanceId = 0;
			GameplayTag lastFamily;
			int step = 0;
			float remainingSeconds = 0.f;
			float appliedMultiplier = 1.f;
		};

		struct UseCounter
		{
			std::string abilityId;
			double uses = 0.0;
		};

		struct PendingExplosion
		{
			sf::Vector2f origin{};
			float damage = 0.f;
			float radius = 0.f;
		};

		void OnEvent(const sas::AbilityEvent& event);
		void TickCascades(float deltaSeconds);
		void AdvanceCascade(CascadeState& state, const GameplayTag& family);
		void ApplyCascadeModifier(CascadeState& state, const ResolvedModuleCascadeRule& rule, const GameplayTag& family);
		void RemoveCascadeModifier(CascadeState& state);
		void TickPhase(float deltaSeconds);
		void StartPhase();
		void EndPhase();
		void ClearPhase();
		void TickAssaultVector(float deltaSeconds);
		void ApplyAssaultDamage(float bonus);
		void ApplyAssaultArmor(float bonus);
		void ClearAssault();
		void AddUses(const std::string& abilityId, double amount);
		void ProcessFieldMastery();
		bool LevelUpAbility(const std::string& abilityId);
		void TransferUses(const std::string& sourceAbilityId);
		void ProcessExplosions();
		int FieldMasteryThreshold() const;
		void Clear();

		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		SpaceShip* mShip = nullptr;
		PlayerSpaceShip* mPlayer = nullptr;
		DelegateHandle mEventHandle;
		std::function<void(const std::string&, int)> mAbilityLevelSink;

		ResolvedModuleEpicRules mRules;
		List<CascadeState> mCascades;
		List<UseCounter> mUseCounters;

		bool mPhaseAfterburning = false;
		float mPhaseRemaining = 0.f;
		float mPhaseCooldown = 0.f;
		bool mPhaseArmed = false;
		List<PendingExplosion> mPendingExplosions;

		bool mHasLastLocation = false;
		sf::Vector2f mLastLocation{};
		float mAssaultDamageBonus = 0.f;
		float mAssaultArmorBonus = 0.f;
		sas::AttributeModifierHandle mAssaultArmorHandle;

		bool mShuttingDown = false;
	};
}
