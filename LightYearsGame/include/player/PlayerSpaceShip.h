#pragma once 
#include "spaceShip/SpaceShip.h"
#include "framework/Core.h"
#include "framework/TimerManager.h"
#include "player/PlayerMovementComponent.h"
#include "gameplay/content/ShipContentCatalog.h"

namespace ly
{
	class World;
	class PlayerSpaceShip : public SpaceShip
	{
	public:

		PlayerSpaceShip(
			World* owningWorld,
			const ShipDefinition& shipDef = content::ShipContentCatalog::GetPlayerFighterDefinition()
		);

		virtual void BeginPlay() override;
		virtual void ApplyDamage(float amt) override;
		virtual void ReceiveDamage(DamageContext context) override;
		virtual void Tick(float deltaTime) override;
		void SetSpeed(float speed) { mPlayerMovement.SetSpeed(speed); }
		float GetSpeed() const { return mPlayerMovement.GetSpeed(); }

		void SetUseScreenClamp(bool useClamp) { mPlayerMovement.SetUseScreenClamp(useClamp); }
		bool GetUseScreenClamp() const { return mPlayerMovement.GetUseScreenClamp(); }
		bool IsAfterburning() const { return mPlayerMovement.IsAfterburning(); }
		float GetAfterburnerCameraZoomOut() const { return mPlayerMovement.GetAfterburnerCameraZoomOut(); }

		virtual void SetupCollisionLayers() override;
		
		virtual void OnActorBeginOverlap(Actor* otherActor) override;
		virtual float GetMovementSpeedCapMultiplier() const override;
		virtual float GetMovementTurnCapabilityMultiplier() const override;
		virtual bool IsAfterburnerRechargeBlocked() const override;

	private:
		virtual void Shoot() override;
		void StopInvulnerability();
		void UpdateInvulnerability(float deltaTime);

		PlayerMovementComponent mPlayerMovement;

		float mInvulnerabilityTime;
		TimerHandle mInvulnerabilityTimerHandle;
		float mInvulnerabilityBlinkInterval;
		float mInvulnerabilityBlinkTimer;
		float mInvulnerabilityDir;
		float mCollisionDamage;
	};
}


