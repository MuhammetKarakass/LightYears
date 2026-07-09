#pragma once 
#include "spaceShip/SpaceShip.h"
#include "framework/Core.h"
#include "framework/TimerManager.h"
#include "gameConfigs/GameplayConfig.h"
#include "gameConfigs/GameplayStructs.h"
#include "gameplay/ability/AbilitySystem.h"
#include "player/Shield.h"

namespace ly
{
	class World;
	class PlayerSpaceShip : public SpaceShip
	{
	public:

		PlayerSpaceShip(World* owningWorld, const ShipDefinition& shipDef = GameData::Ship_Player_Fighter);

		virtual void BeginPlay() override;
		virtual void ApplyDamage(float amt) override;
		virtual void Tick(float deltaTime) override;
		void SetSpeed(float speed) { mSpeed = speed; }
		float GetSpeed()const { return mSpeed; }

		void SetUseScreenClamp(bool useClamp) { mUseScreenClamp = useClamp; }
		bool GetUseScreenClamp() const { return mUseScreenClamp; }

		virtual void SetupCollisionLayers() override;
		
		virtual void OnActorBeginOverlap(Actor* otherActor) override;

		void ActivateShield(float bonusHP, float duration);

		Delegate<bool> onShieldStateChanged;

		Shield& GetShield() { return mShield; }

	private:
		void SetInput();
		void ConsumeInput(float deltaTime);
		void NormalizeInput();
		void ClampInputOnEdge();
		sf::Vector2f GetAdaptiveScreenStrafeDirection() const;
		void RotateTowardMouseCursor(float deltaTime);
		virtual void Shoot() override;
		void StopInvulnerability();
		void UpdateInvulnerability(float deltaTime);

		float mSpeed;
		sf::Vector2f mMoveInput;
		sf::Vector2f mSmoothedMoveInput;

		AbilitySystem mAbilitySystem;

		float mInvulnerabilityTime;
		TimerHandle mInvulnerabilityTimerHandle;
		bool mInvulnerable{ false };

		float mInvulnerabilityBlinkInterval;
		float mInvulnerabilityBlinkTimer;
		float mInvulnerabilityDir;
		float mShaderTime{ 0.f };
		float mCollisionDamage;

		bool mUseScreenClamp{ true };

		void DeactivateShield();
		Shield mShield;
		TimerHandle mShieldTimerHandle;
		void OnShieldStateChanged(bool active);
	};
}
