#pragma once

#include "gameplay/enemy/EnemyCombatProfile.h"
#include "gameplay/enemy/EnemySpawnContext.h"
#include "abilities/AbilityPolicies.h"
#include "abilities/GameplayAbilityInstance.h"

#include <memory>
#include <optional>
#include <cstddef>
#include <string>

struct PrimaryWeaponDefinition;

namespace ly
{
	class CombatRuntime;
	class ShipRuntime;
	class GameAbility;
	struct GameAbilityDefinition;

	enum class EnemyRuntimeState : uint8_t
	{
		Empty,
		Ready,
		Faulted
	};

	class IEnemyAbilityOperations
	{
	public:
		virtual ~IEnemyAbilityOperations() = default;
		virtual sas::AbilityHandle GrantAbility(const GameAbilityDefinition& definition, sas::AbilitySlot slot = sas::AbilitySlot::None, std::string* failureReason = nullptr) = 0;
		virtual bool RemoveAbility(sas::AbilityHandle handle, sas::AbilityEndReason reason = sas::AbilityEndReason::Cancelled) = 0;
		virtual bool SetAbilityLevel(sas::AbilityHandle handle, int level) = 0;
		virtual const GameAbility* GetAbility(sas::AbilitySlot slot) const = 0;
		virtual const GameAbility* GetAbility(sas::AbilityHandle handle) const = 0;
		virtual const GameAbility* GetAbilityById(const std::string& abilityId) const = 0;
		virtual std::size_t GetPassiveAbilityCount() const = 0;
		virtual std::size_t GetMaxPassiveAbilityCount() const = 0;
	};

	class EnemyRuntime
	{
	public:
		inline static constexpr const char* EncounterDamageSourceId = "EnemyRuntime.EncounterDamage";
		inline static constexpr const char* ShieldContributionSourceId = "EnemyRuntime.Progression.MaxShield";

		// Compatibility construction is retained for loadout-only tests and tools.
		explicit EnemyRuntime(CombatRuntime& combatRuntime, IEnemyAbilityOperations* customOperations = nullptr);
		EnemyRuntime(CombatRuntime& combatRuntime, ShipRuntime& shipRuntime, IEnemyAbilityOperations* customOperations = nullptr);
		~EnemyRuntime();

		EnemyRuntime(const EnemyRuntime&) = delete;
		EnemyRuntime& operator=(const EnemyRuntime&) = delete;
		EnemyRuntime(EnemyRuntime&&) = delete;
		EnemyRuntime& operator=(EnemyRuntime&&) = delete;

		bool Initialize(const std::string& profileId, float encounterDamageMultiplier = 1.f, std::string* failureReason = nullptr);
		bool Initialize(const EnemyCombatProfile& profile, float encounterDamageMultiplier = 1.f, std::string* failureReason = nullptr);
		bool Initialize(const std::string& profileId, const EnemySpawnContext& spawnContext, float encounterDamageMultiplier = 1.f, std::string* failureReason = nullptr);
		bool Initialize(const EnemyCombatProfile& profile, const EnemySpawnContext& spawnContext, float encounterDamageMultiplier = 1.f, std::string* failureReason = nullptr);

		void SetEncounterDamageMultiplier(float multiplier);
		float GetEncounterDamageMultiplier() const noexcept { return mEncounterDamageMultiplier; }

		EnemyRuntimeState GetState() const noexcept { return mState; }
		bool IsEmpty() const noexcept { return mState == EnemyRuntimeState::Empty; }
		bool IsReady() const noexcept { return mState == EnemyRuntimeState::Ready; }
		bool IsFaulted() const noexcept { return mState == EnemyRuntimeState::Faulted; }
		bool IsSafeEmpty() const noexcept
		{
			return mState == EnemyRuntimeState::Empty &&
				mOwnedLoadoutHandles.empty() &&
				mUnremovedHandles.empty() &&
				mOwnedProgressionModifierHandles.empty() &&
				!mShieldContributionInstalled &&
				!mEncounterDamageModifierInstalled &&
				!mCurrentProfile.has_value();
		}

