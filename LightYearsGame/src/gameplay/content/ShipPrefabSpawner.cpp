#include "gameplay/content/ShipPrefabSpawner.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/EnemyContentCatalog.h"
#include "player/PlayerSpaceShip.h"
#include "enemy/EnemyActor.h"
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

		struct CatalogPrefabContract
		{
			const char* type;
			const char* selector;
			const char* dataFile;
			bool allowRenderOverride;
		};
		constexpr CatalogPrefabContract PlayerContract{ "PlayerSpaceShip", "Ship", "ships.json", true };
		constexpr CatalogPrefabContract EnemyContract{ "EnemyActor", "Enemy ID", "enemy_definitions.json", false };

		bool Fail(std::string* reason, const std::string& message)
		{
			if (reason) *reason = message;
			return false;
		}

		Json ReadSelection(PrefabLibrary& library, const std::string& path, std::set<std::filesystem::path>& chain, const CatalogPrefabContract& contract)
		{
			std::vector<std::string> warnings;
			const auto document = library.Load(path, warnings); // Engine checks canonical asset-root confinement, including links.
			if (!warnings.empty() || !document.is_object()) throw std::runtime_error(warnings.empty() ? "Invalid catalog prefab" : warnings.front());
			const auto absolute = std::filesystem::weakly_canonical(library.Root() / std::filesystem::u8path(path));
			if (chain.size() >= 64 || !chain.insert(absolute).second) throw std::runtime_error("Catalog prefab cycle or depth limit");
			const auto& base = document.at("base");
			if (base.size() != 1 || base.contains("type") == base.contains("prefab")) throw std::runtime_error("Catalog prefab base must select exactly one type or prefab");
			for (const auto& field : document.items())
				if (field.key() != "format" && field.key() != "version" && field.key() != "name" && field.key() != "base" && field.key() != "properties")
					throw std::runtime_error("Catalog prefab cannot author '" + field.key() + "'; gameplay data belongs to " + contract.dataFile);
			Json selection = base.contains("prefab") ? ReadSelection(library, base.at("prefab").get<std::string>(), chain, contract) : base;
			if (!selection.contains("properties")) selection["properties"] = Json::object();
			if (document.contains("properties"))
			{
				const auto& properties = document.at("properties");
				if (!properties.is_object()) throw std::runtime_error("Catalog prefab properties must be an object");
				for (const auto& property : properties.items())
				{
					if (property.key() == contract.selector)
					{
						if (!property.value().is_string()) throw std::runtime_error(std::string{ contract.selector } + " must be a catalog ID string");
					}
					else if (contract.allowRenderOverride && property.key() == "Render Enabled")
					{
						if (!property.value().is_boolean()) throw std::runtime_error("Render Enabled must be boolean");
					}
					else throw std::runtime_error("Catalog prefab cannot override catalog gameplay property: " + property.key());
				}
				selection["properties"].update(properties);
			}
			return selection;
		}

		bool ValidateCatalogPrefab(EngineContext& context, const std::filesystem::path& assetRoot, const std::string& prefabPath, const std::string& id, const CatalogPrefabContract& contract, std::string* failureReason)
		{
			if (failureReason) failureReason->clear();
			try
			{
				PrefabLibrary library{ context, assetRoot };
				std::set<std::filesystem::path> chain;
				const auto selection = ReadSelection(library, prefabPath, chain, contract);
				if (selection.value("type", std::string{}) != contract.type) return Fail(failureReason, std::string{ "Catalog prefab root must be " } + contract.type);
				if (id.empty() || selection["properties"].value(contract.selector, std::string{}) != id)
					return Fail(failureReason, std::string{ "Catalog prefab " } + contract.selector + " must equal catalog ID '" + id + "'");
				return true;
			}
			catch (const std::exception& error) { return Fail(failureReason, error.what()); }
		}

		template<typename ActorType, typename SelectionGetter>
		std::weak_ptr<ActorType> SpawnCatalogPrefab(World& world, const std::string& prefabPath, const std::string& id, const CatalogPrefabContract& contract, SelectionGetter getSelection, std::string* failureReason)
		{
			if (failureReason) failureReason->clear();
			auto& context = world.GetEngineContext();
			const auto configuredRoot = context.Assets().GetAssetRootDirectory();
			const auto root = configuredRoot.empty() ? std::filesystem::path{ "LightYearsGame/assets" } : std::filesystem::path{ configuredRoot };
			if (!ValidateCatalogPrefab(context, root, prefabPath, id, contract, failureReason)) return {};
			EngineContext::Scope scope{ context };
			RegisterGameTypes();
			// Constructor snapshots must reflect today's catalog, not a cached pre-reload realization.
			PrefabLibrary library{ context, root };
			const auto resolved = library.Resolve(prefabPath);
			if (!resolved || !resolved.warnings.empty())
			{
				Fail(failureReason, resolved.warnings.empty() ? "Cannot resolve catalog prefab" : resolved.warnings.front());
				return {};
			}
			if (resolved.entity.value("type", std::string{}) != contract.type || resolved.entity.at("properties").value(contract.selector, std::string{}) != id)
			{
				Fail(failureReason, "Resolved catalog prefab type or selection differs from the catalog");
				return {};
			}
			std::set<std::uint64_t> before;
			for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>()) if (const auto actor = weak.lock()) before.insert(actor->GetUniqueID());
			const auto unresolved = world.unresolvedPrefabs;
			const auto guid = SceneEntityComponent::GenerateGuid();
			const Json entity{ { "prefab", prefabPath }, { "guid", guid }, { "name", resolved.entity.value("name", std::string{ contract.type }) } };
			SceneLoadResult loaded;
			std::string exception;
			try
			{
				loaded = SceneSerializer::Load(world, { { "format", "vega.scene" }, { "version", 1 }, { "settings", Json::object() }, { "entities", Json::array({ entity }) } }, &library, false);
			}
			catch (const std::exception& error) { exception = error.what(); }
			std::shared_ptr<ActorType> selected;
			std::vector<std::shared_ptr<Actor>> created;
			for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>())
				if (const auto actor = weak.lock(); actor && before.count(actor->GetUniqueID()) == 0)
				{
					created.push_back(actor);
					const auto* identity = actor->FindComponent<SceneEntityComponent>();
					if (identity && identity->guid == guid) selected = std::dynamic_pointer_cast<ActorType>(actor);
				}
			if (!exception.empty() || loaded.spawned != 1 || !loaded.warnings.empty() || !selected || getSelection(*selected) != id || selected->GetIsPendingDestroy() || created.size() != 1 || world.unresolvedPrefabs != unresolved)
			{
				for (const auto& actor : created) actor->Destroy(); // Pending storage is reclaimed by normal world cleanup; never tick the caller here.
				world.unresolvedPrefabs = unresolved;
				Fail(failureReason, !exception.empty() ? exception : loaded.warnings.empty() ? "Catalog prefab composition was incomplete" : loaded.warnings.front());
				return {};
			}
			return selected;
		}
	}

	bool ValidateShipPrefab(EngineContext& context, const std::filesystem::path& assetRoot, const std::string& prefabPath, const std::string& shipId, std::string* failureReason)
	{
		if (shipId.rfind("Ship.Player.", 0) != 0) return Fail(failureReason, "Ship prefab requires a player catalog ID");
		return ValidateCatalogPrefab(context, assetRoot, prefabPath, shipId, PlayerContract, failureReason);
	}

	std::weak_ptr<PlayerSpaceShip> SpawnShipPrefab(World& world, const std::string& prefabPath, const std::string& shipId, std::string* failureReason)
	{
		if (!content::ShipContentCatalog::FindById(shipId) || shipId.rfind("Ship.Player.", 0) != 0) { Fail(failureReason, "Unknown player catalog ship: " + shipId); return {}; }
		return SpawnCatalogPrefab<PlayerSpaceShip>(world, prefabPath, shipId, PlayerContract, [](const PlayerSpaceShip& ship) -> const std::string& { return ship.GetShip(); }, failureReason);
	}

	bool ValidateEnemyPrefab(EngineContext& context, const std::filesystem::path& assetRoot, const std::string& prefabPath, const std::string& enemyId, std::string* failureReason)
	{
		return ValidateCatalogPrefab(context, assetRoot, prefabPath, enemyId, EnemyContract, failureReason);
	}

	std::weak_ptr<EnemyActor> SpawnEnemyPrefab(World& world, const std::string& prefabPath, const std::string& enemyId, std::string* failureReason)
	{
		if (!content::EnemyContentCatalog::FindById(enemyId)) { Fail(failureReason, "Unknown catalog enemy: " + enemyId); return {}; }
		return SpawnCatalogPrefab<EnemyActor>(world, prefabPath, enemyId, EnemyContract, [](const EnemyActor& enemy) -> const std::string& { return enemy.GetEnemyId(); }, failureReason);
	}
}
