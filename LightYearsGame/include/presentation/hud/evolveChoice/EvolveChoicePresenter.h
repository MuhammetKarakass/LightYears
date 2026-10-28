#pragma once

#include "presentation/hud/evolveChoice/EvolveChoiceViewModel.h"
#include <memory>
#include <SFML/Window/Event.hpp>

namespace vega
{
	class EvolveChoicePresenter
	{
	public:
		EvolveChoicePresenter();
		void Tick(float deltaTime);
		bool HandleEvent(const sf::Event& event);
		bool Choose(std::size_t index);
		void Close();
		std::shared_ptr<const EvolveChoiceViewModel> GetViewModel() const { return mViewModel; }
	private:
		std::shared_ptr<EvolveChoiceViewModel> mViewModel;
		bool mRequestedOpen = false;
		bool mToggleKeyDown = false;
	};
}
