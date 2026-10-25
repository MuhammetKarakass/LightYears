#include "gameplay/module/rules/EpicOffenseRuntime.h"

#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/HealthComponent.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/tags/GameplayTags.h"

#include <algorithm>
#include <cstdint>
#include <memory>

namespace vega
{
	namespace
	{
		bool HasTagMatch(const List<GameplayTag>& tags, const GameplayTag& wanted)
		{
			return std::any_of(tags.begin(), tags.end(), [&wanted](const GameplayTag& tag) { return tag.MatchesTag(wanted); });
		}

		// Thermal, Cryo, Electric, Kinetic, Energy in EpicDamageType order; -1 when the hit carries none of them.
		int DamageTypeIndex(const List<GameplayTag>& damageTags)
		{
			const GameplayTag* types[] = { &DamageTypeSchema::Thermal, &DamageTypeSchema::Cryo, &DamageTypeSchema::Electric, &DamageTypeSchema::Kinetic, &DamageTypeSchema::Energy };
			for (int index = 0; index < 5; ++index)
			{
				if (HasTagMatch(damageTags, *types[index])) return index;
			}
			return -1;
		}

		// Current health over MaxHealth in [0, 1]; a ship without health reads as full.
		float HealthRatio(Actor& owner)
		{
			const HealthComponent* health = owner.FindComponent<HealthComponent>();
			if (!health || health->GetMaxHealth() <= 0.f) return 1.f;
			return std::clamp(health->GetHealth() / health->GetMaxHealth(), 0.f, 1.f);
		}

		template<typename Entry>
		void PruneExpiredTargets(List<Entry>& entries)
		{
			entries.erase(std::remove_if(entries.begin(), entries.end(), [](const Entry& entry) { return entry.target.expired(); }), entries.end());
		}
	}

	void ResolveMonochromeCoreLoadout(EpicOffenseEffects& effects, const LightYearsAbilitySystemComponent& abilities)
	{
		effects.monochromeMultipliers.clear();
		if (effects.monochromePairMultiplier <= 1.f) return;
		List<std::pair<GameplayTag, int>> counts;
		for (const auto slot : { sas::AbilitySlot::Ability1, sas::AbilitySlot::Ability2, sas::AbilitySlot::Ability3, sas::AbilitySlot::Ability4 })
		{
			const auto* ability = abilities.GetAbility(slot);
			if (!ability) continue;
			List<GameplayTag> seen;
			for (const auto& type : ability->GetDefinition().damageTags)
			{
				if (std::find(seen.begin(), seen.end(), type) != seen.end()) continue;
				seen.push_back(type);
				const auto found = std::find_if(counts.begin(), counts.end(), [&type](const auto& entry) { return entry.first == type; });
				if (found == counts.end()) counts.push_back({ type, 1 }); else ++found->second;
			}
		}
		for (const auto& [type, count] : counts)
			if (count >= 2) effects.monochromeMultipliers.push_back({ type, effects.monochromePairMultiplier + static_cast<float>(count - 2) * effects.monochromeAdditionalAbilityBonus });
	}

	float ComputeEpicOffenseOutgoingMultiplier(EpicOffenseState& state, const EpicOffenseEffects& effects, Actor& owner, Actor& target, const List<GameplayTag>& sourceAbilityTags, const List<GameplayTag>& damageTags, DamageDeliveryType deliveryType)
	{
		if (!effects.bloodContract && !effects.alternatingCurrent && effects.monochromeMultipliers.empty()) return 1.f;
		const bool offense = HasTagMatch(sourceAbilityTags, GameplayTags::Ability::Offense);
		const bool primary = HasTagMatch(sourceAbilityTags, GameplayTags::Ability::Primary);
		const std::uint64_t targetId = target.GetUniqueID();
		const std::weak_ptr<Actor> weakTarget = target.GetWeakActor();
		float multiplier = 1.f;
		// A multi-tag hit takes the strongest matching type once; it never compounds one hit's types.
		for (const auto& [type, value] : effects.monochromeMultipliers)
			if (HasTagMatch(damageTags, type)) multiplier = std::max(multiplier, value);

		// Blood Contract: Offense damage (not periodic ticks) scales with missing health, and the hit marks the target for the heal window.
		if (effects.bloodContract && offense && deliveryType != DamageDeliveryType::Periodic)
		{
			const float missing = 1.f - HealthRatio(owner);
			multiplier *= effects.bloodFullHealthMultiplier + (effects.bloodEmptyHealthMultiplier - effects.bloodFullHealthMultiplier) * missing;
			const float window = effects.EffectiveBloodHealWindowSeconds();
			const auto mark = std::find_if(state.bloodMarks.begin(), state.bloodMarks.end(), [targetId](const EpicOffenseState::BloodMark& entry) { return entry.targetId == targetId; });
			if (mark != state.bloodMarks.end())
			{
				mark->target = weakTarget;
				mark->remainingSeconds = window;
				mark->healFraction = effects.bloodHealFraction;
			}
			else
			{
				state.bloodMarks.push_back({ weakTarget, targetId, window, effects.bloodHealFraction });
			}
		}

		// Alternating Current: a direct ability hit (not Primary, not periodic) whose damage type differs from this owner's previous direct
		// ability hit on the same enemy receives the bonus. Every direct ability hit becomes the new previous hit.
		if (effects.alternatingCurrent && deliveryType == DamageDeliveryType::Direct && !primary && !sourceAbilityTags.empty())
		{
			const int damageType = DamageTypeIndex(damageTags);
			if (damageType >= 0)
			{
				const auto previous = std::find_if(state.directHits.begin(), state.directHits.end(), [targetId](const EpicOffenseState::DirectHit& entry) { return entry.targetId == targetId; });
				if (previous != state.directHits.end())
				{
					if (previous->damageType >= 0 && previous->damageType != damageType) multiplier *= effects.alternatingMultiplier;
					previous->target = weakTarget;
					previous->damageType = damageType;
				}
				else
				{
					state.directHits.push_back({ weakTarget, targetId, damageType });
				}
			}
		}
		PruneExpiredTargets(state.directHits);
		return multiplier;
	}

