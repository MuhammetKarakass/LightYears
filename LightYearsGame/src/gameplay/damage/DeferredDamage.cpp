#include "gameplay/damage/DeferredDamage.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>

namespace vega
{
	void DeferredDamage::Defer(DamageContext& context)
	{
		if (context.deferredDamage || mSpec.fraction <= 0.f || mSpec.seconds <= 0.f || context.remainingDamage <= 0.f) return;
		const float amount = context.remainingDamage * mSpec.fraction;
		Debt debt;
		debt.rate = amount / mSpec.seconds;
		debt.seconds = mSpec.seconds;
		debt.payload = context.payload;
		debt.payload.hitGroup.reset();
		debt.payload.moduleTriggerPolicy = DamageModuleTriggerPolicy::Suppressed;
		debt.payload.skipModuleDamageRules = true;
		debt.armorShred = context.targetArmorShred;
		if (context.source) debt.source = context.source->GetWeakActor();
		mDebts.push_back(std::move(debt));
		context.remainingDamage -= amount;
	}

	void DeferredDamage::Tick(float seconds)
	{
		if (seconds <= 0.f || mDebts.empty()) return;
		// Take the due work out of the container before damage callbacks can change the spec.
		List<Debt> due;
		due.swap(mDebts);
		for (auto& debt : due)
		{
			if (mOwner.GetIsPendingDestroy() || mOwner.GetHealthComponent().GetHealth() <= 0.f) break;
			const float elapsed = std::min(seconds, debt.seconds);
			DamageContext context;
			const auto source = debt.source.lock();
			context.source = source && !source->GetIsPendingDestroy() ? source.get() : nullptr;
			context.target = &mOwner;
			context.originalDamage = context.remainingDamage = debt.rate * elapsed;
			context.targetArmorShred = debt.armorShred;
			context.payload = debt.payload;
			context.deferredDamage = true;
			context.deliveryType = DamageDeliveryType::Periodic;
			mOwner.ReceiveDamage(context);
			debt.seconds -= elapsed;
			if (debt.seconds > 0.f) mDebts.push_back(std::move(debt));
		}
	}

	float DeferredDamage::GetPendingDamage() const
	{
		float amount = 0.f;
		for (const auto& debt : mDebts) amount += debt.rate * debt.seconds;
		return amount;
	}
}
