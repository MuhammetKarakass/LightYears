#include "framework/debug/Profiler.h"

#include "framework/debug/Log.h"

#include <algorithm>
#include <atomic>
#include <limits>
#include <mutex>
#include <deque>
#include <fstream>
#include <nlohmann/json.hpp>
#include <cctype>

namespace vega::debug
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
		std::deque<ProfileFrame> gFrames;
		std::unordered_map<const char*, ProfileAggregate> gCurrentScopes;
		std::chrono::steady_clock::time_point gFrameStart;
		bool gFrameActive = false;
		std::uint64_t gFrameID = 0;
		thread_local ProfileScope* gParentScope = nullptr;

		void Accumulate(ProfileAggregate& aggregate, const char* name, double elapsed, double self)
		{
			if (!aggregate.callCount)
			{
				aggregate.name = name;
				aggregate.minimumMicroseconds = elapsed;
			}
			++aggregate.callCount;
			aggregate.totalMicroseconds += elapsed;
			aggregate.selfMicroseconds += self;
			aggregate.minimumMicroseconds = std::min(aggregate.minimumMicroseconds, elapsed);
			aggregate.maximumMicroseconds = std::max(aggregate.maximumMicroseconds, elapsed);
		}
	}

	void Profiler::Initialize()
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		gAggregates.clear();
		gCounters.clear();
		gFrames.clear();
		gCurrentScopes.clear();
		gFrameActive = false;
		// Profiling is now compiled into release builds too, but only Debug records from
		// startup as before; release games stay off unless a tool turns it on. The editor
		// explicitly starts paused.
#if defined(NDEBUG)
		gProfilerEnabled.store(false);
#else
		gProfilerEnabled.store(IsCompiledIn());
#endif
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
		gFrames.clear();
		gCurrentScopes.clear();
		gFrameActive = false;
	}

	bool Profiler::IsCompiledIn()
	{
		return VEGA_ENABLE_PROFILING != 0;
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
		RecordCompletedScope(name, elapsedMicroseconds, elapsedMicroseconds);
	}

	void Profiler::RecordCompletedScope(const char* name, double elapsedMicroseconds, double self)
	{
		if (!name || !IsEnabled())
		{
			return;
		}
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		if (gAggregates.size() < 1024 || gAggregates.count(name)) Accumulate(gAggregates[name], name, elapsedMicroseconds, self);
		if (gFrameActive && (gCurrentScopes.size() < 1024 || gCurrentScopes.count(name)))
			Accumulate(gCurrentScopes[name], name, elapsedMicroseconds, self);
	}

	void Profiler::BeginFrame()
	{
		if (!IsEnabled()) return;
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		gCurrentScopes.clear();
		gFrameStart = std::chrono::steady_clock::now();
		gFrameActive = true;
	}

	void Profiler::EndFrame()
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		if (!gFrameActive) return;
		ProfileFrame frame;
		frame.id = ++gFrameID;
		frame.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - gFrameStart).count();
		// Merge identical text from literals in different translation units.
		std::unordered_map<std::string, ProfileAggregate> merged;
		for (const auto& item : gCurrentScopes)
		{
			auto& target = merged[item.second.name];
			const auto& source = item.second;
			if (!target.callCount) target = source;
			else
			{
				target.callCount += source.callCount;
				target.totalMicroseconds += source.totalMicroseconds;
				target.selfMicroseconds += source.selfMicroseconds;
				target.minimumMicroseconds = std::min(target.minimumMicroseconds, source.minimumMicroseconds);
				target.maximumMicroseconds = std::max(target.maximumMicroseconds, source.maximumMicroseconds);
			}
		}
		for (auto& item : merged) frame.scopes.push_back(std::move(item.second));
		frame.counters = gCounters;
		if (gFrames.size() == 300) gFrames.pop_front();
		gFrames.push_back(std::move(frame));
		gFrameActive = false;
	}

	std::vector<ProfileFrame> Profiler::CaptureFrames()
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		return { gFrames.begin(), gFrames.end() };
	}

	std::vector<std::pair<std::uint64_t, float>> Profiler::FrameTimes()
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		std::vector<std::pair<std::uint64_t, float>> times;
		times.reserve(gFrames.size());
		for (const auto& frame : gFrames) times.emplace_back(frame.id, static_cast<float>(frame.milliseconds));
		return times;
	}

	ProfileFrame Profiler::GetFrame(std::uint64_t id)
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		if (!id && !gFrames.empty()) return gFrames.back();
		for (const auto& frame : gFrames) if (frame.id == id) return frame;
		return {};
	}

	bool Profiler::CaptureToFile(const std::filesystem::path& path, std::string& error)
	{
		try
		{
			// Captures are diagnostics, never authored assets (also covers runtime asset copies).
			for (const auto& component : std::filesystem::weakly_canonical(path))
			{
				auto name = component.string();
				std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				if (name == "assets") throw std::runtime_error("Profile captures cannot be written into assets");
			}
			nlohmann::json document = { { "format", "vega.profile" }, { "version", 1 }, { "frames", nlohmann::json::array() } };
			for (const auto& frame : CaptureFrames())
			{
				nlohmann::json scopes = nlohmann::json::array();
				for (const auto& s : frame.scopes) scopes.push_back({ { "name", s.name }, { "calls", s.callCount },
					{ "totalMs", s.totalMicroseconds / 1000.0 }, { "selfMs", s.selfMicroseconds / 1000.0 },
					{ "avgMs", s.totalMicroseconds / (1000.0 * s.callCount) }, { "maxMs", s.maximumMicroseconds / 1000.0 } });
				document["frames"].push_back({ { "id", frame.id }, { "ms", frame.milliseconds }, { "scopes", scopes }, { "counters", frame.counters } });
			}
			if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
			std::ofstream output{ path };
			output << document.dump(2) << '\n';
			output.close();
			if (!output) throw std::runtime_error("Cannot write profile capture: " + path.string());
			error.clear();
			return true;
		}
		catch (const std::exception& failure) { error = failure.what(); return false; }
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
			merged.selfMicroseconds += entry.second.selfMicroseconds;
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
		VEGA_CORE_INFO("Profiler summary: %zu scopes", snapshot.size());
		const std::size_t count = std::min(maximumEntries, snapshot.size());
		for (std::size_t index = 0; index < count; ++index)
		{
			const ProfileAggregate& aggregate = snapshot[index];
			const double average = aggregate.callCount > 0
				? aggregate.totalMicroseconds /
					static_cast<double>(aggregate.callCount)
				: 0.0;
			VEGA_CORE_INFO(
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
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
		if (name && elapsedMicroseconds >= FrameEventThresholdMicroseconds && gFrameEvents.size() < 256)
		{
			gFrameEvents.push_back({ name, elapsedMicroseconds });
		}
	}

	void Profiler::TakeFrameEvents(std::vector<FrameEvent>& out)
	{
		std::lock_guard<std::mutex> lock{ gProfilerMutex };
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
		if (mEnabled)
		{
			mParent = gParentScope;
			gParentScope = this;
		}
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
		gParentScope = mParent;
		if (mParent) mParent->mChildren += elapsedMicroseconds;
		Profiler::RecordCompletedScope(mName, elapsedMicroseconds, std::max(0.0, elapsedMicroseconds - mChildren));
		Profiler::RecordFrameEvent(mName, elapsedMicroseconds);
	}
}
