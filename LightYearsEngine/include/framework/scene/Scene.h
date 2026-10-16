#pragma once
#include "framework/EngineContext.h"

#include <nlohmann/json.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace ly
{
	class Actor;
	class ActorComponent;
	class World;

	struct SceneLoadResult
	{
		int spawned = 0;
		std::vector<std::string> warnings;
	};

	// Optional module extension. Scripts live in their own array and may add
	// components; ordinary reflected components opt into creation through an addable factory.
	struct ComponentSerializer
	{
		std::string array = "scripts";
		std::function<nlohmann::json(const ActorComponent&)> save;
		std::function<void(Actor&, const nlohmann::json&, SceneLoadResult&)> load;
	};

	class SceneSerializer
	{
	public:
		static nlohmann::json Save(World& world, nlohmann::json settings = nlohmann::json::object());
		// Null for destroyed actors or actors without a serializable scene identity/type.
		static nlohmann::json SaveEntity(Actor& actor, bool expanded = false);
		static nlohmann::json SaveSubtree(Actor& root, bool expanded = false);
		// Remap nested instance/subtree GUIDs and internal parent links together.
		static void RemapGuids(nlohmann::json& entities);
		static void RemapReferences(nlohmann::json& data, const std::map<std::string, std::string>& guids);
		static SceneLoadResult Load(World& world, const nlohmann::json& scene);
		static void RegisterComponentSerializer(const std::string& type, ComponentSerializer serializer,
			EngineContext& context = EngineContext::Current());
		// For actors whose reflected construction-only properties select their
		// constructor arguments. The type must still opt in through Spawnable.
		using SpawnFactory = std::function<std::shared_ptr<Actor>(World&, const nlohmann::json&)>;
		static void RegisterSpawnFactory(const std::string& type, SpawnFactory factory,
			EngineContext& context = EngineContext::Current());
	};

	// Extension hooks must follow their registry, particularly when two games
	// register different factories under the same persistent type name.
	struct SceneServices
	{
		std::map<std::string, ComponentSerializer> componentSerializers;
		std::map<std::string, SceneSerializer::SpawnFactory> spawnFactories;
	};
}
