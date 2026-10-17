#include "framework/EngineContext.h"
#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/SpriteComponent.h>
#include <framework/CollisionComponent.h>
#include <framework/ProjectSettings.h>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <stdexcept>

namespace
{
	class MultiActor : public vega::Actor
	{
	public:
		using Actor::Actor;
		bool reject = false;
		int value = 0;
		std::vector<sf::Vector2f> points;
		vega::ActorRef target;
	};
	// A reflected struct used by the struct Inspector and editor-script paths; members are public for assertions.
	struct StructWave
	{
		VEGA_STRUCT(StructWave)
		std::string prefab;
		int count = 1;
		float delay = 0.f;
		bool armed = false;
	};
	class StructHost : public vega::Actor
	{
	public:
		using Actor::Actor;
		StructWave boss;
		std::vector<StructWave> waves;
	};
	class SharedComponent : public vega::ActorComponent
	{
	public:
		using ActorComponent::ActorComponent;
		int strength = 1;
	};
	class FirstComponent : public SharedComponent
	{
	public:
		using SharedComponent::SharedComponent;
	};
	class SecondComponent : public SharedComponent
	{
	public:
		using SharedComponent::SharedComponent;
	};
	int getterReads = 0;
	int failures = 0;
	void Check(bool value, const char* message)
	{
		if (!value) { std::cerr << message << '\n'; ++failures; }
	}
	bool Near(sf::Vector2f a, sf::Vector2f b)
	{
		return std::abs(a.x - b.x) < .001f && std::abs(a.y - b.y) < .001f;
	}

