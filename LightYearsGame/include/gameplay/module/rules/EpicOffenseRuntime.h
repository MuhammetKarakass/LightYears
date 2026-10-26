#pragma once

#include "attributes/AttributeSystem.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/module/rules/EpicOffenseRules.h"

#include <cstddef>

namespace vega
{
	class Actor;
	class CombatRuntime;
	class LightYearsAbilitySystemComponent;
	// Counts each installed Q/E/F/R ability once per distinct authored damage tag; no type means no count.
	void ResolveMonochromeCoreLoadout(EpicOffenseEffects& effects, const LightYearsAbilitySystemComponent& abilities);

	// Outgoing damage multiplier of one owner hit from Blood Contract (Offense damage scaled by missing health) and Alternating Current
	// (a direct ability hit after a different damage type on the same enemy). Records the hit for later comparison; call once per hit.
	float ComputeEpicOffenseOutgoingMultiplier(EpicOffenseState& state, const EpicOffenseEffects& effects, Actor& owner, Actor& target, const List<GameplayTag>& sourceAbilityTags, const List<GameplayTag>& damageTags, DamageDeliveryType deliveryType, float mainEffectMultiplier = 1.f);

	// Ship-owned runtime for the time-based and ship-level part of Epic Offense: the Blood Contract cost, heal windows and Clot Armor.
	// Counters and marks live in the ship's CombatRuntime; this class only owns the Clot Armor modifier handle.
	class ModuleEpicOffenseRuntime final
	{
	public:
		ModuleEpicOffenseRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime);
		~ModuleEpicOffenseRuntime();
		ModuleEpicOffenseRuntime(const ModuleEpicOffenseRuntime&) = delete;
		ModuleEpicOffenseRuntime& operator=(const ModuleEpicOffenseRuntime&) = delete;

		// Drops counters and grants whose rule is no longer installed. Call after the effects were pushed to the CombatRuntime.
		void Replace();
		// One loadout ability use with its tags: charges Storm Capacitor, advances the Prism Core cycle, spends Blood Contract health.
		void OnOwnerAbilityUse(const List<GameplayTag>& tags);
		void Tick(float deltaSeconds);
		// Removes the Clot Armor grant and clears the counters. Idempotent.
		void Shutdown();

		bool HasClotArmor() const { return mClotHandle.IsValid(); }

	private:
		void RemoveClotArmor();

		LightYearsAbilitySystemComponent& mAbilities;
		CombatRuntime& mCombatRuntime;
		sas::AttributeModifierHandle mClotHandle;
		float mClotRemainingSeconds = 0.f;
		bool mShuttingDown = false;
	};
}
