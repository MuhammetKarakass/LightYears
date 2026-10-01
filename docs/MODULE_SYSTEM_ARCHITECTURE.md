# Modül Sistemi Mimarisi

**Tarih:** 2026-10-01 · **Durum:** Mimari tasarım (modül kodu yok) · **Oyun tasarımı:** [MODULE_SYSTEM_DESIGN.md](MODULE_SYSTEM_DESIGN.md)

Bu doküman oyun tasarımındaki kuralların koda nasıl oturacağını tanımlar: sahiplik, veri modeli, akış, mevcut sistemlere bağlantı noktaları ve test sınırları. Kod yolları 2026-10-01 kaynağında doğrulandı.

---

## 0. Mevcut durum (kaynakta doğrulandı)

| Konu | Durum | Kaynak |
|---|---|---|
| Eski attachment sistemi | **Kaldırıldı (2026-10-01).** Yetenek/ana silah başına soket modeliydi ve oyunda bağlı değildi. `AttachmentLoadout`, `AttachmentLoader`, `attachments.json`, `attachmentSlotCapacity`/`attachmentCapabilities`, `Attachment.*` tag/ID şeması ve `GameAbility` attachment kancaları silindi | Bu doküman §8 |
| Yetenek kategorileri | `Ability.<Kategori>.<Aile>.<Varyant>`; geçerli kategoriler Primary, Offense, Defense, Movement, Utility. Validator her yeteneğin **tam bir** kategori tag'i taşımasını ve ID ile eşleşmesini ister | `ContentIdSchema::IsAbilityCategory`, `GameplayTagSchema::IsAbilityCategory`, `GameAbilityDefinitionValidator.cpp` |
| Yetenek kapsamlı modifier | `LightYearsAbilitySystemComponent::AddScopedAbilityRule / RemoveScopedAbilityRule` handle döner; `ScopedAbilityRule{ requiredAbilityTags, blockedAbilityTags, attributeModifiers, levelBonus }`. Ekleme/çıkarma yetenekleri yeniden yapılandırır | `LightYearsAbilitySystemComponent.h/.cpp` |
| Kalkan/afterburner değerleri | `ShipRuntime` kendi `AttributeSystem`'inde `MaxShield`, `ShieldRegen`, `Afterburner*` tutar. Taban + `(seviye-1) × perLevel` + kaynak kimlikli kalkan katkıları | `src/gameplay/ship/ShipRuntime.cpp` |
| Owner attribute'ları | `CombatRuntime` → ASC owner `AttributeSystem::AddModifier` handle döner, `RemoveModifier(handle)` geri alır | `SpaceAbilitySystem/include/attributes/AttributeSystem.h` |
| Koşu durumu | `Player` (Object) gemiden uzun yaşar: `ShipProgression`, satın alınmış yetenek seviyeleri. Respawn'da `RestorePurchasedAbilityLevels` yeni gemiye yeniden uygular | `include/player/Player.h` |
| Düşman kalkanı | Düşmanlar da `ShipRuntime` kullanır | `src/gameplay/enemy/EnemyRuntime.cpp` |

## 1. İlkeler

- **Tek sahip:** Modül verisi `Player`'da (koşu durumu), uygulanmış etki `PlayerSpaceShip`'te (actor ömrü). Kural kararını yalnız saf yerleşim kuralları verir.
- **Saf çekirdek:** Yerleşim doğrulama ve etki çözümleme actor, dünya ve zaman bilmez. Girdi alır, sonuç döner, birim testlenir.
- **Tek yönlü akış:** UI loadout'u okur ve `Player` üzerinden komut gönderir. Gemi loadout revizyonunu izler ve uygular. HUD, gemi durumunu yalnız okur.
- **Tek kaynak:** Modül aileleri yetenek kategori tag'leridir (`GameplayTags::Ability::*`). Ayrı aile listesi tutulmaz.
- **Mevcut mekanizmayı kullan:** Yetenek kapsamlı modifier'lar yeni bir kapsam tipi değil, mevcut `ScopedAbilityRule`'dur.
- **Göreli sistem modülleri:** Kalkan/afterburner değerleri geminin JSON'unda kalır. Sistem modülü o sistemi **açar** ve göreli değiştirir. `ships.json` ve Faz 1 değerleri değişmez, düşmanlar etkilenmez.

