#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/scene/PrefabLibrary.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/reflection/Reflection.h"
#include <framework/debug/Log.h>
#include <framework/debug/Profiler.h>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <algorithm>

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
		using ComponentCounts = std::map<std::string, std::size_t>;
		struct ComponentInventory
		{
			ComponentCounts counts;
			std::vector<std::string> order;
		};
		const ComponentInventory& ConstructorComponents(const TypeInfo& type)
		{
			static std::map<std::string, ComponentInventory> cache;
			const auto existing = cache.find(type.name);
			if (existing != cache.end()) return existing->second;
			ComponentInventory inventory;
			// Never begin or tick the throwaway actor: only constructor ownership matters.
			World scratch{ nullptr };
			if (type.create)
				if (const auto actor = type.create(scratch))
					actor->ForEachComponent([&](const ActorComponent& component)
					{
						const auto* info = TypeRegistry::Get().Find(typeid(component));
						if (info && !dynamic_cast<const SceneEntityComponent*>(&component) && !ComponentSerializers().count(info->name))
						{
							++inventory.counts[info->name];
							inventory.order.push_back(info->name);
						}
					});
			return cache.emplace(type.name, std::move(inventory)).first->second;
		}
		Json Encode(const PropertyInfo& property, const PropertyValue& value)
		{
			if (property.kind == PropertyKind::List)
			{
				Json list = Json::array();
				for (const auto& item : std::get<PropertyList>(value)) list.push_back(Encode(property.Element(), ToValue(item)));
				return list;
			}
			if (property.kind == PropertyKind::Flags)
			{
				Json labels = Json::array();
				for (const auto& label : property.enumLabels)
					if ((std::get<int>(value) & label.first) != 0) labels.push_back(label.second);
				return labels;
			}
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
				else if constexpr (std::is_same_v<T, ly::ActorRef>) return Json{ { "guid", item.Guid() } };
				else if constexpr (std::is_same_v<T, PropertyList>) return nullptr;
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
			case PropertyKind::ActorRef:
				if (!value.is_object() || !value.contains("guid") || !value["guid"].is_string()) throw std::runtime_error("Expected {guid: string}");
				return ly::ActorRef{ value["guid"].get<std::string>() };
			case PropertyKind::List:
			{
				if (!value.is_array() || value.size() > 4096) throw std::runtime_error("Expected array with at most 4096 elements");
				PropertyList list;
				for (const auto& item : value) list.push_back(ToScalar(Decode(property.Element(), item)));
				return list;
			}
			case PropertyKind::Bool: return value.get<bool>();
			case PropertyKind::Int: return Integer(value, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
			case PropertyKind::Float: return Number(value);
			case PropertyKind::String: return value.get<std::string>();
			case PropertyKind::Flags:
			{
				if (value.is_number_integer()) return Integer(value, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
				Json labels = value;
				// Older scenes stored pipe-separated combinations in a single enum label.
				if (value.is_string())
				{
					labels = Json::array();
					const auto text = value.get<std::string>();
					if (text == "None") return 0;
					std::size_t begin = 0;
					do
					{
						const auto end = text.find('|', begin);
						const auto part = text.substr(begin, end == std::string::npos ? end : end - begin);
						const auto first = part.find_first_not_of(" \t");
						labels.push_back(first == std::string::npos ? "" : part.substr(first, part.find_last_not_of(" \t") - first + 1));
						if (end == std::string::npos) break;
						begin = end + 1;
					} while (true);
				}
				if (!labels.is_array()) throw std::runtime_error("Expected flags label array or integer");
				int bits = 0;
				for (const auto& item : labels)
				{
					bool found = false;
					for (const auto& label : property.enumLabels)
						if (item.is_string() && label.second == item.get<std::string>()) { bits |= label.first; found = true; break; }
					if (!found) throw std::runtime_error("Unknown flags label");
				}
				return bits;
			}
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
		LY_PROFILE_SCOPE("Scene Save");
		reflect::RegisterEngineTypes();
		Json scene{ { "format", "vega.scene" }, { "version", 1 }, { "settings", std::move(settings) }, { "entities", Json::array() } };
		// Pending actors are authored state too; no gameplay tick is needed to save.
		auto actors = world.GetActorsByTypeIncludingPending<Actor>();
		const auto depth = [](const auto& weak)
		{
			int value = 0;
			if (const auto actor = weak.lock()) for (auto* parent = actor->GetParent(); parent; parent = parent->GetParent()) ++value;
			return value;
		};
		std::stable_sort(actors.begin(), actors.end(), [&](const auto& a, const auto& b) { return depth(a) < depth(b); });
		for (const auto& weakActor : actors)
		{
			if (const auto actor = weakActor.lock())
			{
				bool insideInstance = false;
				for (auto* parent = actor->GetParent(); parent; parent = parent->GetParent())
				{
					const auto* owner = parent->FindComponent<SceneEntityComponent>();
					if (owner && !owner->prefab.empty() && owner->prefabRootGuid.empty()) insideInstance = true;
				}
				if (insideInstance) continue;
				auto entity = SaveEntity(*actor);
				if (!entity.is_null()) scene["entities"].push_back(std::move(entity));
			}
		}
		for (const auto& missing : world.unresolvedPrefabs) scene["entities"].push_back(missing);
		return scene;
	}

	nlohmann::json SceneSerializer::SaveEntity(Actor& actor, bool expanded)
	{
		reflect::RegisterEngineTypes();
		if (actor.GetIsPendingDestroy()) return nullptr;
		auto& registry = TypeRegistry::Get();
		const auto* identity = actor.FindComponent<SceneEntityComponent>();
		const auto* type = registry.FindMostDerived(actor);
		if (!identity || !type) return nullptr;
		Json entity{ { "guid", identity->guid }, { "name", identity->name }, { "type", type->name },
			{ "properties", SaveProperties(*type, &actor) }, { "components", Json::array() }, { "scripts", Json::array() } };
		if (const auto* parent = actor.GetParent())
		{
			if (const auto* parentIdentity = parent->FindComponent<SceneEntityComponent>()) entity["parent"] = parentIdentity->guid;
			entity["properties"]["Location"] = { actor.GetRelativeLocation().x, actor.GetRelativeLocation().y };
			entity["properties"]["Rotation"] = actor.GetRelativeRotation();
		}
		if (!identity->childId.empty()) entity["id"] = identity->childId;
		const auto& constructor = ConstructorComponents(*type);
		ComponentCounts occurrences;
		std::map<std::string, std::vector<Json>> defaults;
		Json additions = Json::array();
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
			const auto occurrence = occurrences[componentType->name]++;
			const auto baseline = constructor.counts.find(componentType->name);
			const bool added = baseline == constructor.counts.end() || occurrence >= baseline->second;
			Json data{ { "type", componentType->name }, { "properties", std::move(properties) } };
			if (added)
			{
				data["added"] = true;
				additions.push_back(std::move(data));
			}
			else defaults[componentType->name].push_back(std::move(data));
		});
		// Canonical constructor order also covers remove-and-readd edits: runtime
		// insertion order must not change the serialized result after a reload.
		ComponentCounts emitted;
		for (const auto& componentName : constructor.order)
		{
			const auto occurrence = emitted[componentName]++;
			const auto& entries = defaults[componentName];
			if (occurrence < entries.size()) entity["components"].push_back(entries[occurrence]);
		}
		for (auto& addition : additions) entity["components"].push_back(std::move(addition));
		Json removed = Json::array();
		for (const auto& entry : constructor.counts)
			for (auto count = occurrences[entry.first]; count < entry.second; ++count) removed.push_back(entry.first);
		if (!removed.empty()) entity["removed"] = std::move(removed);
		if (!identity->prefab.empty() && identity->prefabRootGuid.empty())
		{
			entity["children"] = Json::array();
			std::set<std::string> ids;
			for (Actor* child : actor.GetChildren())
			{
				auto data = SaveSubtree(*child, true);
				if (data.is_null()) continue;
				if (!data.contains("id")) data["id"] = data["guid"];
				if (!ids.insert(data["id"].get<std::string>()).second)
				{
					auto id = data["guid"].get<std::string>();
					std::replace(id.begin(), id.end(), '/', '_');
					while (!ids.insert(id).second) id += '_';
					data["id"] = id;
				}
				entity["children"].push_back(std::move(data));
			}
		}
		if (!expanded && !identity->prefab.empty() && identity->prefabRootGuid.empty())
		{
			auto delta = PrefabLibrary::Diff(entity, identity->prefabBaseline);
			delta["prefab"] = identity->prefab;
			delta["guid"] = identity->guid;
			delta["name"] = identity->name;
			delta["properties"]["Location"] = entity["properties"]["Location"];
			if (entity.contains("parent")) delta["parent"] = entity["parent"];
			return delta;
		}
		return entity;
	}

	Json SceneSerializer::SaveSubtree(Actor& root, bool expanded)
	{
		auto entity = SaveEntity(root, expanded);
		if (entity.is_null()) return entity;
		const auto* identity = root.FindComponent<SceneEntityComponent>();
		if (!expanded && identity && !identity->prefab.empty() && identity->prefabRootGuid.empty()) return entity;
		if (!root.GetChildren().empty())
		{
			entity["children"] = Json::array();
			std::set<std::string> ids;
			for (Actor* child : root.GetChildren())
			{
				auto data = SaveSubtree(*child, expanded);
				if (data.is_null()) continue;
				if (!data.contains("id")) data["id"] = data["guid"];
				if (!ids.insert(data["id"].get<std::string>()).second)
				{
					auto id = data["guid"].get<std::string>();
					std::replace(id.begin(), id.end(), '/', '_');
					while (!ids.insert(id).second) id += '_';
					data["id"] = id;
				}
				entity["children"].push_back(std::move(data));
			}
		}
		return entity;
	}

	void SceneSerializer::RemapGuids(Json& entities)
	{
		std::map<std::string, std::string> remap;
		const auto visit = [&](auto&& self, Json& nodes, bool assign) -> void
		{
			for (auto& node : nodes)
			{
				if (assign)
				{
					const auto old = node.value("guid", std::string{});
					const auto guid = SceneEntityComponent::GenerateGuid();
					if (!old.empty()) remap[old] = guid;
					node["guid"] = guid;
				}
				else if (node.contains("parent"))
				{
					const auto found = remap.find(node["parent"].get<std::string>());
					if (found != remap.end()) node["parent"] = found->second;
				}
				if (node.contains("children")) self(self, node["children"], assign);
			}
		};
		visit(visit, entities, true);
		visit(visit, entities, false);
		RemapReferences(entities, remap);
	}

	void SceneSerializer::RemapReferences(Json& data, const std::map<std::string, std::string>& remap)
	{
		const auto refs = [&](auto&& self, Json& node) -> void
		{
			if (node.is_object() && node.size() == 1 && node.contains("guid") && node["guid"].is_string())
			{
				const auto found = remap.find(node["guid"].get<std::string>());
				if (found != remap.end()) node["guid"] = found->second;
			}
			else if (node.is_structured()) for (auto& child : node) self(self, child);
		};
		refs(refs, data);
	}

	SceneLoadResult SceneSerializer::Load(World& world, const Json& scene)
	{
		LY_PROFILE_SCOPE("Scene Load");
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
				for (const auto& warning : result.warnings)
					debug::Log::Write(debug::LogLevel::Warning, debug::LogChannel::Scene, LY_SOURCE_LOCATION, warning);
				return result;
			}
			std::set<std::string> guids;
			for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>())
				if (auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy())
					if (const auto* identity = actor->FindComponent<SceneEntityComponent>()) guids.insert(identity->guid);
			for (const auto& missing : world.unresolvedPrefabs)
				if (missing.is_object() && missing.contains("guid") && missing["guid"].is_string()) guids.insert(missing["guid"].get<std::string>());
			Json flattened = Json::array();
			const auto expand = [&](auto&& self, Json entity, Json baseline, const std::string& parent,
				const std::string& prefabPath, const std::string& prefabRoot, unsigned depth) -> void
			{
				if (depth > 128) throw std::runtime_error("Hierarchy depth limit");
				std::string path = prefabPath, root = prefabRoot;
				if (entity.contains("prefab"))
				{
					path = entity.at("prefab").get<std::string>();
					auto resolved = PrefabLibrary::Get().Resolve(path);
					result.warnings.insert(result.warnings.end(), resolved.warnings.begin(), resolved.warnings.end());
					if (!resolved)
					{
						const auto guid = entity.value("guid", std::string{});
						if (!guid.empty() && !guids.insert(guid).second) result.warnings.push_back("Duplicate entity GUID: " + guid);
						else world.unresolvedPrefabs.push_back(entity);
						return;
					}
					baseline = resolved.entity;
					entity = PrefabLibrary::Merge(std::move(resolved.entity), entity);
				}
				if (!entity.contains("guid")) entity["guid"] = SceneEntityComponent::GenerateGuid();
				if (entity.contains("prefab") || (parent.empty() && prefabPath.empty()))
				{
					std::map<std::string, std::string> references;
					const auto assign = [&](auto&& visit, Json& node, const std::string& address, const std::string& fallback, unsigned level) -> void
					{
						if (level > 128) throw std::runtime_error("Hierarchy depth limit");
						if (!node.contains("guid")) node["guid"] = fallback;
						const auto identity = node.at("guid").get<std::string>();
						references["$prefab:" + address] = identity;
						if (node.contains("children") && node["children"].is_array()) for (auto& child : node["children"])
						{
							const auto id = child.at("id").get<std::string>();
							visit(visit, child, address + "/" + id, identity + "/" + id, level + 1);
						}
					};
					assign(assign, entity, {}, entity.at("guid").get<std::string>(), 0);
					RemapReferences(entity, references);
					RemapReferences(baseline, references);
				}
				const auto guid = entity.at("guid").get<std::string>();
				if (!parent.empty()) entity["parent"] = parent;
				if (!path.empty() && root.empty()) root = guid;
				const auto children = entity.value("children", Json::array());
				if (!children.is_array()) throw std::runtime_error("children must be an array");
				entity.erase("children");
				entity.erase("prefab");
				entity["_prefab"] = path;
				entity["_baseline"] = baseline;
				entity["_root"] = root == guid ? std::string{} : root;
				flattened.push_back(std::move(entity));
				std::set<std::string> ids;
				for (auto child : children)
				{
					const auto id = child.at("id").get<std::string>();
					if (id.empty() || !ids.insert(id).second) throw std::runtime_error("Duplicate or empty prefab child id");
					if (!child.contains("guid")) child["guid"] = guid + "/" + id;
					Json childBaseline = Json::object();
					if (baseline.is_object() && baseline.contains("children"))
						for (const auto& candidate : baseline["children"]) if (candidate.at("id") == id) childBaseline = candidate;
					self(self, std::move(child), childBaseline, guid, path, root, depth + 1);
				}
			};
			for (const auto& authored : scene["entities"])
			{
				try { expand(expand, authored, Json::object(), {}, {}, {}, 0); }
				catch (const std::exception& error) { result.warnings.push_back(std::string{ "Hierarchy: " } + error.what()); }
			}
			std::map<std::string, std::shared_ptr<Actor>> actors;
			for (const auto& weak : world.GetActorsByTypeIncludingPending<Actor>())
				if (auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy())
					if (const auto* identity = actor->FindComponent<SceneEntityComponent>()) actors[identity->guid] = actor;
			struct Attachment { std::shared_ptr<Actor> actor; std::string parent; sf::Vector2f location; float rotation; };
			std::vector<Attachment> attachments;
			for (const auto& entity : flattened)
			{
				try
				{
					auto prefabBaseline = entity.at("_baseline");
					auto prefab = entity.at("_prefab").get<std::string>();
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
					identity.childId = entity.value("id", std::string{});
					identity.prefabRootGuid = entity.value("_root", std::string{});
					actors[guid] = actor;
					guids.insert(guid);
					++result.spawned;
					if (entity.contains("removed"))
					{
						if (!entity["removed"].is_array()) result.warnings.push_back(name + ": removed must be an array");
						else for (const auto& removed : entity["removed"])
						{
							if (!removed.is_string()) { result.warnings.push_back(name + ": removed type must be a string"); continue; }
							const auto componentName = removed.get<std::string>();
							const auto* info = TypeRegistry::Get().Find(componentName);
							ActorComponent* found = nullptr;
							actor->ForEachComponent([&](ActorComponent& component)
							{
								if (!found && info && TypeRegistry::Get().Find(typeid(component)) == info) found = &component;
							});
							if (!info || !info->removable || type->RequiresComponent(componentName) || !found || !actor->RemoveComponent(*found))
								result.warnings.push_back(name + ": cannot remove unknown, required or unavailable component " + componentName);
						}
					}
					// Create the complete component set before any property setter can query it.
					if (entity.contains("components") && entity["components"].is_array())
						for (const auto& data : entity["components"])
						{
							try
							{
								if (!data.value("added", false)) continue;
								const auto componentName = data.at("type").get<std::string>();
								const auto* info = TypeRegistry::Get().Find(componentName);
								if (!info || !info->addComponent || ComponentSerializers().count(componentName))
									result.warnings.push_back(name + ": unknown or non-addable component " + componentName);
								else info->addComponent(*actor);
							}
							catch (const std::exception& error) { result.warnings.push_back(name + ": add component: " + error.what()); }
						}
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
								const auto* componentType = TypeRegistry::Get().Find(componentName);
								if (componentData.value("added", false) &&
									(!componentType || !componentType->addComponent || ComponentSerializers().count(componentName))) continue;
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
					if (entity.contains("parent")) attachments.push_back({ actor, entity.at("parent").get<std::string>(), actor->GetActorLocation(), actor->GetActorRotation() });
					// Added instance children have no inherited property address until
					// the root's structural changes are applied to the prefab asset.
					identity.prefab = !identity.prefabRootGuid.empty() && !prefabBaseline.contains("type") ? std::string{} : std::move(prefab);
					identity.prefabBaseline = std::move(prefabBaseline);
				}
				catch (const std::exception& error) { result.warnings.push_back(std::string{ "Entity: " } + error.what()); }
				catch (...) { result.warnings.push_back("Entity load failed"); }
			}
			// References resolve only after all actors and their identities exist.
			world.InvalidateActorReferences();
			for (const auto& entry : actors)
			{
				const auto resolve = [&](const TypeInfo* type, void* object)
				{
					if (!type) return;
					for (const auto* property : type->AllProperties())
					{
						std::string error;
						if (!property->ResolveReferences(object, world, &error)) result.warnings.push_back(type->name + "." + property->name + ": " + error);
					}
				};
				resolve(TypeRegistry::Get().FindMostDerived(*entry.second), entry.second.get());
				entry.second->ForEachComponent([&](ActorComponent& component) { resolve(TypeRegistry::Get().FindMostDerived(component), &component); });
			}
			for (const auto& link : attachments)
			{
				const auto parent = actors.find(link.parent);
				if (parent == actors.end() || !link.actor->AttachTo(parent->second.get(), false))
					result.warnings.push_back("Invalid, missing, cyclic or dynamic parent: " + link.parent);
				else
				{
					link.actor->SetRelativeLocation(link.location);
					link.actor->SetRelativeRotation(link.rotation);
				}
			}
		}
		catch (const std::exception& error) { result.warnings.push_back(std::string{ "Scene: " } + error.what()); }
		catch (...) { result.warnings.push_back("Scene load failed"); }
		for (const auto& warning : result.warnings)
			debug::Log::Write(debug::LogLevel::Warning, debug::LogChannel::Scene, LY_SOURCE_LOCATION, warning);
		return result;
	}
}
