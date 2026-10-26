#pragma once 
#include "spaceShip/SpaceShip.h"
#include "framework/Core.h"
#include "framework/TimerManager.h"
#include "player/PlayerMovementComponent.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/ability/loadout/AbilityLoadoutManager.h"
#include "gameplay/module/ModuleTriggerRuntime.h"
#include "gameplay/module/ShipModuleApplier.h"

namespace vega
{
	class World;
	class EnemyActor;
	class PlayerSpaceShip : public SpaceShip
	{
		VEGA_CLASS(PlayerSpaceShip)
	public:

		PlayerSpaceShip(World* owningWorld, const ShipDefinition& shipDef = content::ShipContentCatalog::GetPlayerFighterDefinition());
		const std::string& GetShip() const { return mShip; }
		void SetShip(const std::string& ship);

		virtual void BeginPlay() override;
		virtual void ApplyDamage(float amt) override;
		virtual void ReceiveDamage(DamageContext context) override;
		virtual void Tick(float deltaTime) override;
		bool HandleWeaponInput(const sf::Event& event);
		bool AssignSecondPrimaryWeapon(const std::string& weaponId);
		void SetSpeed(float speed) { mPlayerMovement.SetSpeed(speed); }
		float GetSpeed() const { return mPlayerMovement.GetSpeed(); }
		// Units this ship travelled on the last tick (velocity times time; teleports do not count). The Player accrues them into run counters.
		Delegate<float> onRunDistanceTraveled;
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

		// Possession (Parasite Hull): the player steers the host. This ship's body is suspended and follows the host, so the
		// camera follows it; movement, aim and Primary go to the host; Q/E/F/R and the Afterburner do nothing meanwhile.
		bool BeginPossession(EnemyActor& host);
		// Releases the host (which keeps whatever state it has) and puts this ship back at location.
		void EndPossession(const sf::Vector2f& location);
		bool IsPossessing() const { return mPlayerMovement.HasPossessedHost(); }

		virtual void SetupCollisionLayers() override;
		
		virtual void OnActorBeginOverlap(Actor* otherActor) override;
		virtual float GetMovementSpeedCapMultiplier() const override;
		virtual float GetMovementTurnCapabilityMultiplier() const override;
		virtual bool IsAfterburnerRechargeBlocked() const override;
		virtual LethalDamageResolution ResolveLethalDamage(const DamageContext& context) override;

	private:
		std::string mShip;
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

