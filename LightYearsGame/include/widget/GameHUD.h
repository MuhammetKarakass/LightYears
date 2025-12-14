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

		void ShowDynamicNotification(const std::string& newText, float fadeIn, float hold, float fadeOut,const sf::Vector2f& location, float size, sf::Color color = sf::Color::Red);

		void ShowNotification(const std::string& newText, float fadeIn, float hold, float fadeOut);

		void ShowTimer(float fadeIn, float hold, float fadeOut);
		void UpdateTimer(float timeLeft);
		void TimerFinished();

		void CreateBossHealthBar(const std::string& bossName,float health, float maxHealth);
		void BossHealthUpdated(float amt, float currentHealth, float maxHealth);
		void RemoveBossHealthBar(Actor* actor);

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

		std::optional<TextWidget> mTopCenterText;
		weak_ptr<TextWidget> mTimerText;
		weak_ptr<TextWidget> mCenterNotificationText;

		weak_ptr<ValueGauge> mBossHealthBar;
		weak_ptr<TextWidget> mBossNameText;

		sf::Vector2u mWindowSize{ 0,0 };
		sf::RenderWindow* mWindowRef{ nullptr };

		float mWidgetSpacingX;
	};
}