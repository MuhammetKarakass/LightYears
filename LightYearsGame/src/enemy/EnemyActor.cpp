#include "gameConfigs/CollisionLayers.h"
#include "gameConfigs/SimulationTimeDomains.h"
#include "enemy/EnemyActor.h"
#include "framework/debug/Profiler.h"

#include "gameConfigs/GameplayTags.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/EnemyFactory.h"
#include "gameplay/enemy/EnemyIds.h"
#include "gameplay/tags/GameplayTags.h"
#include "framework/LightComponent.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include <framework/reflection/Reflection.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace vega
{
	VEGA_REFLECT(EnemyActor, SpaceShip)
	{
		VEGA_SPAWNABLE([](World& world) { return content::ConstructEnemy(world, EnemyIds::ApproachGunnerBasic).lock(); });
		VEGA_CATEGORY("Enemy");
		VEGA_PROPERTY("Enemy ID", GetEnemyId, SetEnemyId).Construction();
		VEGA_READONLY("Combat Profile ID", GetCombatProfileId);
		VEGA_READONLY("Enemy Level", GetEnemyLevel);
		VEGA_PROPERTY("Ship XP Reward", GetShipXPReward, SetShipXPReward).Range(0.f, 100000.f);
		VEGA_READONLY("Collision Damage", GetCollisionDamage);
	}

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

	EnemyActor::EnemyActor(World* owningWorld, const ShipDefinition& shipDefinition, const std::string& enemyId, const EnemyCombatProfile& combatProfile, const EnemyBehaviorProfile& behaviorProfile, float encounterDamageMultiplier, const EnemySpawnContext& spawnContext)
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
		mWindowCullEnabled{ spawnContext.windowCullEnabled },
		mHasChassisPrimaryWeapon{ !shipDefinition.primaryWeaponId.empty() }
	{
		SetSimulationTimeDomain(GameTimeDomains::HostileGameplay);
		SetMovementMode(ShipMovementMode::ThrustDrift);
		SetActorRotation(180.f);
		if (!shipDefinition.engineMounts.empty())
		{
			const EngineMount& mount = shipDefinition.engineMounts.front();
			mAttachedLightTags.push_back(GetOrAddComponent<LightComponent>().AddLight(GameTags::Ship::Engine_Main, mount.pointLightDef, mount.offset));
		}
	}

	void EnemyActor::SetEnemyId(const std::string& enemyId)
	{
		if (enemyId != mEnemyId) throw std::runtime_error("Enemy ID is construction-only; place a prefab with the desired catalog Enemy ID instead of changing an existing EnemyActor.");
	}

	void EnemyActor::ConfigureSpawnContext(float encounterDamageMultiplier, const EnemySpawnContext& spawnContext)
	{
		if (mSpawnContextLocked || GetIsPendingDestroy()) throw std::runtime_error("Enemy spawn context must be configured before BeginPlay.");
		if (spawnContext.level < 1 || !std::isfinite(encounterDamageMultiplier) || encounterDamageMultiplier < 0.f) throw std::runtime_error("Invalid enemy spawn context or encounter damage multiplier.");
		mEncounterDamageMultiplier = encounterDamageMultiplier;
		mSpawnContext = spawnContext;
		mWindowCullEnabled = spawnContext.windowCullEnabled;
	}

	EnemyActor::~EnemyActor()
	{
		mBehaviorRuntime.Clear();
		mEnemyRuntime.Clear();
	}

	void EnemyActor::BeginPlay()
	{
		mSpawnContextLocked = true;
		if (mHasChassisPrimaryWeapon)
		{
			VEGA_GAME_ERROR("EnemyActor '%s' chassis must not own a primary weapon.", mEnemyId.c_str());
			Destroy();
			return;
		}
		std::string combatFailureReason;
		if (!mEnemyRuntime.Initialize(mCombatProfile, mSpawnContext, mEncounterDamageMultiplier, &combatFailureReason))
		{
			VEGA_GAME_ERROR("EnemyActor '%s' failed to initialize combat profile: %s", mEnemyId.c_str(), combatFailureReason.c_str());
			Destroy();
			return;
		}
		const EnemyCombatProfile* profile = mEnemyRuntime.GetCurrentProfile();
		if (profile->allowContactDamageOnly && mCollisionDamage <= 0.f)
		{
			VEGA_GAME_ERROR("EnemyActor '%s' allows contact-only combat but has no collision damage.", mEnemyId.c_str());
			mEnemyRuntime.Clear();
			Destroy();
			return;
		}
		std::string behaviorFailureReason;
		if (!mBehaviorRuntime.Initialize(mBehaviorProfile, &behaviorFailureReason))
		{
			VEGA_GAME_ERROR("EnemyActor '%s' failed to initialize behavior: %s", mEnemyId.c_str(), behaviorFailureReason.c_str());
			mEnemyRuntime.Clear();
			Destroy();
			return;
		}
		SpaceShip::BeginPlay();
	}

	bool EnemyActor::IsOutsideCullView() const
	{
		// Measured from the camera view. The margin keeps at least the old window
		// check's slack (twice the width plus the full size and 10 px), so enemies
		// spawned just off-screen are never removed earlier than before.
		const sf::Vector2f size = GetActorGlobalBounds().size;
		return IsOutsideWorldView(size.x * 2.f + std::max(size.x, size.y) + 10.f);
	}

	void EnemyActor::Tick(float deltaTime)
	{
		ResetControlIntents();
		if (GetIsPendingDestroy()) return;
		if (mWindowCullEnabled && GetWorld() && GetWorld()->GetApplication() && IsOutsideCullView())
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
		if (mWindowCullEnabled && !GetIsPendingDestroy() && GetWorld() && GetWorld()->GetApplication() &&
			IsOutsideCullView()) Destroy();
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
		SetCollisionLayer(GameCollision::Enemy);
		SetCollisionMask(
			GameCollision::Player |
			GameCollision::FriendlySummon |
			GameCollision::PlayerBullet |
			GameCollision::RelayProjectile |
			GameCollision::Environment
		);
	}

	void EnemyActor::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);
		if (!otherActor || !mEnemyRuntime.IsReady() ||
			otherActor->GetCollisionLayer() != GameCollision::Player ||
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
		VEGA_PROFILE_SCOPE("Enemy.Blew");
		const bool wasReady = mEnemyRuntime.IsReady();
		mEnemyRuntime.Clear();
		if (!wasReady) return;
		SpawnReward();
		onScoreAwarded.Broadcast(mScoreAmt);
		onShipXPAwarded.Broadcast(mShipXPReward);
	}
}
