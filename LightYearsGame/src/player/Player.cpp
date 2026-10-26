#include "player/Player.h"
#include "player/PlayerSpaceShip.h"
#include "gameplay/content/ModuleContentCatalog.h"
#include "gameplay/content/ModuleSideFeatureCatalog.h"
#include "gameplay/content/ShipContentCatalog.h"
#include "gameplay/content/ShipPrefabSpawner.h"
#include "gameplay/tags/GameplayTags.h"
#include <framework/World.h>
#include <algorithm>
#include <exception>
#include <unordered_set>
#include <utility>
#include <vector>

namespace vega
{
	namespace
	{
		constexpr bool kAutoLevelAbilitiesForTesting = true;
		// Temporary playtest switch: the first spawn of a run puts one instance of every
		// size variant of every non-default module into the inventory. Nothing is installed.
		// Set to false (or delete) once modules drop from gameplay sources.
		constexpr bool kGrantAllModulesForTesting = true;

		bool HasInstalledLifeSupport(const ModuleLoadout& loadout)
		{
			for (const ModulePlacement& placement : loadout.GetPlacements())
			{
				const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(placement.moduleId);
				if (definition && definition->systemCategory == GameplayTags::Module::System::LifeSupport)
				{
					return true;
				}
			}
			return false;
		}
	}

	Player::Player(std::string shipId):
		mCurrentSpaceShip{},
		mShipId{ std::move(shipId) },
		mLifeCount{3},
		mScore{ 0 },
		mScrap{ 0 }
	{
		mShipLevelChangedHandle = mShipProgression.onLevelChanged.BindAction(this, &Player::OnShipLevelChanged);
	}

	Player::~Player()
	{
		mModuleCreditLifetime.reset();
		mShipProgression.onLevelChanged.UnbindAction(mShipLevelChangedHandle);
		if (const shared_ptr<PlayerSpaceShip> currentShip = mCurrentSpaceShip.lock()) currentShip->onRunDistanceTraveled.UnbindAction(mRunDistanceHandle);
		if (const shared_ptr<PlayerSpaceShip> currentShip = mCurrentSpaceShip.lock())
		{
			currentShip->onActorDestroyed.UnbindAction(mCurrentShipDestroyedHandle);
		}
	}
	
	weak_ptr<PlayerSpaceShip> Player::SpawnSpaceShip(World* world) {
		if (mModuleLoadout.GetRevision() > 0 && !HasInstalledLifeSupport(mModuleLoadout))
		{
			return weak_ptr<PlayerSpaceShip>{};
		}
		if (const shared_ptr<PlayerSpaceShip> currentShip = mCurrentSpaceShip.lock())
		{
			currentShip->onActorDestroyed.UnbindAction(mCurrentShipDestroyedHandle);
			currentShip->onRunDistanceTraveled.UnbindAction(mRunDistanceHandle);
		}
		mCurrentShipDestroyedHandle.Reset();
		mRunDistanceHandle.Reset();
		if (mLifeCount > 0)
		{
			--mLifeCount;
			mShipProgression.ForgetDestroyedAttributes();
			auto windowSize = world->GetWindowSize();
			const auto& shipDefinition = content::ShipContentCatalog::GetPlayerShipDefinition(mShipId);
			if (shipDefinition.prefabPath)
			{
				std::string reason;
				mCurrentSpaceShip = SpawnShipPrefab(*world, *shipDefinition.prefabPath, shipDefinition.catalogId, &reason);
				if (mCurrentSpaceShip.expired()) VEGA_GAME_ERROR("Player ship prefab failed: %s", reason.c_str());
			}
			else mCurrentSpaceShip = world->SpawnActor<PlayerSpaceShip>(shipDefinition);

			auto ship = mCurrentSpaceShip.lock();
			if (!ship)
			{
				mCurrentSpaceShip = weak_ptr<PlayerSpaceShip>{};
				onLifeExhausted.Broadcast();
				return weak_ptr<PlayerSpaceShip>{};
			}

			if (!mShipProgression.IsConfigured())
			{
				mShipProgression.Configure(shipDefinition.progressionDefinition);
			}
			mShipProgression.BindAttributes(ship->GetAbilitySystemComponent().GetAttributes());
			ship->GetShipRuntime().SetShipLevel(mShipProgression.GetLevel());
			RestorePurchasedAbilityLevels(*ship);
			ApplyModuleLoadout(*ship);
			if (mPendingTestAbilityLevelUps > 0 &&
				QueuePendingTestAbilityLevels(*ship, mPendingTestAbilityLevelUps))
			{
				mPendingTestAbilityLevelUps = 0;
			}
			mCurrentShipDestroyedHandle = ship->onActorDestroyed.BindAction(
				this,
				&Player::OnCurrentShipDestroyed
			);
			mRunDistanceHandle = ship->onRunDistanceTraveled.BindAction(this, &Player::OnRunDistanceTraveled);

			ship->SetActorLocation(sf::Vector2f{ windowSize.x / 2.0f, windowSize.y - 100.0f });

			onLifeChange.Broadcast(mLifeCount);
			return mCurrentSpaceShip;
		}
		else
		{
			mCurrentSpaceShip = weak_ptr<PlayerSpaceShip>{};
			ResetRunProgression();
			onLifeExhausted.Broadcast();
			return weak_ptr<PlayerSpaceShip>{};
		}
	}

