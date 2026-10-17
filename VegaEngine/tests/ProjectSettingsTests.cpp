#include "framework/ProjectSettings.h"
#include <fstream>
#include <iostream>

namespace
{
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}
	void Reject(const nlohmann::json& document)
	{
		try { vega::ProjectSettings::Parse(document); Check(false, "Invalid project was accepted"); }
		catch (const std::exception&) {}
	}
}

int main()
{
	vega::ProjectSettings defaults;
	auto document = defaults.ToJson();
	{
		auto configured = document;
		configured["editor"] = { { "assetRoot", "content" }, { "font", "ui/font.ttf" }, { "visualAssets", "art" },
			{ "gameBuildDir", "../../output" }, { "releaseGameBuildDir", "../../release" }, { "exportDir", "../../exports" } };
		configured["runtime"] = { { "executable", "AnotherRuntime.exe" }, { "assetDirectory", "data" }, { "visualDirectory", "art" }, { "log", "Another.log" } };
		const auto other = vega::ProjectSettings::Parse(configured);
		Check(other.ToJson() == configured && other.runtimeExecutable == "AnotherRuntime.exe", "Project layout must survive settings saves");
		for (const auto& key : { "executable", "assetDirectory", "visualDirectory", "log" })
		{
			auto bad = configured; bad["runtime"][key] = "../escape"; Reject(bad);
		}
		{ auto bad = configured; bad["editor"]["font"] = 12; Reject(bad); }
	}
	const auto parsed = vega::ProjectSettings::Parse(document);
	Check(parsed.AutosaveSeconds() == 120, "Autosave must default to two minutes");
	for (const auto seconds : { 0, 1, 120, 86400 })
	{
		auto configured = document;
		configured["editor"]["autosaveSeconds"] = seconds;
		Check(vega::ProjectSettings::Parse(configured).AutosaveSeconds() == static_cast<unsigned int>(seconds), "Autosave interval did not round-trip");
	}
	for (const auto& seconds : { nlohmann::json(-1), nlohmann::json(86401), nlohmann::json(1.5), nlohmann::json("120"), nlohmann::json(true), nlohmann::json(nullptr) })
	{
		auto invalid = document;
		invalid["editor"]["autosaveSeconds"] = seconds;
		Reject(invalid);
	}
	Check(parsed.name == "Game" && parsed.startupScene.empty() && parsed.windowWidth == 1920 && parsed.windowHeight == 1080,
		"Project defaults changed");
	document.erase("window");
	Check(vega::ProjectSettings::Parse(document).windowWidth == 1920, "Missing window must use defaults");
	document["window"] = { { "height", 720 } };
	Check(vega::ProjectSettings::Parse(document).windowWidth == 1920 && vega::ProjectSettings::Parse(document).windowHeight == 720,
		"Missing dimension must use its default");
	document["startupScene"] = "scenes/start.scene.json";
	document["window"] = { { "width", 1280 }, { "height", 720 } };
	const auto path = std::filesystem::path{ PROJECT_TEST_ROOT } / "valid.project.json";
	std::filesystem::create_directories(path.parent_path());
	std::ofstream{ path } << document;
	const auto loaded = vega::ProjectSettings::Load(path);
	Check(loaded.startupScene == "scenes/start.scene.json" && loaded.windowWidth == 1280 && loaded.windowHeight == 720,
		"Project file did not load");
	for (const auto& key : { "format", "version", "name" }) { auto bad = document; bad.erase(key); Reject(bad); }
	for (auto value : { nlohmann::json(0), nlohmann::json(-1), nlohmann::json(16385), nlohmann::json(12.5), nlohmann::json("wide") })
	{
		auto bad = document; bad["window"]["width"] = value; Reject(bad);
	}
	for (const auto& value : { "../outside.scene.json", "C:/outside.scene.json", "/outside.scene.json", "scene.txt", "scenes\\start.scene.json" })
	{
		auto bad = document; bad["startupScene"] = value; Reject(bad);
	}
	for (auto bad : { nlohmann::json::array(), nlohmann::json{nullptr} }) Reject(bad);
	{ auto bad = document; bad["format"] = "other"; Reject(bad); }
	{ auto bad = document; bad["version"] = 2; Reject(bad); }
	{ auto bad = document; bad["version"] = 1.0; Reject(bad); }
	{ auto bad = document; bad["name"] = "  "; Reject(bad); }
	{ auto bad = document; bad["window"] = nullptr; Reject(bad); }
	defaults.name = "My/Game:*?";
	Check(defaults.ExecutableName() == "My_Game___.exe", "Unsafe executable characters survived");
	defaults.name = "CON";
	Check(defaults.ExecutableName() == "_CON.exe", "Reserved Windows filename survived");
	defaults.name = "... ";
	Check(defaults.ExecutableName() == "Game.exe", "Empty sanitized name needs fallback");
	try { vega::ProjectSettings::Load(path.parent_path() / "missing.json"); Check(false, "Missing project accepted"); }
	catch (const std::exception&) {}
	std::ofstream{ path } << "{";
	try { vega::ProjectSettings::Load(path); Check(false, "Malformed project accepted"); }
	catch (const std::exception&) {}
	return failures ? 1 : 0;
}