	void TestMultiCommands()
	{
		using namespace vega;
		vega::World world{ nullptr };
		EditorSelection selection;
		EditorContext context{ &world, selection };
		EditorCamera camera;
		context.camera = &camera;
		CommandHistory history{ context };
		auto a = world.SpawnActor<MultiActor>().lock();
		auto b = world.SpawnActor<MultiActor>().lock();
		auto c = world.SpawnActor<vega::Actor>().lock();
		const auto guidA = a->AddComponent<vega::SceneEntityComponent>().guid;
		const auto guidB = b->AddComponent<vega::SceneEntityComponent>().guid;
		const auto guidC = c->AddComponent<vega::SceneEntityComponent>().guid;
		a->SetActorLocation({ 0.f, 0.f });
		b->SetActorLocation({ 100.f, 0.f });
		c->SetActorLocation({ 50.f, 30.f });
		a->AddComponent<FirstComponent>();
		b->AddComponent<SecondComponent>();
		a->AddComponent<vega::SpriteComponent>();
		b->AddComponent<vega::SpriteComponent>();
		selection.Add(a); selection.Add(b); selection.Add(c);
		const auto starts = CaptureSelectionTransform(selection);
		const auto group = history.BeginMergeGroup();
		Check(TransformSelection(context, history, starts, selection.Centre(), { 10.f, 20.f }, {}, group), "Group move failed");
		Check(TransformSelection(context, history, starts, { 50.f, 10.f }, { 20.f, 40.f }, {}, group), "Second drag frame failed");
		Check(history.UndoCount() == 1 && Near(a->GetActorLocation(), { 20.f, 40.f }) && Near(c->GetActorLocation(), { 70.f, 70.f }), "Drag must merge all actors into one undo entry");
		Check(history.Undo() && Near(a->GetActorLocation(), {}) && Near(b->GetActorLocation(), { 100.f, 0.f }) && Near(c->GetActorLocation(), { 50.f, 30.f }), "One undo must restore all three actors");
		Check(history.Redo() && Near(b->GetActorLocation(), { 120.f, 40.f }), "Group redo failed");
		history.Undo(); history.Clear();
		selection.Remove(c);
		const auto rotateStarts = CaptureSelectionTransform(selection);
		const auto rotationGroup = history.BeginMergeGroup();
		Check(TransformSelection(context, history, rotateStarts, { 50.f, 0.f }, {}, 45.f, rotationGroup), "Group rotate failed");
		Check(TransformSelection(context, history, rotateStarts, { 50.f, 0.f }, {}, 90.f, rotationGroup), "Rotation gesture update failed");
		Check(history.UndoCount() == 1 && Near(a->GetActorLocation(), { 50.f, -50.f }) && Near(b->GetActorLocation(), { 50.f, 50.f }) && a->GetActorRotation() == 90.f, "Rotation must orbit positions and merge into one gesture");
		Check(history.Undo() && Near(a->GetActorLocation(), {}) && a->GetActorRotation() == 0.f, "Rotation undo lost initial transforms");
		history.Clear();
		MultiInspectorCache cache;
		a->SetRenderEnabled(false);
		context.SceneChanged();
		const auto& rows = cache.Get(context);
		const CommonProperty* render = nullptr;
		const CommonProperty* testValue = nullptr;
		const CommonProperty* shared = nullptr;
		for (const auto& row : rows)
		{
			if (row.property->name == "Render Enabled") render = &row;
			if (row.property->name == "Test Value") testValue = &row;
			if (row.property->name == "Strength") shared = &row;
		}
		const int reads = getterReads;
		cache.Get(context);
		Check(getterReads == reads, "Unchanged Inspector must reuse cached values without visiting getters");
		Check(shared && shared->section == "SharedComponent", "Different derived components must expose their common reflected base");
		if (shared)
		{
			Check(SetCommonProperty(context, history, *shared, 7) && a->FindComponent<FirstComponent>()->strength == 7 && b->FindComponent<SecondComponent>()->strength == 7,
				"Common component edit must resolve each actor's concrete component identity");
			Check(history.Undo() && a->FindComponent<FirstComponent>()->strength == 1 && b->FindComponent<SecondComponent>()->strength == 1,
				"Common component undo must restore each concrete component");
			history.Clear();
		}
		Check(render && render->mixed && testValue, "Common inspector must expose derived properties and mixed values");
		if (render)
		{
			Check(SetCommonProperty(context, history, *render, true), "Multi property edit failed");
			Check(a->IsRenderEnabled() && b->IsRenderEnabled() && history.UndoCount() == 1, "Multi property edit must update every actor in one entry");
			Check(history.Undo() && !a->IsRenderEnabled() && b->IsRenderEnabled(), "Property undo must restore each actor's own value");
		}
		history.Clear();
		b->reject = true;
		if (testValue) Check(!SetCommonProperty(context, history, *testValue, 5) && a->value == 0 && b->value == 0 && history.UndoCount() == 0,
			"Failed child must roll back earlier children and keep history empty");
		b->reject = false;
		selection.Add(c);
		bool derived = false, sprite = false;
		for (const auto& row : cache.Get(context))
		{
			derived = derived || row.property->name == "Test Value";
			sprite = sprite || row.section == "SpriteComponent";
		}
		Check(!derived && !sprite, "Heterogeneous selection must show only common bases/components");
		Check(context.FocusSelection(), "Group focus failed");
		const auto bounds = selection.Bounds();
		Check(bounds && camera.GetView().getCenter() == bounds->position + bounds->size * .5f, "Focus must frame combined bounds");
		Check(DuplicateSceneEntity(context, history) && selection.Count() == 3 && history.UndoCount() == 1, "Duplicate must select three copies as one command");
		const auto copies = selection.GetAll();
		Check(copies[0]->FindComponent<vega::SceneEntityComponent>()->guid != guidA && Near(copies[1]->GetActorLocation() - copies[0]->GetActorLocation(), { 100.f, 0.f }), "Duplicate must generate GUIDs and preserve layout");
		Check(history.Undo() && selection.Count() == 3 && selection.Get() == c && context.ResolveGuid(guidA) == a, "Duplicate undo must restore original ordered selection");
		Check(history.Redo() && selection.Count() == 3 && selection.Get()->FindComponent<vega::SceneEntityComponent>()->guid == copies[2]->FindComponent<vega::SceneEntityComponent>()->guid,
			"Duplicate redo must restore all copies by GUID");
		history.Undo(); history.Clear();
		Check(DeleteSelection(context, history) && selection.Count() == 0 && history.UndoCount() == 1, "Delete must remove the whole selection in one entry");
		Check(history.Undo() && selection.Count() == 3 && selection.Get() == context.ResolveGuid(guidC), "Delete undo must restore selection order by GUID");
		Check(context.ResolveGuid(guidA) != a && context.ResolveGuid(guidB) != b, "Delete undo must reconstruct authored actors");
		Check(history.Redo() && selection.Count() == 0 && !context.ResolveGuid(guidA), "Group delete redo failed");
		history.Undo(); history.Clear();
		const auto primary = selection.Get();
		const auto first = selection.GetAll().front();
		Check(history.Execute(std::make_unique<AddComponentCommand>(first->GetUniqueID(), "SpriteComponent")) && selection.Count() == 3 && selection.Get() == primary,
			"Component replacement must preserve secondary selection and primary");
		Check(history.Undo() && selection.Count() == 3, "Component undo must preserve every selection");
		vega::World restored{ nullptr };
		vega::SceneSerializer::Load(restored, vega::SceneSerializer::Save(world));
		context.world = &restored;
		context.SceneChanged();
		Check(selection.Count() == 3 && selection.Get() == context.ResolveGuid(guidC), "World reconstruction must rebind the whole selection");
		SelectRectangle(context, { 101.f, 31.f }, { -1.f, -1.f });
		Check(selection.Count() == 3, "Reverse marquee must select actors including zero-size bounds");
		SelectRectangle(context, { -1.f, -1.f }, { 1.f, 1.f }, false, true);
		Check(selection.Count() == 2 && !selection.Contains(context.ResolveGuid(guidA)), "Control marquee must remove hits only");
		SelectRectangle(context, { -1.f, -1.f }, { 1.f, 1.f }, true);
		Check(selection.Count() == 3 && selection.Get() == context.ResolveGuid(guidA), "Shift marquee must add hits and update primary");
	}

