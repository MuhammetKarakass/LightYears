#include <framework/debug/Log.h>
#include <framework/debug/Profiler.h>
#include "framework/scene/PrefabLibrary.h"
#include "framework/scene/Scene.h"
#include "framework/scene/SceneEntityComponent.h"
#include "framework/AssetManager.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include <fstream>
#include <algorithm>

namespace ly
{
	using Json = nlohmann::json;
	namespace
	{
		Json PropertiesDiff(const Json& value, const Json& base)
		{
			Json result = Json::object();
			for (const auto& item : value.items())
				if (!base.contains(item.key()) || base[item.key()] != item.value()) result[item.key()] = item.value();
			return result;
		}
		Json Entries(const Json& entity, const char* key)
		{
			return entity.contains(key) && entity[key].is_array() ? entity[key] : Json::array();
		}
	}
	PrefabLibrary::PrefabLibrary(std::filesystem::path root) : mRoot{ std::move(root) } {}
	PrefabLibrary& PrefabLibrary::Get()
	{
		static PrefabLibrary library{ {} };
		if (library.mRoot.empty()) library.SetRoot(AssetManager::GetAssetManager().GetAssetRootDirectory());
		return library;
	}
	void PrefabLibrary::SetRoot(std::filesystem::path root)
	{
		mRoot = std::filesystem::absolute(root).lexically_normal();
		Reload();
	}
	void PrefabLibrary::Reload()
	{
		mCache.clear();
		mResolved.clear();
	}
	Json PrefabLibrary::Load(const std::string& path, std::vector<std::string>& warnings)
	{
		try
		{
			const auto relative = std::filesystem::u8path(path);
			const auto absolute = std::filesystem::weakly_canonical(mRoot / relative);
			const auto local = absolute.lexically_relative(std::filesystem::weakly_canonical(mRoot));
			if (relative.is_absolute() || local.empty() || *local.begin() == ".." ||
				path.size() < 12 || path.substr(path.size() - 12) != ".prefab.json")
				throw std::runtime_error("Expected an asset-relative .prefab.json path");
			const auto modified = std::filesystem::last_write_time(absolute);
			const auto size = std::filesystem::file_size(absolute);
			const auto found = mCache.find(absolute);
			if (found != mCache.end() && found->second.modified == modified && found->second.size == size) return found->second.document;
			Json document;
			std::ifstream input{ absolute };
			if (!input) throw std::runtime_error("Cannot open prefab");
			input >> document;
			if (!document.is_object() || document.value("format", std::string{}) != "vega.prefab" ||
				document.value("version", 0) != 1 || !document.contains("base") || !document["base"].is_object())
				throw std::runtime_error("Expected vega.prefab version 1 with base");
			mCache[absolute] = { modified, size, document };
			return document;
		}
		catch (const std::exception& error) { warnings.push_back("Prefab " + path + ": " + error.what()); return nullptr; }
	}
	PrefabResult PrefabLibrary::Resolve(const std::string& path)
	{
		LY_PROFILE_SCOPE("Prefab Resolve");
		const auto report = [](const PrefabResult& result)
		{
			for (const auto& warning : result.warnings)
				debug::Log::Write(debug::LogLevel::Warning, debug::LogChannel::Scene, LY_SOURCE_LOCATION, warning);
			return result;
		};
		std::set<std::filesystem::path> chain;
		try
		{
			const auto relative = std::filesystem::u8path(path);
			const auto key = std::filesystem::weakly_canonical(mRoot / relative);
			const auto local = key.lexically_relative(std::filesystem::weakly_canonical(mRoot));
			if (!relative.is_absolute() && !local.empty() && *local.begin() != ".." &&
				path.size() >= 12 && path.substr(path.size() - 12) == ".prefab.json")
			{
				const auto found = mResolved.find(key);
				if (found != mResolved.end())
				{
					bool current = true;
					for (const auto& dependency : found->second.dependencies)
					{
						std::error_code timeError, sizeError;
						const auto modified = std::filesystem::last_write_time(dependency.first, timeError);
						const auto size = std::filesystem::file_size(dependency.first, sizeError);
						if (timeError || sizeError || modified != dependency.second.first || size != dependency.second.second) current = false;
					}
					if (current) return report(found->second.result);
					mResolved.erase(found);
				}
				++mResolveCount;
				auto result = Resolve(path, chain);
				if (result)
				{
					Resolved cached{ result, {} };
					// Use the stamps of the documents actually loaded, so a concurrent
					// file change cannot mark an older realization as current.
					for (const auto& dependency : chain)
					{
						const auto& document = mCache.at(dependency);
						cached.dependencies[dependency] = { document.modified, document.size };
					}
					mResolved[key] = std::move(cached);
				}
				return report(result);
			}
		}
		catch (const std::filesystem::filesystem_error&) {}
		return report(Resolve(path, chain));
	}
	PrefabResult PrefabLibrary::Resolve(const std::string& path, std::set<std::filesystem::path>& chain)
	{
		PrefabResult result;
		try
		{
			const auto key = std::filesystem::weakly_canonical(mRoot / std::filesystem::u8path(path));
			if (chain.size() >= 64 || !chain.insert(key).second)
			{
				result.warnings.push_back("Prefab cycle or depth limit: " + path);
				return result;
			}
			const auto document = Load(path, result.warnings);
			if (document.is_null()) return result;
			const auto& base = document.at("base");
			if (base.contains("type") == base.contains("prefab")) throw std::runtime_error("Base must contain exactly one of type or prefab");
			if (base.contains("prefab"))
			{
				auto parent = Resolve(base.at("prefab").get<std::string>(), chain);
				result.warnings.insert(result.warnings.end(), parent.warnings.begin(), parent.warnings.end());
				if (!parent) return result;
				result.entity = std::move(parent.entity);
			}
			else
			{
				// Realize constructor defaults through the scene path, including game factories.
				World scratch{ nullptr };
				const auto loaded = SceneSerializer::Load(scratch, { { "format", "vega.scene" }, { "version", 1 },
					{ "settings", Json::object() }, { "entities", Json::array({ base }) } });
				result.warnings.insert(result.warnings.end(), loaded.warnings.begin(), loaded.warnings.end());
				if (loaded.spawned != 1) return result;
				result.entity = SceneSerializer::SaveEntity(*scratch.GetActorsByTypeIncludingPending<Actor>().front().lock(), true);
			}
			result.entity = Merge(std::move(result.entity), document);
			result.entity.erase("guid");
			result.entity.erase("name");
			result.entity["properties"].erase("Location");
			// Normalize setter side effects and canonical composition before diffing.
			World scratch{ nullptr };
			const auto loaded = SceneSerializer::Load(scratch, { { "format", "vega.scene" }, { "version", 1 },
				{ "settings", Json::object() }, { "entities", Json::array({ result.entity }) } });
			result.warnings.insert(result.warnings.end(), loaded.warnings.begin(), loaded.warnings.end());
			if (loaded.spawned != 1) { result.entity = nullptr; return result; }
			result.entity = SceneSerializer::SaveEntity(*scratch.GetActorsByTypeIncludingPending<Actor>().front().lock(), true);
			result.entity.erase("guid");
			result.entity.erase("name");
			result.entity["properties"].erase("Location");
		}
		catch (const std::exception& error) { result.entity = nullptr; result.warnings.push_back("Prefab " + path + ": " + error.what()); }
		return result;
	}

