#include "widget/GameHUD.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "framework/TimerManager.h"
#include <cmath>
#include <cstdio>

namespace ly
{
	GameHUD::GameHUD() :
		mFrameRateText{ std::in_place, "Frame Rate:" },
		mPlayerHealthBar{ std::in_place },
		mPlayerLifeIcon{ std::in_place, "SpaceShooterRedux/PNG/pickups/playerLife1_blue.png" },
		mPlayerLifeText{ std::in_place, " " },
		mPlayerScoreIcon{ std::in_place, "SpaceShooterRedux/PNG/Power-ups/star_gold.png" },
		mPlayerScoreText{ std::in_place, " " },
		mTopCenterText{ std::in_place, "", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf" },
		mWidgetSpacingX{ 10.f }
	{
		mFrameRateText->SetTextSize(20);
		mPlayerLifeText->SetTextSize(20);
		mPlayerScoreText->SetTextSize(20);
		mTopCenterText->SetTextSize(20);
		mTopCenterText->SetVisibility(false);
		mTopCenterText->SetFillColor(sf::Color::Red);

		/*mCenterNotificationText->SetTextSize(90);
		mCenterNotificationText->SetFillColor(sf::Color::Red);
		mCenterNotificationText->CenterOrigin();
		mCenterNotificationText->SetVisibility(false);
		mCenterNotificationText->SetAlpha(0.f);*/
	}

	void GameHUD::Draw(sf::RenderWindow& windowRef)
	{
		mWindowRef = &windowRef;

		if (mFrameRateText.has_value())
			mFrameRateText->NativeDraw(windowRef);

		if (mPlayerHealthBar.has_value())
			mPlayerHealthBar->NativeDraw(windowRef);

		if (mPlayerLifeIcon.has_value())
			mPlayerLifeIcon->NativeDraw(windowRef);

		if (mPlayerLifeText.has_value())
			mPlayerLifeText->NativeDraw(windowRef);

		if (mPlayerScoreIcon.has_value())
			mPlayerScoreIcon->NativeDraw(windowRef);

		if (mPlayerScoreText.has_value())
			mPlayerScoreText->NativeDraw(windowRef);

		if (mTopCenterText.has_value())
			mTopCenterText->NativeDraw(windowRef);

		if (auto centerNotificationText = mCenterNotificationText.lock())
			centerNotificationText->NativeDraw(windowRef);

		if(auto timerText = mTimerText.lock())
			timerText->NativeDraw(windowRef);

		if(auto bossHealthBar = mBossHealthBar.lock())
			bossHealthBar->NativeDraw(windowRef);

		if(auto bossNameText = mBossNameText.lock())
			bossNameText->NativeDraw(windowRef);

		HUD::Draw(windowRef);
	}

	void GameHUD::Tick(float deltaTime)
	{
		static int lastFrameRate = 0;
		int frameRate = deltaTime > 0.f ? int(1.f / deltaTime) : 0;

		if (mFrameRateText.has_value() && frameRate != lastFrameRate)
		{
			lastFrameRate = frameRate;

			static char buffer[32];
			snprintf(buffer, sizeof(buffer), "Frame Rate: %d", frameRate);
			mFrameRateText->SetString(buffer);
		}


		if (auto centerNotificationText = mCenterNotificationText.lock())
		{
			if (centerNotificationText->IsAnimating())
				centerNotificationText->NativeTick(deltaTime);
		}

		if (auto timerText = mTimerText.lock())
		{
			if (timerText->IsAnimating())
				timerText->NativeTick(deltaTime);
		}

		if(auto bossHealthBar = mBossHealthBar.lock())
		{
			if (bossHealthBar->IsAnimating())
				bossHealthBar->NativeTick(deltaTime);
		}

		if(auto bossNameText = mBossNameText.lock())
		{
			if (bossNameText->IsAnimating())
				bossNameText->NativeTick(deltaTime);
		}

		RefreshPlayerHUDState();
		UpdateGameplayWarningVisuals(deltaTime);

	}

	bool GameHUD::HandleEvent(const sf::Event& event)
	{
		return HUD::HandleEvent(event);
	}

	void GameHUD::ShowDynamicNotification(const std::string& newText, float fadeIn, float hold, float fadeOut,const sf::Vector2f& location, float size, sf::Color color)
	{
		LOG("GameHUD::ShowDynamicNotification called with text: %s", newText.c_str());
		mCenterNotificationText = AddWidget<TextWidget>(newText, "SpaceShooterRedux/Bonus/OrbitronBlack.ttf");
		if (auto t = mCenterNotificationText.lock())
		{
			t->SetTextSize(size);
			t->SetFillColor(color);
			t->CenterOrigin();
			t->SetWidgetLocation(location);
			t->SetVisibility(true);
			t->StartFadeAnimation(fadeIn, hold, fadeOut);
			t->SetLifeTime(fadeIn + hold + fadeOut+1.f);
		}
	}

	void GameHUD::Init(sf::RenderWindow& windowRef)
	{
		auto windowSize = windowRef.getSize();
		mWindowSize = windowSize;
		mPlayerHealthBar->SetWidgetLocation(sf::Vector2f{ 20.f, windowSize.y - 50.f });

		sf::Vector2f nextWidgetPos = mPlayerHealthBar->GetWidgetLocation();
		nextWidgetPos += sf::Vector2f{ mPlayerHealthBar->GetBound().size.x + mWidgetSpacingX, 0.f };
		mPlayerLifeIcon->SetWidgetLocation(nextWidgetPos);

		nextWidgetPos += sf::Vector2f{ mPlayerLifeIcon->GetBound().size.x + mWidgetSpacingX,0.f };
		mPlayerLifeText->SetWidgetLocation(nextWidgetPos);

		nextWidgetPos += sf::Vector2f{ mPlayerLifeText->GetBound().size.x + mWidgetSpacingX * 2.f,-2.f };
		mPlayerScoreIcon->SetWidgetLocation(nextWidgetPos);

		nextWidgetPos += sf::Vector2f{ mPlayerScoreIcon->GetBound().size.x + mWidgetSpacingX,+2.f };
		mPlayerScoreText->SetWidgetLocation(nextWidgetPos);

		mTopCenterText->SetWidgetLocation(sf::Vector2f{ windowSize.x / 2.f, 0.f });



		RefreshHealthBar();
		ConnectStatus();
	}


	void GameHUD::RefreshHealthBar()
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();

		if (!player || player->GetCurrentSpaceShip().expired())
		{
			mObservedPlayerSpaceShip.reset();
			mShieldActive = false;
			mPlayerHealthBar->UpdateValue(0.f, 1.f);
			return;
		}

		weak_ptr<PlayerSpaceShip> playerSpaceShip = player->GetCurrentSpaceShip();
		shared_ptr<PlayerSpaceShip> lockedSpaceShip = playerSpaceShip.lock();
		if (!lockedSpaceShip || lockedSpaceShip->GetIsPendingDestroy())
		{
			mObservedPlayerSpaceShip.reset();
			mShieldActive = false;
			mPlayerHealthBar->UpdateValue(0.f, 1.f);
			return;
		}

		if (mObservedPlayerSpaceShip.lock() != lockedSpaceShip)
		{
			mObservedPlayerSpaceShip = lockedSpaceShip;
			lockedSpaceShip->onActorDestroyed.BindAction(GetWeakPtr(), &GameHUD::PlayerSpaceShipDestroyed);
			lockedSpaceShip->GetHealthComponent().onHealthChanged.BindAction(GetWeakPtr(), &GameHUD::PlayerHealthUpdated);
			lockedSpaceShip->onShieldStateChanged.BindAction(GetWeakPtr(), &GameHUD::OnShieldStateChanged);
		}

		HealthComponent& healthComponent = lockedSpaceShip->GetHealthComponent();
		mShieldActive = false;
		PlayerHealthUpdated(0, healthComponent.GetHealth(), healthComponent.GetMaxHealth());
	}

