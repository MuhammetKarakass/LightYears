#include "editor/EditorSelection.h"

#include <framework/Actor.h>
#include <cmath>
#include <algorithm>
#include <framework/scene/SceneEntityComponent.h>
#include <limits>

namespace vega
{
	void EditorSelection::Select(std::weak_ptr<vega::Actor> actor)
	{
		const auto selected = actor.lock();
		if (mActors.size() == 1 && Get() == selected) return;
		Clear();
		Add(selected);
	}

	void EditorSelection::Clear()
	{
		if (mActors.empty()) return;
		mActors.clear();
		++mRevision;
	}

	void EditorSelection::Add(std::weak_ptr<vega::Actor> weak)
	{
		const auto actor = weak.lock();
		if (!actor || actor->GetIsPendingDestroy()) return;
		// Re-adding an existing row makes the last clicked actor primary.
		if (Get() == actor) return;
		Remove(actor);
		const auto* identity = actor->FindComponent<vega::SceneEntityComponent>();
		mActors.push_back({ actor, identity ? identity->guid : std::string{} });
		++mRevision;
	}

	void EditorSelection::Remove(const std::shared_ptr<vega::Actor>& actor)
	{
		if (!actor) return;
		const auto* identity = actor->FindComponent<vega::SceneEntityComponent>();
		const auto size = mActors.size();
		mActors.erase(std::remove_if(mActors.begin(), mActors.end(), [&](const Entry& entry)
		{
			return entry.actor.lock() == actor || (identity && !entry.guid.empty() && entry.guid == identity->guid);
		}), mActors.end());
		if (size != mActors.size()) ++mRevision;
	}

	void EditorSelection::Toggle(const std::shared_ptr<vega::Actor>& actor)
	{
		if (Contains(actor)) Remove(actor); else Add(actor);
	}

	bool EditorSelection::Contains(const std::shared_ptr<vega::Actor>& actor) const
	{
		return actor && !actor->GetIsPendingDestroy() && std::any_of(mActors.begin(), mActors.end(),
			[&](const Entry& entry) { return entry.actor.lock() == actor; });
	}

	std::shared_ptr<vega::Actor> EditorSelection::Get() const
	{
		RefreshValidity();
		for (auto it = mActors.rbegin(); it != mActors.rend(); ++it)
			if (auto actor = it->actor.lock(); actor && !actor->GetIsPendingDestroy()) return actor;
		return {};
	}

	std::vector<std::shared_ptr<vega::Actor>> EditorSelection::GetAll() const
	{
		RefreshValidity();
		std::vector<std::shared_ptr<vega::Actor>> actors;
		actors.reserve(mActors.size());
		for (const auto& entry : mActors)
			if (auto actor = entry.actor.lock(); actor && !actor->GetIsPendingDestroy()) actors.push_back(std::move(actor));
		return actors;
	}

	void EditorSelection::RefreshValidity() const
	{
		for (const auto& entry : mActors)
		{
			const auto actor = entry.actor.lock();
			const bool live = actor && !actor->GetIsPendingDestroy();
			if (entry.live != live) { entry.live = live; ++mRevision; }
		}
	}

	std::size_t EditorSelection::Count() const
	{
		RefreshValidity();
		return static_cast<std::size_t>(std::count_if(mActors.begin(), mActors.end(), [](const Entry& entry) { return entry.live; }));
	}

	void EditorSelection::Rebind(const std::function<std::shared_ptr<vega::Actor>(const std::string&)>& resolve)
	{
		for (auto& entry : mActors)
		{
			if (entry.guid.empty())
			{
				const auto actor = entry.actor.lock();
				if (const auto* identity = actor ? actor->FindComponent<vega::SceneEntityComponent>() : nullptr) entry.guid = identity->guid;
				else continue;
			}
			const auto actor = resolve(entry.guid);
			if (entry.actor.lock() != actor) { entry.actor = actor; ++mRevision; }
		}
	}

	void EditorSelection::Replace(const std::shared_ptr<vega::Actor>& before, const std::shared_ptr<vega::Actor>& after)
	{
		for (auto& entry : mActors) if (entry.actor.lock() == before)
		{
			entry.actor = after;
			++mRevision;
		}
	}

	std::optional<sf::FloatRect> EditorSelection::Bounds() const
	{
		std::optional<sf::FloatRect> result;
		for (const auto& actor : GetAll())
		{
			const auto bounds = actor->GetActorGlobalBounds();
			if (!result) { result = bounds; continue; }
			const sf::Vector2f low{ std::min(result->position.x, bounds.position.x), std::min(result->position.y, bounds.position.y) };
			const sf::Vector2f high{ std::max(result->position.x + result->size.x, bounds.position.x + bounds.size.x),
				std::max(result->position.y + result->size.y, bounds.position.y + bounds.size.y) };
			result = sf::FloatRect{ low, high - low };
		}
		return result;
	}

	sf::Vector2f EditorSelection::Centre() const
	{
		const auto actors = GetAll();
		sf::Vector2f centre{};
		for (const auto& actor : actors) centre += actor->GetActorLocation();
		return actors.empty() ? centre : centre / static_cast<float>(actors.size());
	}

	std::optional<unsigned int> PickBestActor(sf::Vector2f point, const std::vector<ActorPickBounds>& candidates)
	{
		std::optional<unsigned int> best;
		double bestArea = std::numeric_limits<double>::infinity();
		for (const auto& candidate : candidates)
		{
			const sf::FloatRect& bounds = candidate.second;
			const double area = std::abs(static_cast<double>(bounds.size.x) * bounds.size.y);
			if (bounds.contains(point) && std::isfinite(area) &&
				(area < bestArea || (area == bestArea && (!best || candidate.first < *best))))
			{
				best = candidate.first;
				bestArea = area;
			}
		}
		return best;
	}
}
