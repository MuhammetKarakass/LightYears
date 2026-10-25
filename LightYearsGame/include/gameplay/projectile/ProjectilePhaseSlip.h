#pragma once

namespace vega
{
	class Actor;
	// Call only at a physical projectile contact, before reflection, damage or consumption.
	bool TryPassPhaseSlipContact(Actor& projectile, Actor* target);
}
