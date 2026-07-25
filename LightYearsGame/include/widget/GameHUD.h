#pragma once

#include <widget/HUD.h>
#include <widget/TextWidget.h>
#include <widget/ValueGauge.h>
#include <widget/ImageWidget.h>
#include <widget/Button.h>
#include "framework/TimerManager.h"
#include "gameplay/GameplayWarning.h"

namespace ly
{
	class Actor;
	class PlayerSpaceShip;

	class GameHUD : public HUD
	{
	public:
		GameHUD();
		virtual void Draw(sf::RenderWindow& windowRef) override;
		virtual void Tick(float deltaTime) override;
		virtual bool HandleEvent(const sf::Event& event) override;

		void ShowDynamicNotification(const std::string& newText, float fadeIn, float hold, float fadeOut,const sf::Vector2f& location, float size, sf::Color color = sf::Color::Red);

		void ShowTimer(float fadeIn, float hold, float fadeOut);
		void UpdateTimer(float timeLeft);
		void TimerFinished();

		void CreateBossHealthBar(const std::string& bossName,float health, float maxHealth);
		void BossHealthUpdated(float amt, float currentHealth, float maxHealth);
		void RemoveBossHealthBar(Actor* actor);
		sf::Vector2u GetWindowSize() const { return mWindowSize; }

		void ShowGameplayWarning(const GameplayWarning& warning);
		void HideGameplayWarning(GameplayWarningType warningType);

	private:
		virtual void Init(sf::RenderWindow& windowRef) override;
		void RefreshHealthBar();
		void RefreshEnergyBar(float energy, float maxEnergy);
		void PlayerHealthUpdated(float amt, float currentHealth, float maxHealth);
		void PlayerSpaceShipDestroyed(Actor* actor);
		void RefreshHealthBarDeferred();
		void ConnectStatus();
		void RefreshPlayerHUDState();
		void PlayerLifeUpdated(int amt);
		void PlayerScoreUpdated(int amt);
		void UpdateGameplayWarningVisuals(float deltaTime);

		std::optional<ValueGauge> mPlayerHealthBar;
		std::optional<ValueGauge> mPlayerEnergyBar;
		std::optional<TextWidget> mFrameRateText;

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
		bool mIsStatusConnected{ false };
		weak_ptr<PlayerSpaceShip> mObservedPlayerSpaceShip;
		TimerHandle mRefreshHealthBarTimerHandle;
		bool mHasActiveGameplayWarning{ false };
		GameplayWarningType mActiveGameplayWarningType{ GameplayWarningType::ArenaBoundary };
		float mGameplayWarningAnimTime{ 0.f };
		sf::Vector2f mGameplayWarningBaseLocation{ 0.f, 0.f };
	};
}


