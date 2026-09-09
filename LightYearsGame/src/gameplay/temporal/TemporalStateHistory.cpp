#include "gameplay/temporal/TemporalStateHistory.h"

#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>

namespace
{
	float LerpAngleDegrees(float from, float to, float alpha)
	{
		float difference = std::fmod(to - from, 360.f);
		if (difference > 180.f)
		{
			difference -= 360.f;
		}
		else if (difference < -180.f)
		{
			difference += 360.f;
		}
		return from + difference * alpha;
	}
}

namespace ly
{
	void TemporalStateHistory::Reset()
	{
		mSnapshots.clear();
		mElapsedSeconds = 0.f;
	}

	void TemporalStateHistory::CaptureInitial(const SpaceShip& owner)
	{
		Reset();
		Capture(owner);
	}

	void TemporalStateHistory::AdvanceAndCapture(const SpaceShip& owner, float deltaTime)
	{
		mElapsedSeconds += std::max(0.f, deltaTime);
		Capture(owner);
		const float oldestAllowed = mElapsedSeconds - mRetentionSeconds;
		while (mSnapshots.size() > 1 &&
			mSnapshots.front().timestampSeconds < oldestAllowed)
		{
			mSnapshots.pop_front();
		}
	}

	bool TemporalStateHistory::TryGetSnapshotSecondsAgo(
		float secondsAgo,
		TemporalStateSnapshot& outSnapshot
	) const
	{
		if (secondsAgo < 0.f || mSnapshots.empty())
		{
			return false;
		}

		const float targetTime = mElapsedSeconds - secondsAgo;
		if (targetTime < mSnapshots.front().timestampSeconds)
		{
			return false;
		}
		if (targetTime >= mSnapshots.back().timestampSeconds)
		{
			outSnapshot = mSnapshots.back();
			return true;
		}

		for (std::size_t index = 1; index < mSnapshots.size(); ++index)
		{
			const TemporalStateSnapshot& after = mSnapshots[index];
			if (after.timestampSeconds < targetTime)
			{
				continue;
			}

			const TemporalStateSnapshot& before = mSnapshots[index - 1];
			const float interval = after.timestampSeconds - before.timestampSeconds;
			const float alpha = interval > 0.f
				? std::clamp((targetTime - before.timestampSeconds) / interval, 0.f, 1.f)
				: 0.f;
			outSnapshot.timestampSeconds = targetTime;
			outSnapshot.location = before.location + (after.location - before.location) * alpha;
			outSnapshot.velocity = before.velocity + (after.velocity - before.velocity) * alpha;
			outSnapshot.rotationDegrees = LerpAngleDegrees(
				before.rotationDegrees,
				after.rotationDegrees,
				alpha
			);
			outSnapshot.health = before.health + (after.health - before.health) * alpha;
			outSnapshot.shield = before.shield + (after.shield - before.shield) * alpha;
			return true;
		}

		return false;
	}

	void TemporalStateHistory::Capture(const SpaceShip& owner)
	{
		mSnapshots.push_back(TemporalStateSnapshot{
			mElapsedSeconds,
			owner.GetActorLocation(),
			owner.GetVelocity(),
			owner.GetActorRotation(),
			owner.GetHealthComponent().GetHealth(),
			owner.GetShieldComponent().GetShield()
		});
	}
}
