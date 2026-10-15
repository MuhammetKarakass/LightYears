#include "editor/EditorScript.h"
#include <iostream>

namespace
{
	int failures = 0;
	void Check(bool condition, const std::string& message)
	{
		if (!condition)
		{
			std::cerr << message << '\n';
			++failures;
		}
	}
}

int main()
{
	using vega::ParseEditorScriptLine;
	using vega::ScriptOperation;
	const std::pair<const char*, ScriptOperation> cases[]{
		{ "create-prefab prefabs/BigRock.prefab.json", ScriptOperation::CreatePrefab },
		{ "place-prefab prefabs/BigRock.prefab.json", ScriptOperation::PlacePrefab },
		{ "place-prefab prefabs/BigRock.prefab.json 800 400", ScriptOperation::PlacePrefab },
		{ "create-variant prefabs/Rock.prefab.json prefabs/BigRock.prefab.json", ScriptOperation::CreateVariant },
		{ "apply-prefab-prop HealthComponent \"Max Health\"", ScriptOperation::ApplyPrefabProp },
		{ "revert-prefab-prop Actor Velocity", ScriptOperation::RevertPrefabProp },
		{ "expect-prefab none", ScriptOperation::ExpectPrefab },
		{ "expect-overridden Actor Velocity true", ScriptOperation::ExpectOverridden },
		{ "reload-prefabs", ScriptOperation::ReloadPrefabs },
		{ "open-prefab prefabs/BigRock.prefab.json", ScriptOperation::OpenPrefab },
		{ "save-prefab", ScriptOperation::SavePrefab },
		{ "close-prefab", ScriptOperation::ClosePrefab },
		{ "set-project-startup scenes/start.scene.json", ScriptOperation::SetProjectStartup },
		{ "set-project-startup \"\"", ScriptOperation::SetProjectStartup },
		{ "expect-project-startup \"\"", ScriptOperation::ExpectProjectStartup },
		{ "expect-project-startup scenes/start.scene.json", ScriptOperation::ExpectProjectStartup },
		{ "new-scene", ScriptOperation::NewScene }, { "open-scene scenes/sandbox.scene.json", ScriptOperation::OpenScene },
		{ "save-scene \"D:/build/temp scene.scene.json\"", ScriptOperation::SaveScene },
		{ "create Asteroid", ScriptOperation::Create }, { "delete", ScriptOperation::Delete }, { "duplicate", ScriptOperation::Duplicate },
		{ "create-many Asteroid 400", ScriptOperation::CreateMany },
		{ "play-mode", ScriptOperation::PlayMode }, { "stop", ScriptOperation::Stop },
		{ "expect-mode edit", ScriptOperation::ExpectMode }, { "expect-mode play", ScriptOperation::ExpectMode },
		{ "expect-entity-count 7", ScriptOperation::ExpectEntityCount }, { "expect-scene-dirty false", ScriptOperation::ExpectSceneDirty },
		{ "tool move", ScriptOperation::Tool }, { "tool rotate", ScriptOperation::Tool },
		{ "snap on", ScriptOperation::Snap }, { "snap off", ScriptOperation::Snap },
		{ "grid on", ScriptOperation::Grid }, { "grid off", ScriptOperation::Grid },
		{ "colliders on", ScriptOperation::Colliders }, { "colliders off", ScriptOperation::Colliders },
		{ "content-view grid", ScriptOperation::ContentView }, { "content-view list", ScriptOperation::ContentView },
		{ "add-script scripts/spin.lua", ScriptOperation::AddScript },
		{ "expect-script-error none", ScriptOperation::ExpectScriptError },
		{ "expect-prop-error \"Cannot load texture\"", ScriptOperation::ExpectPropError },
		{ "expect-script-error \"spin.lua:4\"", ScriptOperation::ExpectScriptError },
		{ "wait-seconds 0.5", ScriptOperation::WaitSeconds },
		{ "remember-rotation", ScriptOperation::RememberRotation },
		{ "expect-rotation-changed", ScriptOperation::ExpectRotationChanged },
		{ "camera editor", ScriptOperation::Camera }, { "camera game", ScriptOperation::Camera },
		{ "focus", ScriptOperation::Focus }, { "zoom 2", ScriptOperation::Zoom },
		{ "pan -500 0", ScriptOperation::Pan }, { "expect-visible", ScriptOperation::ExpectVisible },
		{ "expect-camera editor", ScriptOperation::ExpectCamera }, { "expect-camera game", ScriptOperation::ExpectCamera },
		{ "pause", ScriptOperation::Pause }, { "play", ScriptOperation::Play },
		{ "wait 12", ScriptOperation::Wait }, { "select 11", ScriptOperation::Select },
		{ "select-type PlayerSpaceShip", ScriptOperation::SelectType },
		{ "select-type Actor 2", ScriptOperation::SelectType },
		{ "move -2.5 300", ScriptOperation::Move }, { "rotate 90", ScriptOperation::Rotate },
		{ "undo", ScriptOperation::Undo }, { "redo", ScriptOperation::Redo },
		{ "expect-selected PlayerSpaceShip", ScriptOperation::ExpectSelected },
		{ "expect-location 400 300", ScriptOperation::ExpectLocation },
		{ "expect-location 400 300 0.01", ScriptOperation::ExpectLocation },
		{ "expect-count Actor 3", ScriptOperation::ExpectCount },
		{ "capture subdir/smoke.png", ScriptOperation::Capture }, { "quit", ScriptOperation::Quit },
		{ "remember-location", ScriptOperation::RememberLocation },
		{ "expect-remembered-location", ScriptOperation::ExpectRememberedLocation },
		{ "open-json content/data/ships.json", ScriptOperation::OpenJson },
		{ "json-set /ships/0/health 280.0", ScriptOperation::JsonSet },
		{ "expect-json /flag true", ScriptOperation::ExpectJson },
		{ "expect-json-dirty false", ScriptOperation::ExpectJsonDirty },
		{ "json-save", ScriptOperation::JsonSave }, { "json-revert", ScriptOperation::JsonRevert },
		{ "export out/LightYears", ScriptOperation::Export },
		{ "export \"path with spaces/LightYears\" overwrite", ScriptOperation::Export },
		{ "expect-export-ok", ScriptOperation::ExpectExportOk }
	};
	for (const auto& test : cases)
	{
		const auto parsed = ParseEditorScriptLine(test.first);
		Check(parsed.command && parsed.error.empty() && parsed.command->operation == test.second, std::string{ "Parse failed: " } + test.first);
		Check(!ParseEditorScriptLine(std::string{ test.first } + " extra").error.empty(), std::string{ "Extra argument accepted: " } + test.first);
		const auto commented = ParseEditorScriptLine(std::string{ " \t" } + test.first + " # trailing comment\r");
		Check(commented.command && commented.command->operation == test.second, "Comment or whitespace changed command");
	}
	for (const char* line : { "", "   \t\r", "# comment", "  #comment" })
	{
		Check(ParseEditorScriptLine(line).IsIgnored(), "Blank/comment must be ignored");
	}
	for (const char* line : { "unknown", "Pause", "select", "select -1", "select 4294967296", "select 2x",
		"create-many Asteroid", "create-many Asteroid -1", "create-many Asteroid 1x", "create-many Asteroid 10001",
		"tool", "tool scale", "tool Move", "snap", "snap true", "grid", "grid 1", "colliders", "colliders yes", "content-view", "content-view tiles",
		"camera", "camera other", "expect-camera", "expect-camera other", "focus 1", "expect-visible 1",
		"zoom", "zoom 0", "zoom -1", "zoom nan", "zoom inf", "zoom 1x", "zoom 1e999",
		"pan", "pan 1", "pan 1 nan", "pan inf 0", "pan 1x 2",
		"select-type", "select-type Actor -1", "wait", "wait -2", "wait 1.5", "move", "move 1", "move 1 abc",
		"move nan 2", "move inf 2", "move 1e999 2", "move 1x 2", "rotate", "rotate bad",
		"expect-selected", "expect-location 1", "expect-location 1 2 -1", "expect-location 1 2 nope",
		"expect-count Actor", "expect-count Actor -1", "capture", "remember-location 1", "quit now" })
	{
		const auto parsed = ParseEditorScriptLine(line);
		Check(!parsed.command && !parsed.error.empty(), std::string{ "Bad command accepted: " } + line);
	}
	Check(ParseEditorScriptLine("select 0").command->number == 0, "ID zero must be valid");
	Check(ParseEditorScriptLine("save-scene").command->operation == ScriptOperation::SaveScene, "Save current scene must parse");
	Check(ParseEditorScriptLine("rename Rock A # comment").command->text == "Rock A", "Rename must join the rest of the line");
	Check(ParseEditorScriptLine("select-name \"Rock # A\"").command->text == "Rock # A", "Quoted entity name lost");
	for (const char* line : { "rename", "rename \"\"", "select-name", "select-name \"\"", "open-scene", "save-scene \"\"",
		"create", "expect-mode", "expect-mode paused", "expect-entity-count -1", "expect-entity-count 1.5", "expect-scene-dirty 1" })
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{ "Bad authoring command accepted: " } + line);
	Check(ParseEditorScriptLine("zoom 2.5").command->factor == 2.5f, "Zoom factor lost");
	Check(ParseEditorScriptLine("pan -500 20").command->location == sf::Vector2f{ -500.f, 20.f }, "Pan values lost");
	Check(ParseEditorScriptLine("camera editor").command->text == "editor", "Camera mode lost");
	Check(ParseEditorScriptLine("wait 12").command->number == 12, "Frame count lost");
	Check(ParseEditorScriptLine("select-type Actor").command->number == 0, "Default type index must be zero");
	Check(ParseEditorScriptLine("select-type Actor 2").command->number == 2, "Type index lost");
	Check(ParseEditorScriptLine("select-type Actor").command->text == "Actor", "Type name lost");
	Check(ParseEditorScriptLine("move -2.5 300").command->location == sf::Vector2f{ -2.5f, 300.f }, "Move values lost");
	Check(ParseEditorScriptLine("rotate -90").command->degrees == -90.f, "Rotation lost");
	Check(ParseEditorScriptLine("expect-location 1 2").command->tolerance == 0.5f, "Default tolerance incorrect");
	Check(ParseEditorScriptLine("expect-location 1 2 0.01").command->tolerance == 0.01f, "Explicit tolerance lost");
	Check(ParseEditorScriptLine("expect-count Actor 3").command->number == 3, "Expected count lost");
	Check(ParseEditorScriptLine("capture subdir/smoke.png").command->text == "subdir/smoke.png", "Capture path lost");
	const auto property = ParseEditorScriptLine("set-prop HealthComponent \"Max Health\" 250");
	Check(property.command && property.command->operation == ScriptOperation::SetProp &&
		property.command->text == "HealthComponent" && property.command->property == "Max Health" &&
		property.command->values == std::vector<std::string>{ "250" }, "Quoted property arguments lost");
	const auto expect = ParseEditorScriptLine("expect-prop Actor \"Location\" 1 2 0.01 # comment");
	Check(expect.command && expect.command->operation == ScriptOperation::ExpectProp &&
		expect.command->values == std::vector<std::string>{ "1", "2", "0.01" }, "Property expectation values lost");
	const auto hash = ParseEditorScriptLine("set-prop Actor \"Text # label\" \"a \\\"quote\\\" and \\\\ slash\"");
	Check(hash.command && hash.command->property == "Text # label" &&
		hash.command->values[0] == "a \"quote\" and \\ slash", "Quoted hashes or escapes lost");
	const auto empty = ParseEditorScriptLine("set-prop Actor \"Text\" \"\"");
	Check(empty.command && empty.command->values == std::vector<std::string>{ "" }, "Empty quoted string lost");
	Check(ParseEditorScriptLine("capture \"path with spaces/image.png\"").command->text == "path with spaces/image.png", "Quoted path lost");
	for (const char* line : { "set-prop", "set-prop Actor", "set-prop Actor \"Rotation\"", "expect-prop Actor \"Rotation\"",
		"set-prop \"\" \"Rotation\" 1", "set-prop Actor \"\" 1", "set-prop Actor \"Rotation 1", "capture \"path\"suffix", "capture ab\"cd" })
	{
		const auto parsed = ParseEditorScriptLine(line);
		Check(!parsed.command && !parsed.error.empty(), std::string{ "Bad quoted/property command accepted: " } + line);
	}
	const auto jsonString = ParseEditorScriptLine("json-set /name \"a # hash, \\\"quote\\\" and \\n newline\" # comment");
	Check(jsonString.command && jsonString.command->values[0] == "\"a # hash, \\\"quote\\\" and \\n newline\"", "JSON literal string quotes/escapes lost");
	Check(ParseEditorScriptLine("json-set \"\" false").command.has_value(), "Empty JSON root pointer must parse");
	Check(ParseEditorScriptLine("expect-json /a~1b~0c/0 -2").command.has_value(), "Escaped JSON pointer failed");
	Check(ParseEditorScriptLine("json-set /name \"\"").command.has_value(), "Empty JSON string failed");
	for (const char* line : { "open-json", "open-json \"\"", "json-save extra", "json-revert extra", "json-set /a",
		"json-set invalid 1", "json-set /bad~2escape 1", "json-set /a null", "json-set /a []", "json-set /a {}",
		"json-set /a nan", "json-set /a 1e999", "json-set /a unquoted", "json-set /a 01", "json-set /a 1 extra",
		"expect-json /a", "expect-json /a true extra", "expect-json-dirty", "expect-json-dirty 1", "expect-json-dirty true extra",
		"export", "export \"\"", "export out/LightYears maybe", "export out/LightYears overwrite extra",
		"export out/LightYears overwrite overwrite", "expect-export-ok 1" })
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{ "Invalid JSON command accepted: " } + line);
	Check(ParseEditorScriptLine("export \"out dir\" overwrite").command->text == "out dir", "Export path lost");
	for (const char* line : { "set-script-prop speed 90", "set-script-prop enabled true", "set-script-prop offset 1 2",
		"set-script-prop color \"#ff8800\"", "expect-script-prop speed 90 0.01", "expect-script-prop offset 1 2 0.1" })
	{
		const auto parsed = ParseEditorScriptLine(line);
		Check(parsed.command && parsed.error.empty() && !parsed.command->property.empty() && !parsed.command->values.empty(), "Script property arguments lost");
	}
	const auto scriptString = ParseEditorScriptLine("set-script-prop label \"\"");
	Check(scriptString.command && scriptString.command->values == std::vector<std::string>{ "" }, "Empty script string lost");
	for (const char* line : { "add-script", "add-script \"\"", "set-script-prop", "set-script-prop speed", "set-script-prop \"\" 1",
		"expect-script-prop speed", "expect-script-error", "expect-script-error \"\"", "wait-seconds", "wait-seconds -1",
		"wait-seconds nan", "wait-seconds inf", "wait-seconds 1x", "wait-seconds 1e999", "wait-seconds 86401" })
		Check(!ParseEditorScriptLine(line).error.empty(), "Invalid scripting command accepted");
	Check(ParseEditorScriptLine("export \"out dir\" overwrite").command->overwrite, "Export overwrite flag lost");
	Check(!ParseEditorScriptLine("export out").command->overwrite, "Export must not overwrite by default");
	Check(ParseEditorScriptLine("add-component HealthComponent").command->operation == ScriptOperation::AddComponent, "Add component parsing failed");
	Check(ParseEditorScriptLine("remove-component SpriteComponent").command->number == 0, "Default component occurrence must be zero");
	Check(ParseEditorScriptLine("remove-component SpriteComponent 2").command->number == 2, "Component occurrence lost");
	Check(ParseEditorScriptLine("expect-component HealthComponent present").command->property == "present", "Component presence lost");
	Check(ParseEditorScriptLine("expect-component HealthComponent absent").command->property == "absent", "Component absence lost");
	for (const char* line : { "add-component", "add-component \"\"", "add-component HealthComponent extra", "remove-component", "remove-component X -1",
		"remove-component X 1x", "remove-component X 4294967296", "remove-component X 0 extra", "expect-component X", "expect-component X true", "expect-component X absent extra" })
		Check(!ParseEditorScriptLine(line).error.empty(), "Invalid component command accepted");
	if (failures) return 1;
	for (const char* line : { "create-prefab", "create-prefab \"\"", "create-prefab p extra", "place-prefab", "place-prefab p 1",
		"place-prefab p nan 2", "place-prefab p 1 2 extra", "apply-prefab-prop Actor", "revert-prefab-prop Actor P extra",
		"expect-prefab", "expect-overridden Actor P yes", "expect-overridden Actor P", "reload-prefabs extra", "create-variant p" })
		Check(!ParseEditorScriptLine(line).error.empty(), "Invalid prefab command accepted");
	if (failures) return 1;
	std::cout << "Editor script parser tests passed\n";
	return 0;
}