	void Player::AddLifeCount(unsigned int count)
	{
		if(count<=0)
			return;
		mLifeCount += count;
		onLifeChange.Broadcast(mLifeCount);
	}

	void Player::AddScore(unsigned int amt)
	{
		if (amt <= 0)
			return;
		mScore += amt;
		onScoreChange.Broadcast(mScore);
	}

	void Player::AwardShipXP(float amount)
	{
		mShipProgression.AddXP(amount);
	}

	void Player::OnShipLevelChanged(int previousLevel, int currentLevel)
	{
		if (const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock();
			ship && !ship->GetIsPendingDestroy())
		{
			ship->GetShipRuntime().SetShipLevel(currentLevel);
			ship->GetModuleApplier().OnShipLevelChanged();
		}

		if (!kAutoLevelAbilitiesForTesting)
		{
			return;
		}

		const int levelCount = currentLevel - previousLevel;
		if (levelCount <= 0)
		{
			return;
		}

		const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock();
		if (!ship || ship->GetIsPendingDestroy() ||
			!QueuePendingTestAbilityLevels(*ship, levelCount))
		{
			mPendingTestAbilityLevelUps += levelCount;
		}
	}

	bool Player::QueuePendingTestAbilityLevels(PlayerSpaceShip& ship, int levelCount)
	{
		if (levelCount <= 0)
		{
			return false;
		}

		auto& abilities = ship.GetAbilitySystemComponent();
		const auto snapshots = abilities.BuildAbilitySnapshots();
		std::unordered_set<std::string> processedAbilityIds;
		bool foundAbility = false;
		for (const sas::AbilityRuntimeSnapshot& snapshot : snapshots)
		{
			GameAbility* ability = abilities.GetAbility(snapshot.handle);
			if (!ability)
			{
				continue;
			}
			const std::string& abilityId = snapshot.abilityId;
			if (abilityId.empty() || !processedAbilityIds.insert(abilityId).second)
			{
				continue;
			}
			foundAbility = true;

			int baseLevel = ability->GetLevel();
			if (const auto purchased = mPurchasedAbilityLevels.find(abilityId);
				purchased != mPurchasedAbilityLevels.end())
			{
				baseLevel = std::max(baseLevel, purchased->second);
			}
			if (const auto pending = mPendingTestAbilityLevels.find(abilityId);
				pending != mPendingTestAbilityLevels.end())
			{
				baseLevel = std::max(baseLevel, pending->second);
			}

			const int targetLevel = static_cast<int>(std::min<long long>(
				ability->GetMaxLevel(),
				static_cast<long long>(baseLevel) + levelCount
			));
			mPurchasedAbilityLevels[abilityId] = targetLevel;
			if (targetLevel > ability->GetLevel())
			{
				mPendingTestAbilityLevels[abilityId] = targetLevel;
			}
			else
			{
				mPendingTestAbilityLevels.erase(abilityId);
			}
		}
		return foundAbility;
	}

