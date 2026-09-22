---
type: system
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
uncommitted_areas:
  - primary-weapon execution and ability integration
  - weapon tests and CMake target
source_files:
  - LightYearsGame/include/gameConfigs/combat/WeaponStructs.h
  - LightYearsGame/assets/content/data/weapons.json
  - LightYearsGame/include/gameplay/weapon/PrimaryWeaponHandler.h
  - LightYearsGame/include/gameplay/weapon/PrimaryWeaponHandlerRegistry.h
  - LightYearsGame/include/gameplay/weapon/PrimaryWeaponExecutionSystem.h
  - LightYearsGame/src/gameplay/weapon/PrimaryWeaponExecutionSystem.cpp
  - LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp
symbols:
  - ly::PrimaryWeaponDefinition
  - ly::PrimaryWeaponHandler
  - ly::PrimaryWeaponFeatureHandler
  - ly::PrimaryWeaponHandlerRegistry
  - ly::PrimaryWeaponExecutionSystem
  - ly::PrimaryWeaponRuntimeState
related:
  - "[[Ability Execution System]]"
  - "[[Primary Weapon Fire Lifecycle]]"
  - "[[PrimaryWeaponExecutionSystem]]"
  - "[[Projectile System]]"
  - "[[Combat and Damage System]]"
---

# Weapon System

## 20 Eylül 2026 kaynak kontrolü

IroncladProtocolAbility::Activate aktif primary lifecycle'ını iptal edip geçici minigun override kurar; ClearRuntimeState override silinmeden aktif ateşi bitirir, effect/modifier/guard temizler. Kaynak: LightYearsGame/src/gameplay/ability/ironcladProtocol/IroncladProtocolAbility.cpp. Bu yol kaynakta mevcut; bütün handler ve override kombinasyonları test edilmedi.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Kapsam

Bu not, birincil silahların definition, handler/feature registry, runtime state ve ability action üzerinden yürütülmesini açıklar. Projectile aktörlerinin uçuş/çarpışma davranışı, damage çözümü ve weapon progression ayrıntıları sonraki aşamalara bırakılmıştır.

## Model ve sorumluluklar

| Katman | Doğrulanmış sorumluluk |
|---|---|
| `PrimaryWeaponDefinition` | Kimlik, somut type tag, presentation tanımı, attributes, muzzles, feature tags, damage tags ve progression verisini taşır |
| `PrimaryWeaponHandlerRegistry` | Type ve feature tag'lerinden built-in veya genişletilmiş handler nesnesini bulur |
| `PrimaryWeaponHandler` | Bir weapon type'ın validation, runtime-state oluşturma ve fire lifecycle kontratı |
| `PrimaryWeaponFeatureHandler` | Type'a eklenen fire gate, runtime değerleri ve begin/tick/inactive/end hook'ları |
| `PrimaryWeaponExecutionSystem` | Definition validation, runtime configuration ve handler/feature lifecycle dispatcher'ı |
| `GameAbilityActionExecutor` | `FireWeaponAction` için resolved attributes/context kurar, execution interval'ini uygular ve weapon lifecycle'ını çağırır |
| `GameAbility` / `sas::GameplayAbilityInstance` | Persistent primary-weapon runtime, resolved attributes/damage tags ve weapon fire interval'ini tutar |

## Definition ve config

Dosya: `LightYearsGame/include/gameConfigs/combat/WeaponStructs.h`  
Sembol: `PrimaryWeaponDefinition`

```cpp
struct PrimaryWeaponDefinition
{
    std::string weaponId;
    ly::GameplayTag weaponTypeTag;
    WeaponPresentationDefinition presentationDefinition;
    sas::GameplayAttributeList attributes;
    ly::List<WeaponMuzzleDefinition> muzzleDefinitions;
    bool automaticFire;
    WeaponProgressionProfile progressionProfile;
    ly::List<ly::GameplayTag> featureTags;
    ly::List<ly::GameplayTag> damageTags;
    PrimaryWeaponCadenceMode cadenceMode{ PrimaryWeaponCadenceMode::FixedInterval };
    std::optional<PrimaryWeaponMagazineDefinition> magazine;
};
```

