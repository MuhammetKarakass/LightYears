#include "gameplay/ability/ironcladProtocol/IroncladProtocolAbility.h"

#include "attributes/AttributeSystem.h"
#include "effects/GameplayEffectSpec.h"
#include "gameConfigs/combat/EffectStructs.h"
#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/ironcladProtocol/IroncladProtocolContracts.h"
#include "gameplay/ability/lanceDrive/LanceDriveContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/content/EffectContentCatalog.h"
#include "gameplay/content/WeaponContentCatalog.h"
#include "gameplay/ship/ShipRuntimeModifiers.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		sas::GameplayAttributeList ResolveValues(GameAbilityBehaviorContext& context)
		{
			return AbilityActionAttributeResolver::ResolveAbilityAttributes(
				AbilityExecutionContext{ &context.abilitySystem, &context.definition, nullptr, &context.instance }
			);
		}

		float FindValue(const sas::GameplayAttributeList& values, const sas::AttributeId& id, float fallback)
		{
			return sas::FindAttributeValue(values, id, fallback);
		}

		void SetBaseValue(sas::GameplayAttributeList& values, const sas::AttributeId& id, float value)
		{
			if (sas::GameplayAttribute* attribute = sas::FindAttribute(values, id))
			{
				attribute->baseValue = value;
				attribute->currentValue = value;
			}
		}

		bool HasExpectedProgression(const GameAbilityDefinition& definition)
		{
			if (definition.levelProgression.size() != 14) return false;
			for (std::size_t index = 0; index < definition.levelProgression.size(); ++index)
			{
				const AbilityLevelStep& step = definition.levelProgression[index];
				if (step.attributeModifiers.size() != 5 || !step.scalingRules.empty()) return false;
				const auto has = [&step](const sas::AttributeId& id, float magnitude)
				{
					return std::any_of(step.attributeModifiers.begin(), step.attributeModifiers.end(),
						[&id, magnitude](const sas::AttributeModifier& modifier)
						{
							return modifier.attributeId == id && modifier.operation == sas::AttributeModifierOperation::Add &&
								std::abs(modifier.magnitude - magnitude) <= 0.0001f;
						});
				};
				if (!has(AbilityData::IroncladProtocol::Attribute::MinigunBaseDamage, 6.f) ||
					!has(AbilityData::IroncladProtocol::Attribute::MinigunAttackPowerScale, 0.04f) ||
					!has(AbilityData::IroncladProtocol::Attribute::BaseDamageReduction, 0.01f) ||
					!has(AbilityData::IroncladProtocol::Attribute::MaxHealthDamageReductionScale, 0.01f) ||
					!has(CommonAttributeIds::Cooldown, -GetGlobalAbilityCooldownStepReduction(definition.cooldown, index)))
				{
					return false;
				}
			}
			return true;
		}
	}

	bool IroncladProtocolAbility::Validate(const GameAbilityDefinition& definition, std::string* failureReason) const
	{
		if (definition.abilityId != AbilityData::IroncladProtocol::AbilityId::Basic ||
			!sas::IsLoadoutAbilitySlot(definition.slot) ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.maxCharges != 1 || definition.duration != 10.f || definition.cooldown != 22.f ||
			definition.attributes.size() != 8 || !HasExpectedProgression(definition))
		{
			if (failureReason) *failureReason = "Ironclad Protocol requires an OnPressed timed form with a positive cooldown.";
			return false;
		}
		for (const sas::AttributeId& required : {
			AbilityData::IroncladProtocol::Attribute::MovementSpeedMultiplier,
			AbilityData::IroncladProtocol::Attribute::MinimumFormDuration,
			AbilityData::IroncladProtocol::Attribute::MinigunBaseDamage,
			AbilityData::IroncladProtocol::Attribute::MinigunAttackPowerScale,
			AbilityData::IroncladProtocol::Attribute::BaseDamageReduction,
			AbilityData::IroncladProtocol::Attribute::MaxHealthReference,
			AbilityData::IroncladProtocol::Attribute::MaxHealthDamageReductionScale,
			AbilityData::IroncladProtocol::Attribute::DamageReductionFalloffHealth })
		{
			if (!sas::FindAttribute(definition.attributes, required))
			{
				if (failureReason) *failureReason = "Ironclad Protocol must declare all form runtime attributes.";
				return false;
			}
		}
		return EffectData::FindGameplayEffectDefinition(AbilityData::IroncladProtocol::Effect::DamageReductionId) &&
			content::WeaponContentCatalog::FindById(AbilityData::IroncladProtocol::Weapon::MinigunId);
	}

	bool IroncladProtocolAbility::Activate(GameAbilityBehaviorContext& context)
	{
		SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner);
		const sas::GameplayEffectDefinition* effect = EffectData::FindGameplayEffectDefinition(
			AbilityData::IroncladProtocol::Effect::DamageReductionId
		);
		const PrimaryWeaponDefinition* minigunTemplate = content::WeaponContentCatalog::FindById(
			AbilityData::IroncladProtocol::Weapon::MinigunId
		);
		if (!ship || !effect || !minigunTemplate || mActive ||
			context.abilitySystem.HasOwnedTag(AbilityData::LanceDrive::State::Active))
		{
			return false;
		}

		const sas::GameplayAttributeList values = ResolveValues(context);
		mMinimumDuration = std::clamp(FindValue(values,
			AbilityData::IroncladProtocol::Attribute::MinimumFormDuration, 1.f), 0.f, context.definition.duration);
		const float movementMultiplier = std::clamp(FindValue(values,
			AbilityData::IroncladProtocol::Attribute::MovementSpeedMultiplier, 0.2f), 0.f, 1.f);
		const float maxHealth = std::max(0.f, context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::MaxHealth));
		const float reference = std::max(0.f, FindValue(values,
			AbilityData::IroncladProtocol::Attribute::MaxHealthReference, 100.f));
		const float falloff = std::max(0.001f, FindValue(values,
			AbilityData::IroncladProtocol::Attribute::DamageReductionFalloffHealth, 500.f));
		const float baseReduction = std::clamp(FindValue(values,
			AbilityData::IroncladProtocol::Attribute::BaseDamageReduction, 0.25f), 0.f, 0.95f);
		const float healthScale = std::max(0.f, FindValue(values,
			AbilityData::IroncladProtocol::Attribute::MaxHealthDamageReductionScale, 0.08f));
		const float bonus = std::max(0.f, maxHealth - reference) / falloff * healthScale;

		sas::GameplayEffectSpec effectSpec = sas::MakeGameplayEffectSpec(*effect);
		effectSpec.duration = context.definition.duration;
		effectSpec.maxStacks = 1;
		effectSpec.attributes = { sas::GameplayAttribute{
			DamageReductionEffectSchema::Fraction, std::min(0.95f, baseReduction + bonus), 0.f, 0.95f } };
		mDamageReductionHandle = context.abilitySystem.ApplyGameplayEffect(
			effectSpec, sas::GameplayEffectSourceContext{ &context.owner, &context.instance }
		);
		if (!mDamageReductionHandle.IsValid()) return false;

		if (GameAbility* primary = context.abilitySystem.GetAbility(sas::AbilitySlot::PrimaryFire);
			primary && primary->IsActive())
		{
			// The old primary must finish before the replacement runtime becomes
			// visible; otherwise one input frame could fire both weapons.
			primary->Cancel(sas::AbilityEndReason::Interrupted);
		}

		PrimaryWeaponDefinition minigun = *minigunTemplate;
		const float minigunBaseDamage = std::max(0.f, FindValue(
			values, AbilityData::IroncladProtocol::Attribute::MinigunBaseDamage, 20.f));
		const float minigunAttackPowerScale = std::max(0.f, FindValue(
			values, AbilityData::IroncladProtocol::Attribute::MinigunAttackPowerScale, 0.30f));
		const float attackPower = std::max(0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackPower));
		SetBaseValue(minigun.attributes, CommonAttributeIds::Damage,
			minigunBaseDamage + attackPower * minigunAttackPowerScale);
		SetBaseValue(minigun.attributes, CommonAttributeIds::FireRate, 8.f);
		mWeaponOverrideHandle = context.abilitySystem.PushPrimaryWeaponOverride(
			sas::ContentId{ context.definition.abilityId }, minigun
		);
		if (mWeaponOverrideHandle == 0)
		{
			context.abilitySystem.RemoveGameplayEffect(mDamageReductionHandle);
			mDamageReductionHandle = {};
			return false;
		}

		ship->GetRuntimeModifiers().Set(context.definition.abilityId, ShipRuntimeModifier{ movementMultiplier });
		const float attackSpeed = std::max(0.f,
			context.abilitySystem.GetAttributes().GetCurrentValue(OwnerAttributeIds::AttackSpeed));
		mFireRateModifierSourceId = 0x49524F4E00000000ull |
			static_cast<temporal::RateModifierSourceId>(context.owner.GetUniqueID());
		ship->SetPrimaryWeaponFireRateModifier(mFireRateModifierSourceId, 1.f + attackSpeed / 200.f);
		const std::string ownAbilityId = context.definition.abilityId;
		mActivationGuard = context.abilitySystem.RegisterAbilityActivationGuard(
			[ownAbilityId](const sas::AbilityLifecycleEvent& event)
			{
				return event.slot == sas::AbilitySlot::PrimaryFire || event.abilityId == ownAbilityId;
			}
		);
		mElapsed = 0.f;
		mCancelAvailable = false;
		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::IroncladProtocol::State::Active);
		EmitEvent(context, AbilityData::IroncladProtocol::Event::Started);
		return true;
	}

	bool IroncladProtocolAbility::OnInputPressed(GameAbilityBehaviorContext& context)
	{
		if (!mActive || !mCancelAvailable)
		{
			return mActive;
		}
		context.instance.Cancel(sas::AbilityEndReason::Cancelled);
		return true;
	}

	void IroncladProtocolAbility::Tick(GameAbilityBehaviorContext& context, float deltaTime)
	{
		if (!mActive) return;
		mElapsed += std::max(0.f, deltaTime);
		if (!mCancelAvailable && mElapsed >= mMinimumDuration)
		{
			mCancelAvailable = true;
			context.abilitySystem.AddOwnedTag(AbilityData::IroncladProtocol::State::CancelAvailable);
			EmitEvent(context, AbilityData::IroncladProtocol::Event::CancelAvailable);
		}
	}

	void IroncladProtocolAbility::End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason)
	{
		(void)reason;
		ClearRuntimeState(context);
		EmitEvent(context, AbilityData::IroncladProtocol::Event::Ended);
	}

	void IroncladProtocolAbility::ClearRuntimeState(GameAbilityBehaviorContext& context)
	{
		if (!mActive) return;
		if (GameAbility* primary = context.abilitySystem.GetAbility(sas::AbilitySlot::PrimaryFire);
			primary && primary->IsActive())
		{
			// End the temporary firing lifecycle before deleting its override runtime.
			primary->Cancel(sas::AbilityEndReason::Interrupted);
		}
		if (mWeaponOverrideHandle != 0)
		{
			context.abilitySystem.RemovePrimaryWeaponOverride(mWeaponOverrideHandle);
			mWeaponOverrideHandle = 0;
		}
		if (mDamageReductionHandle.IsValid())
		{
			context.abilitySystem.RemoveGameplayEffect(mDamageReductionHandle);
			mDamageReductionHandle = {};
		}
		if (SpaceShip* ship = dynamic_cast<SpaceShip*>(&context.owner))
		{
			ship->GetRuntimeModifiers().Remove(context.definition.abilityId);
			ship->RemovePrimaryWeaponFireRateModifier(mFireRateModifierSourceId);
		}
		mFireRateModifierSourceId = 0;
		if (mActivationGuard.IsValid())
		{
			context.abilitySystem.UnregisterAbilityActivationGuard(mActivationGuard);
			mActivationGuard = {};
		}
		context.abilitySystem.RemoveOwnedTag(AbilityData::IroncladProtocol::State::Active);
		context.abilitySystem.RemoveOwnedTag(AbilityData::IroncladProtocol::State::CancelAvailable);
		mElapsed = 0.f;
		mCancelAvailable = false;
		mActive = false;
	}

	void IroncladProtocolAbility::EmitEvent(GameAbilityBehaviorContext& context, const GameplayTag& eventTag) const
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
