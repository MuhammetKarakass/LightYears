#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/reflection/Reflection.h"
#include "framework/scene/PrefabLibrary.h"
#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
	using Json = nlohmann::json;
	using namespace vega::reflect;
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}

	// A plain struct described once; its fields reuse the VEGA_FIELD lines of a class.
	struct WaveEntry
	{
		VEGA_STRUCT(WaveEntry)
		std::string prefab;
		int count = 1;
		float delay = 0.f;
	};
	VEGA_REFLECT_STRUCT(WaveEntry)
	{
		VEGA_FIELD("Prefab", prefab).Asset(vega::reflect::AssetKind::Prefab);
		VEGA_FIELD("Count", count).Range(1.f, 50.f);
		VEGA_FIELD("Delay", delay).Range(0.f, 60.f);
	}

	enum class Phase { Idle = 1, Active = 4 };
	struct Marker
	{
		VEGA_STRUCT(Marker)
		Phase phase = Phase::Idle;
		sf::Vector2f offset{ 1.f, 2.f };
		bool armed = false;
		vega::ActorRef target;
	};
	VEGA_REFLECT_STRUCT(Marker)
	{
		VEGA_FIELD("Phase", phase).Labels({ { 1, "Idle" }, { 4, "Active" } });
		VEGA_FIELD("Offset", offset);
		VEGA_FIELD("Armed", armed);
		VEGA_FIELD("Target", target).RequiredType("WaveSpawner");
	}

	class PlainActor : public vega::Actor
	{
		VEGA_CLASS(PlainActor)
	public:
		explicit PlainActor(vega::World* world = nullptr) : Actor{ world } {}
	};
	VEGA_REFLECT(PlainActor, vega::Actor)
	{
		VEGA_SPAWNABLE();
	}

	class WaveSpawner : public vega::Actor
	{
		VEGA_CLASS(WaveSpawner)
	public:
		explicit WaveSpawner(vega::World* world = nullptr) : Actor{ world } {}
	private:
		WaveEntry mBoss;
		std::vector<WaveEntry> mWaves;
		Marker mMarker;
	};
	VEGA_REFLECT(WaveSpawner, vega::Actor)
	{
		VEGA_SPAWNABLE();
		VEGA_FIELD("Boss", mBoss);
		VEGA_FIELD("Waves", mWaves);
		VEGA_FIELD("Marker", mMarker);
	}

	// Never registered, so a class that uses it must be refused at registration.
	struct Unregistered
	{
		VEGA_STRUCT(Unregistered)
		int value = 0;
	};
	class BrokenHolder
	{
		VEGA_CLASS(BrokenHolder)
	public:
		Unregistered item;
	};
	VEGA_REFLECT_ROOT(BrokenHolder)
	{
		VEGA_FIELD("Item", item);
	}

	// Struct fields are leaves in v1, so these two structs must be refused.
	struct NestedEntry
	{
		VEGA_STRUCT(NestedEntry)
		WaveEntry inner;
	};
	VEGA_REFLECT_STRUCT(NestedEntry)
	{
		VEGA_FIELD("Inner", inner);
	}
	struct ListEntry
	{
		VEGA_STRUCT(ListEntry)
		std::vector<int> values;
	};
	VEGA_REFLECT_STRUCT(ListEntry)
	{
		VEGA_FIELD("Values", values);
	}

	Json Entity(const std::string& type, Json properties)
	{
		return { { "guid", vega::SceneEntityComponent::GenerateGuid() }, { "name", "Test" }, { "type", type },
			{ "properties", std::move(properties) }, { "components", Json::array() }, { "scripts", Json::array() } };
	}
	Json Scene(Json entities)
	{
		return { { "format", "vega.scene" }, { "version", 1 }, { "settings", Json::object() }, { "entities", std::move(entities) } };
	}
	// Reads one named field from a struct value.
	LeafValue Leaf(const StructValue& value, const std::string& name)
	{
		for (const auto& entry : value) if (entry.first == name) return entry.second;
		return false;
	}
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	using vega::SceneSerializer;
	using vega::World;
	RegisterEngineTypes();
	VEGA_REGISTER(WaveEntry);
	VEGA_REGISTER(Marker);
	VEGA_REGISTER(PlainActor);
	VEGA_REGISTER(WaveSpawner);
	auto& registry = engineContext.Types();
	const auto* wave = registry.Find("WaveEntry");
	const auto* spawnerType = registry.Find("WaveSpawner");
	const auto* boss = spawnerType->FindProperty("Boss");
	const auto* waves = spawnerType->FindProperty("Waves");
	const auto* marker = spawnerType->FindProperty("Marker");

	// Registration: field order, metadata and the struct link are visible to tools.
	Check(wave && wave->root == TypeRoot::Other && wave->properties.size() == 3 && wave->properties[0].name == "Prefab" &&
		wave->properties[1].name == "Count" && wave->properties[2].name == "Delay", "Struct fields must keep declaration order");
	Check(wave->properties[0].assetKind == vega::reflect::AssetKind::Prefab && wave->properties[1].max == 50.f, "Struct fields must keep asset and range metadata");
	Check(boss->kind == PropertyKind::Struct && boss->structType == "WaveEntry" && boss->structInfo == wave, "Struct property must link its registered struct");
	Check(waves->kind == PropertyKind::List && waves->elementKind == PropertyKind::Struct && waves->structType == "WaveEntry", "List of structs must use Struct elements");

	World world{ nullptr };
	auto spawner = world.SpawnActor<WaveSpawner>().lock();
	spawner->AddComponent<vega::SceneEntityComponent>("", "Spawner");
	auto plain = world.SpawnActor<PlainActor>().lock();
	plain->AddComponent<vega::SceneEntityComponent>("", "Plain");
	void* root = spawner.get();
	std::string error;

	// Struct and list values round trip with ordered field names.
	const StructValue bossValue{ { "Prefab", std::string{ "enemy.prefab.json" } }, { "Count", 3 }, { "Delay", 1.5f } };
	Check(boss->SetValue(root, bossValue, &error), "Struct property must accept a complete value");
	Check(std::get<StructValue>(boss->getter(root)) == bossValue, "Struct value must read back with the same ordered fields");
	const PropertyList waveValues{
		StructValue{ { "Prefab", std::string{ "a.prefab.json" } }, { "Count", 2 }, { "Delay", 0.5f } },
		StructValue{ { "Prefab", std::string{} }, { "Count", 7 }, { "Delay", 0.f } } };
	Check(waves->SetValue(root, waveValues, &error) && std::get<PropertyList>(waves->getter(root)) == waveValues,
		"List of structs must round trip");

	// Field rules apply inside structs: Range clamps, labels are checked, and errors name the field path.
	Check(boss->SetValue(root, StructValue{ { "Count", 99 }, { "Delay", -4.f } }, &error) &&
		std::get<int>(Leaf(std::get<StructValue>(boss->getter(root)), "Count")) == 50 &&
		std::get<float>(Leaf(std::get<StructValue>(boss->getter(root)), "Delay")) == 0.f, "Struct fields must clamp to their Range");
	Check(boss->SetValue(root, StructValue{ { "Count", 0 } }, &error) && std::get<int>(Leaf(std::get<StructValue>(boss->getter(root)), "Count")) == 1,
		"Struct Int field must clamp to the lower bound");
	Check(!boss->SetValue(root, StructValue{ { "Count", 1.5f } }, &error) && error.find("Boss.Count") != std::string::npos,
		"A struct field of the wrong kind must be refused and named");
	Check(!marker->SetValue(root, StructValue{ { "Phase", 3 } }, &error) && error.find("Phase") != std::string::npos,
		"A labelled enum field must refuse unlisted values");
	Check(!waves->SetValue(root, PropertyList{ StructValue{ { "Count", 2 } }, StructValue{ { "Count", 2.5f } } }, &error) &&
		error.find("Waves[1].Count") != std::string::npos, "List element errors must name the element path");
	Check(std::get<PropertyList>(waves->getter(root)) == waveValues, "A refused list write must leave the list unchanged");

	// A struct ActorRef field keeps its requiredType: a plain actor is refused, a spawner is accepted.
	Check(!marker->SetValue(root, StructValue{ { "Target", vega::ActorRef{ *plain } } }, &error) && error == "Actor reference requires WaveSpawner",
		"Struct ActorRef fields must apply their requiredType");
	Check(marker->SetValue(root, StructValue{ { "Target", vega::ActorRef{ *spawner } } }, &error), "A matching struct ActorRef must be accepted");

	// Scene shape: fields use their own leaf encodings, and a list of structs is an array of objects.
	Check(boss->SetValue(root, bossValue, &error) && marker->SetValue(root, StructValue{ { "Phase", 4 }, { "Offset", sf::Vector2f{ 3.f, 4.f } },
		{ "Armed", true }, { "Target", vega::ActorRef{ *spawner } } }, &error), "Scene fixture values must be writable");
	const auto saved = SceneSerializer::Save(world);
	const auto& properties = saved["entities"][0]["properties"];
	const auto spawnerGuid = spawner->FindComponent<vega::SceneEntityComponent>()->guid;
	Check(properties["Boss"] == Json({ { "Prefab", "enemy.prefab.json" }, { "Count", 3 }, { "Delay", 1.5 } }),
		"Struct scene JSON must use field names and leaf encodings");
	Check(properties["Marker"] == Json({ { "Phase", "Active" }, { "Offset", Json::array({ 3.0, 4.0 }) }, { "Armed", true },
		{ "Target", { { "guid", spawnerGuid } } } }), "Enum struct fields must save as labels and actor fields by guid");
	Check(properties["Waves"] == Json::array({ { { "Prefab", "a.prefab.json" }, { "Count", 2 }, { "Delay", 0.5 } },
		{ { "Prefab", "" }, { "Count", 7 }, { "Delay", 0.0 } } }), "List of structs must save as an array of objects");

	// Unknown keys are ignored and missing keys keep the struct default.
	World loadedWorld{ nullptr };
	const auto loadResult = SceneSerializer::Load(loadedWorld, Scene(Json::array({ Entity("WaveSpawner",
		{ { "Boss", { { "Count", 7 }, { "Bogus", true } } } }) })));
	const auto loadedSpawner = loadedWorld.GetActorsByTypeIncludingPending<WaveSpawner>()[0].lock();
	const auto loadedBoss = loadedSpawner ? std::get<StructValue>(registry.Find("WaveSpawner")->FindProperty("Boss")->getter(loadedSpawner.get())) : StructValue{};
	Check(loadResult.warnings.empty() && loadedSpawner && std::get<int>(Leaf(loadedBoss, "Count")) == 7 &&
		std::get<std::string>(Leaf(loadedBoss, "Prefab")).empty() && std::get<float>(Leaf(loadedBoss, "Delay")) == 0.f,
		"Unknown struct keys must be ignored and missing keys must keep their defaults");

	// Save and load of both shapes must reproduce the same JSON.
	World reloaded{ nullptr };
	const auto reloadResult = SceneSerializer::Load(reloaded, saved);
	Check(reloadResult.warnings.empty() && SceneSerializer::Save(reloaded) == saved, "Struct scene values must save and load identically");

	// A prefab's list override is one unit, and a scene instance's whole-list override survives save and load.
	const std::filesystem::path prefabRoot = STRUCT_TEST_ROOT;
	std::filesystem::create_directories(prefabRoot);
	std::ofstream{ prefabRoot / "spawner.prefab.json" } << Json{ { "format", "vega.prefab" }, { "version", 1 },
		{ "base", { { "type", "WaveSpawner" } } },
		{ "properties", { { "Waves", Json::array({ { { "Prefab", "base.prefab.json" }, { "Count", 1 }, { "Delay", 0.0 } } }) } } } }.dump();
	engineContext.Prefabs().SetRoot(prefabRoot);
	Json instance{ { "guid", "spawn-1" }, { "name", "Instance" }, { "prefab", "spawner.prefab.json" },
		{ "properties", { { "Waves", Json::array({ { { "Prefab", "x.prefab.json" }, { "Count", 4 }, { "Delay", 2.0 } }, { { "Count", 9 } } }) } } } };
	World prefabWorld{ nullptr };
	const auto prefabLoad = SceneSerializer::Load(prefabWorld, Scene(Json::array({ instance })));
	Check(prefabLoad.warnings.empty(), "Prefab instance with a struct list must load without warnings");
	const auto prefabSaved = SceneSerializer::Save(prefabWorld);
	const auto& prefabEntity = prefabSaved["entities"][0];
	Check(prefabEntity["prefab"] == "spawner.prefab.json" && !prefabEntity["properties"].contains("Boss") &&
		prefabEntity["properties"]["Waves"] == Json::array({ { { "Prefab", "x.prefab.json" }, { "Count", 4 }, { "Delay", 2.0 } },
			{ { "Prefab", "" }, { "Count", 9 }, { "Delay", 0.0 } } }),
		"A struct list override must be saved as one whole-list property");
	World prefabReloaded{ nullptr };
	const auto prefabReload = SceneSerializer::Load(prefabReloaded, prefabSaved);
	Check(prefabReload.warnings.empty() && SceneSerializer::Save(prefabReloaded) == prefabSaved, "Struct list overrides must survive save and load");

	// Registration refuses a struct that is unknown or that holds a struct or list.
	try
	{
		VEGA_REGISTER(BrokenHolder);
		Check(false, "A struct field of an unregistered struct must be refused");
	}
	catch (const std::logic_error&) {}
	try
	{
		VEGA_REGISTER(NestedEntry);
		Check(false, "A struct field that is a struct must be refused");
	}
	catch (const std::logic_error&) {}
	try
	{
		VEGA_REGISTER(ListEntry);
		Check(false, "A struct field that is a list must be refused");
	}
	catch (const std::logic_error&) {}

	if (failures) return 1;
	std::cout << "Reflection struct tests passed\n";
	return 0;
}
