#include "player/Shield.h"

namespace ly
{
	Shield::Shield(float amount) : mHealth{amount}, mMaxHealth{amount}, mIsActive{amount > 0.f}
	{
	}

	void Shield::Activate(float amount)
	{
		mHealth = amount;
		mMaxHealth = amount;
		mIsActive = true;
		onShieldStateChanged.Broadcast(true);
	}

	void Shield::Deactivate()
	{
		if (!mIsActive) return;
		mHealth = 0.f;
		mMaxHealth = 0.f;
		mIsActive = false;
		onShieldStateChanged.Broadcast(false);
	}

	float Shield::TakeDamage(float amt)
	{
		if (!mIsActive) return amt;

		if (mHealth >= amt)
		{
			mHealth -= amt;
			if (mHealth <= 0.f)
			{
				Deactivate();
			}
			return 0.f;
		}
		else
		{
			float remaining = amt - mHealth;
			Deactivate();
			return remaining;
		}
	}
}
