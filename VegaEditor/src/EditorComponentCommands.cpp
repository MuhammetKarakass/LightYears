#include "editor/EditorCommands.h"
#include "EditorCommandsInternal.h"
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <algorithm>

namespace vega
{
	using editor_commands_internal::EntityScene;

	bool ComponentCommand::Restore(EditorContext& context, const nlohmann::json& entity)
	{
		return RestoreSceneEntity(context, entity);
	}

	bool ComponentCommand::Execute(EditorContext& context)
	{
		if (!context.world || context.playMode || context.transitioning) return false;
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor || mGuid.empty()) return false;
		if (!mAfter.is_null()) return Restore(context, mAfter);
		const auto* info = context.Engine().Types().Find(mComponent.typeName);
		if (!info || info->root != ly::reflect::TypeRoot::Component) return false;
		if (mAdding ? !info->addComponent : !info->removable) return false;
		mBefore = ly::SceneSerializer::SaveEntity(*actor);
		if (mBefore.is_null()) return false;
		// Stage the complete entity before replacing it. Factories and removal policy
		// run in isolation, and history retains only JSON and the stable GUID.
		ly::World scratch{ context.Engine() };
		auto stagedBefore = mBefore; stagedBefore.erase("parent");
		const auto loaded = ly::SceneSerializer::Load(scratch, EntityScene(stagedBefore));
		if (loaded.spawned < 1 || std::any_of(loaded.warnings.begin(), loaded.warnings.end(), [](const auto& warning)
			{ return warning.find("Dangling actor reference:") == std::string::npos; })) return false;
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
			if (mBefore.contains("parent")) mAfter["parent"] = mBefore["parent"];
		}
		catch (const std::exception&) { return false; }
		return !mAfter.is_null() && Restore(context, mAfter);
	}

	void ComponentCommand::Undo(EditorContext& context)
	{
		if (!mBefore.is_null()) Restore(context, mBefore);
	}

}
