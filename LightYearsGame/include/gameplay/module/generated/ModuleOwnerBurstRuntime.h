#pragma once
#include "abilities/AbilityEvent.h"
#include "gameplay/module/ModuleResolver.h"
#include "gameplay/module/generated/ModuleGeneratedLease.h"
#include "framework/Delegate.h"
#include <SFML/System/Vector2.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace vega
{
	class LightYearsAbilitySystemComponent;
	class SpaceShip;
	class Actor;

	// Owner-centred bursts. The burst centre is the owner, not the target. A ShieldBroken rule fires on a shield break; a
	// KillConfirmed rule fires after chargeKills owner kills within chargeRadius, and its charges are per rule and per owner,
	// not saved. Each rule owns a lease so removing it cancels its pending waves, and its cooldown and charges survive a
	// reapply while the placement and rule are unchanged.
	class ModuleOwnerBurstRuntime final
	{
	public:
		explicit ModuleOwnerBurstRuntime(LightYearsAbilitySystemComponent& abilities);
		~ModuleOwnerBurstRuntime();
		ModuleOwnerBurstRuntime(const ModuleOwnerBurstRuntime&) = delete;
		ModuleOwnerBurstRuntime& operator=(const ModuleOwnerBurstRuntime&) = delete;
		void Replace(const List<ResolvedModuleOwnerBurstRule>& rules);
		void Tick(float deltaSeconds);
		std::size_t GetRuleCount() const;
		// Waves still waiting for their delay: echoes and the detonation that follows a pull.
		std::size_t GetPendingEchoCount() const;
		// One owner-centred push with no damage, stun or lease (Sonic Boom). Every opposing combatant within radius of the owner
		// is pushed away with impulse, through the same push as a burst wave. It creates no damage and no gameplay event.
		static void PushOwnerCentred(SpaceShip& owner, float radius, float impulse);

	private:
		struct State
		{
			std::uint32_t instanceId = 0;
			ModuleOwnerBurstRule rule;
			std::shared_ptr<ModuleGeneratedLease> lease;
			float cooldownRemaining = 0.f;
			int charges = 0;
			bool detonating = false;
			float accumulatedArmor = 0.f;
		};
		struct DelayedWave
		{
			std::shared_ptr<ModuleGeneratedLease> lease;
			ModuleOwnerBurstRule rule;
			float damage = 0.f;
			float pushScale = 1.f;
			sf::Vector2f origin{};
			float remainingSeconds = 0.f;
		};
		void OnShieldDamaged(float absorbed, float remainingShield, float maxShield);
		void OnGameplayEvent(const sas::AbilityEvent& event);
		void Flush();
		void BurstShield();
		void ArmCharged();
		void Detonate(const ModuleOwnerBurstRule& rule, const std::shared_ptr<ModuleGeneratedLease>& lease, float accumulatedArmor = 0.f);
		void Pull(const ModuleOwnerBurstRule& rule, const std::shared_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& origin);
		void Wave(const ModuleOwnerBurstRule& rule, const std::shared_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& origin, float damage, float pushScale);
		void ScheduleWave(const ModuleOwnerBurstRule& rule, const std::shared_ptr<ModuleGeneratedLease>& lease, const sf::Vector2f& origin, float damage, float pushScale, float seconds);
		void Stun(Actor& target, float seconds);
		void Reconcile(const List<ResolvedModuleOwnerBurstRule>& rules);

		LightYearsAbilitySystemComponent& mAbilities;
		SpaceShip* mShip = nullptr;
		DelegateHandle mShieldHandle;
		DelegateHandle mEventHandle;
		List<State> mStates;
		List<DelayedWave> mDelayedWaves;
		int mPendingBreaks = 0;
		bool mBusy = false;
		bool mShuttingDown = false;
	};
}
