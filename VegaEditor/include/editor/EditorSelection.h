#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <memory>
#include <functional>
#include <string>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace vega
{
	class Actor;
}

namespace vega
{
	class EditorSelection
	{
	public:
		void Select(std::weak_ptr<vega::Actor> actor);
		void Clear();
		void Restore(const EditorSelection& saved) { mActors = saved.mActors; ++mRevision; }
		void Add(std::weak_ptr<vega::Actor> actor);
		void Remove(const std::shared_ptr<vega::Actor>& actor);
		void Toggle(const std::shared_ptr<vega::Actor>& actor);
		bool Contains(const std::shared_ptr<vega::Actor>& actor) const;
		std::vector<std::shared_ptr<vega::Actor>> GetAll() const;
		std::size_t Count() const;
		std::uint64_t Revision() const { RefreshValidity(); return mRevision; }
		std::optional<sf::FloatRect> Bounds() const;
		sf::Vector2f Centre() const;
		void Rebind(const std::function<std::shared_ptr<vega::Actor>(const std::string&)>& resolve);
		void Replace(const std::shared_ptr<vega::Actor>& before, const std::shared_ptr<vega::Actor>& after);
		std::shared_ptr<vega::Actor> Get() const;

	private:
		struct Entry
		{
			std::weak_ptr<vega::Actor> actor;
			std::string guid;
			mutable bool live = true;
		};
		std::vector<Entry> mActors;
		mutable std::uint64_t mRevision = 0;
		void RefreshValidity() const;
	};

	using ActorPickBounds = std::pair<unsigned int, sf::FloatRect>;
	// Equal-area hits use the lowest ID so spatial-query traversal order cannot change selection.
	std::optional<unsigned int> PickBestActor(sf::Vector2f point, const std::vector<ActorPickBounds>& candidates);
}
