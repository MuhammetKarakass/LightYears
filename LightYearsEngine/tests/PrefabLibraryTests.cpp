#include "framework/scene/PrefabLibrary.h"
#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/reflection/Reflection.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include <fstream>
#include <chrono>
#include <iostream>

namespace
{
	using Json = nlohmann::json;
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition) { std::cerr << message << '\n'; ++failures; }
	}
	class Part : public ly::ActorComponent
	{
	public:
		using ActorComponent::ActorComponent;
		int value = 10;
	};
	class Rock : public ly::Actor
	{
	public:
		explicit Rock(ly::World* world) : Actor{ world } { AddComponent<Part>(); AddComponent<Part>().value = 20; }
		int size = 1;
	};
	Json Scene(Json entities)
	{
		return { { "format", "vega.scene" }, { "version", 1 }, { "settings", Json::object() }, { "entities", entities } };
	}
	void Write(const std::string& name, const Json& value)
	{
		std::ofstream{ std::filesystem::path(PREFAB_TEST_ROOT) / "prefabs" / (name + ".prefab.json") } << value.dump();
	}
	Json Prefab(Json base)
	{
		return { { "format", "vega.prefab" }, { "version", 1 }, { "name", "Rock" }, { "base", base } };
	}
}
int main()
{
	using namespace ly;
	using namespace ly::reflect;
	RegisterEngineTypes();
	Class<Part, ActorComponent>("PrefabPart").Addable()
		.Property("Value", [](const Part& p) { return p.value; }, [](Part& p, int v) { p.value = v; });
	Class<Rock, Actor>("PrefabRock").Spawnable()
		.Property("Size", [](const Rock& p) { return p.size; }, [](Rock& p, int v) { p.size = v; });
	std::filesystem::create_directories(std::filesystem::path(PREFAB_TEST_ROOT) / "prefabs");
	auto& library = PrefabLibrary::Get();
	library.SetRoot(PREFAB_TEST_ROOT);
	auto rock = Prefab({ { "type", "PrefabRock" } });
	rock["properties"] = { { "Size", 3 } };
	rock["components"] = Json::array({ { { "type", "PrefabPart" }, { "properties", { { "Value", 30 } } } } });
	Write("Rock", rock);
	auto variant = Prefab({ { "prefab", "prefabs/Rock.prefab.json" } });
	variant["properties"] = { { "Size", 5 } };
	Write("BigRock", variant);
	auto resolved = library.Resolve("prefabs/BigRock.prefab.json");
	Check(resolved && resolved.warnings.empty(), "Variant must resolve without warnings");
	Check(resolved.entity["properties"]["Size"] == 5, "Child property must win");
	Check(resolved.entity["components"][0]["properties"]["Value"] == 30, "Parent component property must inherit");
	Check(resolved.entity["components"][1]["properties"]["Value"] == 20, "Constructor defaults must inherit by occurrence");
	Check(!resolved.entity["properties"].contains("Location"), "Prefab must not own placement");
	const auto resolutions = library.ResolveCount();
	Check(library.Resolve("prefabs/./BigRock.prefab.json").entity == resolved.entity && library.ResolveCount() == resolutions,
		"Canonical repeated resolution must reuse the realized entity");
	const auto parentPath = std::filesystem::path(PREFAB_TEST_ROOT) / "prefabs/Rock.prefab.json";
	const auto parentTime = std::filesystem::last_write_time(parentPath);
	std::filesystem::last_write_time(parentPath, parentTime + std::chrono::seconds(2));
	Check(library.Resolve("prefabs/BigRock.prefab.json").entity == resolved.entity && library.ResolveCount() == resolutions + 1,
		"Touching a parent must invalidate the cached child even when size is unchanged");
	const auto refreshedCount = library.ResolveCount();
	library.Resolve("prefabs/BigRock.prefab.json");
	Check(library.ResolveCount() == refreshedCount, "Unchanged parent chain must stay cached");
	auto warningPrefab = rock;
	warningPrefab["components"].push_back({ { "type", "UnavailablePart" } });
	Write("Warning", warningPrefab);
	const auto warningResult = library.Resolve("prefabs/Warning.prefab.json");
	const auto warningCount = library.ResolveCount();
	const auto warningAgain = library.Resolve("prefabs/Warning.prefab.json");
	Check(warningResult && !warningResult.warnings.empty() && warningAgain.warnings == warningResult.warnings && library.ResolveCount() == warningCount,
		"Cached successful resolutions must report their warnings on every use");
	Json instance{ { "guid", "test-guid" }, { "name", "Instance" }, { "prefab", "prefabs/BigRock.prefab.json" },
		{ "properties", { { "Size", 7 }, { "Location", { 20, 40 } } } } };
	World world{ nullptr };
	const auto loaded = SceneSerializer::Load(world, Scene(Json::array({ instance })));
	Check(loaded.spawned == 1 && loaded.warnings.empty(), "Instance must load");
	const auto saved = SceneSerializer::Save(world);
	Check(saved["entities"][0] == instance, "Instance must save only explicit differences and placement");
	World second{ nullptr };
	Check(SceneSerializer::Load(second, saved).spawned == 1 && SceneSerializer::Save(second) == saved, "Sparse override round trip must be stable");
	const auto actor = world.GetActorsByTypeIncludingPending<Actor>().front().lock();
	auto full = SceneSerializer::SaveEntity(*actor, true);
	full["components"][1]["properties"]["Value"] = 99;
	const auto delta = PrefabLibrary::Diff(full, resolved.entity);
	Check(delta["components"].size() == 2 && !delta["components"][0].contains("properties"), "Repeated component addressing must retain a leading empty occurrence");
	Check(PrefabLibrary::Merge(resolved.entity, delta)["components"][1]["properties"]["Value"] == 99, "Repeated component override must merge");
	auto composition = resolved.entity;
	composition["components"].erase(composition["components"].begin() + 1);
	const auto removal = PrefabLibrary::Diff(composition, resolved.entity);
	Check(removal["removed"].size() == 1, "Component removal must diff");
	// Same-type components address surviving occurrences, just like scene entities.
	const auto merged = PrefabLibrary::Merge(resolved.entity, removal);
	World removed{ nullptr };
	Check(SceneSerializer::Load(removed, Scene(Json::array({ merged }))).spawned == 1, "Composition overrides must load");
	auto addition = resolved.entity;
	addition["components"].push_back({ { "type", "PrefabPart" }, { "added", true }, { "properties", { { "Value", 77 }, { "Active", true } } } });
	const auto addedDelta = PrefabLibrary::Diff(addition, resolved.entity);
	Check(PrefabLibrary::Merge(resolved.entity, addedDelta)["components"].size() == 3, "Added component must merge once");
	Json scriptsBase{ { "scripts", Json::array({ { { "path", "scripts/spin.lua" }, { "properties", { { "speed", 45 }, { "enabled", true } } } } }) } };
	auto scriptsChanged = scriptsBase;
	scriptsChanged["scripts"][0]["properties"]["speed"] = 90;
	const auto scriptDelta = PrefabLibrary::Diff(scriptsChanged, scriptsBase);
	Check(scriptDelta["scripts"][0]["properties"].size() == 1, "Script diff must omit inherited properties");
	Check(PrefabLibrary::Merge(scriptsBase, scriptDelta)["scripts"] == scriptsChanged["scripts"], "Script property override must merge");
	scriptsChanged["scripts"][0]["properties"].erase("speed");
	Check(PrefabLibrary::Merge(scriptsBase, PrefabLibrary::Diff(scriptsChanged, scriptsBase))["scripts"] == scriptsChanged["scripts"], "Removing an inherited script property must round trip");
	scriptsChanged["scripts"] = Json::array();
	Check(PrefabLibrary::Merge(scriptsBase, PrefabLibrary::Diff(scriptsChanged, scriptsBase))["scripts"] == scriptsChanged["scripts"], "Empty script list must remove scripts");
	const Json missing{ { "prefab", "prefabs/Missing.prefab.json" }, { "guid", "missing" }, { "name", "Keep me" }, { "future", { 1, 2, 3 } } };
	World absent{ nullptr };
	const auto unavailable = SceneSerializer::Load(absent, Scene(Json::array({ missing })));
	Check(unavailable.spawned == 0 && !unavailable.warnings.empty(), "Missing prefab must warn without spawning");
	Check(SceneSerializer::Save(absent)["entities"][0] == missing, "Missing prefab authored data must survive verbatim");
	Check(SceneSerializer::Load(absent, Scene(Json::array({ missing }))).spawned == 0 && absent.unresolvedPrefabs.size() == 1, "Missing prefab GUID must remain reserved");
	Write("CycleA", Prefab({ { "prefab", "prefabs/CycleB.prefab.json" } }));
	Write("CycleB", Prefab({ { "prefab", "prefabs/./CycleA.prefab.json" } }));
	const auto cycle = library.Resolve("prefabs/CycleA.prefab.json");
	Check(!cycle && !cycle.warnings.empty(), "Canonicalized cycle must warn without throwing");
	Write("Unknown", Prefab({ { "type", "MissingType" } }));
	Check(!library.Resolve("prefabs/Unknown.prefab.json"), "Unknown base must not resolve");
	Write("Invalid", Json{ { "base", 7 } });
	Check(!library.Resolve("prefabs/Invalid.prefab.json"), "Malformed document must warn");
	Check(!library.Resolve("../outside.prefab.json"), "Escaping paths must be refused");
	for (int i = 0; i < 66; ++i)
		Write("Depth" + std::to_string(i), Prefab(i == 65 ? Json{ { "type", "PrefabRock" } } : Json{ { "prefab", "prefabs/Depth" + std::to_string(i + 1) + ".prefab.json" } }));
	Check(!library.Resolve("prefabs/Depth0.prefab.json"), "Depth limit must warn");
	rock["properties"]["Size"] = 12345;
	rock["components"][0]["properties"]["Value"] = 54321;
	Write("Rock", rock);
	Check(library.Resolve("prefabs/BigRock.prefab.json").entity["properties"]["Size"] == 5, "Variant override must survive parent file changes");
	Check(library.Resolve("prefabs/BigRock.prefab.json").entity["components"][0]["properties"]["Value"] == 54321,
		"Parent file changes must update inherited properties in cached children");
	Check(library.Resolve("prefabs/Rock.prefab.json").entity["properties"]["Size"] == 12345, "File changes must invalidate cached documents");
	library.Reload();
	const auto reloadCount = library.ResolveCount();
	Check(library.Resolve("prefabs/Rock.prefab.json").entity["properties"]["Size"] == 12345, "Explicit reload must preserve resolution");
	Check(library.ResolveCount() == reloadCount + 1, "Reload must discard realized entities");
	library.SetRoot(PREFAB_TEST_ROOT);
	library.Resolve("prefabs/Rock.prefab.json");
	Check(library.ResolveCount() == reloadCount + 2, "SetRoot must discard realized entities");
	library.Resolve("prefabs/BigRock.prefab.json");
	std::filesystem::remove(parentPath);
	Check(!library.Resolve("prefabs/BigRock.prefab.json"), "Removing a parent must invalidate a cached child");
	Write("Rock", rock);
	Check(static_cast<bool>(library.Resolve("prefabs/BigRock.prefab.json")), "Restoring a missing parent must recover the child");
	return failures ? 1 : 0;
}
