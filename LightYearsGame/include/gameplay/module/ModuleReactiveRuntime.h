#pragma once

#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "abilities/AbilityEvent.h"
#include "framework/Delegate.h"
#include "gameplay/control/ControlResponse.h"
#include "gameplay/progression/RunGold.h"
#include <functional>
#include <optional>
#include <random>

namespace vega
{
	class SpaceShip;
	class LightYearsAbilitySystemComponent;
	// Ship-owned RAII receiver. Event snapshots contain values, never actor identities.
	class ModuleReactiveRuntime final
	{
	public:
		explicit ModuleReactiveRuntime(LightYearsAbilitySystemComponent& abilities);
		~ModuleReactiveRuntime();
		ModuleReactiveRuntime(const ModuleReactiveRuntime&) = delete;
		ModuleReactiveRuntime& operator=(const ModuleReactiveRuntime&) = delete;
		void Replace(const List<ResolvedModuleEventAction>& actions, const List<ResolvedModuleOwnerAttributeRule>& attributes, const List<ResolvedModuleEffects::MovementFade>& fades = {});
		void SetGoldWallet(std::weak_ptr<RunGold> wallet) { mGoldWallet = std::move(wallet); }
		// Called for each kill the owner confirms with a loadout ability (never for module-generated damage).
		void SetAbilityUseObserver(std::function<void(const List<GameplayTag>&)> observer) { mAbilityUseObserver = std::move(observer); }
		void SetKillObserver(std::function<void(sas::AbilitySlot, ControlTargetClass)> observer) { mKillObserver = std::move(observer); }
		void Tick(float deltaSeconds);
		// Spare Capacitor trickle rules (side feature). Replace keeps the running interval of an unchanged rule; a removed rule drops its state.
		void ReplaceCharge(const List<ResolvedModuleChargeRule>& rules);
		void ReplaceConditionalMovement(const List<ResolvedModuleEffects::ConditionalMovement>& rules);
		// Luck rolls default to a random seed; tests and replays can pin one.
		void SetRandomSeed(std::uint32_t seed) { mRandom.seed(seed); }
		std::size_t GetOwnedHandleCount() const;
		// Sonic state (Escape Velocity). Replace keeps the state of unchanged rules; a removed rule ends its state at once.
		void ReplaceSustainedSpeed(const List<ResolvedModuleEffects::SustainedSpeedRule>& rules);
		std::size_t GetOwnedSustainedHandleCount() const;
		// Smoldering Remains: a Thermal field at origin lives rule.smolderSeconds and pulses rule.smolderStacksPerSecond stacks per second
		// onto combatants within rule.smolderRadius. Replace and destruction cancel every field through its lease.
		void SpawnSmolderingField(const sf::Vector2f& origin, const ModuleThermalBurstRule& rule);
		std::size_t GetOwnedSmolderFieldCount() const;

	private:
		struct ActionState { ResolvedModuleEventAction resolved; int count = 0; float cooldownRemaining = 0.f; float healingRemaining = 0.f; float healingRate = 0.f; };
		struct AttributeState
		{
			ResolvedModuleOwnerAttributeRule resolved;
			sas::AttributeModifierHandle handle;
			sas::AttributeId appliedAttribute;
			float appliedAmount = 0.f;
			float fadeStart = 0.f;
			float fadeElapsed = 0.f;
			float previousDesired = 0.f;
		};
		struct Event { ModuleTriggerKind trigger; sas::ContentId abilityId; };
		struct DesiredRules { List<ResolvedModuleEventAction> actions; List<ResolvedModuleOwnerAttributeRule> attributes; List<ResolvedModuleEffects::MovementFade> fades; };
		void OnEvent(const sas::AbilityEvent& event);
		void NotifyOwnerKill(const sas::AbilityEvent& event);
		void OnHealth(float amount, float health, float maximum);
		void OnShield(float amount, float current, float maximum);
		void UpdateConditionalMovement();
		List<ResolvedModuleEffects::ConditionalMovement> mConditionalMovement;
		void OnAttribute(sas::AttributeId id, float previous, float current);
		void Flush(float deltaSeconds = 0.f);
		void Reconcile(DesiredRules desired);
		void UpdateAttributes(float deltaSeconds);
		void Execute(const Event& event);
		void Remove(AttributeState& state) noexcept;
		LightYearsAbilitySystemComponent& mAbilities;
		SpaceShip* mShip = nullptr; // Same owner's component lifetime; no registry identity.
		DelegateHandle mEventHandle;
		DelegateHandle mHealthHandle;
		DelegateHandle mShieldHandle;
		DelegateHandle mAttributeHandle;
		List<ActionState> mActions;
		List<AttributeState> mAttributes;
		List<ResolvedModuleEffects::MovementFade> mFades;
		struct SustainedSpeedState
		{
			ResolvedModuleEffects::SustainedSpeedRule resolved;
			float fastSeconds = 0.f;
			float lingerSeconds = 0.f;
			bool lingering = false;
			bool sonic = false;
			bool damageRegistered = false;
			bool turnRegistered = false;
			List<std::size_t> primaryHandles;
		};
		void TickSustainedSpeed(float deltaSeconds);
		struct ChargeTrickleState
		{
			ResolvedModuleChargeRule resolved;
			float accumulator = 0.f;
		};
		void TickCharge(float deltaSeconds);
		void GrantTrickle(const ResolvedModuleChargeRule& rule);
		List<ChargeTrickleState> mChargeTrickles;
		void SetSonic(SustainedSpeedState& state, bool active);
		void SetSonicTurn(SustainedSpeedState& state, bool active);
		void SetSonicDamage(SustainedSpeedState& state, bool active);
		void ClearSustained(SustainedSpeedState& state) noexcept;
		List<SustainedSpeedState> mSustainedSpeed;
		List<std::shared_ptr<ModuleGeneratedLease>> mSmolderLeases;
		std::uint64_t mNextLeaseKey = 1;
		void PruneSmolderFields();
		void CancelSmolderFields() noexcept;
		std::weak_ptr<RunGold> mGoldWallet;
		std::function<void(sas::AbilitySlot, ControlTargetClass)> mKillObserver;
		std::function<void(const List<GameplayTag>&)> mAbilityUseObserver;
		List<Event> mEvents;
		std::optional<DesiredRules> mPending;
		std::mt19937 mRandom{ std::random_device{}() };
		bool mBusy = false;
		bool mDirty = false;
		bool mShuttingDown = false;
	};
}