	void Player::FlushPendingTestAbilityLevels(World* expectedWorld)
	{
		if (!kAutoLevelAbilitiesForTesting)
		{
			return;
		}

		const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock();
		if (!ship || ship->GetIsPendingDestroy() ||
			(expectedWorld && ship->GetWorld() != expectedWorld))
		{
			return;
		}

		auto& abilities = ship->GetAbilitySystemComponent();
		if (abilities.IsExecutingAbilityInstanceOperation())
		{
			return;
		}

		if (mPendingTestAbilityLevelUps > 0 &&
			QueuePendingTestAbilityLevels(*ship, mPendingTestAbilityLevelUps))
		{
			mPendingTestAbilityLevelUps = 0;
		}

		std::vector<std::pair<std::string, int>> pendingTargets;
		pendingTargets.reserve(mPendingTestAbilityLevels.size());
		for (const auto& pending : mPendingTestAbilityLevels)
		{
			pendingTargets.emplace_back(pending.first, pending.second);
		}

		std::exception_ptr operationError;
		for (const auto& pending : pendingTargets)
		{
			GameAbility* ability = abilities.GetAbilityById(pending.first);
			if (!ability)
			{
				continue;
			}

			const sas::AbilityHandle handle = ability->GetHandle();
			const int targetLevel = std::min(
				std::max(pending.second, ability->GetLevel()),
				ability->GetMaxLevel()
			);
			mPurchasedAbilityLevels[pending.first] = targetLevel;
			if (ability->GetLevel() < targetLevel)
			{
				try
				{
					(void)abilities.SetAbilityLevel(handle, targetLevel);
				}
				catch (...)
				{
					if (!operationError)
					{
						operationError = std::current_exception();
					}
				}
			}

			const GameAbility* updatedAbility = abilities.GetAbilityById(pending.first);
			const auto pendingTarget = mPendingTestAbilityLevels.find(pending.first);
			if (updatedAbility && pendingTarget != mPendingTestAbilityLevels.end() &&
				updatedAbility->GetLevel() >= pendingTarget->second)
			{
				mPurchasedAbilityLevels[pending.first] = updatedAbility->GetLevel();
				mPendingTestAbilityLevels.erase(pendingTarget);
			}
		}

		if (operationError)
		{
			std::rethrow_exception(operationError);
		}
	}

	void Player::AddShipXP(float amount)
	{
		AwardShipXP(amount);
	}

	void Player::AwardScrap(unsigned int amount)
	{
		if (amount == 0)
		{
			return;
		}

		mScrap += amount;
		onScrapChange.Broadcast(mScrap);
	}

	bool Player::TryPurchaseAbilityLevel(sas::AbilitySlot slot, std::string* failureReason)
	{
		return TryPurchaseAbilityLevel(slot, false, failureReason);
	}

	bool Player::TryPurchaseAbilityLevel(sas::AbilitySlot slot, bool second, std::string* failureReason)
	{
		if (mAbilityPurchaseInProgress)
		{
			if (failureReason)
			{
				*failureReason = "Another ability purchase is already in progress.";
			}
			return false;
		}
		mAbilityPurchaseInProgress = true;
		struct PurchaseScope
		{
			bool& inProgress;
			~PurchaseScope() { inProgress = false; }
		} purchaseScope{ mAbilityPurchaseInProgress };

		const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock();
		if (!ship || ship->GetIsPendingDestroy())
		{
			if (failureReason)
			{
				*failureReason = "No active ship is available for an upgrade.";
			}
			return false;
		}

		auto& abilities = ship->GetAbilitySystemComponent();
		GameAbility* ability = second ? (abilities.IsTwinRackEnabled() ? abilities.GetSecondAbility(slot) : nullptr) : abilities.GetAbility(slot);
		if (!ability)
		{
			if (failureReason)
			{
				*failureReason = "No ability is assigned to this slot.";
			}
			return false;
		}

		const int targetLevel = ability->GetLevel() + 1;
		if (targetLevel > ability->GetMaxLevel())
		{
			if (failureReason)
			{
				*failureReason = "Ability is already at maximum level.";
			}
			return false;
		}

		const GameAbilityDefinition& definition = ability->GetDefinition();
		if (ship->GetModuleApplier().IsScrapUpgradeLocked(definition.abilityTags))
		{
			if (failureReason)
			{
				*failureReason = "Field Mastery levels Utility abilities through use; Scrap upgrades are locked.";
			}
			return false;
		}
		if (!definition.HasScrapCostToReachLevel(targetLevel))
		{
			if (failureReason)
			{
				*failureReason = "This ability has no configured scrap upgrade cost.";
			}
			return false;
		}

		const unsigned int cost = definition.GetScrapCostToReachLevel(targetLevel);
		if (mScrap < cost)
		{
			if (failureReason)
			{
				*failureReason = "Not enough scrap for this upgrade.";
			}
			return false;
		}

		const std::string abilityId = definition.abilityId;
		const sas::AbilityHandle abilityHandle = ability->GetHandle();
		PurchasedAbilityLevels stagedPurchasedLevels;
		const auto stagedLevel = stagedPurchasedLevels.emplace(abilityId, targetLevel);
		AbilityPurchaseCommitContext commit{
			*this,
			cost,
			stagedPurchasedLevels.extract(stagedLevel.first),
			false
		};

		std::exception_ptr operationError;
		try
		{
			(void)abilities.SetAbilityLevel(
				abilityHandle,
				targetLevel,
				&Player::CommitAbilityLevelPurchase,
				&commit
			);
		}
		catch (...)
		{
			operationError = std::current_exception();
		}

		if (commit.committed)
		{
			try
			{
				onScrapChange.Broadcast(mScrap);
			}
			catch (...)
			{
				if (!operationError)
				{
					operationError = std::current_exception();
				}
			}
		}
		if (operationError)
		{
			std::rethrow_exception(operationError);
		}
		if (!commit.committed)
		{
			if (failureReason)
			{
				*failureReason = "Ability level could not be increased.";
			}
			return false;
		}
		return true;
	}