Shipped weapon tanımları `weapons.json` içindedir; `WeaponStructs.h` schema/contract tiplerini taşır. Standard projectile, shotgun, electric arc, continuous heat beam ve expanding cryo wave aileleri vardır. Bu örneklerin aynı zamanda eksiksiz balance doğrulaması olduğu bu notta iddia edilmez.

## Handler ve feature registry

Registry ilk kullanımda built-in handler'ları tembel olarak ekler. `RegisterHandler` ve `RegisterFeature`, null veya geçersiz tag'i reddeder; `emplace` ile duplicate kaydı değiştirmez. `FindHandler` / `FindFeature` tam `GameplayTag` eşleşmesi arar ve bulunamazsa `nullptr` döndürür.

| Tag hedefi | Built-in implementation | Durum |
|---|---|---|
| Standard projectile | `StandardProjectileWeaponHandler` | Implemented |
| Shotgun projectile | `ShotgunWeaponHandler` | Implemented |
| Electric arc | `ElectricArcWeaponHandler` | Implemented |
| Continuous beam | `ContinuousBeamWeaponHandler` | Implemented |
| Expanding wave | `ExpandingWaveWeaponHandler` | Implemented |
| Heat feature | `HeatWeaponFeatureHandler` | Implemented |

Handler registry sahip olduğu handler nesnelerini `unique_ptr` ile static map'te tutar. Weapon runtime ise handler'a yalnız non-owning pointer taşır.

## Validation ve runtime configuration

`PrimaryWeaponExecutionSystem::ValidateDefinition`, `PrimaryWeaponDefinitionValidator`'a yönlenir. Validator type tag'in somut bir handler'a karşılık gelmesini; type/feature'a ait attribute köklerinin doğru olmasını ve family'ye özgü gerekli değerleri denetler. Örneğin testlerde family tag, karışık projectile/beam attribute'ları, eksik shotgun pellet count ve geçersiz arc multiplier reddedilmektedir.

`InitializeRuntime` önce validation ve registry çözümlemesi yapar, sonra handler'ın `CreateRuntimeState()` sonucunu `PrimaryWeaponRuntimeState` içine kurar. Firing durumundaki runtime yeniden yapılandırılamaz. `EnsureRuntimeConfigured`, aynı weapon/handler için kullanılabilir state'i korur; feature set'i değişirse yalnız firing dışındayken feature runtime değerlerinden sahipliği devam edenleri taşır.

## Runtime state ve feature yaşamı

`PrimaryWeaponRuntimeState`, handler pointer'ı, feature pointer listesi, type-specific `unique_ptr` state, `featureValues`, configured weapon kimliği, initialization/firing bayrakları ve requested cooldown taşır.

`PrimaryWeaponExecutionSystem` lifecycle sırası şöyledir:

1. Runtime initialize/ensure edilir.
2. `BeginFire`, handler'a sonra etkin feature'lara çağrı yapar ve `isFiring` durumunu açar.
3. `FireOnce`, önce her feature'ın `CanFire` gate'ini kontrol eder; handler fire yapar, sonra feature `AfterFire` hook'ları çalışır.
4. `TickFire`, handler ve feature tick'lerini çağırır.
5. `TickInactive`, yalnız firing dışındayken feature tick'lerini çağırır.
6. `EndFire`, handler ve feature end hook'larını çağırır ve firing durumunu kapatır.

Feature'ların hepsi generic olarak zorunlu değildir. Mevcut built-in heat feature'ı, fire/inactive tick'lerinde runtime heat değerini yönetir ve gerektiğinde cooldown isteyebilir.

## Magazine ve Cadence Altyapısı (Phase 3B.1)

Ortak primary weapon altyapısında magazine kapasitesi (`capacity`), temel yeniden doldurma süresi (`baseReloadTime`) ve cadence modeli (`cadenceMode`) desteklenir:
- `OwnerAttackSpeedPercentage` modunda atış frekansı $FR_{final} = FR \times (1 + \max(0, AS) / 100)$ ve reload süresi $T_{reload} = T_{base} / (1 + \max(0, AS) / 100)$ formülüyle ölçeklenir.
- Mermi tüketimi başarılı atış (`FireOnce() == true`) gerçekleştiğinde yapılır; reload tamamlanana kadar atış bloke edilir.

