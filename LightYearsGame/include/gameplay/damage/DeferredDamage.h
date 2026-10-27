#pragma once

#include "framework/Core.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/damage/DeferredDamageSpec.h"

namespace vega
{
	class SpaceShip;
	class DeferredDamage
	{
	public:
		explicit DeferredDamage(SpaceShip& owner) : mOwner{ owner } {}
		void SetSpec(DeferredDamageSpec spec) { mSpec = spec; }
		void Defer(DamageContext& context);
		void Tick(float seconds);
		float GetPendingDamage() const;

	private:
		struct Debt
		{
			float rate = 0.f;
			float seconds = 0.f;
			DamagePayload payload;
			float armorShred = 0.f;
			std::weak_ptr<Actor> source;
		};
		SpaceShip& mOwner;
		DeferredDamageSpec mSpec;
		List<Debt> mDebts;
	};
}
