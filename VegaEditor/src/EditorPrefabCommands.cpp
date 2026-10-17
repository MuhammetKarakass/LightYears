#include "editor/EditorCommands.h"
#include "EditorCommandsInternal.h"
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <algorithm>
#include <map>

namespace vega
{
	using editor_commands_internal::EntityScene;

	bool RestoreSceneEntity(EditorContext& context, const nlohmann::json& entity)
	{
		if (!context.world || context.playMode || context.transitioning) return false;
		const auto guid = entity.at("guid").get<std::string>();
		const auto actor = context.ResolveGuid(guid);
		const auto missing = std::find_if(context.world->unresolvedPrefabs.begin(), context.world->unresolvedPrefabs.end(),
			[&](const auto& data) { return data.value("guid", std::string{}) == guid; });
		if (!actor && missing == context.world->unresolvedPrefabs.end()) return false;
		auto staged = entity;
		const auto eraseMissing = [&]()
		{
			if (missing != context.world->unresolvedPrefabs.end()) context.world->unresolvedPrefabs.erase(missing);
		};
		if (staged.value("_unresolved", false) || (staged.contains("prefab") && !staged.contains("type") &&
			!context.Engine().Prefabs().Resolve(staged["prefab"].get<std::string>())))
		{
			staged.erase("_unresolved");
			eraseMissing();
			context.world->unresolvedPrefabs.push_back(std::move(staged));
			if (actor) actor->Destroy();
			context.ResetActorCache();
			return true;
		}
		const auto snapshotPrefab = staged.contains("type") ? staged.value("prefab", std::string{}) : std::string{};
		const auto savedBaseline = staged.value("_prefabBaseline", nlohmann::json::object());
		staged.erase("_prefabBaseline");
		if (!snapshotPrefab.empty()) staged.erase("prefab");
		// Stage with fresh identities so the old subtree stays intact until creation succeeds.
		std::map<std::string, std::string> publish;
		std::map<std::string, std::string> staging;
		const auto stage = [&](auto&& self, nlohmann::json& node) -> void
		{
			const auto old = node.value("guid", std::string{});
			const auto temporary = ly::SceneEntityComponent::GenerateGuid();
			if (!old.empty()) { publish[temporary] = old; staging[old] = temporary; }
			node["guid"] = temporary;
			if (node.contains("children")) for (auto& child : node["children"]) self(self, child);
		};
		stage(stage, staged);
		ly::SceneSerializer::RemapReferences(staged, staging);
		const auto temporary = staged.at("guid").get<std::string>();
		const auto loaded = ly::SceneSerializer::Load(*context.world, EntityScene(staged));
		context.ResetActorCache();
		const auto replacement = context.ResolveGuid(temporary);
		if (loaded.spawned < 1 || !replacement) return false;
		const bool selected = actor && context.selection.Contains(actor);
		const auto oldIdentity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr;
		if (oldIdentity && !oldIdentity->prefabRootGuid.empty())
		{
			auto* identity = replacement->FindComponent<ly::SceneEntityComponent>();
			identity->prefab = oldIdentity->prefab;
			identity->prefabRootGuid = oldIdentity->prefabRootGuid;
			identity->childId = oldIdentity->childId;
			identity->prefabBaseline = oldIdentity->prefabBaseline;
		}
		struct Survivor { ly::Actor* actor; sf::Vector2f location; float rotation; };
		std::vector<Survivor> survivors;
		if (actor && !staged.contains("children") && !entity.contains("prefab"))
			while (!actor->GetChildren().empty())
			{
				auto* child = actor->GetChildren().back();
				survivors.push_back({ child, child->GetRelativeLocation(), child->GetRelativeRotation() });
				child->Detach();
			}
		if (actor) actor->Destroy();
		eraseMissing();
		const auto publishTree = [&](auto&& self, ly::Actor& current, const nlohmann::json& baseline) -> void
		{
			auto* identity = current.FindComponent<ly::SceneEntityComponent>();
			const auto found = publish.find(identity->guid);
			if (found != publish.end()) identity->guid = found->second;
			else if (identity->guid.compare(0, temporary.size(), temporary) == 0)
			{
				const auto old = identity->guid;
				identity->guid = guid + identity->guid.substr(temporary.size());
				publish[old] = identity->guid;
			}
			if (identity->prefabRootGuid == temporary) identity->prefabRootGuid = guid;
			if (!snapshotPrefab.empty())
			{
				identity->prefab = snapshotPrefab;
				identity->prefabRootGuid = &current == replacement.get() ? std::string{} : guid;
				identity->prefabBaseline = baseline;
			}
			for (auto* child : current.GetChildren())
			{
				nlohmann::json childBaseline = nlohmann::json::object();
				if (baseline.contains("children")) for (const auto& candidate : baseline["children"])
					if (candidate.at("id") == child->FindComponent<ly::SceneEntityComponent>()->childId) childBaseline = candidate;
				self(self, *child, childBaseline);
			}
		};
		auto baseline = savedBaseline;
		if (!snapshotPrefab.empty()) if (const auto resolved = context.Engine().Prefabs().Resolve(snapshotPrefab)) baseline = resolved.entity;
		publishTree(publishTree, *replacement, baseline);
		// Staging changes identities twice. Publish property GUIDs and prefab baselines
		// together so undo/refresh never retain references to throwaway identities.
		const auto addresses = [&](auto&& self, ly::Actor& current, const std::string& address) -> void
		{
			publish["$prefab:" + address] = current.FindComponent<ly::SceneEntityComponent>()->guid;
			for (auto* child : current.GetChildren()) self(self, *child, address + "/" + child->FindComponent<ly::SceneEntityComponent>()->childId);
		};
		addresses(addresses, *replacement, {});
		context.world->InvalidateActorReferences();
		const auto references = [&](auto&& self, ly::Actor& current) -> void
		{
			auto* identity = current.FindComponent<ly::SceneEntityComponent>();
			ly::SceneSerializer::RemapReferences(identity->prefabBaseline, publish);
			const auto remap = [&](const ReflectedTarget& target)
			{
				if (!target.type) return;
				for (const auto* property : target.type->AllProperties())
				{
					if (!property->setter) continue;
					auto value = property->getter(target.object);
					const auto update = [&](ly::ActorRef& ref)
					{
						const auto found = publish.find(ref.Guid());
						if (found != publish.end()) ref = ly::ActorRef{found->second};
					};
					if (auto* ref = std::get_if<ly::ActorRef>(&value)) update(*ref);
					else if (auto* list = std::get_if<ly::reflect::PropertyList>(&value))
					{
						for (auto& item : *list) if (auto* ref = std::get_if<ly::ActorRef>(&item)) update(*ref);
					}
					else continue;
					property->SetValue(target.object, value);
				}
			};
			remap(ResolvePropertyTarget(current, {}));
			current.ForEachComponent([&](ly::ActorComponent& component) { remap(ResolvePropertyTarget(current, ly::IdentifyComponent(current, component))); });
			for (auto* child : current.GetChildren()) self(self, *child);
		};
		references(references, *replacement);
		for (const auto& survivor : survivors)
		{
			survivor.actor->AttachTo(replacement.get());
			survivor.actor->SetRelativeLocation(survivor.location);
			survivor.actor->SetRelativeRotation(survivor.rotation);
		}
		context.ResetActorCache();
		if (selected) context.selection.Replace(actor, replacement);
		return true;
	}
	bool ReplaceEntities::Execute(EditorContext& context)
	{
		std::size_t applied = 0;
		for (const auto& entity : mAfter)
		{
			if (!RestoreSceneEntity(context, entity))
			{
				while (applied > 0) RestoreSceneEntity(context, mBefore[--applied]);
				return false;
			}
			++applied;
		}
		return true;
	}
	void ReplaceEntities::Undo(EditorContext& context)
	{
		for (const auto& entity : mBefore) RestoreSceneEntity(context, entity);
	}

}
