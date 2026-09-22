#include "gameplay/enemy/EnemyRuntime.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/ship/ShipRuntime.h"
#include "gameplay/math/MultiplierMath.h"
#include "gameplay/content/EnemyCombatProfileCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameConfigs/ability/AbilityCatalog.h"
#include "framework/debug/Log.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <iterator>

namespace ly
{
	namespace
	{
		class DefaultEnemyAbilityOperations final : public IEnemyAbilityOperations
		{
		public:
			explicit DefaultEnemyAbilityOperations(CombatRuntime& combatRuntime)
				: mCombatRuntime{ combatRuntime }
			{
			}

			sas::AbilityHandle GrantAbility(const GameAbilityDefinition& definition, sas::AbilitySlot slot = sas::AbilitySlot::None, std::string* failureReason = nullptr) override
			{
				return mCombatRuntime.GetAbilitySystemComponent().GrantAbility(definition, slot, failureReason);
			}

			bool RemoveAbility(sas::AbilityHandle handle, sas::AbilityEndReason reason = sas::AbilityEndReason::Cancelled) override
			{
				return mCombatRuntime.GetAbilitySystemComponent().RemoveAbility(handle, reason);
			}

			bool SetAbilityLevel(sas::AbilityHandle handle, int level) override
			{
				return mCombatRuntime.GetAbilitySystemComponent().SetAbilityLevel(handle, level);
			}

			const GameAbility* GetAbility(sas::AbilitySlot slot) const override
			{
				return mCombatRuntime.GetAbilitySystemComponent().GetAbility(slot);
			}

			const GameAbility* GetAbility(sas::AbilityHandle handle) const override
			{
				return mCombatRuntime.GetAbilitySystemComponent().GetAbility(handle);
			}

			const GameAbility* GetAbilityById(const std::string& abilityId) const override
			{
				return mCombatRuntime.GetAbilitySystemComponent().GetAbilityById(abilityId);
			}

			std::size_t GetPassiveAbilityCount() const override
			{
				return mCombatRuntime.GetAbilitySystemComponent().GetPassiveAbilityCount();
			}

			std::size_t GetMaxPassiveAbilityCount() const override
			{
				return LightYearsAbilitySystemComponent::MaxPassiveAbilities;
			}

		private:
			CombatRuntime& mCombatRuntime;
		};

		bool Fail(std::string* failureReason, const std::string& message)
		{
			if (failureReason)
			{
				*failureReason = message;
			}
			return false;
		}

		uint32_t HashCombine(uint32_t value, uint32_t salt) noexcept
		{
			uint32_t result = value ^ (salt + 0x9e3779b9u + (value << 6u) + (value >> 2u));
			result ^= result >> 16u;
			result *= 0x7feb352du;
			result ^= result >> 15u;
			result *= 0x846ca68bu;
			return result ^ (result >> 16u);
		}

		float ResolveVariation(uint32_t seed, uint32_t salt, const EnemyVariationRange& range) noexcept
		{
			if (seed == 0) return 1.f;
			const float normalized = static_cast<float>(HashCombine(seed, salt)) / static_cast<float>(std::numeric_limits<uint32_t>::max());
			return range.minimumMultiplier + (range.maximumMultiplier - range.minimumMultiplier) * normalized;
		}

		float ResolveNaturalGrowthVariation(
			const EnemyProgressionDefinition& progression,
			uint32_t seed,
			const sas::AttributeId& attributeId) noexcept
		{
			if (attributeId == OwnerAttributeIds::MaxHealth)
				return ResolveVariation(seed, 0x48a14c71u, progression.maxHealthVariation);
			if (attributeId == OwnerAttributeIds::Armor)
				return ResolveVariation(seed, 0x91b831cdu, progression.armorVariation);
			// EnemyCombatProfileCatalog currently whitelists only the two channels above.
			// Future growth attributes must opt into a named channel instead of inheriting Armor variation.
			return 1.f;
		}

		void AddUniqueHandle(List<sas::AbilityHandle>& handles, sas::AbilityHandle handle)
		{
			if (handle.IsValid() && std::find(handles.begin(), handles.end(), handle) == handles.end())
			{
				handles.push_back(handle);
			}
		}

		GameAbilityDefinition MakeBoundWeaponAbilityDefinition(const PrimaryWeaponDefinition& weaponDefinition, sas::AbilitySlot slot)
		{
			GameAbilityDefinition definition = AbilityData::MakePrimaryFireAbilityDefinition(weaponDefinition);
			definition.slot = slot;
			return definition;
		}

