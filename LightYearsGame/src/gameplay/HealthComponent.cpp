#include "gameplay/HealthComponent.h"

namespace ly
{
	HealthComponent::HealthComponent(float health, float maxHealth):
		mHealth{health},        
		mMaxHealth{maxHealth}  
	{

	}

	void HealthComponent::SetInitialHealth(float health, float maxHealth)
	{
		mHealth = health;
		mMaxHealth = maxHealth;
		if(health>maxHealth)
		{
			mHealth = maxHealth;
		}
	}

	void HealthComponent::SetMaxHealth(float maxHealth, bool preserveHealthPercent)
	{
		if (maxHealth <= 0.f || maxHealth == mMaxHealth)
		{
			return;
		}

		const float previousHealth = mHealth;
		const float previousMaxHealth = mMaxHealth;
		mMaxHealth = maxHealth;
		if (preserveHealthPercent && previousMaxHealth > 0.f)
		{
			mHealth = mMaxHealth * (previousHealth / previousMaxHealth);
		}
		if (mHealth > mMaxHealth)
		{
			mHealth = mMaxHealth;
		}

		onHealthChanged.Broadcast(mHealth - previousHealth, mHealth, mMaxHealth);
	}

	void HealthComponent::ChangeHealth(float amount)
	{
		if (amount == 0) return;  
		if (mHealth == 0) return; 
		const float previousHealth = mHealth;
		mHealth += amount; 

		if (mHealth < 0)
		{
			mHealth = 0; 
		}

		if (mHealth > mMaxHealth)
		{
			mHealth = mMaxHealth; 
		}

		const float actualDelta = mHealth - previousHealth;
		onHealthChanged.Broadcast(actualDelta, mHealth, mMaxHealth);

		if (actualDelta < 0)
		{
			TakenDamage(-actualDelta);  
			if (mHealth <= 0)
			{
				HealthEmpty();  
			}
		}

	}

	void HealthComponent::TakenDamage(float amount)
	{
		onTakenDamage.Broadcast(amount, mHealth, mMaxHealth);
	}

	void HealthComponent::HealthRegen(float amount)
	{

	}
	
	void HealthComponent::HealthEmpty()
	{
		onHealthEmpty.Broadcast();
	}
}


