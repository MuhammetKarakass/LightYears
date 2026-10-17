#pragma once

#include <framework/ActorComponent.h>
#include <framework/Core.h>
#include <framework/Delegate.h>
#include "gameplay/resource/TemporaryOvercapLedger.h"

#include <string>

namespace vega
{
	// Not ticked automatically (mCanEverTick stays false): the owning actor drives
	// TickTemporaryOverhealths from its own regeneration step so the per-frame
	// order is unchanged (UE tick-prerequisite equivalent).
	class HealthComponent : public ActorComponent
	{
		VEGA_CLASS(HealthComponent)
	public:
		HealthComponent(Actor& owner, float health, float maxHealth);

		float GetHealth() const { return mHealth; };
		float GetMaxHealth() const { return mMaxHealth; };
		void SetInitialHealth(float health, float maxHealth);
		void SetMaxHealth(float maxHealth, bool preserveHealthPercent = false, bool preserveCurrentValue = false);

		void ChangeHealth(float amount);
		void Regenerate(float amount);
		// Grants health that may exceed the normal maximum. Only the excess is
		// recorded; ordinary healing preserves it and damage consumes it first.
		float GrantTemporaryOverhealth(
			const std::string& sourceId,
			float amount,
			float holdDuration,
			float decayPerSecond
		);
		void TickTemporaryOverhealths(float deltaTime);

		Delegate<float,float,float> onHealthChanged;
		Delegate<float, float, float> onTakenDamage;
		Delegate<> onHealthEmpty;

	private:

		void TakenDamage(float amount);
		void HealthEmpty();

		float mHealth;
		float mMaxHealth;
		TemporaryOvercapLedger mTemporaryOverhealths;
	};
}


