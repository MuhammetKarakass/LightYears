#include "enemy/EnemyActor.h"

#include "gameplay/combat/Combatant.h"
#include "gameplay/tags/GameplayTags.h"
#include "framework/MathUtility.h"
#include "framework/World.h"

#include <algorithm>

namespace ly
{
	namespace
	{
		constexpr sas::AbilitySlot ControlledEnemySlots[]{ sas::AbilitySlot::PrimaryFire, sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };

		ShipDefinition MakeEnemyRuntimeShipDefinition(const ShipDefinition& source)
		{
			ShipDefinition result = source;
			result.primaryWeaponId.clear();
			return result;
		}
	}

	EnemyActor::EnemyActor(
		World* owningWorld,
		const ShipDefinition& shipDefinition,
		const std::string& enemyId,
		const EnemyCombatProfile& combatProfile,
		const EnemyBehaviorProfile& behaviorProfile,
		float encounterDamageMultiplier,
		const EnemySpawnContext& spawnContext)
		: SpaceShip{ owningWorld, MakeEnemyRuntimeShipDefinition(shipDefinition) },
		mEnemyRuntime{ GetCombatRuntime(), GetShipRuntime() },
		mCombatProfile{ combatProfile },
		mBehaviorProfile{ behaviorProfile },
		mEnemyId{ enemyId },
		mCombatProfileId{ combatProfile.profileId },
		mCollisionDamage{ shipDefinition.collisionDamage },
		mWeightedRewards{ shipDefinition.rewards },
		mScoreAmt{ shipDefinition.scoreAmt },
		mShipXPReward{ shipDefinition.shipXPReward },
		mEncounterDamageMultiplier{ encounterDamageMultiplier },
		mSpawnContext{ spawnContext },
		mHasChassisPrimaryWeapon{ !shipDefinition.primaryWeaponId.empty() }
	{
		SetSimulationTimeDomain(SimulationTimeDomain::HostileGameplay);
		SetMovementMode(ShipMovementMode::ThrustDrift);
		SetActorRotation(180.f);
		if (!shipDefinition.engineMounts.empty())
		{
			const EngineMount& mount = shipDefinition.engineMounts.front();
			mAttachedLightTags.push_back(AddLight(GameTags::Ship::Engine_Main, mount.pointLightDef, mount.offset));
		}
	}

	EnemyActor::~EnemyActor()
	{
		mBehaviorRuntime.Clear();
		mEnemyRuntime.Clear();
	}

	void EnemyActor::BeginPlay()
	{
		if (mHasChassisPrimaryWeapon)
		{
			LY_GAME_ERROR("EnemyActor '%s' chassis must not own a primary weapon.", mEnemyId.c_str());
			Destroy();
			return;
		}
		std::string combatFailureReason;
		if (!mEnemyRuntime.Initialize(mCombatProfile, mSpawnContext, mEncounterDamageMultiplier, &combatFailureReason))
		{
			LY_GAME_ERROR("EnemyActor '%s' failed to initialize combat profile: %s", mEnemyId.c_str(), combatFailureReason.c_str());
			Destroy();
			return;
		}
		const EnemyCombatProfile* profile = mEnemyRuntime.GetCurrentProfile();
		if (profile->allowContactDamageOnly && mCollisionDamage <= 0.f)
		{
			LY_GAME_ERROR("EnemyActor '%s' allows contact-only combat but has no collision damage.", mEnemyId.c_str());
			mEnemyRuntime.Clear();
			Destroy();
			return;
		}
		std::string behaviorFailureReason;
		if (!mBehaviorRuntime.Initialize(mBehaviorProfile, &behaviorFailureReason))
		{
			LY_GAME_ERROR("EnemyActor '%s' failed to initialize behavior: %s", mEnemyId.c_str(), behaviorFailureReason.c_str());
			mEnemyRuntime.Clear();
			Destroy();
			return;
		}
		SpaceShip::BeginPlay();
	}