	void GameHUD::PlayerHealthUpdated(float amt, float currentHealth, float maxHealth)
	{
		if (maxHealth <= 0.f)
		{
			mPlayerHealthBar->UpdateValue(0.f, 1.f);
			mPlayerHealthBar->SetForegroundColor(sf::Color{ 255, 0, 0, 255 });
			return;
		}

		float totalHealth = currentHealth;
		float totalMax = maxHealth;

		if (mShieldActive)
		{
			Player* player = PlayerManager::GetPlayerManager().GetPlayer();
			if (player && !player->GetCurrentSpaceShip().expired())
			{
				auto ship = player->GetCurrentSpaceShip().lock();
				if (ship)
				{
					totalHealth += ship->GetShield().GetHealth();
					totalMax += ship->GetShield().GetMaxHealth();
				}
			}
		}

		mPlayerHealthBar->UpdateValue(totalHealth, totalMax);

		if (mShieldActive)
		{
			mPlayerHealthBar->SetForegroundColor(sf::Color{ 80, 160, 255, 255 });
			return;
		}

		float healthPercent = currentHealth / maxHealth;
		std::uint8_t r, g;

		if (healthPercent >= 0.5f)
		{
			float t = (healthPercent - 0.5f) * 2.0f;
			r = static_cast<std::uint8_t>(255 * (1.0f - t));
			g = 255;
		}
		else
		{
			float t = healthPercent * 2.0f;
			r = 255;
			g = static_cast<std::uint8_t>(255 * t);
		}

		mPlayerHealthBar->SetForegroundColor(sf::Color{ r, g, 0, 255 });
	}

