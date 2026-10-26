#include "gameplay/module/rules/legendary/LegendarySecondHardpointRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "framework/Actor.h"
#include <algorithm>

namespace vega
{
	LegendarySecondHardpointRuntime::LegendarySecondHardpointRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendarySecondHardpointRuntime::~LegendarySecondHardpointRuntime() { Clear(); }

	void LegendarySecondHardpointRuntime::Replace(const List<ResolvedModuleLegendarySecondHardpointRule>& rules)
	{
		if (!mRules.empty() && !rules.empty() && mRules.front().instanceId == rules.front().instanceId)
		{
			mRules = rules;
			return;
		}
		Clear();
		mRules = rules;
		if (!mRules.empty()) mAbilities.SetSecondHardpointRuntime(this);
	}

	void LegendarySecondHardpointRuntime::Tick(float deltaSeconds)
	{
		if (mRules.empty() || deltaSeconds <= 0.f) return;
		mSwitchRemaining = std::max(0.f, mSwitchRemaining - deltaSeconds);
		for (auto it = mMarks.begin(); it != mMarks.end();)
		{
			it->second.first = std::max(0.f, it->second.first - deltaSeconds);
			it->second.second = std::max(0.f, it->second.second - deltaSeconds);
			const auto target = it->second.target.lock();
			if (!target || target->GetIsPendingDestroy() || (it->second.first == 0.f && it->second.second == 0.f)) it = mMarks.erase(it);
			else ++it;
		}
		const auto* primary = mAbilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		if (mSecondWeapon && !mSecondState->runtime.isFiring && (!mSecondActive || !primary || !primary->IsActive()))
		{
			PrimaryWeaponExecutionContext context{ mAbilities.GetOwner(), *mSecondWeapon, mSecondState->attributes, mSecondWeapon->damageTags };
			PrimaryWeaponExecutionSystem::TickInactive(context, mSecondState->runtime, deltaSeconds);
		}
		if (mSecondActive && primary && primary->IsActive())
		{
			for (const auto& action : primary->GetDefinition().actions)
			{
				const auto* fire = std::get_if<FireWeaponAction>(&action.action);
				if (!fire) continue;
				AbilityExecutionContext resolve{ &mAbilities, &primary->GetDefinition() };
				const auto attributes = AbilityActionAttributeResolver::ResolveAttributes(resolve, &fire->weaponDefinition, fire->weaponDefinition.attributes);
				PrimaryWeaponExecutionContext context{ mAbilities.GetOwner(), fire->weaponDefinition, attributes, fire->weaponDefinition.damageTags };
				PrimaryWeaponExecutionSystem::TickInactive(context, mAbilities.GetAbility(sas::AbilitySlot::PrimaryFire)->GetPrimaryWeaponRuntime(), deltaSeconds);
				break;
			}
		}
	}

	bool LegendarySecondHardpointRuntime::AssignSecondWeapon(const std::string& weaponId)
	{
		if (mRules.empty() || mSecondActive || IsSwitching()) return false;
		const auto* definition = content::WeaponContentCatalog::FindById(weaponId);
		const auto* primary = mAbilities.GetAbility(sas::AbilitySlot::PrimaryFire);
		if (!definition || !primary) return false;
		for (const auto& action : primary->GetDefinition().actions)
			if (const auto* fire = std::get_if<FireWeaponAction>(&action.action); fire && fire->weaponDefinition.weaponId == weaponId) return false;
		PrimaryWeaponAbilityState candidate;
		if (!PrimaryWeaponExecutionSystem::InitializeRuntime(*definition, candidate.runtime).isValid) return false;
		candidate.weaponId = weaponId;
		candidate.attributes = definition->attributes;
		candidate.damageTags = definition->damageTags;
		mSecondWeapon = *definition;
		mSecondState = std::make_shared<PrimaryWeaponAbilityState>(std::move(candidate));
		mMarks.clear();
		return true;
	}

	bool LegendarySecondHardpointRuntime::SwitchWeapon()
	{
		if (mRules.empty() || !mSecondWeapon || IsSwitching() || mAbilities.GetActivePrimaryWeaponOverride() ||
			!mAbilities.IsModuleSystemEnabled(GameplayTags::Module::System::PrimaryWeapon) ||
			mAbilities.HasOwnedTag(GameplayTags::State::Effect::Control::Stunned)) return false;
		// End the old action while its selected definition and runtime still agree.
		if (auto* primary = mAbilities.GetAbility(sas::AbilitySlot::PrimaryFire); primary && primary->IsActive())
			primary->Cancel(sas::AbilityEndReason::Interrupted);
		mSecondActive = !mSecondActive;
		mSwitchRemaining = mRules.front().switchSeconds;
		return true;
	}

	float LegendarySecondHardpointRuntime::ResolveDirectHit(Actor& target, const sas::ContentId& weaponId)
	{
		if (mRules.empty() || !mSecondWeapon || weaponId.ToString().empty()) return 1.f;
		bool second = weaponId.ToString() == mSecondWeapon->weaponId;
		if (!second)
		{
			const auto* primary = mAbilities.GetAbility(sas::AbilitySlot::PrimaryFire);
			bool first = false;
			if (primary) for (const auto& action : primary->GetDefinition().actions)
				if (const auto* fire = std::get_if<FireWeaponAction>(&action.action)) first = first || fire->weaponDefinition.weaponId == weaponId.ToString();
			if (!first) return 1.f;
		}
		auto& mark = mMarks[target.GetUniqueID()];
		mark.target = std::dynamic_pointer_cast<Actor>(target.GetWeakPtr().lock());
		const bool bonus = (second ? mark.second : mark.first) > 0.f;
		(second ? mark.first : mark.second) = mRules.front().markSeconds;
		return bonus ? 1.f + mRules.front().markDamageBonus : 1.f;
	}

	void LegendarySecondHardpointRuntime::AdvanceReserveReload(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float deltaSeconds)
	{
		if (mRules.empty() || !mSecondWeapon || !context.definition.magazine || state.magazineState.reloadRemaining > 0.0 ||
			state.magazineState.roundsRemaining >= context.definition.magazine->capacity) return;
		if (state.magazineState.reserveReloadRemaining <= 0.0)
			state.magazineState.reserveReloadRemaining = PrimaryWeaponExecutionSystem::CalculateReloadDuration(context.definition.magazine->baseReloadTime, context.owner);
		state.magazineState.reserveReloadRemaining = std::max(0.0, state.magazineState.reserveReloadRemaining - deltaSeconds);
		if (state.magazineState.reserveReloadRemaining <= 0.0)
		{
			state.magazineState.roundsRemaining = context.definition.magazine->capacity;
			state.magazineState.shotsFired = 0;
		}
	}

	void LegendarySecondHardpointRuntime::Clear()
	{
		if (mAbilities.GetSecondHardpointRuntime() == this)
		{
			if (mSecondActive)
				if (auto* primary = mAbilities.GetAbility(sas::AbilitySlot::PrimaryFire); primary && primary->IsActive()) primary->Cancel(sas::AbilityEndReason::Interrupted);
			mAbilities.SetSecondHardpointRuntime(nullptr);
		}
		mSecondActive = false;
		mSwitchRemaining = 0.f;
		mMarks.clear();
		mSecondState.reset();
		mSecondWeapon.reset();
		mRules.clear();
	}
}
