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

		onHealthChanged.Broadcast(amount, mHealth, mMaxHealth);
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
		// Ölüm event'ini tetikle (genelde Destroy() çaðrýlýr)
		onHealthEmpty.Broadcast();
	}
}