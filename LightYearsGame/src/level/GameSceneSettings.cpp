#include "level/GameSceneSettings.h"
#include "gameplay/galaxy/GalaxyRoute.h"
#include "gameplay/content/ShipContentCatalog.h"
#include <framework/AssetManager.h>
#include <framework/EngineContext.h>
#include <framework/scene/SceneSettings.h>
#include <framework/debug/Log.h>

namespace vega
{
	void RegisterGameSceneSettings()
	{
		GalaxyRoute route;
		std::string error;
		const auto root = std::filesystem::path{ EngineContext::Current().Assets().GetAssetRootDirectory() };
		std::vector<SceneSettingOption> stops;
		if (route.LoadFromFile(root / "content/data/galaxy_route.json", error) && !route.Stops().empty())
		{
			route.AppendProceduralStops(RouteProceduralStopCount, RouteProceduralSeed);
			if (!route.AppendStopsFromFile(root / "content/data/galaxy_centre_tour.json", error))
				VEGA_GAME_WARN("Scene settings route tour: %s", error.c_str());
			for (std::size_t i = 0; i < route.Stops().size(); ++i)
				stops.push_back({ static_cast<int>(i), std::to_string(i) + " - " + route.Stops()[i].name });
		}
		else
		{
			VEGA_GAME_WARN("Scene settings route: %s", error.c_str());
			stops.push_back({ 0, "0 - Sol (route unavailable)" });
		}
		SceneSettingField stop;
		stop.key = "routeStop";
		stop.label = "Route stop";
		stop.category = "Environment";
		stop.type = SceneSettingType::Enum;
		stop.defaultValue = 0;
		stop.min = 0;
		stop.max = static_cast<double>(stops.size() - 1);
		stop.tooltip = "Campaign route backdrop. Updates immediately in Edit mode.";
		stop.options = [stops = std::move(stops)]() { return stops; };
		EngineContext::Current().SceneSettings().Register(std::move(stop));

		SceneSettingField ship;
		ship.key = "playerShip";
		ship.label = "Player ship";
		ship.category = "Gameplay";
		ship.type = SceneSettingType::CatalogId;
		ship.defaultValue = "Ship.Player.Fighter.Basic";
		ship.tooltip = "Player catalog ship used on Play. A PlayerStart marker overrides this setting.";
		ship.options = []()
		{
			std::vector<SceneSettingOption> result;
			for (const auto& id : content::ShipContentCatalog::GetPlayerShipIds()) result.push_back({ id, id });
			return result;
		};
		EngineContext::Current().SceneSettings().Register(std::move(ship));

		SceneSettingField rules;
		rules.key = "levelRules";
		rules.label = "Level rules";
		rules.category = "Gameplay";
		rules.type = SceneSettingType::String;
		rules.defaultValue = "";
		rules.tooltip = "Empty uses SceneLevel; ArenaTest uses arena playtest rules; LevelOne uses campaign rules. Applied on Play.";
		EngineContext::Current().SceneSettings().Register(std::move(rules));
	}
}
