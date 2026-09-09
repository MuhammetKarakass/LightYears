#pragma once

#include <SFML/System/Vector2.hpp>

#include <deque>

namespace ly
{
	class SpaceShip;

	// A single synchronized sample. Temporal abilities read one sample rather
	// than independently querying position and resources at different times.
	struct TemporalStateSnapshot
	{
		float timestampSeconds = 0.f;
		sf::Vector2f location{};
		sf::Vector2f velocity{};
		float rotationDegrees = 0.f;
		float health = 0.f;
		float shield = 0.f;
	};

	class TemporalStateHistory
	{
	public:
		void Reset();
		void CaptureInitial(const SpaceShip& owner);
		void AdvanceAndCapture(const SpaceShip& owner, float deltaTime);
		bool TryGetSnapshotSecondsAgo(
			float secondsAgo,
			TemporalStateSnapshot& outSnapshot
		) const;

	private:
		void Capture(const SpaceShip& owner);

		std::deque<TemporalStateSnapshot> mSnapshots;
		float mElapsedSeconds = 0.f;
		float mRetentionSeconds = 4.f;
	};
}