		struct OldAbilitySpec
		{
			bool isWeapon = false;
			sas::AbilityHandle oldHandle;
			sas::AbilitySlot slot = sas::AbilitySlot::None;
			int level = 1;
			std::string bindingId;
			const GameAbilityDefinition* abilityDef = nullptr;
			const PrimaryWeaponDefinition* weaponDef = nullptr;
		};

		bool RestoreSpec(IEnemyAbilityOperations& ops, const OldAbilitySpec& spec, sas::AbilityHandle& outHandle, std::string* errorMsg)
		{
			if (spec.isWeapon)
			{
				if (!spec.weaponDef)
				{
					if (errorMsg) *errorMsg = "Weapon definition missing during restore";
					return false;
				}
				outHandle = ops.GrantAbility(MakeBoundWeaponAbilityDefinition(*spec.weaponDef, spec.slot), spec.slot, errorMsg);
				if (!outHandle.IsValid()) return false;
				const GameAbility* instance = ops.GetAbility(outHandle);
				if (!instance || instance->GetRuntimeSlot() != spec.slot)
				{
					if (errorMsg) *errorMsg = "Restored weapon did not bind to its requested slot.";
					if (ops.RemoveAbility(outHandle)) outHandle = {};
					return false;
				}
				if (instance->GetLevel() != spec.level && !ops.SetAbilityLevel(outHandle, spec.level))
				{
					if (errorMsg) *errorMsg = "Failed to set weapon level during restore: " + spec.bindingId;
					if (ops.RemoveAbility(outHandle)) outHandle = {};
					return false;
				}
				return true;
			}

			if (!spec.abilityDef)
			{
				if (errorMsg) *errorMsg = "Ability definition missing during restore: " + spec.bindingId;
				return false;
			}
			outHandle = ops.GrantAbility(*spec.abilityDef, spec.slot, errorMsg);
			if (!outHandle.IsValid())
			{
				return false;
			}
			const GameAbility* instance = ops.GetAbility(outHandle);
			if (!instance || instance->GetRuntimeSlot() != spec.slot)
			{
				if (errorMsg) *errorMsg = "Restored ability did not bind to slot: " + spec.bindingId;
				if (ops.RemoveAbility(outHandle)) outHandle = {};
				return false;
			}
			if (spec.level != 1 && !ops.SetAbilityLevel(outHandle, spec.level))
			{
				if (errorMsg) *errorMsg = "Failed to set level during restore: " + spec.bindingId;
				if (ops.RemoveAbility(outHandle)) outHandle = {};
				return false;
			}
			instance = ops.GetAbility(outHandle);
			if (!instance || instance->GetLevel() != spec.level)
			{
				if (errorMsg) *errorMsg = "Restored ability level did not match requested level: " + spec.bindingId;
				if (ops.RemoveAbility(outHandle)) outHandle = {};
				return false;
			}
			return true;
		}
	}

	struct EnemyRuntime::LoadoutSnapshot
	{
		std::optional<EnemyCombatProfile> profile;
		EnemySpawnContext spawnContext;
		float encounterDamageMultiplier = 1.f;
		ResolvedEnemyLoadout resolved;
		List<OldAbilitySpec> specs;
	};

	struct EnemyRuntime::AppliedLoadout
	{
		List<sas::AbilityHandle> ownedLoadoutHandles;
		List<sas::AbilityHandle> transactionHandles;
		bool encounterModifierInstalled = false;
		List<sas::AttributeModifierHandle> progressionModifierHandles;
		bool shieldContributionInstalled = false;
	};

	enum class EnemyRuntime::RemovalResult : uint8_t
	{
		Complete,
		Partial,
		Failed
	};

	EnemyRuntime::EnemyRuntime(CombatRuntime& combatRuntime, IEnemyAbilityOperations* customOperations)
		: mCombatRuntime{ combatRuntime }
		, mCustomOperations{ customOperations }
		, mDefaultOperations{ customOperations ? nullptr : std::make_unique<DefaultEnemyAbilityOperations>(combatRuntime) }
	{
	}

	EnemyRuntime::EnemyRuntime(CombatRuntime& combatRuntime, ShipRuntime& shipRuntime, IEnemyAbilityOperations* customOperations)
		: EnemyRuntime{ combatRuntime, customOperations }
	{
		mShipRuntime = &shipRuntime;
	}

	EnemyRuntime::~EnemyRuntime()
	{
		std::string clearError;
		if (!Clear(&clearError))
		{
			LY_GAME_FATAL("EnemyRuntime destructor failed to safely clear handles: %s", clearError.c_str());
		}
	}

	IEnemyAbilityOperations& EnemyRuntime::GetOperations()
	{
		return mCustomOperations ? *mCustomOperations : *mDefaultOperations;
	}

