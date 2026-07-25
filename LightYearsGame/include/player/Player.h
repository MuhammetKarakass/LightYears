#pragma once

#include <framework/Object.h>
#include <framework/Delegate.h>
#include "gameplay/progression/ShipProgression.h"
#include <string>

namespace ly
{
	enum class AbilitySlot;
	class Actor;
	class PlayerSpaceShip;
	class World;

	class Player : public Object
	{
	public:
		Player();

		weak_ptr<PlayerSpaceShip> SpawnSpaceShip(World* world);
		const weak_ptr<PlayerSpaceShip> GetCurrentSpaceShip() const { return mCurrentSpaceShip; };

		void AddLifeCount(unsigned int count);
		void AddScore(unsigned int amt);
		void AwardShipXP(float amount);
		void AddShipXP(float amount);
		void AwardScrap(unsigned int amount);
		bool TryPurchaseAbilityLevel(AbilitySlot slot, std::string* failureReason = nullptr);

		unsigned int GetLifeCount() const { return mLifeCount; };
		unsigned int GetScore() const { return mScore; };
		unsigned int GetScrap() const { return mScrap; };
		ShipProgression& GetShipProgression() { return mShipProgression; }
		const ShipProgression& GetShipProgression() const { return mShipProgression; }

		Delegate <int> onLifeChange;
		Delegate <int> onScoreChange;
		Delegate <int> onScrapChange;
		Delegate<> onLifeExhausted;

		void OnScoreAwarded(unsigned int scoreAmount);

	private:
		void RestorePurchasedAbilityLevels(PlayerSpaceShip& ship);
		void OnCurrentShipDestroyed(Actor* destroyedActor);
		void ResetRunProgression();

		weak_ptr<PlayerSpaceShip> mCurrentSpaceShip;
		ShipProgression mShipProgression;
		Map<std::string, int> mPurchasedAbilityLevels;
		unsigned int mLifeCount;
		unsigned int mScore;
		unsigned int mScrap;
	};
}


