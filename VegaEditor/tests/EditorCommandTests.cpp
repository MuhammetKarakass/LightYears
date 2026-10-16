#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <framework/SpriteComponent.h>
#include <framework/LightComponent.h>
#include <iostream>
#include <framework/scene/SceneSettings.h>
#include <limits>
#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include <filesystem>
#include <fstream>

namespace
{
	bool prefixComponent = false;
	class AuthoredActor : public ly::Actor
	{
	public:
		explicit AuthoredActor(ly::World* world) : Actor{ world }
		{
			if (prefixComponent) AddComponent<ly::SpriteComponent>();
			AddComponent<ly::ActorComponent>();
		}
	};
	class ComponentActor : public ly::Actor
	{
	public:
		explicit ComponentActor(ly::World* world) : Actor{ world } { AddComponent<ly::SpriteComponent>(); }
	};
	class RequiredActor : public ComponentActor
	{
	public:
		using ComponentActor::ComponentActor;
	};
	class ThrowingActor : public ly::Actor
	{
	public:
		using Actor::Actor;
	};
	int failures = 0;
	class HierarchyCommandActor : public ly::Actor
	{
	public:
		using Actor::Actor;
	};
	void Check(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << message << '\n';
			++failures;
		}
	}

	void TestHierarchyCommands()
	{
		ly::reflect::Class<HierarchyCommandActor, ly::Actor>("HierarchyCommandActor").Spawnable();
		ly::World world{ nullptr };
		auto root = world.SpawnActor<HierarchyCommandActor>().lock();
		auto child = world.SpawnActor<HierarchyCommandActor>().lock();
		root->AddComponent<ly::SceneEntityComponent>("root", "Root");
		child->AddComponent<ly::SceneEntityComponent>("child", "Child");
		root->SetActorLocation({ 100, 100 }); child->SetActorLocation({ 110, 100 });
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		vega::CommandHistory history{ context };
		Check(history.Execute(std::make_unique<vega::ReparentActor>("child", "root")), "Parent command failed");
		Check(child->GetParent() == root.get() && child->GetRelativeLocation() == sf::Vector2f{ 10, 0 }, "Parent keep-world");
		Check(history.Undo() && !child->GetParent() && child->GetActorLocation() == sf::Vector2f{ 110, 100 }, "Parent undo");
		Check(history.Redo() && child->GetParent() == root.get(), "Parent redo");
		const auto depth = history.UndoCount();
		Check(!history.Execute(std::make_unique<vega::ReparentActor>("root", "child")) && history.UndoCount() == depth, "Cycle command must not enter history");
		selection.Add(child); selection.Add(root);
		const auto starts = vega::CaptureSelectionTransform(selection);
		Check(starts.size() == 1 && vega::TransformSelection(context, history, starts, { 100, 100 }, { 20, 30 }, 90.f), "Hierarchy transform command");
		Check((child->GetActorLocation() - sf::Vector2f{ 120, 140 }).length() < .001f, "Parent+child selection double-transformed");
		Check(history.Undo() && child->GetActorLocation() == sf::Vector2f{ 110, 100 }, "Transform undo must restore subtree");
		selection.Select(root);
		Check(history.Execute(std::make_unique<vega::AddComponentCommand>(root->GetUniqueID(), "LightComponent")), "Root component replacement");
		root = std::dynamic_pointer_cast<HierarchyCommandActor>(context.ResolveGuid("root"));
		Check(child->GetParent() == root.get() && !child->GetIsPendingDestroy(), "Component edit destroyed children");
		Check(history.Undo() && child->GetParent() == context.ResolveGuid("root").get(), "Component undo lost children");
		selection.Select(context.ResolveGuid("child"));
		Check(history.Execute(std::make_unique<vega::SetPropertyCommand>(child->GetUniqueID(), std::nullopt, "Location", sf::Vector2f{ 30, 0 }, 0, true)), "Relative inspector edit");
		Check(child->GetActorLocation() == sf::Vector2f{ 130, 100 } && history.Undo() && child->GetActorLocation() == sf::Vector2f{ 110, 100 }, "Relative inspector undo");
		selection.Select(context.ResolveGuid("root")); selection.Add(child);
		Check(vega::DeleteSelection(context, history) && !context.ResolveGuid("child"), "Hierarchy delete");
		Check(history.Undo() && context.ResolveGuid("child") && context.ResolveGuid("child")->GetParent() == context.ResolveGuid("root").get(), "Hierarchy delete undo");
		Check(vega::DuplicateSceneEntity(context, history), "Subtree duplicate");
		Check(selection.Count() == 1 && selection.Get()->GetChildren().size() == 1, "Duplicate must select copied root and include child once");
		Check(history.Undo() && context.ResolveGuid("child"), "Duplicate undo damaged source");
	}
	void TestHistory()
	{
		ly::World world{ nullptr };
		auto actor = world.SpawnActor<ly::Actor>().lock();
		world.TickInternal(0.f);
		const auto id = actor->GetUniqueID();
		actor->SetActorLocation({ 10.f, 20.f });
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		vega::CommandHistory history{ context };
		Check(!history.CanUndo() && !history.CanRedo(), "History must start empty");
		Check(!history.Undo() && !history.Redo(), "Empty undo/redo must fail");
		Check(history.UndoName().empty() && history.RedoName().empty(), "Empty names must be empty");
		Check(vega::SelectActor{ id }.Execute(context) && selection.Get() == actor, "Selection command failed");
		Check(vega::SetPaused{ true }.Execute(context) && world.IsPaused(), "Pause command failed");
		Check(vega::SetPaused{ false }.Execute(context) && !world.IsPaused(), "Play command failed");
		Check(!history.CanUndo(), "Direct selection/pause must not record history");
		vega::EditorCamera camera;
		// The headless default view is 1000x1000, matching this texture size.
		camera.SetTextureSize({ 1000u, 1000u });
		context.camera = &camera;
		const auto gameView = world.GetWorldView();
		vega::SetPaused{ true }.Execute(context);
		Check(world.GetViewOverride() && world.GetWorldView().getCenter() == gameView.getCenter() &&
			world.GetWorldView().getSize() == gameView.getSize(), "Pause must enter editor camera without a jump");
		context.SetEditorCamera(false);
		context.SyncCamera();
		Check(!world.GetViewOverride(), "Manual game camera choice must survive paused frames");
		Check(context.FocusSelection() && world.GetViewOverride(), "Focus must enter editor camera");
		vega::SetPaused{ false }.Execute(context);
		Check(!world.GetViewOverride() && world.GetWorldView().getCenter() == gameView.getCenter(),
			"Play must restore the game camera");
		context.SetEditorCamera(true);
		vega::SetPaused{ false }.Execute(context);
		Check(!world.GetViewOverride(), "Explicit play must clear a manual editor camera");
		const long owners = actor.use_count();
		Check(history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 30.f, 40.f })), "Move execute failed");
		Check(actor.use_count() == owners, "Commands must not own an actor");
		Check(actor->GetActorLocation() == sf::Vector2f{ 30.f, 40.f }, "Move did not apply");
		Check(history.UndoName() == "Move Actor #" + std::to_string(id), "Move label lacks actor identity");
		Check(history.Undo() && actor->GetActorLocation() == sf::Vector2f{ 10.f, 20.f }, "Undo did not restore original location");
		Check(history.RedoName() == "Move Actor #" + std::to_string(id), "Redo label incorrect");
		Check(history.Redo() && actor->GetActorLocation() == sf::Vector2f{ 30.f, 40.f }, "Redo did not restore move");
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Rotation", 45.f));
		Check(actor->GetActorRotation() == 45.f, "Rotation did not apply");
		Check(history.Undo() && actor->GetActorRotation() == 0.f, "Rotation undo failed");
		Check(history.Redo() && actor->GetActorRotation() == 45.f, "Rotation redo failed");
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "RenderLayer", static_cast<int>(ly::RenderLayer::Foreground)));
		Check(actor->GetRenderLayer() == ly::RenderLayer::Foreground, "Layer did not apply");
		Check(history.Undo() && actor->GetRenderLayer() == ly::RenderLayer::World, "Layer undo failed");
		Check(history.Redo() && actor->GetRenderLayer() == ly::RenderLayer::Foreground, "Layer redo failed");
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Render Enabled", false));
		Check(!actor->IsRenderEnabled(), "Render enabled did not apply");
		Check(history.Undo() && actor->IsRenderEnabled(), "Render enabled undo failed");
		Check(history.Redo() && !actor->IsRenderEnabled(), "Render enabled redo failed");
		history.Undo();
		Check(!history.Execute(std::make_unique<vega::SetPropertyCommand>(id + 1000, std::nullopt, "Location", sf::Vector2f{})) && history.CanRedo(),
			"Failed commands must not invalidate redo");
		Check(!history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Rotation", std::numeric_limits<float>::infinity())), "Nonfinite rotation must fail");
		Check(!history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "RenderLayer", static_cast<int>(ly::RenderLayer::Count))), "Invalid render layer must fail");
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{}));
		Check(!history.CanRedo(), "New edit must invalidate redo");
		history.Undo();
		history.Clear();
		Check(!history.CanUndo() && !history.CanRedo(), "Clear must empty both stacks");
		Check(vega::SelectActor{ std::nullopt }.Execute(context) && !selection.Get(), "Clear selection command failed");
	}

	void TestScriptProperties()
	{
		const std::filesystem::path root{ VEGA_COMMAND_SCRIPT_ROOT };
		std::filesystem::create_directories(root);
		vega::script::SetScriptRoot(root);
		vega::script::RegisterScriptTypes();
		std::ofstream{ root / "command.lua" } << "properties={speed=12,enabled=true,offset={x=1,y=2},label='hello'}";
		ly::World world{ nullptr };
		auto actor = world.SpawnActor<ly::Actor>().lock();
		world.TickInternal(0.f);
		auto& script = actor->AddComponent<vega::script::ScriptComponent>("command.lua");
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		vega::CommandHistory history{ context };
		const auto group = history.BeginMergeGroup();
		const auto set = [&](const std::string& name, ly::reflect::PropertyValue value, std::uint64_t merge = 0)
		{
			return history.Execute(std::make_unique<vega::SetScriptPropertyCommand>(actor->GetUniqueID(), ly::ComponentIdentity{ "ScriptComponent", 0 }, name, value, merge));
		};
		Check(set("speed", 30.f, group) && set("speed", 90.f, group), "Script drag refused");
		Check(history.Undo() && !history.CanUndo() && script.Overrides().empty() && std::get<float>(script.Properties().at("speed")) == 12.f,
			"Merged script undo must restore default and remove override");
		Check(history.Redo() && std::get<float>(script.Properties().at("speed")) == 90.f, "Merged script redo lost final value");
		Check(script.Reload() && std::get<float>(script.Properties().at("speed")) == 90.f, "Command override did not survive reload");
		Check(set("speed", 100.f) && history.Undo() && std::get<float>(script.Overrides().at("speed")) == 90.f, "Undo lost an existing override");
		Check(set("enabled", false) && set("offset", sf::Vector2f{3.f,4.f}) && set("label", std::string{"changed"}), "Script property types refused");
		Check(!set("speed", true) && !set("missing", 1.f) && !set("speed", std::numeric_limits<float>::infinity()), "Invalid script edits accepted");
		actor->Destroy();
		history.Undo();
		Check(!set("speed", 1.f), "Destroyed actor accepted script edits");
	}

	void TestReflectedProperties()
	{
		ly::reflect::RegisterEngineTypes();
		ly::World world{ nullptr };
		auto actor = world.SpawnActor<ly::Actor>().lock();
		auto& first = actor->AddComponent<ly::ActorComponent>();
		auto& second = actor->AddComponent<ly::ActorComponent>();
		world.TickInternal(0.f);
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		vega::CommandHistory history{ context };
		const auto id = actor->GetUniqueID();
		const auto group = history.BeginMergeGroup();
		const auto edit = [&](std::optional<ly::ComponentIdentity> index, const char* property, ly::reflect::PropertyValue value, std::uint64_t merge = 0)
		{
			return history.Execute(std::make_unique<vega::SetPropertyCommand>(id, index, property, std::move(value), merge));
		};
		Check(edit({}, "Rotation", 10.f, group) && edit({}, "Rotation", 45.f, group), "Reflected drag failed");
		Check(history.Undo() && actor->GetActorRotation() == 0.f && !history.CanUndo(), "Reflected drag must merge and keep first before value");
		Check(history.Redo() && actor->GetActorRotation() == 45.f, "Reflected merged redo failed");
		Check(!edit({}, "Rotation", 5) && !edit({}, "Velocity", sf::Vector2f{}) && !edit({}, "Missing", 1.f), "Invalid reflected writes must fail");
		Check(edit(ly::ComponentIdentity{ "ActorComponent", 0 }, "Active", false, group) && edit(ly::ComponentIdentity{ "ActorComponent", 1 }, "Active", false, group), "Component property edits failed");
		Check(history.Undo() && !first.IsActive() && second.IsActive(), "Different component indices must not merge");
		Check(history.Undo() && first.IsActive(), "Component undo failed");
		Check(history.Redo() && !first.IsActive(), "Component redo failed");
		Check(!edit(ly::ComponentIdentity{ "ActorComponent", 100 }, "Active", false) && history.CanRedo(), "Missing component must preserve redo");
		Check(vega::FindPropertyTarget(*actor, "ActorComponent").componentIdentity == ly::ComponentIdentity{ "ActorComponent", 0 }, "Named component target must use first match");
		Check(!vega::FindPropertyTarget(*actor, "Missing").object, "Unknown named target must fail");
		ly::reflect::Class<ThrowingActor, ly::Actor>("ThrowingActor")
			.Property("Throwing", [](const ThrowingActor& a) { return a.GetActorRotation(); },
				[](ThrowingActor&, float) { throw std::runtime_error("editor setter refused"); });
		auto throwingActor = world.SpawnActor<ThrowingActor>().lock();
		const auto undoCount = history.UndoCount();
		const auto redoCount = history.RedoCount();
		Check(!history.Execute(std::make_unique<vega::SetPropertyCommand>(throwingActor->GetUniqueID(), std::nullopt, "Throwing", 12.f)) &&
			context.propertyError == "editor setter refused" && context.propertyErrorActor == throwingActor->GetUniqueID() &&
			context.propertyErrorName == "Throwing" && throwingActor->GetActorRotation() == 0.f &&
			history.UndoCount() == undoCount && history.RedoCount() == redoCount, "Refused setters must report the reason and preserve values/history");
		history.Clear();
		Check(edit({}, "Rotation", 50.f, group) && edit({}, "Render Enabled", false, group), "Mixed property edits failed");
		history.Undo();
		Check(actor->IsRenderEnabled() && actor->GetActorRotation() == 50.f && history.CanUndo(), "Different properties must not merge");
		history.Clear();
		Check(edit({}, "Rotation", 60.f) && edit({}, "Rotation", 70.f), "Discrete reflected edits failed");
		history.Undo();
		Check(actor->GetActorRotation() == 60.f && history.CanUndo(), "Discrete reflected edits must not merge");
		actor->Destroy();
		history.Undo();
		Check(actor->GetActorRotation() == 60.f && !history.Redo(), "Destroyed target must not be mutated by reflected history");
	}

	void TestMergeAndDepth()
	{
		ly::World world{ nullptr };
		auto actor = world.SpawnActor<ly::Actor>().lock();
		auto other = world.SpawnActor<ly::Actor>().lock();
		world.TickInternal(0.f);
		const auto id = actor->GetUniqueID();
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		vega::CommandHistory history{ context, 2 };
		const auto group = history.BeginMergeGroup();
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 1.f, 2.f }, group));
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 3.f, 4.f }, group));
		Check(history.Undo() && actor->GetActorLocation() == sf::Vector2f{} && !history.CanUndo(), "Drag must merge and retain its initial location");
		Check(history.Redo() && actor->GetActorLocation() == sf::Vector2f{ 3.f, 4.f }, "Merged redo must use final location");
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 4.f, 5.f }, group));
		history.Undo();
		Check(actor->GetActorLocation() == sf::Vector2f{ 3.f, 4.f } && history.CanUndo(), "Redo must end the previous merge gesture");
		history.Redo();
		history.Undo();
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 5.f, 6.f }, history.BeginMergeGroup()));
		history.Undo();
		Check(actor->GetActorLocation() == sf::Vector2f{ 3.f, 4.f } && history.CanUndo(), "Separate drags must remain separate");
		history.Clear();
		const auto rotationGroup = history.BeginMergeGroup();
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Rotation", 10.f, rotationGroup));
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Rotation", 20.f, rotationGroup));
		history.Undo();
		Check(actor->GetActorRotation() == 0.f && !history.CanUndo(), "Rotation drag must merge");
		history.Clear();
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 1.f, 1.f }, group));
		history.Execute(std::make_unique<vega::SetPropertyCommand>(other->GetUniqueID(), std::nullopt, "Location", sf::Vector2f{ 2.f, 2.f }, group));
		history.Undo();
		Check(history.CanUndo() && actor->GetActorLocation() == sf::Vector2f{ 1.f, 1.f }, "Different actors must not merge");
		history.Clear();
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 2.f, 2.f }, group));
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Rotation", 30.f, group));
		history.Undo();
		Check(history.CanUndo(), "Different properties must not merge");
		history.Clear();
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 1.f, 1.f }));
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 2.f, 2.f }));
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 3.f, 3.f }));
		history.Undo();
		history.Undo();
		Check(!history.CanUndo() && actor->GetActorLocation() == sf::Vector2f{ 1.f, 1.f }, "Max depth must discard oldest entry; discrete edits must not merge");
		Check(history.Redo() && history.Redo(), "Depth-limited entries must redo");
		history.Clear();
		vega::CommandHistory zeroDepth{ context, 0 };
		Check(zeroDepth.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Rotation", 9.f)) && !zeroDepth.CanUndo(), "Zero depth must execute without storing");
	}

	void TestDestroyAndResolution()
	{
		ly::World world{ nullptr };
		auto actor = world.SpawnActor<ly::Actor>().lock();
		world.TickInternal(0.f);
		const auto id = actor->GetUniqueID();
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		vega::CommandHistory history{ context };
		vega::SelectActor{ id }.Execute(context);
		history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 5.f, 6.f }));
		history.Undo();
		Check(history.CanRedo(), "Destroy test needs a redo entry");
		Check(history.Execute(std::make_unique<vega::DestroyActor>(id)), "Destroy execute failed");
		Check(actor->GetIsPendingDestroy() && !selection.Get(), "Destroy must mark actor and clear selection");
		Check(!history.CanUndo() && !history.CanRedo(), "Destroy must not record itself and must clear redo");
		Check(!context.Resolve(id), "Pending destroyed actors must not resolve");
		Check(!history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Rotation", 1.f)), "Destroyed actors must reject edits");
		Check(!vega::SelectActor{ id }.Execute(context), "Destroyed actors must reject selection");
		auto removed = world.SpawnActor<ly::Actor>().lock();
		world.TickInternal(0.f);
		history.Execute(std::make_unique<vega::SetPropertyCommand>(removed->GetUniqueID(), std::nullopt, "Location", sf::Vector2f{ 9.f, 10.f }));
		removed->Destroy();
		history.Undo();
		Check(removed->GetActorLocation() == sf::Vector2f{ 9.f, 10.f }, "Undo must not edit a retained destroyed actor");
		world.CleanCycle();
		removed.reset();
		Check(!history.Redo(), "Redo must fail safely after the actor expires");
		context.world = nullptr;
		Check(!vega::SetPaused{ true }.Execute(context) && !context.Resolve(id), "Null world must fail safely");
	}

	void TestSceneIdentity()
	{
		ly::reflect::RegisterEngineTypes();
		ly::reflect::Class<AuthoredActor, ly::Actor>("AuthoredActor").Spawnable();
		ly::World original{ nullptr };
		vega::EditorSelection selection;
		vega::EditorContext context{ &original, selection };
		vega::CommandHistory history{ context };
		Check(vega::CreateSceneEntity(context, history, "AuthoredActor"), "Scene create failed");
		auto actor = selection.Get();
		if (!actor) return;
		const auto id = actor->GetUniqueID();
		const auto guid = actor->FindComponent<ly::SceneEntityComponent>()->guid;
		const auto initialLocation = actor->GetActorLocation();
		Check(history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 500.f, 400.f })), "Pending scene actor must resolve");
		Check(vega::RenameSceneEntity(context, history, "Rock A"), "Scene rename failed");
		Check(history.Execute(std::make_unique<vega::SetPropertyCommand>(id, ly::ComponentIdentity{ "ActorComponent", 0 }, "Active", false)), "Component edit failed");
		const auto snapshot = ly::SceneSerializer::Save(original);
		ly::World restored{ nullptr };
		ly::SceneSerializer::Load(restored, snapshot);
		context.world = &restored;
		auto replacement = context.ResolveGuid(guid);
		Check(replacement && replacement->GetUniqueID() != id, "Reload must preserve GUID with a new runtime ID");
		std::string capturedGuid = guid;
		Check(!context.Resolve(id) && context.Resolve(id, capturedGuid) == replacement, "GUID resolution must work when the old runtime ID is absent");
		Check(history.Undo() && replacement->FindComponent<ly::ActorComponent>()->IsActive(), "Component undo must resolve GUID after reload");
		Check(history.Undo() && replacement->FindComponent<ly::SceneEntityComponent>()->name == "AuthoredActor 1", "Rename undo after reload failed");
		Check(history.Undo() && replacement->GetActorLocation() == initialLocation, "Move undo after reload failed");
		Check(history.Redo() && history.Redo() && history.Redo(), "Redo across reload failed");
		selection.Select(replacement);
		Check(history.Execute(std::make_unique<vega::DestroyActor>(replacement->GetUniqueID())), "Scene deletion failed");
		Check(!context.ResolveGuid(guid) && !selection.Get(), "Deleted scene actor must not resolve");
		Check(history.Undo(), "Scene deletion undo failed");
		auto recreated = context.ResolveGuid(guid);
		Check(recreated && recreated->GetUniqueID() != replacement->GetUniqueID() && selection.Get() == recreated, "Delete undo must recreate and select same GUID");
		Check(ly::SceneSerializer::Save(restored) == snapshot, "Delete undo must restore all serialized entity data");
		Check(history.Undo() && recreated->FindComponent<ly::ActorComponent>()->IsActive(), "Earlier component edit must reach recreated actor");
		Check(history.Redo() && history.Redo() && !context.ResolveGuid(guid), "Delete redo must resolve recreated actor");
		Check(history.Undo(), "Repeated delete undo failed");
		selection.Select(context.ResolveGuid(guid));
		Check(vega::DuplicateSceneEntity(context, history), "Duplicate failed");
		const auto duplicate = selection.Get();
		Check(duplicate && duplicate->FindComponent<ly::SceneEntityComponent>()->guid != guid &&
			duplicate->GetActorLocation() == sf::Vector2f{ 532.f, 432.f }, "Duplicate must use new GUID and offset");
		Check(history.Undo() && history.Redo(), "Duplicate undo/redo failed");
		context.playMode = true;
		const auto undoCount = history.UndoCount();
		Check(!history.Undo() && !vega::CreateSceneEntity(context, history, "AuthoredActor") &&
			!history.Execute(std::make_unique<vega::DestroyActor>(selection.Get()->GetUniqueID())) &&
			!vega::RenameSceneEntity(context, history, "Runtime"), "Play must reject authoring commands");
		Check(history.UndoCount() == undoCount, "Play must preserve history");
	}

	void TestComponentCommands()
	{
		using namespace ly;
		using namespace vega;
		reflect::Class<ComponentActor, Actor>("ComponentActor").Spawnable();
		reflect::Class<RequiredActor, ComponentActor>("RequiredActor").Spawnable().RequiresComponent("SpriteComponent");
		World world{ nullptr };
		auto actor = world.SpawnActor<ComponentActor>().lock();
		const auto guid = actor->AddComponent<SceneEntityComponent>("", "Component entity").guid;
		EditorSelection selection;
		EditorContext context{ &world, selection };
		CommandHistory history{ context };
		selection.Select(actor);
		const auto before = SceneSerializer::SaveEntity(*actor);
		Check(history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), std::nullopt, "Rotation", 15.f)), "Component test property setup failed");
		Check(history.Execute(std::make_unique<AddComponentCommand>(actor->GetUniqueID(), "LightComponent")), "Add component command failed");
		actor = std::dynamic_pointer_cast<ComponentActor>(context.ResolveGuid(guid));
		Check(actor && actor->FindComponent<LightComponent>() && selection.Get() == actor, "Add must preserve GUID and selection");
		const auto added = SceneSerializer::SaveEntity(*actor);
		Check(history.Undo() && !context.ResolveGuid(guid)->FindComponent<LightComponent>(), "Add undo failed");
		Check(history.Undo() && SceneSerializer::SaveEntity(*context.ResolveGuid(guid)) == before, "Earlier property undo must resolve recreated GUID");
		Check(history.Redo() && history.Redo() && SceneSerializer::SaveEntity(*context.ResolveGuid(guid)) == added, "Add redo must restore exact JSON");
		Check(history.Execute(std::make_unique<RemoveComponentCommand>(selection.Get()->GetUniqueID(), ComponentIdentity{ "SpriteComponent", 0 })), "Remove constructor component failed");
		const auto removed = SceneSerializer::SaveEntity(*context.ResolveGuid(guid));
		Check(!selection.Get()->FindComponent<SpriteComponent>() && removed["removed"] == nlohmann::json::array({ "SpriteComponent" }), "Constructor removal must persist in JSON");
		Check(history.Undo() && SceneSerializer::SaveEntity(*selection.Get()) == added, "Remove undo must restore exact JSON");
		Check(history.Redo() && SceneSerializer::SaveEntity(*selection.Get()) == removed, "Remove redo must restore exact JSON");
		Check(history.Execute(std::make_unique<AddComponentCommand>(selection.Get()->GetUniqueID(), "LightComponent")), "Duplicate component addition failed");
		Check(history.Execute(std::make_unique<RemoveComponentCommand>(selection.Get()->GetUniqueID(), ComponentIdentity{ "LightComponent", 1 })), "Removal must address a repeated component occurrence");
		Check(history.Undo() && FindComponentByTypeOccurrence(*selection.Get(), { "LightComponent", 1 }), "Repeated component undo lost occurrence");
		Check(history.Redo() && !FindComponentByTypeOccurrence(*selection.Get(), { "LightComponent", 1 }), "Repeated component redo failed");
		const auto count = history.UndoCount();
		const auto stable = SceneSerializer::SaveEntity(*selection.Get());
		Check(!history.Execute(std::make_unique<AddComponentCommand>(selection.Get()->GetUniqueID(), "Missing")) &&
			!history.Execute(std::make_unique<AddComponentCommand>(selection.Get()->GetUniqueID(), "ScriptComponent")) &&
			!history.Execute(std::make_unique<RemoveComponentCommand>(selection.Get()->GetUniqueID(), ComponentIdentity{ "LightComponent", 9 })), "Invalid component operations must fail");
		Check(history.UndoCount() == count && SceneSerializer::SaveEntity(*selection.Get()) == stable, "Failed component commands must not change entity or history");
		context.playMode = true;
		Check(!history.Execute(std::make_unique<AddComponentCommand>(selection.Get()->GetUniqueID(), "LightComponent")) &&
			!history.Execute(std::make_unique<RemoveComponentCommand>(selection.Get()->GetUniqueID(), ComponentIdentity{ "LightComponent", 0 })), "Play must refuse component edits");
		context.playMode = false;
		const auto required = world.SpawnActor<RequiredActor>().lock();
		required->AddComponent<SceneEntityComponent>();
		Check(!history.Execute(std::make_unique<RemoveComponentCommand>(required->GetUniqueID(), ComponentIdentity{ "SpriteComponent", 0 })) && required->FindComponent<SpriteComponent>(), "Required component removal must fail without replacing actor");
		// Editing another entity must preserve the current selection.
		const auto selected = selection.Get();
		Check(history.Execute(std::make_unique<AddComponentCommand>(required->GetUniqueID(), "LightComponent")) && selection.Get() == selected,
			"Component editing must preserve an unrelated selection");
	}

	void TestActorCacheAndRevisions()
	{
		ly::World world{ nullptr };
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		vega::CommandHistory history{ context };
		auto actor = world.SpawnActor<ly::Actor>().lock();
		const auto id = actor->GetUniqueID();
		Check(context.Resolve(id) == actor, "Cache must resolve pending actors");
		const auto guid = actor->AddComponent<ly::SceneEntityComponent>().guid;
		Check(context.ResolveGuid(guid) == actor, "Cache must discover identities added after its build");
		Check(history.Execute(std::make_unique<vega::SelectActor>(id)) && context.sceneRevision == 0,
			"Selection must not revise the scene");
		Check(history.Execute(std::make_unique<vega::SetPropertyCommand>(id, std::nullopt, "Location", sf::Vector2f{ 1.f, 2.f })) && context.sceneRevision == 1,
			"Scene command must revise the scene");
		Check(history.Undo() && context.sceneRevision == 2 && history.Redo() && context.sceneRevision == 3,
			"Undo and redo must create new scene revisions");
		Check(!history.Execute(std::make_unique<vega::SetPropertyCommand>(0, std::nullopt, "Location", sf::Vector2f{})) && context.sceneRevision == 3,
			"Failed commands must not revise the scene");
		world.TickInternal(0.f);
		Check(context.Resolve(id) == actor, "Promotion must preserve cached resolution");
		actor->Destroy();
		Check(!context.Resolve(id) && !context.ResolveGuid(guid), "Cache must reject retained destroyed actors");
		world.CleanCycle();
		auto replacement = world.SpawnActor<ly::Actor>().lock();
		replacement->AddComponent<ly::SceneEntityComponent>(guid, "Replacement");
		Check(context.ResolveGuid(guid) == replacement && context.Resolve(replacement->GetUniqueID()) == replacement,
			"Same-count replacement must refresh cached actors");
		ly::World foreign{ nullptr };
		context.world = &foreign;
		Check(!context.Resolve(id) && !context.Resolve(replacement->GetUniqueID()) && !context.ResolveGuid(guid),
			"Cache must never return an actor from the previous world");
		context.world = &world;
		Check(context.ResolveGuid(guid) == replacement, "Returning to a world must refresh the cache");
		const auto newGuid = ly::SceneEntityComponent::GenerateGuid();
		replacement->FindComponent<ly::SceneEntityComponent>()->guid = newGuid;
		Check(!context.ResolveGuid(guid) && context.ResolveGuid(newGuid) == replacement,
			"Cache hits must validate current GUIDs");
	}

	void TestPrefabReplacements()
	{
		using Json = nlohmann::json;
		ly::World world{ nullptr };
		auto actor = world.SpawnActor<AuthoredActor>().lock();
		const auto guid = actor->AddComponent<ly::SceneEntityComponent>("", "Replacement").guid;
		vega::EditorSelection selection;
		vega::EditorContext context{ &world, selection };
		vega::CommandHistory history{ context };
		selection.Select(actor);
		const auto before = ly::SceneSerializer::SaveEntity(*actor);
		auto after = before;
		after["properties"]["Location"] = { 70, 80 };
		Check(history.Execute(std::make_unique<vega::ReplaceEntities>(Json::array({ before }), Json::array({ after }), "Replace")), "Replacement entity list must not be nested");
		Check(selection.Get() == context.ResolveGuid(guid) && selection.Get()->GetActorLocation() == sf::Vector2f{ 70, 80 }, "Replacement must preserve GUID and selection");
		Check(history.Undo() && ly::SceneSerializer::SaveEntity(*context.ResolveGuid(guid)) == before, "Replacement undo must restore exact values");
		Check(history.Redo() && context.ResolveGuid(guid)->GetActorLocation() == sf::Vector2f{ 70, 80 }, "Replacement redo must re-resolve GUID");
		auto missingTarget = after;
		missingTarget["guid"] = "absent-target";
		auto changed = after;
		changed["properties"]["Location"] = { 900, 800 };
		Check(!history.Execute(std::make_unique<vega::ReplaceEntities>(Json::array({ after, missingTarget }), Json::array({ changed, missingTarget }), "Fail")), "Unavailable replacement must fail");
		Check(context.ResolveGuid(guid)->GetActorLocation() == sf::Vector2f{ 70, 80 }, "Failed multi-entity replacement must roll back earlier entities");
		const auto root = std::filesystem::path(VEGA_COMMAND_SCRIPT_ROOT);
		ly::PrefabLibrary::Get().SetRoot(root);
		std::filesystem::create_directories(root / "prefabs");
		const auto path = root / "prefabs/recovered.prefab.json";
		std::ofstream{ path } << ly::PrefabLibrary::Document(after, "Created").dump();
		ly::PrefabLibrary::Get().Reload();
		const Json instance{ { "guid", guid }, { "name", after["name"] }, { "prefab", "prefabs/recovered.prefab.json" },
			{ "properties", { { "Location", after["properties"]["Location"] } } } };
		Check(history.Execute(std::make_unique<vega::ReplaceEntities>(Json::array({ after }), Json::array({ instance }), "Create Prefab")),
			"Create prefab replacement must load its instance");
		Check(selection.Get() == context.ResolveGuid(guid), "Create prefab must keep GUID and selection");
		Check(history.Undo() && selection.Get() == context.ResolveGuid(guid) && ly::SceneSerializer::SaveEntity(*selection.Get()) == after,
			"Undo create prefab must restore the plain entity, GUID and selection");
		Check(history.Redo() && selection.Get() == context.ResolveGuid(guid) && selection.Get()->FindComponent<ly::SceneEntityComponent>()->prefab == "prefabs/recovered.prefab.json",
			"Redo create prefab must restore the selected instance");
		std::filesystem::remove(path);
		const Json missing{ { "guid", guid }, { "name", "Keep authored" }, { "prefab", "prefabs/recovered.prefab.json" }, { "extra", 42 } };
		Check(vega::RestoreSceneEntity(context, missing) && !context.ResolveGuid(guid), "Missing refreshed prefab must remove the runtime actor");
		Check(world.unresolvedPrefabs == Json::array({ missing }), "Missing refreshed prefab must preserve authored JSON");
		std::ofstream{ path } << Json{ { "format", "vega.prefab" }, { "version", 1 }, { "name", "Recovered" }, { "base", { { "type", "AuthoredActor" } } } }.dump();
		Check(vega::RestoreSceneEntity(context, missing) && context.ResolveGuid(guid) && world.unresolvedPrefabs.empty(), "Reload must recover a restored prefab file");
		Check(!selection.Get(), "Recovering a missing prefab must not steal an empty selection");
	}

	void TestSceneScriptIdentity()
	{
		ly::World original{ nullptr };
		auto actor = original.SpawnActor<AuthoredActor>().lock();
		const auto guid = actor->AddComponent<ly::SceneEntityComponent>().guid;
		auto& script = actor->AddComponent<vega::script::ScriptComponent>("command.lua");
		Check(script.Prepare(), "Scene script preparation failed");
		vega::EditorSelection selection;
		vega::EditorContext context{ &original, selection };
		vega::CommandHistory history{ context };
		Check(history.Execute(std::make_unique<vega::SetScriptPropertyCommand>(actor->GetUniqueID(), ly::ComponentIdentity{ "ScriptComponent", 0 }, "speed", 90.f)), "Scene script edit failed");
		Check(history.Execute(std::make_unique<vega::SetPropertyCommand>(actor->GetUniqueID(), ly::ComponentIdentity{ "ActorComponent", 0 }, "Active", false)), "Stable reflected edit failed");
		ly::World restored{ nullptr };
		// A new default component precedes both targets in the reconstructed actor.
		prefixComponent = true;
		ly::SceneSerializer::Load(restored, ly::SceneSerializer::Save(original));
		prefixComponent = false;
		context.world = &restored;
		const auto replacement = context.ResolveGuid(guid);
		const auto* restoredScript = replacement ? replacement->FindComponent<vega::script::ScriptComponent>() : nullptr;
		Check(restoredScript && std::get<float>(restoredScript->Properties().at("speed")) == 90.f, "Scene script snapshot lost its override");
		const auto* target = ly::FindComponentByTypeOccurrence(*replacement, { "ActorComponent", 0 });
		Check(history.Undo() && target->IsActive() && replacement->FindComponent<ly::SpriteComponent>()->IsActive(),
			"Undo must find the same type after an earlier component is added");
		Check(history.Undo() && restoredScript->Overrides().empty() && std::get<float>(restoredScript->Properties().at("speed")) == 12.f,
			"Script undo must resolve GUID after reload");
		Check(history.Redo() && std::get<float>(restoredScript->Properties().at("speed")) == 90.f, "Script redo after reload failed");
	}
}