	const IEnemyAbilityOperations& EnemyRuntime::GetOperations() const
	{
		return mCustomOperations ? *mCustomOperations : *mDefaultOperations;
	}

	bool EnemyRuntime::IsOwnedHandle(sas::AbilityHandle handle) const noexcept
	{
		if (!handle.IsValid())
		{
			return false;
		}
		if (std::find(mOwnedLoadoutHandles.begin(), mOwnedLoadoutHandles.end(), handle) != mOwnedLoadoutHandles.end())
		{
			return true;
		}
		if (std::find(mUnremovedHandles.begin(), mUnremovedHandles.end(), handle) != mUnremovedHandles.end())
		{
			return true;
		}
		return false;
	}

	bool EnemyRuntime::Initialize(const std::string& profileId, float encounterDamageMultiplier, std::string* failureReason)
	{
		const EnemyCombatProfile* profile = content::EnemyCombatProfileCatalog::FindById(profileId);
		if (!profile)
		{
			return Fail(failureReason, "Enemy combat profile '" + profileId + "' not found.");
		}
		return Initialize(*profile, encounterDamageMultiplier, failureReason);
	}

	bool EnemyRuntime::Initialize(const std::string& profileId, const EnemySpawnContext& spawnContext, float encounterDamageMultiplier, std::string* failureReason)
	{
		const EnemyCombatProfile* profile = content::EnemyCombatProfileCatalog::FindById(profileId);
		return profile ? Initialize(*profile, spawnContext, encounterDamageMultiplier, failureReason) : Fail(failureReason, "Enemy combat profile '" + profileId + "' not found.");
	}

	bool EnemyRuntime::ResolveLoadout(const EnemyCombatProfile& profile, ResolvedEnemyLoadout& result, std::string* failureReason) const
	{
		if (!content::EnemyCombatProfileCatalog::ValidateProfile(profile, failureReason))
		{
			return false;
		}

		result.weapons.clear();
		result.weapons.reserve(profile.weapons.size());
		for (const EnemyWeaponBinding& binding : profile.weapons)
		{
			const PrimaryWeaponDefinition* weapon = content::WeaponContentCatalog::FindById(binding.weaponId);
			if (!weapon)
			{
				return Fail(failureReason, "Weapon '" + binding.weaponId + "' not found.");
			}
			result.weapons.push_back(weapon);
		}

		result.abilities.clear();
		result.abilities.reserve(profile.abilities.size());
		for (const EnemyAbilityBinding& binding : profile.abilities)
		{
			const GameAbilityDefinition* abilityDef = AbilityData::FindShippedAbilityDefinition(binding.abilityId);
			if (!abilityDef)
			{
				return Fail(failureReason, "Ability '" + binding.abilityId + "' not found.");
			}
			result.abilities.push_back(abilityDef);
		}
		return true;
	}

	bool EnemyRuntime::ResolveProgression(const EnemyCombatProfile& profile, const EnemySpawnContext& spawnContext, float encounterDamageMultiplier, ResolvedEnemyProgression& result, std::string* failureReason) const
	{
		if (spawnContext.level < 1) return Fail(failureReason, "Enemy spawn level must be at least 1.");
		if (!content::EnemyCombatProfileCatalog::ValidateProfile(profile, failureReason)) return false;
		const bool hasOwnerGrowth = !profile.progression.naturalGrowth.empty();
		const bool hasShieldGrowth = profile.progression.maxShieldPerLevel > 0.f;
		const bool hasShieldVariation = profile.progression.maxShieldVariation.minimumMultiplier != 1.f ||
			profile.progression.maxShieldVariation.maximumMultiplier != 1.f;
		if (!mShipRuntime && ((spawnContext.level > 1 && (hasOwnerGrowth || hasShieldGrowth)) ||
			(spawnContext.variationSeed != 0 && (hasOwnerGrowth || hasShieldGrowth || hasShieldVariation))))
		{
			return Fail(failureReason, "Enemy progression requires ShipRuntime for owner attributes or shield resolution.");
		}
		result = {};
		const float completedLevels = static_cast<float>(spawnContext.level - 1);
		const float shieldVariation = ResolveVariation(spawnContext.variationSeed, 0x63d83595u, profile.progression.maxShieldVariation);
		const float damageVariation = ResolveVariation(spawnContext.variationSeed, 0xa7f48d13u, profile.progression.outgoingDamageVariation);
		if (!std::isfinite(shieldVariation) || !std::isfinite(damageVariation)) return Fail(failureReason, "Enemy progression variation resolved to a non-finite value.");
		if (mShipRuntime)
		{
			auto& attributes = mCombatRuntime.GetAbilitySystemComponent().GetAttributes();
			for (const AttributeGrowthEntry& growth : profile.progression.naturalGrowth)
			{
				const float base = attributes.GetBaseValue(growth.attributeId);
				const float variation = ResolveNaturalGrowthVariation(profile.progression, spawnContext.variationSeed, growth.attributeId);
				if (!std::isfinite(variation)) return Fail(failureReason, "Enemy natural growth variation resolved to a non-finite value.");
				const float magnitude = (base + growth.perLevel * completedLevels) * variation - base;
				if (!std::isfinite(magnitude)) return Fail(failureReason, "Enemy progression modifier resolved to a non-finite value.");
				if (magnitude != 0.f) result.ownerModifiers.push_back({ growth.attributeId, sas::AttributeModifierOperation::Add, magnitude });
			}
			const float shieldContribution = (mShipRuntime->GetAuthoredBaseMaxShield() + profile.progression.maxShieldPerLevel * completedLevels) * shieldVariation - mShipRuntime->GetAuthoredBaseMaxShield();
			if (!std::isfinite(shieldContribution)) return Fail(failureReason, "Enemy shield progression resolved to a non-finite value.");
			result.shieldContribution = shieldContribution;
		}
		const float multipliers[] = { encounterDamageMultiplier, 1.f + profile.progression.outgoingDamagePerLevel * completedLevels, damageVariation };
		if (!math::TryResolveMultiplierProduct(multipliers, std::size(multipliers), result.outgoingDamageMultiplier)) return Fail(failureReason, "Enemy outgoing damage progression multiplier is invalid.");
		return true;
	}

