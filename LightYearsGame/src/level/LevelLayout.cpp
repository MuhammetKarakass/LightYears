#include "level/LevelLayout.h"
#include "level/ArenaSandboxSetup.h"
#include "level/GameSceneSettings.h"
#include "level/PlayerStart.h"
#include "reflection/GameReflection.h"
#include "enemy/DummyEnemy.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/GameContentBootstrap.h"
#include "gameplay/content/EnemyContentCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/ShipPrefabSpawner.h"
#include "framework/AssetManager.h"
#include "framework/scene/PrefabLibrary.h"
#include "framework/scene/SceneSettings.h"
#include "framework/debug/Log.h"
#include <algorithm>
#include <fstream>
#include <limits>
#include <set>

namespace vega
{
	namespace
	{
		using Json = nlohmann::json;
		void Require(bool valid, const std::string& message) { if (!valid) throw std::runtime_error("Level layout: " + message); }
		bool Finite(float value) { return std::isfinite(value); }
		bool Finite(sf::Vector2f value) { return Finite(value.x) && Finite(value.y); }
		bool PlayerShip(const std::string& id) { return id.rfind("Ship.Player.", 0) == 0 && content::ShipContentCatalog::FindById(id); }
		void ValidateProperties(const Json& properties, const reflect::TypeInfo& type, const std::string& path);
		void ValidateValue(const Json& value, const reflect::PropertyInfo& property, const std::string& path)
		{
			using reflect::PropertyKind;
			auto numeric = [&](const Json& number) { return number.is_number() && std::isfinite(number.get<double>()) && std::abs(number.get<double>()) <= std::numeric_limits<float>::max(); };
			switch (property.kind)
			{
			case PropertyKind::Bool: Require(value.is_boolean(), path + " must be boolean"); break;
			case PropertyKind::Int: Require(value.is_number_integer() && value.get<double>() >= std::numeric_limits<int>::min() && value.get<double>() <= std::numeric_limits<int>::max(), path + " must be an int32"); break;
			case PropertyKind::Float: Require(numeric(value), path + " must be finite float"); break;
			case PropertyKind::Vector2:
			case PropertyKind::Color:
				Require(value.is_array() && value.size() == (property.kind == PropertyKind::Vector2 ? 2u : 4u), path + " has invalid vector/color dimensions");
				for (const auto& item : value)
				{
					Require(numeric(item), path + " must contain finite numbers");
					if (property.kind == PropertyKind::Color) Require(item.is_number_integer() && item.get<double>() >= 0 && item.get<double>() <= 255, path + " color channels must be bytes");
				}
				break;
			case PropertyKind::String: Require(value.is_string(), path + " must be string"); break;
			case PropertyKind::Enum:
				Require(value.is_string(), path + " must be an enum label");
				Require(std::any_of(property.enumLabels.begin(), property.enumLabels.end(), [&](const auto& label) { return value == label.second; }), path + " has unsupported mode/enum label " + value.dump());
				break;
			case PropertyKind::Struct:
				Require(property.structInfo, path + " has no registered struct");
				Require(value.is_object(), path + " must be an object");
				for (const auto& field : property.structInfo->properties) if (field.setter) Require(value.contains(field.name), path + "." + field.name + " is required");
				ValidateProperties(value, *property.structInfo, path);
				break;
			case PropertyKind::List:
				Require(value.is_array() && value.size() <= 4096, path + " must be a list with at most 4096 elements");
				for (size_t index = 0; index < value.size(); ++index) ValidateValue(value[index], property.Element(), path + "[" + std::to_string(index) + "]");
				break;
			case PropertyKind::ActorRef: Require(value.is_object() && value.contains("guid") && value["guid"].is_string(), path + " must be a GUID reference"); break;
			case PropertyKind::Flags: Require(value.is_array(), path + " must be a flag-label list"); break;
			}
		}
		void ValidateProperties(const Json& properties, const reflect::TypeInfo& type, const std::string& path)
		{
			Require(properties.is_object(), path + " must be an object");
			for (const auto& entry : properties.items())
			{
				const auto* field = type.FindProperty(entry.key());
				Require(field && field->setter, path + "." + entry.key() + " is unknown or read-only");
				ValidateValue(entry.value(), *field, path + "." + entry.key());
			}
		}
		void ValidateEntity(const Json& entity, EngineContext& context, PrefabLibrary& prefabs, std::set<std::string>& guids, const std::string& path)
		{
			Require(entity.is_object(), path + " must be an entity object");
			Require(entity.contains("guid") && entity["guid"].is_string() && !entity["guid"].get<std::string>().empty(), path + ".guid must be nonempty");
			Require(guids.insert(entity["guid"].get<std::string>()).second, path + ".guid is duplicate: " + entity["guid"].get<std::string>());
			Require(entity.contains("type") != entity.contains("prefab"), path + " must select exactly one type or prefab");
			Json resolved = entity;
			if (entity.contains("prefab"))
			{
				Require(entity["prefab"].is_string(), path + ".prefab must be string");
				const auto prefab = prefabs.Resolve(entity["prefab"].get<std::string>());
				Require(static_cast<bool>(prefab) && prefab.warnings.empty(), path + ".prefab cannot resolve: " + (prefab.warnings.empty() ? entity["prefab"].dump() : prefab.warnings.front()));
				resolved = PrefabLibrary::Merge(prefab.entity, entity);
			}
			Require(resolved.contains("type") && resolved["type"].is_string(), path + ".type must be string");
			const auto typeName = resolved["type"].get<std::string>();
			const auto* type = context.Types().Find(typeName);
			Require(type && type->spawnable && type->root == reflect::TypeRoot::Actor, path + ": unknown/non-spawnable type " + typeName);
			Require(typeName != "PlayerSpaceShip" && typeName != "EnemyActor", path + ": incompatible prefab/actor " + typeName + "; level rules own player and stage/encounter spawning");
			if (entity.contains("properties")) ValidateProperties(entity["properties"], *type, path + ".properties");
			ValidateProperties(resolved.value("properties", Json::object()), *type, path + ".properties");
			if (typeName == "ArenaSetupActor" || typeName == "EncounterSetupActor" || typeName == "CampaignSetupActor")
				for (const auto& field : type->properties) Require(resolved["properties"].contains(field.name), path + ".properties." + field.name + " is required");
			if (entity.contains("removed"))
			{
				Require(entity["removed"].is_array(), path + ".removed must be array");
				for (const auto& removal : entity["removed"])
				{
					Require(removal.is_string(), path + ".removed must contain component names");
					Require(!type->RequiresComponent(removal.get<std::string>()), path + ": required component cannot be removed: " + removal.get<std::string>());
				}
			}
			if (typeName == "DummyEnemy")
			{
				Require(entity.contains("prefab"), path + ": arena dummy placements must be prefab instances");
				const auto ship = resolved.value("properties", Json::object()).value("Ship", std::string{});
				std::string error;
				const bool compatible = ValidateDummyPrefab(context, prefabs.Root(), entity["prefab"].get<std::string>(), ship, &error);
				Require(compatible, path + ": incompatible dummy prefab: " + error);
			}
			for (const auto* data : { &entity, static_cast<const Json*>(&resolved) }) if (data->contains("components"))
			{
				Require((*data)["components"].is_array(), path + ".components must be array");
				for (const auto& component : (*data)["components"])
				{
					Require(component.is_object() && component.contains("type") && component["type"].is_string(), path + ": component must select a type");
					const auto* info = context.Types().Find(component["type"].get<std::string>());
					Require(info && info->root == reflect::TypeRoot::Component, path + ": unknown component " + component["type"].dump());
					ValidateProperties(component.value("properties", Json::object()), *info, path + "." + info->name);
					if (info->name == "HealthComponent" && component.contains("properties") && component["properties"].contains("Max Health"))
						Require(component["properties"]["Max Health"].get<float>() > 0, path + ".HealthComponent.Max Health must be positive");
				}
			}
			if (entity.contains("children"))
			{
				Require(entity["children"].is_array(), path + ".children must be array");
				for (const auto& child : entity["children"]) ValidateEntity(child, context, prefabs, guids, path + ".children");
			}
		}
		void ResolveSetup(LevelLayout& layout, const ArenaSetupActor& arena, const EncounterSetupActor& encounter)
		{
			const auto& boundary = arena.boundary;
			layout.arena.outOfBoundsMargin = boundary.outOfBoundsMargin;
			layout.arena.outOfBoundsTime = boundary.outOfBoundsTime;
			layout.arena.boundaryVisual = { boundary.enabled, boundary.visualType, boundary.outlineColor, boundary.warningColor, boundary.outlineThickness };
			const auto& respawn = arena.respawn;
			layout.respawn.respawnDelay = respawn.respawnDelay;
			layout.respawn.playerIndex = respawn.playerIndex;
			layout.respawn.createPlayerIfMissing = respawn.createPlayerIfMissing;
			layout.respawn.useScreenClamp = respawn.useScreenClamp;
			layout.respawn.respawnWhenDestroyed = respawn.respawnWhenDestroyed;
			layout.respawn.gameOverWhenRespawnFails = respawn.gameOverWhenRespawnFails;
			layout.respawn.playerShipId = respawn.playerShipId;
			layout.music = arena.music;
			layout.dummyArmor = arena.dummyArmor;
			layout.encounterEnabled = encounter.enabled;
			layout.sequenceMode = encounter.sequenceMode;
			const auto& progression = encounter.progression;
			layout.progression = { progression.additionalEnemyEveryWaves, progression.enemyLevelEveryWaves, progression.maximumAdditionalEnemies, progression.maximumEnemyLevel, static_cast<uint32_t>(progression.seed), encounter.maximumConcurrentEnemies };
			Require(progression.seed >= 0, "Progression.seed must be nonnegative int32");
			layout.spawnPoints = encounter.spawnPoints;
			layout.spawnOffsetSpacing = encounter.spawnOffsetSpacing;
			layout.waves.clear();
			for (const auto& wave : encounter.waves) layout.waves.push_back({ {}, wave.interWaveDelay });
			for (const auto& entry : encounter.entries)
			{
				Require(entry.wave >= 0 && static_cast<size_t>(entry.wave) < layout.waves.size(), "Entries.wave is outside Waves: " + std::to_string(entry.wave));
				Require(content::EnemyContentCatalog::FindById(entry.enemyId), "Entries.enemyId is absent from enemy_definitions.json: " + entry.enemyId);
				layout.waves[entry.wave].entries.push_back({ sas::ContentId{entry.enemyId}, entry.count, entry.spawnInterval });
			}
		}
		void ValidateResolved(const LevelLayout& layout)
		{
			const auto& arena = layout.arena;
			Require(Finite(arena.legalBounds.position) && Finite(arena.size) && arena.size.x > 0 && arena.size.y > 0 && Finite(arena.legalBounds.position + arena.size), "bounds must be finite with positive size");
			Require(Finite(arena.outOfBoundsMargin) && arena.outOfBoundsMargin >= 0 && Finite(arena.outOfBoundsTime) && arena.outOfBoundsTime > 0, "Boundary margin/time must be finite and nonnegative/positive");
			Require(Finite(arena.boundaryVisual.outlineThickness) && arena.boundaryVisual.outlineThickness >= 0, "Boundary.outlineThickness must be finite and nonnegative");
			Require(Finite(layout.respawn.respawnDelay) && layout.respawn.respawnDelay >= 0 && layout.respawn.playerIndex >= 0, "Respawn delay/index must be nonnegative");
			Require(layout.respawn.playerShipId == "Default" || PlayerShip(layout.respawn.playerShipId), "Respawn.playerShipId is unknown: " + layout.respawn.playerShipId);
			Require(Finite(layout.respawn.spawnLocation), "PlayerStart Location must be finite");
			const auto& music = layout.music;
			Require(!music.path.empty() && Finite(music.volume) && music.volume >= 0 && music.volume <= 100 && Finite(music.pitch) && music.pitch > 0 && Finite(music.fadeDuration) && music.fadeDuration >= 0, "Music has invalid path/volume/pitch/fadeDuration");
			Require(layout.sequenceMode == EncounterSequenceMode::EndlessCycle, "SequenceMode must be EndlessCycle for ArenaTest rules");
			const auto& progression = layout.progression;
			Require(progression.additionalEnemyEveryWaves > 0 && progression.enemyLevelEveryWaves > 0 && progression.maximumAdditionalEnemies >= 0 && progression.maximumEnemyLevel >= 1 && progression.maximumConcurrentEnemies >= 0, "Progression/MaximumConcurrentEnemies intervals and caps are invalid");
			Require(!layout.spawnPoints.empty() && Finite(layout.spawnOffsetSpacing) && layout.spawnOffsetSpacing >= 0, "SpawnPoints must be nonempty and SpawnOffsetSpacing finite/nonnegative");
			for (auto point : layout.spawnPoints) Require(Finite(point), "SpawnPoints must be finite");
			Require(!layout.waves.empty(), "Waves must be nonempty");
			for (size_t index = 0; index < layout.waves.size(); ++index)
			{
				const auto& wave = layout.waves[index];
				const auto path = "Waves[" + std::to_string(index) + "]";
				Require(!wave.entries.empty() && Finite(wave.nextWaveDelay) && wave.nextWaveDelay >= 0, path + " needs entries and a finite nonnegative interWaveDelay");
				int64_t total = 0;
				for (const auto& entry : wave.entries)
				{
					Require(entry.count > 0 && Finite(entry.spawnInterval) && entry.spawnInterval >= 0, path + " count must be positive and spawnInterval finite/nonnegative");
					total += entry.count;
				}
				Require(total <= std::numeric_limits<int>::max(), path + " total count overflows int32");
			}
			Require(Finite(layout.dummyArmor) && layout.dummyArmor >= 0, "DummyArmor must be finite/nonnegative");
		}
	}