## 2. Katmanlar

```
Veri (JSON + C++ sözleşme)        Saf çekirdek (test edilir)          Runtime (actor)
───────────────────────────       ─────────────────────────────       ─────────────────────────────
modules.json  ─► ModuleCatalog ─┐
ships.json    ─► ShipSchematic ─┼─► ModulePlacementRules (CanPlace)
                                └─► ModuleResolver ─► ResolvedModuleEffects ─► ShipModuleApplier
Player: ModuleLoadout (+revision) ────────────┘                               (PlayerSpaceShip üyesi)
                                                                               ├─ owner AttributeSystem
UI (MVVM) ─ okur ─► ModuleLoadout                                              ├─ ShipRuntime (sistem aç/kapa + attr)
          ─ komut ─► Player::TryPlaceModule / TryRemoveModule                  ├─ ASC ScopedAbilityRule (yetenekler)
                                                                               └─ ana silah modifier'ları (§6.3)
```

## 3. Veri modeli

### 3.1 Kimlikler

| Tür | Biçim | Not |
|---|---|---|
| Modül içeriği | `ContentId` `Module.<Aile>.<Ad>.<Varyant>` | `ContentIdSchema::ValidateModuleId` eklenir |
| Bölge | `GameplayTag` `Module.Zone.{Nose,Wings,Hull,Core,Engine,Sensor}` | `Hull` serbest bölgedir |
| Aile | Mevcut `GameplayTags::Ability::{Primary,Offense,Defense,Movement,Utility}` | Yeni tag yok |
| Sistem kategorisi | `GameplayTag` `Module.System.{Shield,Afterburner,HealthReadout,DamageReadout}` | Her kategoriden en fazla bir tane |

### 3.2 `ModuleDefinition` (C++ sözleşme, `modules.json`)

```cpp
struct ModuleShape { List<sf::Vector2i> cells; };            // (0,0) tabanlı hücre ofsetleri

enum class ModuleAttributeTarget { Owner, Ship, PrimaryWeapon };

struct ModuleAttributeModifier
{
    ModuleAttributeTarget target = ModuleAttributeTarget::Owner;
    sas::AttributeModifier modifier;
    bool affectedByZoneBonus = true;                          // ana değer = true
};

struct ModuleAbilityModifier                                  // ScopedAbilityRule'a çevrilir
{
    sas::AttributeModifier modifier;                          // ör. Common.Damage ×1.12
    bool affectedByZoneBonus = true;                          // hedef: modülün ailesindeki yetenekler
};

struct ModuleSizeVariant                                      // 2 ve 3 hücre sürümleri
{
    ModuleShape shape;
    List<ModuleAttributeModifier> attributeModifiers;
    List<ModuleAbilityModifier> abilityModifiers;
};

struct ModuleDefinition
{
    sas::ContentId moduleId;
    std::string displayName;
    GameplayTag family;                                       // Ability.Offense vb.
    GameplayTag systemCategory;                               // boşsa normal modül
    List<ModuleSizeVariant> sizeVariants;                     // sabit arketip = tek eleman
};
```

- **Tek aile:** Her modülün tek ailesi var, yeteneklerdeki tek kategori kuralıyla aynı.
- **Arketipler ayrı tip değildir.** Ölçeklenen modül iki varyantlı ve değerleri azalan verimlidir. Genişleyen modülün büyük varyantında fazladan bir efekt satırı vardır. Sabit modülün tek varyantı vardır.
- **Sistem modülü** `systemCategory` taşır ve kategorisinin bölgesine kilitlenir. Kalkan değerleri `Ship` hedefli göreli modifier olarak yazılır (Hızlı şarj: `MaxShield ×0.8`, `ShieldRegen ×1.5`; Ağır: `MaxShield +60`).
- **Tetikli efektler** (olayda X yap) ilk sürümde yok. Eklendiğinde ayrı bir `List<ModuleEventRule>` olur.

