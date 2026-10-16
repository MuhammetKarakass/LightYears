#include "editor/EditorCommands.h"
#include <framework/scene/SceneSettings.h>
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <script/ScriptComponent.h>
#include <cmath>
#include <SFML/Window/Clipboard.hpp>
#include <cstdlib>
#include <typeinfo>
#include <algorithm>
#include <unordered_set>
#include <string_view>
#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace vega
{
	bool SetSceneSettingCommand::Execute(EditorContext& context)
	{
		if (!context.sceneSettings || context.playMode || context.transitioning) return false;
		context.settingErrorKey = mKey;
		auto& settings = *context.sceneSettings;
		std::optional<nlohmann::json> before;
		if (settings.contains(mKey)) before = settings.at(mKey);
		if (before == mAfter) return false;
		const auto* field = ly::SceneSettingsSchema::Get().Find(mKey);
		if (!before && mAfter && field && *mAfter == field->defaultValue) return false;
		if (!ly::SceneSettingsSchema::Get().Set(settings, mKey, mAfter, context.settingError)) return false;
		try
		{
			if (context.applySettings) context.applySettings(settings);
		}
		catch (const std::exception& error)
		{
			if (before) settings[mKey] = *before;
			else settings.erase(mKey);
			context.settingError = error.what();
			return false;
		}
		if (!mExecuted) { mBefore = std::move(before); mExecuted = true; }
		return true;
	}
	void SetSceneSettingCommand::Undo(EditorContext& context)
	{
		if (!context.sceneSettings || !mExecuted) return;
		// Restore raw authored data, including a previously invalid value.
		auto& settings = *context.sceneSettings;
		if (mBefore) settings[mKey] = *mBefore;
		else settings.erase(mKey);
		context.settingError.clear();
		if (context.applySettings) context.applySettings(settings);
	}
	bool SetSceneSettingCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const SetSceneSettingCommand*>(&command);
		if (!next || !mGroup || next->mGroup != mGroup || next->mKey != mKey) return false;
		mAfter = next->mAfter;
		return true;
	}

	SetScriptPropertyCommand::SetScriptPropertyCommand(unsigned int id, ly::ComponentIdentity index, std::string property,
		ly::reflect::PropertyValue value, std::uint64_t group)
		: mID{ id }, mComponentIdentity{ index }, mProperty{ std::move(property) }, mAfter{ std::move(value) }, mMergeGroup{ group } {}

	bool SetScriptPropertyCommand::Execute(EditorContext& context)
	{
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor) return false;
		const auto target = ResolvePropertyTarget(*actor, mComponentIdentity);
		auto* script = dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(target.object));
		if (!script) return false;
		std::optional<ly::reflect::PropertyValue> before;
		const auto entry = script->Overrides().find(mProperty);
		if (entry != script->Overrides().end()) before = entry->second;
		if (!script->SetProperty(mProperty, mAfter)) return false;
		if (!mExecuted) { mBeforeOverride = before; mExecuted = true; }
		return true;
	}
	void SetScriptPropertyCommand::Undo(EditorContext& context)
	{
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor || !mExecuted) return;
		const auto target = ResolvePropertyTarget(*actor, mComponentIdentity);
		if (auto* script = dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(target.object)))
		{
			if (mBeforeOverride) script->SetProperty(mProperty, *mBeforeOverride);
			else script->ClearOverride(mProperty);
		}
	}
	bool SetScriptPropertyCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const SetScriptPropertyCommand*>(&command);
		if (!next || !mMergeGroup || mMergeGroup != next->mMergeGroup || mID != next->mID ||
			mComponentIdentity != next->mComponentIdentity || mProperty != next->mProperty) return false;
		mAfter = next->mAfter;
		return true;
	}

	ReflectedTarget ResolvePropertyTarget(ly::Actor& actor, std::optional<ly::ComponentIdentity> componentIdentity)
	{
		auto& registry = ly::reflect::TypeRegistry::Get();
		if (!componentIdentity) return { &actor, registry.FindMostDerived(actor), {} };
		auto* component = ly::FindComponentByTypeOccurrence(actor, *componentIdentity);
		return component ? ReflectedTarget{ component, registry.FindMostDerived(*component), componentIdentity } : ReflectedTarget{};
	}

	ReflectedTarget FindPropertyTarget(ly::Actor& actor, const std::string& targetName)
	{
		auto result = ResolvePropertyTarget(actor, {});
		if (targetName == "Actor") return result;
		result = {};
		
		actor.ForEachComponent([&](ly::ActorComponent& component)
		{
			const auto* type = ly::reflect::TypeRegistry::Get().FindMostDerived(component);
			for (const auto* base = type; base; base = base->base)
			{
				if (!result.object && base->name == targetName) result = { &component, type, ly::IdentifyComponent(actor, component) };
			}

		});
		return result;
	}

	SetPropertyCommand::SetPropertyCommand(unsigned int id, std::optional<ly::ComponentIdentity> componentIdentity,
		std::string property, ly::reflect::PropertyValue after, std::uint64_t group)
		: mID{ id }, mComponentIdentity{ componentIdentity }, mProperty{ std::move(property) },
		mAfter{ std::move(after) }, mMergeGroup{ group }
	{
	}

	bool SetPropertyCommand::Execute(EditorContext& context)
	{
		context.propertyError.clear();
		context.propertyErrorActor = mID;
		context.propertyErrorComponent = mComponentIdentity;
		context.propertyErrorName = mProperty;
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor) { context.propertyError = "Property target is unavailable"; return false; }
		const auto target = ResolvePropertyTarget(*actor, mComponentIdentity);
		const auto* property = target.type ? target.type->FindProperty(mProperty) : nullptr;
		if (!property) { context.propertyError = "Unknown property"; return false; }
		const auto before = property->getter(target.object);
		if (!property->SetValue(target.object, mAfter, &context.propertyError)) return false;
		if (!mExecuted) { mBefore = before; mActorName = ActorTypeName(*actor); mExecuted = true; }
		return true;
	}

	void SetPropertyCommand::Undo(EditorContext& context)
	{
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor || !mExecuted) return;
		const auto target = ResolvePropertyTarget(*actor, mComponentIdentity);
		if (const auto* property = target.type ? target.type->FindProperty(mProperty) : nullptr)
		{
			context.propertyErrorActor = mID;
			context.propertyErrorComponent = mComponentIdentity;
			context.propertyErrorName = mProperty;
			property->SetValue(target.object, mBefore, &context.propertyError);
		}
	}

	std::string SetPropertyCommand::Name() const
	{
		if (!mComponentIdentity && (mProperty == "Location" || mProperty == "Rotation"))
			return std::string{ mProperty == "Location" ? "Move " : "Rotate " } + mActorName + " #" + std::to_string(mID);
		return "Set " + mProperty + " #" + std::to_string(mID) +
			(mComponentIdentity ? " component " + mComponentIdentity->typeName + "[" + std::to_string(mComponentIdentity->occurrence) + "]" : "");
	}

	bool SetPropertyCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const SetPropertyCommand*>(&command);
		if (!next || !mMergeGroup || mMergeGroup != next->mMergeGroup || mID != next->mID ||
			mComponentIdentity != next->mComponentIdentity || mProperty != next->mProperty) return false;
		mAfter = next->mAfter;
		return true;
	}

	void EditorContext::ApplyCamera()
	{
		if (world && camera && world->GetViewOverride()) world->SetViewOverride(camera->GetView());
	}

	bool EditorContext::SetEditorCamera(bool enabled)
	{
		if (!world || !camera) return false;
		if (world->IsEditMode()) enabled = true;
		if (enabled)
		{
			if (!world->GetViewOverride()) camera->InitFrom(world->GetWorldView());
			world->SetViewOverride(camera->GetView());
		}
		else world->SetViewOverride(std::nullopt);
		return true;
	}

	void EditorContext::SyncCamera()
	{
		if (!world || !camera) return;
		const bool paused = world->IsPaused();
		if (!cameraPaused || *cameraPaused != paused) SetEditorCamera(paused);
		cameraPaused = paused;
		ApplyCamera();
	}

	bool EditorContext::FocusSelection()
	{
		const auto actor = selection.Get();
		if (!actor || actor->GetWorld() != world || !SetEditorCamera(true)) return false;
		camera->Focus(*selection.Bounds());
		ApplyCamera();
		return true;
	}

	std::string ActorTypeName(const ly::Actor& actor)
	{
		std::string name = typeid(actor).name();
#ifdef __GNUG__
		int status = 0;
		std::unique_ptr<char, decltype(&std::free)> demangled{
			abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status), &std::free };
		if (status == 0 && demangled) name = demangled.get();
