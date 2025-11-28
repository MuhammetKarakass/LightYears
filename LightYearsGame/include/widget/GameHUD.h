#pragma once

#include <widget/HUD.h>
#include <widget/TextWidget.h>
#include <widget/ValueGauge.h>
#include <widget/ImageWidget.h>
#include <widget/Button.h>

namespace ly
{
	class Actor;

	class GameHUD : public HUD
	{
	public:
		GameHUD();
		virtual void Draw(sf::RenderWindow& windowRef) override;
		virtual void Tick(float deltaTime) override;
		virtual bool HandleEvent(const sf::Event& event) override;

		void TotalChaosStarted();
		void OnChaosTimerUpdated(float remainingTime);
		void TotalChaosEnded();

	private:
		virtual void Init(sf::RenderWindow& windowRef) override;
		void RefreshHealthBar();
		void PlayerHealthUpdated(float amt, float currentHealth, float maxHealth);
		void PlayerSpaceShipDestroyed(Actor* actor);
		void ConnectStatus();
		void PlayerLifeUpdated(int amt);
		void PlayerScoreUpdated(int amt);


		std::optional<TextWidget> mFrameRateText;
		std::optional<ValueGauge> mPlayerHealthBar;

		std::optional<ImageWidget> mPlayerLifeIcon;
		std::optional<TextWidget> mPlayerLifeText;

		std::optional<ImageWidget> mPlayerScoreIcon;
		std::optional<TextWidget> mPlayerScoreText;

		std::optional<TextWidget> mChaosTimerText;
		std::optional<TextWidget> mSurviveText;

		float mWidgetSpacingX;
		bool mChaosTimerTextIsVisible;
	};
}