	ModuleEpicOffenseRuntime::ModuleEpicOffenseRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime)
		: mAbilities{ abilities }
		, mCombatRuntime{ combatRuntime }
	{
	}

	ModuleEpicOffenseRuntime::~ModuleEpicOffenseRuntime()
	{
		Shutdown();
	}

	void ModuleEpicOffenseRuntime::Shutdown()
	{
		if (mShuttingDown) return;
		mShuttingDown = true;
		RemoveClotArmor();
		EpicOffenseState& state = mCombatRuntime.GetEpicOffenseState();
		state.prismUses = 0;
		state.stormCharges = 0;
		state.directHits.clear();
		state.bloodMarks.clear();
	}

	void ModuleEpicOffenseRuntime::Replace()
	{
		if (mShuttingDown) return;
		const EpicOffenseEffects& effects = mCombatRuntime.GetEpicOffenseEffects();
		EpicOffenseState& state = mCombatRuntime.GetEpicOffenseState();
		if (!effects.prismCore) state.prismUses = 0;
		if (!effects.stormCapacitor) state.stormCharges = 0;
		state.stormCharges = std::min(state.stormCharges, effects.stormChargesRequired);
		if (!effects.alternatingCurrent) state.directHits.clear();
		if (!effects.bloodContract) state.bloodMarks.clear();
		if (effects.clotArmorPerCost <= 0.f) RemoveClotArmor();
	}

	void ModuleEpicOffenseRuntime::OnOwnerAbilityUse(const List<GameplayTag>& tags)
	{
		if (mShuttingDown) return;
		const EpicOffenseEffects& effects = mCombatRuntime.GetEpicOffenseEffects();
		if (effects.Empty()) return;
		EpicOffenseState& state = mCombatRuntime.GetEpicOffenseState();
		const bool offense = HasTagMatch(tags, GameplayTags::Ability::Offense);

		if (effects.stormCapacitor) state.stormCharges = std::min(effects.stormChargesRequired, state.stormCharges + 1);
		if (effects.prismCore && offense) ++state.prismUses;
		if (!effects.bloodContract || !offense) return;

		HealthComponent* health = mAbilities.GetOwner().FindComponent<HealthComponent>();
		if (!health) return;
		const float current = health->GetHealth();
		// The cost never kills: it leaves at least 1 health and bypasses the shield, which health changes do not touch.
		const float cost = std::min(current * effects.bloodCostFraction, current - 1.f);
		if (cost <= 0.f) return;
		health->ChangeHealth(-cost);

		if (effects.clotArmorPerCost > 0.f)
		{
			// The grant refreshes and replaces the previous one; it never stacks.
			RemoveClotArmor();
			LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
			mClotHandle = mAbilities.GetAttributes().AddModifier(sas::AttributeModifier{ OwnerAttributeIds::Armor, sas::AttributeModifierOperation::Add, cost * effects.clotArmorPerCost });
			mClotRemainingSeconds = effects.clotArmorSeconds;
		}
	}

	void ModuleEpicOffenseRuntime::Tick(float deltaSeconds)
	{
		if (mShuttingDown || deltaSeconds <= 0.f) return;
		if (mClotHandle.IsValid())
		{
			mClotRemainingSeconds -= deltaSeconds;
			if (mClotRemainingSeconds <= 0.f) RemoveClotArmor();
		}

		EpicOffenseState& state = mCombatRuntime.GetEpicOffenseState();
		if (state.bloodMarks.empty()) return;
		HealthComponent* ownerHealth = mAbilities.GetOwner().FindComponent<HealthComponent>();
		List<EpicOffenseState::BloodMark> kept;
		for (EpicOffenseState::BloodMark& mark : state.bloodMarks)
		{
			// A marked enemy that is gone or at zero health died by anyone. It heals the owner only when the death came inside the window.
			const bool inWindow = mark.remainingSeconds > 0.f;
			mark.remainingSeconds -= deltaSeconds;
			const std::shared_ptr<Actor> target = mark.target.lock();
			const HealthComponent* targetHealth = target ? target->FindComponent<HealthComponent>() : nullptr;
			const bool dead = !target || target->GetIsPendingDestroy() || (targetHealth && targetHealth->GetHealth() <= 0.f);
			if (dead)
			{
				if (inWindow && ownerHealth && ownerHealth->GetHealth() > 0.f) ownerHealth->Regenerate(ownerHealth->GetMaxHealth() * mark.healFraction);
				continue;
			}
			if (mark.remainingSeconds > 0.f) kept.push_back(mark);
		}
		state.bloodMarks = std::move(kept);
	}

	void ModuleEpicOffenseRuntime::RemoveClotArmor()
	{
		mClotRemainingSeconds = 0.f;
		if (!mClotHandle.IsValid()) return;
		LightYearsAbilitySystemComponent::ModuleAttributeChangeScope scope{ mAbilities };
		mAbilities.GetAttributes().RemoveModifier(mClotHandle);
		mClotHandle = sas::AttributeModifierHandle{};
	}
}
