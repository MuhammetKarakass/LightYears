#pragma once

#include "framework/ActorComponent.h"
#include <nlohmann/json.hpp>
#include <string>

namespace ly
{
	// Runtime object IDs change on every spawn. Scene identity belongs to the
	// authored entity and remains intact when a world is saved and loaded.
	class SceneEntityComponent : public ActorComponent
	{
	public:
		explicit SceneEntityComponent(Actor& owner, std::string guid = {}, std::string name = {});
		const std::string& GetGuid() const { return guid; }
		const std::string& GetName() const { return name; }
		void SetName(const std::string& value) { name = value; }
		static std::string GenerateGuid();

		std::string guid;
		std::string name;
		// Construction-only properties retain requested values for the next load.
		nlohmann::json spawnData = nlohmann::json::object();
		std::string prefab;
		// Retain the load baseline so refresh preserves overrides after disk changes.
		nlohmann::json prefabBaseline;
	};
}
