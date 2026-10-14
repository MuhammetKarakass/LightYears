#include "editor/EditorSelection.h"

#include <framework/Actor.h>
#include <cmath>
#include <limits>

namespace vega
{
	void EditorSelection::Select(std::weak_ptr<ly::Actor> actor)
	{
		mActor = std::move(actor);
	}

	void EditorSelection::Clear()
	{
		mActor.reset();
	}

	std::shared_ptr<ly::Actor> EditorSelection::Get() const
	{
		auto actor = mActor.lock();
		return actor && !actor->GetIsPendingDestroy() ? actor : nullptr;
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
