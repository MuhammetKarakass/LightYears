#pragma once

#include "framework/Core.h"
#include "gameConfigs/WeaponStructs.h"

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
		bool isInitialized = false;
		bool isFiring = false;

		float GetFeatureValue(const GameplayTag& key, float fallback = 0.f) const;
		void SetFeatureValue(const GameplayTag& key, float value);
	};

	struct PrimaryWeaponExecutionContext
	{
		Actor& owner;
		const PrimaryWeaponDefinition& definition;
		const GameplayAttributeList& attributes;
		const List<GameplayTag>& damageTags;
	};

	class PrimaryWeaponHandler
	{
	public:
		virtual ~PrimaryWeaponHandler() = default;
		virtual const GameplayTag& GetTypeTag() const = 0;
		virtual const List<GameplayTag>& GetOwnedAttributeRoots() const = 0;
		virtual const List<GameplayTag>& GetInheritedAttributeRoots() const;
		virtual PrimaryWeaponValidationResult ValidateDefinition(const PrimaryWeaponDefinition& definition) const;
		virtual unique_ptr<PrimaryWeaponTypeRuntimeState> CreateRuntimeState() const;
		virtual bool UsesIntervalFire() const { return true; }
		virtual void BeginFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState& state) const;
		virtual void FireOnce(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState& state) const = 0;
		virtual void TickFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState& state, float deltaTime) const;
		virtual void EndFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState& state) const;
	};

	class PrimaryWeaponFeatureHandler
	{
	public:
		virtual ~PrimaryWeaponFeatureHandler() = default;
		virtual const GameplayTag& GetFeatureTag() const = 0;
		virtual const List<GameplayTag>& GetAttributeRoots() const = 0;
		virtual PrimaryWeaponValidationResult ValidateDefinition(const PrimaryWeaponDefinition& definition) const;
		virtual void BeginFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state) const;
		virtual bool CanFire(const PrimaryWeaponExecutionContext& context, const PrimaryWeaponRuntimeState& state) const;
		virtual void AfterFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state) const;
		virtual void TickFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float deltaTime) const;
		virtual void EndFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state) const;
	};

	class PrimaryWeaponRegistry
	{
	public:
		static bool RegisterHandler(unique_ptr<PrimaryWeaponHandler> handler);
		static bool RegisterFeature(unique_ptr<PrimaryWeaponFeatureHandler> feature);
		static PrimaryWeaponValidationResult ValidateDefinition(const PrimaryWeaponDefinition& definition);
		static PrimaryWeaponValidationResult InitializeRuntime(
			const PrimaryWeaponDefinition& definition,
			PrimaryWeaponRuntimeState& state
		);
		static void BeginFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state);
		static bool UsesIntervalFire(const PrimaryWeaponRuntimeState& state);
		static bool FireOnce(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state);
		static void TickFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float deltaTime);
		static void EndFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state);
		static float BuildBaseFireInterval(const GameplayAttributeList& attributes, float actionInterval);
	};
}