	bool EnemyRuntime::ApplyProgression(const ResolvedEnemyProgression& progression, List<sas::AttributeModifierHandle>& handles, bool& shieldInstalled, bool& damageInstalled, std::string* failureReason)
	{
		handles.clear(); shieldInstalled = false; damageInstalled = false;
		if (mShipRuntime)
		{
			auto& attributes = mCombatRuntime.GetAbilitySystemComponent().GetAttributes();
			for (const sas::AttributeModifier& modifier : progression.ownerModifiers)
			{
				const sas::AttributeModifierHandle handle = attributes.AddModifier(modifier);
				if (!handle.IsValid()) { ClearProgression(handles, shieldInstalled, damageInstalled); return Fail(failureReason, "Failed to install enemy progression attribute modifier."); }
				handles.push_back(handle);
			}
			if (progression.shieldContribution != 0.f)
			{
				if (!mShipRuntime->SetBaseMaxShieldContribution(ShieldContributionSourceId, progression.shieldContribution)) { ClearProgression(handles, shieldInstalled, damageInstalled); return Fail(failureReason, "Failed to install enemy shield progression contribution."); }
				shieldInstalled = true;
			}
		}
		if (!mCombatRuntime.SetRuntimeModifier(EncounterDamageSourceId, CombatRuntimeModifier{ progression.outgoingDamageMultiplier })) { ClearProgression(handles, shieldInstalled, damageInstalled); return Fail(failureReason, "Failed to install enemy outgoing damage modifier."); }
		damageInstalled = true;
		return true;
	}

	void EnemyRuntime::ClearProgression(List<sas::AttributeModifierHandle>& handles, bool& shieldInstalled, bool& damageInstalled)
	{
		if (mShipRuntime) for (const sas::AttributeModifierHandle handle : handles) mCombatRuntime.GetAbilitySystemComponent().GetAttributes().RemoveModifier(handle);
		handles.clear();
		if (mShipRuntime && shieldInstalled) mShipRuntime->RemoveBaseMaxShieldContribution(ShieldContributionSourceId);
		shieldInstalled = false;
		if (damageInstalled) mCombatRuntime.RemoveRuntimeModifier(EncounterDamageSourceId);
		damageInstalled = false;
	}