	void TestCollisionInspector()
	{
		vega::EngineContext first, second;
		vega::ProjectSettings project;
		project.collisionLayers = { { 0, "Solid", {} }, { 7, "Sensor", {} } };
		project.ApplyCollisionLayers(first);
		project.collisionLayers = { { 0, "Other", {} } };
		project.ApplyCollisionLayers(second);
		for (auto* services : { &first, &second })
		{
			vega::World world{ *services };
			vega::EditorSelection selection;
			vega::EditorContext context{ &world, selection };
			context.services = services;
			vega::CommandHistory history{ context };
			for (int index = 0; index < 2; ++index)
			{
				auto actor = world.SpawnActor<vega::Actor>().lock();
				actor->AddComponent<vega::SceneEntityComponent>();
				actor->AddComponent<vega::CollisionComponent>();
				selection.Add(actor);
			}
			vega::MultiInspectorCache cache;
			bool found = false;
			for (const auto& property : cache.Get(context))
				if (property.property->name == "Collision Mask")
				{
					found = true;
					Check(property.property->enumLabels[0].second == (services == &first ? "Solid" : "Other"),
						"Inspector received another context's collision labels");
					Check(vega::SetCommonProperty(context, history, property, 129), "Inspector flag edit failed");
					for (const auto& actor : selection.GetAll())
						Check(static_cast<int>(actor->GetCollisionMask()) == 129, "Inspector changed the wrong collision bits");
				}
			Check(found, "Inspector omitted collision flags");
		}
	}
	void TestSelectionInvalidation()
	{
		vega::World world{ nullptr };
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		auto a = world.SpawnActor<MultiActor>().lock();
		auto b = world.SpawnActor<MultiActor>().lock();
		auto c = world.SpawnActor<MultiActor>().lock();
		const auto guid = a->AddComponent<vega::SceneEntityComponent>().guid;
		selection.Add(a); selection.Add(b); selection.Add(c);
		vega::MultiInspectorCache cache;
		Check(!cache.Get(context).empty(), "Inspector cache fixture must expose properties");
		const auto revision = selection.Revision();
		a->Destroy();
		Check(selection.Revision() > revision && selection.Count() == 2, "Pending destruction must invalidate cached selection values");
		for (const auto& row : cache.Get(context)) Check(row.targets.size() == 2, "Inspector retained a destroyed secondary target");
		const auto replacement = world.SpawnActor<MultiActor>().lock();
		replacement->AddComponent<vega::SceneEntityComponent>().guid = guid;
		selection.Add(replacement);
		context.SceneChanged();
		Check(selection.Count() == 3, "Re-adding a recreated GUID must preserve set uniqueness");
		const auto ownerCount = replacement.use_count();
		cache.Get(context);
		Check(replacement.use_count() == ownerCount, "Inspector cache must not retain actors/components");
	}
	void TestRichProperties()
	{
		using namespace vega;
		vega::World world{ nullptr };
		EditorSelection selection;
		EditorContext context{ &world, selection };
		CommandHistory history{context};
		auto a = world.SpawnActor<MultiActor>().lock(), b = world.SpawnActor<MultiActor>().lock();
		a->AddComponent<vega::SceneEntityComponent>("rich-a", "A"); b->AddComponent<vega::SceneEntityComponent>("rich-b", "B");
		a->points = {{1,2}}; b->points = {{3,4}};
		selection.Add(a); selection.Add(b);
		MultiInspectorCache cache;
		const auto find = [&]() -> CommonProperty
		{
			for (const auto& row : cache.Get(context)) if (row.property->name == "Points") return row;
			throw std::runtime_error("Missing list inspector row");
		};
		Check(find().mixed, "Different lists must mark the entire common property mixed");
		Check(SetCommonProperty(context, history, find(), vega::reflect::PropertyList{sf::Vector2f{8,9}}) && history.UndoCount() == 1, "Common list edit must be one undo step");
		Check(!find().mixed && a->points == b->points, "Common list cache must refresh by revision");
		Check(history.Undo() && find().mixed && a->points[0].x == 1 && b->points[0].x == 3, "Undo must restore each complete list");
		history.Clear();
		const auto group = history.BeginMergeGroup();
		for (float x : {10.f,20.f,30.f}) history.Execute(std::make_unique<SetPropertyCommand>(a->GetUniqueID(), std::nullopt, "Points", vega::reflect::PropertyList{sf::Vector2f{x,2}}, group));
		Check(history.UndoCount() == 1 && history.Undo() && a->points[0].x == 1, "Point drag updates must merge into one undo gesture");
		a->target = vega::ActorRef{*b}; b->target = vega::ActorRef{*a};
		Check(DuplicateSceneEntity(context, history), "Connected multi-selection must duplicate");
		const auto copies = selection.GetAll();
		if (copies.size() == 2)
		{
			const auto first = std::dynamic_pointer_cast<MultiActor>(copies[0]), second = std::dynamic_pointer_cast<MultiActor>(copies[1]);
			Check(first->target.Resolve(world) == second && second->target.Resolve(world) == first, "Cross-root copied references must link the copied set");
		}
	}
	// Struct fields and struct lists go through the same whole-value SetPropertyCommand as other properties.
	void TestStructProperties()
	{
		using namespace vega;
		using vega::reflect::PropertyList;
		using vega::reflect::StructValue;
		vega::World world{ nullptr };
		EditorSelection selection;
		EditorContext context{ &world, selection };
		CommandHistory history{ context };
		auto a = world.SpawnActor<StructHost>().lock(), b = world.SpawnActor<StructHost>().lock();
		a->AddComponent<vega::SceneEntityComponent>("struct-a", "A");
		b->AddComponent<vega::SceneEntityComponent>("struct-b", "B");
		const auto target = ResolvePropertyTarget(*a, {});
		std::string error;
		// Writes one leaf through a path the way the editor script does, returning whether the command ran.
		const auto writeLeaf = [&](const std::string& text, vega::reflect::LeafValue leaf)
		{
			StructLeafPath path;
			if (!ResolveStructLeafPath(target, text, path, error)) return false;
			auto whole = path.property->getter(target.object);
			auto* stored = FindStructLeaf(path, whole, error);
			if (!stored) return false;
			*stored = std::move(leaf);
			return history.Execute(std::make_unique<SetPropertyCommand>(a->GetUniqueID(), std::nullopt, path.property->name, whole));
		};

		// Descriptor defaults are what a new list element starts from.
		const auto* waveType = context.Engine().Types().Find("StructWave");
		Check(waveType && waveType->makeDefault, "Registered struct must provide field defaults");
		const auto defaults = waveType && waveType->makeDefault ? waveType->makeDefault() : StructValue{};
		Check(defaults.size() == 4 && std::get<int>(defaults[1].second) == 1 && std::get<std::string>(defaults[0].second).empty(),
			"Defaults must follow field initialisers");

		// Struct field: one edit is one undo step, and the field Range clamps the written value.
		Check(writeLeaf("Boss.Count", 7) && a->boss.count == 7 && history.UndoCount() == 1, "Struct field edit must write the struct");
		Check(writeLeaf("Boss.Count", 99) && a->boss.count == 50, "Struct field write must apply the field Range");
		Check(history.Undo() && a->boss.count == 7 && history.Undo() && a->boss.count == 1, "Undo must restore each struct field value");
		Check(history.Redo() && a->boss.count == 7, "Redo must reapply the struct field value");
		history.Clear();
		Check(!writeLeaf("Boss.Missing", 1) && !error.empty(), "Unknown struct field must be refused");
		StructLeafPath unused;
		Check(!ResolveStructLeafPath(target, "Waves.Count", unused, error), "A struct list needs an index in a path");
		Check(!ResolveStructLeafPath(target, "Boss[0].Count", unused, error), "A struct property takes no index");
		Check(!ResolveStructLeafPath(target, "Boss.Count.Extra", unused, error), "Nested paths are refused in v1");
		Check(!writeLeaf("Waves[0].Count", 2) && error.find("out of range") != std::string::npos, "Element writes need an existing element");

		// A drag is several frames in one merge group: they become one undo step.
		const auto group = history.BeginMergeGroup();
		for (int count : { 2, 3, 4 })
		{
			StructLeafPath path;
			std::string drag;
			if (!ResolveStructLeafPath(target, "Boss.Count", path, drag)) break;
			auto whole = path.property->getter(target.object);
			if (auto* stored = FindStructLeaf(path, whole, drag)) *stored = count;
			history.Execute(std::make_unique<SetPropertyCommand>(a->GetUniqueID(), std::nullopt, "Boss", whole, group));
		}
		Check(a->boss.count == 4 && history.UndoCount() == 1, "Struct drag frames must merge into one undo step");
		Check(history.Undo() && a->boss.count == 7, "Merged struct drag must undo to the value before the gesture");
		history.Clear();

		// List: add from defaults, edit element fields, reorder, remove. Each step is one undoable command.
		const auto addElement = [&]()
		{
			auto whole = target.type->FindProperty("Waves")->getter(target.object);
			std::get<PropertyList>(whole).emplace_back(waveType->makeDefault());
			return history.Execute(std::make_unique<SetPropertyCommand>(a->GetUniqueID(), std::nullopt, "Waves", whole));
		};
		Check(addElement() && a->waves.size() == 1 && a->waves[0].count == 1, "Add must append a default element");
		Check(addElement() && writeLeaf("Waves[1].Count", 4), "Element field edit must write the list");
		Check(addElement() && writeLeaf("Waves[2].Prefab", std::string{ "x.prefab.json" }), "Element string field edit must write the list");
		Check(a->waves.size() == 3 && a->waves[1].count == 4 && a->waves[2].prefab == "x.prefab.json" && a->waves[0].count == 1,
			"List edits must keep each element");
		{
			// Move element 0 to index 2, as list-move does: the rest shift toward the front.
			auto whole = target.type->FindProperty("Waves")->getter(target.object);
			auto& list = std::get<PropertyList>(whole);
			std::rotate(list.begin(), list.begin() + 1, list.begin() + 3);
			Check(history.Execute(std::make_unique<SetPropertyCommand>(a->GetUniqueID(), std::nullopt, "Waves", whole)), "Reorder must be one edit");
		}
		Check(a->waves[0].count == 4 && a->waves[1].prefab == "x.prefab.json" && a->waves[2].count == 1, "Reorder must move the element to its destination");
		{
			auto whole = target.type->FindProperty("Waves")->getter(target.object);
			auto& list = std::get<PropertyList>(whole);
			list.erase(list.begin());
			Check(history.Execute(std::make_unique<SetPropertyCommand>(a->GetUniqueID(), std::nullopt, "Waves", whole)) && a->waves.size() == 2,
				"Remove must be one edit");
		}
		Check(a->waves[0].prefab == "x.prefab.json", "Remove must keep the remaining order");
		// Seven commands: three adds, two field edits, a reorder and a remove. Undo walks back through them.
		const std::size_t undoneSizes[] = { 3, 3, 3, 2, 2, 1, 0 };
		for (const auto size : undoneSizes)
			Check(history.Undo() && a->waves.size() == size, "Each struct list edit must undo to its previous list");
		Check(a->waves.empty(), "Undoing every list edit must restore an empty list");
		const std::size_t redoneSizes[] = { 1, 2, 2, 3, 3, 3, 2 };
		for (const auto size : redoneSizes)
			Check(history.Redo() && a->waves.size() == size, "Each struct list edit must redo to its next list");
		Check(a->waves[0].prefab == "x.prefab.json" && a->waves[1].prefab.empty(), "Redo must restore the reordered and trimmed list");
		history.Clear();

		// Multi-select: differing structs show mixed, and a common edit writes the whole struct to each selected actor.
		selection.Add(a);
		selection.Add(b);
		MultiInspectorCache cache;
		const auto find = [&]() -> CommonProperty
		{
			for (const auto& row : cache.Get(context)) if (row.property->name == "Boss") return row;
			throw std::runtime_error("Missing struct inspector row");
		};
		Check(find().mixed, "Different structs must mark the struct property mixed");
		const StructValue shared{ { "Prefab", std::string{ "pair.prefab.json" } }, { "Count", 5 }, { "Delay", 0.f }, { "Armed", true } };
		Check(SetCommonProperty(context, history, find(), shared) && a->boss.count == 5 && b->boss.count == 5 &&
			a->boss.prefab == "pair.prefab.json" && b->boss.armed, "Common struct edit must write the whole value to every selected actor");
		Check(!find().mixed && history.UndoCount() == 1, "Common struct edit must be one undo step");
	}

}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	vega::reflect::RegisterEngineTypes();
	vega::reflect::Class<vega::Actor>("Actor").Spawnable();
	vega::reflect::Class<MultiActor, vega::Actor>("MultiActor").Spawnable()
		.Property("Test Value", [](const MultiActor& actor) { ++getterReads; return actor.value; }, [](MultiActor& actor, int value)
		{
			if (actor.reject) throw std::runtime_error("Rejected test value");
			actor.value = value;
		});
	vega::reflect::Class<MultiActor, vega::Actor>("MultiActor")
		.Property("Points", [](const MultiActor& a) { return a.points; }, [](MultiActor& a, const std::vector<sf::Vector2f>& v) { a.points = v; })
		.Property("Target", [](const MultiActor& a) { return a.target; }, [](MultiActor& a, const vega::ActorRef& v) { a.target = v; });
	vega::reflect::Class<StructWave>("StructWave")
		.Field("Prefab", &StructWave::prefab)
		.Field("Count", &StructWave::count).Range(1.f, 50.f)
		.Field("Delay", &StructWave::delay).Range(0.f, 60.f)
		.Field("Armed", &StructWave::armed);
	vega::reflect::Class<StructHost, vega::Actor>("StructHost").Spawnable()
		.Property("Boss", [](const StructHost& host) { return host.boss; }, [](StructHost& host, const StructWave& value) { host.boss = value; })
		.Property("Waves", [](const StructHost& host) { return host.waves; }, [](StructHost& host, const std::vector<StructWave>& value) { host.waves = value; });
	vega::reflect::Class<SharedComponent, vega::ActorComponent>("SharedComponent")
		.Property("Strength", [](const SharedComponent& component) { return component.strength; }, [](SharedComponent& component, int value) { component.strength = value; });
	vega::reflect::Class<FirstComponent, SharedComponent>("FirstComponent").Addable();
	vega::reflect::Class<SecondComponent, SharedComponent>("SecondComponent").Addable();
	TestMultiCommands();
	TestCollisionInspector();
	TestSelectionInvalidation();
	TestRichProperties();
	TestStructProperties();
	return failures ? 1 : 0;
}
