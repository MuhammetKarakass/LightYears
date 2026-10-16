#include "editor/EditorScript.h"
#include <iostream>
#include "editor/ScriptExecution.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <editor/SkyScriptCommands.h>
#include <gas/GasScriptCommands.h>
#include <LightYearsScriptCommands.h>


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

int main(int argc, char** argv)
{
	vega::ScriptCommandRegistry registry;
	vega::RegisterCoreScriptCommands(registry);
	const auto unused = [](vega::ScriptExecution&, const vega::EditorScriptCommand&) {};
	vega::RegisterSkyScriptCommands(registry, unused);
	vega::RegisterGasScriptCommands(registry, unused);
	const auto ParseEditorScriptLine = [&](const std::string& line) { return vega::ParseEditorScriptLine(line, registry); };
	for (const char* line : { "asset-move scripts moved/scripts", "asset-rename scripts/spin.lua rotate.lua", "asset-delete data/unused.json confirm", "expect-asset scripts/spin.lua true" })
		Check(ParseEditorScriptLine(line).command.has_value(), std::string{ "Valid asset command refused: " } + line);
	for (const char* line : { "asset-move one", "asset-rename a b extra", "asset-delete a", "asset-delete a true", "expect-asset a maybe", "expect-asset \"\" true" })
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{ "Invalid asset command accepted: " } + line);
	for (const char* line : { "autosave-now", "simulate-unclean-exit", "expect-autosave scenes/main.scene.json", "expect-autosave scenes/main.scene.json false", "expect-recovery scenes/main.scene.json true", "recover-autosave \"scenes/with spaces.scene.json\"" })
		Check(ParseEditorScriptLine(line).command.has_value(), std::string{ "Valid autosave command refused: " } + line);
	for (const char* line : { "autosave-now extra", "simulate-unclean-exit true", "expect-autosave", "expect-autosave \"\"", "expect-autosave a maybe", "recover-autosave a true", "expect-recovery a true extra" })
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{ "Invalid autosave command accepted: " } + line);
	for (const char* line : { "set-ref Actor Target Door", "expect-ref Trigger Target none", "list-add Actor Patrol 10 20", "list-add Actor Names \"a b\"", "list-set Actor Patrol 0 20 30", "list-remove Actor Patrol 0", "expect-list-size Actor Patrol 2" })
		Check(ParseEditorScriptLine(line).command.has_value(), std::string{ "Valid rich property command refused: " } + line);
	for (const char* line : { "set-ref Actor Target", "set-ref Actor Target none extra", "expect-ref Actor Target \"\"", "list-add Actor Patrol", "list-set Actor Patrol -1 10 20", "list-set Actor Patrol 0", "list-remove Actor Patrol -1", "list-remove Actor Patrol 0 extra", "expect-list-size Actor Patrol -1", "expect-list-size Actor Patrol 1.5" })
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{ "Invalid rich property command accepted: " } + line);
	for (const char* line : { "parent Child Root", "unparent Child", "expect-parent Child Root", "expect-parent Child none", "expect-world-location Child 10 -20 0.01" })
		Check(ParseEditorScriptLine(line).command.has_value(), std::string{ "Valid hierarchy command refused: " } + line);
	for (const char* line : { "parent Child", "parent Child Root extra", "unparent", "unparent Child Root", "expect-parent Child", "expect-world-location Child nan 2", "expect-world-location Child 1 2 -1", "expect-world-location Child 1" })
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{ "Invalid hierarchy command accepted: " } + line);
	{
		vega::ScriptCommandRegistry extensionRegistry;
		extensionRegistry.Add(vega::TokenScriptCommand("inspect-widget", vega::Schema("label:text count:uint"), "Test extension.", vega::ScriptMode::Any,
			[](vega::EditorContext&, vega::CommandHistory&, const std::vector<std::string>&, std::string&) { return true; }));
		const auto extension = vega::ParseEditorScriptLine("inspect-widget \"with spaces\" 12 # comment", extensionRegistry);
		Check(extension.command && extension.command->operation == "inspect-widget" &&
			extension.command->values == std::vector<std::string>{ "with spaces", "12" },
			"Registered extension must receive decoded arguments");
		Check(!ParseEditorScriptLine("inspect-widget value").error.empty(), "An unregistered extension must remain unknown");
	}
	for (const char* line : {"sky-open", "sky-open sol extra", "sky-save extra", "sky-set", "sky-set band.cool [1,2", "sky-set planet.at null", "sky-set band..cool 1", "sky-set band.cool {\"a\":1}", "sky-drag planet 1", "sky-drag bad 1 2", "sky-drag object.-1 0 0", "sky-drag object.0.spin 1 2", "sky-drag planet nan 0", "sky-drag planet 0 inf", "expect-sky band.cool", "capture-sky \"\""})
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{"Invalid sky command accepted: "} + line);
	for (const char* line : { "gas-filter unknown x", "gas-filter attributes", "gas-filter tags x extra", "gas-select-player extra", "gas-reset-cooldowns extra", "gas-apply-effect", "gas-remove-effect \"\"", "gas-activate", "gas-panel maybe", "expect-ability A blocked", "expect-effect E yes", "expect-tag T 1", "expect-attribute A != 1", "expect-attribute A > nan", "expect-attribute A > inf", "expect-attribute A > 1 extra", "expect-attribute A >" })
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{ "Invalid GAS command accepted: " } + line);
	const std::pair<const char*, std::string> cases[]{
		{"sky-open sol", "sky-open"}, {"sky-save", "sky-save"},
		{"sky-set band.cool [0.2, 0.5, 1.0]", "sky-set"},
		{"sky-set orbit.framing \"disk\"", "sky-set"},
		{"expect-sky objects[0].at [0.2,0.7]", "expect-sky"},
		{"capture-sky \"Saved/sky test.png\"", "capture-sky"},
		{"sky-drag planet -120 60", "sky-drag"},
		{"sky-drag object.0.rotate 15 0", "sky-drag"},
		{"sky-drag orbit.disk 0 30", "sky-drag"},
		{ "gas-select-player", "gas-select-player" },
		{ "gas-reset-cooldowns", "gas-reset-cooldowns" },
		{ "gas-apply-effect Effect.Buff", "gas-apply-effect" },
		{ "gas-remove-effect Effect.Buff", "gas-remove-effect" },
		{ "gas-activate Ability.Test", "gas-activate" },
		{ "gas-panel on", "gas-panel" },
		{ "gas-filter attributes MovementSlow", "gas-filter" },
		{ "gas-filter events \"\"", "gas-filter" },
		{ "expect-ability Ability.Test ready", "expect-ability" },
		{ "expect-ability Ability.Test cooldown", "expect-ability" },
		{ "expect-ability Ability.Test active", "expect-ability" },
		{ "expect-effect Effect.Buff true", "expect-effect" },
		{ "expect-tag State.Buff false", "expect-tag" },
		{ "expect-attribute Owner.Speed == 1", "expect-attribute" },
		{ "expect-attribute Owner.Speed < -1", "expect-attribute" },
		{ "expect-attribute Owner.Speed > 1.25", "expect-attribute" },
		{ "expect-attribute Owner.Speed <= 1", "expect-attribute" },
		{ "expect-attribute Owner.Speed >= 1", "expect-attribute" },
		{ "select-add \"Asteroid left\"", "select-add" },
		{ "select-remove 42", "select-remove" },
		{ "select-all", "select-all" },
		{ "select-rect -10 20 50 -70", "select-rect" },
		{ "expect-selection-count 3", "expect-selection-count" },
		{ "copy", "copy" }, { "paste", "paste" },
		{ "paste 100 -20", "paste" },
		{ "move-selection -10 25", "move-selection" },
		{ "set-prop-all \"Render Enabled\" false", "set-prop-all" },
		{ "expect-console Info \"Lua says hello\"", "expect-console" },
		{ "expect-console Warning \"missing prefab\"", "expect-console" },
		{ "expect-console Error \"bad.lua:3\"", "expect-console" },
		{ "expect-no-errors", "expect-no-errors" }, { "clear-console", "clear-console" },
		{ "profiler on", "profiler" }, { "profiler off", "profiler" },
		{ "diagnostics on", "diagnostics" }, { "diagnostics off", "diagnostics" },
		{ "expect-profile-scope \"World Tick\"", "expect-profile-scope" },
		{ "capture-profile \"Saved/profile test.json\"", "capture-profile" },
		{ "create-prefab prefabs/BigRock.prefab.json", "create-prefab" },
		{ "place-prefab prefabs/BigRock.prefab.json", "place-prefab" },
		{ "place-prefab prefabs/BigRock.prefab.json 800 400", "place-prefab" },
		{ "create-variant prefabs/Rock.prefab.json prefabs/BigRock.prefab.json", "create-variant" },
		{ "apply-prefab-prop HealthComponent \"Max Health\"", "apply-prefab-prop" },
		{ "revert-prefab-prop Actor Velocity", "revert-prefab-prop" },
		{ "expect-prefab none", "expect-prefab" },
		{ "expect-overridden Actor Velocity true", "expect-overridden" },
		{ "reload-prefabs", "reload-prefabs" },
		{ "open-prefab prefabs/BigRock.prefab.json", "open-prefab" },
		{ "save-prefab", "save-prefab" },
		{ "close-prefab", "close-prefab" },
		{ "set-project-startup scenes/start.scene.json", "set-project-startup" },
		{ "set-project-startup \"\"", "set-project-startup" },
		{ "expect-project-startup \"\"", "expect-project-startup" },
		{ "expect-project-startup scenes/start.scene.json", "expect-project-startup" },
		{ "new-scene", "new-scene" }, { "open-scene scenes/sandbox.scene.json", "open-scene" },
		{ "save-scene \"D:/build/temp scene.scene.json\"", "save-scene" },
		{ "create Asteroid", "create" }, { "delete", "delete" }, { "duplicate", "duplicate" },
		{ "create-many Asteroid 400", "create-many" },
		{ "play-mode", "play-mode" }, { "stop", "stop" },
		{ "expect-mode edit", "expect-mode" }, { "expect-mode play", "expect-mode" },
		{ "expect-entity-count 7", "expect-entity-count" }, { "expect-scene-dirty false", "expect-scene-dirty" },
		{ "tool move", "tool" }, { "tool rotate", "tool" },
		{ "snap on", "snap" }, { "snap off", "snap" },
		{ "grid on", "grid" }, { "grid off", "grid" },
		{ "colliders on", "colliders" }, { "colliders off", "colliders" },
		{ "content-view grid", "content-view" }, { "content-view list", "content-view" },
		{ "add-script scripts/spin.lua", "add-script" },
		{ "expect-script-error none", "expect-script-error" },
		{ "expect-prop-error \"Cannot load texture\"", "expect-prop-error" },
		{ "expect-script-error \"spin.lua:4\"", "expect-script-error" },
		{ "wait-seconds 0.5", "wait-seconds" },
		{ "remember-rotation", "remember-rotation" },
		{ "expect-rotation-changed", "expect-rotation-changed" },
		{ "camera editor", "camera" }, { "camera game", "camera" },
		{ "focus", "focus" }, { "zoom 2", "zoom" },
		{ "pan -500 0", "pan" }, { "expect-visible", "expect-visible" },
		{ "expect-camera editor", "expect-camera" }, { "expect-camera game", "expect-camera" },
		{ "pause", "pause" }, { "play", "play" },
		{ "wait 12", "wait" }, { "select 11", "select" },
		{ "select-type PlayerSpaceShip", "select-type" },
		{ "select-type Actor 2", "select-type" },
		{ "move -2.5 300", "move" }, { "rotate 90", "rotate" },
		{ "undo", "undo" }, { "redo", "redo" },
		{ "expect-selected PlayerSpaceShip", "expect-selected" },
		{ "expect-location 400 300", "expect-location" },
		{ "expect-location 400 300 0.01", "expect-location" },
		{ "expect-count Actor 3", "expect-count" },
		{ "capture subdir/smoke.png", "capture" }, { "quit", "quit" },
		{ "remember-location", "remember-location" },
		{ "expect-remembered-location", "expect-remembered-location" },
		{ "open-json content/data/ships.json", "open-json" },
		{ "json-set /ships/0/health 280.0", "json-set" },
		{ "expect-json /flag true", "expect-json" },
		{ "expect-json-dirty false", "expect-json-dirty" },
		{ "json-save", "json-save" }, { "json-revert", "json-revert" },
		{ "export out/LightYears", "export" },
		{ "export \"path with spaces/LightYears\" overwrite", "export" },
		{ "expect-export-ok", "expect-export-ok" }
	};
	for (const auto& test : cases)
	{
		const auto parsed = ParseEditorScriptLine(test.first);
		Check(parsed.command && parsed.error.empty() && parsed.command->operation == test.second, std::string{ "Parse failed: " } + test.first);
		Check(test.second == "set-prop-all" || !ParseEditorScriptLine(std::string{ test.first } + " extra").error.empty(), std::string{ "Extra argument accepted: " } + test.first);
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
	for (const char* line : { "expect-console", "expect-console Debug x", "expect-console info x", "expect-console Error \"\"",
		"profiler", "profiler true", "diagnostics yes", "expect-profile-scope", "expect-profile-scope \"\"", "capture-profile \"\"", "clear-console extra", "expect-no-errors extra" })
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{ "Invalid diagnostics command accepted: " } + line);
	const auto console = ParseEditorScriptLine("expect-console Error \"a.lua:4 # details\"");
	Check(console.command && console.command->property == "Error" && console.command->text == "a.lua:4 # details", "Console arguments preserved");
	Check(ParseEditorScriptLine("save-scene").command->operation == "save-scene", "Save current scene must parse");
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
	Check(property.command && property.command->operation == "set-prop" &&
		property.command->text == "HealthComponent" && property.command->property == "Max Health" &&
		property.command->values == std::vector<std::string>{ "250" }, "Quoted property arguments lost");
	const auto expect = ParseEditorScriptLine("expect-prop Actor \"Location\" 1 2 0.01 # comment");
	Check(expect.command && expect.command->operation == "expect-prop" &&
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
	Check(ParseEditorScriptLine("add-component HealthComponent").command->operation == "add-component", "Add component parsing failed");
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
	for (const char* line : { "set-setting \"routeStop\" 1", "expect-setting \"playerShip\" \"Ship.Player.Breacher.Basic\"",
		"set-setting offset [1,2]", "set-setting color [0,1,2,255]", "set-setting enabled true", "set-setting title \"\"", "reset-setting routeStop", "set-setting bounds null", "expect-setting bounds null" })
		Check(ParseEditorScriptLine(line).command.has_value(), "Valid scene setting command rejected");
	for (const char* line : { "set-setting", "set-setting routeStop", "set-setting routeStop 1 extra", "set-setting \"\" 1",
		"set-setting routeStop nan", "set-setting routeStop {}", "expect-setting key", "reset-setting", "reset-setting key extra" })
		Check(!ParseEditorScriptLine(line).error.empty(), "Invalid scene setting command accepted");
	Check(ParseEditorScriptLine("set-setting title \"hello world\"").command->values[0] == "\"hello world\"", "Setting string JSON lost");
	for (const char* line : { "select-add", "select-add \"\"", "select-add a b", "select-remove", "select-all 1",
		"expect-selection-count -1", "expect-selection-count 4294967296", "expect-selection-count 1x", "expect-selection-count",
		"select-rect 1 2 3", "select-rect 1 2 3 nan", "select-rect 1 2 3 4 extra", "select-rect inf 1 2 3",
		"paste 1", "paste 1 2 extra", "paste 1 nan", "copy extra", "move-selection 1", "move-selection inf 0",
		"set-prop-all", "set-prop-all P", "set-prop-all \"\" true" })
		Check(!ParseEditorScriptLine(line).error.empty(), "Invalid multi-selection command accepted");
	const auto rectangle = ParseEditorScriptLine("select-rect -10 20 50 -70").command;
	Check(rectangle && rectangle->location == sf::Vector2f{ -10.f, 20.f } && rectangle->rectangleEnd == sf::Vector2f{ 50.f, -70.f }, "Rectangle endpoints lost");
	Check(ParseEditorScriptLine("paste").command->number == 0 && ParseEditorScriptLine("paste 0 0").command->number == 1, "Paste origin must differ from omitted position");
	if (failures) return 1;
	for (const char* line : { "content-open ability Ability.Movement.Dash.Basic", "content-new effect TimedModifier Effect.Test.Copy.Basic",
		"content-rename effect Effect.Test.Copy.Basic Effect.Test.Renamed.Basic", "content-set duration 3.5", "content-set modifiers[0].magnitude -4.25",
		"content-set /modifiers/0/attributeId \"Owner.AttackPower\"", "expect-content effect Effect.Test.Copy.Basic duration 3.5", "content-validate", "content-save", "content-reload" })
		Check(ParseEditorScriptLine(line).command.has_value(), std::string{ "Valid content command rejected: " } + line);
	for (const char* line : { "content-open", "content-open bad/type id", "content-open effect", "content-new effect x", "content-rename effect x",
		"content-set", "content-set duration nan", "content-set duration 1e999", "content-set duration 2 extra", "content-set x..y 1", "content-set x[no] 1",
		"content-set /x~2 1", "content-set x. 1", "content-set x null", "expect-content effect id duration", "content-save extra", "content-validate extra" })
		Check(!ParseEditorScriptLine(line).error.empty(), std::string{ "Invalid content command accepted: " } + line);
	Check(ParseEditorScriptLine("content-set name \"hello world\"").command->values[0] == "\"hello world\"", "Content string spelling lost");
	Check(ParseEditorScriptLine(R"(content-set modifiers [{"attributeId": "Owner.AttackPower", "operation": "Add", "magnitude": 2.0}])").command.has_value(), "Composite JSON argument rejected");
	for (const char* line : { "content-set values [1,", "content-set values [1]x", "content-set values {bad}", "content-set values [{]" })
		Check(!ParseEditorScriptLine(line).error.empty(), "Malformed composite content JSON accepted");
	Check(ParseEditorScriptLine("help").command.has_value() && ParseEditorScriptLine("help move").command.has_value(), "Help schema failed");
	Check(!ParseEditorScriptLine("help move extra").error.empty(), "Help accepted extra arguments");
	Check(registry.Help().find("sky-drag") != std::string::npos && registry.Help("move").find("x:real") != std::string::npos, "Help must use registry declarations");
	bool duplicateRejected = false;
	try { registry.Add(*registry.Find("move")); } catch (const std::invalid_argument&) { duplicateRejected = true; }
	Check(duplicateRejected, "Duplicate commands must be rejected");
	registry.Add(ly::PlayerShipScriptCommand(
		[](vega::EditorContext&, vega::CommandHistory&, const std::vector<std::string>&, std::string&) { return true; }));
	if (argc == 2 && std::string{argv[1]} == "--write-doc")
	{
		std::ofstream document{VEGA_COMMAND_DOC, std::ios::binary};
		document << registry.Markdown();
		return document && !failures ? 0 : 1;
	}
	std::ifstream document{VEGA_COMMAND_DOC, std::ios::binary};
	std::ostringstream contents; contents << document.rdbuf();
	Check(contents.str() == registry.Markdown(), "docs/editor-script-commands.md is out of sync with the registry");
	if (failures) return 1;
	std::cout << "Editor script parser tests passed\n";
	return 0;
}
