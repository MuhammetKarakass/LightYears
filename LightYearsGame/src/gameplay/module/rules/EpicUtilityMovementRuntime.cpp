#include "gameplay/module/rules/EpicUtilityMovementRuntime.h"

#include "framework/World.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/CombatRuntimeModifiers.h"
#include "gameplay/damage/DamageHitGroup.h"
#include "gameplay/module/ModuleAbilitySlots.h"
#include "gameplay/tags/GameplayTags.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "player/PlayerSpaceShip.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace vega
{
	namespace
	{
		constexpr sas::AbilitySlot kLoadoutSlots[] = { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 };
		const char* const kAssaultDamageId = "ModuleAssaultVector.Damage";
		const char* const kPhaseProtectionId = "ModulePhaseSlip.Protection";


		bool HasTag(const List<GameplayTag>& tags, const GameplayTag& wanted)
		{
			return std::any_of(tags.begin(), tags.end(), [&wanted](const GameplayTag& tag) { return tag.MatchesTag(wanted); });
		}

		// Family of one ability use: the first of the four loadout families its tags name, or an invalid tag.
		GameplayTag FamilyOf(const List<GameplayTag>& tags)
		{
			const GameplayTag families[] = { GameplayTags::Ability::Offense, GameplayTags::Ability::Defense, GameplayTags::Ability::Movement, GameplayTags::Ability::Utility };
			for (const GameplayTag& family : families)
			{
				if (HasTag(tags, family)) return family;
			}
			return {};
		}

		const ResolvedModuleCascadeRule* FindCascade(const List<ResolvedModuleCascadeRule>& rules, std::uint32_t instanceId)
		{
			for (const ResolvedModuleCascadeRule& rule : rules)
			{
				if (rule.instanceId == instanceId) return &rule;
			}
			return nullptr;
		}

		float Length(const sf::Vector2f& value) { return std::sqrt(value.x * value.x + value.y * value.y); }
	}

	ModuleEpicUtilityMovementRuntime::ModuleEpicUtilityMovementRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime)
		: mAbilities{ abilities }
		, mCombatRuntime{ combatRuntime }
		, mShip{ dynamic_cast<SpaceShip*>(&abilities.GetOwner()) }
		, mPlayer{ dynamic_cast<PlayerSpaceShip*>(&abilities.GetOwner()) }
	{
		mEventHandle = mAbilities.onGameplayEvent.BindAction(this, &ModuleEpicUtilityMovementRuntime::OnEvent);
	}

	ModuleEpicUtilityMovementRuntime::~ModuleEpicUtilityMovementRuntime()
	{
		mShuttingDown = true;
		mAbilities.onGameplayEvent.UnbindAction(mEventHandle);
		try { Clear(); } catch (...) {}
	}

	void ModuleEpicUtilityMovementRuntime::Replace(const ResolvedModuleEpicRules& rules)
	{
		if (mShuttingDown) return;
		mRules = rules;

		// Keep committed chains only for placements still present; the next use reads the current resolved level.
		List<CascadeState> kept;
		for (CascadeState& state : mCascades)
		{
			if (FindCascade(rules.cascades, state.instanceId)) kept.push_back(state);
		}
		for (const ResolvedModuleCascadeRule& rule : rules.cascades)
		{
			const bool known = std::any_of(kept.begin(), kept.end(), [&rule](const CascadeState& state) { return state.instanceId == rule.instanceId; });
			if (known) continue;
			CascadeState fresh;
			fresh.instanceId = rule.instanceId;
			kept.push_back(fresh);
		}
		mCascades = std::move(kept);

		if (rules.assaultVectors.empty()) ClearAssault();
		if (rules.phaseSlips.empty()) ClearPhase();
	}

	void ModuleEpicUtilityMovementRuntime::Tick(float deltaSeconds)
	{
		if (mShuttingDown) return;
		TickCascades(deltaSeconds);
		TickPhase(deltaSeconds);
		TickAssaultVector(deltaSeconds);
		ProcessFieldMastery();
		ProcessExplosions();
	}

	void ModuleEpicUtilityMovementRuntime::OnAbilityUse(const std::string& abilityId, const List<GameplayTag>& tags)
	{
		if (mShuttingDown) return;
		const GameplayTag family = FamilyOf(tags);
		if (family.IsValid())
		{
			for (CascadeState& state : mCascades) AdvanceCascade(state, family);
		}
		if (HasTag(tags, GameplayTags::Ability::Utility) && FieldMasteryThreshold() > 0) AddUses(abilityId, 1.0);
	}

	bool ModuleEpicUtilityMovementRuntime::IsSystemLocked(const GameplayTag& systemTag) const
	{
		if (!IsPhaseActive()) return false;
		return systemTag == GameplayTags::Module::System::PrimaryWeapon || AbilitySlotForModuleSystem(systemTag) != sas::AbilitySlot::None;
	}

	bool ModuleEpicUtilityMovementRuntime::IsScrapUpgradeLocked(const List<GameplayTag>& abilityTags) const
	{
		return FieldMasteryThreshold() > 0 && HasTag(abilityTags, GameplayTags::Ability::Utility);
	}

	int ModuleEpicUtilityMovementRuntime::GetCascadeStep(std::uint32_t instanceId) const
	{
		for (const CascadeState& state : mCascades)
		{
			if (state.instanceId == instanceId) return state.step;
		}
		return 0;
	}

	float ModuleEpicUtilityMovementRuntime::GetUseCount(const std::string& abilityId) const
	{
		for (const UseCounter& counter : mUseCounters)
		{
			if (counter.abilityId == abilityId) return static_cast<float>(counter.uses);
		}
		return 0.f;
	}

	// Cascade Reactor: the first use starts the chain without a bonus. A different family advances it by one step (capped);
	// the same family or a lapsed window restarts it. The powered use captures its quantity multiplier before activation.
	void ModuleEpicUtilityMovementRuntime::AdvanceCascade(CascadeState& state, const GameplayTag& family)
	{
		const ResolvedModuleCascadeRule* rule = FindCascade(mRules.cascades, state.instanceId);
		if (!rule) return;
		const bool continuing = state.lastFamily.IsValid() && state.remainingSeconds > 0.f && !(family == state.lastFamily);
		state.step = continuing ? std::min(state.step + 1, rule->maxSteps) : 0;
		state.lastFamily = family;
		state.remainingSeconds = rule->windowSeconds;
	}

	float ModuleEpicUtilityMovementRuntime::GetNextCascadeMultiplier(const List<GameplayTag>& tags) const
	{
		const GameplayTag family = FamilyOf(tags);
		if (!family.IsValid()) return 1.f;
		float multiplier = 1.f;
		for (const CascadeState& state : mCascades)
		{
			const auto* rule = FindCascade(mRules.cascades, state.instanceId);
			if (!rule || !state.lastFamily.IsValid() || state.remainingSeconds <= 0.f || family == state.lastFamily) continue;
			multiplier *= 1.f + rule->stepBonus * static_cast<float>(std::min(state.step + 1, rule->maxSteps));
		}
		return multiplier;
	}

	void ModuleEpicUtilityMovementRuntime::TickCascades(float deltaSeconds)
	{
		for (CascadeState& state : mCascades)
		{
			if (state.remainingSeconds <= 0.f) continue;
			state.remainingSeconds -= deltaSeconds;
			if (state.remainingSeconds > 0.f) continue;
			state.remainingSeconds = 0.f;
			state.step = 0;
			state.lastFamily = {};
		}
	}

	// Phase Slip: an Afterburner start (rising edge) opens the phase while the cooldown is ready. The phase locks Primary and
	// loadout abilities and blocks incoming hits; when it ends the next Primary hit detonates.
	void ModuleEpicUtilityMovementRuntime::TickPhase(float deltaSeconds)
	{
		mPhaseCooldown = std::max(0.f, mPhaseCooldown - deltaSeconds);
		if (mPhaseRemaining > 0.f)
		{
			mPhaseRemaining -= deltaSeconds;
			if (mPhaseRemaining <= 0.f) EndPhase();
		}
		const bool afterburning = mPlayer && mPlayer->IsAfterburning();
		if (afterburning && !mPhaseAfterburning && !IsPhaseActive() && mPhaseCooldown <= 0.f && !mRules.phaseSlips.empty()) StartPhase();
		mPhaseAfterburning = afterburning;
	}

	void ModuleEpicUtilityMovementRuntime::StartPhase()
	{
		const ResolvedModulePhaseSlipRule& rule = mRules.phaseSlips.front();
		mPhaseRemaining = rule.phaseSeconds;
		mPhaseCooldown = rule.cooldownSeconds;
		mPhaseArmed = false;
		mCombatRuntime.SetDamageProtection(kPhaseProtectionId, true, false);
		mAbilities.CancelAbilitiesUnavailableByModules();
	}

	void ModuleEpicUtilityMovementRuntime::EndPhase()
	{
		mPhaseRemaining = 0.f;
		mCombatRuntime.RemoveDamageProtection(kPhaseProtectionId);
		mPhaseArmed = !mRules.phaseSlips.empty();
	}

	void ModuleEpicUtilityMovementRuntime::ClearPhase()
	{
		if (mPhaseRemaining > 0.f) mCombatRuntime.RemoveDamageProtection(kPhaseProtectionId);
		mPhaseRemaining = 0.f;
		mPhaseArmed = false;
	}

	// Outgoing Primary hit after the phase: queue one area explosion at the impact (processed on Tick, never inside the event).
	void ModuleEpicUtilityMovementRuntime::OnEvent(const sas::AbilityEvent& event)
	{
		if (mShuttingDown || !mShip || !mPhaseArmed || mRules.phaseSlips.empty() || event.eventTag == GameplayTags::Event::Ability::Activated) return;
		const DamageContext* context = event.GetContext<DamageContext>();
		if (!context || context->source != mShip || context->originalDamage <= 0.f) return;
		if (context->payload.skipModuleDamageRules || context->payload.moduleTriggerPolicy == DamageModuleTriggerPolicy::Suppressed) return;
		const bool primaryShot = (context->payload.hitGroup && context->payload.hitGroup->kind == DamageHitGroup::Kind::PrimaryShot) || HasTag(event.sourceAbilityTags, GameplayTags::Ability::Primary);
		if (!primaryShot) return;
		const ResolvedModulePhaseSlipRule& rule = mRules.phaseSlips.front();
		mPhaseArmed = false;
		mPendingExplosions.push_back({ sf::Vector2f{ context->targetLocationAtResolution.x, context->targetLocationAtResolution.y }, context->originalDamage * rule.primaryDamageMultiplier, rule.explosionRadius });
	}

	void ModuleEpicUtilityMovementRuntime::ProcessExplosions()
	{
		if (mPendingExplosions.empty()) return;
		const List<PendingExplosion> due = std::move(mPendingExplosions);
		mPendingExplosions.clear();
		World* world = mShip ? mShip->GetWorld() : nullptr;
		if (!world || mShuttingDown) return;
		for (const PendingExplosion& explosion : due)
		{
			const auto targets = targeting::FindOpposingCombatants(*world, *mShip, explosion.origin, explosion.radius, false);
			for (const auto& target : targets)
			{
				if (target && !target->GetIsPendingDestroy()) ApplyCombatDamage(*target, explosion.damage, mShip, {});
			}
		}
	}

	// Assault Vector: velocity comes from the position change across ticks. Moving toward the nearest enemy adds outgoing damage,
	// moving away adds Armor; sideways or slow movement adds neither.
	void ModuleEpicUtilityMovementRuntime::TickAssaultVector(float deltaSeconds)
	{
		if (mRules.assaultVectors.empty() || !mShip || deltaSeconds <= 0.f)
		{
			ClearAssault();
			return;
		}
		const sf::Vector2f location = mShip->GetActorLocation();
		const sf::Vector2f velocity = mHasLastLocation ? (location - mLastLocation) / deltaSeconds : sf::Vector2f{ 0.f, 0.f };
		mLastLocation = location;
		mHasLastLocation = true;

		const ResolvedModuleAssaultVectorRule& first = mRules.assaultVectors.front();
		const float speed = Length(velocity);
		float damage = 0.f;
		float armor = 0.f;
		World* world = mShip->GetWorld();
		if (world && speed > 0.f && speed >= first.minimumSpeed)
		{
			const auto targets = targeting::FindOpposingCombatants(*world, *mShip, location, first.targetRange, false);
			const Actor* nearest = nullptr;
			float nearestDistance = 0.f;
			sf::Vector2f toEnemy{};
			for (const auto& target : targets)
			{
				if (!target) continue;
				const sf::Vector2f offset = target->GetActorLocation() - location;
				const float distance = Length(offset);
				if (distance <= 0.f) continue;
				if (!nearest || distance < nearestDistance)
				{
					nearest = target.get();
					nearestDistance = distance;
					toEnemy = offset / distance;
				}
			}
			if (nearest)
			{
				const float cosine = (velocity.x * toEnemy.x + velocity.y * toEnemy.y) / speed;
				for (const ResolvedModuleAssaultVectorRule& rule : mRules.assaultVectors)
				{
					if (cosine >= rule.minimumCosine) damage += rule.damageBonus;
					else if (cosine < -rule.minimumCosine) armor += rule.armorBonus;
				}
			}
		}
		ApplyAssaultDamage(damage);
		ApplyAssaultArmor(armor);
	}

	void ModuleEpicUtilityMovementRuntime::ApplyAssaultDamage(float bonus)
	{
		if (bonus == mAssaultDamageBonus) return;
		mAssaultDamageBonus = bonus;
		if (bonus > 0.f) mCombatRuntime.SetRuntimeModifier(kAssaultDamageId, CombatRuntimeModifier{ 1.f + bonus });
		else mCombatRuntime.RemoveRuntimeModifier(kAssaultDamageId);
	}

	void ModuleEpicUtilityMovementRuntime::ApplyAssaultArmor(float bonus)
	{
		if (bonus == mAssaultArmorBonus) return;
		LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
		if (mAssaultArmorHandle.IsValid()) mAbilities.GetAttributes().RemoveModifier(mAssaultArmorHandle);
		mAssaultArmorHandle = sas::AttributeModifierHandle{};
		mAssaultArmorBonus = bonus;
		if (bonus > 0.f) mAssaultArmorHandle = mAbilities.GetAttributes().AddModifier(sas::AttributeModifier{ OwnerAttributeIds::Armor, sas::AttributeModifierOperation::Add, bonus });
	}

	void ModuleEpicUtilityMovementRuntime::ClearAssault()
	{
		ApplyAssaultDamage(0.f);
		ApplyAssaultArmor(0.f);
		mHasLastLocation = false;
	}

	int ModuleEpicUtilityMovementRuntime::FieldMasteryThreshold() const
	{
		for (const ResolvedModuleFieldMasteryRule& rule : mRules.fieldMasteries)
		{
			if (rule.usesPerLevelUp > 0) return rule.usesPerLevelUp;
		}
		return 0;
	}

	void ModuleEpicUtilityMovementRuntime::AddUses(const std::string& abilityId, double amount)
	{
		for (UseCounter& counter : mUseCounters)
		{
			if (counter.abilityId != abilityId) continue;
			counter.uses += amount;
			return;
		}
		mUseCounters.push_back({ abilityId, amount });
	}

	// Field Mastery: each threshold of uses of one Utility ability grants it one run level, capped by the ability's own maximum.
	void ModuleEpicUtilityMovementRuntime::ProcessFieldMastery()
	{
		const int threshold = FieldMasteryThreshold();
		if (threshold <= 0) return;
		for (std::size_t index = 0; index < mUseCounters.size(); ++index)
		{
			if (mUseCounters[index].uses < static_cast<double>(threshold)) continue;
			const std::string abilityId = mUseCounters[index].abilityId;
			GameAbility* ability = mAbilities.FindAbilityById<GameAbility>(abilityId);
			if (!ability) continue;
			if (ability->GetLevel() >= ability->GetMaxLevel())
			{
				mUseCounters[index].uses = 0.0;
				continue;
			}
			mUseCounters[index].uses -= static_cast<double>(threshold);
			if (LevelUpAbility(abilityId)) TransferUses(abilityId);
		}
	}

	bool ModuleEpicUtilityMovementRuntime::LevelUpAbility(const std::string& abilityId)
	{
		GameAbility* ability = mAbilities.FindAbilityById<GameAbility>(abilityId);
		if (!ability || ability->GetLevel() >= ability->GetMaxLevel()) return false;
		(void)mAbilities.SetAbilityLevel(ability->GetHandle(), ability->GetLevel() + 1);
		if (mAbilityLevelSink) mAbilityLevelSink(abilityId, ability->GetLevel());
		return true;
	}

	// Knowledge Transfer (side feature): a level gained through this module adds its transfer uses to every other installed Utility ability.
	void ModuleEpicUtilityMovementRuntime::TransferUses(const std::string& sourceAbilityId)
	{
		float amount = 0.f;
		for (const ResolvedModuleFieldMasteryRule& rule : mRules.fieldMasteries) amount += rule.transferUses;
		if (amount <= 0.f) return;
		for (const sas::AbilitySlot slot : kLoadoutSlots)
		{
			const GameAbility* other = mAbilities.GetAbility(slot);
			if (!other) continue;
			const std::string otherId = other->GetDefinition().abilityId;
			if (otherId == sourceAbilityId || !HasTag(other->GetDefinition().abilityTags, GameplayTags::Ability::Utility)) continue;
			AddUses(otherId, static_cast<double>(amount));
		}
	}

	void ModuleEpicUtilityMovementRuntime::Clear()
	{
		mCascades.clear();
		ClearAssault();
		ClearPhase();
		mPendingExplosions.clear();
	}
}