	void GameHUD::PlayerSpaceShipDestroyed(Actor* actor)
	{
		mObservedPlayerSpaceShip.reset();
		TimerManager::GetGlobalTimerManager().ClearTimer(mRefreshHealthBarTimerHandle);
		mRefreshHealthBarTimerHandle = TimerManager::GetGlobalTimerManager().SetTimer(
			GetWeakPtr(),
			&GameHUD::RefreshHealthBarDeferred,
			0.01f,
			false
		);
	}

	void GameHUD::RefreshHealthBarDeferred()
	{
		RefreshHealthBar();
	}

	void GameHUD::ConnectStatus()
	{
		if (mIsStatusConnected)
		{
			return;
		}

		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player)
			return;

		mIsStatusConnected = true;

		int lifeCount = player->GetLifeCount();
		mPlayerLifeText->SetString(std::to_string(lifeCount));
		player->onLifeChange.BindAction(GetWeakPtr(), &GameHUD::PlayerLifeUpdated);
		int scoreCount = player->GetScore();
		mPlayerScoreText->SetString(std::to_string(scoreCount));
		player->onScoreChange.BindAction(GetWeakPtr(), &GameHUD::PlayerScoreUpdated);
	}

	void GameHUD::RefreshPlayerHUDState()
	{
		ConnectStatus();

		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player)
		{
			return;
		}

		weak_ptr<PlayerSpaceShip> currentPlayerShip = player->GetCurrentSpaceShip();
		shared_ptr<PlayerSpaceShip> currentShip = currentPlayerShip.lock();
		shared_ptr<PlayerSpaceShip> observedShip = mObservedPlayerSpaceShip.lock();

		if (!currentShip)
		{
			return;
		}

		if (currentShip->GetIsPendingDestroy())
		{
			return;
		}

		if (currentShip != observedShip)
		{
			RefreshHealthBar();
		}
	}

	void GameHUD::PlayerLifeUpdated(int amt)
	{
		mPlayerLifeText->SetString(std::to_string(amt));
	}

	void GameHUD::PlayerScoreUpdated(int amt)
	{
		mPlayerScoreText->SetString(std::to_string(amt));
	}

	void GameHUD::OnShieldStateChanged(bool active)
	{
		mShieldActive = active;

		// Force a health bar color refresh
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (player && !player->GetCurrentSpaceShip().expired())
		{
			auto ship = player->GetCurrentSpaceShip().lock();
			if (ship)
			{
				HealthComponent& health = ship->GetHealthComponent();
				PlayerHealthUpdated(0.f, health.GetHealth(), health.GetMaxHealth());
			}
		}
	}

	void GameHUD::ShowTimer(float fadeIn, float hold, float fadeOut)
	{
		if (!mTimerText.expired()) return;
		mTimerText = AddWidget<TextWidget>("", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 20);

		if (auto timerTextLocked = mTimerText.lock())
		{
			timerTextLocked->SetVisibility(true);
			timerTextLocked->SetWidgetLocation(sf::Vector2f{ mWindowSize.x / 2.f, 0.f });
			timerTextLocked->SetFillColor(sf::Color::Red);
			timerTextLocked->CenterOrigin();
			timerTextLocked->SetAlpha(0.f);
			timerTextLocked->StartFadeAnimation(fadeIn, hold, fadeOut);
		}
	}

	void GameHUD::UpdateTimer(float timeLeft)
	{
		if (auto timerTextLocked = mTimerText.lock())
		{
			int timeInt = static_cast<int>(std::ceil(timeLeft));
			timerTextLocked->SetString("TIME LEFT: " + std::to_string(timeInt));
		}
	}

	void GameHUD::TimerFinished()
	{
		RemoveWidget(mTimerText);
	}

	void GameHUD::CreateBossHealthBar(const std::string& bossName, float health, float maxHealth)
	{
		mBossNameText = AddWidget<TextWidget>(bossName, "SpaceShooterRedux/Bonus/OrbitronBlack.ttf", 20);
		mBossHealthBar = AddWidget<ValueGauge>(sf::Vector2f{450.f,30.f},1.f, sf::Color::Red, sf::Color{50,50,50,200});
		if (auto bossHealthBarLocked = mBossHealthBar.lock() )
		{
			if(auto bossNameTextLocked = mBossNameText.lock())
			{
				bossNameTextLocked->SetWidgetLocation(sf::Vector2f{ mWindowSize.x / 2.f, 30.f });
				bossNameTextLocked->CenterOrigin();
				bossNameTextLocked->SetFillColor(sf::Color{181,60,0});
				bossNameTextLocked->SetVisibility(true);
				bossNameTextLocked->SetAlpha(0.f);
				bossNameTextLocked->StartFadeAnimation(2.f, 0.f, 0.f);
				auto textLocation = bossNameTextLocked->GetWidgetLocation();
				bossHealthBarLocked->SetWidgetLocation(sf::Vector2f{ mWindowSize.x / 2.f, textLocation.y+35.f });
				bossHealthBarLocked->CenterOrigin();
				bossHealthBarLocked->SetVisibility(true);
				bossHealthBarLocked->SetAlpha(0.f);
				bossHealthBarLocked->StartFadeAnimation(2.f, 0.f, 0.f);
				bossHealthBarLocked->UpdateValue(health, maxHealth);
			}
		}
	}

	void GameHUD::BossHealthUpdated(float amt, float currentHealth, float maxHealth)
	{
		if (auto bossHealthBarLocked = mBossHealthBar.lock())
		{
			bossHealthBarLocked->UpdateValue(currentHealth, maxHealth);
		}
	}

	void GameHUD::RemoveBossHealthBar(Actor* actor)
	{
		if (auto bossHealthBar = mBossHealthBar.lock())
		{
			bossHealthBar->StartFadeAnimation(0.f, 0.f, 2.f);
			bossHealthBar->SetLifeTime(2.5f);
		}

		if (auto bossNameText = mBossNameText.lock())
		{
			bossNameText->StartFadeAnimation(0.f, 0.f, 2.f);
			bossNameText->SetLifeTime(2.5f);
		}
	}

	void GameHUD::ShowGameplayWarning(const GameplayWarning& warning)
	{
		if (!mTopCenterText.has_value())
		{
			return;
		}

		char warningText[128];
		if (warning.hasCountdown)
		{
			snprintf(
				warningText,
				sizeof(warningText),
				"%s\n%s %.2f",
				warning.title.c_str(),
				warning.message.c_str(),
				warning.remainingTime
			);
		}
		else
		{
			snprintf(
				warningText,
				sizeof(warningText),
				"%s\n%s",
				warning.title.c_str(),
				warning.message.c_str()
			);
		}

		const bool isNewWarning = !mHasActiveGameplayWarning || mActiveGameplayWarningType != warning.type;
		mHasActiveGameplayWarning = true;
		mActiveGameplayWarningType = warning.type;

		if (isNewWarning)
		{
			mGameplayWarningAnimTime = 0.f;
		}

		mTopCenterText->SetString(warningText);
		mTopCenterText->SetTextSize(24);
		mTopCenterText->SetFillColor(sf::Color{ 255, 45, 45, 245 });
		mTopCenterText->CenterOrigin();
		mGameplayWarningBaseLocation = sf::Vector2f{ mWindowSize.x * 0.5f, 54.f };
		mTopCenterText->SetWidgetLocation(mGameplayWarningBaseLocation);
		mTopCenterText->SetVisibility(true);
	}

	void GameHUD::HideGameplayWarning(GameplayWarningType warningType)
	{
		if (!mTopCenterText.has_value() || !mHasActiveGameplayWarning)
		{
			return;
		}

		if (mActiveGameplayWarningType != warningType)
		{
			return;
		}

		mHasActiveGameplayWarning = false;
		mTopCenterText->SetVisibility(false);
	}

	void GameHUD::UpdateGameplayWarningVisuals(float deltaTime)
	{
		if (!mHasActiveGameplayWarning || !mTopCenterText.has_value())
		{
			return;
		}

		mGameplayWarningAnimTime += deltaTime;

		const float pulse = (std::sin(mGameplayWarningAnimTime * 9.5f) + 1.f) * 0.5f;
		const float flicker = (std::sin(mGameplayWarningAnimTime * 37.f) + 1.f) * 0.5f;
		const float threat = std::max(pulse, flicker * 0.65f);

		const std::uint8_t greenBlue = static_cast<std::uint8_t>(35.f + threat * 45.f);
		const std::uint8_t alpha = static_cast<std::uint8_t>(205.f + threat * 50.f);

		const float shakeX = std::sin(mGameplayWarningAnimTime * 51.f) * 1.8f * threat;
		const float shakeY = std::sin(mGameplayWarningAnimTime * 29.f) * 1.2f * threat;

		mTopCenterText->SetFillColor(sf::Color{ 255, greenBlue, greenBlue, alpha });
		mTopCenterText->SetWidgetLocation(mGameplayWarningBaseLocation + sf::Vector2f{ shakeX, shakeY });
	}

}


