#include "editor/EditorCommands.h"
#include "EditorCommandsInternal.h"
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <algorithm>
#include <unordered_set>
#include <string_view>

namespace vega
{
	using editor_commands_internal::EntityScene;
	using editor_commands_internal::SelectionSubtrees;

	bool ReparentActor::Execute(EditorContext& context)
	{
		if (context.playMode || context.transitioning) return false;
		const auto child = context.ResolveGuid(mChild), parent = context.ResolveGuid(mParent);
		if (!child || (!mParent.empty() && !parent)) return false;
		const auto* before = child->GetParent() ? child->GetParent()->FindComponent<vega::SceneEntityComponent>() : nullptr;
		const auto beforeParent = before ? before->guid : std::string{};
		const auto location = child->GetRelativeLocation();
		const auto rotation = child->GetRelativeRotation();
		auto states = nlohmann::json::array();
		const auto capture = [&](auto&& self, vega::Actor& actor) -> void
		{
			if (const auto* identity = actor.FindComponent<vega::SceneEntityComponent>())
				states.push_back({ { "guid", identity->guid }, { "prefab", identity->prefab }, { "root", identity->prefabRootGuid },
					{ "id", identity->childId }, { "baseline", identity->prefabBaseline } });
			for (auto* descendant : actor.GetChildren()) self(self, *descendant);
		};
		capture(capture, *child);
		if (beforeParent == mParent || !child->AttachTo(parent.get())) return false;
		// Moving an inherited child changes its prefab address. Preserve it as an
		// added subtree at the destination; undo restores the inherited identities.
		if (const auto* identity = child->FindComponent<vega::SceneEntityComponent>(); identity && !identity->prefabRootGuid.empty())
			for (const auto& state : states)
				if (const auto actor = context.ResolveGuid(state.at("guid").get<std::string>()))
				{
					auto* data = actor->FindComponent<vega::SceneEntityComponent>();
					data->prefab.clear(); data->prefabRootGuid.clear(); data->prefabBaseline = nullptr;
					data->childId = data->guid;
					std::replace(data->childId.begin(), data->childId.end(), '/', '_');
				}
		if (!mExecuted)
		{
			mBeforeParent = beforeParent; mBeforeLocation = location; mBeforeRotation = rotation;
			mPrefabStates = std::move(states); mExecuted = true;
		}
		return true;
	}

	void ReparentActor::Undo(EditorContext& context)
	{
		const auto child = context.ResolveGuid(mChild), parent = context.ResolveGuid(mBeforeParent);
		if (!child || !mExecuted || (!mBeforeParent.empty() && !parent) || !child->AttachTo(parent.get())) return;
		child->SetRelativeLocation(mBeforeLocation);
		child->SetRelativeRotation(mBeforeRotation);
		for (const auto& state : mPrefabStates)
			if (const auto actor = context.ResolveGuid(state.at("guid").get<std::string>()))
			{
				auto* data = actor->FindComponent<vega::SceneEntityComponent>();
				data->prefab = state.at("prefab").get<std::string>(); data->prefabRootGuid = state.at("root").get<std::string>();
				data->childId = state.at("id").get<std::string>(); data->prefabBaseline = state.at("baseline");
			}
	}

	bool DestroyActor::Execute(EditorContext& context)
	{
		if (context.playMode) return false;
		auto actor = context.Resolve(mID, mGuid);
		if (!actor) return false;
		if (!mGuid.empty())
		{
			mEntity = vega::SceneSerializer::SaveSubtree(*actor);
			if (mEntity.is_null()) return false;
		}
		actor->Destroy();
		context.selection.Remove(actor);
		return true;
	}

	void DestroyActor::Undo(EditorContext& context)
	{
		if (mEntity.is_null() || !context.world) return;
		if (vega::SceneSerializer::Load(*context.world, EntityScene(mEntity)).spawned >= 1)
			context.selection.Select(context.ResolveGuid(mGuid));
	}

	bool CreateEntity::Execute(EditorContext& context)
	{
		if (!context.world || context.playMode) return false;
		if (vega::SceneSerializer::Load(*context.world, EntityScene(mEntity)).spawned < 1) return false;
		const auto guid = mEntity.at("guid").get<std::string>();
		// Constructors may choose random defaults (asteroid textures). Freeze the
		// realized serialized data so redo recreates exactly the authored entity.
		const auto actor = context.ResolveGuid(guid);
		if (!actor) return false;
		if (!context.attachmentRoot.empty() && !mEntity.contains("parent"))
		{
			const auto parent = context.ResolveGuid(context.attachmentRoot);
			if (!parent || !actor->AttachTo(parent.get())) { actor->Destroy(); return false; }
		}
		mEntity = vega::SceneSerializer::SaveSubtree(*actor);
		context.selection.Select(actor);
		return true;
	}

	void CreateEntity::Undo(EditorContext& context)
	{
		if (auto actor = context.ResolveGuid(mEntity.at("guid").get<std::string>())) actor->Destroy();
		context.selection.Clear();
	}

	bool CreateSceneEntity(EditorContext& context, CommandHistory& history, const std::string& type)
	{
		if (!context.world || context.playMode) return false;
		const auto* info = context.Engine().Types().Find(type);
		if (!info || !info->spawnable || !info->create || info->root != vega::reflect::TypeRoot::Actor) return false;
		std::unordered_set<std::string_view> names;
		for (const auto& weak : context.world->GetActorsByTypeIncludingPending<vega::Actor>())
			if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy())
				if (const auto* identity = actor->FindComponent<vega::SceneEntityComponent>()) names.insert(identity->name);
		unsigned int number = 1;
		std::string name;
		do
		{
			name = type + " " + std::to_string(number++);
		} while (names.count(name));
		const auto centre = context.camera ? context.camera->GetView().getCenter() : context.world->GetWorldView().getCenter();
		return history.Execute(std::make_unique<CreateEntity>(nlohmann::json{
			{ "guid", vega::SceneEntityComponent::GenerateGuid() }, { "name", name }, { "type", type },
			{ "properties", { { "Location", { centre.x, centre.y } } } } }));
	}

	bool DuplicateSceneEntity(EditorContext& context, CommandHistory& history)
	{
		if (!context.world || context.playMode || context.transitioning) return false;
		auto command = std::make_unique<CompoundCommand>("Duplicate selection", 0, true);
		auto entities = SelectionSubtrees(context);
		if (entities.empty()) return false;
		vega::SceneSerializer::RemapGuids(entities);
		for (auto& entity : entities)
		{
			auto& location = entity["properties"]["Location"];
			location = { location[0].get<float>() + 32.f, location[1].get<float>() + 32.f };
			command->Add(std::make_unique<CreateEntity>(std::move(entity)));
		}
		return history.Execute(std::move(command));
	}

	bool RenameSceneEntity(EditorContext& context, CommandHistory& history, const std::string& name, std::uint64_t group)
	{
		const auto actor = context.selection.Get();
		if (!actor || context.playMode) return false;
		const auto target = FindPropertyTarget(*actor, "SceneEntityComponent");
		return target.object && history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIdentity, "Name", name, group));
	}
}