	Json PrefabLibrary::Merge(Json base, const Json& overrides)
	{
		if (overrides.contains("properties")) base["properties"].update(overrides.at("properties"));
		auto components = Entries(base, "components");
		auto removed = Entries(base, "removed");
		for (const auto& name : Entries(overrides, "removed"))
		{
			auto found = std::find_if(components.begin(), components.end(), [&](const Json& c) { return c.at("type") == name; });
			if (found != components.end())
			{
				if (!found->value("added", false)) removed.push_back(name);
				components.erase(found);
			}
		}
		std::map<std::string, std::size_t> occurrences;
		for (const auto& component : Entries(overrides, "components"))
		{
			const auto type = component.at("type").get<std::string>();
			const auto occurrence = occurrences[type]++;
			std::size_t matching = 0;
			auto found = std::find_if(components.begin(), components.end(), [&](const Json& c) { return c.at("type") == type && matching++ == occurrence; });
			if (found != components.end())
			{
				if (component.contains("properties")) (*found)["properties"].update(component["properties"]);
			}
			else
			{
				components.push_back(component);
				auto removal = std::find(removed.begin(), removed.end(), Json(type));
				if (!component.value("added", false) && removal != removed.end()) removed.erase(removal);
			}
		}
		base["components"] = std::move(components);
		if (removed.empty()) base.erase("removed"); else base["removed"] = std::move(removed);
		if (overrides.contains("scripts"))
		{
			// An explicit script list defines membership. Matching path occurrences inherit properties.
			auto scripts = Entries(overrides, "scripts");
			const auto inherited = Entries(base, "scripts");
			std::map<std::string, std::size_t> seen;
			for (auto& script : scripts)
			{
				const auto path = script.at("path").get<std::string>();
				const auto occurrence = seen[path]++;
				std::size_t matching = 0;
				for (const auto& previous : inherited)
					if (previous.at("path") == path && matching++ == occurrence)
					{
						auto properties = previous.value("properties", Json::object());
						properties.update(script.value("properties", Json::object()));
						for (auto it = properties.begin(); it != properties.end();)
							if (it->is_null()) it = properties.erase(it); else ++it;
						script["properties"] = std::move(properties);
						break;
					}
			}
			base["scripts"] = std::move(scripts);
		}
		for (const char* key : { "guid", "name" }) if (overrides.contains(key)) base[key] = overrides[key];
		return base;
	}
	Json PrefabLibrary::Diff(const Json& entity, const Json& baseline)
	{
		Json result = Json::object();
		const auto properties = PropertiesDiff(entity.value("properties", Json::object()), baseline.value("properties", Json::object()));
		if (!properties.empty()) result["properties"] = properties;
		const auto components = Entries(entity, "components");
		auto previous = Entries(baseline, "components");
		Json changes = Json::array(), removed = Json::array();
		// Removals consume the first surviving occurrence, exactly as scene loading
		// does. Compare properties against the survivors, not the pre-removal list.
		std::map<std::string, std::size_t> desired, inherited;
		for (const auto& c : components) ++desired[c.at("type").get<std::string>()];
		for (const auto& c : previous) ++inherited[c.at("type").get<std::string>()];
		for (auto it = previous.begin(); it != previous.end();)
		{
			const auto type = it->at("type").get<std::string>();
			if (inherited[type] > desired[type])
			{
				removed.push_back(type); --inherited[type]; it = previous.erase(it);
			}
			else ++it;
		}
		std::map<std::string, std::size_t> counts;
		for (const auto& component : components)
		{
			const auto type = component.at("type").get<std::string>();
			const auto occurrence = counts[type]++;
			std::size_t matching = 0;
			const Json* before = nullptr;
			for (const auto& candidate : previous) if (candidate.at("type") == type && matching++ == occurrence) { before = &candidate; break; }
			Json change{ { "type", type } };
			const auto props = PropertiesDiff(component.value("properties", Json::object()), before ? before->value("properties", Json::object()) : Json::object());
			if (!props.empty()) change["properties"] = props;
			if (!before) change["added"] = component.value("added", false);
			// Empty occurrences retain addressing when a later component of the same type differs.
			changes.push_back(change);
		}
		for (std::size_t i = changes.size(); i > 0; --i)
			if (changes[i - 1].size() == 1)
			{
				bool later = false;
				for (std::size_t j = i; j < changes.size(); ++j) if (changes[j]["type"] == changes[i - 1]["type"]) later = true;
				if (!later) changes.erase(changes.begin() + static_cast<Json::difference_type>(i - 1));
			}
		if (!changes.empty()) result["components"] = changes;
		if (!removed.empty()) result["removed"] = removed;
		if (Entries(entity, "scripts") != Entries(baseline, "scripts"))
		{
			auto scripts = Entries(entity, "scripts");
			std::map<std::string, std::size_t> occurrences;
			for (auto& script : scripts)
			{
				const auto path = script.at("path").get<std::string>();
				const auto occurrence = occurrences[path]++;
				std::size_t matching = 0;
				for (const auto& before : Entries(baseline, "scripts"))
					if (before.at("path") == path && matching++ == occurrence)
					{
						const auto authored = script.value("properties", Json::object());
						const auto inheritedProperties = before.value("properties", Json::object());
						script["properties"] = PropertiesDiff(authored, inheritedProperties);
						for (const auto& property : inheritedProperties.items())
							if (!authored.contains(property.key())) script["properties"][property.key()] = nullptr;
						break;
					}
			}
			result["scripts"] = scripts;
		}
		return result;
	}
	Json PrefabLibrary::Document(Json entity, const std::string& name)
	{
		entity["base"] = entity.contains("prefab") ? Json{ { "prefab", entity["prefab"] } } : Json{ { "type", entity.at("type") } };
		for (const char* key : { "guid", "name", "type", "prefab" }) entity.erase(key);
		if (entity.contains("properties")) entity["properties"].erase("Location");
		entity["format"] = "vega.prefab";
		entity["version"] = 1;
		entity["name"] = name;
		return entity;
	}
}
