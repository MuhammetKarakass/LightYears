#pragma once

#include "abilities/AbilityEvent.h"
#include "framework/Delegate.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/ship/ShipRuntime.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class CombatRuntime;
	class ShieldComponent;

	class ModuleTriggerRuntime final
	{
	public:
		ModuleTriggerRuntime(LightYearsAbilitySystemComponent& abilities, ShipRuntime& shipRuntime, ShipRuntimeModifiers& runtimeModifiers, CombatRuntime& combatRuntime, ShieldComponent& shield);
		~ModuleTriggerRuntime();
		ModuleTriggerRuntime(const ModuleTriggerRuntime&) = delete;
		ModuleTriggerRuntime& operator=(const ModuleTriggerRuntime&) = delete;

		// The returned generation identifies this rule set; ClearRules ignores stale owners.
		void SetRules(const List<ResolvedModuleTriggerBuff>& rules, std::uint64_t& generation);
		void ClearRules(std::uint64_t generation);
		void Tick(float deltaSeconds);
		bool Force(std::uint32_t instanceId, float powerMultiplier);

		std::size_t GetActiveBuffCount() const { return mActiveBuffs.size(); }
		std::size_t GetActiveModifierHandleCount() const;

	private:
		struct EventSnapshot
		{
			ModuleTriggerKind trigger = ModuleTriggerKind::None;
			float magnitude = 0.f;
			List<GameplayTag> sourceAbilityTags;
			List<GameplayTag> payloadTags;
			bool moduleGenerated = false;
			bool suppressModuleTriggers = false;
		};

		struct OwnedAttributeModifier
		{
			ModuleAttributeTarget target = ModuleAttributeTarget::Owner;
			sas::AttributeModifierHandle attributeHandle;
			std::size_t primaryWeaponHandle = 0;
		};

		struct ActiveBuff
		{
			std::uint32_t instanceId = 0;
			std::size_t ruleIndex = 0;
			ModuleTriggerBuff rule;
			float remainingSeconds = 0.f;
			float powerMultiplier = 1.f;
			std::string movementSourceId;
			bool ownsMovementModifier = false;
			List<OwnedAttributeModifier> modifiers;
		};

		void OnGameplayEvent(const sas::AbilityEvent& event);
		void OnShieldDamaged(float absorbed, float remainingShield, float maxShield);
		void OnShieldChanged(float delta, float remainingShield, float maxShield);
		void ProcessEvent(const EventSnapshot& event);
		bool Fire(const ResolvedModuleTriggerBuff& resolved, float powerMultiplier, bool natural);
		void ProcessDeferredUpdates();
		void ReconcileRules(const List<ResolvedModuleTriggerBuff>& rules);
		void RemoveActiveBuff(std::size_t index);
		void RemoveBuffEffects(ActiveBuff& buff);
		ActiveBuff CreateActiveBuff(const ResolvedModuleTriggerBuff& resolved);
		bool MatchesTrigger(const ModuleTriggerBuff& rule, const EventSnapshot& event) const;
		static bool SameKey(const ActiveBuff& active, const ResolvedModuleTriggerBuff& resolved);
		static std::string MakeMovementSourceId(std::uint32_t instanceId, std::size_t ruleIndex);

		LightYearsAbilitySystemComponent& mAbilities;
		ShipRuntime& mShipRuntime;
		ShipRuntimeModifiers& mRuntimeModifiers;
		CombatRuntime& mCombatRuntime;
		ShieldComponent& mShield;
		DelegateHandle mGameplayEventHandle;
		DelegateHandle mShieldDamagedHandle;
		DelegateHandle mShieldChangedHandle;
		// Set by a ShieldBroken event while the shield's re-arm fraction is active; cleared on refill.
		bool mShieldBreakLatched = false;
		List<ResolvedModuleTriggerBuff> mRules;
		List<ActiveBuff> mActiveBuffs;
		std::optional<List<ResolvedModuleTriggerBuff>> mPendingRules;
		List<EventSnapshot> mPendingEvents;
		std::size_t mDispatchDepth = 0;
		std::uint64_t mNextRulesGeneration = 1;
		std::uint64_t mRulesGeneration = 0;
		bool mShuttingDown = false;
	};
}
