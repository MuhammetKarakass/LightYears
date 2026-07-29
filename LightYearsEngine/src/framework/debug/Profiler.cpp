#include "framework/debug/Profiler.h"

#include "framework/debug/Log.h"

#include <algorithm>
#include <limits>
#include <mutex>

namespace ly::debug
{
	namespace
	{
		std::mutex gProfilerMutex;
		std::unordered_map<std::string, ProfileAggregate> gAggregates;
		std::unordered_map<std::string, double> gCounters;
		bool gProfilerEnabled = false;
	}

	void Profiler::Initialize()
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		gAggregates.clear();
		gCounters.clear();
		gProfilerEnabled = IsCompiledIn();
	}

	void Profiler::Shutdown()
	{
		if (IsEnabled())
		{
			LogSummary();
		}
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		gProfilerEnabled = false;
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
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		return gProfilerEnabled;
	}

	void Profiler::SetEnabled(bool enabled)
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		gProfilerEnabled = enabled && IsCompiledIn();
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
		std::vector<ProfileAggregate> snapshot;
		snapshot.reserve(gAggregates.size());
		for (const auto& entry : gAggregates)
		{
			snapshot.push_back(entry.second);
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
	}
}
