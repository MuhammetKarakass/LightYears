#include "player/Player.h"
#include "player/PlayerSpaceShip.h"
#include "gameplay/content/ShipContentCatalog.h"
#include <framework/World.h>
#include <algorithm>
#include <exception>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ly
{
	namespace
	{
		constexpr bool kAutoLevelAbilitiesForTesting = true;
	}

	Player::Player():
		mLifeCount{3},
		mScore{ 0 },
		mScrap{ 0 },
		mCurrentSpaceShip{}
	{
		mShipLevelChangedHandle = mShipProgression.onLevelChanged.BindAction(this, &Player::OnShipLevelChanged);
	}

	Player::~Player()
	{
		mShipProgression.onLevelChanged.UnbindAction(mShipLevelChangedHandle);
		if (const shared_ptr<PlayerSpaceShip> currentShip = mCurrentSpaceShip.lock())
		{
			currentShip->onActorDestroyed.UnbindAction(mCurrentShipDestroyedHandle);
		}
	}
	
	weak_ptr<PlayerSpaceShip> Player::SpawnSpaceShip(World* world) {
		if (const shared_ptr<PlayerSpaceShip> currentShip = mCurrentSpaceShip.lock())
		{
			currentShip->onActorDestroyed.UnbindAction(mCurrentShipDestroyedHandle);
		}
		mCurrentShipDestroyedHandle.Reset();
		if (mLifeCount > 0)
		{
			--mLifeCount;
			mShipProgression.ForgetDestroyedAttributes();
			auto windowSize = world->GetWindowSize();
			mCurrentSpaceShip = world->SpawnActor<PlayerSpaceShip>();

			auto ship = mCurrentSpaceShip.lock();
			if (!ship)
			{
				mCurrentSpaceShip = weak_ptr<PlayerSpaceShip>{};
				onLifeExhausted.Broadcast();
				return weak_ptr<PlayerSpaceShip>{};
			}

			if (!mShipProgression.IsConfigured())
			{
				mShipProgression.Configure(
					content::ShipContentCatalog::GetPlayerFighterDefinition().progressionDefinition
				);
			}
			mShipProgression.BindAttributes(ship->GetAbilitySystemComponent().GetAttributes());
			RestorePurchasedAbilityLevels(*ship);
			if (mPendingTestAbilityLevelUps > 0 &&
				QueuePendingTestAbilityLevels(*ship, mPendingTestAbilityLevelUps))
			{
				mPendingTestAbilityLevelUps = 0;
			}
			mCurrentShipDestroyedHandle = ship->onActorDestroyed.BindAction(
				this,
				&Player::OnCurrentShipDestroyed
			);

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

		sas::AbilitySystemComponent& abilities = ship.GetAbilitySystemComponent();
		const auto snapshots = abilities.BuildAbilitySnapshots();
		std::unordered_set<std::string> processedAbilityIds;
		bool foundAbility = false;
		for (const sas::AbilityRuntimeSnapshot& snapshot : snapshots)
		{
			GameAbility* ability = abilities.FindAbility<GameAbility>(snapshot.handle);
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

		sas::AbilitySystemComponent& abilities = ship->GetAbilitySystemComponent();
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
			GameAbility* ability = abilities.FindAbilityById<GameAbility>(pending.first);
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

			const GameAbility* updatedAbility =
				abilities.FindAbilityById<GameAbility>(pending.first);
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

		sas::AbilitySystemComponent& abilities =
			ship->GetAbilitySystemComponent();
		GameAbility* ability =
			abilities.FindAbility<GameAbility>(slot);
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
		sas::AbilitySystemComponent& abilities =
			ship.GetAbilitySystemComponent();
		for (auto& purchasedLevel : mPurchasedAbilityLevels)
		{
			GameAbility* ability =
				abilities.FindAbilityById<GameAbility>(
					purchasedLevel.first
				);
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

	void Player::ResetRunProgression()
	{
		mShipProgression.ResetForNewRun();
		mPurchasedAbilityLevels.clear();
		mPendingTestAbilityLevels.clear();
		mPendingTestAbilityLevelUps = 0;
		if (mScrap != 0)
		{
			mScrap = 0;
			onScrapChange.Broadcast(mScrap);
		}
	}

}


