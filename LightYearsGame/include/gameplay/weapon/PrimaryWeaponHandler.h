#pragma once

#include "attributes/AttributeSystem.h"

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
		Map<sas::AttributeId, float> featureValues;
		std::string configuredWeaponId;
		bool isInitialized = false;
		bool isFiring = false;
		float requestedCooldown = 0.f;

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

	class PrimaryWeaponHandler
	{
	public:
		virtual ~PrimaryWeaponHandler() = default;
		virtual PrimaryWeaponType GetType() const = 0;
		virtual const List<sas::AttributeId>& GetOwnedAttributeRoots() const = 0;
		virtual const List<sas::AttributeId>& GetInheritedAttributeRoots() const;
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
		virtual PrimaryWeaponFeatureType GetFeatureType() const = 0;
		virtual const List<sas::AttributeId>& GetAttributeRoots() const = 0;
		virtual const List<sas::AttributeId>& GetRuntimeValueKeys() const;
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
