#pragma once

#include "framework/Core.h"

#include <cstdint>
#include <string>
#include <unordered_set>

namespace ly
{
	class Actor;
	class SpaceShip;

	// One cast owns one flight ledger. It outlives the source projectile when a
	// Relay Prism replaces it with several clones, then returns the aggregate
	// shield exactly once when the final live frisbee reaches its original owner.
	class AegisReaverFlightState final
	{
	public:
		explicit AegisReaverFlightState(SpaceShip& owner);
		~AegisReaverFlightState();

		void RegisterProjectile();
		void ReportProjectileReturned(float shieldPayload);
		void ReportProjectileLost();
		SpaceShip* GetOwner() const { return ResolveOwner(); }

		// The original design permits one hit per target in each leg. The registry
		// is flight-wide rather than projectile-local so Relay clones cannot turn
		// one allowed hit into four duplicate shield steals.
		bool TryRegisterTargetHit(Actor& target, bool returning);

	private:
		SpaceShip* ResolveOwner() const;
		void CompleteIfFinished();
		void ReleaseRegenLock();

		weak_ptr<SpaceShip> mOwner;
		SpaceShip* mUnmanagedOwner = nullptr;
		std::string mRegenLockSourceId;
		std::unordered_set<std::uint64_t> mOutboundTargets;
		std::unordered_set<std::uint64_t> mReturnTargets;
		float mReturnedShield = 0.f;
		int mActiveProjectileCount = 0;
		bool mCompleted = false;
		bool mRegenLockReleased = false;
	};
}