	void LevelLayout::RegisterTypesAndContent()
	{
		RegisterGameSceneTypes(EngineContext::Current());
		Require(GameContentBootstrap::Register(), "game content registration failed");
		if (!EngineContext::Current().SceneSettings().Find("levelRules")) RegisterGameSceneSettings();
	}

	LevelLayout LevelLayout::Defaults()
	{
		LevelLayout layout;
		layout.arena = CreateSandboxArenaDefinition();
		layout.respawn = ConfigureSandboxPlayerRespawn({});
		layout.respawn.spawnLocation = { 3000.f, 1500.f };
		// Same authoring defaults as Create in the editor; no actor lifecycle or second runtime.
		World scratch{ EngineContext::Current() };
		ArenaSetupActor arena{ &scratch };
		EncounterSetupActor encounter{ &scratch };
		ResolveSetup(layout, arena, encounter);
		return layout;
	}

	LevelLayout LevelLayout::Parse(const Json& scene, EngineContext& context, const std::string& rules)
	{
		EngineContext::Scope scope{ context };
		RegisterTypesAndContent();
		Require(scene.is_object() && scene.contains("format") && scene["format"].is_string() && scene["format"] == "vega.scene", "format must be vega.scene");
		Require(scene.contains("version") && scene["version"].is_number_integer() && scene["version"] == 1, "version must be integer 1");
		Require(scene.contains("settings") && scene["settings"].is_object(), "settings must be object");
		const auto& settings = scene["settings"];
		Require(rules == "ArenaTest" || rules == "LevelOne", "unknown rules owner: " + rules);
		Require(settings.contains("levelRules") && settings["levelRules"].is_string() && settings["levelRules"] == rules, "settings.levelRules must be " + rules);
		const bool campaign = rules == "LevelOne";
		for (const auto& key : {"bounds", "playerShip", "routeStop", "levelRules"})
		{
			if (campaign && !settings.contains(key)) continue;
			Require(settings.contains(key), std::string{"settings."} + key + " is required");
			const auto* field = context.SceneSettings().Find(key);
			Require(field && field->Validate(settings[key]).empty(), std::string{"settings."} + key + ": " + (field ? field->Validate(settings[key]) : "unregistered"));
		}
		if (!campaign) Require(PlayerShip(settings["playerShip"].get<std::string>()), "settings.playerShip must name a player catalog ship");
		const auto bounds = ReadSceneBounds(settings, context.SceneSettings());
		if (!campaign) Require(bounds.has_value(), "settings.bounds is required and must have finite positive dimensions");
		Require(scene.contains("entities") && scene["entities"].is_array(), "entities must be array");
		PrefabLibrary prefabs{ context, context.Prefabs().Root() };
		std::set<std::string> guids;
		for (size_t index = 0; index < scene["entities"].size(); ++index) ValidateEntity(scene["entities"][index], context, prefabs, guids, "entities[" + std::to_string(index) + "]");
		// Complete preflight in an unticked world. Setter/composition/prefab errors may never
		// partially initialize the live arena, PlayerManager, music or encounter.
		World staging{ context };
		const auto loaded = SceneSerializer::Load(staging, scene, &prefabs, false);
		Require(loaded.warnings.empty(), loaded.warnings.empty() ? "" : loaded.warnings.front());
		const auto campaigns = staging.GetActorsByTypeIncludingPending<CampaignSetupActor>();
		if (campaign)
		{
			Require(campaigns.size() == 1, "LevelOne requires exactly one CampaignSetupActor");
			// Campaign rules own player spawn/respawn. Arena setup or player markers
			// would promise behavior these rules do not implement.
			for (const auto& handle : staging.GetActorsByTypeIncludingPending<Actor>())
			{
				const auto actor = handle.lock();
				Require(!dynamic_cast<ArenaSetupActor*>(actor.get()) && !dynamic_cast<EncounterSetupActor*>(actor.get()) &&
					!dynamic_cast<PlayerStart*>(actor.get()) && !dynamic_cast<DummyEnemy*>(actor.get()), "LevelOne has an incompatible arena/player/dummy setup actor");
			}
			LevelLayout layout;
			layout.campaign = campaigns[0].lock()->plan;
			layout.campaign.Validate();
			layout.scene = scene;
			layout.fromScene = true;
			return layout;
		}
		Require(campaigns.empty(), "ArenaTest cannot contain CampaignSetupActor");
		const auto arenas = staging.GetActorsByTypeIncludingPending<ArenaSetupActor>();
		const auto encounters = staging.GetActorsByTypeIncludingPending<EncounterSetupActor>();
		const auto starts = staging.GetActorsByTypeIncludingPending<PlayerStart>();
		Require(arenas.size() == 1, "exactly one ArenaSetupActor is required");
		Require(encounters.size() == 1, "exactly one EncounterSetupActor is required");
		Require(starts.size() == 1, "exactly one PlayerStart is required");
		LevelLayout layout;
		layout.arena.legalBounds = *bounds;
		layout.arena.size = bounds->size;
		ResolveSetup(layout, *arenas[0].lock(), *encounters[0].lock());
		layout.respawn.spawnLocation = starts[0].lock()->GetActorLocation();
		layout.playerShip = starts[0].lock()->GetShip();
		Require(PlayerShip(layout.playerShip), "PlayerStart.Ship must name a player catalog ship: " + layout.playerShip);
		ValidateResolved(layout);
		for (const auto& handle : staging.GetActorsByTypeIncludingPending<DummyEnemy>())
		{
			const auto dummy = handle.lock();
			Require(dummy->GetHealthComponent().GetMaxHealth() > 0 && Finite(dummy->GetHealthComponent().GetMaxHealth()), "Dummy Max Health must be finite/positive");
		}
		layout.scene = scene;
		layout.fromScene = true;
		return layout;
	}

