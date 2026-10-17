#include "framework/ProjectSettings.h"
#include "framework/EngineContext.h"
#include "framework/Actor.h"
#include "framework/CollisionComponent.h"
#include "framework/reflection/Reflection.h"
#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/scene/PrefabLibrary.h"
#include "framework/debug/Log.h"
#include <fstream>
#include <iostream>

namespace
{
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { ++failures; std::cerr << message << '\n'; }
	}
	const vega::reflect::PropertyInfo& Mask(vega::EngineContext& context)
	{
		return *context.Types().Find("CollisionComponent")->FindProperty("Collision Mask");
	}
}

int main()
{
	using Json = nlohmann::json;
	vega::EngineContext first, second;
	auto document = vega::ProjectSettings{}.ToJson();
	document["collisionLayers"] = Json::array({
		{ { "bit", 0 }, { "name", "Solid" }, { "aliases", { "OldSolid" } } },
		{ { "bit", 7 }, { "name", "Sensor" } } });
	const auto project = vega::ProjectSettings::Parse(document);
	Check(project.ToJson() == document, "Project layer names/aliases did not survive save");
	project.ApplyCollisionLayers(first);
	vega::ProjectSettings{}.ApplyCollisionLayers(second);
	vega::reflect::Class<vega::Actor>("Actor", first.Types()).Spawnable();
	vega::reflect::Class<vega::Actor>("Actor", second.Types()).Spawnable();
	Check(Mask(first).enumLabels[0].second == "Solid" && Mask(first).enumLabels[7].second == "Sensor" &&
		Mask(first).enumLabels[1].second == "Layer 1", "Custom and unspecified labels were not installed");
	Check(Mask(second).enumLabels[0].second == "Layer 0" && &Mask(first) != &Mask(second), "Contexts share collision descriptors");
	vega::reflect::RegisterEngineTypes(first.Types());
	Check(Mask(first).enumLabels[0].second == "Solid", "Repeated engine registration reset project labels");

	vega::World source{ first };
	auto actor = source.SpawnActor<vega::Actor>().lock();
	actor->AddComponent<vega::SceneEntityComponent>("collision", "Collision");
	auto& collision = actor->AddComponent<vega::CollisionComponent>();
	collision.SetCollisionLayer(CollisionLayer::Layer0);
	collision.SetCollisionMask(CollisionLayer::Layer0 | CollisionLayer::Layer7);
	const auto saved = vega::SceneSerializer::Save(source);
	const auto& flags = saved["entities"][0]["components"][0]["properties"];
	Check(flags["Collision Layer"] == Json::array({ "Solid" }) && flags["Collision Mask"] == Json::array({ "Solid", "Sensor" }),
		"Serializer did not use context labels in bit order");
	vega::World reopened{ first };
	Check(vega::SceneSerializer::Load(reopened, saved).warnings.empty() && vega::SceneSerializer::Save(reopened).dump() == saved.dump(),
		"Project-labelled scene did not round-trip byte-identically");
	const std::filesystem::path root{ COLLISION_TEST_ROOT };
	std::filesystem::create_directories(root);
	const Json prefab = { { "format", "vega.prefab" }, { "version", 1 }, { "name", "Collider" },
		{ "base", { { "type", "Actor" } } }, { "components", saved["entities"][0]["components"] } };
	std::ofstream{ root / "Collider.prefab.json" } << prefab.dump();
	first.Prefabs().SetRoot(root);
	const auto resolved = first.Prefabs().Resolve("Collider.prefab.json");
	Check(resolved && resolved.warnings.empty() && resolved.entity["components"] == prefab["components"],
		"Prefab staging did not use the owning context's collision labels");
	for (const auto& value : { Json("OldSolid | Sensor"), Json::array({ "Layer 0", "Sensor" }), Json("129"), Json(129) })
	{
		auto legacy = saved;
		legacy["entities"][0]["components"][0]["properties"]["Collision Mask"] = value;
		vega::World loaded{ first };
		const auto result = vega::SceneSerializer::Load(loaded, legacy);
		Check(result.warnings.empty() == value.is_number_integer(), "Alias/numeric string fallback must warn; native numeric masks remain supported");
		Check(vega::SceneSerializer::Save(loaded) == saved, "Alias or numeric mask changed bits");
	}
	int warnings = 0;
	const auto sink = vega::debug::Log::AddSink([&](const vega::debug::LogRecord& record)
	{
		if (record.level == vega::debug::LogLevel::Warning && record.message.find("collisionLayers") != std::string::npos) ++warnings;
	});
	for (const auto& invalid : { Json(nullptr), Json::object(), Json::array({ { { "bit", 8 }, { "name", "Bad" } } }),
		Json::array({ { { "bit", -1 }, { "name", "Bad" } } }), Json::array({ { { "bit", 1.5 }, { "name", "Bad" } } }),
		Json::array({ { { "bit", 0 }, { "name", "Same" } }, { { "bit", 1 }, { "name", "Same" } } }),
		Json::array({ { { "bit", 0 }, { "name", "One" } }, { { "bit", 0 }, { "name", "Two" } } }),
		Json::array({ { { "bit", 0 }, { "name", "One" }, { "aliases", { "Layer 1" } } } }),
		Json::array({ { { "bit", 0 }, { "name", "One" }, { "aliases", { "One" } } } }),
		Json::array({ { { "bit", 0 }, { "name", "" } } }), Json::array({ { { "bit", 0 }, { "name", "None" } } }),
		Json::array({ { { "bit", 0 }, { "name", "One" }, { "aliases", 42 } } }) })
	{
		auto bad = document;
		bad["collisionLayers"] = invalid;
		const int before = warnings;
		vega::ProjectSettings::Parse(bad).ApplyCollisionLayers(first);
		Check(warnings == before + 1 && Mask(first).enumLabels == Mask(second).enumLabels && Mask(first).flagAliases.empty(),
			"Invalid layer configuration must warn once and restore neutral defaults");
	}
	vega::debug::Log::RemoveSink(sink);
	// Updating one host must never reset the other host's descriptors or values.
	project.ApplyCollisionLayers(first);
	Check(Mask(second).enumLabels[0].second == "Layer 0" && Mask(first).enumLabels[0].second == "Solid",
		"Applying settings mutated a different registry");
	vega::ProjectSettings::Load(std::filesystem::path{ COLLISION_SOURCE_ROOT } / "Templates/Blank/Assets/vega.project.json").ApplyCollisionLayers(second);
	int legacyBlankBits = 0;
	bool legacyBlank = false;
	Check(Mask(second).ParseFlags("Player | Powerup", legacyBlankBits, &legacyBlank) && legacyBlankBits == 17 && legacyBlank,
		"Existing Blank sample labels must retain bits 0 and 4 through the Pickup alias");
	return failures ? 1 : 0;
}
