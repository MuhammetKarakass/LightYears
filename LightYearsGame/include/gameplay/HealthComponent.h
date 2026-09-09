#pragma once

#include <framework/Core.h>
#include <framework/Delegate.h>
#include "gameplay/resource/TemporaryOvercapLedger.h"

#include <string>

namespace ly
{
	class HealthComponent
	{
	public:
		HealthComponent(float health, float maxHealth);

		float GetHealth() const { return mHealth; };
		float GetMaxHealth() const { return mMaxHealth; };
		void SetInitialHealth(float health, float maxHealth);
		void SetMaxHealth(float maxHealth, bool preserveHealthPercent = false);

		void ChangeHealth(float amount);
		void Regenerate(float amount);
		// Grants health that may exceed the normal maximum. Only the excess is
		// recorded, so ordinary healing and damage keep their existing semantics.
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


