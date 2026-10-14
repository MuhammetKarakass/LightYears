#include "framework/debug/Profiler.h"

#include "framework/debug/Log.h"

#include <algorithm>
#include <atomic>
#include <limits>
#include <mutex>

namespace ly::debug
{
	namespace
	{
		std::mutex gProfilerMutex;
		// Keyed by the name pointer (string literals and typeid names live for the whole run), so recording a
		// scope neither builds a std::string nor hashes text; BuildSnapshot merges entries that share a name.
		std::unordered_map<const char*, ProfileAggregate> gAggregates;
		std::unordered_map<std::string, double> gCounters;
		// Read by every scope; atomic so the check does not take the mutex.
		std::atomic<bool> gProfilerEnabled{ false };
		// Main-thread only, like the frame loop that drains it.
		std::vector<FrameEvent> gFrameEvents;
	}

	void Profiler::Initialize()
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		gAggregates.clear();
		gCounters.clear();
		gProfilerEnabled.store(IsCompiledIn());
	}

	void Profiler::Shutdown()
	{
		if (IsEnabled())
		{
			LogSummary(40);
		}
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		gProfilerEnabled.store(false);
		gAggregates.clear();
		gCounters.clear();
	}

	void Profiler::Reset()
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		gAggregates.clear();
		gCounters.clear();
	}

	bool Profiler::IsCompiledIn()
	{
		return LY_ENABLE_PROFILING != 0;
	}

	bool Profiler::IsEnabled()
	{
		return gProfilerEnabled.load(std::memory_order_relaxed);
	}

	void Profiler::SetEnabled(bool enabled)
	{
		gProfilerEnabled.store(enabled && IsCompiledIn());
	}

	void Profiler::RecordScope(const char* name, double elapsedMicroseconds)
	{
		if (!name || !IsEnabled())
		{
			return;
		}
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		ProfileAggregate& aggregate = gAggregates[name];
		if (aggregate.callCount == 0)
		{
			aggregate.name = name;
			aggregate.minimumMicroseconds =
				std::numeric_limits<double>::max();
		}
		++aggregate.callCount;
		aggregate.totalMicroseconds += elapsedMicroseconds;
		aggregate.minimumMicroseconds =
			std::min(aggregate.minimumMicroseconds, elapsedMicroseconds);
		aggregate.maximumMicroseconds =
			std::max(aggregate.maximumMicroseconds, elapsedMicroseconds);
	}

	void Profiler::SetCounter(const char* name, double value)
	{
		if (!name || !IsEnabled())
		{
			return;
		}
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		gCounters[name] = value;
	}

	std::vector<ProfileAggregate> Profiler::BuildSnapshot()
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		// The same name can sit under several pointers (identical literals in different translation units).
		std::unordered_map<std::string, ProfileAggregate> byName;
		for (const auto& entry : gAggregates)
		{
			ProfileAggregate& merged = byName[entry.second.name];
			if (merged.callCount == 0)
			{
				merged = entry.second;
				continue;
			}
			merged.callCount += entry.second.callCount;
			merged.totalMicroseconds += entry.second.totalMicroseconds;
			merged.minimumMicroseconds = std::min(merged.minimumMicroseconds, entry.second.minimumMicroseconds);
			merged.maximumMicroseconds = std::max(merged.maximumMicroseconds, entry.second.maximumMicroseconds);
		}
		std::vector<ProfileAggregate> snapshot;
		snapshot.reserve(byName.size());
		for (auto& entry : byName)
		{
			snapshot.push_back(std::move(entry.second));
		}
		std::sort(
			snapshot.begin(),
			snapshot.end(),
			[](const ProfileAggregate& left, const ProfileAggregate& right)
			{
				return left.totalMicroseconds > right.totalMicroseconds;
			}
		);
		return snapshot;
	}

	std::unordered_map<std::string, double> Profiler::BuildCounterSnapshot()
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		return gCounters;
	}

	void Profiler::LogSummary(std::size_t maximumEntries)
	{
		const std::vector<ProfileAggregate> snapshot = BuildSnapshot();
		if (snapshot.empty())
		{
			return;
		}
		LY_CORE_INFO("Profiler summary: %zu scopes", snapshot.size());
		const std::size_t count = std::min(maximumEntries, snapshot.size());
		for (std::size_t index = 0; index < count; ++index)
		{
			const ProfileAggregate& aggregate = snapshot[index];
			const double average = aggregate.callCount > 0
				? aggregate.totalMicroseconds /
					static_cast<double>(aggregate.callCount)
				: 0.0;
			LY_CORE_INFO(
				"  %s calls=%llu total=%.3fms avg=%.3fus max=%.3fus",
				aggregate.name.c_str(),
				static_cast<unsigned long long>(aggregate.callCount),
				aggregate.totalMicroseconds / 1000.0,
				average,
				aggregate.maximumMicroseconds
			);
		}
	}

	void Profiler::RecordFrameEvent(const char* name, double elapsedMicroseconds)
	{
		if (name && elapsedMicroseconds >= FrameEventThresholdMicroseconds && gFrameEvents.size() < 256)
		{
			gFrameEvents.push_back({ name, elapsedMicroseconds });
		}
	}

	void Profiler::TakeFrameEvents(std::vector<FrameEvent>& out)
	{
		out.clear();
		out.swap(gFrameEvents);
	}

	ProfileScope::ProfileScope(const char* name)
		: mName{ name }
		, mEnabled{ Profiler::IsEnabled() }
		, mStart{ mEnabled
			? std::chrono::steady_clock::now()
			: std::chrono::steady_clock::time_point{} }
	{
	}

	ProfileScope::~ProfileScope()
	{
		if (!mEnabled)
		{
			return;
		}
		const auto end = std::chrono::steady_clock::now();
		const double elapsedMicroseconds =
			std::chrono::duration<double, std::micro>(end - mStart).count();
		Profiler::RecordScope(mName, elapsedMicroseconds);
		Profiler::RecordFrameEvent(mName, elapsedMicroseconds);
	}
}