	void Player::CommitAbilityLevelPurchase(
		void* context,
		sas::AbilityHandle handle,
		int level
	) noexcept
	{
		(void)handle;
		AbilityPurchaseCommitContext& commit =
			*static_cast<AbilityPurchaseCommitContext*>(context);
		commit.purchasedLevel.mapped() = level;
		const auto insertion = commit.player.mPurchasedAbilityLevels.insert(
			std::move(commit.purchasedLevel)
		);
		if (!insertion.inserted)
		{
			insertion.position->second = level;
		}
		commit.player.mScrap -= commit.cost;
		commit.committed = true;
	}
	
	void Player::OnScoreAwarded(unsigned int scoreAmount)
	{
		AddScore(scoreAmount);
	}

	void Player::RestorePurchasedAbilityLevels(PlayerSpaceShip& ship)
	{
		auto& abilities = ship.GetAbilitySystemComponent();
		for (auto& purchasedLevel : mPurchasedAbilityLevels)
		{
			GameAbility* ability = abilities.GetAbilityById(purchasedLevel.first);
			if (!ability)
			{
				continue;
			}

			abilities.SetAbilityLevel(
				ability->GetHandle(),
				purchasedLevel.second
			);
			purchasedLevel.second = ability->GetLevel();
		}
	}

	std::optional<ModuleRulesContext> Player::BuildModuleRules() const
	{
		const ShipDefinition& definition = content::ShipContentCatalog::GetPlayerShipDefinition(mShipId);
		if (!definition.schematic) return std::nullopt;
		return ModuleRulesContext{
			*definition.schematic,
			[](const sas::ContentId& id) { return content::ModuleContentCatalog::FindById(id); },
			content::ModuleContentCatalog::GetZoneTable(),
			[](const sas::ContentId& id) { return content::ModuleSideFeatureCatalog::FindById(id); }
		};
	}

