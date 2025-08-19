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

		void ChangeHealth(float amount);

		Delegate<float,float,float> onHealthChanged;

	private:

		void TakenDamage(float amount);
		void HealthRegen(float amount);
		void HealthEmpty();

		float mHealth;
		float mMaxHealth;
	};
}