	bool EnemyRuntime::CanReplaceCurrentLoadout(const EnemyCombatProfile& profile, std::string* failureReason) const
	{
		const auto& ops = GetOperations();
		if (!mUnremovedHandles.empty())
		{
			return Fail(failureReason, "Tracked enemy loadout has unresolved ability handles.");
		}
		for (const sas::AbilityHandle handle : mOwnedLoadoutHandles)
		{
			if (handle.IsValid() && !ops.GetAbility(handle))
			{
				return Fail(failureReason, "Tracked enemy ability handle no longer resolves.");
			}
		}

		const auto rejectExternalId = [&](const std::string& abilityId, const char* bindingKind)
		{
			const GameAbility* existing = ops.GetAbilityById(abilityId);
			if (existing && !IsOwnedHandle(existing->GetHandle()))
			{
				return Fail(failureReason, std::string{ bindingKind } + " '" + abilityId + "' is already granted outside the enemy loadout.");
			}
			return true;
		};
		for (const EnemyWeaponBinding& binding : profile.weapons)
		{
			if (!rejectExternalId(binding.weaponId, "Weapon")) return false;
		}
		for (const EnemyAbilityBinding& binding : profile.abilities)
		{
			if (!rejectExternalId(binding.abilityId, "Ability")) return false;
		}

		std::size_t ownedPassiveCount = 0;
		for (const sas::AbilityHandle handle : mOwnedLoadoutHandles)
		{
			const GameAbility* ability = ops.GetAbility(handle);
			if (ability && ability->GetRuntimeSlot() == sas::AbilitySlot::None)
			{
				++ownedPassiveCount;
			}
		}
		const std::size_t currentPassiveCount = ops.GetPassiveAbilityCount();
		if (ownedPassiveCount > currentPassiveCount)
		{
			return Fail(failureReason, "Tracked enemy passive ability count exceeds the ability system count.");
		}
		std::size_t requestedPassiveCount = 0;
		for (const EnemyAbilityBinding& binding : profile.abilities)
		{
			if (binding.slot == sas::AbilitySlot::None) ++requestedPassiveCount;
		}
		const std::size_t externalPassiveCount = currentPassiveCount - ownedPassiveCount;
		if (externalPassiveCount + requestedPassiveCount > ops.GetMaxPassiveAbilityCount())
		{
			return Fail(failureReason, "Enemy loadout exceeds the available passive ability capacity.");
		}

		std::vector<sas::AbilitySlot> targetSlots;
		for (const EnemyWeaponBinding& binding : profile.weapons) targetSlots.push_back(binding.slot);
		for (const EnemyAbilityBinding& binding : profile.abilities)
		{
			if (binding.slot != sas::AbilitySlot::None) targetSlots.push_back(binding.slot);
		}
		for (const sas::AbilitySlot slot : targetSlots)
		{
			if (const GameAbility* existing = ops.GetAbility(slot))
			{
				if (!IsOwnedHandle(existing->GetHandle()))
				{
					return Fail(failureReason, "Target slot is occupied by an external ability.");
				}
			}
		}
		return true;
	}

	bool EnemyRuntime::CaptureCurrentLoadout(LoadoutSnapshot& snapshot, std::string* failureReason) const
	{
		snapshot = {};
		snapshot.profile = mCurrentProfile;
		snapshot.spawnContext = mSpawnContext;
		snapshot.encounterDamageMultiplier = mEncounterDamageMultiplier;
		if (!snapshot.profile.has_value())
		{
			return (mOwnedLoadoutHandles.empty() && mOwnedProgressionModifierHandles.empty() &&
				!mShieldContributionInstalled && !mEncounterDamageModifierInstalled) ||
				Fail(failureReason, "EnemyRuntime has owned state without a current profile.");
		}
		if (!ResolveLoadout(*snapshot.profile, snapshot.resolved, failureReason))
		{
			return false;
		}
		const size_t bindingCount = snapshot.profile->weapons.size() + snapshot.profile->abilities.size();
		if (bindingCount != mOwnedLoadoutHandles.size())
		{
			return Fail(failureReason, "EnemyRuntime loadout handle state does not match its current profile.");
		}
		size_t handleIndex = 0;
		for (size_t i = 0; i < snapshot.profile->weapons.size(); ++i)
		{
			const EnemyWeaponBinding& binding = snapshot.profile->weapons[i];
			snapshot.specs.push_back({ true, mOwnedLoadoutHandles[handleIndex++], binding.slot, binding.level, binding.weaponId, nullptr, snapshot.resolved.weapons[i] });
		}
		for (size_t i = 0; i < snapshot.profile->abilities.size(); ++i)
		{
			const EnemyAbilityBinding& binding = snapshot.profile->abilities[i];
			snapshot.specs.push_back({ false, mOwnedLoadoutHandles[handleIndex++], binding.slot, binding.level, binding.abilityId, snapshot.resolved.abilities[i], nullptr });
		}
		return true;
	}

	EnemyRuntime::RemovalResult EnemyRuntime::RemoveOwnedLoadout(const LoadoutSnapshot& snapshot, List<sas::AbilityHandle>& removedHandles, List<sas::AbilityHandle>& activeHandles, std::string* failureReason)
	{
		auto& ops = GetOperations();
		removedHandles.clear();
		activeHandles.clear();
		for (size_t i = 0; i < snapshot.specs.size(); ++i)
		{
			const sas::AbilityHandle handle = snapshot.specs[i].oldHandle;
			if (ops.RemoveAbility(handle))
			{
				removedHandles.push_back(handle);
				continue;
			}
			for (; i < snapshot.specs.size(); ++i)
			{
				AddUniqueHandle(activeHandles, snapshot.specs[i].oldHandle);
			}
			Fail(failureReason, "Failed to remove previous loadout handle.");
			return removedHandles.empty() ? RemovalResult::Failed : RemovalResult::Partial;
		}
		return RemovalResult::Complete;
	}

