#include "framework/EngineContext.h"
#include <framework/Actor.h>
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <framework/reflection/Reflection.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace
{
	using Json = nlohmann::json;
	int failures = 0;
	void Check(bool ok, const char* message)
	{
		if (!ok) { ++failures; std::cerr << message << '\n'; }
	}
	void Near(sf::Vector2f actual, sf::Vector2f expected, const char* message)
	{
		Check((actual - expected).length() < .002f, message);
	}
	class HierarchyActor : public vega::Actor
	{
	public:
		using Actor::Actor;
		float GetPhysicsCollisionRadius() const override { return 5.f; }
	};
	Json Scene(Json entities)
	{
		return { { "format", "vega.scene" }, { "version", 1 }, { "settings", Json::object() }, { "entities", std::move(entities) } };
	}
	std::shared_ptr<vega::Actor> Find(vega::World& world, const std::string& guid)
	{
		for (const auto& weak : world.GetActorsByTypeIncludingPending<vega::Actor>())
			if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy())
				if (const auto* id = actor->FindComponent<vega::SceneEntityComponent>(); id && id->guid == guid) return actor;
		return {};
	}
	void Write(const std::string& path, const Json& document)
	{
		std::ofstream{ std::filesystem::path{ HIERARCHY_TEST_ROOT } / path } << document.dump();
	}
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	vega::reflect::RegisterEngineTypes();
	vega::reflect::Class<HierarchyActor, vega::Actor>("HierarchyActor").Spawnable();
	{
		vega::World world{ engineContext };
		auto root = world.SpawnActor<HierarchyActor>().lock();
		auto& identity = root->AddComponent<vega::SceneEntityComponent>("root", "Root");
		identity.prefab = "prefabs/unused.prefab.json";
		for (const auto* guid : { "first", "b/c", "b_c" })
		{
			auto child = world.SpawnActor<HierarchyActor>().lock();
			child->AddComponent<vega::SceneEntityComponent>(guid, guid).childId = "same";
			Check(child->AttachTo(root.get()), "Serialization fixture attachment failed");
		}
		const auto subtree = vega::SceneSerializer::SaveSubtree(*root, true);
		Check(subtree["children"].dump() == vega::SceneSerializer::SaveEntity(*root, true)["children"].dump(), "SaveEntity and SaveSubtree child bytes must match");
		Check(subtree["children"][0]["id"] == "same" && subtree["children"][1]["id"] == "b_c" && subtree["children"][2]["id"] == "b_c_", "Child ID collision spelling must remain byte-identical");
		vega::World copy{ engineContext };
		Check(vega::SceneSerializer::Load(copy, Scene(Json::array({ subtree }))).warnings.empty(), "Child ID round trip load failed");
		Check(vega::SceneSerializer::SaveSubtree(*Find(copy, "root"), true).dump() == subtree.dump(), "Child ID round trip bytes changed");
	}
	{
		vega::World world{ nullptr }, other{ nullptr };
		auto parent = world.SpawnActor<HierarchyActor>().lock();
		auto child = world.SpawnActor<HierarchyActor>().lock();
		auto grandchild = world.SpawnActor<HierarchyActor>().lock();
		parent->SetActorLocation({ 10, 20 });
		child->SetActorLocation({ 20, 20 });
		Check(child->AttachTo(parent.get()), "Attach failed");
		Near(child->GetRelativeLocation(), { 10, 0 }, "Keep-world relative transform");
		parent->SetActorRotation(90);
		Near(child->GetActorLocation(), { 10, 30 }, "Rotated parent must orbit child");
		Check(std::abs(child->GetActorRotation() - 90.f) < .001f, "Rotation composition");
		child->SetActorLocation({ 10, 40 });
		Near(child->GetRelativeLocation(), { 20, 0 }, "World setter must invert parent transform");
		child->SetActorRotation(120);
		Check(std::abs(child->GetRelativeRotation() - 30.f) < .001f, "World rotation setter");
		grandchild->SetRelativeLocation({ 5, 0 });
		Check(grandchild->AttachTo(child.get(), false), "Keep-relative attach");
		Near(grandchild->GetActorLocation(), { 7.5f, 44.330127f }, "Grandchild composition");
		Check(!parent->AttachTo(grandchild.get()) && !parent->AttachTo(parent.get()), "Cycles must be rejected");
		Check(!child->AttachTo(other.SpawnActor<HierarchyActor>().lock().get()), "Cross-world parent rejected");
		const auto location = child->GetActorLocation();
		child->Detach();
		Near(child->GetActorLocation(), location, "Detach keeps world");
		Check(child->GetActorRotation() == 120.f && parent->GetChildren().empty(), "Detach rotation and membership");
		child->SetEnablePhysics(true);
		Check(!child->AttachTo(parent.get()), "Dynamic body attachment rejected");
		child->SetPhysicsBodyType(vega::PhysicsBodyType::Static);
		Check(child->AttachTo(parent.get()), "Static body can follow parent");
		parent->SetActorLocation({ 50, 60 });
		const auto bodyLocation = child->GetActorLocation();
		world.StepPhysics(.016f);
		Near(child->GetActorLocation(), bodyLocation, "Physics must not overwrite attachment");
		child->SetPhysicsBodyType(vega::PhysicsBodyType::Dynamic);
		Check(!child->GetParent(), "Switching to dynamic detaches");
		child->SetEnablePhysics(false);
		Check(child->AttachTo(parent.get()), "Reattach after disabling physics");
		parent->Destroy();
		Check(child->GetIsPendingDestroy() && grandchild->GetIsPendingDestroy(), "Destroy must cascade");
		Check(!child->AttachTo(other.SpawnActor<HierarchyActor>().lock().get()), "Dead actor cannot reattach");
	}
	{
		vega::World world{ nullptr };
		const auto loaded = vega::SceneSerializer::Load(world, Scene(Json::array({
			{ { "guid", "child" }, { "type", "HierarchyActor" }, { "parent", "parent" }, { "properties", { { "Location", { 10, 0 } }, { "Rotation", 15 } } } },
			{ { "guid", "parent" }, { "type", "HierarchyActor" }, { "properties", { { "Location", { 100, 200 } }, { "Rotation", 90 } } } }
		})));
		Check(loaded.spawned == 2 && loaded.warnings.empty(), "Forward parent references load");
		Near(Find(world, "child")->GetActorLocation(), { 100, 210 }, "Scene uses relative Location");
		const auto saved = vega::SceneSerializer::Save(world);
		Check(saved["entities"][0]["guid"] == "parent" && saved["entities"][1]["parent"] == "parent", "Save parents first");
		vega::World copy{ nullptr };
		Check(vega::SceneSerializer::Load(copy, saved).warnings.empty(), "Hierarchy round trip");
		Check(vega::SceneSerializer::Save(copy) == saved, "Round trip must preserve hierarchy values");
		auto subtree = Json::array({ vega::SceneSerializer::SaveSubtree(*Find(world, "parent")) });
		vega::SceneSerializer::RemapGuids(subtree);
		Check(subtree[0]["children"][0]["parent"] == subtree[0]["guid"], "Duplicate remaps internal parent links");
		Check(vega::SceneSerializer::Load(world, Scene(subtree)).spawned == 2, "Remapped subtree can coexist");
	}
	std::filesystem::create_directories(std::filesystem::path{ HIERARCHY_TEST_ROOT } / "prefabs");
	vega::PrefabLibrary::Get().SetRoot(HIERARCHY_TEST_ROOT);
	const Json leaf{ { "format", "vega.prefab" }, { "version", 1 }, { "name", "Leaf" }, { "base", { { "type", "HierarchyActor" } } },
		{ "children", Json::array({ { { "id", "tip" }, { "name", "Tip" }, { "type", "HierarchyActor" }, { "properties", { { "Location", { 5, 0 } } } } } }) } };
	Write("prefabs/Leaf.prefab.json", leaf);
	Json tree{ { "format", "vega.prefab" }, { "version", 1 }, { "name", "Tree" }, { "base", { { "type", "HierarchyActor" } } },
		{ "children", Json::array({ { { "id", "left" }, { "prefab", "prefabs/Leaf.prefab.json" }, { "properties", { { "Location", { 10, 0 } } } } },
			{ { "id", "right" }, { "prefab", "prefabs/Leaf.prefab.json" }, { "properties", { { "Location", { -10, 0 } } } } } }) } };
	Write("prefabs/Tree.prefab.json", tree);
	const auto resolved = vega::PrefabLibrary::Get().Resolve("prefabs/Tree.prefab.json");
	Check(resolved && resolved.warnings.empty(), "Repeated nested prefab references are not cycles");
	if (resolved)
	{
		auto edited = resolved.entity;
		edited["children"][0]["children"][0]["properties"]["Rotation"] = 42;
		const auto delta = vega::PrefabLibrary::Diff(edited, resolved.entity);
		Check(vega::PrefabLibrary::Merge(resolved.entity, delta) == edited, "Stable-id nested child diff/merge");
		std::swap(edited["children"][0], edited["children"][1]);
		Check(vega::PrefabLibrary::Diff(edited, resolved.entity) == delta, "Child override addressing survives reorder");
		vega::World world{ nullptr };
		const auto loaded = vega::SceneSerializer::Load(world, Scene(Json::array({ { { "guid", "one" }, { "prefab", "prefabs/Tree.prefab.json" },
			{ "properties", { { "Location", { 100, 100 } }, { "Rotation", 90 } } } }, { { "guid", "two" }, { "prefab", "prefabs/Tree.prefab.json" } } })));
		Check(loaded.spawned == 10 && loaded.warnings.empty(), "Two hierarchy instances spawn independently");
		Near(Find(world, "one/left/tip")->GetActorLocation(), { 100, 115 }, "Nested prefab transform");
		Find(world, "one/left/tip")->SetRelativeRotation(42);
		const auto saved = vega::SceneSerializer::Save(world);
		Check(saved["entities"].size() == 2, "Instance children must not be saved twice");
		vega::World copy{ nullptr };
		Check(vega::SceneSerializer::Load(copy, saved).spawned == 10, "Hierarchy prefab round trip");
		Check(Find(copy, "one/left/tip")->GetRelativeRotation() == 42 && Find(copy, "two/left/tip")->GetRelativeRotation() == 0, "Child override isolation");
		Check(vega::SceneSerializer::Save(copy) == saved, "Prefab round trip stable");
		auto extra = copy.SpawnActor<HierarchyActor>().lock();
		extra->AddComponent<vega::SceneEntityComponent>("extra", "Added child");
		Check(extra->AttachTo(Find(copy, "two").get()), "Attach a plain actor to an instance");
		const auto withExtra = vega::SceneSerializer::Save(copy);
		Check(withExtra["entities"].size() == 2, "Added instance child must not serialize twice");
		vega::World extraCopy{ nullptr };
		Check(vega::SceneSerializer::Load(extraCopy, withExtra).spawned == 11 && Find(extraCopy, "extra")->GetParent() == Find(extraCopy, "two").get(), "Added instance child round trip");
		Find(copy, "one")->Destroy();
		Check(!Find(copy, "one/left/tip") && Find(copy, "two/left/tip"), "Instance destroy isolation");
	}
	tree["children"][0]["prefab"] = "prefabs/Tree.prefab.json";
	Write("prefabs/Cycle.prefab.json", tree);
	tree["children"][0]["prefab"] = "prefabs/Cycle.prefab.json";
	Write("prefabs/Tree.prefab.json", tree);
	Check(!vega::PrefabLibrary::Get().Resolve("prefabs/Cycle.prefab.json"), "Nested prefab cycle rejected");
	tree["children"][1]["id"] = "left";
	Write("prefabs/Duplicate.prefab.json", tree);
	Check(!vega::PrefabLibrary::Get().Resolve("prefabs/Duplicate.prefab.json"), "Duplicate stable child ids rejected");
	return failures ? 1 : 0;
}
