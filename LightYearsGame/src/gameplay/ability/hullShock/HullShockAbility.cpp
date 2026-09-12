#include "gameplay/ability/hullShock/HullShockAbility.h"

#include "gameplay/ability/runtime/FocusActionLocks.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/hullShock/HullShockContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/hullShock/HullShockPresentationIds.h"
#include "presentation/ability/hullShock/HullShockPresentationProfile.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		float FindValue(
			const sas::GameplayAttributeList& values,
			const sas::AttributeId& attributeId,
			float fallback
		)
		{
			return sas::FindAttributeValue(values, attributeId, fallback);
		}

		sas::GameplayAttributeList ResolveValues(GameAbilityBehaviorContext& context)
		{
			AbilityExecutionContext executionContext{
				&context.abilitySystem,
				&context.definition,
				nullptr,
				&context.instance
			};
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(executionContext);
		}

		bool IsFinite(float value)
		{
			return std::isfinite(value);
		}

		int ResolveElectricStackCount(
			float chargeProgress,
			float electricThresholdProgress,
			int maxStacks
		)
		{
			const int safeMaxStacks = std::max(1, maxStacks);
			const float progress = std::clamp(chargeProgress, 0.f, 1.f);
			if (progress < 0.30f)
			{
				return std::min(1, safeMaxStacks);
			}
			if (progress < 0.60f)
			{
				return std::min(2, safeMaxStacks);
			}
			if (progress < std::clamp(electricThresholdProgress, 0.f, 1.f))
			{
				return std::min(3, safeMaxStacks);
			}
			return safeMaxStacks;
		}

		float ResolveChargedRadius(
			float minimumRadius,
			float maximumRadius,
			float chargeProgress,
			const AreaTelegraphVisualDefinition& telegraphDefinition
		)
		{
			const float curvedProgress = ResolveAreaTelegraphRadialProgress(
				telegraphDefinition,
				chargeProgress
			);
			return minimumRadius +
				(maximumRadius - minimumRadius) * std::clamp(curvedProgress, 0.f, 1.f);
		}
	}

	bool HullShockAbility::Validate(
		const GameAbilityDefinition& definition,
		std::string* failureReason
	) const
	{
		if (definition.abilityId != AbilityData::HullShock::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::WhileHeld ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::WhileInputHeld ||
			definition.maxCharges != 1 ||
			!IsFinite(definition.cooldown) || definition.cooldown <= 0.f ||
			!IsFinite(definition.duration) || definition.duration <= 0.f)
		{
			if (failureReason)
			{
				*failureReason =
					"Hull Shock requires a loadout slot, held activation, and one charge.";
			}
			return false;
		}

		for (const sas::AttributeId& required : {
			AbilityData::HullShock::Attribute::Radius,
			AbilityData::HullShock::Attribute::Damage,
			AbilityData::HullShock::Attribute::FullRadiusDuration,
			AbilityData::HullShock::Attribute::MinimumChargeRadius,
			AbilityData::HullShock::Attribute::ElectricChargeThreshold,
			AbilityData::HullShock::Attribute::MinimumChargeDamageMultiplier,
			AbilityData::HullShock::Attribute::ElectricDamageTakenMultiplierPerStack,
			AbilityData::HullShock::Attribute::ElectricDuration,
			AbilityData::HullShock::Attribute::ElectricMaxStacks
		})
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes,
				required
			);
			if (!attribute || !IsFinite(attribute->baseValue))
			{
				if (failureReason)
				{
					*failureReason = "Hull Shock must declare every runtime attribute.";
				}
				return false;
			}
		}

		const sas::GameplayAttribute* radius = sas::FindAttribute(
			definition.attributes,
			AbilityData::HullShock::Attribute::Radius
		);
		const sas::GameplayAttribute* damage = sas::FindAttribute(
			definition.attributes,
			AbilityData::HullShock::Attribute::Damage
		);
		const sas::GameplayAttribute* fullRadiusDuration = sas::FindAttribute(
			definition.attributes,
			AbilityData::HullShock::Attribute::FullRadiusDuration
		);
		const sas::GameplayAttribute* minimumChargeRadius = sas::FindAttribute(
			definition.attributes,
			AbilityData::HullShock::Attribute::MinimumChargeRadius
		);
		const sas::GameplayAttribute* electricThreshold = sas::FindAttribute(
			definition.attributes,
			AbilityData::HullShock::Attribute::ElectricChargeThreshold
		);
		const sas::GameplayAttribute* minimumDamageMultiplier = sas::FindAttribute(
			definition.attributes,
			AbilityData::HullShock::Attribute::MinimumChargeDamageMultiplier
		);
		const sas::GameplayAttribute* electricMultiplier = sas::FindAttribute(
			definition.attributes,
			AbilityData::HullShock::Attribute::ElectricDamageTakenMultiplierPerStack
		);
		const sas::GameplayAttribute* electricDuration = sas::FindAttribute(
			definition.attributes,
			AbilityData::HullShock::Attribute::ElectricDuration
		);
		const sas::GameplayAttribute* electricMaxStacks = sas::FindAttribute(
			definition.attributes,
			AbilityData::HullShock::Attribute::ElectricMaxStacks
		);
		if (radius->baseValue <= 0.f || damage->baseValue < 0.f ||
			fullRadiusDuration->baseValue <= 0.f ||
			fullRadiusDuration->baseValue > definition.duration ||
			minimumChargeRadius->baseValue <= 0.f ||
			minimumChargeRadius->baseValue > radius->baseValue ||
			electricThreshold->baseValue < fullRadiusDuration->baseValue ||
			electricThreshold->baseValue > definition.duration ||
			minimumDamageMultiplier->baseValue < 0.f ||
			minimumDamageMultiplier->baseValue > 1.f ||
			electricMultiplier->baseValue < 0.f ||
			electricDuration->baseValue <= 0.f ||
			electricMaxStacks->baseValue < 1.f ||
			std::round(electricMaxStacks->baseValue) != electricMaxStacks->baseValue)
		{
			if (failureReason)
			{
				*failureReason = "Hull Shock contains invalid charge or Electric values.";
			}
			return false;
		}

		if (definition.levelProgression.size() != 14 ||
			definition.damageTags.size() != 1 ||
			definition.damageTags.front() != DamageTypeSchema::Electric)
		{
			if (failureReason)
			{
				*failureReason =
					"Hull Shock requires fourteen levels and exactly the Electric damage type.";
			}
			return false;
		}

		return true;
	}

	bool HullShockAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (!dynamic_cast<SpaceShip*>(&context.owner) || mDischarged)
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float radius = std::max(
			1.f,
			FindValue(values, AbilityData::HullShock::Attribute::Radius, 600.f)
		);
		const float minimumChargeRadius = std::clamp(
			FindValue(
				values,
				AbilityData::HullShock::Attribute::MinimumChargeRadius,
				300.f
			),
			1.f,
			radius
		);
		const float focusDuration = std::max(0.001f, context.definition.duration);
		const float fullRadiusDuration = std::clamp(
			FindValue(
				values,
				AbilityData::HullShock::Attribute::FullRadiusDuration,
				1.25f
			),
			0.f,
			focusDuration
		);
		mFocusElapsed = 0.f;
		mDischarged = false;

		// The common focus rule atomically blocks voluntary movement, primary fire,
		// and other ability activation while allowing rotation and world movement.
		ability::ApplyFocusActionLocks(context.abilitySystem);
		context.abilitySystem.AddOwnedTag(AbilityData::HullShock::State::Focusing);

		if (World* world = context.owner.GetWorld())
		{
			if (const HullShockPresentationProfile* profile =
				PresentationProfileRegistry<HullShockPresentationProfile>::Find(
					HullShockPresentationIds::FocusBasic
				))
			{
				AreaTelegraphVisualDefinition telegraphDefinition = profile->focusTelegraph;
				// The profile supplies the reusable curve; the ability's balance
				// attribute supplies the exact point where its radius becomes full.
				telegraphDefinition.radialGrowthPrimaryPhaseEnd =
					fullRadiusDuration / focusDuration;
				telegraphDefinition.minimumFillRadiusRatio =
					minimumChargeRadius / radius;
				mTelegraph = world->SpawnActor<AreaTelegraphActor>(
					AreaTelegraphActor::SpawnParams{
						context.owner.GetActorLocation(),
						radius,
						0.f,
						telegraphDefinition,
						AreaTelegraphAnchorMode::FollowActor,
						AreaTelegraphProgressDriver::External,
						&context.owner
					}
				);
			}
		}

		EmitEvent(context, AbilityData::HullShock::Event::Started);
		return true;
	}

	void HullShockAbility::Tick(
		GameAbilityBehaviorContext& context,
		float deltaTime
	)
	{
		if (mDischarged)
		{
			return;
		}

		mFocusElapsed += std::max(0.f, deltaTime);
		const float focusDuration = std::max(0.001f, context.definition.duration);
		const float progress = std::clamp(mFocusElapsed / focusDuration, 0.f, 1.f);
		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->SetExternalProgress(progress);
		}

		if (progress < 1.f)
		{
			return;
		}

		mFocusElapsed = focusDuration;
		Discharge(context);
		// WhileInputHeld waits for release, so the maximum focus time must end
		// the ability explicitly once the automatic discharge has happened.
		context.instance.Cancel(sas::AbilityEndReason::Completed);
	}

	void HullShockAbility::End(
		GameAbilityBehaviorContext& context,
		sas::AbilityEndReason reason
	)
	{
		if (!mDischarged && reason == sas::AbilityEndReason::InputReleased)
		{
			Discharge(context);
		}

		ability::RemoveFocusActionLocks(context.abilitySystem);
		context.abilitySystem.RemoveOwnedTag(AbilityData::HullShock::State::Focusing);

		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
				if (!telegraph->IsInCompletionFeedback())
			{
				telegraph->Destroy();
			}
		}
		mTelegraph.reset();
		mFocusElapsed = 0.f;
		mDischarged = false;
		EmitEvent(context, AbilityData::HullShock::Event::Ended);
	}

	void HullShockAbility::Discharge(GameAbilityBehaviorContext& context)
	{
		if (mDischarged)
		{
			return;
		}
		mDischarged = true;

		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		World* world = context.owner.GetWorld();
		if (!ship || !world)
		{
			return;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		const float focusDuration = std::max(0.001f, context.definition.duration);
		const float chargeProgress = std::clamp(
			mFocusElapsed / focusDuration,
			0.f,
			1.f
		);
		const float baseDamage = std::max(
			0.f,
			FindValue(values, AbilityData::HullShock::Attribute::Damage, 30.f)
		);
		const float minimumDamageMultiplier = std::clamp(
			FindValue(
				values,
				AbilityData::HullShock::Attribute::MinimumChargeDamageMultiplier,
				0.25f
			),
			0.f,
			1.f
		);
		const float damage = baseDamage * (
			minimumDamageMultiplier +
			chargeProgress * (1.f - minimumDamageMultiplier)
		);
		const float maximumRadius = std::max(
			1.f,
			FindValue(values, AbilityData::HullShock::Attribute::Radius, 600.f)
		);
		const float minimumChargeRadius = std::clamp(
			FindValue(
				values,
				AbilityData::HullShock::Attribute::MinimumChargeRadius,
				300.f
			),
			1.f,
			maximumRadius
		);
		const float fullRadiusDuration = std::clamp(
			FindValue(
				values,
				AbilityData::HullShock::Attribute::FullRadiusDuration,
				1.25f
			),
			0.f,
			focusDuration
		);
		AreaTelegraphVisualDefinition telegraphDefinition;
		telegraphDefinition.radialGrowthLogStrength = 2.f;
		telegraphDefinition.radialGrowthPrimaryPhaseFill = 1.f;
		if (const HullShockPresentationProfile* profile =
			PresentationProfileRegistry<HullShockPresentationProfile>::Find(
				HullShockPresentationIds::FocusBasic
			))
		{
			telegraphDefinition = profile->focusTelegraph;
		}
		telegraphDefinition.radialGrowthPrimaryPhaseEnd = fullRadiusDuration / focusDuration;
		const float radius = std::max(
			1.f,
			ResolveChargedRadius(
				minimumChargeRadius,
				maximumRadius,
				chargeProgress,
				telegraphDefinition
			)
		);
		const float electricThresholdProgress = std::clamp(
			FindValue(
				values,
				AbilityData::HullShock::Attribute::ElectricChargeThreshold,
				1.8f
			) / focusDuration,
			0.f,
			1.f
		);
		const int electricMaxStacks = std::max(
			1,
			static_cast<int>(std::lround(FindValue(
				values,
				AbilityData::HullShock::Attribute::ElectricMaxStacks,
				4.f
			)))
		);
		const int electricStacks = ResolveElectricStackCount(
			chargeProgress,
			electricThresholdProgress,
			electricMaxStacks
		);
		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags(AttachmentHostKind::Ability);
		DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags, values);
		// The definition owns these values in the HullShock family namespace;
		// project them into the shared combat payload at the boundary.
		payload.electricDamageTakenMultiplierPerStack = std::clamp(
			FindValue(
				values,
				AbilityData::HullShock::Attribute::ElectricDamageTakenMultiplierPerStack,
				0.04f
			),
			0.f,
			1.f
		);
		payload.electricDuration = std::max(
			0.f,
			FindValue(
				values,
				AbilityData::HullShock::Attribute::ElectricDuration,
				3.f
			)
		);
		// Hull Shock deliberately selects 1/2/3/4 stacks from charge instead of
		// inheriting the one-stack default used by ordinary Electric weapon hits.
		payload.electricStacks = electricStacks;
		payload.electricMaxStacks = electricMaxStacks;
		payload.criticalPolicy = DamageCriticalPolicy::Disabled;

		for (const shared_ptr<Actor>& target : targeting::FindOpposingCombatants(
			*world,
			*ship,
			radius
		))
		{
			if (target && !target->GetIsPendingDestroy())
			{
				ApplyCombatDamage(
					*target,
					damage,
					ship,
					damageTags,
					payload,
					sas::ContentId{ context.definition.abilityId },
					context.definition.abilityTags
				);
			}
		}

		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->Complete(chargeProgress);
		}
		EmitEvent(context, AbilityData::HullShock::Event::Discharged);
	}

	void HullShockAbility::EmitEvent(
		GameAbilityBehaviorContext& context,
		const GameplayTag& eventTag
	) const
	{
		sas::AbilityEvent event;
		event.eventTag = eventTag;
		event.sourceAbilityId = sas::ContentId{ context.definition.abilityId };
		event.sourceAbilityTags = context.definition.abilityTags;
		event.SetSource(&context.owner);
		event.SetTarget(&context.owner);
		context.abilitySystem.HandleGameplayEvent(event);
	}
}
