#include "effects/GameplayEffectDefinitionValidation.h"
#include "effects/GameplayEffectSystem.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
	int Fail(const char* message)
	{
		std::cerr << message << '\n';
		return 1;
	}

	sas::GameplayEffectDefinition MakeDecayEffect(
		const std::string& effectId,
		float duration = 2.f,
		int maxStacks = 4,
		float decayInterval = 1.f
	)
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = effectId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::Stack;
		definition.duration = duration;
		definition.maxStacks = maxStacks;
		definition.stackLifetimePolicy =
			sas::GameplayEffectStackLifetimePolicy::DecayAfterDuration;
		definition.stackDecayInterval = decayInterval;
		return definition;
	}

	sas::GameplayEffectDefinition MakeRefreshEffect(const std::string& effectId)
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = effectId;
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
		definition.duration = 2.f;
		return definition;
	}

	struct Probe
	{
		std::vector<std::string> events;
		std::vector<int> changedStacks;
		std::vector<int> stackChangedStacks;
		std::vector<int> tickStacks;
		int removedCount = 0;
	};

	void InstallProbe(
		sas::GameplayEffectSystem& runtime,
		Probe& probe
	)
	{
		sas::GameplayEffectRuntimeCallbacks<sas::ActiveGameplayEffect> callbacks;
		callbacks.tick = [&probe](sas::ActiveGameplayEffect& effect, float)
		{
			probe.tickStacks.push_back(effect.stackCount);
			probe.events.push_back("tick");
			return sas::GameplayEffectBehaviorResult{};
		};
		callbacks.stackChanged = [&probe](sas::ActiveGameplayEffect& effect)
		{
			probe.stackChangedStacks.push_back(effect.stackCount);
			probe.events.push_back("stackChanged");
		};
		callbacks.changed = [&probe](sas::ActiveGameplayEffect& effect)
		{
			probe.changedStacks.push_back(effect.stackCount);
			probe.events.push_back("changed");
		};
		callbacks.removed = [&probe](sas::GameplayEffectHandle)
		{
			++probe.removedCount;
			probe.events.push_back("removed");
		};
		runtime.SetCallbacks(std::move(callbacks));
	}

	void ClearProbe(Probe& probe)
	{
		probe.events.clear();
		probe.changedStacks.clear();
		probe.stackChangedStacks.clear();
		probe.tickStacks.clear();
		probe.removedCount = 0;
	}
}

