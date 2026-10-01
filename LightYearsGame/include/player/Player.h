#pragma once

#include <framework/Object.h>
#include <framework/Delegate.h>
#include "gameplay/progression/ShipProgression.h"
#include "abilities/AbilityPolicies.h"
#include "abilities/AbilityHandle.h"
#include "gameplay/module/ModuleLoadout.h"
#include <optional>
#include <string>

namespace ly
{
	class Actor;
	class PlayerSpaceShip;
	class World;

	class Player : public Object
	{
	public:
		explicit Player(std::string shipId = "Ship.Player.Fighter.Basic");
		~Player() override;

		weak_ptr<PlayerSpaceShip> SpawnSpaceShip(World* world);
		const weak_ptr<PlayerSpaceShip> GetCurrentSpaceShip() const { return mCurrentSpaceShip; };

		void AddLifeCount(unsigned int count);
		void AddScore(unsigned int amt);
		void AwardShipXP(float amount);
		void AddShipXP(float amount);
		void AwardScrap(unsigned int amount);
		bool TryPurchaseAbilityLevel(sas::AbilitySlot slot, std::string* failureReason = nullptr);
		void FlushPendingTestAbilityLevels(World* expectedWorld = nullptr);

		// Run-state module loadout; survives respawn. Mutations are validated by ModuleLoadout and
		// pushed to the live ship. Ships without a schematic have no modules.
		const ModuleLoadout& GetModuleLoadout() const { return mModuleLoadout; }
		std::uint32_t AddModuleToInventory(const sas::ContentId& moduleId, int variantIndex = 0, ModuleRarity rarity = ModuleRarity::Common);
		ModulePlacementFailure TryInstallModule(std::uint32_t instanceId, sf::Vector2i origin, int rotationQuarterTurns);
		ModulePlacementFailure TryMoveModule(std::uint32_t instanceId, sf::Vector2i origin, int rotationQuarterTurns);
		bool TryUninstallModule(std::uint32_t instanceId);

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
		using PurchasedAbilityLevels = Map<std::string, int>;
		struct AbilityPurchaseCommitContext
		{
			Player& player;
			unsigned int cost;
			PurchasedAbilityLevels::node_type purchasedLevel;
			bool committed = false;
		};

		static void CommitAbilityLevelPurchase(
			void* context,
			sas::AbilityHandle handle,
			int level
		) noexcept;
		void OnShipLevelChanged(int previousLevel, int currentLevel);
		bool QueuePendingTestAbilityLevels(PlayerSpaceShip& ship, int levelCount);
		void RestorePurchasedAbilityLevels(PlayerSpaceShip& ship);
		std::optional<ModuleRulesContext> BuildModuleRules() const;
		void ApplyModuleLoadout(PlayerSpaceShip& ship);
		void OnCurrentShipDestroyed(Actor* destroyedActor);
		void ResetRunProgression();

		weak_ptr<PlayerSpaceShip> mCurrentSpaceShip;
		std::string mShipId;
		DelegateHandle mCurrentShipDestroyedHandle;
		DelegateHandle mShipLevelChangedHandle;
		ShipProgression mShipProgression;
		PurchasedAbilityLevels mPurchasedAbilityLevels;
		PurchasedAbilityLevels mPendingTestAbilityLevels;
		ModuleLoadout mModuleLoadout;
		unsigned int mLifeCount;
		unsigned int mScore;
		unsigned int mScrap;
		int mPendingTestAbilityLevelUps = 0;
		bool mAbilityPurchaseInProgress = false;
	};
}


