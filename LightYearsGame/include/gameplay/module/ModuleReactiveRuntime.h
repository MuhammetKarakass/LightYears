#pragma once

#include "gameplay/module/ModuleResolver.h"
#include "abilities/AbilityEvent.h"
#include "framework/Delegate.h"
#include "gameplay/progression/RunGold.h"
#include <optional>
#include <random>

namespace ly
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
		void Tick(float deltaSeconds);
		std::size_t GetOwnedHandleCount() const;

	private:
		struct ActionState { ResolvedModuleEventAction resolved; int count = 0; float cooldownRemaining = 0.f; };
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
		void OnHealth(float amount, float health, float maximum);
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
		DelegateHandle mAttributeHandle;
		List<ActionState> mActions;
		List<AttributeState> mAttributes;
		List<ResolvedModuleEffects::MovementFade> mFades;
		std::weak_ptr<RunGold> mGoldWallet;
		List<Event> mEvents;
		std::optional<DesiredRules> mPending;
		std::mt19937 mRandom{ std::random_device{}() };
		bool mBusy = false;
		bool mDirty = false;
		bool mShuttingDown = false;
	};
}
