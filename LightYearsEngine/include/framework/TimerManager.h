#pragma once
#include "framework/Core.h"
#include "framework/Object.h"
#include <functional>

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

		// Her frame tüm timer'larý günceller
		void UpdateTimer(float deltaTime);

		void ClearTimer(TimerHandle timerIndex);

		void ClearAllTimers();

		// Yeni timer kurar - template ile tip güvenli callback baðlama
		template<typename ClassName>
		TimerHandle SetTimer(weak_ptr<Object> weakRef, void(ClassName::* callback)(), float duration, bool repeat = false)
		{
			// Lambda ile member function pointer'ý std::function'a dönüþtür ve timer listesine ekle
			TimerHandle newHandle{};
			mTimers.insert({ newHandle, Timer(weakRef, [=] {(static_cast<ClassName*>(weakRef.lock().get())->*callback)(); }, duration, repeat) });
			return newHandle;
		}

		TimerHandle SetTimer(weak_ptr<Object> weakRef, std::function<void()> callback, float duration, bool repeat = false)
		{
			TimerHandle newHandle{};
			mTimers.insert({ newHandle, Timer(weakRef, callback, duration, repeat) });
			return newHandle;
		}	

	protected:
		// Protected constructor - sadece singleton eriþimi
		TimerManager();

	private:

		static unique_ptr<TimerManager> timerManager;
		static unique_ptr<TimerManager> globalTimerManager;
		static unique_ptr<TimerManager> gameTimerManager;
		Dictionary<TimerHandle, Timer, TimerHandleHashFunction> mTimers;
	};
}