	LevelLayout LevelLayout::ReadArenaTest(EngineContext& context)
	{
		EngineContext::Scope scope{ context };
		RegisterTypesAndContent();
		const auto path = std::filesystem::path{ context.Assets().GetAssetRootDirectory() } / "scenes/arena-test.scene.json";
		if (!std::filesystem::exists(path))
		{
			VEGA_GAME_WARN("ArenaTest scene is absent (%s); using legacy C++ defaults.", path.string().c_str());
			return Defaults();
		}
		std::ifstream input{ path };
		Require(static_cast<bool>(input), "cannot open " + path.string());
		try { return Parse(Json::parse(input), context); }
		catch (const std::exception& error) { throw std::runtime_error(path.string() + ": " + error.what()); }
	}

	LevelLayout LevelLayout::ReadLevelOne(EngineContext& context)
	{
		EngineContext::Scope scope{ context };
		RegisterTypesAndContent();
		const auto path = std::filesystem::path{ context.Assets().GetAssetRootDirectory() } / "scenes/level-one.scene.json";
		if (!std::filesystem::exists(path))
		{
			VEGA_GAME_WARN("LevelOne scene is absent (%s); using legacy C++ defaults.", path.string().c_str());
			return LevelLayout{};
		}
		std::ifstream input{ path };
		Require(static_cast<bool>(input), "cannot open " + path.string());
		try
		{
			const auto scene = Json::parse(input);
			Require(scene.contains("settings") && scene["settings"].is_object() && scene["settings"].value("levelRules", std::string{}) == "LevelOne", "settings.levelRules must be LevelOne");
			return Parse(scene, context, "LevelOne");
		}
		catch (const std::exception& error) { throw std::runtime_error(path.string() + ": " + error.what()); }
	}