	void Player::ApplyModuleLoadout(PlayerSpaceShip& ship)
	{
		ship.GetModuleApplier().SetGoldWallet(mGold);
		ship.GetModuleApplier().GetEpicUtilityMovementRuntime().SetFieldMasteryRunState(mFieldMasteryRunState);
		const std::weak_ptr<bool> lifetime = mModuleCreditLifetime;
		ship.GetModuleApplier().SetRunCounterCreditSink([this, lifetime](std::uint32_t instanceId, double units) { if (!lifetime.expired()) mModuleLoadout.AddRunCounterCredit(instanceId, units); });
		ship.GetModuleApplier().SetKillCreditSink([this, lifetime](std::uint32_t instanceId, double stacks, double damageBonus) { if (!lifetime.expired()) mModuleLoadout.AddKillCredit(instanceId, stacks, damageBonus); });
		ship.GetModuleApplier().SetReviveSpentSink([this, lifetime](std::uint32_t instanceId) { if (!lifetime.expired()) mModuleLoadout.AddReviveChargeSpent(instanceId); });
		ship.GetModuleApplier().SetAbilityLevelSink([this, lifetime](const std::string& abilityId, int level) { if (lifetime.expired()) return; const auto found = mPurchasedAbilityLevels.find(abilityId); if (found != mPurchasedAbilityLevels.end()) found->second = level; else mPurchasedAbilityLevels.emplace(abilityId, level); });
		const std::optional<ModuleRulesContext> rules = BuildModuleRules();
		if (!rules) return;
		if (mModuleLoadout.GetRevision() == 0 && !mModuleLoadout.SeedDefaults(*rules))
		{
			VEGA_GAME_ERROR("Default modules for '%s' are invalid; the ship starts without modules.", mShipId.c_str());
		}
		if (kGrantAllModulesForTesting && !mTestModulesGranted)
		{
			mTestModulesGranted = true;
			std::unordered_set<std::string> defaultIds;
			for (const DefaultModulePlacement& placement : rules->schematic.defaultModules)
			{
				defaultIds.insert(placement.moduleId.ToString());
			}
			for (const ModuleDefinition& definition : content::ModuleContentCatalog::GetDefinitions())
			{
				if (defaultIds.count(definition.moduleId.ToString()) == 0)
				{
					for (std::size_t variant = 0; variant < definition.sizeVariants.size(); ++variant)
					{
						if (definition.abilityModule)
						{
							for (int slot = 0; slot < 4; ++slot) mModuleLoadout.AddToInventory(*rules, definition, static_cast<int>(variant), definition.rarity, 1);
							continue;
						}
						if (definition.rarity == ModuleRarity::Common)
						{
							mModuleLoadout.AddToInventory(definition.moduleId, static_cast<int>(variant));
							continue;
						}
						std::vector<sas::ContentId> sideFeatureIds = definition.sideFeatures;
						for (const ModuleSharedPoolPermission& permission : definition.sharedPools)
							for (const sas::ContentId& featureId : permission.allowedFeatureIds)
								if (std::find(sideFeatureIds.begin(), sideFeatureIds.end(), featureId) == sideFeatureIds.end()) sideFeatureIds.push_back(featureId);
						mModuleLoadout.AddToInventory(*rules, definition, static_cast<int>(variant), definition.rarity, 1);
						for (const sas::ContentId& featureId : sideFeatureIds)
							mModuleLoadout.AddToInventory(*rules, definition, static_cast<int>(variant), definition.rarity, 1, featureId);
					}
				}
			}
		}
		ship.GetModuleApplier().Apply(mModuleLoadout, *rules, ship.IsInCombat());
	}

	std::uint32_t Player::AddModuleToInventory(const sas::ContentId& moduleId, int variantIndex, ModuleRarity rarity, int level, sas::ContentId sideFeatureId)
	{
		const std::optional<ModuleRulesContext> rules = BuildModuleRules();
		const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(moduleId);
		if (!rules || !definition) return 0;
		return mModuleLoadout.AddToInventory(*rules, *definition, variantIndex, rarity, level, std::move(sideFeatureId));
	}

	bool Player::SetModuleLevel(std::uint32_t instanceId, int level)
	{
		const std::optional<ModuleRulesContext> rules = BuildModuleRules();
		if (!rules || !mModuleLoadout.SetLevel(*rules, instanceId, level)) return false;
		if (const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock()) ApplyModuleLoadout(*ship);
		return true;
	}

	bool Player::SetModuleSideFeature(std::uint32_t instanceId, const sas::ContentId& sideFeatureId)
	{
		const std::optional<ModuleRulesContext> rules = BuildModuleRules();
		if (!rules || !mModuleLoadout.SetSideFeature(*rules, instanceId, sideFeatureId)) return false;
		if (const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock()) ApplyModuleLoadout(*ship);
		return true;
	}

