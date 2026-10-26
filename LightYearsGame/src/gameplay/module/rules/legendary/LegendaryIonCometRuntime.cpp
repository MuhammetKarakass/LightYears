#include "gameplay/module/rules/legendary/LegendaryIonCometRuntime.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameConfigs/combat/CombatTick.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/ability/ionComet/IonCometVisualActor.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "framework/World.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include <algorithm>
#include <cmath>

namespace vega
{
	LegendaryIonCometRuntime::LegendaryIonCometRuntime(LightYearsAbilitySystemComponent& abilities, CombatRuntime& combatRuntime) : mAbilities{ abilities }, mCombatRuntime{ combatRuntime } {}
	LegendaryIonCometRuntime::~LegendaryIonCometRuntime() { Clear(); }

	void LegendaryIonCometRuntime::Replace(const List<ResolvedModuleLegendaryIonCometRule>& rules)
	{
		if (!mRules.empty() && !rules.empty() && mRules.front().instanceId == rules.front().instanceId)
		{
			mRules = rules;
			mCharge = std::min(mCharge, rules.front().maximumCharge);
			return;
		}
		Clear();
		mRules = rules;
		if (auto* ship = dynamic_cast<PlayerSpaceShip*>(&mAbilities.GetOwner())) mTeleportRevision = ship->GetMovementComponent().GetTeleportRevision();
	}

	float LegendaryIonCometRuntime::EffectiveSpeed(float speed) const
	{
		const auto& rule = mRules.front();
		if (speed <= rule.diminishingSpeedThreshold) return speed;
		const float excess = speed - rule.diminishingSpeedThreshold;
		const float bands = std::floor(excess / rule.diminishingSpeedInterval);
		const float factor = std::pow(rule.diminishingSpeedFactor, bands);
		return rule.diminishingSpeedThreshold + rule.diminishingSpeedInterval * rule.diminishingSpeedFactor * (1.f - factor) / (1.f - rule.diminishingSpeedFactor) + (excess - rule.diminishingSpeedInterval * bands) * factor * rule.diminishingSpeedFactor;
	}

	float LegendaryIonCometRuntime::Radius(float effectiveSpeed) const
	{
		const auto& rule = mRules.front();
		return rule.baseRadius + (effectiveSpeed - rule.minimumSpeed) * rule.radiusPerSpeed;
	}

	void LegendaryIonCometRuntime::ObservePlayerControl(float speedBeforeInput, bool braking, float deltaSeconds)
	{
		if (mRules.empty()) return;
		mSpeedBeforeInput = speedBeforeInput;
		mBraking = braking;
		mControlSeconds = std::max(0.f, deltaSeconds);
		mHasControlSample = true;
	}

	void LegendaryIonCometRuntime::ApplyArea(float radius, float damage, int stacks, bool burst)
	{
		auto& owner = mAbilities.GetOwner();
		if (!owner.GetWorld()) return;
		auto payload = DamageTypeSystem::BuildPayload({ DamageTypeSchema::Electric });
		payload.electricStacks = stacks;
		payload.kineticStacks = 0;
		payload.criticalPolicy = DamageCriticalPolicy::Disabled;
		payload.skipModuleDamageRules = true;
		const auto targets = targeting::FindOpposingCombatants(*owner.GetWorld(), owner, owner.GetActorLocation(), radius, false);
		for (const auto& target : targets)
		{
			if (mRules.empty() || owner.GetIsPendingDestroy()) break;
			if (target && !target->GetIsPendingDestroy()) ApplyCombatDamage(*target, damage, &owner, { DamageTypeSchema::Electric }, payload, {}, { GameplayTags::Ability::Movement }, burst ? DamageDeliveryType::Area : DamageDeliveryType::Periodic);
		}
	}

	void LegendaryIonCometRuntime::UpdateVisual()
	{
		auto* ship = dynamic_cast<PlayerSpaceShip*>(&mAbilities.GetOwner());
		if (!ship || !ship->GetWorld() || ship->GetIsPendingDestroy()) return;
		auto visual = mVisual.lock();
		if (!visual || visual->GetIsPendingDestroy())
		{
			const auto* profile = PresentationProfileRegistry<IonCometPresentationProfile>::Find(IonCometPresentationIds::Basic.ToString());
			if (!profile) return;
			visual = ship->GetWorld()->SpawnActor<IonCometVisualActor>(ship->GetWeakActor(), *profile).lock();
			mVisual = visual;
		}
		if (visual) visual->SetFieldRadius(mFieldRadius);
	}

