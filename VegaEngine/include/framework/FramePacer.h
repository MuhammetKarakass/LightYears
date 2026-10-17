#pragma once

#include <chrono>

namespace vega
{
	// Holds the main loop to a fixed frame rate against absolute deadlines.
	// sf::Window::setFramerateLimit sleeps a whole-millisecond amount and restarts its clock after every sleep, so
	// the Sleep overshoot is never paid back and frames land at 16-18 ms instead of a steady 16.67 ms. Here each
	// deadline is the previous one plus the period: a late frame makes the next wait shorter, and the last stretch
	// before the deadline is spun so the wake-up is accurate to a few microseconds.
	class FramePacer
	{
	public:
		explicit FramePacer(float framesPerSecond);
		~FramePacer();
		FramePacer(const FramePacer&) = delete;
		FramePacer& operator=(const FramePacer&) = delete;

		// Blocks until the current frame's deadline, then starts the next frame.
		void Wait();

	private:
		using Clock = std::chrono::steady_clock;

		void SleepFor(Clock::duration duration);

		Clock::duration mPeriod;
		Clock::time_point mDeadline;
		void* mTimer = nullptr;
		bool mRaisedTimerResolution = false;
	};
}
