#pragma once

#include "framework/Core.h"
#include "gameConfigs/combat/WeaponStructs.h"

namespace ly
{
	class Actor;

	struct PrimaryWeaponValidationResult
	{
		bool isValid = false;
		std::string reason;
	};

	class PrimaryWeaponTypeRuntimeState
	{
	public:
		virtual ~PrimaryWeaponTypeRuntimeState() = default;
	};

	class PrimaryWeaponHandler;
	class PrimaryWeaponFeatureHandler;

	struct PrimaryWeaponRuntimeState
	{
		const PrimaryWeaponHandler* handler = nullptr;
		List<const PrimaryWeaponFeatureHandler*> features;
		unique_ptr<PrimaryWeaponTypeRuntimeState> typeState;
		Map<GameplayTag, float> featureValues;
		std::string configuredWeaponId;
		bool isInitialized = false;
		bool isFiring = false;
		float requestedCooldown = 0.f;

		float GetFeatureValue(const GameplayTag& key, float fallback = 0.f) const;
		void SetFeatureValue(const GameplayTag& key, float value);
		void RequestCooldown(float duration);
		float ConsumeRequestedCooldown();
	};

	struct PrimaryWeaponExecutionContext
	{
		Actor& owner;
		const PrimaryWeaponDefinition& definition;
		const GameplayAttributeList& attributes;
		List<GameplayTag> damageTags;
		const List<GameplayTag>* abilityUpgradeIds = nullptr;
		PrimaryWeaponRuntimeState* runtime = nullptr;

		bool HasAbilityUpgrade(const GameplayTag& upgradeId) const
		{
			if (!abilityUpgradeIds)
			{
				return false;
			}
			for (const GameplayTag& unlockedUpgradeId : *abilityUpgradeIds)
			{
				if (unlockedUpgradeId.MatchesTag(upgradeId))
				{
					return true;
				}
			}
			return false;
		}
	};

	class PrimaryWeaponHandler
	{
	public:
		virtual ~PrimaryWeaponHandler() = default;
		virtual const GameplayTag& GetTypeTag() const = 0;
		virtual const List<GameplayTag>& GetOwnedAttributeRoots() const = 0;
		virtual const List<GameplayTag>& GetInheritedAttributeRoots() const;
		virtual PrimaryWeaponValidationResult ValidateDefinition(
			const PrimaryWeaponDefinition& definition
		) const;
		virtual unique_ptr<PrimaryWeaponTypeRuntimeState> CreateRuntimeState() const;
		virtual bool UsesIntervalFire() const { return true; }
		virtual void BeginFire(
			const PrimaryWeaponExecutionContext& context,
			PrimaryWeaponTypeRuntimeState& state
		) const;
		virtual void FireOnce(
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
		virtual const GameplayTag& GetFeatureTag() const = 0;
		virtual const List<GameplayTag>& GetAttributeRoots() const = 0;
		virtual const List<GameplayTag>& GetRuntimeValueKeys() const;
		virtual PrimaryWeaponValidationResult ValidateDefinition(
			const PrimaryWeaponDefinition& definition
		) const;
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
