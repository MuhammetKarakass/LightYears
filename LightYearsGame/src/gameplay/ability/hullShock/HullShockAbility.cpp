#include "gameplay/ability/hullShock/HullShockAbility.h"

#include "gameplay/ability/runtime/FocusActionLocks.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/actors/AreaTelegraphActor.h"
#include "gameplay/ability/hullShock/HullShockContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/content/DamageStatusBalanceCatalog.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "gameplay/tags/GameplayTags.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/hullShock/HullShockPresentationIds.h"
#include "presentation/ability/hullShock/HullShockPresentationProfile.h"
#include "spaceShip/SpaceShip.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorSpawner.h"
#include "gameplay/ability/hullShock/HullShockDischarge.h"
#include "presentation/ability/hullShock/HullShockConePresentation.h"
#include "gameConfigs/combat/EffectStructs.h"

#include <algorithm>
#include <cmath>

namespace vega
{
	namespace
	{
		float FindValue(const sas::GameplayAttributeList& values, const sas::AttributeId& attributeId, float fallback)
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

		float Setting(const GameAbilityDefinition& definition, const char* name)
		{
			return definition.configurationContent->numericSettings.at(name);
		}

		AreaTelegraphVisualDefinition ChargeVisual(const GameAbilityDefinition& definition, const sas::GameplayAttributeList& values)
		{
			const auto* profile = PresentationProfileRegistry<HullShockPresentationProfile>::Find(HullShockPresentationIds::FocusBasic);
			AreaTelegraphVisualDefinition visual = profile ? profile->focusTelegraph : AreaTelegraphVisualDefinition{};
			visual.radialGrowthPrimaryPhaseEnd = FindValue(values, AbilityData::HullShock::Attribute::FullRadiusDuration, 1.25f) / std::max(.001f, definition.duration);
			return visual;
		}

		bool IsFinite(float value)
		{
			return std::isfinite(value);
		}

		int ResolveElectricStackCount(float chargeProgress, float electricThresholdProgress, int maxStacks)
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

		float ResolveChargedRadius(float minimumRadius, float maximumRadius, float chargeProgress, const AreaTelegraphVisualDefinition& telegraphDefinition)
		{
			const float curvedProgress = ResolveAreaTelegraphRadialProgress(
				telegraphDefinition,
				chargeProgress
			);
			return minimumRadius +
				(maximumRadius - minimumRadius) * std::clamp(curvedProgress, 0.f, 1.f);
		}
	}

	bool HullShockAbility::Validate(const GameAbilityDefinition& definition, std::string* failureReason) const
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
			AbilityData::HullShock::Attribute::MinimumChargeDamageMultiplier
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
		if (radius->baseValue <= 0.f || damage->baseValue < 0.f ||
			fullRadiusDuration->baseValue <= 0.f ||
			fullRadiusDuration->baseValue > definition.duration ||
			minimumChargeRadius->baseValue <= 0.f ||
			minimumChargeRadius->baseValue > radius->baseValue ||
			electricThreshold->baseValue < fullRadiusDuration->baseValue ||
			electricThreshold->baseValue > definition.duration ||
			minimumDamageMultiplier->baseValue < 0.f ||
			minimumDamageMultiplier->baseValue > 1.f)
		{
			if (failureReason)
			{
				*failureReason = "Hull Shock contains invalid charge or Electric values.";
			}
			return false;
		}

