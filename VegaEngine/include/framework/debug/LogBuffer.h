#pragma once

#include "framework/debug/Log.h"
#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <mutex>

namespace vega::debug
{
	struct ConsoleRecord
	{
		std::uint64_t id = 0;
		LogLevel level;
		LogChannel channel;
		std::string message, timestamp, file, function;
		int line = 0;
		std::size_t count = 1;
	};

	struct LogBufferState
	{
		std::uint64_t revision = 0;
		std::array<std::size_t, 3> counts{};
		std::shared_ptr<const ConsoleRecord> latestIssue;
	};

	// Own source strings: Lua paths and other caller-owned locations need not outlive Write().
	class LogBuffer
	{
	public:
		explicit LogBuffer(std::size_t capacity = 5000) : mCapacity{ std::max(std::size_t{ 1 }, capacity) } {}
		void Push(const LogRecord& record)
		{
			std::lock_guard<std::mutex> lock{ mMutex };
			if (mRecords.size() == mCapacity)
			{
				--mCounts[Group(mRecords.front()->level)];
				mRecords.pop_front();
			}
			auto captured = std::make_shared<const ConsoleRecord>(ConsoleRecord{ ++mNextID, record.level, record.channel, record.message, record.timestamp,
				record.location.file ? record.location.file : "", record.location.function ? record.location.function : "", record.location.line });
			++mCounts[Group(record.level)];
			if (Group(record.level)) mLatestIssue = captured;
			mRecords.push_back(std::move(captured));
			++mRevision;
		}
		void Clear()
		{
			std::lock_guard<std::mutex> lock{ mMutex };
			mRecords.clear();
			mCounts = {};
			mLatestIssue.reset();
			++mRevision;
		}
		std::uint64_t Revision() const
		{
			std::lock_guard<std::mutex> lock{ mMutex };
			return mRevision;
		}
		std::vector<ConsoleRecord> Snapshot(bool collapse = false) const
		{
			std::lock_guard<std::mutex> lock{ mMutex };
			std::vector<ConsoleRecord> rows;
			rows.reserve(mRecords.size());
			for (const auto& record : mRecords)
			{
				if (collapse && !rows.empty() && Identical(rows.back(), *record)) ++rows.back().count;
				else rows.push_back(*record);
			}
			return rows;
		}
		// UI snapshots share immutable strings, so a new line never copies 5,000 messages.
		std::vector<std::shared_ptr<const ConsoleRecord>> SharedSnapshot() const
		{
			std::lock_guard<std::mutex> lock{ mMutex };
			return { mRecords.begin(), mRecords.end() };
		}
		LogBufferState State() const
		{
			std::lock_guard<std::mutex> lock{ mMutex };
			return { mRevision, mCounts, mLatestIssue };
		}
		static bool Identical(const ConsoleRecord& a, const ConsoleRecord& b)
		{
			return a.level == b.level && a.channel == b.channel && a.message == b.message && a.file == b.file && a.line == b.line;
		}
	private:
		static int Group(LogLevel level) { return level >= LogLevel::Error ? 2 : level == LogLevel::Warning ? 1 : 0; }
		mutable std::mutex mMutex;
		std::deque<std::shared_ptr<const ConsoleRecord>> mRecords;
		std::array<std::size_t, 3> mCounts{};
		std::shared_ptr<const ConsoleRecord> mLatestIssue;
		std::size_t mCapacity;
		std::uint64_t mRevision = 1, mNextID = 0;
	};

	class LogCapture
	{
	public:
		LogCapture() : mBuffer{ std::make_shared<LogBuffer>() }, mSink{ Log::AddSink([buffer = mBuffer](const LogRecord& r) { buffer->Push(r); }) } {}
		~LogCapture() { Log::RemoveSink(mSink); }
		LogCapture(const LogCapture&) = delete;
		LogCapture& operator=(const LogCapture&) = delete;
		LogBuffer& Buffer() const { return *mBuffer; }
	private:
		// An in-flight callback owns the buffer even after RemoveSink returns.
		std::shared_ptr<LogBuffer> mBuffer;
		std::uint64_t mSink;
	};
}
