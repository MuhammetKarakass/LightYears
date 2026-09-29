---
type: code-walkthrough
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
source_files:
  - LightYearsGame/src/player/Player.cpp
  - SpaceAbilitySystem/include/abilities/AbilityRuntimeSystem.h
  - SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h
  - LightYearsGame/include/gameConfigs/ability/AbilityCatalog.h
symbols:
  - ly::Player::TryPurchaseAbilityLevel
  - sas::AbilitySystemComponent::LevelUpAbility
  - sas::GameplayAbilityInstance::SetLevel
  - ly::GameAbility::RebuildDefinitionForLevel
  - ly::Player::RestorePurchasedAbilityLevels
related:
  - "[[Ability Skill Progression System]]"
  - "[[AbilityInstance]]"
  - "[[Ship Progression and Respawn Flow]]"
  - "[[Primary Weapon Fire Lifecycle]]"
---

# Ability Level Purchase and Respawn Restore Flow

## 20 Eylül 2026 kaynak kontrolü

Bu walkthrough mevcut API'nin nasıl çağrılacağını anlatır; üretim kodunda AwardScrap/TryPurchaseAbilityLevel çağrısı bulunmadığı için kullanıcıdan başlayan satın alma akışı değildir. Respawn restore bellektedir. [[Save and Load]].

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Amaç

Bu walkthrough, player'ın scrap ile ability level satın almasını, runtime definition'ın yeniden kurulmasını ve ship respawn'ında purchased level'ların geri yüklenmesini izler. Ayrı evolve seçimi kodda doğrulanmadığı için bu flow'a dahil değildir.

```mermaid
sequenceDiagram
    participant Player
    participant AbilitySystem as AbilitySystemComponent
    participant Instance as GameAbility
    participant Weapon as PrimaryWeaponExecutionSystem
    participant Ship as New PlayerSpaceShip

    Player->>Player: validate ship, slot, max level, cost, scrap
    Player->>AbilitySystem: LevelUpAbility(slot)
    AbilitySystem->>Instance: SetLevel(current + 1)
    opt ability active
        Instance->>Instance: EndAbility(Interrupted)
    end
    Instance->>Instance: RebuildDefinitionForLevel
    Instance->>Weapon: refresh configured runtime (if initialized)
    Instance-->>Player: level changed
    Player->>Player: subtract scrap; save abilityId -> level
    Note over Player,Ship: Respawn
    Player->>Ship: spawn and grant base abilities
    Player->>AbilitySystem: SetAbilityLevel(handle, stored level)
```

## 1. Purchase gate

`Player::TryPurchaseAbilityLevel` yalnız active, non-pending player ship üzerinde çalışır. Slotta ability bulunmalı, target level max level'i aşmamalı, definition o target level için configured positive scrap cost taşımalı ve Player'da yeterli scrap bulunmalıdır. Bu kontrollerden biri başarısızsa level veya scrap değişmez.

## 2. Runtime level değişimi

`sas::AbilitySystemComponent::LevelUpAbility`, slot instance'ına `SetLevel(current + 1)` yönlendirir. `GameAbility` active ise execution `Interrupted` reason ile end edilir. Sonra base definition resetlenir, mevcut runtime level + scoped bonus kadar level step'i sırasıyla eklenir ve primary weapon runtime (varsa) yeni definition/unlocked IDs ile refresh edilmeye çalışılır.

Level değişimi gerçekleşmezse Player scrap düşmez. Başarılı olursa Player önce ability ID'yi alır, scrap'i düşer, `mPurchasedAbilityLevels[abilityId]` değerini günceller ve scrap delegate'ini yayınlar.

## 3. Respawn restore

Yeni `PlayerSpaceShip` constructor'ı base ability'leri grant eder. Player, ship spawn'dan sonra `RestorePurchasedAbilityLevels` çağırır; stored map'teki ability ID yeni instance'ta bulunursa `SetAbilityLevel(handle, storedLevel)` kullanılır. API level'i max level'e clamp ettiği için stored value geçerli aralığa iner; map değeri resulting level ile güncellenir.

Bu restore yolu scrap harcamaz ve purchased map'te olmayan ability'leri atlar.

## 4. Primary weapon conversion

Primary weapon, normal bir PrimaryFire ability olarak grant edilir. Weapon profile level step'leri ability definition'a çevrilir; bu yüzden aynı Player purchase flow'u weapon level'ını da yükseltebilir. Runtime weapon state initialize edilmişse refresh, unlocked feature tag'lerini resolver'a iletir.

## Karar noktaları

| Nokta | Sonuç |
|---|---|
| Empty cost list veya cost/step length uyuşmazlığı | Definition grant validation başarısız olur veya purchase yapılamaz |
| Target max level üzerinde | Purchase reddedilir |
| Ability active iken level change | Önce interrupted end/cleanup |
| Level step duplicate/invalid upgrade tag | Grant validation reddi |
| Respawn'da ability ID yok | Stored level atlanır |
| Evolve seçim talebi | Doğrulanmış ayrı API yok |

## Kaynak doğrulaması

- Son doğrulanan commit: `f83fe57b44777de4c31799b4e8bafac04a18700d`.
- Dirty worktree incelendi; test/build bu aşamada çalıştırılmadı.