### 3.3 Bölge tablosu (`modules.json`, tek kaynak)

```json
"zones": [
  { "zone": "Module.Zone.Nose",   "family": "Ability.Primary",  "bonus": 0.25 },
  { "zone": "Module.Zone.Wings",  "family": "Ability.Offense",  "bonus": 0.25 },
  { "zone": "Module.Zone.Hull",   "family": "*",                "bonus": 0.10 },
  { "zone": "Module.Zone.Core",   "family": "Ability.Defense",  "bonus": 0.25 },
  { "zone": "Module.Zone.Engine", "family": "Ability.Movement", "bonus": 0.25 },
  { "zone": "Module.Zone.Sensor", "family": "Ability.Utility",  "bonus": 0.25 }
],
"systemZones": { "Module.System.Shield": "Module.Zone.Core", "Module.System.Afterburner": "Module.Zone.Engine",
                 "Module.System.HealthReadout": "Module.Zone.Sensor", "Module.System.DamageReadout": "Module.Zone.Sensor" },
"rebootDurationSeconds": 1.5,
"combatWindowSeconds": 3.0
```

### 3.4 Gemi şematiği (`ships.json` → `schematic`)

Izgara ASCII satırlarla yazılır, okunur ve diff'lenir. Örnek biçimi gösterir, hücre sayıları temsili değildir:

```json
"schematic": {
  "legend": { "N": "Module.Zone.Nose", "W": "Module.Zone.Wings", "H": "Module.Zone.Hull",
              "C": "Module.Zone.Core", "E": "Module.Zone.Engine", "S": "Module.Zone.Sensor" },
  "rows": [
    "..NN..",
    "SSNNSS",
    "WWHHWW",
    "WCCCCW",
    "HHCCHH",
    "EEEEEE"
  ],
  "defaultModules": [
    { "moduleId": "Module.Defense.ShieldGenerator.Standard", "variant": 0, "origin": [2, 3], "rotation": 0 },
    { "moduleId": "Module.Movement.Afterburner.Standard",   "variant": 0, "origin": [1, 5], "rotation": 0 }
  ]
}
```

`.` boş hücredir. Yükleyici şunları doğrular: legend dışında karakter yok, varsayılanlar hariç toplam 28 hücre, varsayılan modüller kurallara uygun yerleşiyor.

### 3.5 `ModuleLoadout` (koşu durumu, `Player` sahibi)

```cpp
struct ModulePlacement
{
    uint32_t instanceId = 0;          // Player içinde monoton
    sas::ContentId moduleId;
    int variantIndex = 0;
    sf::Vector2i origin;
    int rotationQuarterTurns = 0;     // 0..3
    ModuleRarity rarity = ModuleRarity::Common;
    List<sas::AttributeModifier> rolledSideStats;   // bölge bonusundan etkilenmez
};

class ModuleLoadout
{
public:
    const List<ModulePlacement>& GetPlacements() const;
    const List<ModulePlacement>& GetInventory() const;   // takılı olmayanlar
    uint64_t GetRevision() const;
    // mutasyonlar yalnız ModulePlacementRules onayından sonra Player çağırır
};
```

## 4. Saf çekirdek

### 4.1 `ModulePlacementRules`

```cpp
enum class ModulePlacementFailure { None, OutOfBounds, CellBlocked, Overlap, SystemZoneMismatch, DuplicateSystem, UnknownModule, InvalidVariant };

ModulePlacementFailure CanPlace(const ShipSchematic&, const ModuleCatalog&, const ModuleLoadout&,
                                const ModulePlacement& candidate, uint32_t ignoredInstanceId = 0);
```

Kontroller: döndürülmüş hücreler ızgarada ve `.` değil; çakışma yok (`ignoredInstanceId` taşıma içindir); sistem modülünün tüm hücreleri kendi bölgesinde; aynı sistem kategorisinden ikinci modül yok. Döndürme `(x, y) → (-y, x)` ve en küçük x/y sıfıra normalize edilir.