	bool EnemyRuntime::ApplyResolvedLoadout(const EnemyCombatProfile& profile, const ResolvedEnemyLoadout& resolved, float encounterDamageMultiplier, AppliedLoadout& appliedLoadout, std::string* failureReason)
	{
		appliedLoadout.ownedLoadoutHandles.clear();
		appliedLoadout.transactionHandles.clear();
		auto& ops = GetOperations();
		for (size_t i = 0; i < resolved.weapons.size(); ++i)
		{
			const EnemyWeaponBinding& binding = profile.weapons[i];
			const sas::AbilityHandle handle = ops.GrantAbility(MakeBoundWeaponAbilityDefinition(*resolved.weapons[i], binding.slot), binding.slot, failureReason);
			if (!handle.IsValid())
			{
				return false;
			}
			appliedLoadout.transactionHandles.push_back(handle);
			appliedLoadout.ownedLoadoutHandles.push_back(handle);
			const GameAbility* instance = ops.GetAbility(handle);
			if (!instance || instance->GetRuntimeSlot() != binding.slot)
			{
				return Fail(failureReason, "Weapon '" + binding.weaponId + "' did not bind to its requested slot.");
			}
			if (instance->GetLevel() != binding.level && !ops.SetAbilityLevel(handle, binding.level))
			{
				return Fail(failureReason, "Failed to set weapon level " + std::to_string(binding.level) + " for " + binding.weaponId);
			}
			instance = ops.GetAbility(handle);
			if (!instance || instance->GetLevel() != binding.level)
			{
				return Fail(failureReason, "Weapon '" + binding.weaponId + "' did not retain its requested level.");
			}
		}

		for (size_t i = 0; i < profile.abilities.size(); ++i)
		{
			const EnemyAbilityBinding& binding = profile.abilities[i];
			const sas::AbilityHandle handle = ops.GrantAbility(*resolved.abilities[i], binding.slot, failureReason);
			if (!handle.IsValid())
			{
				return false;
			}
			appliedLoadout.transactionHandles.push_back(handle);
			appliedLoadout.ownedLoadoutHandles.push_back(handle);
			const GameAbility* instance = ops.GetAbility(handle);
			if (!instance || instance->GetRuntimeSlot() != binding.slot)
			{
				return Fail(failureReason, "Ability '" + binding.abilityId + "' did not bind to its requested slot.");
			}
			if (instance->GetLevel() != binding.level && !ops.SetAbilityLevel(handle, binding.level))
			{
				return Fail(failureReason, "Failed to set ability level " + std::to_string(binding.level) + " for " + binding.abilityId);
			}
			instance = ops.GetAbility(handle);
			if (!instance || instance->GetLevel() != binding.level)
			{
				return Fail(failureReason, "Ability '" + binding.abilityId + "' did not retain its requested level.");
			}
		}

		(void)encounterDamageMultiplier;
		return true;
	}

	bool EnemyRuntime::CleanupAppliedLoadout(AppliedLoadout& appliedLoadout)
	{
		bool cleaned = true;
		auto& ops = GetOperations();
		for (const sas::AbilityHandle handle : appliedLoadout.transactionHandles)
		{
			if (handle.IsValid() && !ops.RemoveAbility(handle))
			{
				cleaned = false;
				AddUniqueHandle(mUnremovedHandles, handle);
			}
		}
		return cleaned;
	}

	bool EnemyRuntime::RestoreSnapshot(const LoadoutSnapshot& snapshot, const List<sas::AbilityHandle>& removedHandles, AppliedLoadout& restoredLoadout, std::string* failureReason)
	{
		restoredLoadout.ownedLoadoutHandles.clear();
		restoredLoadout.transactionHandles.clear();
		if (!snapshot.profile.has_value())
		{
			return true;
		}

		auto& ops = GetOperations();
		for (const OldAbilitySpec& spec : snapshot.specs)
		{
			sas::AbilityHandle handle = spec.oldHandle;
			if (std::find(removedHandles.begin(), removedHandles.end(), spec.oldHandle) != removedHandles.end())
			{
				if (!RestoreSpec(ops, spec, handle, failureReason))
				{
					AddUniqueHandle(restoredLoadout.transactionHandles, handle);
					return false;
				}
				restoredLoadout.transactionHandles.push_back(handle);
			}
			restoredLoadout.ownedLoadoutHandles.push_back(handle);
		}

		return true;
	}

