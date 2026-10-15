#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/reflection/Reflection.h"
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace ly
{
	namespace
	{
		using Json = nlohmann::json;
		using namespace reflect;
		std::map<std::string, ComponentSerializer>& ComponentSerializers()
		{
			static std::map<std::string, ComponentSerializer> serializers;
			return serializers;
		}
		std::map<std::string, SceneSerializer::SpawnFactory>& SpawnFactories()
		{
			static std::map<std::string, SceneSerializer::SpawnFactory> factories;
			return factories;
		}
		Json Encode(const PropertyInfo& property, const PropertyValue& value)
		{
			if (property.kind == PropertyKind::Enum)
			{
				for (const auto& label : property.enumLabels)
					if (label.first == std::get<int>(value)) return label.second;
				return std::get<int>(value);
			}
			return std::visit([](const auto& item) -> Json
			{
				using T = std::decay_t<decltype(item)>;
				if constexpr (std::is_same_v<T, sf::Vector2f>) return Json::array({ item.x, item.y });
				else if constexpr (std::is_same_v<T, sf::Color>) return Json::array({ item.r, item.g, item.b, item.a });
				else return item;
			}, value);
		}
		float Number(const Json& input)
		{
			if (!input.is_number()) throw std::runtime_error("Expected number");
			const float value = input.get<float>();
			if (!std::isfinite(value)) throw std::runtime_error("Expected finite number");
			return value;
		}
		int Integer(const Json& input, int min, int max)
		{
			if (!input.is_number_integer()) throw std::runtime_error("Expected integer");
			const double value = input.get<double>();
			if (value < min || value > max) throw std::runtime_error("Integer out of range");
			return static_cast<int>(value);
		}
		PropertyValue Decode(const PropertyInfo& property, const Json& value)
		{
			switch (property.kind)
			{
			case PropertyKind::Bool: return value.get<bool>();
			case PropertyKind::Int: return Integer(value, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
			case PropertyKind::Float: return Number(value);
			case PropertyKind::String: return value.get<std::string>();
			case PropertyKind::Enum:
				if (value.is_number_integer()) return Integer(value, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
				if (value.is_string())
					for (const auto& label : property.enumLabels)
						if (label.second == value.get<std::string>()) return label.first;
				throw std::runtime_error("Unknown enum label");
			case PropertyKind::Vector2:
				if (!value.is_array() || value.size() != 2) throw std::runtime_error("Expected [x,y]");
				return sf::Vector2f{ Number(value[0]), Number(value[1]) };
			case PropertyKind::Color:
				if (!value.is_array() || value.size() != 4) throw std::runtime_error("Expected [r,g,b,a]");
				return sf::Color{ static_cast<std::uint8_t>(Integer(value[0], 0, 255)),
					static_cast<std::uint8_t>(Integer(value[1], 0, 255)),
					static_cast<std::uint8_t>(Integer(value[2], 0, 255)),
					static_cast<std::uint8_t>(Integer(value[3], 0, 255)) };
			}
			throw std::runtime_error("Unsupported property kind");
		}
		Json SaveProperties(const TypeInfo& type, const void* object)
		{
			Json result = Json::object();
			for (const auto* property : type.AllProperties())
				if (property->setter) result[property->name] = Encode(*property, property->getter(object));
			return result;
		}
		void ApplyProperties(const TypeInfo& type, void* object, const Json& properties, SceneLoadResult& result)
		{
			if (!properties.is_object())
			{
				result.warnings.push_back(type.name + ": properties must be an object");
				return;
			}
			for (const auto& entry : properties.items())
			{
				const auto* property = type.FindProperty(entry.key());
				try
				{
					if (!property || !property->setter) throw std::runtime_error("Unknown or read-only property");
					std::string error;
					if (!property->SetValue(object, Decode(*property, entry.value()), &error)) throw std::runtime_error(error);
				}
				catch (const std::exception& error)
				{
					result.warnings.push_back(type.name + "." + entry.key() + ": " + error.what());
				}
				catch (...) { result.warnings.push_back(type.name + "." + entry.key() + ": property failed"); }
			}
		}
		const Json& EmptyProperties()
		{
			static const Json empty = Json::object();
			return empty;
		}
	}

	void SceneSerializer::RegisterComponentSerializer(const std::string& type, ComponentSerializer serializer)
	{
		ComponentSerializers()[type] = std::move(serializer);
	}
	void SceneSerializer::RegisterSpawnFactory(const std::string& type, SpawnFactory factory)
	{
		SpawnFactories()[type] = std::move(factory);
	}

	nlohmann::json SceneSerializer::Save(World& world, Json settings)
	{
		reflect::RegisterEngineTypes();
		Json scene{ { "format", "vega.scene" }, { "version", 1 }, { "settings", std::move(settings) }, { "entities", Json::array() } };
		// Pending actors are authored state too; no gameplay tick is needed to save.
		for (const auto& weakActor : world.GetActorsByTypeIncludingPending<Actor>())
		{
			if (const auto actor = weakActor.lock())
			{
				auto entity = SaveEntity(*actor);
				if (!entity.is_null()) scene["entities"].push_back(std::move(entity));
			}
		}
		return scene;
	}

	nlohmann::json SceneSerializer::SaveEntity(Actor& actor)
	{
		reflect::RegisterEngineTypes();
		if (actor.GetIsPendingDestroy()) return nullptr;
		auto& registry = TypeRegistry::Get();
		const auto* identity = actor.FindComponent<SceneEntityComponent>();
		const auto* type = registry.FindMostDerived(actor);
		if (!identity || !type) return nullptr;
		Json entity{ { "guid", identity->guid }, { "name", identity->name }, { "type", type->name },
			{ "properties", SaveProperties(*type, &actor) }, { "components", Json::array() }, { "scripts", Json::array() } };
		actor.ForEachComponent([&](const ActorComponent& component)
		{
			if (dynamic_cast<const SceneEntityComponent*>(&component)) return;
			// Do not mistake an unregistered subclass for the base component.
			const auto* componentType = registry.Find(typeid(component));
			if (!componentType) return;
			const auto hook = ComponentSerializers().find(componentType->name);
			if (hook != ComponentSerializers().end())
			{
				const auto& serializer = hook->second;
				if (!entity.contains(serializer.array)) entity[serializer.array] = Json::array();
				entity[serializer.array].push_back(serializer.save(component));
				return;
			}
			Json properties = SaveProperties(*componentType, &component);
			if (!properties.empty()) entity["components"].push_back({ { "type", componentType->name }, { "properties", std::move(properties) } });
		});
		return entity;
	}

	SceneLoadResult SceneSerializer::Load(World& world, const Json& scene)
	{
		reflect::RegisterEngineTypes();
		SceneLoadResult result;
		try
		{
			if (!scene.is_object() || scene.value("format", std::string{}) != "vega.scene" ||
				!scene.contains("version") || !scene["version"].is_number_integer() || scene["version"] != 1 ||
				!scene.contains("entities") || !scene["entities"].is_array() ||
				!scene.contains("settings") || !scene["settings"].is_object())
			{
				result.warnings.push_back("Expected vega.scene version 1 with settings object and entities array");
				return result;
			}
			std::set<std::string> guids;
			for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>())
				if (auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy())
					if (const auto* identity = actor->FindComponent<SceneEntityComponent>()) guids.insert(identity->guid);
			for (const auto& entity : scene["entities"])
			{
				try
				{
					const std::string name = entity.at("type").get<std::string>();
					const auto* type = TypeRegistry::Get().Find(name);
					if (!type || !type->spawnable || !type->create || type->root != TypeRoot::Actor)
					{
						result.warnings.push_back("Unknown or non-spawnable actor type: " + name);
						continue;
					}
					std::string guid = entity.value("guid", std::string{});
					const std::string entityName = entity.value("name", std::string{});
					if (guid.empty()) guid = SceneEntityComponent::GenerateGuid();
					if (guids.count(guid))
					{
						result.warnings.push_back("Duplicate entity GUID: " + guid);
						continue;
					}
					const auto& properties = entity.contains("properties") ? entity["properties"] : EmptyProperties();
					const auto factory = SpawnFactories().find(name);
					const auto actor = factory != SpawnFactories().end() ? factory->second(world, properties) : type->create(world);
					if (!actor || actor->GetWorld() != &world || actor->GetIsPendingDestroy())
					{
						result.warnings.push_back("Factory failed for " + name);
						continue;
					}
					auto& identity = actor->GetOrAddComponent<SceneEntityComponent>();
					identity.guid = guid;
					identity.name = entityName;
					guids.insert(guid);
					++result.spawned;
					ApplyProperties(*type, actor.get(), properties, result);
					if (entity.contains("components"))
					{
						std::map<std::string, std::size_t> occurrences;
						if (!entity["components"].is_array()) result.warnings.push_back(name + ": components must be an array");
						else for (const auto& componentData : entity["components"])
						{
							try
							{
								const auto componentName = componentData.at("type").get<std::string>();
								const auto occurrence = occurrences[componentName]++;
								std::size_t matching = 0;
								ActorComponent* found = nullptr;
								actor->ForEachComponent([&](ActorComponent& component)
								{
									const auto* info = TypeRegistry::Get().Find(typeid(component));
									if (info && info->name == componentName && !dynamic_cast<SceneEntityComponent*>(&component) && matching++ == occurrence) found = &component;
								});
								if (!found || ComponentSerializers().count(componentName))
									result.warnings.push_back(name + ": unavailable component " + componentName);
								else ApplyProperties(*TypeRegistry::Get().Find(componentName), found,
									componentData.contains("properties") ? componentData["properties"] : EmptyProperties(), result);
							}
							catch (const std::exception& error) { result.warnings.push_back(name + ": component: " + error.what()); }
							catch (...) { result.warnings.push_back(name + ": component failed"); }
						}
					}
					bool scriptsHandled = false;
					for (const auto& hook : ComponentSerializers())
					{
						const auto& serializer = hook.second;
						if (serializer.array == "scripts") scriptsHandled = true;
						if (!entity.contains(serializer.array)) continue;
						if (!entity[serializer.array].is_array()) result.warnings.push_back(name + ": " + serializer.array + " must be an array");
						else for (const auto& data : entity[serializer.array])
						{
							try { serializer.load(*actor, data, result); }
							catch (const std::exception& error) { result.warnings.push_back(hook.first + ": " + error.what()); }
							catch (...) { result.warnings.push_back(hook.first + ": load failed"); }
						}
					}
					if (!scriptsHandled && entity.contains("scripts") && !entity["scripts"].empty())
						result.warnings.push_back(name + ": no script serializer registered");
				}
				catch (const std::exception& error) { result.warnings.push_back(std::string{ "Entity: " } + error.what()); }
				catch (...) { result.warnings.push_back("Entity load failed"); }
			}
		}
		catch (const std::exception& error) { result.warnings.push_back(std::string{ "Scene: " } + error.what()); }
		catch (...) { result.warnings.push_back("Scene load failed"); }
		return result;
	}
}