### 4.2 `ModuleResolver`

```cpp
struct ResolvedModuleEffects
{
    List<sas::AttributeModifier> ownerModifiers;
    List<sas::AttributeModifier> shipModifiers;
    List<sas::AttributeModifier> primaryWeaponModifiers;
    List<ScopedAbilityRule> abilityRules;       // requiredAbilityTags = { modül ailesi }
    List<GameplayTag> enabledSystems;           // Module.System.*
};

ResolvedModuleEffects Resolve(const ShipSchematic&, const ModuleCatalog&, const ModuleZoneTable&,
                              const List<ModulePlacement>& activePlacements);
```

Bölge bonusu kuralı (tek yer):
- Modülün **tüm hücreleri tek bölgede** ise o bölge değerlendirilir; taşan modül bonus almaz.
- Bölgenin ailesi modülün ailesiyle eşleşiyorsa `b = 0.25`, bölge Gövde ise `b = 0.10`, aksi halde 0.
- `affectedByZoneBonus` olan modifier'ın büyüklüğü ölçeklenir. `Add`: `m × (1+b)`. `Multiply`: `1 + (m−1) × (1+b)`. Böylece "+%12 hasar" eşleşen bölgede "+%15" olur.
- Yan statlar ve sistem açma bonusla ölçeklenmez.

## 5. Runtime

### 5.1 `Player` (sahip)

- `ModuleLoadout mModuleLoadout` üyesi.
- `TryPlaceModule`, `TryMoveModule`, `TryRemoveModule` önce `CanPlace` çağırır, sonra loadout'u değiştirir ve revizyonu artırır.
- Yeni gemi doğduğunda (ilk doğuş/respawn) gemiye loadout bağlanır; `RestorePurchasedAbilityLevels` ile aynı yerde.
- İlk doğuşta loadout boşsa şematiğin `defaultModules` listesi yerleştirilir.

### 5.2 `ShipModuleApplier` (`PlayerSpaceShip` üyesi)

```cpp
class ShipModuleApplier
{
public:
    void Bind(const ModuleLoadout&, const ShipSchematic&);      // respawn'da
    void Tick(float deltaTime, bool inCombat);                   // reboot sayaçları
    bool IsRebooting(uint32_t instanceId) const;                 // HUD/UI okur
private:
    void Reapply();                                              // hepsini sök, yeniden uygula
    List<sas::AttributeModifierHandle> mOwnerHandles, mShipHandles;
    List<std::size_t> mScopedRuleHandles;
    Map<uint32_t, float> mRebootRemaining;
    uint64_t mAppliedRevision = 0;
};
```

- **Tam yeniden uygulama:** Revizyon değişince tüm handle'lar kaldırılır ve hepsi yeniden eklenir. Modül sayısı ~10, maliyeti önemsiz; ayrı uygulama yolu hatası oluşmaz.
- **Yeniden başlatma:** Revizyonda yeni görülen `instanceId`, `inCombat` ise `rebootDurationSeconds` sayacına girer ve bitene kadar `activePlacements`'a dahil edilmez. Sökme anında etkilidir.
- **Owner modifier'ları:** ASC owner `AttributeSystem::AddModifier`.
- **Ship modifier'ları:** `ShipRuntime::GetAttributes().AddModifier`; taban hesaplaması `ShipRuntime`'da kalır.
- **Yetenek kuralları:** `AddScopedAbilityRule` / `RemoveScopedAbilityRule` handle'larıyla.
- **Sistemler:** `ShipRuntime::SetSystemEnabled` (§6.1).

### 5.3 Savaş durumu (`inCombat`)

Karar: gemi son 3 saniyede hasar aldıysa ya da verdiyse savaş içindedir. `CombatRuntime`'ın mevcut kaynak/hedef hasar olaylarından türetilir; okunur durumdur, oyun durumunu değiştirmez.

