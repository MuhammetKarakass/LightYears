#pragma once

#include <framework/Object.h>
#include <framework/Delegate.h>
#include "gameplay/progression/ShipProgression.h"
#include "gameplay/progression/RunGold.h"
#include "abilities/AbilityPolicies.h"
#include "abilities/AbilityHandle.h"
#include "gameplay/module/ModuleLoadout.h"
#include <optional>
#include <string>

namespace vega
{
	class Actor;
	class PlayerSpaceShip;
	class World;

	class Player : public Object
	{
	public:
		explicit Player(std::string shipId = DefaultPlayerShipId);
		~Player() override;

		weak_ptr<PlayerSpaceShip> SpawnSpaceShip(World* world);
		const std::string& GetShipId() const { return mShipId; }
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
		std::uint32_t AddModuleToInventory(const sas::ContentId& moduleId, int variantIndex = 0, ModuleRarity rarity = ModuleRarity::Common, int level = 1, sas::ContentId sideFeatureId = {});
		bool SetModuleLevel(std::uint32_t instanceId, int level);
		bool SetModuleSideFeature(std::uint32_t instanceId, const sas::ContentId& sideFeatureId);
		ModulePlacementFailure TryInstallModule(std::uint32_t instanceId, sf::Vector2i origin, int rotationQuarterTurns);
		ModulePlacementFailure TryMoveModule(std::uint32_t instanceId, sf::Vector2i origin, int rotationQuarterTurns);
		// F1 Parçalama: install or move a fragmented placement, one cell per fragment.
		ModulePlacementFailure TryInstallModuleFragments(std::uint32_t instanceId, const List<sf::Vector2i>& cells);
		ModulePlacementFailure TryMoveModuleFragments(std::uint32_t instanceId, const List<sf::Vector2i>& cells);
		bool TryUninstallModule(std::uint32_t instanceId);
		// Read-only rules for the ship's schematic; empty for ships without one.
		std::optional<ModuleRulesContext> BuildModuleRules() const;

		unsigned int GetLifeCount() const { return mLifeCount; };
		unsigned int GetScore() const { return mScore; };
		unsigned int GetScrap() const { return mScrap; };
		unsigned int GetGold() const { return mGold->GetAmount(); }
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
		void ApplyModuleLoadout(PlayerSpaceShip& ship);
		void OnCurrentShipDestroyed(Actor* destroyedActor);
		void OnRunDistanceTraveled(float distance);
		void ResetRunProgression();

		weak_ptr<PlayerSpaceShip> mCurrentSpaceShip;
		std::string mShipId;
		DelegateHandle mCurrentShipDestroyedHandle;
		DelegateHandle mRunDistanceHandle;
		DelegateHandle mShipLevelChangedHandle;
		ShipProgression mShipProgression;
		PurchasedAbilityLevels mPurchasedAbilityLevels;
		PurchasedAbilityLevels mPendingTestAbilityLevels;
		ModuleLoadout mModuleLoadout;
		// Players live by value; callback validity must not depend on shared Object ownership.
		std::shared_ptr<bool> mModuleCreditLifetime = std::make_shared<bool>(true);
		unsigned int mLifeCount;
		unsigned int mScore;
		unsigned int mScrap;
		std::shared_ptr<RunGold> mGold = std::make_shared<RunGold>();
		int mPendingTestAbilityLevelUps = 0;
		bool mTestModulesGranted = false;
		bool mAbilityPurchaseInProgress = false;
	};
}
