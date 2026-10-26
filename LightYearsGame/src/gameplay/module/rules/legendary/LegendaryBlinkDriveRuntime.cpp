#include "gameplay/module/rules/legendary/LegendaryBlinkDriveRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/movement/MovementCollisionService.h"
#include "player/PlayerSpaceShip.h"
#include "framework/MathUtility.h"
#include <algorithm>

namespace vega
{
	LegendaryBlinkDriveRuntime::LegendaryBlinkDriveRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryBlinkDriveRuntime::~LegendaryBlinkDriveRuntime() { Clear(); }

	void LegendaryBlinkDriveRuntime::Replace(const List<ResolvedModuleLegendaryBlinkDriveRule>& rules)
	{
		// An unrelated loadout rebuild must not refill charges or cancel a committed boost.
		if (!mRules.empty() && !rules.empty() && mRules.front().instanceId == rules.front().instanceId)
		{
			mRules = rules;
			mCharges = std::min(mCharges, rules.front().charges);
			mRechargeRemaining = std::min(mRechargeRemaining, rules.front().rechargeSeconds);
			if (mBoostRemaining > 0.f)
				if (auto* ship = dynamic_cast<PlayerSpaceShip*>(&mAbilities.GetOwner())) ship->GetRuntimeModifiers().Set("LegendaryBlinkDrive.Boost", { 1.f + rules.front().movementSpeedBonus });
			return;
		}
		Clear();
		mRules = rules;
		mCharges = GetMaximumCharges();
		if (IsInstalled())
			if (auto* ship = dynamic_cast<PlayerSpaceShip*>(&mAbilities.GetOwner()))
				if (auto* controls = ship->FindComponent<PlayerMovementComponent>()) controls->StopAfterburner();
	}

	bool LegendaryBlinkDriveRuntime::TryBlink(const sf::Vector2f& worldInput)
	{
		auto* ship = dynamic_cast<PlayerSpaceShip*>(&mAbilities.GetOwner());
		if (!IsInstalled() || !ship || ship->GetIsPendingDestroy() || ship->IsInPortalTransit() || mCharges <= 0 || mUseCooldownRemaining > 0.f) return false;
		if (mAbilities.HasOwnedTag(GameplayTags::State::ActionLock::MovementInput) || mAbilities.HasOwnedTag(GameplayTags::State::Effect::Control::Stunned) || mAbilities.HasOwnedTag(GameplayTags::State::Effect::Control::Staggered) || mAbilities.HasOwnedTag(GameplayTags::Status::Damage::Cryo::Frozen)) return false;
		const auto rule = mRules.front();
		sf::Vector2f direction = worldInput;
		if (GetVectorLength(direction) <= .001f) direction = ship->GetVelocity();
		if (GetVectorLength(direction) <= .001f) direction = ship->GetActorForwardDirection();
		NormalizeVector(direction);
		const auto destination = movement::ResolveTeleportDestination(*ship, ship->GetActorLocation() + direction * rule.distance);
		if (GetVectorLength(destination - ship->GetActorLocation()) <= .001f) return false;
		ship->GetMovementComponent().TeleportTo(destination);
		--mCharges;
		if (mRechargeRemaining <= 0.f) mRechargeRemaining = rule.rechargeSeconds;
		mUseCooldownRemaining = rule.useCooldownSeconds;
		mBoostRemaining = rule.boostSeconds;
		ship->GetRuntimeModifiers().Set("LegendaryBlinkDrive.Boost", { 1.f + rule.movementSpeedBonus });
		return true;
	}

	void LegendaryBlinkDriveRuntime::Tick(float deltaSeconds)
	{
		if (!IsInstalled()) return;
		const float seconds = std::max(0.f, deltaSeconds);
		mUseCooldownRemaining = std::max(0.f, mUseCooldownRemaining - seconds);
		if (mBoostRemaining > 0.f)
		{
			mBoostRemaining = std::max(0.f, mBoostRemaining - seconds);
			if (mBoostRemaining == 0.f)
				if (auto* ship = dynamic_cast<PlayerSpaceShip*>(&mAbilities.GetOwner())) ship->GetRuntimeModifiers().Remove("LegendaryBlinkDrive.Boost");
		}
		if (mCharges >= GetMaximumCharges()) return;
		mRechargeRemaining -= seconds;
		while (mRechargeRemaining <= 0.f && mCharges < GetMaximumCharges())
		{
			++mCharges;
			if (mCharges < GetMaximumCharges()) mRechargeRemaining += mRules.front().rechargeSeconds;
			else mRechargeRemaining = 0.f;
		}
	}

	void LegendaryBlinkDriveRuntime::Clear()
	{
		mRules.clear();
		mCharges = 0;
		mRechargeRemaining = mUseCooldownRemaining = mBoostRemaining = 0.f;
		if (auto* ship = dynamic_cast<PlayerSpaceShip*>(&mAbilities.GetOwner())) ship->GetRuntimeModifiers().Remove("LegendaryBlinkDrive.Boost");
	}
}
