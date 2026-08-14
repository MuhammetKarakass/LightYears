#include "gameplay/ability/runtime/AbilityUseHistory.h"

#include <utility>

namespace ly
{
	std::uint64_t AbilityUseHistory::Record(AbilityUseRecord record)
	{
		record.sequence = mNextSequence++;
		record.consumed = false;
		mRecords.push_front(std::move(record));
		if (mRecords.size() > MaxRecords)
		{
			mRecords.pop_back();
		}
		return mRecords.front().sequence;
	}

	const AbilityUseRecord* AbilityUseHistory::FindLatestUnconsumed(
		const std::function<bool(const AbilityUseRecord&)>& predicate
	) const
	{
		for (const AbilityUseRecord& record : mRecords)
		{
			if (!record.consumed && (!predicate || predicate(record)))
			{
				return &record;
			}
		}
		return nullptr;
	}

	bool AbilityUseHistory::Consume(std::uint64_t sequence)
	{
		for (auto record = mRecords.begin(); record != mRecords.end(); ++record)
		{
			if (record->sequence != sequence)
			{
				continue;
			}
			// Keep the snapshot for future systems. Echo maintains a separate
			// sequence cursor, so this flag does not make older abilities eligible
			// for a later Echo invocation.
			record->consumed = true;
			return true;
		}
		return false;
	}

	void AbilityUseHistory::Clear()
	{
		mRecords.clear();
		mNextSequence = 1;
	}
}
