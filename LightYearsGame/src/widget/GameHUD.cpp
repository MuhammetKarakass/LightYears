#include "attributes/AttributeSystem.h"
#include "widget/GameHUD.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "framework/TimerManager.h"
#include <framework/MathUtility.h>
#include <framework/World.h>
#include "gameConfigs/combat/EffectStructs.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ly
{
	GameHUD::GameHUD() :
		mFrameRateText{ std::in_place, "Frame Rate:" },
		mPlayerSpeedText{ std::in_place, "Speed:" },
		mPlayerHealthBar{ std::in_place },
		mPlayerShieldBar{ std::in_place, sf::Vector2f{ 220.f, 18.f }, 1.f, sf::Color{ 120, 180, 255, 255 }, sf::Color{ 35, 55, 95, 255 } },
		mPlayerEnergyBar{ std::in_place, sf::Vector2f{ 220.f, 18.f }, 1.f, sf::Color{ 70, 205, 255, 255 }, sf::Color{ 35, 70, 95, 255 } },
		mPlayerLifeIcon{ std::in_place, "SpaceShooterRedux/PNG/pickups/playerLife1_blue.png" },
		mPlayerLifeText{ std::in_place, " " },
		mPlayerScoreIcon{ std::in_place, "SpaceShooterRedux/PNG/Power-ups/star_gold.png" },
		mPlayerScoreText{ std::in_place, " " },
		mTopCenterText{ std::in_place, "", "SpaceShooterRedux/Bonus/OrbitronBlack.ttf" },
		mWidgetSpacingX{ 10.f }
	{
		mFrameRateText->SetTextSize(20);
		mPlayerSpeedText->SetTextSize(20);
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

		if (mPlayerSpeedText.has_value())
			mPlayerSpeedText->NativeDraw(windowRef);

		if (mPlayerHealthBar.has_value())
			mPlayerHealthBar->NativeDraw(windowRef);

		if (mPlayerShieldBar.has_value())
			mPlayerShieldBar->NativeDraw(windowRef);

		if (mPlayerEnergyBar.has_value())
			mPlayerEnergyBar->NativeDraw(windowRef);

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


		RefreshPlayerHUDState();
		ConnectDamageObservers();
		UpdatePlayerSpeed();
		UpdateDamageNumberVisuals(deltaTime);
		UpdateGameplayWarningVisuals(deltaTime);
		HUD::Tick(deltaTime);

	}

	bool GameHUD::HandleEvent(const sf::Event& event)
	{
		return HUD::HandleEvent(event);
	}

	void GameHUD::ShowDynamicNotification(const std::string& newText, float fadeIn, float hold, float fadeOut,const sf::Vector2f& location, float size, sf::Color color)
	{
		LY_GAME_DEBUG("GameHUD::ShowDynamicNotification called with text: %s", newText.c_str());
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
		mWindowRef = &windowRef;
		mFrameRateText->SetWidgetLocation(sf::Vector2f{ 20.f, 18.f });
		mPlayerSpeedText->SetWidgetLocation(sf::Vector2f{ 20.f, 43.f });
		mPlayerHealthBar->SetWidgetLocation(sf::Vector2f{ 20.f, windowSize.y - 50.f });
		mPlayerShieldBar->SetWidgetLocation(sf::Vector2f{ 20.f, windowSize.y - 74.f });
		mPlayerEnergyBar->SetWidgetLocation(sf::Vector2f{ 20.f, windowSize.y - 98.f });

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
			mPlayerHealthBar->UpdateValue(0.f, 1.f);
			mPlayerShieldBar->UpdateValue(0.f, 1.f);
			return;
		}

		weak_ptr<PlayerSpaceShip> playerSpaceShip = player->GetCurrentSpaceShip();
		shared_ptr<PlayerSpaceShip> lockedSpaceShip = playerSpaceShip.lock();
		if (!lockedSpaceShip || lockedSpaceShip->GetIsPendingDestroy())
		{
			mObservedPlayerSpaceShip.reset();
			mPlayerHealthBar->UpdateValue(0.f, 1.f);
			mPlayerShieldBar->UpdateValue(0.f, 1.f);
			return;
		}

		if (mObservedPlayerSpaceShip.lock() != lockedSpaceShip)
		{
			mObservedPlayerSpaceShip = lockedSpaceShip;
			lockedSpaceShip->onActorDestroyed.BindAction(GetWeakPtr(), &GameHUD::PlayerSpaceShipDestroyed);
			lockedSpaceShip->GetHealthComponent().onHealthChanged.BindAction(GetWeakPtr(), &GameHUD::PlayerHealthUpdated);
			lockedSpaceShip->GetShieldComponent().onShieldChanged.BindAction(GetWeakPtr(), &GameHUD::PlayerShieldUpdated);
		}

		HealthComponent& healthComponent = lockedSpaceShip->GetHealthComponent();
		PlayerHealthUpdated(0, healthComponent.GetHealth(), healthComponent.GetMaxHealth());
		const ShieldComponent& shieldComponent = lockedSpaceShip->GetShieldComponent();
		PlayerShieldUpdated(0, shieldComponent.GetShield(), shieldComponent.GetMaxShield());
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

		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (player && !player->GetCurrentSpaceShip().expired())
		{
			auto ship = player->GetCurrentSpaceShip().lock();
			if (ship)
			{
				for (const sas::GameplayEffectRuntimeSnapshot& effectSnapshot :
					ship->GetAbilitySystemComponent()
						.BuildGameplayEffectSnapshots())
				{
					const sas::GameplayAttribute* capacity = sas::FindAttribute(
						effectSnapshot.runtimeAttributes,
						BarrierEffectSchema::Capacity
					);
					if (capacity && capacity->baseValue > 0.f)
					{
						totalHealth += capacity->currentValue;
						totalMax += capacity->baseValue;
					}
				}
			}
		}

		mPlayerHealthBar->UpdateValue(totalHealth, totalMax);

		if (totalMax > maxHealth)
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

	void GameHUD::PlayerShieldUpdated(float amt, float currentShield, float maxShield)
	{
		if (maxShield <= 0.f)
		{
			mPlayerShieldBar->UpdateValue(0.f, 1.f);
			return;
		}

		mPlayerShieldBar->UpdateValue(std::max(0.f, currentShield), maxShield);
	}

	void GameHUD::RefreshEnergyBar(float energy, float maxEnergy)
	{
		if (!mPlayerEnergyBar.has_value())
		{
			return;
		}

		mPlayerEnergyBar->UpdateValue(
			std::max(0.f, energy),
			maxEnergy > 0.f ? maxEnergy : 1.f
		);
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
			RefreshHealthBar();
			RefreshEnergyBar(0.f, 1.f);
			return;
		}

		weak_ptr<PlayerSpaceShip> currentPlayerShip = player->GetCurrentSpaceShip();
		shared_ptr<PlayerSpaceShip> currentShip = currentPlayerShip.lock();
		shared_ptr<PlayerSpaceShip> observedShip = mObservedPlayerSpaceShip.lock();

		if (!currentShip)
		{
			RefreshHealthBar();
			RefreshEnergyBar(0.f, 1.f);
			return;
		}

		if (currentShip->GetIsPendingDestroy())
		{
			RefreshHealthBar();
			RefreshEnergyBar(0.f, 1.f);
			return;
		}

		if (currentShip != observedShip)
		{
			RefreshHealthBar();
			RefreshEnergyBar(
				currentShip->GetEnergyComponent().GetEnergy(),
				currentShip->GetEnergyComponent().GetMaxEnergy()
			);
			return;
		}

		RefreshHealthBar();
		RefreshEnergyBar(
			currentShip->GetEnergyComponent().GetEnergy(),
			currentShip->GetEnergyComponent().GetMaxEnergy()
		);
	}

	void GameHUD::PlayerLifeUpdated(int amt)
	{
		mPlayerLifeText->SetString(std::to_string(amt));
	}

	void GameHUD::PlayerScoreUpdated(int amt)
	{
		mPlayerScoreText->SetString(std::to_string(amt));
	}

	void GameHUD::ConnectDamageObservers()
	{
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		if (!player)
		{
			return;
		}

		shared_ptr<PlayerSpaceShip> currentShip = player->GetCurrentSpaceShip().lock();
		if (!currentShip || !currentShip->GetWorld())
		{
			return;
		}

		for (const weak_ptr<SpaceShip>& weakShip : currentShip->GetWorld()->GetActorsByType<SpaceShip>())
		{
			shared_ptr<SpaceShip> ship = weakShip.lock();
			if (!ship || mObservedDamageShips.find(ship.get()) != mObservedDamageShips.end())
			{
				continue;
			}

			ship->onDamageTaken.BindAction(GetWeakPtr(), &GameHUD::ShipDamageTaken);
			mObservedDamageShips.insert(ship.get());
		}
	}

	void GameHUD::ShipDamageTaken(SpaceShip* ship, float amount, float health, float maxHealth)
	{
		(void)health;
		(void)maxHealth;
		if (!ship || amount <= 0.f || !mWindowRef)
		{
			return;
		}

		DamageNumberEntry entry;
		entry.ship = std::dynamic_pointer_cast<SpaceShip>(ship->GetWeakPtr().lock());
		if (entry.ship.expired())
		{
			return;
		}

		const std::string damageText = std::to_string(static_cast<int>(std::round(amount)));
		weak_ptr<TextWidget> widget = AddWidget<TextWidget>(
			damageText,
			"SpaceShooterRedux/Bonus/OrbitronBlack.ttf",
			22
		);
		entry.widget = widget;
		mDamageNumbers.insert(mDamageNumbers.begin(), entry);

		int sameShipCount = 0;
		for (auto it = mDamageNumbers.begin(); it != mDamageNumbers.end();)
		{
			if (it->ship.lock().get() != ship)
			{
				++it;
				continue;
			}

			++sameShipCount;
			if (sameShipCount > 5)
			{
				RemoveWidget(it->widget);
				it = mDamageNumbers.erase(it);
				continue;
			}
			++it;
		}

		if (auto lockedWidget = widget.lock())
		{
			lockedWidget->SetFillColor(
				dynamic_cast<PlayerSpaceShip*>(ship)
					? sf::Color{ 255, 105, 105, 255 }
					: sf::Color{ 255, 220, 120, 255 }
			);
			lockedWidget->SetOriginNormalized(0.f, 1.f);
			lockedWidget->SetLifeTime(1.5f);
		}
	}

	void GameHUD::UpdateDamageNumberVisuals(float deltaTime)
	{
		if (!mWindowRef)
		{
			return;
		}

		for (auto it = mDamageNumbers.begin(); it != mDamageNumbers.end();)
		{
			it->age += std::max(0.f, deltaTime);
			shared_ptr<SpaceShip> ship = it->ship.lock();
			shared_ptr<TextWidget> widget = it->widget.lock();
			if (!ship || ship->GetIsPendingDestroy() || !widget || it->age >= 1.5f)
			{
				if (widget)
				{
					RemoveWidget(it->widget);
				}
				it = mDamageNumbers.erase(it);
				continue;
			}

			int slot = 0;
			for (const DamageNumberEntry& candidate : mDamageNumbers)
			{
				if (&candidate == &(*it))
				{
					break;
				}
				if (candidate.ship.lock().get() == ship.get())
				{
					++slot;
				}
			}

			const sf::FloatRect bounds = ship->GetActorGlobalBounds();
			const sf::Vector2f worldAnchor{
				bounds.position.x + bounds.size.x + 8.f,
				bounds.position.y - 5.f
			};
			const sf::Vector2i pixelAnchor = mWindowRef->mapCoordsToPixel(
				worldAnchor,
				ship->GetWorld()->GetWorldView()
			);
			const float rise = static_cast<float>(slot) * 24.f + it->age * 14.f;
			const unsigned int textSize = static_cast<unsigned int>(
				std::max(12, 22 - slot * 2)
			);
			widget->SetTextSize(textSize);
			widget->SetOriginNormalized(0.f, 1.f);
			widget->SetWidgetLocation(sf::Vector2f{
				static_cast<float>(pixelAnchor.x),
				static_cast<float>(pixelAnchor.y) - rise
			});

			const float fadeStart = 1.1f;
			const float alpha = it->age <= fadeStart
				? 1.f
				: std::clamp((1.5f - it->age) / 0.4f, 0.f, 1.f);
			widget->SetAlpha(alpha);
			++it;
		}
	}

	void GameHUD::UpdatePlayerSpeed()
	{
		if (!mPlayerSpeedText.has_value())
		{
			return;
		}

		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		shared_ptr<PlayerSpaceShip> ship = player ? player->GetCurrentSpaceShip().lock() : nullptr;
		const float speed = ship ? GetVectorLength(ship->GetVelocity()) : 0.f;
		char buffer[48];
		snprintf(buffer, sizeof(buffer), "Speed: %.0f", speed);
		mPlayerSpeedText->SetString(buffer);
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