	void EnemyActor::Tick(float deltaTime)
	{
		ResetControlIntents();
		if (GetIsPendingDestroy()) return;
		if (GetWorld() && GetWorld()->GetApplication() && IsActorOutOfWindow(GetActorGlobalBounds().size.x * 2.f))
		{
			Destroy();
			ResetControlIntents();
			return;
		}
		if (mEnemyRuntime.IsReady())
		{
			const EnemyBehaviorIntent intent = mBehaviorRuntime.Tick(*this, deltaTime);
			ApplyBehaviorIntent(intent, deltaTime);
		}
		if (GetIsPendingDestroy())
		{
			ResetControlIntents();
			return;
		}
		SpaceShip::Tick(deltaTime);
		if (!GetIsPendingDestroy() && GetWorld() && GetWorld()->GetApplication() &&
			IsActorOutOfWindow(GetActorGlobalBounds().size.x * 2.f)) Destroy();
		ResetControlIntents();
	}

	void EnemyActor::ResetControlIntents()
	{
		auto& abilities = GetAbilitySystemComponent();
		for (const sas::AbilitySlot slot : ControlledEnemySlots) abilities.SetAbilitySlotInput(slot, false);
	}

	void EnemyActor::ApplyBehaviorIntent(const EnemyBehaviorIntent& intent, float deltaTime)
	{
		const shared_ptr<Actor> target = intent.target.lock();
		if (GetIsPendingDestroy()) return;
		const EnemyBehaviorProfile* profile = mBehaviorRuntime.GetProfile();
		if (!profile) return;
		if (target && !target->GetIsPendingDestroy())
		{
			RotateTowardWorldLocation(intent.aimTargetLocation, std::max(0.f, deltaTime) * profile->aimTurnSpeed);
			const sf::Vector2f right = GetActorRightDirection();
			const sf::Vector2f forward = GetActorForwardDirection();
			const float localStrafe = intent.movementDirection.x * right.x + intent.movementDirection.y * right.y;
			const float localForward = intent.movementDirection.x * forward.x + intent.movementDirection.y * forward.y;
			GetMovementComponent().AddShipRelativeThrust({ localStrafe, localForward }, deltaTime);
		}
		for (const EnemySlotCommand& command : intent.slotCommands)
		{
			if (command.slot == sas::AbilitySlot::None ||
				(command.slot != sas::AbilitySlot::PrimaryFire && !sas::IsLoadoutAbilitySlot(command.slot)))
			{
				continue;
			}
			GetAbilitySystemComponent().SetAbilitySlotInput(command.slot, command.inputHeld);
		}
	}

	void EnemyActor::SetupCollisionLayers()
	{
		SetCollisionLayer(CollisionLayer::Enemy);
		SetCollisionMask(
			CollisionLayer::Player |
			CollisionLayer::FriendlySummon |
			CollisionLayer::PlayerBullet |
			CollisionLayer::RelayProjectile |
			CollisionLayer::Environment
		);
	}

	void EnemyActor::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);
		if (!otherActor || !mEnemyRuntime.IsReady() ||
			otherActor->GetCollisionLayer() != CollisionLayer::Player ||
			!CanApplyContactDamage(*this, *otherActor)) return;
		ApplyCombatDamage(*otherActor, mCollisionDamage, this);
	}

	void EnemyActor::SpawnReward()
	{
		if (mWeightedRewards.empty()) return;
		float totalWeight = 0.f;
		for (const WeightedReward& reward : mWeightedRewards) totalWeight += reward.weight;
		const float randomValue = RandRange(0.f, 1.f);
		if (randomValue > totalWeight) return;
		float currentWeight = 0.f;
		for (const WeightedReward& reward : mWeightedRewards)
		{
			currentWeight += reward.weight;
			if (randomValue <= currentWeight)
			{
				if (auto spawnedReward = reward.factory(GetWorld()).lock()) spawnedReward->SetActorLocation(GetActorLocation());
				return;
			}
		}
	}

	void EnemyActor::Blew()
	{
		const bool wasReady = mEnemyRuntime.IsReady();
		mEnemyRuntime.Clear();
		if (!wasReady) return;
		SpawnReward();
		onScoreAwarded.Broadcast(mScoreAmt);
		onShipXPAwarded.Broadcast(mShipXPReward);
	}
}