	void EnemyRuntime::CommitLoadout(const std::optional<EnemyCombatProfile>& profile, const EnemySpawnContext& spawnContext, float encounterDamageMultiplier, AppliedLoadout&& appliedLoadout)
	{
		mOwnedLoadoutHandles = std::move(appliedLoadout.ownedLoadoutHandles);
		mOwnedProgressionModifierHandles = std::move(appliedLoadout.progressionModifierHandles);
		mShieldContributionInstalled = appliedLoadout.shieldContributionInstalled;
		mEncounterDamageModifierInstalled = appliedLoadout.encounterModifierInstalled;
		mCurrentProfile = profile;
		mSpawnContext = profile.has_value() ? spawnContext : EnemySpawnContext{};
		mEncounterDamageMultiplier = profile.has_value() ? encounterDamageMultiplier : 1.f;
		mState = profile.has_value() ? EnemyRuntimeState::Ready : EnemyRuntimeState::Empty;
	}

	void EnemyRuntime::EnterFaultedState(const List<sas::AbilityHandle>& handlesToTrack)
	{
		for (const sas::AbilityHandle handle : handlesToTrack)
		{
			AddUniqueHandle(mUnremovedHandles, handle);
		}
		mOwnedLoadoutHandles.clear();
		ClearProgression(mOwnedProgressionModifierHandles, mShieldContributionInstalled, mEncounterDamageModifierInstalled);
		mCurrentProfile.reset();
		mSpawnContext = {};
		mEncounterDamageMultiplier = 1.f;
		mCombatRuntime.RemoveRuntimeModifier(EncounterDamageSourceId);
		mState = EnemyRuntimeState::Faulted;
	}

	bool EnemyRuntime::Initialize(const EnemyCombatProfile& profile, float encounterDamageMultiplier, std::string* failureReason)
	{
		// Legacy callers did not own spawn context; retain their pre-progression
		// encounter-only behaviour instead of implicitly sampling variation.
		EnemyCombatProfile legacyProfile = profile;
		legacyProfile.progression = {};
		return Initialize(legacyProfile, EnemySpawnContext{}, encounterDamageMultiplier, failureReason);
	}

