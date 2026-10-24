#pragma once

#include "gameplay/HealthComponent.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/CombatRuntimeModifiers.h"
#include "gameplay/module/ModulePlacementRules.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/rules/EpicDefenseRules.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"

#include <cstddef>

namespace vega
{
	// Last Stand's post-revive bonus: one timed set of owner attribute modifiers. A new revive restarts the full duration and
	// replaces the previous set, so bonuses never stack. The stats are Attack Power, Energy Power, Attack Speed and Armor.
	class ModuleReviveBonusRuntime final
	{
	public:
		explicit ModuleReviveBonusRuntime(LightYearsAbilitySystemComponent& abilities) : mAbilities{ abilities } {}
		ModuleReviveBonusRuntime(const ModuleReviveBonusRuntime&) = delete;
		ModuleReviveBonusRuntime& operator=(const ModuleReviveBonusRuntime&) = delete;

		void Start(float seconds, float multiplier);
		void Tick(float deltaSeconds);
		void Reset();
		bool IsActive() const { return mRemainingSeconds > 0.f; }
		float GetRemainingSeconds() const { return mRemainingSeconds; }
		std::size_t GetOwnedHandleCount() const { return mHandles.size(); }

	private:
		void RemoveModifiers();

		LightYearsAbilitySystemComponent& mAbilities;
		List<sas::AttributeModifierHandle> mHandles;
		float mRemainingSeconds = 0.f;
	};

	// One resolved hull-threshold rule of a placement (main package or active side feature), at its effective level.
	struct ModuleEmergencyEntry
	{
		float healthAtMost = 0.f;
		float cooldownRecoveryMultiplier = 1.f;
		float abilityDamageMultiplier = 1.f;
		float movementSpeedMultiplier = 1.f;
	};

	// Emergency Protocol runtime. Entries are rebuilt with every applier rebuild. Each tick the strongest active multiplier of each
	// kind applies (entries do not stack): Q/E/F/R cooldowns recover faster, Q/E/F/R outgoing damage is scoped per loadout slot, and
	// movement speed is a source-owned ship modifier. Nothing resets when health rises back above the threshold.
	class ModuleEmergencyProtocolRuntime final
	{
	public:
		ModuleEmergencyProtocolRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
		ModuleEmergencyProtocolRuntime(const ModuleEmergencyProtocolRuntime&) = delete;
		ModuleEmergencyProtocolRuntime& operator=(const ModuleEmergencyProtocolRuntime&) = delete;

		void Replace(const List<ModulePlacement>& activePlacements, const ModuleRulesContext& rules, const ResolvedModuleEffects& effects, ShipRuntimeModifiers* movementModifiers);
		void Tick(float deltaSeconds, const HealthComponent* health);
		void Clear();
		std::size_t GetEntryCount() const { return mEntries.size(); }

	private:
		void RecoverCooldowns(float seconds);
		void SyncDamage(float multiplier);
		void SyncMovement(float multiplier);

		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		ShipRuntimeModifiers* mMovementModifiers = nullptr;
		List<ModuleEmergencyEntry> mEntries;
		float mDamageMultiplier = 1.f;
		float mMovementMultiplier = 1.f;
	};
}
