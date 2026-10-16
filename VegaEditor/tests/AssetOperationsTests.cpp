#include "editor/AssetOperations.h"
#include <framework/content/ContentSchema.h>
#include <fstream>
#include <iostream>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace
{
	int failures = 0;
	void Check(bool value, const char* message) { if (!value) { ++failures; std::cerr << message << '\n'; } }
	void Write(const std::filesystem::path& path, const std::string& text) { std::ofstream{ path, std::ios::binary } << text; }
	std::string Read(const std::filesystem::path& path)
	{
		std::ifstream input{ path, std::ios::binary };
		return { std::istreambuf_iterator<char>{ input }, {} };
	}
}

int main()
{
	using namespace vega;
	const auto root = std::filesystem::path{ VEGA_ASSET_OPERATIONS_TEST_ROOT };
	if (!root.is_absolute() || root.filename() != "asset-operations-test") return 1;
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root / "scripts");
	std::filesystem::create_directories(root / "destination");
	Write(root / "scripts/start.lua", "return {}\n");
	const std::string original = "{\r\n\t\"scripts\":[{\"path\":\"scripts/start.lua\"}], \"opaque\":0.70, \"children\":[{\"prefab\":\"scripts/nested.prefab.json\"}]\r\n}\r\n";
	Write(root / "a.json", original);
	Write(root / "z.json", "{\"file\":\"scripts/start.lua\"}\n");
	Write(root / "scripts/nested.prefab.json", "{\"base\":{\"type\":\"Actor\"},\"scripts\":[{\"path\":\"scripts/start.lua\"}]}\n");
	AssetMovePlan move;
	Check(move.Prepare(root, "scripts", "destination/code") && move.References().size() == 4, "Move preview must include nested prefab/script references");
	Check(move.Apply() && !std::filesystem::exists(root / "scripts") && std::filesystem::exists(root / "destination/code/start.lua"), "Folder move failed");
	Check(Read(root / "a.json").find("0.70") != std::string::npos && Read(root / "a.json").find("destination/code/nested.prefab.json") != std::string::npos, "Move must preserve formatting and update nested paths");
	Write(root / "new.json", "{\"path\":\"destination/code/start.lua\"}");
	Check(!move.Apply(false), "Undo must refuse a new unreviewed reference file");
	std::filesystem::remove(root / "new.json");
	Check(move.Apply(false) && Read(root / "a.json") == original, "Undo must restore exact reference bytes and original paths");
	Check(move.Apply() && move.Apply(false), "Redo/undo failed");
	AssetMovePlan stale;
	Check(stale.Prepare(root, "scripts/start.lua", "scripts/renamed.lua"), "Rename preview failed");
	Write(root / "z.json", "{\"file\":\"scripts/start.lua\",\"new\":true}\n");
	Check(!stale.Apply() && std::filesystem::exists(root / "scripts/start.lua"), "Stale preview replaced an external edit");
	Write(root / "z.json", "{\"file\":\"scripts/start.lua\"}\n");
	for (const auto& path : { "../outside", ".vega/data", "C:/outside" })
		Check(!stale.Prepare(root, "scripts", path), "Unsafe destination accepted");
	Check(!stale.Prepare(root, "scripts", "scripts/inside") && !stale.Prepare(root, "scripts/start.lua", "a.json"), "Self move/overwrite accepted");
	Write(root / "bad.json", "{");
	Check(!stale.Prepare(root, "scripts", "destination/code"), "Malformed reference document must block a move");
	std::filesystem::remove(root / "bad.json");
#ifdef _WIN32
	Check(stale.Prepare(root, "scripts", "destination/code"), "Rollback preview failed");
	const auto lock = CreateFileW((root / "z.json").c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	Check(lock != INVALID_HANDLE_VALUE, "Cannot reserve rollback failure fixture");
	Check(!stale.Apply(), "Locked reference file should fail atomic replacement");
	if (lock != INVALID_HANDLE_VALUE) CloseHandle(lock);
	Check(std::filesystem::exists(root / "scripts/start.lua") && !std::filesystem::exists(root / "destination/code") && Read(root / "a.json") == original, "Failed move did not roll back the rename and earlier reference edits");
#endif
	const auto project = root.parent_path() / "external.project.json";
	Write(project, "{\"startupScene\":\"scripts/main.scene.json\"}");
	Check(stale.Prepare(root, "scripts", "destination/code", project), "External project preview failed");
	Check(stale.Apply() && Read(project).find("destination/code/main.scene.json") != std::string::npos && stale.Apply(false), "External project startup reference was not updated/restored");
	std::filesystem::remove(project);
	{
		const auto directory = root / ".vega/AssetTransactions";
		std::filesystem::create_directories(directory);
		const auto after = "{\"script\":\"destination/code/start.lua\"}\n";
		const Json journal{ { "format", "vega.asset-transaction" }, { "version", 1 }, { "from", "scripts" }, { "to", "destination/code" }, { "committed", false },
			{ "documents", Json::array({ Json{ { "beforePath", (root / "a.json").generic_string() }, { "afterPath", (root / "a.json").generic_string() }, { "before", original }, { "after", after } } }) } };
		std::filesystem::rename(root / "scripts", root / "destination/code");
		Write(root / "a.json", after);
		Write(directory / "pending.json", journal.dump());
		std::string error;
		Check(AssetMovePlan::RecoverPending(root, {}, error) && Read(root / "a.json") == original && std::filesystem::exists(root / "scripts/start.lua"), "Interrupted move must restore original references and file paths");
		std::filesystem::rename(root / "scripts", root / "destination/code");
		Write(root / "a.json", "{\"external\":true}");
		Write(directory / "pending.json", journal.dump());
		Check(!AssetMovePlan::RecoverPending(root, {}, error) && Read(root / "a.json") == "{\"external\":true}" && std::filesystem::exists(directory / "pending.json"), "Crash recovery must preserve conflicting external edits and its journal");
	}
	ly::content::ContentTypeSchema fixed;
	fixed.type = "fixed"; fixed.file = "destination/code/nested.prefab.json";
	ly::content::ContentSchemaRegistry::Get().Register(std::move(fixed));
	Check(!stale.Prepare(root, "destination/code", "scripts"), "Module-owned fixed catalog paths must not be moved");
	return failures ? 1 : 0;
}
