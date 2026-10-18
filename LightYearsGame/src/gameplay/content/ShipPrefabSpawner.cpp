#include "gameplay/content/ShipPrefabSpawner.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "player/PlayerSpaceShip.h"
#include "reflection/GameReflection.h"
#include <framework/AssetManager.h>
#include <framework/World.h>
#include <framework/scene/PrefabLibrary.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <vector>

namespace vega
{
	namespace
	{
		using Json = nlohmann::json;

		bool Fail(std::string* reason, const std::string& message)
		{
			if (reason) *reason = message;
			return false;
		}

		Json ReadSelection(PrefabLibrary& library, const std::string& path, std::set<std::filesystem::path>& chain)
		{
			std::vector<std::string> warnings;
			const auto document = library.Load(path, warnings); // Engine checks canonical asset-root confinement, including links.
			if (!warnings.empty() || !document.is_object()) throw std::runtime_error(warnings.empty() ? "Invalid ship prefab" : warnings.front());
			const auto absolute = std::filesystem::weakly_canonical(library.Root() / std::filesystem::u8path(path));
			if (chain.size() >= 64 || !chain.insert(absolute).second) throw std::runtime_error("Ship prefab cycle or depth limit");
			const auto& base = document.at("base");
			if (base.size() != 1 || base.contains("type") == base.contains("prefab")) throw std::runtime_error("Ship prefab base must select exactly one type or prefab");
			// Player prefabs admit no component/script/hierarchy snapshots or gameplay overrides.
			for (const auto& field : document.items())
				if (field.key() != "format" && field.key() != "version" && field.key() != "name" && field.key() != "base" && field.key() != "properties")
					throw std::runtime_error("Ship prefab cannot author '" + field.key() + "'; gameplay data belongs to ships.json");
			Json selection = base.contains("prefab") ? ReadSelection(library, base.at("prefab").get<std::string>(), chain) : base;
			if (!selection.contains("properties")) selection["properties"] = Json::object();
			if (document.contains("properties"))
			{
				const auto& properties = document.at("properties");
				if (!properties.is_object()) throw std::runtime_error("Ship prefab properties must be an object");
				for (const auto& property : properties.items())
				{
					if (property.key() != "Ship" && property.key() != "Render Enabled") throw std::runtime_error("Ship prefab cannot override catalog gameplay property: " + property.key());
					if (property.key() == "Ship" && !property.value().is_string()) throw std::runtime_error("Ship must be a catalog ID string");
					if (property.key() == "Render Enabled" && !property.value().is_boolean()) throw std::runtime_error("Render Enabled must be boolean");
				}
				selection["properties"].update(properties);
			}
			return selection;
		}
	}

	bool ValidateShipPrefab(EngineContext& context, const std::filesystem::path& assetRoot, const std::string& prefabPath, const std::string& shipId, std::string* failureReason)
	{
		if (failureReason) failureReason->clear();
		try
		{
			PrefabLibrary library{ context, assetRoot };
			std::set<std::filesystem::path> chain;
			const auto selection = ReadSelection(library, prefabPath, chain);
			if (selection.value("type", std::string{}) != "PlayerSpaceShip") return Fail(failureReason, "Ship prefab root must be PlayerSpaceShip");
			if (shipId.rfind("Ship.Player.", 0) != 0 || selection["properties"].value("Ship", std::string{}) != shipId)
				return Fail(failureReason, "Ship prefab Ship must equal catalog ship ID '" + shipId + "'");
			return true;
		}
		catch (const std::exception& error) { return Fail(failureReason, error.what()); }
	}

	std::weak_ptr<PlayerSpaceShip> SpawnShipPrefab(World& world, const std::string& prefabPath, const std::string& shipId, std::string* failureReason)
	{
		if (failureReason) failureReason->clear();
		auto& context = world.GetEngineContext();
		const auto configuredRoot = context.Assets().GetAssetRootDirectory();
		const auto root = configuredRoot.empty() ? std::filesystem::path{ "LightYearsGame/assets" } : std::filesystem::path{ configuredRoot };
		if (!content::ShipContentCatalog::FindById(shipId)) { Fail(failureReason, "Unknown catalog ship: " + shipId); return {}; }
		if (!ValidateShipPrefab(context, root, prefabPath, shipId, failureReason)) return {};
		// Legacy runtime entry points also spawn without registering editor reflection first.
		EngineContext::Scope scope{ context };
		RegisterGameTypes();
		// Constructor snapshots must reflect today's catalog, not a cached pre-reload realization.
		PrefabLibrary library{ context, root };
		const auto resolved = library.Resolve(prefabPath);
		if (!resolved || !resolved.warnings.empty())
		{
			Fail(failureReason, resolved.warnings.empty() ? "Cannot resolve ship prefab" : resolved.warnings.front());
			return {};
		}
		if (resolved.entity.value("type", std::string{}) != "PlayerSpaceShip" || resolved.entity.at("properties").value("Ship", std::string{}) != shipId)
		{
			Fail(failureReason, "Resolved ship prefab type or Ship differs from the catalog selection");
			return {};
		}
		std::set<std::uint64_t> before;
		for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>()) if (const auto actor = weak.lock()) before.insert(actor->GetUniqueID());
		const auto unresolved = world.unresolvedPrefabs;
		const auto guid = SceneEntityComponent::GenerateGuid();
		const Json entity{ { "prefab", prefabPath }, { "guid", guid }, { "name", resolved.entity.value("name", std::string{ "Fighter" }) } };
		const auto loaded = SceneSerializer::Load(world, { { "format", "vega.scene" }, { "version", 1 }, { "settings", Json::object() }, { "entities", Json::array({ entity }) } }, &library, false);
		std::shared_ptr<PlayerSpaceShip> ship;
		std::vector<std::shared_ptr<Actor>> created;
		for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>())
			if (const auto actor = weak.lock(); actor && before.count(actor->GetUniqueID()) == 0)
			{
				created.push_back(actor);
				const auto* identity = actor->FindComponent<SceneEntityComponent>();
				if (identity && identity->guid == guid) ship = std::dynamic_pointer_cast<PlayerSpaceShip>(actor);
			}
		if (loaded.spawned != 1 || !loaded.warnings.empty() || !ship || ship->GetShip() != shipId || created.size() != 1 || world.unresolvedPrefabs != unresolved)
		{
			for (const auto& actor : created) actor->Destroy(); // Also destroys attached descendants; pending storage is reclaimed by the world's normal cleanup.
			world.unresolvedPrefabs = unresolved;
			Fail(failureReason, loaded.warnings.empty() ? "Ship prefab composition was incomplete" : loaded.warnings.front());
			return {};
		}
		return ship;
	}
}