## Empowered Shot ve Crit Politikası (Phase 3B.2)

`PrimaryWeaponDefinition`, projectile interval silahları için opsiyonel `empoweredShot` tanımı taşıyabilir. Tanım; `everySuccessfulShots`, `finalMagazineRounds`, bonus base damage, `sourceAttributeId + coefficient` owner scaling'i ve guaranteed crit içerir. Rounding empowered'a değil weapon-level `damageRoundingPolicy`'ye aittir. Runtime empowered durumunu yalnız başarılı atıştan hemen önceki magazine state'inden üretir; başarısız spawn ammo/cadence/empowered sırasını ilerletmez.

Fighter BasicRapidLaser bunun ilk kullanıcısıdır: her başarılı 6. fire ve son 6 round empowered'dır. Periyodik sayaç reload ile sıfırlanmaz; shotgun gibi çok projectile üreten silahlar bir volley'i tek fire sayar. Birleşen koşullar tek empowered sonuç üretir. Empowered projectile metadata'sı (`isEmpowered`, crit policy, rounding) projectile ile taşınır; bu aşamada ayrı görsel, status veya effect cadence yoktur.

Kritik kararının sahibi `DamagePayload.criticalPolicy`'dir: `Random`, `Guaranteed`, `Disabled`. Kritik çarpanının tek sahibi kaynağın `Owner.CriticalDamage` attribute'udur; başlangıç `1.5`tir. Zorunlu crit random crit zarını ikinci kez çalıştırmaz. Fighter'ın `ceil` kuralı critten sonra, Armor'dan önce uygulanır; tüm hasar türlerine yayılan genel bir rounding kuralı değildir.
- Ayrıntılı kontrat ve test kanıtları için bkz. [`docs/PROJECT_DOCUMENTATION.md`](../../PROJECT_DOCUMENTATION.md#45-phase-3b1--primary-weapon-magazine--reload-mimarisi-ve-cadence-modeli) ve `LightYearsGame/tests/PrimaryWeaponMagazineTests.cpp`.

## Ability bağlantısı

`FireWeaponAction`, bir `PrimaryWeaponDefinition` değerini ability action variant'ı içinde taşır. `GameAbilityActionExecutor`, attachment/attribute çözümlemesinden gelen weapon attributes ve primary-weapon damage tag'leri ile `PrimaryWeaponExecutionContext` kurar. Runtime state, bir `GameAbility` mevcutsa `mPrimaryWeaponRuntime` içine yönlendirilir; aynı `weaponId` korunduğunda feature state'i activation'lar arasında yaşayabilir. Standard/shotgun fiziksel delivery'si [[Projectile System]] içindedir; diğer weapon family'leri aynı actor yolunu kullanmaz.

`sas::GameplayAbilityInstance`, aktif olmayan persistent runtime için `TickInactive` çağrısını ayrıca yapar; `GameAbility::TickInactive` bunu `TickInactivePrimaryWeaponRuntime` ile birincil silah runtime'ına bağlar. Bu bağlantı heat benzeri feature'ların silah ateşlemiyorken de güncellenebilmesine olanak verir.

Ayrıntılı çağrı sırası: [[Primary Weapon Fire Lifecycle]].

## Sınırlar ve belirsizlikler

- `automaticFire` alanının bütün input/policy dallarındaki etkisi bu aşamada uçtan uca incelenmedi.
- Projectile, beam, wave ve arc'ın hedefleme/çarpışma/damage ayrıntıları bu notun kapsamı dışındadır.
- Registry için unregister/reset API'si doğrulanmadı.
- Weapon progression profile mevcut ve runtime'a unlocked upgrade tag'leri veriliyor; level/evolve kuralları ayrı progression aşamasında incelenmelidir.

## Kaynak doğrulaması

- İncelenen durum: dirty worktree; belge mevcut çalışma ağacını açıklar.
- `GasLiteCoreTests.cpp` içinde validation, registry extension, feature, heat, beam ve shipped weapon validation kapsaması bulundu; bu aşamada test/build çalıştırılmadı.
