#include "framework/TimerManager.h"

namespace ly
{
	unsigned int TimerHandle::mTimerKeyCounter = 0;
	unique_ptr<TimerManager> TimerManager::timerManager{ nullptr };
	unique_ptr<TimerManager> TimerManager::globalTimerManager{ nullptr };
	unique_ptr<TimerManager> TimerManager::gameTimerManager{ nullptr };

	TimerHandle::TimerHandle()
		: mTimerKey{ GetNextTimerKey() }
	{

	}

	TimerManager& TimerManager::GetTimerManager()
	{
		if (timerManager == nullptr)
		{
			timerManager = unique_ptr<TimerManager>(new TimerManager({}));
		}
		return *timerManager;
	}

	TimerManager& TimerManager::GetGlobalTimerManager()
	{
		if(globalTimerManager == nullptr)
		{
			globalTimerManager = unique_ptr<TimerManager>(new TimerManager({}));
		}
		return *globalTimerManager;
	}

	TimerManager& TimerManager::GetGameTimerManager()
	{
		if(gameTimerManager == nullptr)
		{
			gameTimerManager = unique_ptr<TimerManager>(new TimerManager({}));
		}
		return *gameTimerManager;
	}

	void TimerManager::ShutdownTimerManagers()
	{
		if (timerManager)
		{
			timerManager->ClearAllTimers();
			timerManager.reset();
		}

		if (globalTimerManager)
		{
			globalTimerManager->ClearAllTimers();
			globalTimerManager.reset();
		}

		if (gameTimerManager)
		{
			gameTimerManager->ClearAllTimers();
			gameTimerManager.reset();
		}
	}

	bool operator==(const TimerHandle& lhs, const TimerHandle& rhs)
	{
		return lhs.GetTimerKey() == rhs.GetTimerKey();
	}

	void TimerManager::UpdateTimer(float deltaTime)
	{
		mIsUpdating = true;
		try
		{
			for (auto& timerEntry : mTimers)
			{
				if (!timerEntry.second.IsExpired())
				{
					timerEntry.second.TickTimer(deltaTime);
				}
			}
		}
		catch (...)
		{
			mIsUpdating = false;
			FlushExpiredAndPendingTimers();
			throw;
		}

		mIsUpdating = false;
		FlushExpiredAndPendingTimers();
	}

	void TimerManager::ClearTimer(TimerHandle timerHandle)
	{
		auto iter = mTimers.find(timerHandle);
		if (iter != mTimers.end())
		{
			iter->second.SetExpired();
		}

		mPendingTimers.erase(timerHandle);
	}

	void TimerManager::ClearAllTimers()
	{
		mPendingTimers.clear();
		if (mIsUpdating)
		{
			for (auto& timerEntry : mTimers)
			{
				timerEntry.second.SetExpired();
			}
			return;
		}

		mTimers.clear();
	}

	TimerManager::TimerManager():
		mTimers{},
		mPendingTimers{},
		mIsUpdating{ false }
	{
	}

	TimerHandle TimerManager::AddTimer(Timer timer)
	{
		TimerHandle newHandle{};
		auto& destination = mIsUpdating ? mPendingTimers : mTimers;
		destination.emplace(newHandle, std::move(timer));
		return newHandle;
	}

	void TimerManager::FlushExpiredAndPendingTimers()
	{
		for (auto iter = mTimers.begin(); iter != mTimers.end();)
		{
			if (iter->second.IsExpired())
			{
				iter = mTimers.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		if (mPendingTimers.empty())
		{
			return;
		}

		mTimers.reserve(mTimers.size() + mPendingTimers.size());
		for (auto& pendingTimer : mPendingTimers)
		{
			mTimers.emplace(pendingTimer.first, std::move(pendingTimer.second));
		}
		mPendingTimers.clear();
	}

	Timer::Timer(weak_ptr<Object> weakRef, std::function<void()> callBack, float duration, bool repeat):
		mListener{ weakRef, callBack },
		mDuration{ duration },
		mTimeCounter{ 0.f },
		mRepeat{ repeat },
		mIsExpired{ false }
	{
	}

	void Timer::TickTimer(float deltaTime)
	{
		if (IsExpired()) return;
		mTimeCounter += deltaTime;
		if (mTimeCounter >= mDuration)
		{
			mListener.second();

			if(mRepeat)
			{
				mTimeCounter = 0.f;
			}
			else
			{
				SetExpired();
			}
		}
	}

	bool Timer::IsExpired() const
	{
		if (mIsExpired)
		{
			return true;
		}

		const shared_ptr<Object> listener = mListener.first.lock();
		return !listener || listener->GetIsPendingDestroy();
	}

	void Timer::SetExpired()
	{
		mIsExpired = true;
	}

}
