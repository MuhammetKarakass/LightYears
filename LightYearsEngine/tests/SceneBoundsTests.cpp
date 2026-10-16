#include <framework/scene/SceneSettings.h>
#include <framework/scene/Scene.h>
#include <framework/World.h>
#include <iostream>
#include <limits>

int main()
{
	using Json = nlohmann::json;
	int failures = 0;
	const auto check = [&](bool value, const char* text) { if (!value) { ++failures; std::cerr << text << '\n'; } };
	Json settings = Json::object();
	std::string error;
	auto& schema = ly::SceneSettingsSchema::Get();
	check(!ly::ReadSceneBounds(settings), "Bounds must be opt-in");
	check(schema.Set(settings, "bounds", Json{ -20, 30, 400, 500 }, error), "Valid rect rejected");
	const auto bounds = ly::ReadSceneBounds(settings);
	check(bounds && bounds->position == sf::Vector2f{ -20, 30 } && bounds->size == sf::Vector2f{ 400, 500 }, "Rect coordinates changed");
	for (const auto& bad : { Json{ 0, 0, 0, 10 }, Json{ 0, 0, -1, 10 }, Json{ 0, 0, 10 }, Json{ "x", 0, 1, 1 }, Json{ 0, 0, 1, 1000001 }, Json{ 0, 0, 1, std::numeric_limits<double>::infinity() } })
		check(!schema.Set(settings, "bounds", bad, error), "Invalid rect accepted");
	ly::World world{ nullptr };
	Json scene{ { "format", "vega.scene" }, { "version", 1 }, { "settings", settings }, { "entities", Json::array() } };
	ly::SceneSerializer::Load(world, scene);
	check(world.GetSceneBounds() == bounds, "Scene did not apply bounds to world");
	scene["settings"] = Json::object();
	ly::SceneSerializer::Load(world, scene);
	check(world.GetSceneBounds() == bounds, "Appending a prefab cleared bounds");
	check(schema.Set(settings, "bounds", std::nullopt, error) && !ly::ReadSceneBounds(settings), "Reset failed");
	return failures ? 1 : 0;
}
