#pragma once

#include "framework/Core.h"
#include "gameplay/damage/DamageContext.h"
#include "gameplay/module/rules/ModuleSideRules.h"

namespace vega
{
	class SpaceShip;
	class ModuleDeferredDamageRuntime
	{
	public:
		explicit ModuleDeferredDamageRuntime(SpaceShip& owner) : mOwner{ owner } {}
		void SetRule(ModuleDeferredDamageRule rule) { mRule = rule; }
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
		ModuleDeferredDamageRule mRule;
		List<Debt> mDebts;
	};
}
