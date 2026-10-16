#include "editor/EditorCommands.h"
#include <framework/scene/SceneSettings.h>
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <framework/scene/PrefabLibrary.h>
#include <script/ScriptComponent.h>
#include <cmath>
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
		camera->Focus(actor->GetActorGlobalBounds());
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
		if (!context.selection.Get()) context.selection.Clear();
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
		const bool selected = context.selection.Get() == actor;
		actor->Destroy();
		replacement->FindComponent<ly::SceneEntityComponent>()->guid = mGuid;
		context.ResetActorCache();
		if (selected) context.selection.Select(replacement);
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
		const bool selected = actor && context.selection.Get() == actor;
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
		if (selected) context.selection.Select(replacement);
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
		const auto actor = context.selection.Get();
		const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr;
		if (!identity || !context.world || actor->GetWorld() != context.world || context.playMode) return false;
		auto entity = ly::SceneSerializer::SaveEntity(*actor);
		if (entity.is_null()) return false;
		entity["guid"] = ly::SceneEntityComponent::GenerateGuid();
		const auto location = actor->GetActorLocation() + sf::Vector2f{ 32.f, 32.f };
		entity["properties"]["Location"] = { location.x, location.y };
		return history.Execute(std::make_unique<CreateEntity>(std::move(entity)));
	}

	bool RenameSceneEntity(EditorContext& context, CommandHistory& history, const std::string& name, std::uint64_t group)
	{
		const auto actor = context.selection.Get();
		if (!actor || context.playMode) return false;
		const auto target = FindPropertyTarget(*actor, "SceneEntityComponent");
		return target.object && history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIdentity, "Name", name, group));
	}
}
