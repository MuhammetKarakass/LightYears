#include "gameplay/combat/IncomingDamageWindow.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "framework/Actor.h"
#include <algorithm>

namespace vega
{
	IncomingDamageWindow::IncomingDamageWindow(Actor& owner) : mOwner{ owner.GetWeakActor() }
	{
		// An unmanaged owner cannot support a lifetime-safe RAII subscription.
		if (mOwner.expired()) return;
		if (auto* combatant = dynamic_cast<Combatant*>(&owner)) mHandle = combatant->GetCombatRuntime().onDamageResolved.BindAction(this, &IncomingDamageWindow::OnDamage);
	}
	IncomingDamageWindow::~IncomingDamageWindow()
	{
		if (const auto owner = mOwner.lock())
			if (auto* combatant = dynamic_cast<Combatant*>(owner.get())) combatant->GetCombatRuntime().onDamageResolved.UnbindAction(mHandle);
	}
	void IncomingDamageWindow::OnDamage(const DamageContext& context)
	{
		mDamage += std::max(0.f, context.appliedDamage);
	}
}
