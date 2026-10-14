#include "widget/GameHUD.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"
#include "gameplay/damage/DamageContext.h"
#include <framework/MathUtility.h>
#include <framework/World.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ly
{
	GameHUD::~GameHUD()
	{
	}

	GameHUD::GameHUD() :
		mFrameRateText{ std::in_place, "Frame Rate:" },
		mPlayerSpeedText{ std::in_place, "Speed:" }
	{
		mFrameRateText->SetTextSize(20);
		mPlayerSpeedText->SetTextSize(20);

	}

	void GameHUD::Draw(sf::RenderTarget& windowRef)
	{
		mWindowRef = &windowRef;

		if (mFrameRateText.has_value())
			mFrameRateText->NativeDraw(windowRef);

		if (mPlayerSpeedText.has_value())
			mPlayerSpeedText->NativeDraw(windowRef);

		HUD::Draw(windowRef);
	}

	void GameHUD::Tick(float deltaTime)
	{
		// Frames counted over half a second and rounded: truncating 1/dt of single frames turned the normal
		// microsecond jitter of a 16.67 ms frame into a counter that kept flickering between 58 and 60.
		static int lastFrameRate = 0;
		static int sampleFrames = 0;
		static float sampleSeconds = 0.f;
		++sampleFrames;
		sampleSeconds += deltaTime;
		if (sampleSeconds >= 0.5f)
		{
			const int frameRate = static_cast<int>(std::lround(sampleFrames / sampleSeconds));
			sampleFrames = 0;
			sampleSeconds = 0.f;
			if (mFrameRateText.has_value() && frameRate != lastFrameRate)
			{
				lastFrameRate = frameRate;

				static char buffer[32];
				snprintf(buffer, sizeof(buffer), "Frame Rate: %d", frameRate);
				mFrameRateText->SetString(buffer);
			}
		}


		ConnectDamageObservers();
		UpdatePlayerSpeed();
		UpdateDamageNumberVisuals(deltaTime);
		HUD::Tick(deltaTime);

	}

	void GameHUD::Init(sf::RenderTarget& windowRef)
	{
		mWindowRef = &windowRef;
		mFrameRateText->SetWidgetLocation(sf::Vector2f{ 20.f, 18.f });
		mPlayerSpeedText->SetWidgetLocation(sf::Vector2f{ 20.f, 43.f });
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
			if (!ship)
			{
				continue;
			}

			// The unique id is monotonic and never recycled, unlike the actor address.
			const unsigned int shipId = ship->GetUniqueID();
			if (mObservedDamageShips.find(shipId) != mObservedDamageShips.end())
			{
				continue;
			}

			ship->GetCombatRuntime().onDamageResolved.BindAction(GetWeakPtr(), &GameHUD::ShipDamageResolved);
			mObservedDamageShips.insert(shipId);
		}
	}

	void GameHUD::ShipDamageResolved(const DamageContext& context)
	{
		SpaceShip* ship = context.target ? dynamic_cast<SpaceShip*>(context.target) : nullptr;
		if (!ship || context.appliedDamage <= 0.f || !mWindowRef)
		{
			return;
		}

		// DamageReadout module: numbers on anything but the player's own ship are damage
		// the player dealt. One-way read of the module state; no ship or no loadout shows them.
		if (!dynamic_cast<PlayerSpaceShip*>(ship))
		{
			Player* player = PlayerManager::GetPlayerManager().GetPlayer();
			const shared_ptr<PlayerSpaceShip> playerShip = player ? player->GetCurrentSpaceShip().lock() : nullptr;
			if (playerShip && !playerShip->GetModuleApplier().IsReadoutEnabled(GameplayTags::Module::System::DamageReadout))
			{
				return;
			}
		}

		DamageNumberEntry entry;
		entry.ship = std::dynamic_pointer_cast<SpaceShip>(ship->GetWeakPtr().lock());
		if (entry.ship.expired())
		{
			return;
		}

		const std::string damageText = std::to_string(static_cast<int>(std::round(context.appliedDamage)));
		weak_ptr<TextWidget> widget = AddWidget<TextWidget>(
			damageText,
			"SpaceShooterRedux/Bonus/OrbitronBlack.ttf",
			22
		);
		entry.shipId = ship->GetUniqueID();
		entry.widget = widget;
		mDamageNumbers.insert(mDamageNumbers.begin(), entry);

		int sameShipCount = 0;
		for (auto it = mDamageNumbers.begin(); it != mDamageNumbers.end();)
		{
			if (it->shipId != entry.shipId)
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
			// A ship killed by this very hit is already pending destroy when the damage is
			// broadcast, so the killing blow must not drop its number. The entry lives out
			// its full lifetime, frozen at the last position it was drawn at.
			if (!widget || it->age >= 1.5f || (!ship && !it->hasAnchor))
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
				if (candidate.shipId == it->shipId)
				{
					++slot;
				}
			}

			if (ship && ship->GetWorld())
			{
				const sf::FloatRect bounds = ship->GetActorGlobalBounds();
				const sf::Vector2f worldAnchor{
					bounds.position.x + bounds.size.x + 8.f,
					bounds.position.y - 5.f
				};
				const sf::Vector2i pixelAnchor = mWindowRef->mapCoordsToPixel(
					worldAnchor,
					ship->GetWorld()->GetWorldView()
				);
				it->lastPixelAnchor = sf::Vector2f{
					static_cast<float>(pixelAnchor.x),
					static_cast<float>(pixelAnchor.y)
				};
				it->hasAnchor = true;
			}

			const float rise = static_cast<float>(slot) * 24.f + it->age * 14.f;
			const unsigned int textSize = static_cast<unsigned int>(
				std::max(12, 22 - slot * 2)
			);
			widget->SetTextSize(textSize);
			widget->SetOriginNormalized(0.f, 1.f);
			widget->SetWidgetLocation(sf::Vector2f{
				it->lastPixelAnchor.x,
				it->lastPixelAnchor.y - rise
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

}
