#include "editor/EditorCommands.h"
#include "EditorCommandsInternal.h"
#include <framework/World.h>
#include <framework/scene/Scene.h>
#include <framework/scene/SceneEntityComponent.h>
#include <cmath>
#include <SFML/Window/Clipboard.hpp>
#include <algorithm>

namespace vega
{
	using editor_commands_internal::HasSelectedAncestor;
	using editor_commands_internal::SelectionSubtrees;

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

	bool DeleteSelection(EditorContext& context, CommandHistory& history)
	{
		auto command = std::make_unique<CompoundCommand>("Delete selection");
		for (const auto& actor : context.selection.GetAll())
		{
			if (HasSelectedAncestor(context.selection, *actor)) continue;
			if (!actor->FindComponent<ly::SceneEntityComponent>() || actor->GetWorld() != context.world) return false;
			command->Add(std::make_unique<DestroyActor>(actor->GetUniqueID()));
		}
		return history.Execute(std::move(command));
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
		auto entities = SelectionSubtrees(context);
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
			ly::SceneSerializer::RemapGuids(entities);
			for (auto& entity : entities)
			{
				
				auto& location = entity["properties"]["Location"];
				location = { location[0].get<float>() + centre.x - pivot.x, location[1].get<float>() + centre.y - pivot.y };
				command->Add(std::make_unique<CreateEntity>(std::move(entity)));
			}
			return history.Execute(std::move(command));
		}
		catch (const std::exception&) { return false; }
	}

}
