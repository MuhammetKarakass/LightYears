#pragma once

#include "editor/EditorCommands.h"
#include <framework/scene/Scene.h>

namespace vega
{
	namespace editor_commands_internal
	{
		inline bool HasSelectedAncestor(const EditorSelection& selection, const ly::Actor& actor)
		{
			for (auto* parent = actor.GetParent(); parent; parent = parent->GetParent())
				if (selection.Contains(parent->GetWeakActor().lock())) return true;
			return false;
		}
		inline nlohmann::json SelectionSubtrees(EditorContext& context)
		{
			auto result = nlohmann::json::array();
			for (const auto& actor : context.selection.GetAll())
			{
				if (HasSelectedAncestor(context.selection, *actor)) continue;
				auto entity = ly::SceneSerializer::SaveSubtree(*actor);
				if (entity.is_null()) return nlohmann::json::array();
				entity.erase("parent");
				entity["properties"]["Location"] = { actor->GetActorLocation().x, actor->GetActorLocation().y };
				entity["properties"]["Rotation"] = actor->GetActorRotation();
				result.push_back(std::move(entity));
			}
			return result;
		}
		inline nlohmann::json EntityScene(const nlohmann::json& entity)
		{
			return { { "format", "vega.scene" }, { "version", 1 }, { "settings", nlohmann::json::object() },
				{ "entities", nlohmann::json::array({ entity }) } };
		}
	}
}