## 6. Mevcut sistemlere değişiklikler

### 6.1 `ShipRuntime`: sistem aç/kapa

```cpp
enum class ShipSystem { Shield, Afterburner };
void SetSystemEnabled(ShipSystem system, bool enabled);   // varsayılan: açık
```

- `RecalculateAttributes` kapalı sistemin taban değerini 0 yazar. Seviye büyümesi ve kalkan katkıları korunur, sistem açılınca geri gelir.
- Varsayılan "açık" olduğu için düşmanlar ve modülsüz yollar değişmez.
- Afterburner kapasitesi 0 olduğunda girişin afterburner'ı açmadığı mevcut davranışla doğrulanır; gerekirse `PlayerMovementComponent`'e açık kontrol eklenir.

### 6.2 Yetenek kapsamı: `ScopedAbilityRule`

- Offense/Defense/Movement/Utility aileli modüllerin yetenek etkisi `ScopedAbilityRule{ requiredAbilityTags = { aile } }` olarak eklenir.
- `AbilityActionAttributeResolver::ResolveAttributeValue` scoped modifier'ları yalnız `weaponDefinition` yokken uygular; yani ana silaha ulaşmaz (§6.3).
- Ekleme/çıkarma yetenekleri yeniden yapılandırır; modül değişiminin yetenek önbelleğine yansıdığı bir testle doğrulanır.

### 6.3 Ana silah (Primary ailesi)

Scoped kurallar ana silaha uygulanmadığı için Primary modülleri ayrı hedef taşır (`ModuleAttributeTarget::PrimaryWeapon`). **Uygulandı:** ASC kendi ana silah değiştirici listesini tutar (`AddPrimaryWeaponModifier` / `RemovePrimaryWeaponModifier`) ve `GetPrimaryWeaponModifierRevision()` ile revizyonunu açar. `AbilityActionAttributeResolver` PrimaryFire dalı bu listeyi sahip ölçeklemesinden **önce** uygular. Revizyon `FireWeaponRuntimeState` ve `GameAbility` primary önbelleğine girer; modül değişimi önbelleğe takılmaz. `ShipModuleApplier` bu listeyi yalnız kendi tutamaçlarıyla yönetir.

### 6.4 HUD göstergeleri

- `HealthReadout` ve `DamageReadout` sunum sistemleridir. Uygulayıcı `enabledSystems`'i gemide okunur durum olarak tutar.
- Vitals ve hasar sayısı Presenter'ları bu durumu okur ve View'ı gizler. UI gameplay'e geri yazmaz.
- **Uygulandı (adım 9):** `ShipModuleApplier::IsReadoutEnabled(tag)` = `!HasApplied() || IsSystemEnabled(tag)`; şematiksiz gemi (düşman, modülsüz akış) göstergeleri korur.
- `HealthReadout` → `VitalsPresenter::Tick` gemiden okur, `VitalsViewModel::healthReadoutVisible` yazar, `VitalsView` yalnız HP/zırh göstergesini (`displayHealth`, bariyer dahil) gizler. Kalkan ve enerji barları kendi sistemlerine bağlı kalır. Gemi yokken son durum korunur; yeni gemi ilk Tick'te durumu yeniden okur.
- `DamageReadout` → `GameHUD::ShipDamageResolved` oyuncu gemisi dışındaki hedeflerde (oyuncunun verdiği hasar) sayı üretmez. Oyuncunun aldığı hasar sayısı değişmez; hasarın kendisine dokunulmaz.

## 7. UI (modül ekranı)

- Oyunu **durdurmaz**. Mevcut MVVM/Presenter tabanında yeni bir ekran.
- ViewModel: şematik ızgarası, bölge renkleri, yerleşimler, envanter, sürüklenen modülün önizlemesi (`CanPlace` ve bölge bonusu: yeşil +%25 / +%10).
- Komutlar yalnız `Player::Try*` çağırır. Hata nedeni `ModulePlacementFailure`'dan metne çevrilir.
- Yeniden başlatılan modül `ShipModuleApplier::IsRebooting` ile yükleme göstergesi alır.

