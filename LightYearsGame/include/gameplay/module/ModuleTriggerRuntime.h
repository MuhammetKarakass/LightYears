#pragma once

#include "abilities/AbilityEvent.h"
#include "framework/Delegate.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/ship/ShipRuntime.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"

#include <cstddef>
#include <optional>

namespace ly
{
	class LightYearsAbilitySystemComponent;

	class ModuleTriggerRuntime final
	{
	public:
		ModuleTriggerRuntime(
			LightYearsAbilitySystemComponent& abilities,
			ShipRuntime& shipRuntime,
			ShipRuntimeModifiers& runtimeModifiers
		);
		~ModuleTriggerRuntime();
		ModuleTriggerRuntime(const ModuleTriggerRuntime&) = delete;
		ModuleTriggerRuntime& operator=(const ModuleTriggerRuntime&) = delete;

		void SetRules(const List<ResolvedModuleTriggerBuff>& rules);
		void Tick(float deltaSeconds);

		std::size_t GetActiveBuffCount() const { return mActiveBuffs.size(); }
		std::size_t GetActiveModifierHandleCount() const;

	private:
		struct EventSnapshot
		{
			GameplayTag eventTag;
			float magnitude = 0.f;
			List<GameplayTag> sourceAbilityTags;
			List<GameplayTag> payloadTags;
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
			std::string movementSourceId;
			bool ownsMovementModifier = false;
			List<OwnedAttributeModifier> modifiers;
		};

		void OnGameplayEvent(const sas::AbilityEvent& event);
		void ProcessEvent(const EventSnapshot& event);
		void ProcessDeferredUpdates();
		void ReconcileRules(const List<ResolvedModuleTriggerBuff>& rules);
		void RemoveActiveBuff(std::size_t index);
		void RemoveBuffEffects(ActiveBuff& buff);
		ActiveBuff CreateActiveBuff(const ResolvedModuleTriggerBuff& resolved);
		bool MatchesTrigger(const ModuleTriggerBuff& rule, const EventSnapshot& event) const;
		static bool SameKey(const ActiveBuff& active, const ResolvedModuleTriggerBuff& resolved);
		static bool SameRule(const ModuleTriggerBuff& left, const ModuleTriggerBuff& right);
		static std::string MakeMovementSourceId(std::uint32_t instanceId, std::size_t ruleIndex);

		LightYearsAbilitySystemComponent& mAbilities;
		ShipRuntime& mShipRuntime;
		ShipRuntimeModifiers& mRuntimeModifiers;
		DelegateHandle mGameplayEventHandle;
		List<ResolvedModuleTriggerBuff> mRules;
		List<ActiveBuff> mActiveBuffs;
		std::optional<List<ResolvedModuleTriggerBuff>> mPendingRules;
		List<EventSnapshot> mPendingEvents;
		std::size_t mDispatchDepth = 0;
		bool mShuttingDown = false;
	};
}
