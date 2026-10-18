#include "gameFramework/LightYearsAssets.h"
#include <framework/scene/SceneSettings.h>
#include <framework/scene/Scene.h>
#include <framework/World.h>
#include <framework/AssetManager.h>
#include "level/GameSceneSettings.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/galaxy/GalaxyRoute.h"
#include <iostream>
#include <limits>

namespace
{
	int failures = 0;
	void Check(bool value, const char* message)
	{
		if (!value) { std::cerr << message << '\n'; ++failures; }
	}
}

int main()
{
	vega::InstallVegaEngineAssets();
	using namespace vega;
	using Json = nlohmann::json;
	const auto root = std::filesystem::path{ LIGHT_YEARS_PROJECT_SOURCE_DIR } / "LightYearsGame/assets";
	vega::EngineContext::Current().Assets().SetAssetRootDirectory(root.generic_string() + "/");
	Check(GameContentBootstrap::Register(), "Content bootstrap failed");
	RegisterGameSceneSettings();
	auto& schema = vega::EngineContext::Current().SceneSettings();
	const auto* route = schema.Find("routeStop");
	const auto* ship = schema.Find("playerShip");
	// Engine fields: bounds, gravity; game fields: routeStop, playerShip.
	Check(route && ship && schema.Find("bounds") && schema.Find("gravity") && schema.Fields().size() == 4, "Engine/game schema registration incomplete");
	if (!route || !ship) return 1;
	GalaxyRoute galaxy;
	std::string error;
	Check(galaxy.LoadFromFile(root / "content/data/galaxy_route.json", error), "Route load failed");
	galaxy.AppendProceduralStops(RouteProceduralStopCount, RouteProceduralSeed);
	Check(galaxy.AppendStopsFromFile(root / "content/data/galaxy_centre_tour.json", error), "Tour load failed");
	const auto stops = route->options();
	Check(stops.size() == galaxy.Stops().size() && route->max == static_cast<double>(stops.size() - 1), "Route schema range differs from backdrop route");
	for (std::size_t i = 0; i < stops.size(); ++i)
		Check(stops[i].value == i && stops[i].label.find(galaxy.Stops()[i].name) != std::string::npos, "Route labels/order differ");
	Check(ship->options().size() == content::ShipContentCatalog::GetPlayerShipIds().size(), "Player catalog options differ");
	for (const auto& option : ship->options()) Check(ship->Validate(option.value).empty(), "Catalog entry rejected");
	Check(!route->Validate(-1).empty() && !route->Validate(stops.size()).empty() && !route->Validate(1.5).empty(), "Invalid route accepted");
	Check(!ship->Validate("Ship.Enemy.RangeKeeper.Basic").empty() && !ship->Validate("missing").empty(), "Invalid player catalog id accepted");
	Json authored = Json::object();
	Check(schema.Value(authored, "routeStop") == 0 && authored.empty(), "Reading default materializes JSON");
	Check(schema.Set(authored, "routeStop", Json(0), error) && authored.empty(), "Untouched default should stay omitted");
	authored["future"] = { { "nested", { 1, "untouched", false } } };
	const auto unknown = authored["future"];
	Check(schema.Set(authored, "routeStop", Json(1), error) && authored["routeStop"] == 1, "Valid setting write failed");
	const auto before = authored;
	Check(!schema.Set(authored, "routeStop", Json(-1), error) && authored == before, "Invalid write changed data");
	Check(!schema.Set(authored, "future", Json(2), error) && authored == before, "Unknown setting writable");
	Check(schema.Set(authored, "routeStop", std::nullopt, error) && !authored.contains("routeStop"), "Reset must omit override");
	World world{ nullptr };
	const auto saved = SceneSerializer::Save(world, authored);
	World reloaded{ nullptr };
	Check(SceneSerializer::Load(reloaded, saved).warnings.empty(), "Sparse settings scene rejected");
	Check(SceneSerializer::Save(reloaded, saved["settings"]) == saved && saved["settings"]["future"] == unknown, "Unknown structured setting lost on round trip");
	Check(!saved["settings"].contains("playerShip") && !saved["settings"].contains("routeStop"), "Save materialized defaults");
	SceneSettingsSchema local;
	const auto test = [&](SceneSettingType type, Json valid, Json invalid)
	{
		SceneSettingField field;
		field.key = "value";
		field.type = type;
		field.defaultValue = valid;
		local.Register(field);
		Check(local.Fields().size() == 1 && local.Find("value")->Validate(valid).empty(), "Scalar registration failed");
		Check(!field.Validate(invalid).empty(), "Invalid typed value accepted");
	};
	test(SceneSettingType::Bool, true, 1);
	test(SceneSettingType::Int, 2, 2.5);
	test(SceneSettingType::Float, 2.5, "2.5");
	test(SceneSettingType::String, "hello", false);
	test(SceneSettingType::Vec2, Json::array({ 1, 2 }), Json::array({ 1 }));
	test(SceneSettingType::Color, Json::array({ 0, 128, 255, 255 }), Json::array({ 0, 0, 0, 256 }));
	SceneSettingField bounded;
	bounded.key = "bounded";
	bounded.type = SceneSettingType::Float;
	bounded.defaultValue = 1;
	bounded.min = 0;
	bounded.max = 2;
	Check(!bounded.Validate(-.1).empty() && !bounded.Validate(2.1).empty() && !bounded.Validate(std::numeric_limits<double>::infinity()).empty(), "Numeric bounds/finite check failed");
	bounded.defaultValue = 3;
	bool rejected = false;
	try { local.Register(bounded); } catch (const std::invalid_argument&) { rejected = true; }
	Check(rejected, "Invalid schema default accepted");
	RegisterGameSceneSettings();
	Check(schema.Fields().size() == 4, "Repeated registration duplicates fields");
	return failures ? 1 : 0;
}
