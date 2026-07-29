#pragma once
#include "framework/Core.h"
#include "framework/Object.h"
#include <functional>
#include <utility>

namespace ly
{
	struct TimerHandle
	{
	public:
		TimerHandle();
		unsigned int GetTimerKey() const { return mTimerKey; }
	private:
		unsigned int mTimerKey;
		static unsigned int mTimerKeyCounter;
		static unsigned int GetNextTimerKey() { return ++mTimerKeyCounter; }
	};

	struct TimerHandleHashFunction
	{
	public:
		std::size_t operator()(const TimerHandle& timerHandle) const
		{
			return timerHandle.GetTimerKey();
		}
	};

	bool operator==(const TimerHandle& lhs, const TimerHandle& rhs);

	struct Timer
	{
	public:
		// Timer oluþturucu - nesne, callback, süre ve tekrar bilgisi alýr
		Timer(weak_ptr<Object> weakRef, std::function<void()> callBack, float duration, bool repeat = false);

		void TickTimer(float deltaTime);
		
		bool IsExpired() const;
		
		void SetExpired();
		
	private:
		std::pair<weak_ptr<Object>, std::function<void()>> mListener;  // Nesne + callback çifti
		float mDuration;      // Toplam süre (saniye)
		float mTimeCounter;   // Geçen süre sayacý
		bool mRepeat;         // Tekrar edecek mi?
		bool mIsExpired;      // Manuel olarak sonlandýrýldý mý?
	};

	class TimerManager
	{
	public:
		static TimerManager& GetTimerManager();

		static TimerManager& GetGlobalTimerManager();

		static TimerManager& GetGameTimerManager();

		static void ShutdownTimerManagers();

		// Her frame tüm timer'larý günceller
		void UpdateTimer(float deltaTime);

		void ClearTimer(TimerHandle timerIndex);

		void ClearAllTimers();

		// Yeni timer kurar - template ile tip güvenli callback baðlama
		template<typename ClassName>
		TimerHandle SetTimer(weak_ptr<Object> weakRef, void(ClassName::* callback)(), float duration, bool repeat = false)
		{
			return AddTimer(
				Timer(
					weakRef,
					[weakRef, callback]
					{
						if (shared_ptr<Object> strongRef = weakRef.lock())
						{
							(static_cast<ClassName*>(strongRef.get())->*callback)();
						}
					},
					duration,
					repeat
				)
			);
		}

		TimerHandle SetTimer(weak_ptr<Object> weakRef, std::function<void()> callback, float duration, bool repeat = false)
		{
			return AddTimer(Timer(weakRef, std::move(callback), duration, repeat));
		}	

	protected:
		// Protected constructor - sadece singleton eriþimi
		TimerManager();

	private:
		TimerHandle AddTimer(Timer timer);
		void FlushExpiredAndPendingTimers();

		static unique_ptr<TimerManager> timerManager;
		static unique_ptr<TimerManager> globalTimerManager;
		static unique_ptr<TimerManager> gameTimerManager;
		Dictionary<TimerHandle, Timer, TimerHandleHashFunction> mTimers;
		Dictionary<TimerHandle, Timer, TimerHandleHashFunction> mPendingTimers;
		bool mIsUpdating;
	};
}