		if ((definition.levelProgression.empty() && definition.repeatingLevelProgression.empty()) ||
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

		const auto hasEvolve = [&](const char* id) { return std::find(definition.activeEvolveIds.begin(), definition.activeEvolveIds.end(), id) != definition.activeEvolveIds.end(); };
		const auto validSetting = [&](const char* name, float maximum)
		{
			if (!definition.configurationContent) return false;
			const auto it = definition.configurationContent->numericSettings.find(name);
			return it != definition.configurationContent->numericSettings.end() && IsFinite(it->second) && it->second > 0.f && it->second <= maximum;
		};
		using S = AbilityData::HullShock::Setting;
		const auto* capacitor = definition.FindEffectSpec(sas::ContentId{ AbilityData::HullShock::Effect::ReactiveCapacitor });
		if ((hasEvolve(AbilityData::HullShock::Evolve::DirectedDischarge) && (!validSetting(S::ConeAngleDegrees, 180.f) || !validSetting(S::DirectedDamageMultiplier, 100.f))) ||
			(hasEvolve(AbilityData::HullShock::Evolve::ReactiveCapacitor) && (!validSetting(S::RecordedDamageShieldFraction, 1.f) || !validSetting(S::MaxHealthShieldCapFraction, 1.f) || !capacitor || sas::FindAttributeValue(capacitor->attributes, DamageReductionEffectSchema::Fraction, 0.f) <= 0.f)) ||
			(hasEvolve(AbilityData::HullShock::Evolve::AftershockResonance) && (!validSetting(S::AftershockDelay, 100.f) || !validSetting(S::AftershockDamageFraction, 1.f) || !validSetting(S::AftershockElectricStacks, 4.f))))
		{
			if (failureReason) *failureReason = "Hull Shock evolves require their captured numeric settings and reduction effect spec.";
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
		mAimDirection = AbilityActorSpawner::ResolveDirection(context.owner, sas::AbilityDirectionPolicy::MouseWorld);
		if (context.instance.HasEvolve(AbilityData::HullShock::Evolve::ReactiveCapacitor))
		{
			const auto* effect = EffectData::FindGameplayEffectDefinition(AbilityData::HullShock::Effect::ReactiveCapacitor);
			const auto* authored = context.definition.FindEffectSpec(sas::ContentId{ AbilityData::HullShock::Effect::ReactiveCapacitor });
			if (!effect || !authored) return false;
			auto spec = sas::MakeGameplayEffectSpec(*effect);
			spec.duration = focusDuration;
			spec.maxStacks = 1;
			spec.attributes = authored->attributes;
			mReduction = context.abilitySystem.ApplyGameplayEffect(spec, sas::GameplayEffectSourceContext{ &context.owner, &context.instance });
			if (!mReduction.IsValid()) return false;
			mIncomingDamage = std::make_unique<IncomingDamageWindow>(context.owner);
		}

		// The common focus rule atomically blocks voluntary movement, primary fire,
		// and other ability activation while allowing rotation and world movement.
		ability::ApplyFocusActionLocks(context.abilitySystem);
		context.abilitySystem.AddOwnedTag(AbilityData::HullShock::State::Focusing);

		if (World* world = context.owner.GetWorld())
		{
			if (context.instance.HasEvolve(AbilityData::HullShock::Evolve::DirectedDischarge))
			{
				const float halfAngle = Setting(context.definition, AbilityData::HullShock::Setting::ConeAngleDegrees) * 3.14159265f / 360.f;
				mConeTelegraph = world->SpawnActor<HullShockConeVisualActor>(&context.owner, mAimDirection, halfAngle, minimumChargeRadius);
			}
			else if (const HullShockPresentationProfile* profile =
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

	void HullShockAbility::Tick(GameAbilityBehaviorContext& context, float deltaTime)
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

		if (const auto cone = mConeTelegraph.lock())
		{
			const auto values = ResolveValues(context);
			cone->SetRadius(ResolveChargedRadius(FindValue(values, AbilityData::HullShock::Attribute::MinimumChargeRadius, 300.f), FindValue(values, AbilityData::HullShock::Attribute::Radius, 600.f), progress, ChargeVisual(context.definition, values)));
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

	void HullShockAbility::End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason)
	{
		if (!mDischarged && reason == sas::AbilityEndReason::InputReleased && !context.abilitySystem.HasOwnedTag(GameplayTags::State::Effect::Control::Stunned))
		{
			Discharge(context);
		}

		EndCapacitor(context);
		ability::RemoveFocusActionLocks(context.abilitySystem);
		context.abilitySystem.RemoveOwnedTag(AbilityData::HullShock::State::Focusing);

		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
				if (!telegraph->IsInCompletionFeedback())
			{
				telegraph->Destroy();
			}
		}
		if (const auto cone = mConeTelegraph.lock(); cone && !cone->IsComplete()) cone->Destroy();
		mConeTelegraph.reset();
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
		if (!ship || !world || ship->GetIsPendingDestroy() || ship->GetHealthComponent().GetHealth() <= 0.f)
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
		const float directedMultiplier = context.instance.HasEvolve(AbilityData::HullShock::Evolve::DirectedDischarge) ? Setting(context.definition, AbilityData::HullShock::Setting::DirectedDamageMultiplier) : 1.f;
		const float damage = baseDamage * directedMultiplier * (
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
		const int electricStacks = ResolveElectricStackCount(
			chargeProgress,
			electricThresholdProgress,
			content::DamageStatusBalanceCatalog::Get().electric.maxStacks
		);
		const List<GameplayTag> damageTags =
			context.instance.GetResolvedDamageTags();
		DamagePayload payload = DamageTypeSystem::BuildPayload(damageTags, values);
		payload.mainEffectMultiplier = context.definition.mainEffectGroupMultiplier;
		payload.hitGroup = context.instance.GetDamageHitGroup();
		// Hull Shock deliberately selects 1/2/3/4 stacks from charge instead of
		// inheriting the one-stack default used by ordinary Electric weapon hits.
		payload.electricStacks = electricStacks;
		payload.criticalPolicy = DamageCriticalPolicy::Disabled;

		HullShockDischarge discharge;
		discharge.damage = damage;
		discharge.radius = radius;
		discharge.direction = mAimDirection;
		discharge.damageTags = damageTags;
		discharge.payload = payload;
		discharge.abilityId = sas::ContentId{ context.definition.abilityId };
		discharge.abilityTags = context.definition.abilityTags;
		if (context.instance.HasEvolve(AbilityData::HullShock::Evolve::DirectedDischarge)) discharge.sectorHalfAngle = Setting(context.definition, AbilityData::HullShock::Setting::ConeAngleDegrees) * 3.14159265f / 360.f;
		const float recordedDamage = mIncomingDamage ? mIncomingDamage->GetDamage() : 0.f;
		EndCapacitor(context);
		const int hits = ApplyHullShockDischarge(*ship, discharge);
		if (hits > 0 && context.instance.HasEvolve(AbilityData::HullShock::Evolve::ReactiveCapacitor))
		{
			const float gain = std::min(recordedDamage * Setting(context.definition, AbilityData::HullShock::Setting::RecordedDamageShieldFraction), ship->GetHealthComponent().GetMaxHealth() * Setting(context.definition, AbilityData::HullShock::Setting::MaxHealthShieldCapFraction));
			ship->GetShieldComponent().GrantShield(AbilityData::HullShock::Evolve::ReactiveCapacitor, gain);
		}
		if (context.instance.HasEvolve(AbilityData::HullShock::Evolve::AftershockResonance)) world->SpawnActor<HullShockAftershockActor>(ship, discharge, context.definition.configurationContent);
		if (const auto cone = mConeTelegraph.lock()) { cone->SetRadius(radius); cone->Complete(); }

		if (const shared_ptr<AreaTelegraphActor> telegraph = mTelegraph.lock())
		{
			telegraph->Complete(chargeProgress);
		}
		EmitEvent(context, AbilityData::HullShock::Event::Discharged);
	}

	void HullShockAbility::EndCapacitor(GameAbilityBehaviorContext& context)
	{
		mIncomingDamage.reset();
		if (mReduction.IsValid()) context.abilitySystem.RemoveGameplayEffect(mReduction);
		mReduction = {};
	}

	void HullShockAbility::EmitEvent(GameAbilityBehaviorContext& context, const GameplayTag& eventTag) const
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
