#include "editor/EditorCommands.h"
#include <framework/World.h>
#include <cmath>
#include <cstdlib>
#include <typeinfo>
#ifdef __GNUG__
#include <cxxabi.h>
#endif

namespace vega
{
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
		const auto actor = context.Resolve(mID);
		if (!actor) return false;
		const auto target = ResolvePropertyTarget(*actor, mComponentIndex);
		const auto* property = target.type ? target.type->FindProperty(mProperty) : nullptr;
		if (!property || !property->setter) return false;
		const auto before = property->getter(target.object);
		if (!property->SetValue(target.object, mAfter)) return false;
		if (!mExecuted) { mBefore = before; mExecuted = true; }
		return true;
	}

	void SetPropertyCommand::Undo(EditorContext& context)
	{
		const auto actor = context.Resolve(mID);
		if (!actor || !mExecuted) return;
		const auto target = ResolvePropertyTarget(*actor, mComponentIndex);
		if (const auto* property = target.type ? target.type->FindProperty(mProperty) : nullptr)
			property->SetValue(target.object, mBefore);
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
		for (const auto& weak : world->GetActorsByType<ly::Actor>())
		{
			if (auto actor = weak.lock(); actor && actor->GetUniqueID() == id && !actor->GetIsPendingDestroy())
			{
				return actor;
			}
		}
		return {};
	}

	CommandHistory::CommandHistory(EditorContext& context, std::size_t maxDepth)
		: mContext{ context }, mMaxDepth{ maxDepth } {}

	bool CommandHistory::Execute(std::unique_ptr<EditorCommand> command)
	{
		if (!command || !command->Execute(mContext)) return false;
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
		if (!CanUndo()) return false;
		mMergeAllowed = false;
		mUndo.back()->Undo(mContext);
		mRedo.push_back(std::move(mUndo.back()));
		mUndo.pop_back();
		return true;
	}

	bool CommandHistory::Redo()
	{
		if (!CanRedo() || !mRedo.back()->Execute(mContext)) return false;
		mMergeAllowed = false;
		mUndo.push_back(std::move(mRedo.back()));
		mRedo.pop_back();
		return true;
	}

	std::string CommandHistory::UndoName() const { return CanUndo() ? mUndo.back()->Name() : ""; }
	std::string CommandHistory::RedoName() const { return CanRedo() ? mRedo.back()->Name() : ""; }
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
		auto actor = context.Resolve(mID);
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
		if (auto actor = context.Resolve(mID); actor && mExecuted) Apply(*actor, mBefore);
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
		auto actor = context.Resolve(mID);
		if (!actor) return false;
		actor->Destroy();
		if (!context.selection.Get()) context.selection.Clear();
		return true;
	}
}