namespace
{
	void TestSceneSettingsCommands()
	{
		using Json = nlohmann::json;
		ly::SceneSettingField field;
		field.key = "testSetting";
		field.type = ly::SceneSettingType::Int;
		field.defaultValue = 0;
		field.min = 0; field.max = 10;
		ly::SceneSettingsSchema::Get().Register(field);
		vega::EditorSelection selection;
		vega::EditorContext context{ nullptr, selection };
		vega::CommandHistory history{ context };
		Json settings = { { "unknown", { { "deep", { 1, false } } } } };
		const auto original = settings;
		context.sceneSettings = &settings;
		int applied = -1;
		context.applySettings = [&](const Json& authored)
		{
			applied = ly::SceneSettingsSchema::Get().Value(authored, "testSetting").get<int>();
		};
		Check(history.Execute(std::make_unique<vega::SetSceneSettingCommand>("testSetting", Json(2))), "Setting command failed");
		Check(applied == 2 && context.sceneRevision == 1, "Setting did not apply live or mark revision");
		Check(history.Undo() && applied == 0 && settings == original, "Undo did not restore missing override");
		Check(history.Redo() && applied == 2, "Setting redo failed");
		const auto revision = context.sceneRevision;
		Check(!history.Execute(std::make_unique<vega::SetSceneSettingCommand>("testSetting", Json(11))) && context.sceneRevision == revision && applied == 2, "Invalid setting dirtied or applied");
		Check(!history.Execute(std::make_unique<vega::SetSceneSettingCommand>("unknown", Json(1))), "Unknown setting was writable");
		Check(history.Execute(std::make_unique<vega::SetSceneSettingCommand>("testSetting", std::nullopt)) && settings == original && applied == 0, "Reset failed");
		Check(history.Undo() && settings["testSetting"] == 2 && applied == 2, "Reset undo failed");
		context.playMode = true;
		Check(!history.Execute(std::make_unique<vega::SetSceneSettingCommand>("testSetting", Json(3))) && !history.Undo(), "Play allowed settings edits");
		context.playMode = false;
		history.Clear();
		const auto group = history.BeginMergeGroup();
		Check(history.Execute(std::make_unique<vega::SetSceneSettingCommand>("testSetting", Json(3), group)) &&
			history.Execute(std::make_unique<vega::SetSceneSettingCommand>("testSetting", Json(4), group)) && history.UndoCount() == 1, "Gesture did not merge");
		Check(history.Undo() && settings["testSetting"] == 2, "Merged undo lost initial value");
		Check(settings["unknown"] == original["unknown"], "Command dropped unknown data");
	}
}

int main()
{
	TestSceneSettingsCommands();
	TestScriptProperties();
	TestReflectedProperties();
	TestHistory();
	TestMergeAndDepth();
	TestDestroyAndResolution();
	TestSceneIdentity();
	TestSceneScriptIdentity();
	TestActorCacheAndRevisions();
	TestComponentCommands();
	TestPrefabReplacements();
	TestHierarchyCommands();
	if (failures) return 1;
	std::cout << "Editor command tests passed\n";
	return 0;
}
