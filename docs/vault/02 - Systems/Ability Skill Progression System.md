---
type: system
status: partially-implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
uncommitted_areas:
  - ability, weapon and player integrations
source_files:
  - LightYearsGame/include/gameplay/ability/content/GameAbilityDefinition.h
  - SpaceAbilitySystem/include/abilities/AbilityRuntimeSystem.h
  - SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h
  - LightYearsGame/src/player/Player.cpp
  - LightYearsGame/include/gameConfigs/ability/AbilityCatalog.h
symbols:
  - ly::AbilityLevelStep
  - ly::GameAbilityDefinition::levelProgression
  - sas::GameplayAbilityInstance::SetLevel
  - ly::Player::TryPurchaseAbilityLevel
  - sas::AbilitySystemComponent::SetAbilityLevel
related:
  - "[[Ability Level Purchase and Respawn Restore Flow]]"
  - "[[AbilityInstance]]"
  - "[[Weapon System]]"
  - "[[Ship Progression System]]"
---

# Ability Skill Progression System

## 20 Eylül 2026 kaynak kontrolü

Player::TryPurchaseAbilityLevel ve RestorePurchasedAbilityLevels API'leri var; ancak AwardScrap ve TryPurchaseAbilityLevel için üretim kaynaklarında çağrı bulunmadı. Bu yüzden oyuncuya bağlı scrap ekonomisi kısmen uygulanmış sayılır. Ayrı evolve seçim/persistence akışı doğrulanmadı. [[Game Flow and UI]].

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Durum özeti

Ability level progression API düzeyinde **Implemented**, oyuncu ekonomisi entegrasyonu **Partially Implemented** durumundadır. Definition level step'leri, scrap economy ile player purchase, runtime definition rebuild, level change notification ve respawn restore kodda vardır.

“Evolve” veya “Evolution” adlı ayrı bir runtime sistem, definition tipi, registry, API ya da config sembolü aramada bulunmadı. Bu nedenle ayrı ability evolve sistemi bu doğrulama için **Unclear / Doğrulanamadı** olarak işaretlenir; level step içindeki upgrade tag'leri evolve uygulaması olarak gösterilmez.

## Model

| Veri/katman | Doğrulanmış sorumluluk |
|---|---|
| `AbilityLevelStep` | Level başına attribute modifier, unlocked upgrade ID, ek action ve ek trigger tanımı |
| `AbilityDefinition::levelProgression` | Level 2'den başlayan sıralı progression step listesi |
| `levelUpgradeScrapCosts` | Target level − 2 indeksli satın alma maliyeti; boşsa run economy ile satın alınamaz |
| `GameAbility` / `sas::GameplayAbilityInstance` | Base definition, current resolved definition ve level runtime state'i |
| `sas::AbilitySystemComponent` | Set/level-up yönlendirmesi ve grant-time progression validation |
| `Player` | Scrap balance, purchase gate ve respawn sonrası purchased level restore state'i |
| `PrimaryWeaponDefinition` conversion | Weapon progression step'lerini primary-fire ability level step'lerine dönüştürür |

## Definition kuralları

`GameAbilityDefinition::GetMaxLevel()` `1 + levelProgression.size()` döndürür. Runtime `GameAbility::GetMaximumLevel()` buna scoped ability level bonus'u ekleyebilir; Player purchase gate'i runtime ability'nin `GetMaxLevel()` sonucunu kullanır. Scrap cost listesi boş değilse step sayısıyla aynı uzunlukta ve tüm maliyetleri pozitif olmalıdır; aksi grant-time `ValidateLevelProgression` failure verir. Base ve tüm level step unlocked upgrade ID'leri geçerli ve birbirinden unique olmak zorundadır. Step içindeki action/trigger'lar da normal action validation'dan geçer.

```cpp
struct AbilityLevelStep
{
    List<AttributeModifier> attributeModifiers;
    List<GameplayTag> unlockedUpgradeIds;
    List<AbilityActionSpec> addedActions;
    List<AbilityTriggerSpec> addedTriggers;
};
```

## Level rebuild

`sas::GameplayAbilityInstance::SetLevel`, istenen değeri `[1, GetMaximumLevel()]` aralığına clamp eder. Ability aktifse önce `Interrupted` reason ile end edilir. Ardından resolved `mDefinition`, `mBaseDefinition`dan sıfırdan kurulur ve mevcut runtime level ile scoped level bonus'unun izin verdiği step'ler sırasıyla uygulanır:

- Modifier'lar definition modifier listesine eklenir.
- Upgrade ID'leri duplicate olmadan eklenir.
- Ek action ve trigger'lar append edilir.

Bu yeniden kurma, level düşürülse dahi eski level eklerinin kalmamasını sağlar. Başarılı seviye değişiminden sonra instance primary weapon runtime configuration'ını yenilemeye çalışır ve AbilitySystem level-change bildirimi yayınlar.

## Scrap purchase ve respawn

`Player::TryPurchaseAbilityLevel`, active ve pending-destroy olmayan ship, slotta ability, max level, configured scrap cost ve yeterli scrap kontrollerini yapar. `sas::AbilitySystemComponent::LevelUpAbility` başarılı olduktan sonra scrap düşer ve ability ID → level değeri `mPurchasedAbilityLevels` map'ine yazılır.

`SetAbilityLevel` / `sas::GameplayAbilityInstance::SetLevel` cost kontrolü yapmaz; Player satın alma akışının dışında restore veya başka runtime caller'lar tarafından kullanılabilir. Respawn'da `RestorePurchasedAbilityLevels`, yeni ship üzerindeki aynı ability ID'yi bulur ve stored level'i bu API ile uygular. Run reset, purchased level map'ini temizler.

## Weapon progression bağlantısı

Primary fire ability, `MakePrimaryFireAbilityDefinition` içinde `WeaponProgressionProfile` step'lerini `AbilityLevelStep`e çevirir. Weapon attribute modifier'ları ile unlocked upgrade/feature tag'leri primary ability progression'ına taşınır. Level değişimi önceden initialize edilmiş primary weapon runtime varsa `EnsureRuntimeConfigured` çağrısını unlocked upgrade ID'lerle yeniler.

Bu bağlantı weapon feature unlock'larının ability level üzerinden tüketilebildiğini doğrular; weapon progression'ın bütün balance/feature davranışlarını kapsamaz. Ayrıntı: [[Weapon System]].

## Evolve sınırı

- Level step'ler value modifier, action, trigger ve upgrade tag ekleyebilir.
- Ayrı evolve selection, branch, mutation, evolve ID veya evolve-specific persistence API'si bulunamadı.
- Presentation profile kurallarındaki “evolve variant” ifadesi mimari kontrattır; bu doğrulamada concrete gameplay evolve sistemi kanıtı değildir.

## Kaynak doğrulaması

- İncelenen durum: dirty worktree.
- Code graph ile `SetLevel`, `RebuildDefinitionForLevel`, purchase/restore çağrıları bulundu; ilgili config ve implementation bölümleri doğrudan okundu.
- Test/build bu aşamada çalıştırılmadı.
