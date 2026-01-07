#pragma once 
#include "spaceShip/SpaceShip.h"
#include "framework/Core.h"
#include "framework/TimerManager.h"
#include "gameConfigs/GameplayConfig.h"
#include "gameConfigs/GameplayStructs.h"

namespace ly
{
	class World;
	class Shooter;
	class PlayerSpaceShip : public SpaceShip
	{
	public:

		PlayerSpaceShip(World* owningWorld, const ShipDefinition& shipDef = GameData::Ship_Player_Fighter);

		virtual void BeginPlay() override;
		virtual void ApplyDamage(float amt) override;
		virtual void Tick(float deltaTime) override;
		void SetSpeed(float speed) { mSpeed = speed; }
		float GetSpeed()const { return mSpeed; }

		void SetShooter(unique_ptr<Shooter>&& shooter, WeaponType type);
		Shooter* GetShooter() const { return mShooter.get(); }

		virtual void SetupCollisionLayers() override;
		
		virtual void OnActorBeginOverlap(Actor* otherActor) override;

		void SetWeaponType(WeaponType type) { mWeaponType = type; };
		WeaponType GetWeaponType() const { return mWeaponType; };
		int GetWeaponLevel() const;

		void ApplyWeaponState(const WeaponState& state);

		Delegate<WeaponType, int> onWeaponStateBeforeDeath;

	private:
		void SetInput();
		void ConsumeInput();
		void NormalizeInput();
		void ClampInputOnEdge();
		virtual void Shoot() override;
		void StopInvulnerability();
		void UpdateInvulnerability(float deltaTime);

		float mSpeed;
		sf::Vector2f mMoveInput;

		unique_ptr<Shooter> mShooter;
		WeaponType mWeaponType{ WeaponType::Default };

		float mInvulnerabilityTime;
		TimerHandle mInvulnerabilityTimerHandle;
		bool mInvulnerable{ false };

		float mInvulnerabilityBlinkInterval;
		float mInvulnerabilityBlinkTimer;
		float mInvulnerabilityDir;
		float mShaderTime{ 0.f };
		float mCollisionDamage;
	};
}