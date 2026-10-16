#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/SpriteComponent.h>
#include <iostream>
#include <cmath>

namespace
{
	class MultiActor : public ly::Actor
	{
	public:
		using Actor::Actor;
		bool reject = false;
		int value = 0;
	};
	class SharedComponent : public ly::ActorComponent
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
		ly::World world{ nullptr };
		EditorSelection selection;
		EditorContext context{ &world, selection };
		EditorCamera camera;
		context.camera = &camera;
		CommandHistory history{ context };
		auto a = world.SpawnActor<MultiActor>().lock();
		auto b = world.SpawnActor<MultiActor>().lock();
		auto c = world.SpawnActor<ly::Actor>().lock();
		const auto guidA = a->AddComponent<ly::SceneEntityComponent>().guid;
		const auto guidB = b->AddComponent<ly::SceneEntityComponent>().guid;
		const auto guidC = c->AddComponent<ly::SceneEntityComponent>().guid;
		a->SetActorLocation({ 0.f, 0.f });
		b->SetActorLocation({ 100.f, 0.f });
		c->SetActorLocation({ 50.f, 30.f });
		a->AddComponent<FirstComponent>();
		b->AddComponent<SecondComponent>();
		a->AddComponent<ly::SpriteComponent>();
		b->AddComponent<ly::SpriteComponent>();
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
		Check(copies[0]->FindComponent<ly::SceneEntityComponent>()->guid != guidA && Near(copies[1]->GetActorLocation() - copies[0]->GetActorLocation(), { 100.f, 0.f }), "Duplicate must generate GUIDs and preserve layout");
		Check(history.Undo() && selection.Count() == 3 && selection.Get() == c && context.ResolveGuid(guidA) == a, "Duplicate undo must restore original ordered selection");
		Check(history.Redo() && selection.Count() == 3 && selection.Get()->FindComponent<ly::SceneEntityComponent>()->guid == copies[2]->FindComponent<ly::SceneEntityComponent>()->guid,
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
		ly::World restored{ nullptr };
		ly::SceneSerializer::Load(restored, ly::SceneSerializer::Save(world));
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
	void TestSelectionInvalidation()
	{
		ly::World world{ nullptr };
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		auto a = world.SpawnActor<MultiActor>().lock();
		auto b = world.SpawnActor<MultiActor>().lock();
		auto c = world.SpawnActor<MultiActor>().lock();
		const auto guid = a->AddComponent<ly::SceneEntityComponent>().guid;
		selection.Add(a); selection.Add(b); selection.Add(c);
		vega::MultiInspectorCache cache;
		Check(!cache.Get(context).empty(), "Inspector cache fixture must expose properties");
		const auto revision = selection.Revision();
		a->Destroy();
		Check(selection.Revision() > revision && selection.Count() == 2, "Pending destruction must invalidate cached selection values");
		for (const auto& row : cache.Get(context)) Check(row.targets.size() == 2, "Inspector retained a destroyed secondary target");
		const auto replacement = world.SpawnActor<MultiActor>().lock();
		replacement->AddComponent<ly::SceneEntityComponent>().guid = guid;
		selection.Add(replacement);
		context.SceneChanged();
		Check(selection.Count() == 3, "Re-adding a recreated GUID must preserve set uniqueness");
		const auto ownerCount = replacement.use_count();
		cache.Get(context);
		Check(replacement.use_count() == ownerCount, "Inspector cache must not retain actors/components");
	}

}

int main()
{
	ly::reflect::RegisterEngineTypes();
	ly::reflect::Class<ly::Actor>("Actor").Spawnable();
	ly::reflect::Class<MultiActor, ly::Actor>("MultiActor").Spawnable()
		.Property("Test Value", [](const MultiActor& actor) { ++getterReads; return actor.value; }, [](MultiActor& actor, int value)
		{
			if (actor.reject) throw std::runtime_error("Rejected test value");
			actor.value = value;
		});
	ly::reflect::Class<SharedComponent, ly::ActorComponent>("SharedComponent")
		.Property("Strength", [](const SharedComponent& component) { return component.strength; }, [](SharedComponent& component, int value) { component.strength = value; });
	ly::reflect::Class<FirstComponent, SharedComponent>("FirstComponent").Addable();
	ly::reflect::Class<SecondComponent, SharedComponent>("SecondComponent").Addable();
	TestMultiCommands();
	TestSelectionInvalidation();
	return failures ? 1 : 0;
}
