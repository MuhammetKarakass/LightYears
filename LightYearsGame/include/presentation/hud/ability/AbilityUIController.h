#pragma once

#include "framework/Core.h"
#include "presentation/hud/HUDController.h"
#include "presentation/hud/ability/AbilityUIViewModel.h"
#include <SFML/Graphics.hpp>

namespace ly
{
	class GameHUD;
	class ImageWidget;
	class TextWidget;

	class AbilityUIController : public HUDController
	{
	public:
		AbilityUIController(weak_ptr<GameHUD> gameHUD);

		virtual void Tick(float deltaTime) override;

	private:
		void EnsureWidgetsCreated();
		void ResetWidgets();
		void RefreshFromAbilitySystem();
		void UpdateWidgetVisuals(AbilitySlot slot);
		void PositionWidgets();

		weak_ptr<GameHUD> mGameHUD;
		AbilityUIViewModel mViewModel;

		struct SlotWidgets
		{
			weak_ptr<ImageWidget> icon;
			weak_ptr<TextWidget> inputLabel;
			weak_ptr<TextWidget> stateText;
		};
		Map<AbilitySlot, SlotWidgets> mSlotWidgets;

		bool mWidgetsCreated = false;
		unsigned int mObservedShipId = 0;
		sf::Vector2u mLastWindowSize;
	};
}


