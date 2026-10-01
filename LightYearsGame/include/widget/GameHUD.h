#pragma once

#include <widget/HUD.h>
#include <widget/TextWidget.h>
#include <unordered_set>

namespace ly
{
	class SpaceShip;
	struct DamageContext;
	struct GameHUDDamageE2ETestAccess;

	class GameHUD : public HUD
	{
		friend struct GameHUDDamageE2ETestAccess;

	public:
		GameHUD();
		~GameHUD() override;
		virtual void Draw(sf::RenderWindow& windowRef) override;
		virtual void Tick(float deltaTime) override;

	private:
		virtual void Init(sf::RenderWindow& windowRef) override;
		void ConnectDamageObservers();
		void ShipDamageResolved(const DamageContext& context);
		void UpdateDamageNumberVisuals(float deltaTime);
		void UpdatePlayerSpeed();

		struct DamageNumberEntry
		{
			weak_ptr<SpaceShip> ship;
			weak_ptr<TextWidget> widget;
			float age{ 0.f };
			// Grouping key so numbers stay stacked per ship even after the ship is gone.
			unsigned int shipId{ 0 };
			// Screen-space anchor captured while the ship still existed, so the number from a
			// killing blow survives the ship's destruction instead of being erased instantly.
			sf::Vector2f lastPixelAnchor{ 0.f, 0.f };
			bool hasAnchor{ false };
		};

		// TEMPORARY TEST UI: damage numbers and the speed readout are prototype
		// presentation only and will be replaced/repositioned during HUD polish.
		std::optional<TextWidget> mFrameRateText;
		std::optional<TextWidget> mPlayerSpeedText;

		List<DamageNumberEntry> mDamageNumbers;
		// Keyed on Object::GetUniqueID(), not on the raw actor address: destroyed enemies
		// free their address and the spawner reuses it, so a pointer key made a fresh
		// enemy look already observed and it never received a damage observer.
		std::unordered_set<unsigned int> mObservedDamageShips;

		sf::RenderWindow* mWindowRef{ nullptr };

	};
}