	ModulePlacementFailure Player::TryInstallModule(std::uint32_t instanceId, sf::Vector2i origin, int rotationQuarterTurns)
	{
		const std::optional<ModuleRulesContext> rules = BuildModuleRules();
		if (!rules) return ModulePlacementFailure::OutOfBounds;
		const ModulePlacementFailure failure = mModuleLoadout.Install(*rules, instanceId, origin, rotationQuarterTurns);
		if (failure == ModulePlacementFailure::None)
		{
			if (const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock()) ApplyModuleLoadout(*ship);
		}
		return failure;
	}

	ModulePlacementFailure Player::TryMoveModule(std::uint32_t instanceId, sf::Vector2i origin, int rotationQuarterTurns)
	{
		const std::optional<ModuleRulesContext> rules = BuildModuleRules();
		if (!rules) return ModulePlacementFailure::OutOfBounds;
		const ModulePlacementFailure failure = mModuleLoadout.Move(*rules, instanceId, origin, rotationQuarterTurns);
		if (failure == ModulePlacementFailure::None)
		{
			if (const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock()) ApplyModuleLoadout(*ship);
		}
		return failure;
	}

	ModulePlacementFailure Player::TryInstallModuleFragments(std::uint32_t instanceId, const List<sf::Vector2i>& cells)
	{
		const std::optional<ModuleRulesContext> rules = BuildModuleRules();
		if (!rules) return ModulePlacementFailure::OutOfBounds;
		const ModulePlacementFailure failure = mModuleLoadout.InstallFragmented(*rules, instanceId, cells);
		if (failure == ModulePlacementFailure::None)
		{
			if (const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock()) ApplyModuleLoadout(*ship);
		}
		return failure;
	}

	ModulePlacementFailure Player::TryMoveModuleFragments(std::uint32_t instanceId, const List<sf::Vector2i>& cells)
	{
		const std::optional<ModuleRulesContext> rules = BuildModuleRules();
		if (!rules) return ModulePlacementFailure::OutOfBounds;
		const ModulePlacementFailure failure = mModuleLoadout.MoveFragments(*rules, instanceId, cells);
		if (failure == ModulePlacementFailure::None)
		{
			if (const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock()) ApplyModuleLoadout(*ship);
		}
		return failure;
	}

	bool Player::TryUninstallModule(std::uint32_t instanceId)
	{
		bool removesLifeSupport = false;
		if (const ModulePlacement* placement = mModuleLoadout.FindPlacement(instanceId))
		{
			const ModuleDefinition* definition = content::ModuleContentCatalog::FindById(placement->moduleId);
			removesLifeSupport = definition &&
				definition->systemCategory == GameplayTags::Module::System::LifeSupport;
		}
		if (!mModuleLoadout.Uninstall(instanceId)) return false;
		if (const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock())
		{
			ApplyModuleLoadout(*ship);
			if (removesLifeSupport) ship->Destroy();
		}
		return true;
	}

	void Player::OnCurrentShipDestroyed(Actor* destroyedActor)
	{
		const shared_ptr<PlayerSpaceShip> currentShip = mCurrentSpaceShip.lock();
		if (!currentShip || currentShip.get() != destroyedActor)
		{
			return;
		}

		destroyedActor->onActorDestroyed.UnbindAction(mCurrentShipDestroyedHandle);
		mCurrentShipDestroyedHandle.Reset();
		mShipProgression.ForgetDestroyedAttributes();
		mCurrentSpaceShip = weak_ptr<PlayerSpaceShip>{};
	}

	void Player::OnRunDistanceTraveled(float distance)
	{
		const shared_ptr<PlayerSpaceShip> ship = mCurrentSpaceShip.lock();
		if (!ship) return;
		const std::optional<ModuleRulesContext> rules = BuildModuleRules();
		if (!rules || !mModuleLoadout.AccrueRunCounters(*rules, distance)) return;
		ApplyModuleLoadout(*ship);
	}

	void Player::ResetRunProgression()
	{
		mGold = std::make_shared<RunGold>();
		mShipProgression.ResetForNewRun();
		mModuleLoadout = ModuleLoadout{};
		mTestModulesGranted = false;
		mPurchasedAbilityLevels.clear();
		mFieldMasteryRunState = std::make_shared<FieldMasteryRunState>();
		mPendingTestAbilityLevels.clear();
		mPendingTestAbilityLevelUps = 0;
		if (mScrap != 0)
		{
			mScrap = 0;
			onScrapChange.Broadcast(mScrap);
		}
	}

}


