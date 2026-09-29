---
type: system
status: implemented-unverified
verified_worktree_state: dirty
source_files:
  - SpaceAbilitySystem/include/abilities/AbilityBehavior.h
  - SpaceAbilitySystem/include/abilities/AbilityBehaviorRegistry.h
  - LightYearsGame/include/gameplay/ability/GameAbility.h
  - LightYearsGame/src/gameplay/ability/LightYearsAbilitySystemComponent.cpp
symbols:
  - sas::AbilityBehavior
  - sas::AbilityBehaviorRegistry
  - ly::GameAbilityBehavior
  - ly::RegisterGameAbilityBehaviors
related:
  - "[[Ability System]]"
  - "[[Ability Behavior Registration and Dispatch Flow]]"
---

# Ability Behavior System

## Framework

`sas::AbilityBehavior<Definition, Context>` Validate/Activate/Tick/End
kontratını sağlar. `sas::AbilityBehaviorRegistry<Behavior, Key, Hash>` typed
factory storage, duplicate reddi ve instance üretimini sahiplenir.

## Game content

`GameAbilityBehaviorBinding.h`, SAS kontratını `GameAbilityDefinition`,
`GameAbility`, owner `Actor` ve `LightYearsAbilitySystemComponent` context'iyle
eşler. Dash, Shield, Gravity Anomaly, Rocket, Sun Beam, Inferno Spray,
Overdrive Core, Null Pulse, Phase Drift, Shield Harvest, Hull Shock, Orbital
Drones, Execution Drive ve Relay Prism somut behavior'ları
oyun içeriğidir.

`RegisterGameAbilityBehaviors()` yalnız shipped factory'leri SAS registry'ye
kaydeder. Game tarafında ayrı registry/container implementasyonu yoktur.

## Durum

Registry sahipliği SAS'a taşındı. Build/test tüm taşıma sonunda yapılacaktır.
