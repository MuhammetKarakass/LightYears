---
type: code-walkthrough
status: implemented-unverified
verified_worktree_state: dirty
source_files:
  - LightYearsGame/src/gameFramework/GameApplication.cpp
  - LightYearsGame/src/gameplay/ability/LightYearsAbilitySystemComponent.cpp
  - SpaceAbilitySystem/include/abilities/AbilityBehaviorRegistry.h
  - SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h
  - LightYearsGame/src/gameplay/ability/GameAbility.cpp
symbols:
  - ly::RegisterGameAbilityContent
  - ly::RegisterGameAbilityBehaviors
  - sas::AbilityBehaviorRegistry
  - sas::GameplayAbilityInstance
related:
  - "[[Ability Behavior System]]"
  - "[[Ability System]]"
---

# Ability Behavior Registration and Dispatch Flow

```mermaid
sequenceDiagram
    participant App as GameApplication
    participant Content as LightYearsAbilitySystemComponent
    participant Registry as sas::AbilityBehaviorRegistry
    participant Runtime as sas::AbilityRuntimeSystem
    participant Instance as sas::GameplayAbilityInstance
    participant Binding as GameAbility
    participant Behavior as GameAbilityBehavior

    App->>Content: RegisterGameAbilityContent
    Content->>Registry: Register shipped factories
    Runtime->>Registry: Create for validation/grant
    Registry-->>Runtime: unique behavior
    Runtime->>Binding: construct concrete binding
    Instance->>Binding: ActivateContent
    Binding->>Behavior: Activate
    Instance->>Binding: TickExecution / EndContent
    Binding->>Behavior: Tick / End
```

Registry storage ve generic instance yaşam döngüsü SAS'a aittir. Content
bootstrap ile Actor/weapon/attachment/action çağrıları LightYearsGame'de kalır.
Build/test tüm taşıma sonunda çalıştırılacaktır.
