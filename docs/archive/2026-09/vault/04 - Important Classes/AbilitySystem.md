---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - LightYearsAbilitySystemComponent grant, execution and attachment integrations
source_files:
  - SpaceAbilitySystem/include/AbilitySystemComponent.h
  - SpaceAbilitySystem/include/abilities/AbilityRuntimeSystem.h
  - LightYearsGame/include/gameplay/ability/LightYearsAbilitySystemComponent.h
  - LightYearsGame/src/gameplay/ability/LightYearsAbilitySystemComponent.cpp
  - LightYearsGame/include/gameplay/combat/CombatRuntime.h
symbols:
  - sas::AbilitySystemComponent::GrantAbility
  - sas::AbilitySystemComponent::SetAbilitySlotInput
  - sas::AbilitySystemComponent::Tick
  - ly::LightYearsAbilitySystemComponent::GetAbility
related:
  - "[[Ability System]]"
  - "[[Ability Grant and Activation Flow]]"
  - "[[CombatRuntime]]"
---

# LightYearsAbilitySystemComponent

> [!note] SAS runtime ownership update
> Grant/remove, handle and slot/passive registration, instance ticking,
> clearing and snapshot construction are owned by
> `sas::AbilityRuntimeSystem<GameAbilityDefinition, GameAbility>`.
> `ly::LightYearsAbilitySystemComponent` is the Actor, trigger, attachment
> and delegate adapter over `sas::AbilitySystemComponent`.

## Sorumluluk

`ly::LightYearsAbilitySystemComponent`, `CombatRuntime` içinde value olarak yaşayan oyun component'ıdır. SAS component üzerinden definition grant, slot/ID/handle lookup, input yönlendirme, instance tick'i, ability event delegate'leri ve attachment endpoint'lerini bağlar.

## Sahibi ve runtime state

- Sahibi: `CombatRuntime` değer üyesi olarak `mAbilitySystemComponent`.
- Owner bağımlılıkları: `LightYearsAbilitySystemComponent` constructor'ı `Actor&` alır; attribute, tag, effect ve typed ability runtime state'i SAS component içinde value/owned üyeler olarak tutulur.
- Owned runtime kayıtları: `sas::AbilityRuntimeSystem<GameAbilityDefinition, GameAbility>` içinde collection/instance sahipliği.
- İndeksler ve sayaç: collection içinde ID, slot, passive handle listesi ve handle allocator birlikte tutulur.

## Definition, tag ve effect ilişkisi

- `GrantAbility`, `AbilityDefinition` kabul eder ve `behaviorType` registry selector'ı için behavior oluşturur.
- Ability'nin runtime lookup anahtarı tag değildir; slot, handle ve `abilityId` kullanılır.
- `requiredOwnerTags` / `blockedOwnerTags`, instance activation gate'inde `GetOwnedTags` üzerinden sorgulanır.
- Effect sistemi `sas::AbilitySystemComponent` facade'ı üzerinden erişilir; effect runtime ayrıntısı bu sınıfın sorumluluğu değildir.

## Ana fonksiyonlar

| Fonksiyon | Görev |
|---|---|
| `GrantAbility` | Definition'dan runtime instance üretir ve indekslere yerleştirir |
| `SetAbilitySlotInput` | Input held state'ini slotun instance'ına iletir |
| `Tick` | Her ability instance'ını tick eder |
| `FindAbility` / `FindAbilityById` ve game `GetAbility` | Slot, handle veya ID ile lookup yapar |
| `HasAllOwnerTags` / `HasAnyOwnerTags` | Activation koşullarına ortak tag sorgusu verir |

## Kod okuma sırası

1. `SpaceAbilitySystem/include/AbilitySystemComponent.h`: component public sınırı.
2. `SpaceAbilitySystem/include/abilities/AbilityRuntimeSystem.h::GrantAbility`: definition → runtime kayıt dönüşümü.
3. `LightYearsGame/src/gameplay/ability/LightYearsAbilitySystemComponent.cpp`: game callback/attachment uyarlaması.
4. `SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h::TryActivate`: instance gate ve execution giriş noktası.

## Sonraki inceleme

Trigger event yönlendirmesi, cooldown azaltma, level/attachment işlemleri ve `GameAbilityActionExecutor` action implementasyonları burada ayrıntılandırılmadı.
