#pragma once

#include <framework/Object.h>
#include <framework/Delegate.h>

namespace ly
{
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

		unsigned int GetLifeCount() const { return mLifeCount; };
		unsigned int GetScore() const { return mScore; };

		Delegate <int> onLifeChange;
		Delegate <int> onScoreChange;
		Delegate<> onLifeExhausted;

		void OnScoreAwarded(unsigned int scoreAmount);

	private:
		weak_ptr<PlayerSpaceShip> mCurrentSpaceShip;
		unsigned int mLifeCount;
		unsigned int mScore;


	};
}