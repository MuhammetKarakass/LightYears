#include <framework/debug/LogBuffer.h>
#include <framework/debug/Profiler.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>

namespace
{
	int failures = 0;
	void Check(bool condition, const char* text)
	{
		if (!condition) { ++failures; std::cerr << text << '\n'; }
	}
}

int main()
{
	using namespace ly::debug;
	LogBuffer buffer{ 3 };
	LogRecord record{ LogLevel::Info, LogChannel::Lua, "same", { "a.lua", "print", 7 }, "12:00:00" };
	buffer.Push(record); buffer.Push(record);
	Check(buffer.Snapshot(true).size() == 1 && buffer.Snapshot(true)[0].count == 2, "Consecutive messages collapse");
	record.location.line = 8; buffer.Push(record);
	Check(buffer.Snapshot(true).size() == 2, "Source line participates in collapse identity");
	record.level = LogLevel::Error; buffer.Push(record);
	Check(buffer.Snapshot().size() == 3 && buffer.Snapshot().front().id == 2, "Ring drops oldest raw record");
	Check(buffer.State().counts[0] == 2 && buffer.State().counts[2] == 1, "Cached counts subtract evicted records");
	const auto revision = buffer.Revision(); buffer.Clear();
	Check(buffer.Snapshot().empty() && buffer.Revision() > revision, "Clear invalidates caches");
	Check(buffer.State().counts[0] == 0 && !buffer.State().latestIssue, "Clear resets status state");
	{
		std::string file = "temporary.lua";
		record.location.file = file.c_str(); buffer.Push(record);
	}
	Check(buffer.Snapshot()[0].file == "temporary.lua", "Source file storage must be owned");
	const auto retained = buffer.SharedSnapshot();
	buffer.Clear(); record.location.file = "thread.lua";
	Check(retained.size() == 1 && retained[0]->file == "temporary.lua", "Immutable UI snapshots survive ring mutation");
	std::thread writer([&]() { for (int i = 0; i < 1000; ++i) buffer.Push(record); });
	for (int i = 0; i < 1000; ++i) Check(buffer.Snapshot().size() <= 3, "Concurrent snapshots stay bounded");
	writer.join();
	Check(buffer.Snapshot().size() == 3, "Concurrent ring remains bounded");

	Log::Initialize("diagnostics-test.log");
	int original = 0, subscriber = 0;
	Log::SetSink([&](const auto&) { ++original; });
	const auto subscription = Log::AddSink([&](const auto&) { ++subscriber; });
	Log::Write(LogLevel::Info, LogChannel::Core, {}, "multi-sink preserved");
	Log::RemoveSink(subscription);
	Log::Write(LogLevel::Info, LogChannel::Core, {}, "after removal");
	Log::Flush();
	Check(original == 2 && subscriber == 1, "Subscribers preserve the legacy callback");
	std::ifstream log{ "diagnostics-test.log" };
	const std::string contents{ std::istreambuf_iterator<char>{ log }, {} };
	Check(contents.find("multi-sink preserved") != std::string::npos, "File output survives subscriptions");
	Log::SetSink({}); Log::Shutdown();

	Profiler::Initialize(); Profiler::SetEnabled(true);
	if (Profiler::IsCompiledIn())
	{
		Profiler::BeginFrame();
		{
			ProfileScope parent{ "Parent" };
			{ ProfileScope child{ "Child" }; }
			{ ProfileScope child{ "Child" }; }
		}
		Profiler::SetCounter("Actors", 12); Profiler::EndFrame();
		const auto frame = Profiler::GetFrame();
		const ProfileAggregate* parent = nullptr; const ProfileAggregate* child = nullptr;
		for (const auto& scope : frame.scopes) { if (scope.name == "Parent") parent = &scope; if (scope.name == "Child") child = &scope; }
		Check(parent && child && child->callCount == 2, "Per-frame names and calls captured");
		Check(parent && child && parent->selfMicroseconds >= 0 &&
			std::abs(parent->totalMicroseconds - parent->selfMicroseconds - child->totalMicroseconds) < .01, "Parent self time excludes children");
		Check(frame.counters.at("Actors") == 12, "Counters captured with frame");
		for (int i = 0; i < 305; ++i) { Profiler::BeginFrame(); Profiler::RecordScope("Ring", 10); Profiler::EndFrame(); }
		Check(Profiler::CaptureFrames().size() == 300 && Profiler::FrameTimes().size() == 300, "Frame history bounded at 300");
		Profiler::SetEnabled(false);
		const auto id = Profiler::GetFrame().id;
		Profiler::BeginFrame(); { ProfileScope disabled{ "Disabled" }; } Profiler::EndFrame();
		Check(Profiler::GetFrame().id == id, "Paused profiler retains captured frames");
		std::string error;
		Check(!Profiler::CaptureToFile("assets/forbidden.json", error), "Captures cannot enter assets");
		Check(Profiler::CaptureToFile("diagnostics-profile.json", error), "Profile capture writes JSON");
		std::ifstream input{ "diagnostics-profile.json" }; nlohmann::json json; input >> json;
		Check(json["frames"].size() == 300 && json["frames"][0]["scopes"][0]["name"] == "Ring", "Capture JSON round trip");
	}
	Profiler::SetEnabled(false); Profiler::Shutdown();
	return failures ? 1 : 0;
}
