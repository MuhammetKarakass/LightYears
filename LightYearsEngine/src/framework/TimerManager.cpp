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

	bool operator==(const TimerHandle& lhs, const TimerHandle& rhs)
	{
		return lhs.GetTimerKey() == rhs.GetTimerKey();
	}

	void TimerManager::UpdateTimer(float deltaTime)
	{
		for(auto iter=mTimers.begin(); iter!=mTimers.end();)
		{
			if (iter->second.IsExpired())
			{
				iter = mTimers.erase(iter);
			}
			else
			{
				iter->second.TickTimer(deltaTime);
				iter++;
			}
		}
	}

	void TimerManager::ClearTimer(TimerHandle timerHandle)
	{
		auto iter = mTimers.find(timerHandle);
		if (iter != mTimers.end())
		{
			iter->second.SetExpired();
		}
	}

	void TimerManager::ClearAllTimers()
	{
		mTimers.clear();
	}

	TimerManager::TimerManager():
		mTimers{}
	{
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
		return mIsExpired || mListener.first.expired() || mListener.first.lock()->GetIsPendingDestroy();
	}

	void Timer::SetExpired()
	{
		mIsExpired = true;
	}

}