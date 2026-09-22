---
type: system
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
source_files:
  - SpaceAbilitySystem/include/AbilitySystemComponent.h
  - SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h
  - SpaceAbilitySystem/include/abilities/AbilityRuntimeSystem.h
  - SpaceAbilitySystem/include/abilities/AbilityBehaviorRegistry.h
  - SpaceAbilitySystem/include/abilities/AbilityEvent.h
  - LightYearsGame/include/gameplay/ability/content/GameAbilityDefinition.h
  - LightYearsGame/include/gameplay/ability/LightYearsAbilitySystemComponent.h
  - LightYearsGame/include/gameplay/ability/GameAbility.h
symbols:
  - sas::AbilitySystemComponent
  - sas::GameplayAbilityInstance
  - sas::AbilityRuntimeSystem
  - ly::GameAbilityDefinition
  - ly::LightYearsAbilitySystemComponent
  - ly::GameAbility
related:
  - "[[Gameplay Tag System]]"
  - "[[Attribute System]]"
  - "[[Gameplay Effect System]]"
  - "[[Ability Grant and Activation Flow]]"
  - "[[Ability Cooldown and Charge Flow]]"
  - "[[Ability Execution System]]"
  - "[[Ability Behavior System]]"
---

# Ability System

## 20 Eylül 2026 kaynak kontrolü

Bu 7 Eylül snapshot'ındaki loadout güncel binding kaynağı değildir. Güncel çalışma
ağacının slot sahibi `LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp`
dosyasıdır; bu belge complete slot tablosunu kopyalamaz. Katalog genişledi;
[[Ability Content Inventory]] kayıt envanterini verir. RegisterGameAbilityContent
presentation/behavior/actor/effect kayıtlarını validasyondan önce yapar. Bütün
aileler uçtan uca doğrulanmadı; [[Temporal Runtime]].

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Sahiplik sınırı

> Güncel içerik için [[Ability Content Inventory]]; runtime slot binding için
> `DefaultAbilityLoadout.cpp` önceliklidir. [[00 - Runtime Snapshot]] tarihsel
> bir snapshot'tır.

`sas::AbilitySystemComponent`, bir owner için attribute, owned tag, ability
runtime ve gameplay effect runtime erişiminin merkezi framework nesnesidir.
Ability grant/remove, slot input, lookup, level değişimi, snapshot, event
yönlendirme ve tick component üzerinden yürütülür.

`sas::GameplayAbilityInstance<Definition, Execution>` tek ability'nin generic
yaşam döngüsünü sahiplenir:

- input edge değerlendirme ve activation isteği;
- activation/end ve instant/duration kararları;
- cooldown, charge ve active-duration ilerletme;
- level değiştirme ve runtime snapshot üretme;
- activated/ended/changed/level-changed bildirimleri.

`sas::AbilityRuntimeBinding`, bu instance'ın anlık equipment slotunu taşır.
Definition slotu ile aynı olmak zorunda değildir; loadout bir ability'yi taşırken
content ID'si, behavior'ı ve balance verisi sabit kalır.

## Game tarafında kalanlar

`ly::GameAbilityDefinition`, Light Years'a ait action/trigger/level, weapon,
attachment, economy ve UI içeriğidir. `ly::GameAbility`,
`sas::GameplayAbilityInstance` tabanına yalnız şu concrete bağlamaları sağlar:

- owner tag gate;
- shipped `GameAbilityBehavior` çağrıları;
- FireWeapon/SpawnActor/ApplyEffect action binding;
- weapon ve attachment runtime verisi;
- Actor, World ve `DamageContext` erişimi.

`ly::LightYearsAbilitySystemComponent`, `sas::AbilitySystemComponent`'tan türeyen
game kurulumu adaptörüdür. Base component attribute/tag/effect/ability storage'ını
sahiplenir; game component ise typed `GameAbility` runtime callback'lerini,
behavior/action/content doğrulamasını ve gameplay event adaptasyonunu kurar.

## Event sınırı

`sas::AbilityEvent` tek event taşıyıcısıdır. Tag ve magnitude doğrudan SAS
tipindedir. Source, target ve context type-safe opaque referans olarak bağlanır;
böylece SAS, Actor veya `DamageContext` header'larına bağımlı olmaz.
`GameAbilityEvent` adlı ikinci bir oyun tipi yoktur.

## Durum

Sahiplik düzeltmesi uygulanmıştır. Game adaptörü concrete behavior, actor,
weapon, attachment ve `DamageContext` bağlarını taşır. Yeni shipped behavior'lar
`ShieldHarvest`, `HullShock`, `OrbitalDrones`, `ExecutionDrive`, `RelayPrism`,
`EchoProtocol`, `MineLayer`, `RailBurst`, `CrescentReaver`, `EnergySpear` ve
`ScorchDrive` registry/content akışına bağlıdır.