int main()
{
	std::string failureReason;
	sas::GameplayEffectDefinition validDefinition = MakeDecayEffect(
		"Effect.Test.Decay"
	);
	if (!sas::ValidateGameplayEffectDefinition(validDefinition, &failureReason))
	{
		return Fail("Valid stack decay definition was rejected");
	}
	validDefinition.stackDecayInterval = 0.f;
	if (sas::ValidateGameplayEffectDefinition(validDefinition, &failureReason))
	{
		return Fail("Invalid stack decay interval was accepted");
	}
	validDefinition.stackDecayInterval = 1.f;
	validDefinition.stackingPolicy = sas::GameplayEffectStackingPolicy::RefreshDuration;
	if (sas::ValidateGameplayEffectDefinition(validDefinition, &failureReason))
	{
		return Fail("Decay policy was accepted for a non-Stack effect");
	}

	sas::AttributeSystem attributes;
	ly::GameplayTagContainer ownedTags;
	sas::GameplayEffectSystem runtime{ attributes, ownedTags };
	Probe probe;
	InstallProbe(runtime, probe);
	const sas::GameplayEffectDefinition decayDefinition = MakeDecayEffect(
		"Effect.Test.Decay"
	);
	const sas::GameplayEffectHandle handle = runtime.ApplyEffect(decayDefinition);
	if (!handle.IsValid())
	{
		return Fail("Decay effect could not be applied");
	}
	for (int stack = 1; stack < decayDefinition.maxStacks; ++stack)
	{
		runtime.ApplyEffect(decayDefinition);
	}
	const sas::ActiveGameplayEffect* active = runtime.FindEffect(handle);
	if (!active || active->stackCount != 4 || active->remainingDuration != 2.f)
	{
		return Fail("Decay effect did not reach its configured stack cap");
	}

	runtime.Tick(1.5f);
	active = runtime.FindEffect(handle);
	if (!active || active->stackCount != 4 || active->remainingDuration != 0.5f)
	{
		return Fail("Decay effect changed before its full duration elapsed");
	}
	runtime.ApplyEffect(decayDefinition);
	active = runtime.FindEffect(handle);
	if (!active || active->stackCount != 4 || active->remainingDuration != 2.f)
	{
		return Fail("A capped stack hit did not refresh the full duration");
	}

	ClearProbe(probe);
	runtime.Tick(2.1f);
	active = runtime.FindEffect(handle);
	if (!active || active->stackCount != 3 || active->remainingDuration < 0.89f ||
		active->remainingDuration > 0.91f || probe.stackChangedStacks.size() != 1 ||
		probe.changedStacks.size() != 1 || probe.changedStacks.front() != 3 ||
		probe.tickStacks.size() != 2 || probe.tickStacks[0] != 4 || probe.tickStacks[1] != 3)
	{
		return Fail("Decay did not remove one stack and notify changed at duration expiry");
	}
	runtime.ApplyEffect(decayDefinition);
	active = runtime.FindEffect(handle);
	if (!active || active->stackCount != 4 || active->remainingDuration != 2.f)
	{
		return Fail("A hit during decay did not add from the current stack count");
	}

	ClearProbe(probe);
	runtime.Tick(4.1f);
	active = runtime.FindEffect(handle);
	if (!active || active->stackCount != 1 || active->remainingDuration < 0.89f ||
		active->remainingDuration > 0.91f || probe.stackChangedStacks.size() != 3 ||
		probe.stackChangedStacks[0] != 3 || probe.stackChangedStacks[1] != 2 ||
		probe.stackChangedStacks[2] != 1 || probe.changedStacks.size() != 3 ||
		probe.changedStacks[0] != 3 || probe.changedStacks[1] != 2 ||
		probe.changedStacks[2] != 1 || probe.tickStacks.size() != 4 ||
		probe.tickStacks[0] != 4 || probe.tickStacks[1] != 3 ||
		probe.tickStacks[2] != 2 || probe.tickStacks[3] != 1)
	{
		return Fail("A multi-second tick did not apply the required number of decays");
	}

	ClearProbe(probe);
	runtime.Tick(1.f);
	if (runtime.FindEffect(handle) || probe.stackChangedStacks.size() != 1 ||
		probe.changedStacks.size() != 1 || probe.changedStacks.front() != 0 ||
		probe.removedCount != 1)
	{
		return Fail("The final decay did not notify changed before removing the effect");
	}

	const sas::GameplayEffectDefinition refreshDefinition = MakeRefreshEffect(
		"Effect.Test.Refresh"
	);
	const sas::GameplayEffectHandle refreshHandle = runtime.ApplyEffect(refreshDefinition);
	if (!refreshHandle.IsValid())
	{
		return Fail("RefreshDuration effect could not be applied");
	}
	runtime.Tick(1.5f);
	runtime.ApplyEffect(refreshDefinition);
	runtime.Tick(1.5f);
	if (!runtime.FindEffect(refreshHandle))
	{
		return Fail("RefreshDuration did not preserve its normal refresh behavior");
	}
	runtime.Tick(0.6f);
	if (runtime.FindEffect(refreshHandle))
	{
		return Fail("RefreshDuration effect did not expire after the refreshed duration");
	}

	ClearProbe(probe);
	const sas::GameplayEffectDefinition normalStackDefinition = []
	{
		sas::GameplayEffectDefinition definition;
		definition.effectId = "Effect.Test.NormalStack";
		definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
		definition.stackingPolicy = sas::GameplayEffectStackingPolicy::Stack;
		definition.duration = 2.f;
		definition.maxStacks = 2;
		return definition;
	}();
	const sas::GameplayEffectHandle normalStackHandle =
		runtime.ApplyEffect(normalStackDefinition);
	runtime.ApplyEffect(normalStackDefinition);
	runtime.Tick(2.1f);
	if (runtime.FindEffect(normalStackHandle))
	{
		return Fail("A non-decaying Stack effect inherited the decay policy");
	}
	if (probe.tickStacks.size() != 1 || probe.tickStacks.front() != 2)
	{
		return Fail("A non-decaying Stack effect was incorrectly split into decay slices");
	}

	ClearProbe(probe);
	const sas::GameplayEffectDefinition orderedDefinition = MakeDecayEffect(
		"Effect.Test.Ordered",
		2.f,
		2
	);
	const sas::GameplayEffectHandle orderedHandle = runtime.ApplyEffect(orderedDefinition);
	runtime.ApplyEffect(orderedDefinition);
	ClearProbe(probe);
	runtime.Tick(2.f);
	if (!runtime.FindEffect(orderedHandle) || probe.events.size() < 3 ||
		probe.events[0] != "tick" || probe.events[1] != "stackChanged" ||
		probe.events[2] != "changed" || probe.tickStacks.front() != 2 ||
		runtime.FindEffect(orderedHandle)->stackCount != 1)
	{
		return Fail("Behavior tick and stack decay did not use the deterministic order");
	}

	sas::AttributeSystem callbackAttributes;
	ly::GameplayTagContainer callbackTags;
	sas::GameplayEffectSystem callbackRuntime{ callbackAttributes, callbackTags };
	const sas::GameplayEffectDefinition callbackDefinition = MakeDecayEffect(
		"Effect.Test.CallbackMutation",
		2.f,
		2
	);
	const sas::GameplayEffectHandle removedDuringTickHandle =
		callbackRuntime.ApplyEffect(callbackDefinition);
	bool removedDuringTick = false;
	sas::GameplayEffectRuntimeCallbacks<sas::ActiveGameplayEffect> removeCallbacks;
	removeCallbacks.tick = [&callbackRuntime, &removedDuringTick](
		sas::ActiveGameplayEffect& effect,
		float
	)
	{
		if (!removedDuringTick)
		{
			removedDuringTick = true;
			callbackRuntime.RemoveEffect(effect.handle);
		}
		return sas::GameplayEffectBehaviorResult{};
	};
	callbackRuntime.SetCallbacks(std::move(removeCallbacks));
	callbackRuntime.Tick(3.f);
	if (callbackRuntime.FindEffect(removedDuringTickHandle))
	{
		return Fail("A decay callback removal was followed by a stale duration tick");
	}

	sas::AttributeSystem reapplyAttributes;
	ly::GameplayTagContainer reapplyTags;
	sas::GameplayEffectSystem reapplyRuntime{ reapplyAttributes, reapplyTags };
	const sas::GameplayEffectHandle replacedDuringTickHandle =
		reapplyRuntime.ApplyEffect(callbackDefinition);
	sas::GameplayEffectHandle reappliedHandle{};
	bool reappliedDuringTick = false;
	sas::GameplayEffectRuntimeCallbacks<sas::ActiveGameplayEffect> reapplyCallbacks;
	reapplyCallbacks.tick = [
		&reapplyRuntime,
		&callbackDefinition,
		&reappliedHandle,
		&reappliedDuringTick
	](sas::ActiveGameplayEffect& effect, float)
	{
		if (!reappliedDuringTick)
		{
			reappliedDuringTick = true;
			reapplyRuntime.RemoveEffect(effect.handle);
			reappliedHandle = reapplyRuntime.ApplyEffect(callbackDefinition);
		}
		return sas::GameplayEffectBehaviorResult{};
	};
	reapplyRuntime.SetCallbacks(std::move(reapplyCallbacks));
	reapplyRuntime.Tick(3.f);
	const sas::ActiveGameplayEffect* reappliedEffect = reapplyRuntime.FindEffect(reappliedHandle);
	if (reapplyRuntime.FindEffect(replacedDuringTickHandle) || !reappliedEffect ||
		reappliedEffect->stackCount != 1 || reappliedEffect->remainingDuration != 2.f)
	{
		return Fail("A decay callback reapply used stale duration state");
	}

	sas::AttributeSystem cappedAttributes;
	ly::GameplayTagContainer cappedTags;
	sas::GameplayEffectSystem cappedRuntime{ cappedAttributes, cappedTags };
	sas::GameplayEffectDefinition cappedDefinition;
	cappedDefinition.effectId = "Effect.Test.CappedReapply";
	cappedDefinition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
	cappedDefinition.stackingPolicy = sas::GameplayEffectStackingPolicy::Stack;
	cappedDefinition.duration = 2.f;
	cappedDefinition.maxStacks = 2;
	const sas::AttributeId cappedAttributeId{ "Attribute.Test.Capped" };
	cappedDefinition.attributes = {
		sas::GameplayAttribute{ cappedAttributeId, 1.f, 0.f }
	};
	sas::GameplayEffectSpec firstSpec = sas::MakeGameplayEffectSpec(cappedDefinition);
	sas::GameplayEffectSpec secondSpec = firstSpec;
	secondSpec.attributes.front().baseValue = 2.f;
	secondSpec.attributes.front().currentValue = 2.f;
	sas::GameplayEffectSpec thirdSpec = firstSpec;
	thirdSpec.attributes.front().baseValue = 3.f;
	thirdSpec.attributes.front().currentValue = 3.f;
	const sas::GameplayEffectHandle cappedHandle = cappedRuntime.ApplyEffect(firstSpec);
	cappedRuntime.ApplyEffect(secondSpec);
	cappedRuntime.ApplyEffect(thirdSpec);
	const sas::ActiveGameplayEffect* cappedEffect = cappedRuntime.FindEffect(cappedHandle);
	if (!cappedEffect || cappedEffect->stackCount != 2 ||
		sas::FindAttributeValue(cappedEffect->spec.attributes, cappedAttributeId, 0.f) != 2.f ||
		sas::FindAttributeValue(cappedEffect->runtimeAttributes, cappedAttributeId, 0.f) != 2.f)
	{
		return Fail("Capped RefreshStackDuration changed generic attributes without opt-in");
	}
	sas::GameplayEffectRuntimeCallbacks<sas::ActiveGameplayEffect> cappedCallbacks;
	cappedCallbacks.cappedReapply =
		[](sas::ActiveGameplayEffect& effect, const sas::GameplayEffectSpec& incomingSpec)
		{
			effect.spec.attributes = incomingSpec.attributes;
			effect.ResetRuntimeAttributesFromSpec();
			return true;
		};
	cappedRuntime.SetCallbacks(std::move(cappedCallbacks));
	cappedRuntime.ApplyEffect(firstSpec);
	cappedEffect = cappedRuntime.FindEffect(cappedHandle);
	if (!cappedEffect ||
		sas::FindAttributeValue(cappedEffect->spec.attributes, cappedAttributeId, 0.f) != 1.f ||
		sas::FindAttributeValue(cappedEffect->runtimeAttributes, cappedAttributeId, 0.f) != 1.f)
	{
		return Fail("Opt-in capped reapply did not replace spec and runtime attributes");
	}

	return 0;
}
