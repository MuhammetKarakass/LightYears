#pragma once

#include <framework/Core.h>
#include <framework/Delegate.h>

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

		Delegate<float,float,float> onHealthChanged;
		Delegate<float, float, float> onTakenDamage;
		Delegate<> onHealthEmpty;

	private:

		void TakenDamage(float amount);
		void HealthEmpty();

		float mHealth;
		float mMaxHealth;
	};
}


