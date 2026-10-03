#pragma once 
#include "spaceShip/SpaceShip.h"
#include "framework/Core.h"
#include "framework/TimerManager.h"
#include "player/PlayerMovementComponent.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/ability/loadout/AbilityLoadoutManager.h"
#include "gameplay/module/ModuleTriggerRuntime.h"
#include "gameplay/module/ShipModuleApplier.h"

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
		ShipModuleApplier& GetModuleApplier() { return mModuleApplier; }
		const ShipModuleApplier& GetModuleApplier() const { return mModuleApplier; }
		ModuleTriggerRuntime& GetModuleTriggerRuntime() { return mModuleTriggerRuntime; }
		const ModuleTriggerRuntime& GetModuleTriggerRuntime() const { return mModuleTriggerRuntime; }
		// Enabled module systems, readable for HUD work.
		const List<GameplayTag>& GetEnabledModuleSystems() const { return mModuleApplier.GetEnabledSystems(); }
		// True while this ship took or dealt damage within the module combat window.
		bool IsInCombat() const { return mCombatWindowRemaining > 0.f; }
		float GetCombatWindowRemaining() const { return mCombatWindowRemaining; }
		AbilityLoadoutManager& GetAbilityLoadout() { return mAbilityLoadout; }
		const AbilityLoadoutManager& GetAbilityLoadout() const { return mAbilityLoadout; }

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
		void OnCombatDamageObserved(const DamageContext& context);
		void UpdateInvulnerability(float deltaTime);

		AbilityLoadoutManager mAbilityLoadout;
		PlayerMovementComponent& mPlayerMovement;
		ModuleTriggerRuntime mModuleTriggerRuntime;
		ShipModuleApplier mModuleApplier;
		float mCombatWindowRemaining = 0.f;

		float mInvulnerabilityTime;
		TimerHandle mInvulnerabilityTimerHandle;
		float mInvulnerabilityBlinkInterval;
		float mInvulnerabilityBlinkTimer;
		float mInvulnerabilityDir;
		float mCollisionDamage;
	};
}

