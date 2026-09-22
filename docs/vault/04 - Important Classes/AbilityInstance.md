---
type: class
status: implemented-unverified
verified_worktree_state: dirty
source_files:
  - SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h
  - SpaceAbilitySystem/include/abilities/AbilityRuntimeEntry.h
  - SpaceAbilitySystem/include/abilities/AbilityRuntimeState.h
  - LightYearsGame/include/gameplay/ability/GameAbility.h
  - LightYearsGame/src/gameplay/ability/GameAbility.cpp
symbols:
  - sas::GameplayAbilityInstance
  - ly::GameAbility
related:
  - "[[Ability System]]"
  - "[[Ability Cooldown and Charge Flow]]"
  - "[[Ability Skill Progression System]]"
---

# AbilityInstance

## Güncel ayrım

Eski `ly::GameAbilityInstance` kaldırılmıştır. Generic instance sorumluluğu
`sas::GameplayAbilityInstance<Definition, Execution>` içindedir.

SAS tabanı input, activation, end, cooldown, duration, charge, level ve snapshot
akışını yürütür. `AbilityRuntimeEntry` handle/base definition/resolved
definition/runtime state/execution storage'ını sağlar.

`ly::GameAbility` bu tabanın Light Years content uzantısıdır. Yalnız concrete
behavior/action dispatch'i, weapon runtime ve attachment loadout işlemlerini
uygular. Framework lifecycle kararı içermez.

## State akışı

```mermaid
stateDiagram-v2
    [*] --> Ready
    Ready --> Active: SAS activation gates pass
    Active --> CoolingDown: SAS EndAbility / cooldown > 0
    Active --> Ready: SAS EndAbility / cooldown <= 0
    CoolingDown --> Ready: SAS cooldown tick completes
```

## Durum

Kod taşındı; build/test tüm SAS geçişi sonunda çalıştırılacaktır.
