---
type: important-class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsGame/include/gameplay/weapon/PrimaryWeaponExecutionSystem.h
  - LightYearsGame/src/gameplay/weapon/PrimaryWeaponExecutionSystem.cpp
  - LightYearsGame/include/gameplay/weapon/PrimaryWeaponHandler.h
symbols:
  - ly::PrimaryWeaponExecutionSystem
  - ly::PrimaryWeaponRuntimeState
  - ly::PrimaryWeaponExecutionContext
related:
  - "[[Weapon System]]"
  - "[[Primary Weapon Fire Lifecycle]]"
  - "[[Ability Execution System]]"
---

# PrimaryWeaponExecutionSystem

## Rol

`ly::PrimaryWeaponExecutionSystem`, stateless primary-weapon dispatcher'ıdır. Kendi active weapon container'ı yoktur; caller'ın verdiği `PrimaryWeaponRuntimeState` üzerinde validation, configuration ve handler/feature lifecycle'ını yürütür.

## Public yüzey

Dosya: `LightYearsGame/include/gameplay/weapon/PrimaryWeaponExecutionSystem.h`

```cpp
static PrimaryWeaponValidationResult InitializeRuntime(
    const PrimaryWeaponDefinition& definition,
    PrimaryWeaponRuntimeState& state,
    const List<GameplayTag>* unlockedUpgradeIds = nullptr);
static PrimaryWeaponValidationResult EnsureRuntimeConfigured(
    const PrimaryWeaponDefinition& definition,
    PrimaryWeaponRuntimeState& state,
    const List<GameplayTag>* unlockedUpgradeIds = nullptr);
static void BeginFire(const PrimaryWeaponExecutionContext& context,
    PrimaryWeaponRuntimeState& state);
static bool FireOnce(const PrimaryWeaponExecutionContext& context,
    PrimaryWeaponRuntimeState& state);
static void TickFire(const PrimaryWeaponExecutionContext& context,
    PrimaryWeaponRuntimeState& state, float deltaTime);
static void EndFire(const PrimaryWeaponExecutionContext& context,
    PrimaryWeaponRuntimeState& state);
```

Ek public operasyonlar `ValidateDefinition`, `UsesIntervalFire`, `TickInactive`, `ConsumeRequestedCooldown` ve `BuildBaseFireInterval`dır.

## Context ve state

`PrimaryWeaponExecutionContext`; owner actor, definition, resolved attributes, damage tags, isteğe bağlı unlocked ability upgrade tag'leri ve isteğe bağlı runtime pointer taşır. System context'i `WithRuntimeState` ile handler/feature çağrılarına runtime pointer içerecek biçimde kopyalar.

`PrimaryWeaponRuntimeState` handler ve feature'lara sahip değildir; bunları registry-owned nesnelere işaret eden pointer'lar olarak tutar. Type-specific mutable durum için `unique_ptr<PrimaryWeaponTypeRuntimeState>` kullanır. `featureValues` yalnız aktif feature'ların runtime anahtarları için korunur.

## Yapılandırma kuralları

- `InitializeRuntime`, önce aday configuration kurar; başarısız validation mevcut geçerli state'i değiştirmez.
- Firing runtime yeniden kurulamaz.
- `EnsureRuntimeConfigured`, aynı weapon/handler/type state kombinasyonunu korur.
- Feature listesi değişmişse, firing dışındayken yalnız hâlâ etkin feature'ların sahip olduğu runtime değerleri retained olur.
- Unknown veya type state üretemeyen handler, geçerli runtime configuration oluşturmaz.

## Dispatch sırası

| Operasyon | Handler | Features | State sonucu |
|---|---|---|---|
| `BeginFire` | Önce `BeginFire` | Sonra tüm `BeginFire` | `isFiring = true` |
| `FireOnce` | Feature gate'leri sonrası `FireOnce` | Önce `CanFire`, sonra `AfterFire` | Başarı bool'u |
| `TickFire` | `TickFire` | Ardından `TickFire` | Runtime güncellenir |
| `TickInactive` | Çağrılmaz | `TickInactive` | Yalnız firing dışındayken |
| `EndFire` | Önce `EndFire` | Sonra tüm `EndFire` | `isFiring = false` |

## Requested cooldown

Feature handler'ı `PrimaryWeaponRuntimeState::RequestCooldown` ile cooldown isteyebilir. System bu değeri `ConsumeRequestedCooldown` ile yalnız iletir; ability action'ın fire interval'ine uygulama kararı `FireWeaponActionRuntime` içindeki `GameAbilityActionExecutor` akışındadır. Bu ayrım weapon feature'ın cooldown kuralını bildirmesini, ability execution katmanının ise zamanlamayı sahiplenmesini sağlar.

## Kullanım sınırı

- Bu sınıf projectile actor spawn/impact, arc target seçimi, beam hit testi veya wave geometry'nin sahibi değildir; bunlar concrete handler'ların domain davranışıdır.
- Attribute attachment çözümü de burada yapılmaz; executor tarafından hazırlanmış `context.attributes` kullanılır.
- Runtime'ın ömrü caller'a aittir: ability yolunda persistent state `GameAbility` içindedir, doğrudan test/çağrıda stack veya başka owner olabilir.

## İlgili notlar

- Sistem görünümü: [[Weapon System]]
- Uçtan uca action akışı: [[Primary Weapon Fire Lifecycle]]
- Ability action katmanı: [[Ability Execution System]], [[Ability Action Execution Flow]]

## Kaynak doğrulaması

- Son doğrulanan commit: `b2e24c11d157c64b89bd1cf47c1390bf72784056`.
- Sınıfın public yüzeyi ve implementation lifecycle'ı doğrudan okundu; test/build bu aşamada çalıştırılmadı.
