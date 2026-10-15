#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <script/ScriptComponent.h>
#include <cmath>
#include <cstdlib>
#include <typeinfo>
#include <algorithm>
#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace vega
{
	SetScriptPropertyCommand::SetScriptPropertyCommand(unsigned int id, std::size_t index, std::string property,
		ly::reflect::PropertyValue value, std::uint64_t group)
		: mID{ id }, mComponentIndex{ index }, mProperty{ std::move(property) }, mAfter{ std::move(value) }, mMergeGroup{ group } {}

	bool SetScriptPropertyCommand::Execute(EditorContext& context)
	{
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor) return false;
		const auto target = ResolvePropertyTarget(*actor, mComponentIndex);
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
		const auto target = ResolvePropertyTarget(*actor, mComponentIndex);
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
			mComponentIndex != next->mComponentIndex || mProperty != next->mProperty) return false;
		mAfter = next->mAfter;
		return true;
	}

	ReflectedTarget ResolvePropertyTarget(ly::Actor& actor, std::optional<std::size_t> componentIndex)
	{
		auto& registry = ly::reflect::TypeRegistry::Get();
		if (!componentIndex) return { &actor, registry.FindMostDerived(actor), {} };
		ReflectedTarget result;
		std::size_t index = 0;
		actor.ForEachComponent([&](ly::ActorComponent& component)
		{
			if (index == *componentIndex) result = { &component, registry.FindMostDerived(component), index };
			++index;
		});
		return result;
	}

	ReflectedTarget FindPropertyTarget(ly::Actor& actor, const std::string& targetName)
	{
		auto result = ResolvePropertyTarget(actor, {});
		if (targetName == "Actor") return result;
		result = {};
		std::size_t index = 0;
		actor.ForEachComponent([&](ly::ActorComponent& component)
		{
			const auto* type = ly::reflect::TypeRegistry::Get().FindMostDerived(component);
			for (const auto* base = type; base; base = base->base)
			{
				if (!result.object && base->name == targetName) result = { &component, type, index };
			}
			++index;
		});
		return result;
	}

	SetPropertyCommand::SetPropertyCommand(unsigned int id, std::optional<std::size_t> componentIndex,
		std::string property, ly::reflect::PropertyValue after, std::uint64_t group)
		: mID{ id }, mComponentIndex{ componentIndex }, mProperty{ std::move(property) },
		mAfter{ std::move(after) }, mMergeGroup{ group }
	{
	}

	bool SetPropertyCommand::Execute(EditorContext& context)
	{
		context.propertyError.clear();
		context.propertyErrorActor = mID;
		context.propertyErrorComponent = mComponentIndex;
		context.propertyErrorName = mProperty;
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor) { context.propertyError = "Property target is unavailable"; return false; }
		const auto target = ResolvePropertyTarget(*actor, mComponentIndex);
		const auto* property = target.type ? target.type->FindProperty(mProperty) : nullptr;
		if (!property) { context.propertyError = "Unknown property"; return false; }
		const auto before = property->getter(target.object);
		if (!property->SetValue(target.object, mAfter, &context.propertyError)) return false;
		if (!mExecuted) { mBefore = before; mExecuted = true; }
		return true;
	}

	void SetPropertyCommand::Undo(EditorContext& context)
	{
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor || !mExecuted) return;
		const auto target = ResolvePropertyTarget(*actor, mComponentIndex);
		if (const auto* property = target.type ? target.type->FindProperty(mProperty) : nullptr)
		{
			context.propertyErrorActor = mID;
			context.propertyErrorComponent = mComponentIndex;
			context.propertyErrorName = mProperty;
			property->SetValue(target.object, mBefore, &context.propertyError);
		}
	}

	std::string SetPropertyCommand::Name() const
	{
		return "Set " + mProperty + " #" + std::to_string(mID) +
			(mComponentIndex ? " component " + std::to_string(*mComponentIndex) : "");
	}

	bool SetPropertyCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const SetPropertyCommand*>(&command);
		if (!next || !mMergeGroup || mMergeGroup != next->mMergeGroup || mID != next->mID ||
			mComponentIndex != next->mComponentIndex || mProperty != next->mProperty) return false;
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

	std::shared_ptr<ly::Actor> EditorContext::Resolve(unsigned int id) const
	{
		if (!world) return {};
		for (const auto& weak : world->GetActorsByTypeIncludingPending<ly::Actor>())
		{
			if (auto actor = weak.lock(); actor && actor->GetUniqueID() == id && !actor->GetIsPendingDestroy())
			{
				return actor;
			}
		}
		return {};
	}

	std::shared_ptr<ly::Actor> EditorContext::ResolveGuid(const std::string& guid) const
	{
		if (!world || guid.empty()) return {};
		for (const auto& weak : world->GetActorsByTypeIncludingPending<ly::Actor>())
		{
			const auto actor = weak.lock();
			const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr;
			if (identity && identity->guid == guid && !actor->GetIsPendingDestroy()) return actor;
		}
		return {};
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
		if (!command || mContext.transitioning || (mContext.playMode && command->IsUndoable()) || !command->Execute(mContext)) return false;
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
		mRedo.push_back(std::move(mUndo.back()));
		mUndo.pop_back();
		return true;
	}

	bool CommandHistory::Redo()
	{
		PruneInvalidCommands();
		if (mContext.playMode || mContext.transitioning || !CanRedo() || !mRedo.back()->Execute(mContext)) return false;
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

	ActorPropertyCommand::ActorPropertyCommand(unsigned int id, Property property, Value value, std::uint64_t group)
		: mID{ id }, mProperty{ property }, mAfter{ value }, mMergeGroup{ group } {}

	bool ActorPropertyCommand::Execute(EditorContext& context)
	{
		auto actor = context.Resolve(mID, mGuid);
		if (!actor) return false;
		if (mProperty == Property::Location)
		{
			const auto value = std::get<sf::Vector2f>(mAfter);
			if (!std::isfinite(value.x) || !std::isfinite(value.y)) return false;
		}
		if (mProperty == Property::Rotation && !std::isfinite(std::get<float>(mAfter))) return false;
		if (mProperty == Property::Layer && std::get<ly::RenderLayer>(mAfter) >= ly::RenderLayer::Count) return false;
		if (!mExecuted)
		{
			switch (mProperty)
			{
			case Property::Location: mBefore = actor->GetActorLocation(); break;
			case Property::Rotation: mBefore = actor->GetActorRotation(); break;
			case Property::Layer: mBefore = actor->GetRenderLayer(); break;
			case Property::Enabled: mBefore = actor->IsRenderEnabled(); break;
			}
			mActorName = ActorTypeName(*actor);
			mExecuted = true;
		}
		Apply(*actor, mAfter);
		return true;
	}

	void ActorPropertyCommand::Apply(ly::Actor& actor, const Value& value) const
	{
		switch (mProperty)
		{
		case Property::Location: actor.SetActorLocation(std::get<sf::Vector2f>(value)); break;
		case Property::Rotation: actor.SetActorRotation(std::get<float>(value)); break;
		case Property::Layer: actor.SetRenderLayer(std::get<ly::RenderLayer>(value)); break;
		case Property::Enabled: actor.SetRenderEnabled(std::get<bool>(value)); break;
		}
	}

	void ActorPropertyCommand::Undo(EditorContext& context)
	{
		// Gameplay may remove an actor while its edits remain in history; never resurrect an expired target.
		if (auto actor = context.Resolve(mID, mGuid); actor && mExecuted) Apply(*actor, mBefore);
	}

	std::string ActorPropertyCommand::Name() const
	{
		static const char* actions[]{ "Move", "Rotate", "Set render layer", "Set render enabled" };
		return std::string{ actions[static_cast<int>(mProperty)] } + " " + mActorName + " #" + std::to_string(mID);
	}

	bool ActorPropertyCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const ActorPropertyCommand*>(&command);
		if (!next || !mMergeGroup || mMergeGroup != next->mMergeGroup || mID != next->mID || mProperty != next->mProperty)
		{
			return false;
		}
		mAfter = next->mAfter;
		return true;
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
			const auto scene = ly::SceneSerializer::Save(*context.world);
			for (const auto& entity : scene["entities"])
				if (entity["guid"] == mGuid) { mEntity = entity; break; }
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

	bool CreateEntity::Execute(EditorContext& context)
	{
		if (!context.world || context.playMode) return false;
		if (ly::SceneSerializer::Load(*context.world, EntityScene(mEntity)).spawned != 1) return false;
		const auto guid = mEntity.at("guid").get<std::string>();
		// Constructors may choose random defaults (asteroid textures). Freeze the
		// realized serialized data so redo recreates exactly the authored entity.
		const auto scene = ly::SceneSerializer::Save(*context.world);
		for (const auto& entity : scene["entities"])
			if (entity["guid"] == guid) { mEntity = entity; break; }
		context.selection.Select(context.ResolveGuid(guid));
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
		const auto scene = ly::SceneSerializer::Save(*context.world);
		unsigned int number = 1;
		std::string name;
		do
		{
			name = type + " " + std::to_string(number++);
		} while (std::any_of(scene["entities"].begin(), scene["entities"].end(), [&](const auto& entity) { return entity["name"] == name; }));
		const auto centre = context.camera ? context.camera->GetView().getCenter() : context.world->GetWorldView().getCenter();
		return history.Execute(std::make_unique<CreateEntity>(nlohmann::json{
			{ "guid", ly::SceneEntityComponent::GenerateGuid() }, { "name", name }, { "type", type },
			{ "properties", { { "Location", { centre.x, centre.y } } } } }));
	}

	bool DuplicateSceneEntity(EditorContext& context, CommandHistory& history)
	{
		const auto actor = context.selection.Get();
		const auto* identity = actor ? actor->FindComponent<ly::SceneEntityComponent>() : nullptr;
		if (!identity || !context.world || context.playMode) return false;
		const auto scene = ly::SceneSerializer::Save(*context.world);
		for (auto entity : scene["entities"])
		{
			if (entity["guid"] != identity->guid) continue;
			entity["guid"] = ly::SceneEntityComponent::GenerateGuid();
			const auto location = actor->GetActorLocation() + sf::Vector2f{ 32.f, 32.f };
			entity["properties"]["Location"] = { location.x, location.y };
			return history.Execute(std::make_unique<CreateEntity>(std::move(entity)));
		}
		return false;
	}

	bool RenameSceneEntity(EditorContext& context, CommandHistory& history, const std::string& name, std::uint64_t group)
	{
		const auto actor = context.selection.Get();
		if (!actor || context.playMode) return false;
		const auto target = FindPropertyTarget(*actor, "SceneEntityComponent");
		return target.object && history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIndex, "Name", name, group));
	}
}
