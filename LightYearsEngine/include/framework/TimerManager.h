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

	// Tek bir timer'ýn bilgilerini tutan yapý
	struct Timer
	{
	public:
		// Timer oluþturucu - nesne, callback, süre ve tekrar bilgisi alýr
		Timer(weak_ptr<Object> weakRef, std::function<void()> callBack, float duration, bool repeat = false);

		// Her frame çaðrýlýr - zamaný kontrol eder ve callback'i tetikler
		void TickTimer(float deltaTime);
		
		// Timer'ýn süresi doldu mu veya sahibi ölü mü kontrol eder
		bool IsExpired() const;
		
		// Timer'ý manuel olarak bitir
		void SetExpired();
		
	private:
		std::pair<weak_ptr<Object>, std::function<void()>> mListener;  // Nesne + callback çifti
		float mDuration;      // Toplam süre (saniye)
		float mTimeCounter;   // Geçen süre sayacý
		bool mRepeat;         // Tekrar edecek mi?
		bool mIsExpired;      // Manuel olarak sonlandýrýldý mý?
	};

	// Tüm timer'larý yöneten singleton sýnýf (Unity'deki Invoke sistemi benzeri)
	class TimerManager
	{
	public:
		// Singleton instance'ýný döndürür
		static TimerManager& GetTimerManager();

		// Her frame tüm timer'larý günceller
		void UpdateTimer(float deltaTime);

		void ClearTimer(TimerHandle timerIndex);

		// Yeni timer kurar - template ile tip güvenli callback baðlama
		template<typename ClassName>
		TimerHandle SetTimer(weak_ptr<Object> weakRef, void(ClassName::* callback)(), float duration, bool repeat = false)
		{
			// Lambda ile member function pointer'ý std::function'a dönüþtür ve timer listesine ekle
			TimerHandle newHandle{};
			mTimers.insert({ newHandle, Timer(weakRef, [=] {(static_cast<ClassName*>(weakRef.lock().get())->*callback)(); }, duration, repeat) });
			return newHandle;
		}
		
	protected:
		// Protected constructor - sadece singleton eriþimi
		TimerManager();
		
	private:

		static unique_ptr<TimerManager> timerManager;  // Singleton instance
		Dictionary<TimerHandle,Timer,TimerHandleHashFunction> mTimers;  // Aktif timer'larýn listesi
	};
}