	bool EnemyRuntime::Initialize(const EnemyCombatProfile& profile, const EnemySpawnContext& spawnContext, float encounterDamageMultiplier, std::string* failureReason)
	{
		if (mState == EnemyRuntimeState::Faulted)
		{
			return Fail(failureReason, "Cannot initialize: EnemyRuntime is in a Faulted state.");
		}
		if (!std::isfinite(encounterDamageMultiplier))
		{
			return Fail(failureReason, "Encounter damage multiplier must be finite.");
		}
		encounterDamageMultiplier = std::max(0.f, encounterDamageMultiplier);

		ResolvedEnemyLoadout resolved;
		ResolvedEnemyProgression resolvedProgression;
		if (!ResolveLoadout(profile, resolved, failureReason) || !ResolveProgression(profile, spawnContext, encounterDamageMultiplier, resolvedProgression, failureReason) || !CanReplaceCurrentLoadout(profile, failureReason))
		{
			return false;
		}

		LoadoutSnapshot snapshot;
		if (!CaptureCurrentLoadout(snapshot, failureReason))
		{
			return false;
		}

		List<sas::AbilityHandle> removedHandles;
		List<sas::AbilityHandle> activeHandles;
		const RemovalResult removal = RemoveOwnedLoadout(snapshot, removedHandles, activeHandles, failureReason);
		if (removal == RemovalResult::Failed)
		{
			return Fail(failureReason, "Failed to remove previous loadout handle; previous profile was preserved.");
		}
		if (removal == RemovalResult::Partial)
		{
			AppliedLoadout restoredLoadout;
			restoredLoadout.progressionModifierHandles = mOwnedProgressionModifierHandles;
			restoredLoadout.shieldContributionInstalled = mShieldContributionInstalled;
			restoredLoadout.encounterModifierInstalled = mEncounterDamageModifierInstalled;
			std::string restoreError;
			if (RestoreSnapshot(snapshot, removedHandles, restoredLoadout, &restoreError))
			{
				CommitLoadout(snapshot.profile, snapshot.spawnContext, snapshot.encounterDamageMultiplier, std::move(restoredLoadout));
				return Fail(failureReason, "Failed to remove previous loadout handle; previous profile was restored.");
			}
			CleanupAppliedLoadout(restoredLoadout);
			EnterFaultedState(activeHandles);
			return Fail(failureReason, "Failed to remove previous loadout and rollback failed; runtime is Faulted: " + restoreError);
		}

		AppliedLoadout appliedLoadout;
		std::string applyError;
		ClearProgression(mOwnedProgressionModifierHandles, mShieldContributionInstalled, mEncounterDamageModifierInstalled);
		if (ApplyProgression(resolvedProgression, appliedLoadout.progressionModifierHandles, appliedLoadout.shieldContributionInstalled, appliedLoadout.encounterModifierInstalled, &applyError) &&
			ApplyResolvedLoadout(profile, resolved, encounterDamageMultiplier, appliedLoadout, &applyError))
		{
			CommitLoadout(profile, spawnContext, encounterDamageMultiplier, std::move(appliedLoadout));
			return true;
		}

		if (!CleanupAppliedLoadout(appliedLoadout))
		{
			EnterFaultedState({});
			return Fail(failureReason, "Failed to apply profile and failed to remove newly granted handles; runtime is Faulted.");
		}
		ClearProgression(appliedLoadout.progressionModifierHandles, appliedLoadout.shieldContributionInstalled, appliedLoadout.encounterModifierInstalled);
		if (!snapshot.profile.has_value())
		{
			CommitLoadout(std::nullopt, {}, 1.f, {});
			return Fail(failureReason, "Failed to apply profile '" + profile.profileId + "': " + applyError);
		}

		AppliedLoadout restoredLoadout;
		std::string restoreError;
		ResolvedEnemyProgression restoredProgression;
		if (ResolveProgression(*snapshot.profile, snapshot.spawnContext, snapshot.encounterDamageMultiplier, restoredProgression, &restoreError) &&
			ApplyProgression(restoredProgression, restoredLoadout.progressionModifierHandles, restoredLoadout.shieldContributionInstalled, restoredLoadout.encounterModifierInstalled, &restoreError) &&
			RestoreSnapshot(snapshot, removedHandles, restoredLoadout, &restoreError))
		{
			CommitLoadout(snapshot.profile, snapshot.spawnContext, snapshot.encounterDamageMultiplier, std::move(restoredLoadout));
			return Fail(failureReason, "Failed to apply profile '" + profile.profileId + "'; previous profile was restored: " + applyError);
		}
		CleanupAppliedLoadout(restoredLoadout);
		ClearProgression(restoredLoadout.progressionModifierHandles, restoredLoadout.shieldContributionInstalled, restoredLoadout.encounterModifierInstalled);
		EnterFaultedState({});
		return Fail(failureReason, "Failed to apply profile '" + profile.profileId + "' and rollback failed; runtime is Faulted: " + restoreError);
	}

	void EnemyRuntime::SetEncounterDamageMultiplier(float multiplier)
	{
		if (mState != EnemyRuntimeState::Ready || !std::isfinite(multiplier))
		{
			return;
		}
		ResolvedEnemyProgression resolved;
		if (!mCurrentProfile || !ResolveProgression(*mCurrentProfile, mSpawnContext, std::max(0.f, multiplier), resolved, nullptr)) return;
		mEncounterDamageMultiplier = std::max(0.f, multiplier);
		mCombatRuntime.SetRuntimeModifier(EncounterDamageSourceId, CombatRuntimeModifier{ resolved.outgoingDamageMultiplier });
	}

	bool EnemyRuntime::Clear(std::string* failureReason)
	{
		auto& ops = GetOperations();
		std::string errorMsg;

		for (sas::AbilityHandle h : mOwnedLoadoutHandles)
		{
			if (h.IsValid())
			{
				if (!ops.RemoveAbility(h))
				{
					if (std::find(mUnremovedHandles.begin(), mUnremovedHandles.end(), h) == mUnremovedHandles.end())
					{
						mUnremovedHandles.push_back(h);
					}
					errorMsg += "Failed to remove granted ability handle. ";
				}
			}
		}
		mOwnedLoadoutHandles.clear();

		List<sas::AbilityHandle> stillUnremoved;
		for (sas::AbilityHandle h : mUnremovedHandles)
		{
			if (h.IsValid())
			{
				if (!ops.RemoveAbility(h))
				{
					stillUnremoved.push_back(h);
				}
			}
		}
		mUnremovedHandles = std::move(stillUnremoved);

		ClearProgression(mOwnedProgressionModifierHandles, mShieldContributionInstalled, mEncounterDamageModifierInstalled);
		mCurrentProfile.reset();
		mSpawnContext = {};
		mEncounterDamageMultiplier = 1.f;

		if (mUnremovedHandles.empty())
		{
			mState = EnemyRuntimeState::Empty;
			return true;
		}

		mState = EnemyRuntimeState::Faulted;
		return Fail(failureReason, errorMsg.empty() ? "Unremoved handles remain in ASC." : errorMsg);
	}
}
