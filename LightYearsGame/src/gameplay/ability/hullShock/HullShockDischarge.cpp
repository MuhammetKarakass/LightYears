#include "gameplay/ability/hullShock/HullShockDischarge.h"
#include "gameplay/ability/hullShock/HullShockContracts.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "presentation/ability/hullShock/HullShockConePresentation.h"
#include "presentation/ability/hullShock/HullShockPresentationProfile.h"
#include "presentation/ability/hullShock/HullShockPresentationIds.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include <algorithm>

namespace vega
{
	int ApplyHullShockDischarge(Actor& owner, const HullShockDischarge& discharge)
	{
		World* world = owner.GetWorld();
		if (!world || owner.GetIsPendingDestroy()) return 0;
		const auto targets = discharge.sectorHalfAngle > 0.f
			? targeting::FindOpposingCombatantsInSector(*world, owner, owner.GetActorLocation(), discharge.direction, discharge.radius, discharge.sectorHalfAngle)
			: targeting::FindOpposingCombatants(*world, owner, discharge.radius);
		int hits = 0;
		for (const auto& target : targets)
		{
			if (!target || target->GetIsPendingDestroy()) continue;
			ApplyCombatDamage(*target, discharge.damage, &owner, discharge.damageTags, discharge.payload, discharge.abilityId, discharge.abilityTags);
			++hits;
		}
		return hits;
	}
	HullShockAftershockActor::HullShockAftershockActor(World* world, Actor* owner, const HullShockDischarge& discharge, std::shared_ptr<const AbilityConfigurationContent> configuration) : AbilityWorldActor{ world, owner }, mDischarge{ discharge }
	{
		CaptureAbilityConfiguration(std::move(configuration));
		SetSourceAbility(discharge.abilityId, discharge.abilityTags);
		SetAbilityPhysicsEnabled(false);
		SetRenderEnabled(false);
	}
	void HullShockAftershockActor::Tick(float deltaTime)
	{
		if (mFired) return;
		const auto owner = LockOwnerActor();
		const auto* ship = owner ? dynamic_cast<const SpaceShip*>(owner.get()) : nullptr;
		if (!owner || owner->GetIsPendingDestroy() || (ship && ship->GetHealthComponent().GetHealth() <= 0.f)) { Destroy(); return; }
		mElapsed += std::max(0.f, deltaTime);
		if (mElapsed < FindCapturedNumericSetting(AbilityData::HullShock::Setting::AftershockDelay).value_or(0.f)) return;
		mFired = true;
		SetActorLocation(owner->GetActorLocation());
		mDischarge.damage *= FindCapturedNumericSetting(AbilityData::HullShock::Setting::AftershockDamageFraction).value_or(0.f);
		mDischarge.payload.electricStacks = static_cast<int>(FindCapturedNumericSetting(AbilityData::HullShock::Setting::AftershockElectricStacks).value_or(0.f));
		// Resolve live cursor aim outside the old activation's aim scope.
		mDischarge.direction = AbilityActorSpawner::ResolveDirection(*owner, sas::AbilityDirectionPolicy::MouseWorld);
		ApplyHullShockDischarge(*owner, mDischarge);
		if (mDischarge.sectorHalfAngle > 0.f)
		{
			if (const auto visual = GetWorld()->SpawnActor<HullShockConeVisualActor>(owner.get(), mDischarge.direction, mDischarge.sectorHalfAngle, mDischarge.radius).lock()) visual->Complete();
		}
		else if (const auto* profile = PresentationProfileRegistry<HullShockPresentationProfile>::Find(HullShockPresentationIds::FocusBasic))
		{
			if (const auto visual = GetWorld()->SpawnActor<AreaTelegraphActor>(GetActorLocation(), mDischarge.radius, 0.f, profile->focusTelegraph).lock()) visual->Complete();
		}
		Destroy();
	}
}
