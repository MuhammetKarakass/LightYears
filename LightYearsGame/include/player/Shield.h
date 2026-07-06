#pragma once
#include <framework/Delegate.h>

namespace ly
{
	class Shield
	{
	public:
		Shield(float amount = 0.f);
		void Activate(float amount);
		void Deactivate();
		float TakeDamage(float amt); // Returns the remaining damage that penetrates the shield
		float GetHealth() const { return mHealth; }
		float GetMaxHealth() const { return mMaxHealth; }
		bool IsActive() const { return mIsActive; }

		Delegate<bool> onShieldStateChanged;
	private:
		float mHealth;
		float mMaxHealth;
		bool mIsActive;
	};
}
