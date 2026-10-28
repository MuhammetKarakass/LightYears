#pragma once

#include "framework/Core.h"
#include "framework/Delegate.h"
#include "gameplay/damage/DamageContext.h"

namespace vega
{
	class Actor;
	// RAII subscription to resolved incoming damage. Counts actual hull damage plus
	// the source-damage amount spent on shield, after reductions; barriers add nothing.
	class IncomingDamageWindow final
	{
	public:
		explicit IncomingDamageWindow(Actor& owner);
		~IncomingDamageWindow();
		IncomingDamageWindow(const IncomingDamageWindow&) = delete;
		IncomingDamageWindow& operator=(const IncomingDamageWindow&) = delete;
		float GetDamage() const { return mDamage; }
	private:
		void OnDamage(const DamageContext& context);
		weak_ptr<Actor> mOwner;
		DelegateHandle mHandle;
		float mDamage = 0.f;
	};
}
