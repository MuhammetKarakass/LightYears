#include "widget/GameHUD.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "framework/TimerManager.h"

namespace ly
{
	GameHUD::GameHUD() :
		mFrameRateText{ std::in_place, "Frame Rate:" },
		mPlayerHealthBar{ std::in_place },
		mPlayerLifeIcon{ std::in_place, "SpaceShooterRedux/PNG/pickups/playerLife1_blue.png" },
		mPlayerLifeText{ std::in_place, " " },
		mPlayerScoreIcon{ std::in_place, "SpaceShooterRedux/PNG/Power-ups/star_gold.png" },
		mPlayerScoreText{ std::in_place, " "},
		mChaosTimerText{ std::in_place, "", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf" },
		mSurviveText{ std::in_place, "SURVIVE!","SpaceShooterRedux/Bonus/OrbitronBlack.ttf" },
		mWidgetSpacingX{10.f},
		mChaosTimerTextIsVisible{ false }
	{
		mFrameRateText->SetTextSize(20);
		mPlayerLifeText->SetTextSize(20);
		mPlayerScoreText->SetTextSize(20);
		mChaosTimerText->SetTextSize(20);
		mChaosTimerText->SetVisibility(false); // Baþlangýçta gizli
		mChaosTimerText->SetFillColor(sf::Color::Red);

		mSurviveText->SetTextSize(90);
		mSurviveText->SetFillColor(sf::Color::Red);
		mSurviveText->CenterOrigin();
		mSurviveText->SetAlpha(0.f);
	}

	void GameHUD::Draw(sf::RenderWindow& windowRef)
	{
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

		if (mChaosTimerText.has_value())
			mChaosTimerText->NativeDraw(windowRef);

		if (mSurviveText.has_value())
			mSurviveText->NativeDraw(windowRef);
	}

	void GameHUD::Tick(float deltaTime)
	{
		static int lastFrameRate = 0;
		int frameRate = int(1.f / deltaTime);

		if (mFrameRateText.has_value() && frameRate != lastFrameRate)
		{
			lastFrameRate = frameRate;

			static char buffer[32];
			snprintf(buffer, sizeof(buffer), "Frame Rate: %d", frameRate);
			mFrameRateText->SetString(buffer);
		}

		if (mSurviveText.has_value() && mSurviveText->IsAnimating())
			mSurviveText->NativeTick(deltaTime);

		if (mChaosTimerText.has_value() && mChaosTimerText->IsAnimating())
			mChaosTimerText->NativeTick(deltaTime);
	}

	bool GameHUD::HandleEvent(const sf::Event& event)
	{
		return HUD::HandleEvent(event);
	}

	void GameHUD::Init(sf::RenderWindow& windowRef)
	{
		auto windowSize = windowRef.getSize();
		mPlayerHealthBar->SetWidgetLocation(sf::Vector2f{ 20.f, windowSize.y - 50.f });

		sf::Vector2f nextWidgetPos = mPlayerHealthBar->GetWidgetLocation();
		nextWidgetPos += sf::Vector2f{ mPlayerHealthBar->GetBound().size.x + mWidgetSpacingX, 0.f };
		mPlayerLifeIcon->SetWidgetLocation(nextWidgetPos);

		nextWidgetPos += sf::Vector2f{ mPlayerLifeIcon->GetBound().size.x + mWidgetSpacingX,0.f };
		mPlayerLifeText->SetWidgetLocation(nextWidgetPos);

		nextWidgetPos += sf::Vector2f{ mPlayerLifeText->GetBound().size.x + mWidgetSpacingX * 2.f,-2.f};
		mPlayerScoreIcon->SetWidgetLocation(nextWidgetPos);

		nextWidgetPos += sf::Vector2f{ mPlayerScoreIcon->GetBound().size.x + mWidgetSpacingX,+2.f };
		mPlayerScoreText->SetWidgetLocation(nextWidgetPos);

		// Chaos Timer'ý ekranýn ortasýnda, üstte konumlandýr
		mChaosTimerText->SetWidgetLocation(sf::Vector2f{ windowSize.x / 2.f, 0.f });

		mSurviveText->SetWidgetLocation(sf::Vector2f{ windowSize.x / 2.f, windowSize.y / 2.f-50.f });


		RefreshHealthBar();
		ConnectStatus();
	}

	void GameHUD::RefreshHealthBar()
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		
		if (player && !player->GetCurrentSpaceShip().expired())
		{
			weak_ptr<PlayerSpaceShip> playerSpaceShip = player->GetCurrentSpaceShip();
			shared_ptr<PlayerSpaceShip> lockedSpaceShip = playerSpaceShip.lock();
			if (lockedSpaceShip)
			{
				lockedSpaceShip->onActorDestroyed.BindAction(GetWeakPtr(), &GameHUD::PlayerSpaceShipDestroyed);
				HealthComponent& healthComponent = lockedSpaceShip->GetHealthComponent();
				healthComponent.onHealthChanged.BindAction(GetWeakPtr(), &GameHUD::PlayerHealthUpdated);
				PlayerHealthUpdated(0,healthComponent.GetHealth(),healthComponent.GetMaxHealth());
			}
		}
	}
	
	void GameHUD::PlayerHealthUpdated(float amt, float currentHealth, float maxHealth)
	{
		mPlayerHealthBar->UpdateValue(currentHealth,maxHealth);
		
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
		RefreshHealthBar();
	}
	
	void GameHUD::ConnectStatus()
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player)
			return;
		int lifeCount = player->GetLifeCount();
		mPlayerLifeText->SetString(std::to_string(lifeCount));
		player->onLifeChange.BindAction(GetWeakPtr(), &GameHUD::PlayerLifeUpdated);
		int scoreCount = player->GetScore();
		mPlayerScoreText->SetString(std::to_string(scoreCount));
		player->onScoreChange.BindAction(GetWeakPtr(), &GameHUD::PlayerScoreUpdated);
	}
	
	void GameHUD::PlayerLifeUpdated(int amt)
	{
		mPlayerLifeText->SetString(std::to_string(amt));
	}
	
	void GameHUD::PlayerScoreUpdated(int amt)
	{
		mPlayerScoreText->SetString(std::to_string(amt));
	}

	void GameHUD::TotalChaosStarted()
	{
		LOG("*** TOTAL CHAOS STARTED - HUD ACTIVATED ***");
		mChaosTimerText->SetVisibility(true);
		TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			[this]() {
				mSurviveText->StartFadeAnimation(1.0f, 1.5f, 1.0f);
			},
			5.0f,
			false
		);
		

	}

	void GameHUD::OnChaosTimerUpdated(float remainingTime)
	{
		if (!mChaosTimerTextIsVisible)
		{
			mChaosTimerTextIsVisible = true;
			mChaosTimerText->SetAlpha(0.f);
			mChaosTimerText->StartFadeAnimation(.5f, 0.f, 0.f);
		}
		
		int seconds = static_cast<int>(remainingTime);
		mChaosTimerText->SetString("Survive: " + std::to_string(seconds));

	}

	void GameHUD::TotalChaosEnded()
	{
		mChaosTimerText->StartFadeAnimation(0.f, 0.f, 0.5f);
		TimerManager::GetGameTimerManager().SetTimer(
			GetWeakPtr(),
			[this]() {
				mChaosTimerText->SetVisibility(false);
			},
			0.5f,
			false
		);
	}
}