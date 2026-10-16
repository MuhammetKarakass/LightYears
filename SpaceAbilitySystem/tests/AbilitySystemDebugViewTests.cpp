#include "AbilitySystemComponent.h"
#include <iostream>

int main()
{
	int failures = 0;
	const auto check = [&](bool value, const char* message)
	{
		if (!value) { std::cerr << message << '\n'; ++failures; }
	};
	sas::AbilitySystemComponent asc;
	asc.GetAttributes().RegisterAttribute(sas::AttributeId{ "Owner.Speed" }, 10.f);
	sas::GameplayEffectDefinition definition;
	definition.effectId = "Effect.Test.DebugBuff";
	definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
	definition.duration = 5.f;
	definition.modifiers.emplace_back(sas::AttributeId{ "Owner.Speed" }, 3.f);
	definition.grantedTags.emplace_back("State.Buff.Speed");
	sas::AbilitySystemDebugView::BeginPlay();
	const auto handle = asc.ApplyGameplayEffect(definition);
	auto frozen = asc.BuildDebugSnapshot();
	check(frozen.effects.size() == 1 && frozen.effects[0].handle == handle, "Active handle missing");
	check(frozen.effects[0].modifiers.size() == 1 && frozen.effects[0].grantedTags.size() == 1, "Effect detail missing");
	check(frozen.attributes.size() == 1 && frozen.attributes[0].baseValue == 10.f && frozen.attributes[0].currentValue == 13.f, "Attribute snapshot incorrect");
	check(frozen.tags.size() == 1 && frozen.tags[0].second == 1, "Tag counts missing");
	asc.AddOwnedTag(ly::GameplayTag{ "State.Buff.Speed" });
	check(asc.BuildDebugSnapshot().tags[0].second == 2, "Owned tag count incorrect");
	asc.Tick(1.f);
	check(frozen.effects[0].remainingDuration == 5.f, "Snapshot retains mutable state");
	asc.RemoveGameplayEffect(handle);
	check(asc.BuildDebugSnapshot().effects.empty(), "Removed effect still visible");
	const auto events = asc.BuildDebugSnapshot().events;
	check(events.size() == 2 && events.back().message.find(definition.effectId) != std::string::npos, "Effect lifecycle log missing ID");
	definition.effectId = "Effect.Test.Instant";
	definition.durationPolicy = sas::GameplayEffectDurationPolicy::Instant;
	asc.ApplyGameplayEffect(definition);
	check(asc.BuildDebugSnapshot().events.back().message.find(definition.effectId) != std::string::npos, "Instant effect ID missing");
	sas::AbilityEvent event;
	event.eventTag = ly::GameplayTag{ "Event.Test.Debug" };
	for (int i = 0; i < 120; ++i) asc.HandleGameplayEvent(event);
	const auto ring = asc.BuildDebugSnapshot().events;
	check(ring.size() == 100 && ring.front().seconds <= ring.back().seconds, "Event ring bound/timestamps incorrect");
	sas::AbilitySystemDebugView::BeginPlay();
	check(asc.BuildDebugSnapshot().events.empty(), "Previous Play log retained");
	asc.HandleGameplayEvent(event);
	check(asc.BuildDebugSnapshot().events.size() == 1, "New session log incorrect");
	definition.durationPolicy = sas::GameplayEffectDurationPolicy::Duration;
	definition.effectId = "Effect.Test.SessionBoundary";
	const auto previousSessionEffect = asc.ApplyGameplayEffect(definition);
	definition.effectId = "Effect.Test.SecondSessionBoundary";
	const auto secondSessionEffect = asc.ApplyGameplayEffect(definition);
	sas::AbilitySystemDebugView::BeginPlay();
	asc.RemoveGameplayEffect(previousSessionEffect);
	asc.RemoveGameplayEffect(secondSessionEffect);
	const auto newSessionEvents = asc.BuildDebugSnapshot().events;
	check(newSessionEvents.size() == 2 && newSessionEvents[0].message == "Effect removed Effect.Test.SessionBoundary" &&
		newSessionEvents[1].message == "Effect removed " + definition.effectId,
		"First removal in a new session lost its ID or retained old events");
	sas::AbilitySystemDebugView::recording = false;
	asc.HandleGameplayEvent(event);
	check(asc.BuildDebugSnapshot().events.size() == 2, "Disabled recording appended an event");
	sas::AbilityDebugEntry ability;
	check(std::string{ ability.State() } == "ready", "Ready state incorrect");
	ability.runtime.cooldownRemaining = 1.f;
	check(std::string{ ability.State() } == "cooldown", "Cooldown state incorrect");
	ability.blockedReason = "Required owner tag missing";
	check(std::string{ ability.State() } == "blocked", "Blocked state incorrect");
	ability.runtime.active = true;
	check(std::string{ ability.State() } == "active", "Active state incorrect");
	sas::AbilitySystemDebugView::recording = false;
	return failures ? 1 : 0;
}