	void LegendaryIonCometRuntime::Tick(float deltaSeconds)
	{
		if (mRules.empty()) return;
		auto* ship = dynamic_cast<PlayerSpaceShip*>(&mAbilities.GetOwner());
		if (!ship) return;
		const auto rule = mRules.front();
		const auto& movement = ship->GetMovementComponent();
		const float seconds = std::max(0.f, deltaSeconds);
		const bool available = !ship->GetIsPendingDestroy() && !ship->IsInPortalTransit();
		const float speed = available ? movement.GetCurrentMovementSpeed() : 0.f;
		const bool teleported = mTeleportRevision != movement.GetTeleportRevision();
		bool burstReleased = false;
		mTeleportRevision = movement.GetTeleportRevision();
		const bool controlledBrake = available && mHasControlSample && mBraking && mControlSeconds > 0.f && !teleported && !movement.WasMovementInterrupted() &&
			!mAbilities.HasOwnedTag(GameplayTags::State::Effect::Control::Stunned) && !mAbilities.HasOwnedTag(GameplayTags::State::Effect::Control::Staggered) &&
			!mAbilities.HasOwnedTag(GameplayTags::State::ActionLock::MovementInput) && !mAbilities.HasOwnedTag(GameplayTags::Status::Damage::Cryo::Frozen);
		if (!controlledBrake) mBrakeActive = false;
		else
		{
			if (!mBrakeActive && mSpeedBeforeInput >= rule.minimumSpeed && mCharge >= rule.maximumCharge)
			{
				mBrakeActive = true;
				mBrakeRemaining = rule.brakeWindowSeconds;
				mBrakeSpeed = mSpeedBeforeInput;
				mBrakeEffectiveSpeed = EffectiveSpeed(mBrakeSpeed);
				mBrakeRadius = Radius(mBrakeEffectiveSpeed);
			}
			if (mBrakeActive)
			{
				mBrakeRemaining -= mControlSeconds;
				if (mBrakeRemaining >= -.00001f && speed <= mBrakeSpeed * (1.f - rule.brakeSpeedLossFraction) && mCharge >= rule.maximumCharge)
				{
					mCharge = 0.f;
					burstReleased = true;
					mBrakeRemaining = -1.f;
					mLastBurstRadius = mBrakeRadius;
					mLastBurstDamage = (rule.burstDamage + rule.burstEnergyPowerCoefficient * mAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)) * mBrakeEffectiveSpeed / rule.damageSpeedDivisor;
					++mBurstCount;
					UpdateVisual();
					if (auto visual = mVisual.lock()) visual->Burst(ship->GetActorLocation(), mBrakeRadius);
					ApplyArea(mLastBurstRadius, mLastBurstDamage, rule.burstElectricStacks, true);
					if (mRules.empty()) return;
				}
			}
		}
		mHasControlSample = false;
		mEffectiveSpeed = EffectiveSpeed(speed);
		mFieldRadius = speed >= rule.minimumSpeed ? Radius(mEffectiveSpeed) : 0.f;
		if (!burstReleased && mFieldRadius > 0.f) mCharge = std::min(rule.maximumCharge, mCharge + rule.chargePerSecond * std::clamp((speed - rule.minimumSpeed) / rule.chargeSpeedSpan, 0.f, 1.f) * seconds);
		const int ticks = time::ConsumePeriodicTicks(mTickAccumulator, seconds, CombatTick::Interval);
		if (mFieldRadius > 0.f)
		{
			const float damage = (rule.tickDamage + rule.tickEnergyPowerCoefficient * mAbilities.GetAttributes().GetCurrentValue(OwnerAttributeIds::EnergyPower)) * mEffectiveSpeed / rule.damageSpeedDivisor;
			for (int index = 0; index < ticks && !mRules.empty(); ++index) ApplyArea(mFieldRadius, damage, rule.tickElectricStacks, false);
		}
		if (!mRules.empty()) UpdateVisual();
	}

	void LegendaryIonCometRuntime::Clear()
	{
		mRules.clear();
		mCharge = mFieldRadius = mEffectiveSpeed = mTickAccumulator = 0.f;
		mHasControlSample = mBrakeActive = false;
		mBrakeRemaining = mLastBurstRadius = mLastBurstDamage = 0.f;
		mBurstCount = 0;
		if (auto visual = mVisual.lock()) visual->Destroy();
		mVisual.reset();
	}
}
