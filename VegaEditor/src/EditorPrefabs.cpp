#include "editor/EditorPrefabs.h"
#include "editor/JsonDocument.h"
#include "editor/EditorApplication.h"
#include <framework/scene/Scene.h>
#include <framework/scene/PrefabLibrary.h>
#include <framework/scene/SceneEntityComponent.h>
#include <script/ScriptComponent.h>
#include <algorithm>
#include <imgui.h>

namespace vega
{
	using PrefabJson = nlohmann::json;
	namespace
	{
		PrefabJson Full(ly::Actor& actor)
		{
			auto full = ly::SceneSerializer::SaveEntity(actor, true);
			const auto* identity = actor.FindComponent<ly::SceneEntityComponent>();
			if (identity && !identity->prefab.empty() && identity->prefabRootGuid.empty())
			{
				full["prefab"] = identity->prefab;
				full["_prefabBaseline"] = identity->prefabBaseline;
			}
			return full;
		}
		PrefabJson* Section(PrefabJson& entity, ly::Actor& actor, const ReflectedTarget& target)
		{
			if (!target.componentIdentity) return &entity["properties"];
			if (auto* script = dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(target.object)))
			{
				std::size_t occurrence = 0, index = 0;
				actor.ForEachComponent([&](ly::ActorComponent& c)
				{
					if (auto* s = dynamic_cast<script::ScriptComponent*>(&c); s && s->ScriptPath() == script->ScriptPath())
					{
						if (&c == target.object) index = occurrence;
						++occurrence;
					}
				});
				occurrence = 0;
				if (entity.contains("scripts")) for (auto& s : entity["scripts"])
					if (s.at("path") == script->ScriptPath() && occurrence++ == index) return &s["properties"];
				return nullptr;
			}
			std::size_t occurrence = 0;
			if (entity.contains("components")) for (auto& c : entity["components"])
				if (c.at("type") == target.componentIdentity->typeName && occurrence++ == target.componentIdentity->occurrence) return &c["properties"];
			return nullptr;
		}
		PrefabJson* SectionOrAdd(PrefabJson& entity, PrefabJson& source, ly::Actor& actor, const ReflectedTarget& target)
		{
			if (auto* section = Section(entity, actor, target)) return section;
			if (!target.componentIdentity) return nullptr;
			const auto* sourceSection = Section(source, actor, target);
			if (!sourceSection) return nullptr;
			const auto* script = dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(target.object));
			const char* array = script ? "scripts" : "components";
			const char* key = script ? "path" : "type";
			const auto value = script ? script->ScriptPath() : target.componentIdentity->typeName;
			if (!entity.contains(array)) entity[array] = PrefabJson::array();
			std::size_t existing = 0, occurrence = 0;
			for (const auto& node : entity[array]) if (node.at(key) == value) ++existing;
			// Applying a property on an added component also applies its membership.
			// Copy preceding missing occurrences so its identity remains addressable.
			for (const auto& node : source[array]) if (node.at(key) == value)
			{
				if (occurrence++ >= existing) entity[array].push_back(node);
				if (node.contains("properties") && &node["properties"] == sourceSection) break;
			}
			return Section(entity, actor, target);
		}
		bool Refresh(EditorContext& context, CommandHistory& history, PrefabJson before, PrefabJson after, const std::string& name)
		{
			context.Engine().Prefabs().Reload();
			return after.empty() || history.Execute(std::make_unique<ReplaceEntities>(std::move(before), std::move(after), name));
		}
		void Snapshots(EditorContext& context, PrefabJson& before, PrefabJson& after)
		{
			before = PrefabJson::array(); after = PrefabJson::array();
			for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
				if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy())
					if (const auto* identity = actor->FindComponent<ly::SceneEntityComponent>(); identity && !identity->prefab.empty() && identity->prefabRootGuid.empty())
					{
						before.push_back(Full(*actor));
						after.push_back(ly::SceneSerializer::SaveEntity(*actor));
					}
			for (const auto& missing : context.world->unresolvedPrefabs)
				if (missing.is_object() && missing.contains("guid"))
				{
					auto snapshot = missing;
					snapshot["_unresolved"] = true;
					before.push_back(std::move(snapshot));
					after.push_back(missing);
				}
		}
		void PreserveNestedReferences(ly::EngineContext& engine, PrefabJson& realized, const PrefabJson& authored);
		bool ApplyResolved(EditorContext& context, CommandHistory& history, ly::Actor& actor, const PrefabJson& updated, const PrefabJson& sourceBaseline)
		{
			const auto* identity = actor.FindComponent<ly::SceneEntityComponent>();
			std::vector<std::string> warnings;
			const auto document = context.Engine().Prefabs().Load(identity->prefab, warnings);
			if (document.is_null()) return false;
			PrefabJson authored = updated;
			ly::PrefabLibrary::LocalizeReferences(authored);
			if (document["base"].contains("type")) PreserveNestedReferences(context.Engine(), authored, document);
			if (document["base"].contains("prefab"))
			{
				const auto parent = context.Engine().Prefabs().Resolve(document["base"]["prefab"].get<std::string>());
				if (!parent) return false;
				authored = ly::PrefabLibrary::Diff(authored, parent.entity);
			}
			for (const char* key : { "guid", "name", "type" }) authored.erase(key);
			if (authored.contains("properties")) authored["properties"].erase("Location");
			authored["format"] = "vega.prefab";
			authored["version"] = 1;
			authored["name"] = document.value("name", std::string{});
			authored["base"] = document["base"];
			const auto clean = [&](auto&& self, PrefabJson& node) -> void
			{
				node.erase("guid"); node.erase("parent");
				if (node.contains("children")) for (auto& child : node["children"]) self(self, child);
			};
			clean(clean, authored);
			PrefabJson before, after;
			Snapshots(context, before, after);
			const auto full = ly::SceneSerializer::SaveEntity(actor, true);
			if (!WritePrefab(context, identity->prefab, authored)) return false;
			for (auto& entity : after) if (entity["guid"] == identity->guid)
			{
				entity = ly::PrefabLibrary::Diff(full, sourceBaseline);
				entity["guid"] = identity->guid;
				entity["name"] = identity->name;
				entity["prefab"] = identity->prefab;
				entity["properties"]["Location"] = full["properties"]["Location"];
			}
			return Refresh(context, history, std::move(before), std::move(after), "Apply to Prefab");
		}
		PrefabJson* ChildNode(PrefabJson& root, const ly::Actor& actor)
		{
			std::vector<std::string> ids;
			for (auto* current = &actor; current; current = current->GetParent())
			{
				const auto* identity = current->FindComponent<ly::SceneEntityComponent>();
				if (!identity || identity->prefabRootGuid.empty()) break;
				ids.push_back(identity->childId);
			}
			auto* node = &root;
			for (auto it = ids.rbegin(); it != ids.rend(); ++it)
			{
				if (!node->contains("children")) return nullptr;
				auto& children = (*node)["children"];
				const auto found = std::find_if(children.begin(), children.end(), [&](const PrefabJson& child) { return child.at("id") == *it; });
				if (found == children.end()) return nullptr;
				node = &*found;
			}
			return node;
		}
		bool CopyCanonicalProperty(ly::EngineContext& engine, PrefabJson& destination, PrefabJson source, const ReflectedTarget& target, const std::string& property)
		{
			// A reflected setter can also change component state (e.g. Size -> sprite
			// Scale). Realize that setter so the alias does not undo the requested edit
			// when scene loading applies component properties after actor properties.
			ly::World currentWorld{ engine }, sourceWorld{ engine };
			auto current = destination;
			current.erase("parent"); source.erase("parent");
			const auto load = [](ly::World& world, const PrefabJson& node)
			{
				return ly::SceneSerializer::Load(world, { { "format", "vega.scene" }, { "version", 1 },
					{ "settings", PrefabJson::object() }, { "entities", PrefabJson::array({ node }) } }).spawned > 0;
			};
			if (!load(currentWorld, current) || !load(sourceWorld, source)) return false;
			const auto actor = currentWorld.GetActorsByTypeIncludingPending<ly::Actor>().front().lock();
			const auto sourceActor = sourceWorld.GetActorsByTypeIncludingPending<ly::Actor>().front().lock();
			const auto to = ResolvePropertyTarget(*actor, target.componentIdentity), from = ResolvePropertyTarget(*sourceActor, target.componentIdentity);
			if (!to.object || !from.object) return false;
			if (target.componentIdentity && dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(to.object)))
			{
				auto& script = *static_cast<script::ScriptComponent*>(to.object);
				const auto& properties = static_cast<script::ScriptComponent*>(from.object)->Properties();
				const auto found = properties.find(property);
				if (found == properties.end()) script.ClearOverride(property);
				else if (!script.SetProperty(property, found->second)) return false;
			}
			else
			{
				const auto* toProperty = to.type->FindProperty(property), *fromProperty = from.type->FindProperty(property);
				if (!toProperty || !fromProperty || !toProperty->SetValue(to.object, fromProperty->getter(from.object))) return false;
			}
			auto saved = ly::SceneSerializer::SaveSubtree(*actor, true);
			if (destination.contains("parent")) saved["parent"] = destination["parent"];
			destination = std::move(saved);
			return true;
		}
		void PreserveNestedReferences(ly::EngineContext& engine, PrefabJson& realized, const PrefabJson& authored)
		{
			if (!authored.contains("children") || !realized.contains("children")) return;
			for (const auto& original : authored["children"])
				for (auto& child : realized["children"])
					if (child.at("id") == original.at("id"))
					{
						PreserveNestedReferences(engine, child, original);
						if (!original.contains("prefab")) continue;
						const auto path = original.at("prefab").get<std::string>();
						const auto nested = engine.Prefabs().Resolve(path);
						if (!nested) continue;
						auto delta = ly::PrefabLibrary::Diff(child, nested.entity);
						for (const char* key : { "id", "guid", "name" }) if (child.contains(key)) delta[key] = child[key];
						delta["prefab"] = path;
						delta["properties"]["Location"] = child["properties"]["Location"];
						child = std::move(delta);
					}
		}
		bool ChangeChild(EditorContext& context, CommandHistory& history, ly::Actor& actor,
			const ReflectedTarget& target, const std::string& property, bool apply, bool whole = false)
		{
			const auto* identity = actor.FindComponent<ly::SceneEntityComponent>();
			const auto root = context.ResolveGuid(identity->prefabRootGuid);
			if (!root) return false;
			const auto* rootIdentity = root->FindComponent<ly::SceneEntityComponent>();
			auto full = ly::SceneSerializer::SaveEntity(*root, true), base = rootIdentity->prefabBaseline;
			auto* currentNode = ChildNode(full, actor);
			auto* baseNode = ChildNode(base, actor);
			if (!currentNode || !baseNode) return false;
			auto* current = whole ? currentNode : Section(*currentNode, actor, target);
			auto* baseline = whole ? baseNode : apply ? SectionOrAdd(*baseNode, *currentNode, actor, target) : Section(*baseNode, actor, target);
			if (!current) return false;
			if (!baseline)
				return !apply && target.componentIdentity && history.Execute(std::make_unique<RemoveComponentCommand>(actor.GetUniqueID(), *target.componentIdentity));
			if (!apply)
			{
				if (property.empty())
				{
					const auto guid = currentNode->at("guid"), name = currentNode->at("name");
					*current = *baseline;
					(*currentNode)["guid"] = guid; (*currentNode)["name"] = name;
					if (!whole)
					{
						const auto desired = *baseline;
						for (const auto& value : desired.items())
							if (!CopyCanonicalProperty(context.Engine(), *currentNode, *baseNode, target, value.key())) return false;
					}
				}
				else if (baseline->contains(property))
				{
					if (!CopyCanonicalProperty(context.Engine(), *currentNode, *baseNode, target, property)) return false;
				}
				else current->erase(property);
				auto after = ly::PrefabLibrary::Diff(full, rootIdentity->prefabBaseline);
				after["guid"] = rootIdentity->guid; after["name"] = rootIdentity->name; after["prefab"] = rootIdentity->prefab;
				after["properties"]["Location"] = full["properties"]["Location"];
				if (full.contains("parent")) after["parent"] = full["parent"];
				return history.Execute(std::make_unique<ReplaceEntities>(PrefabJson::array({ Full(*root) }), PrefabJson::array({ after }), "Revert prefab child"));
			}
			auto resolved = context.Engine().Prefabs().Resolve(rootIdentity->prefab);
			if (!resolved) return false;
			auto* destinationNode = ChildNode(resolved.entity, actor);
			auto* destination = whole ? destinationNode : destinationNode ? SectionOrAdd(*destinationNode, *currentNode, actor, target) : nullptr;
			if (!destination) return false;
			if (property.empty())
			{
				const auto desired = *current;
				*destination = desired; *baseline = desired;
				if (!whole) for (const auto& value : desired.items())
					if (!CopyCanonicalProperty(context.Engine(), *destinationNode, *currentNode, target, value.key()) || !CopyCanonicalProperty(context.Engine(), *baseNode, *currentNode, target, value.key())) return false;
			}
			else
			{
				if (!CopyCanonicalProperty(context.Engine(), *destinationNode, *currentNode, target, property) || !CopyCanonicalProperty(context.Engine(), *baseNode, *currentNode, target, property)) return false;
			}
			return ApplyResolved(context, history, *root, resolved.entity, base);
		}
	}
	bool IsPrefabPath(const std::filesystem::path& path)
	{
		const auto name = path.generic_u8string();
		return name.size() >= 12 && name.substr(name.size() - 12) == ".prefab.json";
	}
	std::vector<std::string> PrefabPaths(const std::filesystem::path& root)
	{
		std::vector<std::string> paths;
		std::error_code error;
		for (std::filesystem::recursive_directory_iterator it{ root / "prefabs", error }, end; !error && it != end; it.increment(error))
			if (it->is_regular_file(error) && IsPrefabPath(it->path())) paths.push_back(it->path().lexically_relative(root).generic_u8string());
		std::sort(paths.begin(), paths.end());
		return paths;
	}
	bool WritePrefab(EditorContext& context, const std::string& path, const PrefabJson& document, bool overwrite)
	{
		if (!context.document || context.playMode || !IsPrefabPath(path)) return false;
		std::filesystem::path absolute;
		const auto relative = std::filesystem::u8path(path).lexically_normal();
		if (relative.empty() || *relative.begin() != "prefabs" || !context.document->Resolve(relative, absolute)) return false;
		std::error_code error;
		if (!overwrite && std::filesystem::exists(absolute, error)) { context.sceneError = "Prefab already exists"; return false; }
		std::filesystem::create_directories(absolute.parent_path(), error);
		if (error) { context.sceneError = error.message(); return false; }
		if (!AtomicWriteText(absolute, document.dump(1, '\t') + "\n", context.sceneError)) return false;
		if (context.contentChanged) context.contentChanged();
		return true;
	}
	bool CreatePrefab(EditorContext& context, CommandHistory& history, const std::string& path)
	{
		const auto actor = context.selection.Get();
		if (!actor || !actor->FindComponent<ly::SceneEntityComponent>()) return false;
		const auto before = ly::SceneSerializer::SaveSubtree(*actor);
		const auto document = ly::PrefabLibrary::Document(before, std::filesystem::path(path).stem().stem().u8string());
		if (!WritePrefab(context, path, document, false)) return false;
		context.Engine().Prefabs().Reload();
		PrefabJson after{ { "prefab", path }, { "guid", before["guid"] }, { "name", before["name"] },
			{ "properties", { { "Location", before["properties"]["Location"] } } } };
		const auto identities = [&](auto&& self, const PrefabJson& source) -> PrefabJson
		{
			PrefabJson node;
			for (const char* key : { "id", "guid", "name" }) if (source.contains(key)) node[key] = source[key];
			if (source.contains("children")) for (const auto& child : source["children"]) node["children"].push_back(self(self, child));
			return node;
		};
		if (before.contains("children")) for (const auto& child : before["children"]) after["children"].push_back(identities(identities, child));
		if (before.contains("parent")) after["parent"] = before["parent"];
		return history.Execute(std::make_unique<ReplaceEntities>(PrefabJson::array({ before }), PrefabJson::array({ after }), "Create Prefab"));
	}
	bool CreateVariant(EditorContext& context, const std::string& parent, const std::string& path)
	{
		if (!context.Engine().Prefabs().Resolve(parent)) return false;
		return WritePrefab(context, path, { { "format", "vega.prefab" }, { "version", 1 },
			{ "name", std::filesystem::path(path).stem().stem().u8string() }, { "base", { { "prefab", parent } } } }, false);
	}
	bool PlacePrefab(EditorContext& context, CommandHistory& history, const std::string& path, sf::Vector2f location)
	{
		if (!context.Engine().Prefabs().Resolve(path)) return false;
		return history.Execute(std::make_unique<CreateEntity>(PrefabJson{ { "prefab", path }, { "guid", ly::SceneEntityComponent::GenerateGuid() },
			{ "name", std::filesystem::path(path).stem().stem().u8string() }, { "properties", { { "Location", { location.x, location.y } } } } }));
	}
	bool RefreshPrefabs(EditorContext& context, CommandHistory& history)
	{
		if (!context.world || context.playMode) return false;
		PrefabJson before, after;
		Snapshots(context, before, after);
		return Refresh(context, history, std::move(before), std::move(after), "Reload Prefabs");
	}
	bool PrefabPropertyOverridden(EditorContext& context, ly::Actor& actor, const ReflectedTarget& target, const std::string& property)
	{
		if (!target.object || (property == "Location" && actor.FindComponent<ly::SceneEntityComponent>() && actor.FindComponent<ly::SceneEntityComponent>()->prefabRootGuid.empty())) return false;
		const auto* identity = actor.FindComponent<ly::SceneEntityComponent>();
		if (!identity || identity->prefab.empty()) return false;
		if (context.prefabOverrideActor != actor.GetUniqueID() || context.prefabOverrideRevision != context.sceneRevision)
		{
			// Commands, gestures and prefab replacements all advance the scene revision.
			// Serialize once per edit, then drawing each property only queries its set.
			auto full = ly::SceneSerializer::SaveEntity(actor, true);
			auto base = identity->prefabBaseline;
			context.prefabOverrides.clear();
			const auto collect = [&](const ReflectedTarget& section)
			{
				const auto* current = Section(full, actor, section);
				const auto* baseline = Section(base, actor, section);
				if (current) for (const auto& value : current->items())
					if ((value.key() != "Location" || !identity->prefabRootGuid.empty()) && (!baseline || !baseline->contains(value.key()) || baseline->at(value.key()) != value.value()))
						context.prefabOverrides[section.object].insert(value.key());
			};
			collect(ResolvePropertyTarget(actor, {}));
			actor.ForEachComponent([&](ly::ActorComponent& component)
			{
				const auto componentIdentity = ly::IdentifyComponent(actor, component);
				if (componentIdentity) collect({ &component, nullptr, componentIdentity });
			});
			context.prefabOverrideActor = actor.GetUniqueID();
			context.prefabOverrideRevision = context.sceneRevision;
		}
		const auto section = context.prefabOverrides.find(target.object);
		return section != context.prefabOverrides.end() && section->second.count(property) != 0;
	}
	bool ChangePrefabProperty(EditorContext& context, CommandHistory& history, const ReflectedTarget& target, const std::string& property, bool apply)
	{
		const auto actor = context.selection.Get();
		const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr;
		if (!identity || identity->prefab.empty() || context.playMode) return false;
		if (!identity->prefabRootGuid.empty()) return ChangeChild(context, history, *actor, target, property, apply);
		if (property == "Location") return false;
		auto full = ly::SceneSerializer::SaveEntity(*actor, true), base = identity->prefabBaseline;
		auto* current = Section(full, *actor, target);
		auto* baseline = apply ? SectionOrAdd(base, full, *actor, target) : Section(base, *actor, target);
		if (!current || !current->contains(property)) return false;
		if (!baseline)
			return !apply && target.componentIdentity && history.Execute(std::make_unique<RemoveComponentCommand>(actor->GetUniqueID(), *target.componentIdentity));
		if (!apply)
		{
			if (baseline->contains(property)) (*current)[property] = baseline->at(property); else current->erase(property);
			auto delta = ly::PrefabLibrary::Diff(full, identity->prefabBaseline);
			delta["prefab"] = identity->prefab; delta["guid"] = identity->guid; delta["name"] = identity->name;
			delta["properties"]["Location"] = full["properties"]["Location"];
			return history.Execute(std::make_unique<ReplaceEntities>(PrefabJson::array({ Full(*actor) }), PrefabJson::array({ delta }), "Revert to Prefab"));
		}
		// Resolve again before writing so applying one property cannot overwrite
		// another change made to the asset since this instance was loaded.
		auto resolved = context.Engine().Prefabs().Resolve(identity->prefab);
		if (!resolved) return false;
		auto* destination = SectionOrAdd(resolved.entity, full, *actor, target);
		if (!destination) return false;
		(*destination)[property] = current->at(property);
		(*baseline)[property] = current->at(property);
		return ApplyResolved(context, history, *actor, resolved.entity, base);
	}
	bool ChangePrefabSection(EditorContext& context, CommandHistory& history, const ReflectedTarget& target, bool apply)
	{
		const auto actor = context.selection.Get();
		const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr;
		if (!identity || identity->prefab.empty() || context.playMode) return false;
		if (!identity->prefabRootGuid.empty()) return ChangeChild(context, history, *actor, target, {}, apply);
		auto full = ly::SceneSerializer::SaveEntity(*actor, true), base = identity->prefabBaseline;
		auto* current = Section(full, *actor, target);
		auto* baseline = apply ? SectionOrAdd(base, full, *actor, target) : Section(base, *actor, target);
		if (!current) return false;
		if (!baseline)
			return !apply && target.componentIdentity && history.Execute(std::make_unique<RemoveComponentCommand>(actor->GetUniqueID(), *target.componentIdentity));
		if (apply)
		{
			auto resolved = context.Engine().Prefabs().Resolve(identity->prefab);
			if (!resolved) return false;
			auto* destination = SectionOrAdd(resolved.entity, full, *actor, target);
			if (!destination) return false;
			*destination = *current;
			*baseline = *current;
			if (!target.componentIdentity)
			{
				destination->erase("Location");
				baseline->erase("Location");
			}
			return ApplyResolved(context, history, *actor, resolved.entity, base);
		}
		const auto location = full["properties"]["Location"];
		*current = *baseline;
		full["properties"]["Location"] = location;
		auto after = ly::PrefabLibrary::Diff(full, identity->prefabBaseline);
		after["guid"] = identity->guid;
		after["name"] = identity->name;
		after["prefab"] = identity->prefab;
		after["properties"]["Location"] = location;
		return history.Execute(std::make_unique<ReplaceEntities>(PrefabJson::array({ Full(*actor) }), PrefabJson::array({ after }), "Revert section to Prefab"));
	}
	bool ChangeAllPrefab(EditorContext& context, CommandHistory& history, bool apply)
	{
		const auto actor = context.selection.Get();
		const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr;
		if (!identity || identity->prefab.empty()) return false;
		if (!identity->prefabRootGuid.empty()) return ChangeChild(context, history, *actor, ResolvePropertyTarget(*actor, {}), {}, apply, true);
		if (apply)
		{
			const auto full = ly::SceneSerializer::SaveEntity(*actor, true);
			return ApplyResolved(context, history, *actor, full, full);
		}
		PrefabJson after{ { "guid", identity->guid }, { "name", identity->name }, { "prefab", identity->prefab },
			{ "properties", { { "Location", { actor->GetActorLocation().x, actor->GetActorLocation().y } } } } };
		return history.Execute(std::make_unique<ReplaceEntities>(PrefabJson::array({ Full(*actor) }), PrefabJson::array({ after }), "Revert all to Prefab"));
	}

	bool EditorApplication::PrefabDirty() const
	{
		const auto actor = mPrefabContext.ResolveGuid(mPrefabRootGuid);
		if (!actor) return !mPrefabRootGuid.empty();
		if (mPrefabDirtyRevision != mPrefabContext.sceneRevision)
		{
			mPrefabDirty = ly::SceneSerializer::SaveSubtree(*actor, true) != mPrefabSaved;
			mPrefabDirtyRevision = mPrefabContext.sceneRevision;
		}
		return mPrefabDirty;
	}
	void EditorApplication::ClosePrefab()
	{
		mPrefabHistory.Clear();
		mPrefabSelection.Clear();
		mPrefabContext.world = nullptr;
		mPrefabContext.ResetActorCache();
		mPrefabWorld.reset();
		mPrefabPath.clear();
		mPrefabRootGuid.clear();
		mPrefabContext.attachmentRoot.clear();
	}
	bool EditorApplication::OpenPrefab(const std::filesystem::path& path)
	{
		if (mContext.playMode) return false;
		auto result = GetEngineContext().Prefabs().Resolve(path.generic_u8string());
		if (!result) { mContext.sceneError = result.warnings.empty() ? "Cannot resolve prefab" : result.warnings.front(); return false; }
		std::vector<std::string> warnings;
		auto document = GetEngineContext().Prefabs().Load(path.generic_u8string(), warnings);
		if (document.is_null()) return false;
		auto world = std::make_unique<ly::World>(GetEngineContext());
		result.entity["name"] = document.value("name", std::string{});
		const auto loaded = ly::SceneSerializer::Load(*world, { { "format", "vega.scene" }, { "version", 1 },
			{ "settings", PrefabJson::object() }, { "entities", PrefabJson::array({ result.entity }) } });
		if (loaded.spawned < 1) return false;
		ClosePrefab();
		mPrefabWorld = std::move(world);
		mPrefabContext.world = mPrefabWorld.get();
		mPrefabContext.document = &mDocument;
		mPrefabSelection.Select(mPrefabWorld->GetActorsByTypeIncludingPending<ly::Actor>().front().lock());
		mPrefabRootGuid = mPrefabSelection.Get()->FindComponent<ly::SceneEntityComponent>()->guid;
		mPrefabContext.attachmentRoot = mPrefabRootGuid;
		mPrefabSaved = ly::SceneSerializer::SaveSubtree(*mPrefabSelection.Get(), true);
		mPrefabDirty = false;
		mPrefabDirtyRevision = mPrefabContext.sceneRevision;
		mPrefabDocument = std::move(document);
		mPrefabPath = path;
		return true;
	}
	void EditorApplication::RequestPrefab(const std::filesystem::path& path)
	{
		if (mContext.playMode) return;
		if (PrefabDirty()) { mPendingPrefab = path; mPrefabCloseRequested = path.empty(); mConfirmPrefab = true; }
		else if (path.empty()) ClosePrefab();
		else OpenPrefab(path);
	}
	nlohmann::json EditorApplication::CapturePrefab() const
	{
		const auto actor = mPrefabContext.ResolveGuid(mPrefabRootGuid);
		if (!actor) return nullptr;
		const auto full = ly::SceneSerializer::SaveSubtree(*actor, true);
		auto authored = full;
		PreserveNestedReferences(GetEngineContext(), authored, mPrefabDocument);
		auto document = ly::PrefabLibrary::Document(authored, full.value("name", std::string{}));
		if (mPrefabDocument["base"].contains("prefab"))
		{
			const auto parent = GetEngineContext().Prefabs().Resolve(mPrefabDocument["base"]["prefab"].get<std::string>());
			if (!parent) return nullptr;
			auto localized = full;
			ly::PrefabLibrary::LocalizeReferences(localized);
			document = ly::PrefabLibrary::Diff(localized, parent.entity);
			if (document.contains("properties")) document["properties"].erase("Location");
			document["format"] = "vega.prefab"; document["version"] = 1; document["name"] = full.value("name", std::string{});
		}
		document["base"] = mPrefabDocument["base"];
		return document;
	}
	bool EditorApplication::SavePrefab()
	{
		const auto actor = mPrefabContext.ResolveGuid(mPrefabRootGuid);
		if (!actor) return false;
		auto document = CapturePrefab();
		if (document.is_null()) return false;
		if (!WritePrefab(mContext, mPrefabPath.generic_u8string(), document)) return false;
		mPrefabDocument = std::move(document);
		mPrefabSaved = ly::SceneSerializer::SaveSubtree(*actor, true);
		mPrefabDirty = false;
		mPrefabDirtyRevision = mPrefabContext.sceneRevision;
		return RefreshPrefabs(mContext, mHistory);
	}
	void EditorApplication::BuildPrefabDialogs()
	{
		if (mCreatePrefabDialog) { ImGui::OpenPopup("Create Prefab"); mCreatePrefabDialog = false; }
		if (ImGui::BeginPopupModal("Create Prefab", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted(mVariantParent.empty() ? "Prefab name" : "Variant name");
			ImGui::InputText("Name", mPrefabName, sizeof(mPrefabName));
			if (ImGui::Button("Create"))
			{
				const std::string name = mPrefabName;
				if (!name.empty() && name.find_first_of("<>:\"/\\|?*.") == std::string::npos)
				{
					const auto path = "prefabs/" + name + ".prefab.json";
					const bool success = mVariantParent.empty() ? CreatePrefab(mContext, mHistory, path) : CreateVariant(mContext, mVariantParent, path);
					if (success) ImGui::CloseCurrentPopup();
				}
				else mContext.sceneError = "Enter a valid file name";
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
			if (!mContext.sceneError.empty()) ImGui::TextWrapped("%s", mContext.sceneError.c_str());
			ImGui::EndPopup();
		}
		if (mConfirmPrefab) { ImGui::OpenPopup("Unsaved prefab changes"); mConfirmPrefab = false; }
		if (ImGui::BeginPopupModal("Unsaved prefab changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("Save prefab changes before continuing?");
			const bool save = ImGui::Button("Save"); ImGui::SameLine();
			const bool discard = ImGui::Button("Discard"); ImGui::SameLine();
			if ((save && SavePrefab()) || discard)
			{
				ClosePrefab();
				if (!mPendingPrefab.empty()) OpenPrefab(mPendingPrefab);
				mPendingPrefab.clear(); mPrefabCloseRequested = false;
				ImGui::CloseCurrentPopup();
				if (!mPrefabPendingSceneAction.empty())
				{
					const auto action = mPrefabPendingSceneAction; const auto path = mPrefabPendingScenePath;
					mPrefabPendingSceneAction.clear(); RequestScene(action, path);
				}
			}
			if (ImGui::Button("Cancel"))
			{
				mPendingPrefab.clear(); mPrefabCloseRequested = false; mPrefabPendingSceneAction.clear(); ImGui::CloseCurrentPopup();
			}
			if (!mContext.sceneError.empty()) ImGui::TextWrapped("%s", mContext.sceneError.c_str());
			ImGui::EndPopup();
		}
	}
	void EditorApplication::PrefabPropertyMenu(ly::Actor& actor, const ReflectedTarget& target, const std::string& property)
	{
		if (mPrefabWorld || mContext.playMode) return;
		const auto* identity = actor.FindComponent<ly::SceneEntityComponent>();
		if (!identity || identity->prefab.empty()) return;
		if (property == "Location" && identity->prefabRootGuid.empty()) return;
		// Composite/disabled widgets may leave a last item without an ImGui ID.
		const auto popup = "Prefab property##" + property;
		if (ImGui::BeginPopupContextItem(popup.c_str()))
		{
			if (ImGui::MenuItem("Revert to Prefab")) ChangePrefabProperty(mContext, mHistory, target, property, false);
			if (ImGui::MenuItem("Apply to Prefab")) ChangePrefabProperty(mContext, mHistory, target, property, true);
			ImGui::EndPopup();
		}
	}
}
