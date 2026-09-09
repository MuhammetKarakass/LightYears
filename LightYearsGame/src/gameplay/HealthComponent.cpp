#include "gameplay/HealthComponent.h"

#include <algorithm>

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
		mTemporaryOverhealths.Clear();
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
		mTemporaryOverhealths.Reconcile(mHealth, mMaxHealth);

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

		if (amount > 0.f && mHealth > mMaxHealth)
		{
			mHealth = mMaxHealth; 
		}

		const float actualDelta = mHealth - previousHealth;
		onHealthChanged.Broadcast(actualDelta, mHealth, mMaxHealth);

		if (actualDelta < 0)
		{
			const float previousExcess = std::max(0.f, previousHealth - mMaxHealth);
			const float currentExcess = std::max(0.f, mHealth - mMaxHealth);
			mTemporaryOverhealths.Consume(
				std::max(0.f, previousExcess - currentExcess)
			);
			TakenDamage(-actualDelta);  
			if (mHealth <= 0)
			{
				HealthEmpty();  
			}
		}

	}

	void HealthComponent::Regenerate(float amount)
	{
		if (amount > 0.f)
		{
			ChangeHealth(amount);
		}
	}

	float HealthComponent::GrantTemporaryOverhealth(
		const std::string& sourceId,
		float amount,
		float holdDuration,
		float decayPerSecond
	)
	{
		const float safeAmount = std::max(0.f, amount);
		if (safeAmount <= 0.f || mHealth <= 0.f)
		{
			return 0.f;
		}

		const float previousHealth = mHealth;
		const float previousExcess = std::max(0.f, previousHealth - mMaxHealth);
		mHealth += safeAmount;
		const float newExcess = std::max(0.f, mHealth - mMaxHealth);
		mTemporaryOverhealths.Add(TemporaryOvercapRequest{
			sourceId,
			std::max(0.f, newExcess - previousExcess),
			holdDuration,
			decayPerSecond
		});
		onHealthChanged.Broadcast(mHealth - previousHealth, mHealth, mMaxHealth);
		return safeAmount;
	}

	void HealthComponent::TickTemporaryOverhealths(float deltaTime)
	{
		const float decay = mTemporaryOverhealths.Tick(deltaTime, mHealth, mMaxHealth);
		if (decay <= 0.f)
		{
			return;
		}

		const float previousHealth = mHealth;
		mHealth = std::max(mMaxHealth, mHealth - decay);
		onHealthChanged.Broadcast(mHealth - previousHealth, mHealth, mMaxHealth);
	}

	void HealthComponent::TakenDamage(float amount)
	{
		onTakenDamage.Broadcast(amount, mHealth, mMaxHealth);
	}

	void HealthComponent::HealthEmpty()
	{
		onHealthEmpty.Broadcast();
	}
}


