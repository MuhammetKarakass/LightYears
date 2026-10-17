#include "framework/EngineContext.h"
#include "editor/ContentWorkspace.h"
#include <fstream>
#include <iostream>

namespace
{
	int failures = 0, validations = 0;
	void Check(bool value, const char* message) { if (!value) { ++failures; std::cerr << message << '\n'; } }
	std::string Read(const std::filesystem::path& path)
	{
		std::ifstream input{ path, std::ios::binary };
		return { std::istreambuf_iterator<char>{ input }, {} };
	}
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	using namespace vega;
	using namespace vega::content;
	const auto root = std::filesystem::path{ VEGA_CONTENT_TEST_ROOT };
	std::filesystem::create_directories(root);
	const std::string original = "{\r\n\t\"effects\": [\r\n\t\t{\"id\":\"old\", \"duration\":2.00, \"opaque\":{\"id\":\"old\",\"spelling\":1e+03}},\r\n\t\t{\"id\":\"stable\", \"duration\":0.70}\r\n\t]\r\n}\r\n";
	{ std::ofstream output{ root / "effects.json", std::ios::binary }; output << original; }
	{ std::ofstream output{ root / "abilities.json", std::ios::binary }; output << "{\"abilities\":[{\"id\":\"ability\",\"effectId\":\"old\",\"description\":\"old\"}]}\n"; }
	auto& registry = ContentSchemaRegistry::Get();
	ContentTypeSchema effects;
	effects.type = "effect"; effects.file = "effects.json"; effects.collection = "effects";
	effects.fields = { { "/id", FieldKind::String, "" }, { "/duration", FieldKind::Number, 1.0 } };
	effects.templates["Timed"] = Json{ { "id", "template" }, { "duration", 2.0 } };
	effects.validateId = [](const auto& id, auto& error) { if (id.empty()) { error = "Empty ID"; return false; } return true; };
	effects.validate = [](const ContentDocuments& documents, const std::string& id)
	{
		++validations;
		for (const auto& value : documents.at("effect").at("effects")) if (value.at("id") == id && value.at("duration").get<double>() < 0.) return std::string{ "Negative duration from existing validator" };
		return std::string{};
	};
	registry.Register(std::move(effects));
	ContentTypeSchema abilities;
	abilities.type = "ability"; abilities.file = "abilities.json"; abilities.collection = "abilities";
	abilities.fields = { { "/id", FieldKind::String, "" }, { "/effectId", FieldKind::Reference, "", "", "", "", "effect" }, { "/description", FieldKind::String, "" } };
	registry.Register(std::move(abilities));
	EditorSelection selection;
	EditorContext context{ nullptr, selection };
	CommandHistory history{ context };
	ContentWorkspace workspace{ context.Engine() };
	vega::EngineContext otherContext;
	vega::EngineContext::Scope otherScope{ otherContext };
	Check(workspace.Load(root), "Workspace load failed");
	Check(workspace.Open("effect", "old"), "Cannot select effect");
	Check(workspace.Validate().empty(), "Initial validation failed");
	const auto validationCount = validations;
	workspace.Validate(); workspace.Entries(); workspace.References();
	Check(validations == validationCount, "Unchanged revision was revalidated");
	Check(workspace.References().size() == 1, "Opaque values must not become references");
	Check(!workspace.Set(history, "opaque.spelling", 3.0), "Unknown keys must be read-only");
	Check(!workspace.Delete(history, "effect", "old"), "Referenced definition was deleted");
	const auto undoCount = history.UndoCount();
	Check(workspace.Rename(history, "effect", "old", "renamed"), "Rename failed");
	Check(history.UndoCount() == undoCount + 1, "Rename must be one history step");
	Check(workspace.Definition("ability", "ability")->at("effectId") == "renamed", "Cross-file reference not renamed");
	Check(workspace.Definition("ability", "ability")->at("description") == "old", "Ordinary string was renamed");
	Check(workspace.Definition("effect", "renamed")->at("opaque").at("id") == "old", "Opaque key changed");
	Check(history.Undo() && workspace.Definition("effect", "old") && !workspace.Dirty(), "Rename undo did not restore original bytes");
	Check(workspace.selectedId == "old", "Rename undo did not restore form selection");
	Check(history.Redo() && workspace.Definition("effect", "renamed"), "Rename redo failed");
	Check(workspace.selectedId == "renamed", "Rename redo did not restore form selection");
	Check(workspace.New(history, "effect", "renamed", "copy"), "Duplicate failed");
	Check(workspace.Set(history, "duration", 3.5), "Duration edit failed");
	Check(workspace.New(history, "effect", "Timed", "new"), "Template creation failed");
	Check(!workspace.New(history, "effect", "Timed", "new"), "Duplicate ID accepted");
	Check(workspace.Open("effect", "new") && workspace.Set(history, "duration", -1.), "Invalid intermediate edit must remain undoable");
	Check(!workspace.Validate().empty() && !workspace.Save(), "Validator failure did not prevent save");
	const auto snapshots = workspace.DirtyDocuments();
	Check(snapshots.size() == 2, "Autosave must include every dirty catalog, even intermediate invalid edits");
	Check(!workspace.Recover("effects.json", original), "Recovery must refuse an existing dirty content buffer");
	Check(history.Undo() && workspace.Validate().empty(), "Undo did not clear validation error");
	Check(workspace.Save() && !workspace.Dirty(), "Save failed");
	const auto saved = Read(root / "effects.json");
	Check(saved.find("{\"id\":\"stable\", \"duration\":0.70}") != std::string::npos, "Unchanged definition bytes changed");
	Check(saved.find("\"spelling\":1e+03") != std::string::npos, "Opaque numeric lexeme changed");
	Check(saved.size() >= 2 && saved.substr(saved.size() - 2) == "\r\n", "Line endings changed");
	Check(workspace.Load(root), "Reopen failed");
	history.PruneInvalidCommands();
	Check(!history.CanUndo(), "Reload did not invalidate old history");
	Check(workspace.Open("effect", "copy") && workspace.Selected()->at("duration") == 3.5, "Saved values were lost");
	Check(workspace.Recover("effects.json", snapshots.at(std::filesystem::canonical(root / "effects.json"))), "Content recovery failed");
	Check(workspace.Dirty() && Read(root / "effects.json") == saved, "Content recovery must preserve the authored catalog");
	Check(!workspace.Validate().empty(), "Recovered invalid content must still be validated before Save");
	Check(workspace.Revert() && !workspace.Dirty(), "Recovered content revert failed");
	Check(workspace.Delete(history, "effect", "copy") && !workspace.Definition("effect", "copy"), "Unreferenced deletion failed");
	Check(history.Undo() && workspace.Definition("effect", "copy"), "Delete undo failed");
	Check(context.sceneRevision == 0, "Content edits dirtied the scene");
	JsonDocument first{ root }, second{ root };
	Check(first.Open("effects.json") && second.Open("effects.json"), "Concurrent fixture open failed");
	Check(first.Set("/effects/0/duration", 4.0) && first.Save(), "First writer failed");
	Check(second.Set("/effects/0/duration", 6.0) && !second.Save(), "Stale document overwrote a newer file");
	Check(Read(root / "effects.json").find("4.0") != std::string::npos, "Refused save changed the first writer's data");
	std::cout << "Content workspace tests: " << failures << " failures\n";
	return failures ? 1 : 0;
}