	SceneLoadResult LevelLayout::LoadEntities(World& world) const
	{
		if (!fromScene) return {};
		std::set<uint64_t> before;
		for (const auto& handle : world.GetActorsByTypeIncludingPending<Actor>()) if (const auto actor = handle.lock()) before.insert(actor->GetUniqueID());
		const auto unresolved = world.unresolvedPrefabs;
		try
		{
			PrefabLibrary prefabs{ world.GetEngineContext(), world.GetEngineContext().Prefabs().Root() };
			const auto result = SceneSerializer::Load(world, scene, &prefabs, false);
			Require(result.warnings.empty() && world.unresolvedPrefabs == unresolved, result.warnings.empty() ? "entity realization was incomplete" : result.warnings.front());
			return result;
		}
		catch (...)
		{
			for (const auto& handle : world.GetActorsByTypeIncludingPending<Actor>()) if (const auto actor = handle.lock(); !before.count(actor->GetUniqueID())) actor->Destroy();
			world.unresolvedPrefabs = unresolved;
			throw;
		}
	}

	void LevelLayout::ConfigureDummies(World& world) const
	{
		for (const auto& handle : world.GetActorsByTypeIncludingPending<DummyEnemy>())
		{
			const auto dummy = handle.lock();
			auto& attributes = dummy->GetAbilitySystemComponent().GetAttributes();
			attributes.SetBaseValue(OwnerAttributeIds::MaxHealth, dummy->GetHealthComponent().GetMaxHealth());
			auto definition = *content::ShipContentCatalog::FindById(dummy->GetShip());
			definition.health = dummy->GetHealthComponent().GetMaxHealth();
			// DummyArmor is the fallback contribution for chassis that author no Armor of
			// their own; a catalog entry that already carries Armor (the heavy dummy) keeps
			// its authored value instead of stacking a second contribution.
			const bool authorsArmor = std::any_of(definition.baseOwnerAttributes.begin(), definition.baseOwnerAttributes.end(),
				[](const OwnerAttributeBaseEntry& entry) { return entry.attributeId == OwnerAttributeIds::Armor; });
			if (!authorsArmor) definition.baseOwnerAttributes.push_back({ OwnerAttributeIds::Armor, dummyArmor });
			definition.energyAttributes.baseMaxShield = dummy->GetShipRuntime().GetAuthoredBaseMaxShield();
			// Rebind through the original ship system, including health-derived regen and
			// owned base contributions, rather than leaving a health component snapshot.
			dummy->GetShipRuntime().InitializeFromShipDefinition(definition);
		}
	}
}
