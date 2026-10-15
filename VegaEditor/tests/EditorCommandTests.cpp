#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <iostream>
#include <limits>
#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include <filesystem>
#include <fstream>

namespace
{
	class AuthoredActor : public ly::Actor
	{
	public:
		explicit AuthoredActor(ly::World* world) : Actor{ world } { AddComponent<ly::ActorComponent>(); }
	};
	class ThrowingActor : public ly::Actor
	{
	public:
		using Actor::Actor;
	};
	int failures = 0;
	void Check(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << message << '\n';
			++failures;
		}
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
		Check(history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 30.f, 40.f })), "Move execute failed");
		Check(actor.use_count() == owners, "Commands must not own an actor");
		Check(actor->GetActorLocation() == sf::Vector2f{ 30.f, 40.f }, "Move did not apply");
		Check(history.UndoName() == "Move Actor #" + std::to_string(id), "Move label lacks actor identity");
		Check(history.Undo() && actor->GetActorLocation() == sf::Vector2f{ 10.f, 20.f }, "Undo did not restore original location");
		Check(history.RedoName() == "Move Actor #" + std::to_string(id), "Redo label incorrect");
		Check(history.Redo() && actor->GetActorLocation() == sf::Vector2f{ 30.f, 40.f }, "Redo did not restore move");
		history.Execute(std::make_unique<vega::SetActorRotation>(id, 45.f));
		Check(actor->GetActorRotation() == 45.f, "Rotation did not apply");
		Check(history.Undo() && actor->GetActorRotation() == 0.f, "Rotation undo failed");
		Check(history.Redo() && actor->GetActorRotation() == 45.f, "Rotation redo failed");
		history.Execute(std::make_unique<vega::SetRenderLayer>(id, ly::RenderLayer::Foreground));
		Check(actor->GetRenderLayer() == ly::RenderLayer::Foreground, "Layer did not apply");
		Check(history.Undo() && actor->GetRenderLayer() == ly::RenderLayer::World, "Layer undo failed");
		Check(history.Redo() && actor->GetRenderLayer() == ly::RenderLayer::Foreground, "Layer redo failed");
		history.Execute(std::make_unique<vega::SetRenderEnabled>(id, false));
		Check(!actor->IsRenderEnabled(), "Render enabled did not apply");
		Check(history.Undo() && actor->IsRenderEnabled(), "Render enabled undo failed");
		Check(history.Redo() && !actor->IsRenderEnabled(), "Render enabled redo failed");
		history.Undo();
		Check(!history.Execute(std::make_unique<vega::SetActorLocation>(id + 1000, sf::Vector2f{})) && history.CanRedo(),
			"Failed commands must not invalidate redo");
		Check(!history.Execute(std::make_unique<vega::SetActorRotation>(id, std::numeric_limits<float>::infinity())), "Nonfinite rotation must fail");
		Check(!history.Execute(std::make_unique<vega::SetRenderLayer>(id, ly::RenderLayer::Count)), "Invalid render layer must fail");
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{}));
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
			return history.Execute(std::make_unique<vega::SetScriptPropertyCommand>(actor->GetUniqueID(), 0, name, value, merge));
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
		const auto edit = [&](std::optional<std::size_t> index, const char* property, ly::reflect::PropertyValue value, std::uint64_t merge = 0)
		{
			return history.Execute(std::make_unique<vega::SetPropertyCommand>(id, index, property, std::move(value), merge));
		};
		Check(edit({}, "Rotation", 10.f, group) && edit({}, "Rotation", 45.f, group), "Reflected drag failed");
		Check(history.Undo() && actor->GetActorRotation() == 0.f && !history.CanUndo(), "Reflected drag must merge and keep first before value");
		Check(history.Redo() && actor->GetActorRotation() == 45.f, "Reflected merged redo failed");
		Check(!edit({}, "Rotation", 5) && !edit({}, "Velocity", sf::Vector2f{}) && !edit({}, "Missing", 1.f), "Invalid reflected writes must fail");
		Check(edit(0, "Active", false, group) && edit(1, "Active", false, group), "Component property edits failed");
		Check(history.Undo() && !first.IsActive() && second.IsActive(), "Different component indices must not merge");
		Check(history.Undo() && first.IsActive(), "Component undo failed");
		Check(history.Redo() && !first.IsActive(), "Component redo failed");
		Check(!edit(100, "Active", false) && history.CanRedo(), "Missing component must preserve redo");
		Check(vega::FindPropertyTarget(*actor, "ActorComponent").componentIndex == 0, "Named component target must use first match");
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
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 1.f, 2.f }, group));
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 3.f, 4.f }, group));
		Check(history.Undo() && actor->GetActorLocation() == sf::Vector2f{} && !history.CanUndo(), "Drag must merge and retain its initial location");
		Check(history.Redo() && actor->GetActorLocation() == sf::Vector2f{ 3.f, 4.f }, "Merged redo must use final location");
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 4.f, 5.f }, group));
		history.Undo();
		Check(actor->GetActorLocation() == sf::Vector2f{ 3.f, 4.f } && history.CanUndo(), "Redo must end the previous merge gesture");
		history.Redo();
		history.Undo();
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 5.f, 6.f }, history.BeginMergeGroup()));
		history.Undo();
		Check(actor->GetActorLocation() == sf::Vector2f{ 3.f, 4.f } && history.CanUndo(), "Separate drags must remain separate");
		history.Clear();
		const auto rotationGroup = history.BeginMergeGroup();
		history.Execute(std::make_unique<vega::SetActorRotation>(id, 10.f, rotationGroup));
		history.Execute(std::make_unique<vega::SetActorRotation>(id, 20.f, rotationGroup));
		history.Undo();
		Check(actor->GetActorRotation() == 0.f && !history.CanUndo(), "Rotation drag must merge");
		history.Clear();
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 1.f, 1.f }, group));
		history.Execute(std::make_unique<vega::SetActorLocation>(other->GetUniqueID(), sf::Vector2f{ 2.f, 2.f }, group));
		history.Undo();
		Check(history.CanUndo() && actor->GetActorLocation() == sf::Vector2f{ 1.f, 1.f }, "Different actors must not merge");
		history.Clear();
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 2.f, 2.f }, group));
		history.Execute(std::make_unique<vega::SetActorRotation>(id, 30.f, group));
		history.Undo();
		Check(history.CanUndo(), "Different properties must not merge");
		history.Clear();
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 1.f, 1.f }));
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 2.f, 2.f }));
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 3.f, 3.f }));
		history.Undo();
		history.Undo();
		Check(!history.CanUndo() && actor->GetActorLocation() == sf::Vector2f{ 1.f, 1.f }, "Max depth must discard oldest entry; discrete edits must not merge");
		Check(history.Redo() && history.Redo(), "Depth-limited entries must redo");
		history.Clear();
		vega::CommandHistory zeroDepth{ context, 0 };
		Check(zeroDepth.Execute(std::make_unique<vega::SetActorRotation>(id, 9.f)) && !zeroDepth.CanUndo(), "Zero depth must execute without storing");
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
		history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 5.f, 6.f }));
		history.Undo();
		Check(history.CanRedo(), "Destroy test needs a redo entry");
		Check(history.Execute(std::make_unique<vega::DestroyActor>(id)), "Destroy execute failed");
		Check(actor->GetIsPendingDestroy() && !selection.Get(), "Destroy must mark actor and clear selection");
		Check(!history.CanUndo() && !history.CanRedo(), "Destroy must not record itself and must clear redo");
		Check(!context.Resolve(id), "Pending destroyed actors must not resolve");
		Check(!history.Execute(std::make_unique<vega::SetActorRotation>(id, 1.f)), "Destroyed actors must reject edits");
		Check(!vega::SelectActor{ id }.Execute(context), "Destroyed actors must reject selection");
		auto removed = world.SpawnActor<ly::Actor>().lock();
		world.TickInternal(0.f);
		history.Execute(std::make_unique<vega::SetActorLocation>(removed->GetUniqueID(), sf::Vector2f{ 9.f, 10.f }));
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
		Check(history.Execute(std::make_unique<vega::SetActorLocation>(id, sf::Vector2f{ 500.f, 400.f })), "Pending scene actor must resolve");
		Check(vega::RenameSceneEntity(context, history, "Rock A"), "Scene rename failed");
		Check(history.Execute(std::make_unique<vega::SetPropertyCommand>(id, 0, "Active", false)), "Component edit failed");
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
		Check(history.Execute(std::make_unique<vega::SetScriptPropertyCommand>(actor->GetUniqueID(), 2, "speed", 90.f)), "Scene script edit failed");
		ly::World restored{ nullptr };
		ly::SceneSerializer::Load(restored, ly::SceneSerializer::Save(original));
		context.world = &restored;
		const auto replacement = context.ResolveGuid(guid);
		const auto* restoredScript = replacement ? replacement->FindComponent<vega::script::ScriptComponent>() : nullptr;
		Check(restoredScript && std::get<float>(restoredScript->Properties().at("speed")) == 90.f, "Scene script snapshot lost its override");
		Check(history.Undo() && restoredScript->Overrides().empty() && std::get<float>(restoredScript->Properties().at("speed")) == 12.f,
			"Script undo must resolve GUID after reload");
		Check(history.Redo() && std::get<float>(restoredScript->Properties().at("speed")) == 90.f, "Script redo after reload failed");
	}
}

int main()
{
	TestScriptProperties();
	TestReflectedProperties();
	TestHistory();
	TestMergeAndDepth();
	TestDestroyAndResolution();
	TestSceneIdentity();
	TestSceneScriptIdentity();
	if (failures) return 1;
	std::cout << "Editor command tests passed\n";
	return 0;
}
