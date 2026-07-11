#pragma once 
#include "spaceShip/SpaceShip.h"
#include "framework/Core.h"
#include "framework/TimerManager.h"
#include "gameConfigs/ShipConfig.h"

namespace ly
{
	class World;
	class PlayerSpaceShip : public SpaceShip
	{
	public:

		PlayerSpaceShip(World* owningWorld, const ShipDefinition& shipDef = ShipData::Ship_Player_Fighter);

		virtual void BeginPlay() override;
		virtual void ApplyDamage(float amt) override;
		virtual void ReceiveDamage(DamageContext context) override;
		virtual void Tick(float deltaTime) override;
		void SetSpeed(float speed) { mSpeed = speed; }
		float GetSpeed()const { return mSpeed; }

		void SetUseScreenClamp(bool useClamp) { mUseScreenClamp = useClamp; }
		bool GetUseScreenClamp() const { return mUseScreenClamp; }

		virtual void SetupCollisionLayers() override;
		
		virtual void OnActorBeginOverlap(Actor* otherActor) override;

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

		float mInvulnerabilityTime;
		TimerHandle mInvulnerabilityTimerHandle;
		float mInvulnerabilityBlinkInterval;
		float mInvulnerabilityBlinkTimer;
		float mInvulnerabilityDir;
		float mShaderTime{ 0.f };
		float mCollisionDamage;

		bool mUseScreenClamp{ true };
	};
}


