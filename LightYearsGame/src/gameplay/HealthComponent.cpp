#include "gameplay/HealthComponent.h"

namespace ly
{
	HealthComponent::HealthComponent(float health, float maxHealth):
		mHealth{health},
		mMaxHealth{maxHealth}
	{

	}

	void HealthComponent::ChangeHealth(float amount)
	{
		if (amount == 0) return;
		if (mHealth == 0) return;

		mHealth += amount;

		if (mHealth < 0)
		{
			mHealth = 0;
		}

		if (mHealth > mMaxHealth)
		{
			mHealth = mMaxHealth;
		}

		if (amount < 0)
		{
			TakenDamage(-amount);
			if (mHealth <= 0)
			{
				HealthEmpty();
			}
		}

		if (amount > 0)
		{
			HealthRegen(amount);
		}
	}
	void HealthComponent::TakenDamage(float amount)
	{
		LOG("Damage Taken:%f   Health: %f/&f", amount, mHealth, mMaxHealth);
	}
	void HealthComponent::HealthRegen(float amount)
	{
		LOG("Health Regen:%f   Health: %f/&f", amount, mHealth, mMaxHealth);

	}
	void HealthComponent::HealthEmpty()
	{
		LOG("Health Empty");
	}
}