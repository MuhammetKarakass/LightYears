#include "gameplay/ability/aegisReaver/AegisReaverFlightState.h"

#include "framework/Actor.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <atomic>
#include <memory>

namespace ly
{
	namespace
	{
		std::atomic_uint64_t NextFlightId{ 1 };

		weak_ptr<SpaceShip> MakeWeakShip(SpaceShip& ship)
		{
			const shared_ptr<Object> object = ship.GetWeakPtr().lock();
			return object
				? std::dynamic_pointer_cast<SpaceShip>(object)
				: weak_ptr<SpaceShip>{};
		}
	}

	AegisReaverFlightState::AegisReaverFlightState(SpaceShip& owner)
		: mOwner{ MakeWeakShip(owner) },
		  mUnmanagedOwner{ mOwner.expired() ? &owner : nullptr },
		  mRegenLockSourceId{ "AegisReaver.Flight." +
			std::to_string(NextFlightId.fetch_add(1)) }
	{
		owner.GetShieldComponent().SetPassiveRegenBlocked(mRegenLockSourceId, true);
	}

	AegisReaverFlightState::~AegisReaverFlightState()
	{
		ReleaseRegenLock();
	}

	void AegisReaverFlightState::RegisterProjectile()
	{
		if (!mCompleted)
		{
			++mActiveProjectileCount;
		}
	}

	void AegisReaverFlightState::ReportProjectileReturned(float shieldPayload)
	{
		if (mCompleted)
		{
			return;
		}
		mReturnedShield += std::max(0.f, shieldPayload);
		mActiveProjectileCount = std::max(0, mActiveProjectileCount - 1);
		CompleteIfFinished();
	}

	void AegisReaverFlightState::ReportProjectileLost()
	{
		if (mCompleted)
		{
			return;
		}
		mActiveProjectileCount = std::max(0, mActiveProjectileCount - 1);
		CompleteIfFinished();
	}

	bool AegisReaverFlightState::TryRegisterTargetHit(
		Actor& target,
		bool returning
	)
	{
		std::unordered_set<std::uint64_t>& hitTargets = returning
			? mReturnTargets
			: mOutboundTargets;
		return hitTargets.insert(target.GetUniqueID()).second;
	}

	SpaceShip* AegisReaverFlightState::ResolveOwner() const
	{
		const shared_ptr<SpaceShip> owner = mOwner.lock();
		return owner ? owner.get() : mUnmanagedOwner;
	}

	void AegisReaverFlightState::CompleteIfFinished()
	{
		if (mCompleted || mActiveProjectileCount > 0)
		{
			return;
		}
		mCompleted = true;

		if (SpaceShip* owner = ResolveOwner(); owner && !owner->GetIsPendingDestroy())
		{
			// Aegis returns an actual resource payload rather than normal recharge.
			// A zero hold/decay entry means the overshield remains until damage uses it.
			owner->GetShieldComponent().GrantTemporaryOvershield(
				mRegenLockSourceId,
				mReturnedShield,
				0.f,
				0.f
			);
			owner->GetShieldComponent().ClearRechargeDelay();
		}
		ReleaseRegenLock();
	}

	void AegisReaverFlightState::ReleaseRegenLock()
	{
		if (mRegenLockReleased)
		{
			return;
		}
		if (SpaceShip* owner = ResolveOwner())
		{
			owner->GetShieldComponent().SetPassiveRegenBlocked(mRegenLockSourceId, false);
		}
		mRegenLockReleased = true;
	}
}
