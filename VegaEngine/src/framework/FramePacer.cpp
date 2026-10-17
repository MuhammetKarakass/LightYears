#include "framework/FramePacer.h"

#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
#endif

namespace vega
{
	namespace
	{
		// The OS sleep only covers the part of the wait it can hit reliably; the rest is spun. A high-resolution
		// waitable timer wakes within ~0.5 ms, a plain 1 ms-resolution Sleep within ~2 ms.
		constexpr std::chrono::microseconds TimerSpinMargin{ 1000 };
		constexpr std::chrono::microseconds SleepSpinMargin{ 2000 };
	}

	FramePacer::FramePacer(float framesPerSecond)
		: mPeriod{ std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / framesPerSecond)) },
		mDeadline{ Clock::now() + mPeriod }
	{
#ifdef _WIN32
		mTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
		if (mTimer == nullptr)
		{
			// Before Windows 10 1803: fall back to a 1 ms system timer for the whole run rather than toggling it
			// around every Sleep, which is what SFML does and which makes each sleep's accuracy unpredictable.
			mRaisedTimerResolution = timeBeginPeriod(1) == TIMERR_NOERROR;
		}
#endif
	}

	FramePacer::~FramePacer()
	{
#ifdef _WIN32
		if (mTimer != nullptr)
		{
			CloseHandle(mTimer);
		}
		if (mRaisedTimerResolution)
		{
			timeEndPeriod(1);
		}
#endif
	}

	void FramePacer::Wait()
	{
		const Clock::time_point now = Clock::now();
		if (now >= mDeadline)
		{
			// A frame that ran over its slot starts a fresh schedule instead of rushing several short frames to catch up.
			mDeadline = now - mDeadline > mPeriod ? now + mPeriod : mDeadline + mPeriod;
			return;
		}

		const Clock::duration margin = mTimer != nullptr ? Clock::duration(TimerSpinMargin) : Clock::duration(SleepSpinMargin);
		if (mDeadline - now > margin)
		{
			SleepFor(mDeadline - now - margin);
		}
		while (Clock::now() < mDeadline)
		{
			std::this_thread::yield();
		}
		mDeadline += mPeriod;
	}

	void FramePacer::SleepFor(Clock::duration duration)
	{
#ifdef _WIN32
		if (mTimer != nullptr)
		{
			// Relative due time in 100 ns units.
			LARGE_INTEGER dueTime;
			dueTime.QuadPart = -static_cast<LONGLONG>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() / 100);
			if (SetWaitableTimerEx(mTimer, &dueTime, 0, nullptr, nullptr, nullptr, 0))
			{
				WaitForSingleObject(mTimer, INFINITE);
				return;
			}
		}
		Sleep(static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()));
#else
		std::this_thread::sleep_for(duration);
#endif
	}
}