#endif
		// MSVC RTTI is demangled but includes the class/struct keyword.
		if (name.compare(0, 6, "class ") == 0) name.erase(0, 6);
		if (name.compare(0, 7, "struct ") == 0) name.erase(0, 7);
		const auto scope = name.rfind("::");
		if (scope != std::string::npos) name.erase(0, scope + 2);
		return name;
	}

	void EditorContext::RefreshActorCache() const
	{
		if (!world)
		{
			actorsByID.clear();
			actorsByGuid.clear();
			cachedWorldID.reset();
			return;
		}
		const auto count = world->GetActorCountIncludingPending();
		if (cachedWorldID == world->GetUniqueID() && cachedActorCount == count) return;
		actorsByID.clear();
		actorsByGuid.clear();
		cachedWorldID = world->GetUniqueID();
		cachedActorCount = count;
		for (const auto& weak : world->GetActorsByTypeIncludingPending<ly::Actor>())
		{
			if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy() && actor->GetWorld() == world)
			{
				actorsByID.emplace(actor->GetUniqueID(), actor);
				if (const auto* identity = actor->FindComponent<ly::SceneEntityComponent>())
					actorsByGuid.emplace(identity->guid, actor);
			}
		}
	}

	std::shared_ptr<ly::Actor> EditorContext::Resolve(unsigned int id) const
	{
		RefreshActorCache();
		auto entry = actorsByID.find(id);
		// A remove/spawn pair can leave the count unchanged between lookups.
		if (entry == actorsByID.end() && world)
		{
			ResetActorCache();
			RefreshActorCache();
			entry = actorsByID.find(id);
		}
		const auto actor = entry != actorsByID.end() ? entry->second.lock() : nullptr;
		return actor && actor->GetWorld() == world && !actor->GetIsPendingDestroy() ? actor : nullptr;
	}

	std::shared_ptr<ly::Actor> EditorContext::ResolveGuid(const std::string& guid) const
	{
		if (!world || guid.empty()) return {};
		RefreshActorCache();
		const auto find = [&]() -> std::shared_ptr<ly::Actor>
		{
			const auto entry = actorsByGuid.find(guid);
			const auto actor = entry != actorsByGuid.end() ? entry->second.lock() : nullptr;
			if (!actor || actor->GetWorld() != world || actor->GetIsPendingDestroy()) return {};
			const auto* identity = actor->FindComponent<ly::SceneEntityComponent>();
			return identity && identity->guid == guid ? actor : nullptr;
		};
		if (auto actor = find()) return actor;
		// Identity components can be attached after spawn, including before the next frame.
		ResetActorCache();
		RefreshActorCache();
		return find();
	}

	std::shared_ptr<ly::Actor> EditorContext::Resolve(unsigned int id, std::string& guid) const
	{
		// Once captured, GUID is authoritative: a missing entity must not hit an unrelated runtime ID.
		if (!guid.empty()) return ResolveGuid(guid);
		const auto actor = Resolve(id);
		if (const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr) guid = identity->guid;
		return actor;
	}

	CommandHistory::CommandHistory(EditorContext& context, std::size_t maxDepth)
		: mContext{ context }, mMaxDepth{ maxDepth } {}

	bool CommandHistory::Execute(std::unique_ptr<EditorCommand> command)
	{
		PruneInvalidCommands();
		if (!command || mContext.transitioning || (mContext.playMode && command->IsUndoable())) return false;
		if (!command->Execute(mContext))
		{
			ly::debug::Log::Write(ly::debug::LogLevel::Error, ly::debug::LogChannel::Editor, LY_SOURCE_LOCATION,
				command->Name() + ": " + (!mContext.propertyError.empty() ? mContext.propertyError : mContext.sceneError));
			return false;
		}
		if (command->ChangesScene()) mContext.SceneChanged();
		const bool hadRedo = CanRedo();
		mRedo.clear();
		if (!command->IsUndoable() || mMaxDepth == 0)
		{
			mMergeAllowed = false;
			return true;
		}
		// Never merge across an undo branch: its initial value belongs to a different edit.
		if (mMergeAllowed && !hadRedo && !mUndo.empty() && mUndo.back()->MergeWith(*command)) return true;
		mMergeAllowed = true;
		mUndo.push_back(std::move(command));
		if (mUndo.size() > mMaxDepth) mUndo.erase(mUndo.begin());
		return true;
	}

	bool CommandHistory::Undo()
	{
		PruneInvalidCommands();
		if (mContext.playMode || mContext.transitioning || !CanUndo()) return false;
		mMergeAllowed = false;
		mUndo.back()->Undo(mContext);
		if (mUndo.back()->ChangesScene()) mContext.SceneChanged();
		mRedo.push_back(std::move(mUndo.back()));
		mUndo.pop_back();
		return true;
	}

	bool CommandHistory::Redo()
	{
		PruneInvalidCommands();
		if (mContext.playMode || mContext.transitioning || !CanRedo() || !mRedo.back()->Execute(mContext)) return false;
		if (mRedo.back()->ChangesScene()) mContext.SceneChanged();
		mMergeAllowed = false;
		mUndo.push_back(std::move(mRedo.back()));
		mRedo.pop_back();
		return true;
	}

	std::string CommandHistory::UndoName() const { return CanUndo() ? mUndo.back()->Name() : ""; }
	std::string CommandHistory::RedoName() const { return CanRedo() ? mRedo.back()->Name() : ""; }
	void CommandHistory::PruneInvalidCommands()
	{
		// Reload/close invalidates JSON history without discarding unrelated actor edits.
		const auto prune = [&](auto& stack)
		{
			const auto size = stack.size();
			stack.erase(std::remove_if(stack.begin(), stack.end(), [&](const auto& command)
			{
				return !command->IsValid(mContext);
			}), stack.end());
			if (stack.size() != size) mMergeAllowed = false;
		};
		prune(mUndo);
		prune(mRedo);
	}

	void CommandHistory::Clear()
	{
		mUndo.clear();
		mRedo.clear();
		mMergeAllowed = false;
	}

	bool SelectActor::Execute(EditorContext& context)
	{
		if (!mID)
		{
			context.selection.Clear();
			return true;
		}
		auto actor = context.Resolve(*mID);
		if (!actor) return false;
		context.selection.Select(actor);
		return true;
	}

	bool SetPaused::Execute(EditorContext& context)
	{
		if (!context.world) return false;
		context.world->SetPaused(mPaused);
		context.SetEditorCamera(mPaused);
		context.cameraPaused = mPaused;
		return true;
	}

	bool DestroyActor::Execute(EditorContext& context)
	{
		if (context.playMode) return false;
		auto actor = context.Resolve(mID, mGuid);
		if (!actor) return false;
		if (!mGuid.empty())
		{
			mEntity = ly::SceneSerializer::SaveEntity(*actor);
			if (mEntity.is_null()) return false;
		}
		actor->Destroy();
		context.selection.Remove(actor);
		return true;
	}

	namespace
	{
		nlohmann::json EntityScene(const nlohmann::json& entity)
		{
			return { { "format", "vega.scene" }, { "version", 1 }, { "settings", nlohmann::json::object() },
				{ "entities", nlohmann::json::array({ entity }) } };
		}
	}

	void DestroyActor::Undo(EditorContext& context)
	{
		if (mEntity.is_null() || !context.world) return;
		if (ly::SceneSerializer::Load(*context.world, EntityScene(mEntity)).spawned == 1)
			context.selection.Select(context.ResolveGuid(mGuid));
	}

	bool ComponentCommand::Restore(EditorContext& context, const nlohmann::json& entity)
	{
		if (!context.world || context.playMode || context.transitioning) return false;
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor) return false;
		// Load with a temporary identity so a failed factory/property cannot destroy
		// the authored entity. Publish the original GUID only once loading succeeds.
		auto stagedEntity = entity;
		const auto temporaryGuid = ly::SceneEntityComponent::GenerateGuid();
		stagedEntity["guid"] = temporaryGuid;
		const auto result = ly::SceneSerializer::Load(*context.world, EntityScene(stagedEntity));
		context.ResetActorCache();
		const auto replacement = context.ResolveGuid(temporaryGuid);
		if (result.spawned != 1 || !result.warnings.empty() || !replacement)
		{
			if (replacement) replacement->Destroy();
			context.ResetActorCache();
			return false;
		}
		const bool selected = context.selection.Contains(actor);
		actor->Destroy();
		replacement->FindComponent<ly::SceneEntityComponent>()->guid = mGuid;
		context.ResetActorCache();
		if (selected) context.selection.Replace(actor, replacement);
		return true;
	}

	bool ComponentCommand::Execute(EditorContext& context)
	{
		if (!context.world || context.playMode || context.transitioning) return false;
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor || mGuid.empty()) return false;
		if (!mAfter.is_null()) return Restore(context, mAfter);
		const auto* info = ly::reflect::TypeRegistry::Get().Find(mComponent.typeName);
		if (!info || info->root != ly::reflect::TypeRoot::Component) return false;
		if (mAdding ? !info->addComponent : !info->removable) return false;
		mBefore = ly::SceneSerializer::SaveEntity(*actor);
		if (mBefore.is_null()) return false;
		// Stage the complete entity before replacing it. Factories and removal policy
		// run in isolation, and history retains only JSON and the stable GUID.
		ly::World scratch{ nullptr };
		const auto loaded = ly::SceneSerializer::Load(scratch, EntityScene(mBefore));
		if (loaded.spawned != 1 || !loaded.warnings.empty()) return false;
		const auto staged = scratch.GetActorsByTypeIncludingPending<ly::Actor>().front().lock();
		try
		{
			if (mAdding) info->addComponent(*staged);
			else
			{
				auto* component = ly::FindComponentByTypeOccurrence(*staged, mComponent);
				if (!component || !staged->RemoveComponent(*component)) return false;
			}
			mAfter = ly::SceneSerializer::SaveEntity(*staged);
		}
		catch (const std::exception&) { return false; }
		return !mAfter.is_null() && Restore(context, mAfter);
	}

	void ComponentCommand::Undo(EditorContext& context)
	{
		if (!mBefore.is_null()) Restore(context, mBefore);
	}

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
			!ly::PrefabLibrary::Get().Resolve(staged["prefab"].get<std::string>())))
		{
			staged.erase("_unresolved");
			eraseMissing();
			context.world->unresolvedPrefabs.push_back(std::move(staged));
			if (actor) actor->Destroy();
			context.ResetActorCache();
			return true;
		}
		// Internal undo snapshots contain the full type and values as well as the
		// prefab link. Load those values directly, then compare to today's baseline.
		const auto snapshotPrefab = staged.contains("type") ? staged.value("prefab", std::string{}) : std::string{};
		const auto savedBaseline = staged.value("_prefabBaseline", nlohmann::json::object());
		staged.erase("_prefabBaseline");
		if (!snapshotPrefab.empty()) staged.erase("prefab");
		const auto temporary = ly::SceneEntityComponent::GenerateGuid();
		staged["guid"] = temporary;
		const auto loaded = ly::SceneSerializer::Load(*context.world, EntityScene(staged));
		context.ResetActorCache();
		const auto replacement = context.ResolveGuid(temporary);
		if (loaded.spawned != 1 || !replacement) return false;
		const bool selected = actor && context.selection.Contains(actor);
		if (actor) actor->Destroy();
		eraseMissing();
		replacement->FindComponent<ly::SceneEntityComponent>()->guid = guid;
		if (!snapshotPrefab.empty())
		{
			auto* identity = replacement->FindComponent<ly::SceneEntityComponent>();
			identity->prefab = snapshotPrefab;
			const auto resolved = ly::PrefabLibrary::Get().Resolve(snapshotPrefab);
			identity->prefabBaseline = resolved ? resolved.entity : savedBaseline;
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

	bool CreateEntity::Execute(EditorContext& context)
	{
		if (!context.world || context.playMode) return false;
		if (ly::SceneSerializer::Load(*context.world, EntityScene(mEntity)).spawned != 1) return false;
		const auto guid = mEntity.at("guid").get<std::string>();
		// Constructors may choose random defaults (asteroid textures). Freeze the
		// realized serialized data so redo recreates exactly the authored entity.
		const auto actor = context.ResolveGuid(guid);
		if (!actor) return false;
		mEntity = ly::SceneSerializer::SaveEntity(*actor);
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
		const auto* info = ly::reflect::TypeRegistry::Get().Find(type);
		if (!info || !info->spawnable || !info->create || info->root != ly::reflect::TypeRoot::Actor) return false;
		std::unordered_set<std::string_view> names;
		for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
			if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy())
				if (const auto* identity = actor->FindComponent<ly::SceneEntityComponent>()) names.insert(identity->name);
		unsigned int number = 1;
		std::string name;
		do
		{
			name = type + " " + std::to_string(number++);
		} while (names.count(name));
		const auto centre = context.camera ? context.camera->GetView().getCenter() : context.world->GetWorldView().getCenter();
		return history.Execute(std::make_unique<CreateEntity>(nlohmann::json{
			{ "guid", ly::SceneEntityComponent::GenerateGuid() }, { "name", name }, { "type", type },
			{ "properties", { { "Location", { centre.x, centre.y } } } } }));
	}

	bool DuplicateSceneEntity(EditorContext& context, CommandHistory& history)
	{
		if (!context.world || context.playMode || context.transitioning) return false;
		auto command = std::make_unique<CompoundCommand>("Duplicate selection", 0, true);
		const auto actors = context.selection.GetAll();
		if (actors.empty()) return false;
		for (const auto& actor : actors)
		{
			if (actor->GetWorld() != context.world || !actor->FindComponent<ly::SceneEntityComponent>()) return false;
			auto entity = ly::SceneSerializer::SaveEntity(*actor);
			if (entity.is_null()) return false;
			entity["guid"] = ly::SceneEntityComponent::GenerateGuid();
			const auto location = actor->GetActorLocation() + sf::Vector2f{ 32.f, 32.f };
			entity["properties"]["Location"] = { location.x, location.y };
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
	bool CompoundCommand::Execute(EditorContext& context)
	{
		if (mCommands.empty() || context.playMode || context.transitioning) return false;
		const auto before = context.selection;
		EditorSelection created;
		std::size_t applied = 0;
		for (auto& command : mCommands)
		{
			if (!command->Execute(context))
			{
				const auto error = context.propertyError;
				while (applied > 0) mCommands[--applied]->Undo(context);
				context.selection.Restore(before);
				context.selection.Rebind([&](const std::string& guid) { return context.ResolveGuid(guid); });
				context.propertyError = error;
				return false;
			}
			++applied;
			if (mSelectCreated) created.Add(context.selection.Get());
		}
		if (!mExecuted)
		{
			mBefore = before;
			if (mSelectCreated) context.selection.Restore(created);
			mAfter = context.selection;
			mExecuted = true;
		}
		else context.selection.Restore(mAfter);
		context.selection.Rebind([&](const std::string& guid) { return context.ResolveGuid(guid); });
		return true;
	}

	void CompoundCommand::Undo(EditorContext& context)
	{
		for (auto it = mCommands.rbegin(); it != mCommands.rend(); ++it) (*it)->Undo(context);
		context.selection.Restore(mBefore);
		context.selection.Rebind([&](const std::string& guid) { return context.ResolveGuid(guid); });
	}

	bool CompoundCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const CompoundCommand*>(&command);
		if (!next || !mGroup || next->mGroup != mGroup || next->mName != mName || next->mCommands.size() != mCommands.size()) return false;
		for (std::size_t i = 0; i < mCommands.size(); ++i)
			if (mCommands[i]->Name() != next->mCommands[i]->Name() || typeid(*mCommands[i]) != typeid(*next->mCommands[i])) return false;
		for (std::size_t i = 0; i < mCommands.size(); ++i)
			if (!mCommands[i]->MergeWith(*next->mCommands[i])) return false;
		mAfter = next->mAfter;
		return true;
	}

	bool CompoundCommand::IsValid(const EditorContext& context) const
	{
		return std::all_of(mCommands.begin(), mCommands.end(), [&](const auto& command) { return command->IsValid(context); });
	}

	bool DeleteSelection(EditorContext& context, CommandHistory& history)
	{
		auto command = std::make_unique<CompoundCommand>("Delete selection");
		for (const auto& actor : context.selection.GetAll())
		{
			if (!actor->FindComponent<ly::SceneEntityComponent>() || actor->GetWorld() != context.world) return false;
			command->Add(std::make_unique<DestroyActor>(actor->GetUniqueID()));
		}
		return history.Execute(std::move(command));
	}

	std::vector<SelectionTransform> CaptureSelectionTransform(const EditorSelection& selection)
	{
		std::vector<SelectionTransform> result;
		for (const auto& actor : selection.GetAll()) result.push_back({ actor->GetUniqueID(), actor->GetActorLocation(), actor->GetActorRotation() });
		return result;
	}

	bool TransformSelection(EditorContext& context, CommandHistory& history, const std::vector<SelectionTransform>& starts,
		sf::Vector2f pivot, sf::Vector2f delta, std::optional<float> angle, std::uint64_t group)
	{
		auto command = std::make_unique<CompoundCommand>(angle ? "Rotate selection" : "Move selection", group);
		const float radians = angle.value_or(0.f) * 3.141592654f / 180.f;
		bool changed = false;
		for (const auto& start : starts)
		{
			const auto actor = context.Resolve(start.id);
			if (!actor || !context.selection.Contains(actor)) return false;
			const auto offset = start.location - pivot;
			const sf::Vector2f location = pivot + delta + sf::Vector2f{
				offset.x * std::cos(radians) - offset.y * std::sin(radians),
				offset.x * std::sin(radians) + offset.y * std::cos(radians) };
			changed = changed || location != actor->GetActorLocation() || (angle && start.rotation + *angle != actor->GetActorRotation());
			command->Add(std::make_unique<SetPropertyCommand>(start.id, std::nullopt, "Location", location, group));
			if (angle) command->Add(std::make_unique<SetPropertyCommand>(start.id, std::nullopt, "Rotation", start.rotation + *angle, group));
		}
		return changed && history.Execute(std::move(command));
	}

	void SelectRectangle(EditorContext& context, sf::Vector2f first, sf::Vector2f last, bool add, bool remove)
	{
		if (!context.world) return;
		const sf::Vector2f low{ std::min(first.x, last.x), std::min(first.y, last.y) };
		const sf::Vector2f high{ std::max(first.x, last.x), std::max(first.y, last.y) };
		const sf::FloatRect rectangle{ low, high - low };
		if (!add && !remove) context.selection.Clear();
		for (const auto& weak : context.world->GetActorsByTypeIncludingPending<ly::Actor>())
		{
			const auto actor = weak.lock();
			if (!actor || actor->GetIsPendingDestroy()) continue;
			if (rectangle.findIntersection(actor->GetActorGlobalBounds()) || rectangle.contains(actor->GetActorLocation()))
			{
				if (remove) context.selection.Remove(actor); else context.selection.Add(actor);
			}
		}
	}

	bool CopySelection(EditorContext& context)
	{
		nlohmann::json entities = nlohmann::json::array();
		for (const auto& actor : context.selection.GetAll())
		{
			if (actor->GetWorld() != context.world) return false;
			auto entity = ly::SceneSerializer::SaveEntity(*actor);
			if (entity.is_null()) return false;
			entities.push_back(std::move(entity));
		}
		if (entities.empty()) return false;
		const auto text = nlohmann::json{ { "format", "vega.selection" }, { "version", 1 }, { "entities", entities } }.dump();
		sf::Clipboard::setString(sf::String::fromUtf8(text.begin(), text.end()));
		return true;
	}

	bool PasteSelection(EditorContext& context, CommandHistory& history, sf::Vector2f centre)
	{
		if (!context.world || context.playMode || context.transitioning) return false;
		try
		{
			const auto bytes = sf::Clipboard::getString().toUtf8();
			const auto data = nlohmann::json::parse(bytes.begin(), bytes.end());
			if (data.at("format") != "vega.selection" || data.at("version") != 1) return false;
			auto entities = data.at("entities");
			if (!entities.is_array() || entities.empty() || entities.size() > 10000) return false;
			sf::Vector2f pivot{};
			for (const auto& entity : entities)
			{
				const auto& location = entity.at("properties").at("Location");
				const sf::Vector2f point{ location.at(0).get<float>(), location.at(1).get<float>() };
				if (!std::isfinite(point.x) || !std::isfinite(point.y)) return false;
				pivot += point;
			}
			pivot /= static_cast<float>(entities.size());
			auto command = std::make_unique<CompoundCommand>("Paste selection", 0, true);
			for (auto& entity : entities)
			{
				entity["guid"] = ly::SceneEntityComponent::GenerateGuid();
				auto& location = entity["properties"]["Location"];
				location = { location[0].get<float>() + centre.x - pivot.x, location[1].get<float>() + centre.y - pivot.y };
				command->Add(std::make_unique<CreateEntity>(std::move(entity)));
			}
			return history.Execute(std::move(command));
		}
		catch (const std::exception&) { return false; }
	}

	const std::vector<CommonProperty>& MultiInspectorCache::Get(EditorContext& context)
	{
		const auto world = context.world ? std::optional<unsigned int>{ context.world->GetUniqueID() } : std::nullopt;
		if (mWorld == world && mSceneRevision == context.sceneRevision && mSelectionRevision == context.selection.Revision()) return mProperties;
		mWorld = world;
		mSceneRevision = context.sceneRevision;
		mSelectionRevision = context.selection.Revision();
		mProperties.clear();
		const auto actors = context.selection.GetAll();
		if (actors.empty()) return mProperties;
		const auto collect = [&](std::vector<ReflectedTarget> targets)
		{
			const auto* common = targets.front().type;
			for (const auto& target : targets)
				while (common && (!target.object || !common->isInstance(target.object))) common = common->base;
			if (!common) return;
			for (const auto* property : common->AllProperties())
			{
				// A derived descriptor shadows a property of the same name on its base.
				if (common->FindProperty(property->name) != property) continue;
				CommonProperty row{ common->name, property, {}, property->getter(targets.front().object), false };
				for (const auto& target : targets)
				{
					row.targets.push_back(target.componentIdentity);
					row.mixed = row.mixed || property->getter(target.object) != row.value;
				}
				mProperties.push_back(std::move(row));
			}
		};
		std::vector<ReflectedTarget> roots;
		for (const auto& actor : actors) roots.push_back(ResolvePropertyTarget(*actor, {}));
		collect(std::move(roots));
		std::unordered_set<std::string> componentSets;
		actors.front()->ForEachComponent([&](ly::ActorComponent& component)
		{
			const auto* type = ly::reflect::TypeRegistry::Get().FindMostDerived(component);
			// Match occurrences within a reflected component chain. Unrelated sibling
			// components do not become common merely by inheriting ActorComponent.
			for (const auto* base = type; base && (base == type || base->name != "ActorComponent"); base = base->base)
			{
				std::size_t occurrence = 0;
				bool reached = false;
				actors.front()->ForEachComponent([&](ly::ActorComponent& entry)
				{
					if (&entry == &component) reached = true;
					if (!reached && base->isInstance(&entry)) ++occurrence;
				});
				std::vector<ReflectedTarget> targets;
				std::string key;
				for (const auto& actor : actors)
				{
					std::size_t index = 0;
					ReflectedTarget target;
					actor->ForEachComponent([&](ly::ActorComponent& entry)
					{
						if (base->isInstance(&entry) && index++ == occurrence)
							target = { &entry, base, ly::IdentifyComponent(*actor, entry) };
					});
					if (!target.object || !target.componentIdentity) break;
					key += target.componentIdentity->typeName + ":" + std::to_string(target.componentIdentity->occurrence) + ";";
					targets.push_back(std::move(target));
				}
				if (targets.size() == actors.size())
				{
					if (componentSets.insert(key).second) collect(std::move(targets));
					break;
				}
			}
		});
		return mProperties;
	}

	bool SetCommonProperty(EditorContext& context, CommandHistory& history, const CommonProperty& row,
		const ly::reflect::PropertyValue& value, std::uint64_t group)
	{
		const auto actors = context.selection.GetAll();
		if (!row.property || !row.property->setter || actors.size() != row.targets.size()) return false;
		auto command = std::make_unique<CompoundCommand>("Set selection " + row.section + "/" + row.property->name, group);
		for (std::size_t i = 0; i < actors.size(); ++i)
			command->Add(std::make_unique<SetPropertyCommand>(actors[i]->GetUniqueID(), row.targets[i], row.property->name, value, group));
		return history.Execute(std::move(command));
	}

}
