#pragma once

#include <string>
#include <vector>

namespace ly
{
	// A resource can temporarily exceed its normal maximum, but the excess must
	// still be attributable to a source so damage can consume it before its timer
	// expires. Health and shield share this rule; ability code must not duplicate it.
	struct TemporaryOvercapRequest
	{
		std::string sourceId;
		float amount = 0.f;
		float holdDuration = 0.f;
		float decayPerSecond = 0.f;
	};

	class TemporaryOvercapLedger
	{
	public:
		void Add(const TemporaryOvercapRequest& request);
		void Consume(float amount);
		void Reconcile(float currentValue, float normalMaximum);
		// Returns the amount the owning resource should remove this frame.
		float Tick(float deltaTime, float currentValue, float normalMaximum);
		void Clear();
		bool IsEmpty() const { return mEntries.empty(); }

	private:
		struct Entry
		{
			std::string sourceId;
			float amount = 0.f;
			float holdRemaining = 0.f;
			float decayPerSecond = 0.f;
		};

		void RemoveEmptyEntries();

		std::vector<Entry> mEntries;
	};
}
