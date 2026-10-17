#include "editor/EditorFunctionCommands.h"
#include <framework/scene/SceneEntityComponent.h>
#include <framework/debug/Log.h>
#include <map>

namespace vega
{
	bool CallReflectedFunction(EditorContext& context, const ReflectedTarget& target, unsigned int actorId,
		const std::string& functionName, std::string& error)
	{
		const auto* function = target.type ? target.type->FindFunction(functionName) : nullptr;
		if (!target.object || !function || !function->invoke) { error = "Unknown function: " + functionName; return false; }
		if (context.playMode && !function->callableInPlay)
		{
			error = "Function is not callable in play mode: " + function->name;
			return false;
		}
		try
		{
			function->invoke(target.object);
			return true;
		}
		catch (const std::exception& exception) { error = exception.what(); }
		catch (...) { error = "Function threw a non-standard exception"; }
		// Report exactly as a refused property write does, and keep the reason on the Console panel.
		context.propertyError = error;
		context.propertyErrorActor = actorId;
		context.propertyErrorComponent = target.componentIdentity;
		context.propertyErrorName = function->name;
		ly::debug::Log::Write(ly::debug::LogLevel::Error, ly::debug::LogChannel::Editor, LY_SOURCE_LOCATION,
			"CallInEditor " + function->name + " failed: " + error);
		return false;
	}

	std::vector<SelectionFunction> CommonSelectionFunctions(EditorContext& context)
	{
		std::vector<SelectionFunction> result;
		const auto actors = context.selection.GetAll();
		if (actors.size() < 2) return result;
		// Key each target by the reflected slot it occupies, so the same component on each actor pairs up.
		std::vector<std::map<std::string, ReflectedTarget>> maps;
		for (const auto& actor : actors)
		{
			std::map<std::string, ReflectedTarget> map;
			if (const auto root = ResolvePropertyTarget(*actor, {}); root.type) map["Actor:" + root.type->name] = root;
			actor->ForEachComponent([&](ly::ActorComponent& component)
			{
				if (dynamic_cast<ly::SceneEntityComponent*>(&component)) return;
				const auto* type = context.Engine().Types().FindMostDerived(component);
				if (!type) return;
				if (const auto identity = ly::IdentifyComponent(*actor, component))
					map["Component:" + identity->typeName + "#" + std::to_string(identity->occurrence)] = { &component, type, identity };
			});
			maps.push_back(std::move(map));
		}
		for (const auto& entry : maps.front())
		{
			std::vector<ReflectedTarget> targets;
			bool complete = true;
			for (const auto& map : maps)
			{
				const auto found = map.find(entry.first);
				if (found == map.end()) { complete = false; break; }
				targets.push_back(found->second);
			}
			if (!complete || !targets.front().type) continue;
			for (const auto* function : targets.front().type->AllFunctions())
			{
				// A function is common only when every paired target's type exposes the same name.
				bool shared = true;
				for (const auto& target : targets)
					if (!target.type || !target.type->FindFunction(function->name)) { shared = false; break; }
				if (!shared) continue;
				result.push_back(SelectionFunction{ targets.front().type->name, function->name, function->label,
					function->tooltip, function->callableInPlay, targets });
			}
		}
		return result;
	}

	bool InvokeSelectionFunction(EditorContext& context, const SelectionFunction& row, std::string& error)
	{
		const auto actors = context.selection.GetAll();
		if (actors.size() != row.targets.size()) { error = "Selection changed"; return false; }
		for (std::size_t i = 0; i < actors.size(); ++i)
			if (!CallReflectedFunction(context, row.targets[i], actors[i]->GetUniqueID(), row.name, error)) return false;
		context.SceneChanged();
		return true;
	}
} // namespace vega
