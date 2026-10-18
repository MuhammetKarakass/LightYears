#pragma once

namespace vega
{
	// Game-owned damage contract. The engine no longer exposes Actor::ApplyDamage,
	// so actors that can absorb a raw damage amount implement this interface and
	// callers that only hold a base Actor* test for it before applying damage.
	class IDamageable
	{
	public:
		virtual ~IDamageable() = default;
		virtual void ApplyDamage(float amount) = 0;
	};
}
