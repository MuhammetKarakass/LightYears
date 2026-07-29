#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef LY_ENABLE_PROFILING
	#if defined(NDEBUG)
		#define LY_ENABLE_PROFILING 0
	#else
		#define LY_ENABLE_PROFILING 1
	#endif
#endif

namespace ly::debug
{
	struct ProfileAggregate
	{
		std::string name;
		std::uint64_t callCount = 0;
		double totalMicroseconds = 0.0;
		double minimumMicroseconds = 0.0;
		double maximumMicroseconds = 0.0;
	};

	class Profiler
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void Reset();

		static bool IsCompiledIn();
		static bool IsEnabled();
		static void SetEnabled(bool enabled);

		static void RecordScope(const char* name, double elapsedMicroseconds);
		static void SetCounter(const char* name, double value);
		static std::vector<ProfileAggregate> BuildSnapshot();
		static std::unordered_map<std::string, double> BuildCounterSnapshot();
		static void LogSummary(std::size_t maximumEntries = 20u);
	};

	class ProfileScope
	{
	public:
		explicit ProfileScope(const char* name);
		~ProfileScope();

		ProfileScope(const ProfileScope&) = delete;
		ProfileScope& operator=(const ProfileScope&) = delete;

	private:
		const char* mName;
		bool mEnabled;
		std::chrono::steady_clock::time_point mStart;
	};
}

#define LY_DIAGNOSTICS_CONCAT_INNER(left, right) left##right
#define LY_DIAGNOSTICS_CONCAT(left, right) \
	LY_DIAGNOSTICS_CONCAT_INNER(left, right)

#if LY_ENABLE_PROFILING
	#define LY_PROFILE_SCOPE(name) \
		::ly::debug::ProfileScope \
			LY_DIAGNOSTICS_CONCAT(lyProfileScope_, __LINE__){ name }
	#define LY_PROFILE_FUNCTION() LY_PROFILE_SCOPE(__func__)
	#define LY_PROFILE_FRAME() LY_PROFILE_SCOPE("Application.Frame")
	#define LY_PROFILE_COUNTER(name, value) \
		::ly::debug::Profiler::SetCounter(name, static_cast<double>(value))
#else
	#define LY_PROFILE_SCOPE(name) ((void)0)
	#define LY_PROFILE_FUNCTION() ((void)0)
	#define LY_PROFILE_FRAME() ((void)0)
	#define LY_PROFILE_COUNTER(name, value) ((void)0)
#endif
