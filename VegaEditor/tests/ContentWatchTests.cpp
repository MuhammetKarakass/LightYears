#include "editor/ContentWatch.h"
#include "editor/JsonDocument.h"
#include <fstream>
#include <iostream>

int main()
{
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
	return failures ? 1 : 0;
}
