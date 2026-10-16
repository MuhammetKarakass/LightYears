#include "editor/AutosaveStore.h"
#include <fstream>
#include <iostream>

namespace
{
	int failures = 0;
	void Check(bool value, const char* message)
	{
		if (!value) { ++failures; std::cerr << message << '\n'; }
	}
	std::string Read(const std::filesystem::path& path)
	{
		std::ifstream input{ path, std::ios::binary };
		return { std::istreambuf_iterator<char>{ input }, {} };
	}
}

int main()
{
	using namespace vega;
	const auto root = std::filesystem::path{ VEGA_AUTOSAVE_TEST_ROOT };
	std::filesystem::create_directories(root / "data");
	const auto original = root / "data/test.json";
	std::ofstream{ original, std::ios::binary } << "{\"value\":1}\n";
	{
		const auto unicode = std::filesystem::path{ L"data/\u6f22\u5b57-\U0001f680.json" };
		std::ofstream{ root / unicode } << "{}";
		AutosaveStore unicodeStore;
		Check(unicodeStore.Begin(root) && unicodeStore.Write(unicode, "json", "{\"value\":2}"), "Unicode autosave must write without throwing");
		const auto entries = unicodeStore.Entries();
		Check(entries.size() == 1 && entries.front().source == unicode, "Unicode autosave source must round trip");
		for (const auto& entry : entries) Check(unicodeStore.Discard(entry), "Unicode snapshot discard failed");
	}
	std::filesystem::path crashed;
	{
		AutosaveStore first, second;
		Check(first.Begin(root) && second.Begin(root), "Concurrent sessions must use different directories");
		Check(first.Write("data/test.json", "json", "{\"value\":2}\n"), "Snapshot write failed");
		Check(second.Entries(true).empty() && second.Entries().empty(), "A live editor must not be offered for recovery");
		Check(first.Write("data/test.json", "json", "{\"value\":3}\n"), "Snapshot replacement failed");
		Check(first.Entries().size() == 1, "Repeated autosave must replace only that session's same document");
		Check(!first.Write("../outside.json", "json", "{}"), "Escaping source accepted");
		Check(!first.Write(".vega/secret.json", "json", "{}"), "Internal metadata accepted as a source");
		Check(!first.Write("data/test.json", "json", "{"), "Malformed JSON accepted");
		Check(Read(original) == "{\"value\":1}\n", "Autosave changed the user's file");
		crashed = first.Session();
		first.SimulateUncleanExit();
	}
	AutosaveStore next;
	Check(next.Begin(root), "Next session failed");
	auto recovery = next.Entries(true);
	Check(recovery.size() == 1 && recovery.front().source == "data/test.json" && recovery.front().unclean, "Unclean recovery entry missing");
	if (!recovery.empty())
	{
		Check(recovery.front().summary.find("replace /value") != std::string::npos, "Diff summary must identify the changed property");
		JsonDocument document{ root };
		Check(document.Recover(recovery.front().source, recovery.front().text) && document.Dirty(), "Recovery must create a dirty buffer");
		Check(Read(original) == "{\"value\":1}\n" && document.Data().at("value") == 3, "Recovery overwrote the original");
		std::ofstream{ original } << "{\"value\":4}\n";
		Check(!document.Save(), "Recovery save must refuse a concurrent disk edit");
		Check(document.Recover("data/test.json", recovery.front().text) && document.Save(), "Explicit save of recovered data failed");
		Check(next.Entries(true).empty(), "Already saved data must not prompt after a crash");
		Check(next.Entries().size() == 1, "Manual recovery must retain saved snapshots");
		Check(next.Discard(recovery.front()) && next.Entries().empty(), "Discard must remove only the selected snapshot");
	}
	{
		std::ofstream{ crashed / "snapshot-corrupt.json" } << "{";
		std::ofstream{ crashed / "snapshot-escape.json" } << Json{ { "format", "vega.autosave" }, { "version", 1 }, { "source", "../escape.json" }, { "kind", "json" }, { "text", "{}" } };
		Check(next.Entries().empty(), "Damaged and escaping envelopes must be ignored");
	}
	{
		JsonDocument document{ root };
		Check(document.Recover("data/missing.json", "{\"value\":9}\n") && document.Dirty(), "Missing source recovery failed");
		Check(!std::filesystem::exists(root / "data/missing.json"), "Recover created an authored file before Save");
		Check(document.Save() && Read(root / "data/missing.json") == "{\"value\":9}\n", "Explicit Save of missing source failed");
		Check(!document.Recover("../escape.json", "{}") && !document.Recover("data/test.json", "{\"x\":1,\"x\":2}"), "Unsafe recovery JSON/path accepted");
	}
	std::filesystem::remove(root / "data/missing.json");
	return failures ? 1 : 0;
}
