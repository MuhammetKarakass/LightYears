---
type: important-class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsGame/include/gameplay/progression/ShipProgression.h
  - LightYearsGame/src/gameplay/progression/ShipProgression.cpp
  - LightYearsGame/include/gameConfigs/ship/ShipStructs.h
symbols:
  - ly::ShipProgression
  - ly::ShipProgression::Configure
  - ly::ShipProgression::AddXP
  - ly::ShipProgression::BindAttributes
related:
  - "[[Ship Progression System]]"
  - "[[Ship Progression and Respawn Flow]]"
  - "[[CombatRuntime]]"
---

# ShipProgression

## Rol

`ly::ShipProgression`, player run'ına ait XP/level state'ini active ship actor'ından bağımsız tutar. Attribute modifier'larını temporary olarak bound `CombatRuntime::AttributeSystem`e uygular; ship respawn olduğunda aynı state'i yeni runtime'a rebind eder.

## Public yüzey

Dosya: `LightYearsGame/include/gameplay/progression/ShipProgression.h`

```cpp
void Configure(const ShipProgressionDefinition& definition);
void BindAttributes(AttributeSystem& attributes);
void UnbindAttributes();
void AddXP(float amount);
void ResetForNewRun();
float GetXPRequiredForNextLevel() const;
float GetGrowthMultiplier(const GameplayTag& attributeId) const;
```

## State ve sahiplik

| Alan | Anlam |
|---|---|
| `mDefinition` | XP curve ve growth override config value copy |
| `mBoundAttributes` | Active ship owner attribute system'e non-owning pointer |
| `mLevelModifierHandles` | O binding için eklenmiş Add modifier handle'ları |
| `mCurrentXP`, `mCurrentLevel` | Player-run progression state'i |
| `mIsConfigured` | Definition kurulmuş mu |

Class `Player` içinde value member'dır; `SpaceShip` veya `CombatRuntime` tarafından sahiplenilmez.

## Kritik lifecycle davranışı

`Configure`, definition'ı saklar ve bağlı runtime varsa modifier'ları rebuild eder. `BindAttributes`, aynı attribute system'e tekrar bağlanırsa rebuild yapar; farklı runtime'da old pointer'ı dereference etmeden pointer/handle listesini değiştirip yeni tarafta total bonusu kurar.

`UnbindAttributes`, güvenlik için old modifier'ları kaldırmaz; yalnız pointer ve handle listesini temizler. Kodun belirttiği kontrat, önceki runtime'ın destruction aşamasında olabilmesidir. Bu nedenle normal rebind'de old runtime'dan modifier removal beklenmemelidir.

## XP ve modifier davranışı

`AddXP`, birden fazla level geçişini destekler. Level gerçekten değiştiğinde modifier'lar kaldırılıp toplam bonus olarak yeniden eklenir ve delegate yayınlanır. `ResetForNewRun` XP/leveli başlangıca döndürür, binding'i kaldırır fakat configure state'ini açıkça false yapmaz.

Growth override listesinde olmayan base attribute'lar default `0.25` multiplier kullanır. Assertion, derived attribute için override verilmesini debug build'de yakalamayı amaçlar; release davranışı ayrı doğrulanmadı.

## Sınırlar

- Class ability level, scrap economy veya weapon progression profile state'ini yönetmez.
- Direct UI/save persistence sahibi değildir.
- `UnbindAttributes` sonrası old runtime'da kalan modifier cleanup'ı owner runtime destruction/clear lifecycle'ına dayanır; bu bağımsız olarak test edilmedi.

## İlgili notlar

- [[Ship Progression System]]
- [[Ship Progression and Respawn Flow]]
- [[CombatRuntime]]

## Kaynak doğrulaması

- Son doğrulanan commit: `b2e24c11d157c64b89bd1cf47c1390bf72784056`.
- Header/implementation ve Player çağrı noktaları doğrudan okundu; test/build bu aşamada çalıştırılmadı.
