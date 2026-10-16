#include "framework/EngineContext.h"
#include <framework/debug/LogBuffer.h>
#include "editor/ContentWatch.h"
#include "editor/JsonDocument.h"
#include <fstream>
#include <iostream>

int main()
{
	ly::EngineContext engineContext;
	using namespace vega;
	const auto root = std::filesystem::path{ VEGA_CONTENT_WATCH_TEST_ROOT };
	std::filesystem::create_directories(root / "data");
	std::filesystem::create_directories(root / ".vega");
	std::ofstream{ root / "data/item.json" } << "{\"value\":1}";
	ContentWatch watch;
	watch.Begin(root);
	int failures = 0;
	const auto check = [&](bool value, const char* text) { if (!value) { ++failures; std::cerr << text << '\n'; } };
	check(watch.Poll().empty(), "Unchanged files reloaded");
	std::string error;
	check(AtomicWriteText(root / "data/item.json", "{\"value\":1234}", error), "Cannot edit fixture");
	check(watch.Poll().size() == 1, "External atomic JSON replacement missed");
	check(watch.Poll().empty(), "Same replacement dispatched twice");
	std::ofstream{ root / "data/item.json" } << "{";
	check(watch.Poll().empty(), "Partial JSON dispatched");
	std::ofstream{ root / "data/item.json" } << "{\"value\":23}";
	check(watch.Poll().size() == 1, "Completed external write missed");
	std::ofstream{ root / ".vega/snapshot.json" } << "{}";
	std::ofstream{ root / "main.scene.json" } << "{}";
	std::ofstream{ root / "vega.project.json" } << "{}";
	check(watch.Poll().empty(), "Metadata or scenes dispatched as runtime content");
	std::ofstream{ root / "new.prefab.json" } << "{}";
	check(watch.Poll().size() == 1, "New prefab missed");
	std::filesystem::remove(root / "new.prefab.json");
	check(watch.Poll().empty(), "Deletion must retain last valid definitions");
	const auto unicode = std::filesystem::path{ L"\u6f22\u5b57-\U0001f680" };
	std::ofstream{ root / (unicode.native() + std::filesystem::path{ ".png" }.native()) } << "image";
	std::ofstream{ root / (unicode.native() + std::filesystem::path{ ".json" }.native()) } << "{}";
	watch.Begin(root);
	check(watch.Poll().empty(), "Unicode names must be enumerated without repeated changes");
	bool failStamp = false;
	ContentWatch errors{ [&](const auto& entry, auto& time, auto& size, auto& status)
	{
		if (failStamp && entry.path().filename() == "blocked.json") { status = std::make_error_code(std::errc::permission_denied); return; }
		time = entry.last_write_time(status);
		if (!status) size = entry.file_size(status);
	} };
	errors.Begin(root);
	std::ofstream{ root / "blocked.json" } << "{}";
	std::ofstream{ root / "good.json" } << "{}";
	failStamp = true;
	ly::debug::LogCapture capture;
	check(errors.Poll().size() == 1, "A bad entry must not hide a readable changed file");
	check(errors.Poll().empty() && errors.Poll().empty(), "Persistent stamp errors must not repeat successful changes");
	check(capture.Buffer().Snapshot().size() == 1, "Persistent errors must be logged once");
	failStamp = false;
	check(errors.Poll().size() == 1 && errors.Poll().empty(), "Recovered unreadable entry must dispatch once");
	std::filesystem::remove(root / "blocked.json");
	std::filesystem::remove(root / "good.json");
	return failures ? 1 : 0;
}
