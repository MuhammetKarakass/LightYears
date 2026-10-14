#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace ly
{
	class Actor;
}

namespace vega
{
	class EditorSelection
	{
	public:
		void Select(std::weak_ptr<ly::Actor> actor);
		void Clear();
		std::shared_ptr<ly::Actor> Get() const;

	private:
		std::weak_ptr<ly::Actor> mActor;
	};

	using ActorPickBounds = std::pair<unsigned int, sf::FloatRect>;
	// Equal-area hits use the lowest ID so spatial-query traversal order cannot change selection.
	std::optional<unsigned int> PickBestActor(sf::Vector2f point, const std::vector<ActorPickBounds>& candidates);
}
