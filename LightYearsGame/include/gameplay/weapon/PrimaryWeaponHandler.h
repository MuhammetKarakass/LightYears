#pragma once

#include "framework/Core.h"
#include "gameConfigs/combat/WeaponStructs.h"
#include "gameplay/weapon/PrimaryWeaponMountProfile.h"
#include "gameplay/weapon/PrimaryWeaponValidationContract.h"

#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <functional>
#include <optional>

namespace vega
{
	class Actor;

	class PrimaryWeaponTypeRuntimeState
	{
	public:
		virtual ~PrimaryWeaponTypeRuntimeState() = default;
	};

	struct PrimaryWeaponMagazineRuntimeState
	{
		int roundsRemaining = 0;
		// Successful firing events since the last completed reload, independent of muzzle/pellet count.
		int shotsFired = 0;
		double reloadRemaining = 0.0;
	};

	class PrimaryWeaponHandler;
	class PrimaryWeaponFeatureHandler;

	struct PrimaryWeaponRuntimeState
	{
		const PrimaryWeaponHandler* handler = nullptr;
		List<const PrimaryWeaponFeatureHandler*> features;
		unique_ptr<PrimaryWeaponTypeRuntimeState> typeState;
		Map<sas::AttributeId, float> featureValues;
		PrimaryWeaponMagazineRuntimeState magazineState;
		std::optional<PrimaryWeaponMagazineDefinition> configuredMagazine;
		PrimaryWeaponCadenceMode configuredCadenceMode = PrimaryWeaponCadenceMode::AuthoredScaling;
		PrimaryWeaponDamageRoundingPolicy configuredDamageRoundingPolicy = PrimaryWeaponDamageRoundingPolicy::None;
		std::optional<PrimaryWeaponEmpoweredShotDefinition> configuredEmpoweredShot;
		std::string configuredWeaponId;
		bool isInitialized = false;
		bool isFiring = false;
		bool handlerEndFirePending = false;
		List<uint8_t> featureEndFirePending;
		uint64_t successfulFireCount = 0;
		// Empowered trigger state (see PrimaryWeaponEmpoweredTrigger).
		std::shared_ptr<PrimaryWeaponHitLedger> hitLedger;
		std::shared_ptr<PrimaryWeaponFrostMeterState> frostMeter;
		int empoweredCharges = 0;
		uint64_t observedAbilityActivationCount = 0;
		bool abilityActivationCountObserved = false;
		// Set by a feature (Frost Meter Whiteout) to force or scale the next shot.
		bool featureRequestsEmpoweredShot = false;
		float featureFireIntervalScale = 1.f;
		// Multiplier for the interval scheduled after the last successful shot.
		float nextFireIntervalScale = 1.f;
		float requestedCooldown = 0.f;
		float unprocessedSimulationTime = 0.f;
		float fireIntervalRemaining = 0.f;

		bool HasPendingEndFireCleanup() const
		{
			if (handlerEndFirePending) return true;
			for (const uint8_t pending : featureEndFirePending)
			{
				if (pending != 0) return true;
			}
			return false;
		}

		float GetFeatureValue(const sas::AttributeId& key, float fallback = 0.f) const;
		void SetFeatureValue(const sas::AttributeId& key, float value);
		void RequestCooldown(float duration);
		float ConsumeRequestedCooldown();
	};

	struct PrimaryWeaponExecutionContext
	{
		Actor& owner;
		const PrimaryWeaponDefinition& definition;
		const sas::GameplayAttributeList& attributes;
		List<GameplayTag> damageTags;
		const List<std::string>* abilityUpgradeIds = nullptr;
		PrimaryWeaponRuntimeState* runtime = nullptr;
		PrimaryWeaponShotMetadata shotMetadata;
		std::function<bool()> shouldContinue;
		// World position of the player's cursor. Empty for AI shooters and for
		// shots fired without a cursor, so handlers fall back to their own default.
		std::optional<sf::Vector2f> aimWorldPosition;
		// Module mount shape of this shot (Turret Mount, Broadside Mount); the default is the plain forward shot.
		PrimaryWeaponMountProfile mountProfile;

		bool ShouldContinue() const
		{
			return !shouldContinue || shouldContinue();
		}

		bool HasAbilityUpgrade(const std::string& upgradeId) const
		{
			if (!abilityUpgradeIds)
			{
				return false;
			}
			for (const std::string& unlockedUpgradeId : *abilityUpgradeIds)
			{
				if (unlockedUpgradeId == upgradeId)
				{
					return true;
				}
			}
			return false;
		}
	};

	// Base damage plus the empowered bonus when this shot is empowered.
	float ResolvePrimaryWeaponShotDamage(const PrimaryWeaponExecutionContext& context);

	class PrimaryWeaponHandler
	{
	public:
		virtual ~PrimaryWeaponHandler() = default;
		virtual PrimaryWeaponType GetType() const = 0;
		virtual unique_ptr<PrimaryWeaponTypeRuntimeState> CreateRuntimeState() const;
		bool UsesIntervalFire() const;
		virtual void BeginFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponTypeRuntimeState& state
		) const;
		virtual bool FireOnce(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponTypeRuntimeState& state
		) const = 0;
		virtual void TickFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponTypeRuntimeState& state,
			float deltaTime
		) const;
		virtual void EndFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponTypeRuntimeState& state
		) const;
	};

	class PrimaryWeaponFeatureHandler
	{
	public:
		virtual ~PrimaryWeaponFeatureHandler() = default;
		virtual PrimaryWeaponFeatureType GetFeatureType() const = 0;
		virtual const List<sas::AttributeId>& GetRuntimeValueKeys() const;
		virtual void BeginFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state
		) const;
		virtual bool CanFire(
			const PrimaryWeaponExecutionContext& context,
			const PrimaryWeaponRuntimeState& state
		) const;
		virtual void AfterFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state
		) const;
		virtual void TickFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state,
			float deltaTime
		) const;
		virtual void TickInactive(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state,
			float deltaTime
		) const;
		virtual void EndFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponRuntimeState& state
		) const;
	};
}