		const EnemyCombatProfile* GetCurrentProfile() const noexcept
		{
			return mCurrentProfile ? &*mCurrentProfile : nullptr;
		}

		const List<sas::AbilityHandle>& GetOwnedLoadoutHandles() const noexcept { return mOwnedLoadoutHandles; }
		const List<sas::AbilityHandle>& GetGrantedAbilityHandles() const noexcept { return mOwnedLoadoutHandles; }
		const List<sas::AbilityHandle>& GetUnremovedHandles() const noexcept { return mUnremovedHandles; }

		bool Clear(std::string* failureReason = nullptr);

	private:
		struct ResolvedEnemyLoadout
		{
			List<const PrimaryWeaponDefinition*> weapons;
			List<const GameAbilityDefinition*> abilities;
		};
		struct ResolvedEnemyProgression
		{
			List<sas::AttributeModifier> ownerModifiers;
			float shieldContribution = 0.f;
			float outgoingDamageMultiplier = 1.f;
		};
		struct LoadoutSnapshot;
		struct AppliedLoadout;
		enum class RemovalResult : uint8_t;

		IEnemyAbilityOperations& GetOperations();
		const IEnemyAbilityOperations& GetOperations() const;

		bool IsOwnedHandle(sas::AbilityHandle handle) const noexcept;

		bool ResolveLoadout(const EnemyCombatProfile& profile, ResolvedEnemyLoadout& result, std::string* failureReason) const;
		bool ResolveProgression(const EnemyCombatProfile& profile, const EnemySpawnContext& spawnContext, float encounterDamageMultiplier, ResolvedEnemyProgression& result, std::string* failureReason) const;
		bool ApplyProgression(const ResolvedEnemyProgression& progression, List<sas::AttributeModifierHandle>& handles, bool& shieldInstalled, bool& damageInstalled, std::string* failureReason);
		void ClearProgression(List<sas::AttributeModifierHandle>& handles, bool& shieldInstalled, bool& damageInstalled);
		bool CanReplaceCurrentLoadout(const EnemyCombatProfile& profile, std::string* failureReason) const;

		bool CaptureCurrentLoadout(LoadoutSnapshot& snapshot, std::string* failureReason) const;
		RemovalResult RemoveOwnedLoadout(const LoadoutSnapshot& snapshot, List<sas::AbilityHandle>& removedHandles, List<sas::AbilityHandle>& activeHandles, std::string* failureReason);
		bool ApplyResolvedLoadout(const EnemyCombatProfile& profile, const ResolvedEnemyLoadout& resolved, float encounterDamageMultiplier, AppliedLoadout& appliedLoadout, std::string* failureReason);
		bool CleanupAppliedLoadout(AppliedLoadout& appliedLoadout);
		bool RestoreSnapshot(const LoadoutSnapshot& snapshot, const List<sas::AbilityHandle>& removedHandles, AppliedLoadout& restoredLoadout, std::string* failureReason);
		void CommitLoadout(const std::optional<EnemyCombatProfile>& profile, const EnemySpawnContext& spawnContext, float encounterDamageMultiplier, AppliedLoadout&& appliedLoadout);
		void EnterFaultedState(const List<sas::AbilityHandle>& handlesToTrack);

		CombatRuntime& mCombatRuntime;
		ShipRuntime* mShipRuntime = nullptr;
		IEnemyAbilityOperations* mCustomOperations = nullptr;
		std::unique_ptr<IEnemyAbilityOperations> mDefaultOperations;
		EnemyRuntimeState mState = EnemyRuntimeState::Empty;
		std::optional<EnemyCombatProfile> mCurrentProfile;
		List<sas::AbilityHandle> mOwnedLoadoutHandles;
		List<sas::AbilityHandle> mUnremovedHandles;
		List<sas::AttributeModifierHandle> mOwnedProgressionModifierHandles;
		bool mShieldContributionInstalled = false;
		bool mEncounterDamageModifierInstalled = false;
		EnemySpawnContext mSpawnContext;
		float mEncounterDamageMultiplier = 1.f;
	};
}
