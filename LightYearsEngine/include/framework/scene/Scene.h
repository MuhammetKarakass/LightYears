#pragma once

#include <nlohmann/json.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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
		static SceneLoadResult Load(World& world, const nlohmann::json& scene);
		static void RegisterComponentSerializer(const std::string& type, ComponentSerializer serializer);
		// For actors whose reflected construction-only properties select their
		// constructor arguments. The type must still opt in through Spawnable.
		using SpawnFactory = std::function<std::shared_ptr<Actor>(World&, const nlohmann::json&)>;
		static void RegisterSpawnFactory(const std::string& type, SpawnFactory factory);
	};
}
