#include "gameplay/ability/wingSentinels/WingSentinelsAbility.h"

#include "gameplay/ability/actions/AbilityActionAttributeResolver.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "gameplay/ability/wingSentinels/WingSentinelActor.h"
#include "gameplay/ability/wingSentinels/WingSentinelsContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "presentation/ability/wingSentinels/WingSentinelsPresentationProfile.h"
#include "framework/World.h"

#include <cmath>

namespace ly
{
	namespace
	{
		float Value(const sas::GameplayAttributeList& values, const sas::AttributeId& id, float fallback)
		{
			return sas::FindAttributeValue(values, id, fallback);
		}
	}

	bool WingSentinelsAbility::Validate(const GameAbilityDefinition& definition, std::string* failureReason) const
	{
		if (definition.abilityId != AbilityData::WingSentinels::AbilityId::Basic ||
			definition.behaviorType != AbilityBehaviorType::WingSentinels ||
			definition.activationPolicy != sas::AbilityActivationPolicy::OnPressed ||
			definition.lifetimePolicy != sas::AbilityLifetimePolicy::Duration ||
			definition.duration <= 0.f || definition.cooldown <= 0.f || definition.maxCharges != 1 ||
			definition.levelProgression.size() != 14)
		{
			if (failureReason) *failureReason = "Wing Sentinels requires the shipped timed OnPressed lifecycle.";
			return false;
		}
		for (const sas::AttributeId& id : { AbilityData::WingSentinels::Attribute::DroneCount, AbilityData::WingSentinels::Attribute::SideOffset, AbilityData::WingSentinels::Attribute::BaseAttackRate, CommonAttributeIds::Range })
		{
			if (!sas::FindAttribute(definition.attributes, id))
			{
				if (failureReason) *failureReason = "Wing Sentinels is missing a required formation attribute.";
				return false;
			}
		}
		const AbilityActorDefinition* projectile = AbilityData::FindAbilityActorDefinition(AbilityData::WingSentinels::Actor::Projectile::BasicDefinitionId);
		if (!projectile)
		{
			if (failureReason) *failureReason = "Wing Sentinels requires its projectile definition.";
			return false;
		}
		if (!projectile->presentationProfileId.IsValid() || !PresentationProfileRegistry<WingSentinelsPresentationProfile>::Find(projectile->presentationProfileId.ToString()))
		{
			if (failureReason) *failureReason = "Wing Sentinels requires its presentation profile.";
			return false;
		}
		return true;
	}

	bool WingSentinelsAbility::Activate(GameAbilityBehaviorContext& context)
	{
		if (mActive || !context.owner.GetWorld()) return false;
		const AbilityActorDefinition* projectile = AbilityData::FindAbilityActorDefinition(AbilityData::WingSentinels::Actor::Projectile::BasicDefinitionId);
		if (!projectile || !projectile->presentationProfileId.IsValid()) return false;
		const WingSentinelsPresentationProfile* profile = PresentationProfileRegistry<WingSentinelsPresentationProfile>::Find(projectile->presentationProfileId.ToString());
		if (!profile) return false;
		AbilityExecutionContext execution{ &context.abilitySystem, &context.definition, nullptr, &context.instance };
		const sas::GameplayAttributeList values = AbilityActionAttributeResolver::ResolveAbilityAttributes(execution);
		if (std::round(Value(values, AbilityData::WingSentinels::Attribute::DroneCount, 0.f)) != 2.f) return false;
		const float duration = context.instance.GetActiveDuration();
		if (duration <= 0.f) return false;
		mDrones.clear();
		for (const WingSentinelActor::Side side : { WingSentinelActor::Side::Left, WingSentinelActor::Side::Right })
		{
			WingSentinelActor::Configuration config;
			config.side = side;
			config.sideOffset = Value(values, AbilityData::WingSentinels::Attribute::SideOffset, 110.f);
			config.targetingRange = Value(values, CommonAttributeIds::Range, 650.f);
			config.baseAttackRate = Value(values, AbilityData::WingSentinels::Attribute::BaseAttackRate, 1.5f);
			config.abilitySystem = &context.abilitySystem;
			config.abilityDefinition = &context.definition;
			config.abilityInstance = &context.instance;
			const weak_ptr<WingSentinelActor> drone = context.owner.GetWorld()->SpawnActor<WingSentinelActor>(&context.owner, *profile, config);
			if (const shared_ptr<WingSentinelActor> spawned = drone.lock())
			{
				spawned->SetLifeTime(duration);
				mDrones.push_back(drone);
			}
			else { DestroyDrones(); return false; }
		}
		mActive = true;
		context.abilitySystem.AddOwnedTag(AbilityData::WingSentinels::State::Active);
		EmitEvent(context, AbilityData::WingSentinels::Event::Started);
		return true;
	}

	void WingSentinelsAbility::End(GameAbilityBehaviorContext& context, sas::AbilityEndReason reason)
	{
		(void)reason;
		DestroyDrones();
		if (!mActive) return;
		mActive = false;
		context.abilitySystem.RemoveOwnedTag(AbilityData::WingSentinels::State::Active);
		EmitEvent(context, AbilityData::WingSentinels::Event::Ended);
	}

	void WingSentinelsAbility::DestroyDrones()
	{
		for (const weak_ptr<WingSentinelActor>& drone : mDrones) if (const shared_ptr<WingSentinelActor> actor = drone.lock()) actor->Destroy();
		mDrones.clear();
	}

	void WingSentinelsAbility::EmitEvent(GameAbilityBehaviorContext& context, const GameplayTag& tag) const
	{
		sas::AbilityEvent event; event.eventTag = tag; event.sourceAbilityId = sas::ContentId{ context.definition.abilityId }; event.sourceAbilityTags = context.definition.abilityTags; event.SetSource(&context.owner); event.SetTarget(&context.owner); context.abilitySystem.HandleGameplayEvent(event);
	}
}