## 8. Tamamlanan ön temizlik (2026-10-01)

1. **Eski attachment sistemi kaldırıldı:** dosyalar, CMake kayıtları, `weapons.json` alanları, yetenek config'lerindeki `attachmentCapabilities`, `Attachment.*` tag/ID şeması, `GameAbility`/ASC/resolver kancaları ve ilgili testler. `GetResolvedDamageTags()` artık parametresiz, `AbilityActionAttributeResolver::ResolveAttributes` damage tag parametresi almıyor.
2. **Yetenek kategorileri:** `Ability.Control` silindi; Cryo Bola, Frost Maelstrom, Gravity Anomaly, Null Pulse → `Ability.Utility.*` (config'ler `gameConfigs/ability/utility/`). Scorch Drive ve Lance Drive → `Ability.Movement.*` (config'ler `gameConfigs/ability/movement/`).
3. **Doğrulama:** Tam build exit 0 (yalnız Faz 1'den kalan `GasLiteCoreTests.cpp` `shieldAffinity` hatası hariç). `GasLiteCoreTests.cpp`'deki o iki satır geçici olarak devre dışı bırakılıp tüm suite koşuldu: 41 testten 39'u geçti. Kalan iki hata Faz 1 değişikliklerine ait: `LightYearsContent` 4 gemi bekliyor (roster artık 9), `LightYearsGasLiteCore` telemetri testi ham 5 hasar bekliyor (Fighter artık Armor 2). Geçici değişiklik geri alındı.

## 9. Uygulama adımları

Her adım derlenir, testler koşar, davranış değişimi yalnız ilgili adımda olur.

**Durum:** adım 1-2 `343c785`, 3-4 `403ae1a`, 5-6 `a3044a5`, 7 `3daba62`, 8 `1844223` ile **tamamlandı**. Adım 9 **sürüyor** (kod ve testler yazıldı, görsel kabul bekliyor). Adım 10-11 başlamadı.

| # | Adım | Kanıt |
|---|---|---|
| 1 | Kimlikler (`Module.*` ContentId, `Module.Zone.*`, `Module.System.*`), `ModuleDefinition`, `ModuleCatalog`, `modules.json` yükleyici + doğrulayıcı | Yükleyici testleri: geçerli, bilinmeyen aile/bölge, boş varyant, bozuk şekil |
| 2 | `ShipSchematic` (ASCII ızgara) yükleme ve 6 gemi şeması | 28 hücre, legend, varsayılan yerleşim testleri |
| 3 | `ModulePlacementRules` + döndürme | Sınır, çakışma, sistem bölgesi, tek sistem, 4 döndürme |
| 4 | `ModuleResolver` + bölge bonusu | Eşleşen/Gövde/eşleşmeyen/taşan, Add/Multiply ölçekleme, yan stat ölçeklenmez |
| 5 | `ShipRuntime::SetSystemEnabled` | Kapalı kalkan 0, seviye büyümesi korunur, düşman değişmez |
| 6 | `ShipModuleApplier` + `Player` loadout + respawn bağlama; varsayılan modüller | **Davranış değişmez:** varsayılanlarla kalkan/afterburner bugünküyle birebir; respawn sonrası handle sızıntısı yok |
| 7 | Yeniden başlatma + `inCombat` (3 s) | Savaşta 1,5 s gecikme, savaş dışında anında, sökme anında |
| 8 | Yetenek kuralları (`ScopedAbilityRule`) + ana silah hedefi ve önbellek revizyonu | Modül değişince yetenek ve ana silah değeri hemen güncellenir |
| 9 | HUD göstergeleri | Gösterge sökülünce View gizlenir |
| 10 | Modül ekranı UI | Görsel kabul kullanıcıda |
| 11 | İlk 16 modül içeriği | İçerik doğrulama |

## 10. Açık kararlar

Sonraya bırakılanlar (tasarım dokümanı §12): düşme kaynakları, koşu sonu kalıcılık, envanter sınırı.
