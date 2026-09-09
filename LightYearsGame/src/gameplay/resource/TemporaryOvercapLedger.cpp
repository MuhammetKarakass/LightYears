#include "gameplay/resource/TemporaryOvercapLedger.h"

#include <algorithm>

namespace ly
{
	void TemporaryOvercapLedger::Add(const TemporaryOvercapRequest& request)
	{
		if (request.amount <= 0.f)
		{
			return;
		}

		mEntries.push_back(Entry{
			request.sourceId,
			request.amount,
			std::max(0.f, request.holdDuration),
			std::max(0.f, request.decayPerSecond)
		});
	}

	void TemporaryOvercapLedger::Consume(float amount)
	{
		float remaining = std::max(0.f, amount);
		for (Entry& entry : mEntries)
		{
			if (remaining <= 0.f)
			{
				break;
			}
			const float consumed = std::min(entry.amount, remaining);
			entry.amount -= consumed;
			remaining -= consumed;
		}
		RemoveEmptyEntries();
	}

	void TemporaryOvercapLedger::Reconcile(float currentValue, float normalMaximum)
	{
		const float availableExcess = std::max(0.f, currentValue - normalMaximum);
		float recordedExcess = 0.f;
		for (const Entry& entry : mEntries)
		{
			recordedExcess += std::max(0.f, entry.amount);
		}
		if (recordedExcess > availableExcess)
		{
			Consume(recordedExcess - availableExcess);
		}
		RemoveEmptyEntries();
	}

	float TemporaryOvercapLedger::Tick(float deltaTime, float currentValue, float normalMaximum)
	{
		if (deltaTime <= 0.f || mEntries.empty())
		{
			return 0.f;
		}

		Reconcile(currentValue, normalMaximum);
		float decayAmount = 0.f;
		for (Entry& entry : mEntries)
		{
			float decayTime = deltaTime;
			if (entry.holdRemaining > 0.f)
			{
				const float heldTime = std::min(entry.holdRemaining, decayTime);
				entry.holdRemaining -= heldTime;
				decayTime -= heldTime;
			}
			if (decayTime <= 0.f || entry.decayPerSecond <= 0.f)
			{
				continue;
			}

			// Decay is continuous and time-proportional. Quantizing it into a
			// fixed 0.1 second tick makes a 0.01 second frame do nothing and
			// then removes ten points in one frame. Multiplying by elapsed time
			// keeps the same rate at every frame size: 100/s means 1 in 0.01s
			// and 10 in 0.1s.
			const float frameDecay = std::min(
				entry.amount,
				entry.decayPerSecond * decayTime
			);
			entry.amount -= frameDecay;
			decayAmount += frameDecay;
		}

		RemoveEmptyEntries();
		return std::min(decayAmount, std::max(0.f, currentValue - normalMaximum));
	}

	void TemporaryOvercapLedger::Clear()
	{
		mEntries.clear();
	}

	void TemporaryOvercapLedger::RemoveEmptyEntries()
	{
		mEntries.erase(
			std::remove_if(
				mEntries.begin(),
				mEntries.end(),
				[](const Entry& entry) { return entry.amount <= 0.001f; }
			),
			mEntries.end()
		);
	}
}
