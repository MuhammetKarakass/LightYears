#include "editor/JsonDocument.h"
#include "editor/JsonCommands.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace
{
	int failures = 0;
	void Check(bool condition, const std::string& message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}
	std::string Read(const std::filesystem::path& path)
	{
		std::ifstream input{ path, std::ios::binary };
		return { std::istreambuf_iterator<char>{ input }, {} };
	}
	void Write(const std::filesystem::path& path, const std::string& text)
	{
		std::ofstream output{ path, std::ios::binary };
		output << text;
		Check(static_cast<bool>(output), "Fixture write failed");
	}
}

int main()
{
	using vega::Json;
	using vega::JsonDocument;
	const auto spaces = JsonDocument::DetectStyle("{\r\n    \"x\": 1\r\n}\r\n");
	Check(spaces.indent == 4 && spaces.indentChar == ' ' && spaces.newline == "\r\n" && spaces.trailingNewline, "CRLF/space style detection failed");
	const auto tabs = JsonDocument::DetectStyle("{\n\t\"x\": 1\n}");
	Check(tabs.indent == 1 && tabs.indentChar == '\t' && tabs.newline == "\n" && !tabs.trailingNewline, "LF/tab style detection failed");
	Check(!JsonDocument::DetectStyle("{}").trailingNewline, "Compact trailing newline detection failed");

	// These tests only read the source data. Every write goes to the build-tree fixture root.
	JsonDocument content{ VEGA_TEST_DATA_DIR };
	std::size_t dataFiles = 0;
	for (const auto& entry : std::filesystem::directory_iterator{ VEGA_TEST_DATA_DIR })
	{
		if (entry.path().extension() != ".json") continue;
		++dataFiles;
		const auto original = Read(entry.path());
		const bool opened = content.Open(entry.path().filename());
		Check(opened, "Cannot open data file: " + entry.path().string());
		Check(opened && content.Serialize() == original && !content.Dirty(), "Round trip failed: " + entry.path().string());
		// The layout-preserving serializer supports all current scalar-only data layouts.
		Check(!content.ReadOnly(), "Unexpected read-only data: " + entry.path().filename().string() + " " + content.ReadOnlyReason());
		std::cout << entry.path().filename().string() << ": " << (content.ReadOnly() ? content.ReadOnlyReason() : "byte-exact round trip (editable)") << '\n';
	}
	Check(dataFiles > 0, "No data files tested");

	const auto root = std::filesystem::path{ VEGA_TEST_TEMP_DIR } /
		std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
	std::filesystem::create_directories(root);
	const std::string original = "{\r\n\t\"i\": -2, \"u\": 2, \"f\": 0.70,\r\n\t\"b\": true, \"s\": \"hello\", \"a/b~c\": [1, 2], \"null\": null\r\n}\r\n";
	Write(root / "test.json", original);
	JsonDocument document{ root };
	Check(document.Open("test.json") && !document.ReadOnly() && document.Serialize() == original, "Mixed layout round trip failed");
	Check(document.Style().indentChar == '\t' && document.Style().newline == "\r\n", "Document style lost");
	Check(document.Get("/a~1b~0c/1") && *document.Get("/a~1b~0c/1") == 2, "Escaped array pointer failed");
	Check(!document.Get("/missing") && !document.Get("invalid") && !document.Get("/a~1b~0c/99"), "Invalid pointer accepted");
	Check(!document.Set("/missing", 2) && !document.Set("/a~1b~0c/-", 2), "Adding nodes must be rejected");
	Check(!document.Set("/null", 2) && !document.Set("/a~1b~0c", Json::array({ 3 })), "Retyping null/container accepted");
	Check(!document.Set("/i", 2.0) && !document.Set("/f", 2) && !document.Set("/b", "true"), "Scalar retyping accepted");
	Check(!document.Set("/u", -1) && !document.Set("/i", UINT64_MAX), "Out of range integer accepted");
	Check(document.Set("/i", 3u) && document.Get("/i")->type() == Json::value_t::number_integer, "Signed integer type not preserved");
	Check(document.Set("/u", 3) && document.Get("/u")->type() == Json::value_t::number_unsigned, "Unsigned integer type not preserved");
	Check(document.Set("/f", 1.5) && document.Get("/f")->is_number_float(), "Float type not preserved");
	Check(document.Set("/b", false) && document.Set("/s", "new\ntext") && document.Set("/a~1b~0c/1", 5), "Scalar pointer edits failed");
	const auto edited = document.Serialize();
	Check(edited.find("\r\n\t") != std::string::npos && edited.substr(edited.size() - 2) == "\r\n", "Edit changed whitespace style");
	Check(Json::parse(edited).at("s") == "new\ntext", "String escaping failed");
	Check(document.Dirty() && document.Save() && !document.Dirty(), "Atomic save or saved dirty state failed");
	Check(Read(root / "test.json") == edited, "Saved bytes differ");
	Check(document.Set("/f", 3.0) && document.Revert() && *document.Get("/f") == 1.5 && !document.Dirty(), "Revert failed");

	vega::EditorSelection selection;
	vega::EditorContext context{ nullptr, selection };
	context.document = &document;
	vega::CommandHistory history{ context };
	const auto group = history.BeginMergeGroup();
	const Json before = *document.Get("/f");
	Check(history.Execute(std::make_unique<vega::SetJsonValueCommand>(document.ID(), "/f", before, 2.0, group)), "JSON history edit failed");
	Check(history.Execute(std::make_unique<vega::SetJsonValueCommand>(document.ID(), "/f", 2.0, 3.0, group)), "JSON history merge failed");
	Check(history.Undo() && *document.Get("/f") == before && !document.Dirty() && !history.CanUndo(), "Merged undo must return to saved text");
	Check(history.Redo() && *document.Get("/f") == 3.0 && document.Dirty(), "JSON redo failed");
	Check(document.Save() && history.Undo() && document.Dirty(), "Undo across save must be dirty");
	Check(history.Redo() && !document.Dirty(), "Redo to saved text must clear dirty");
	const auto oldID = document.ID();
	Check(document.Revert() && document.ID() != oldID, "Reload must invalidate document ID");
	history.PruneInvalidCommands();
	Check(!history.CanUndo() && !history.CanRedo(), "Reload must prune stale document history");
	vega::SetJsonValueCommand stale{ oldID, "/f", 3.0, 8.0 };
	Check(!stale.Execute(context), "Old document command must fail");
	stale.Undo(context);
	Check(*document.Get("/f") == 3.0, "Stale undo changed reloaded document");

#ifdef _WIN32
	// Refusing replacement must leave the original intact and clean up the sibling temp file.
	HANDLE locked = CreateFileW((root / "test.json").c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	Check(locked != INVALID_HANDLE_VALUE, "Cannot lock save failure fixture");
	Check(document.Set("/f", 4.0) && !document.Save() && document.Dirty(), "Locked target save should fail");
	CloseHandle(locked);
	Check(Json::parse(Read(root / "test.json")).at("f") == 3.0, "Failed save changed original");
#endif
	for (const auto& entry : std::filesystem::directory_iterator{ root })
		Check(entry.path().extension() != ".tmp", "Save leaked a temporary file");

	std::filesystem::path resolved;
	Check(!document.Resolve("../escape.json", resolved), "Parent escape accepted");
	Check(!document.Resolve(root / "test.json", resolved), "Absolute path accepted");
	Check(!document.Resolve("../../json-test-assets-other/file.json", resolved), "Sibling prefix escape accepted");
	Check(document.Resolve("nested/../test.json", resolved) && resolved == std::filesystem::canonical(root / "test.json"), "Contained canonical path rejected");
	Check(!document.Open("../escape.json"), "Open accepted root escape");
	std::error_code linkError;
	std::filesystem::create_directory_symlink(root.parent_path(), root / "escape-link", linkError);
	if (!linkError) Check(!document.Resolve("escape-link/outside.json", resolved), "Symlink root escape accepted");
	else std::cout << "Symlink fixture unavailable: " << linkError.message() << '\n';
	if (!linkError)
	{
		const auto restrictedRoot = root / "restricted";
		std::filesystem::create_directory(restrictedRoot);
		Write(restrictedRoot / "target.json", "{\"x\":1}");
		JsonDocument restricted{ restrictedRoot };
		Check(restricted.Open("target.json") && restricted.Set("/x", 2), "Save escape fixture failed");
		const auto outsideBytes = Read(root / "test.json");
		std::filesystem::remove(restrictedRoot / "target.json");
		std::filesystem::create_symlink(root / "test.json", restrictedRoot / "target.json", linkError);
		if (!linkError)
		{
			Check(!restricted.Save(), "Save accepted a changed symlink outside its root");
			Check(Read(root / "test.json") == outsideBytes, "Escaping save changed the external target");
		}
	}

	Write(root / "duplicate.json", "{\"x\":1,\"x\":2}");
	Check(document.Open("duplicate.json") && document.ReadOnly() && !document.ReadOnlyReason().empty(), "Ambiguous layout must open read-only");
	Check(!document.Set("/x", 3) && !document.Save() && Read(root / "duplicate.json") == "{\"x\":1,\"x\":2}", "Read-only document was changed");
	Write(root / "invalid.json", "{");
	Check(!document.Open("invalid.json") && document.Path().filename() == "duplicate.json", "Failed open must preserve current document");
	Write(root / "bom.json", std::string{ "\xEF\xBB\xBF" } + "{\"x\":1}");
	Check(document.Open("bom.json") && document.ReadOnly(), "Unsupported BOM layout must open read-only");
	Write(root / "escaped.json", "{\"unicode\":\"\\u0041\",\"number\":1e-3}");
	Check(document.Open("escaped.json") && !document.ReadOnly() && !document.Dirty(), "Escaped lexical spellings should round trip");
	Check(document.Set("/unicode", "B") && document.Set("/unicode", "A") && !document.Dirty(), "Undo equivalent string must restore original escape spelling");
	document.Close();
	Check(!document.IsOpen() && !document.Dirty() && !document.Save(), "Close state failed");
	// The root is a unique child of the compile-time build-tree test directory.
	if (root.parent_path() == std::filesystem::path{ VEGA_TEST_TEMP_DIR }) std::filesystem::remove_all(root);
	std::cout << "JSON document tests: " << failures << " failures\n";
	return failures ? 1 : 0;
}
