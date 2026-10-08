# Modül Sistemi Mimarisi

**Tarih:** 2026-10-04 · **Durum:** Modül runtime'ı, ekranı ve Common11 uygulanmış; fiziksel sistem genişlemesi ayrı kapsamdır · **Oyun tasarımı:** [MODULE_SYSTEM_DESIGN.md](MODULE_SYSTEM_DESIGN.md)

Bu doküman sahiplik, veri modeli, akış ve sistem bağlantılarını tanımlar. Güncel Common kimlik/değer/olay sözleşmesi [uygulama planı §2–§3](MODULE_COMMON_IMPLEMENTATION_PLAN.md#2-modül-şartnamesi) ve `modules.json`'dadır. Aşağıdaki ilk tasarım kod örnekleri ile §9'daki eski adım kanıtları tarihsel olarak korunur; somut API için güncel kaynak esas alınır.

## Güncel Common çalışma zamanı (2026-10-04)

- `ModuleDefinition`/loader tipli tetik, hasar ve bölge-cooldown listelerinin sahibidir; JSON sayısal içeriği taşır. Resolver yan etkisizdir; tetik/hasar kimliği `(instanceId, ruleIndex)`'dir.
- Yerleştirme sahibi `ModulePlacementRules` aynı modül kimliğinin ikinci takılı örneğini `DuplicateModule` ile reddeder; taşıma/envanter kopyası davranışı korunur.
- `PlayerSpaceShip` tetik runtime'ını sahiplenir; applier kuralları eşitler. Buff refresh yığılmaz, değişmeyen pencere korunur, sökme/reboot temizler.
- Applier'ın RAII token'ı CombatRuntime hasar kurallarını taşır. Bonus Energy kritiksiz ve özyinelemesizdir; normal SourceDamageDealt bildirimi zincire açıktır. Dolu Şarjör, FireOnce metadata'sına kopyalanır; impact anında yeniden uygulanmaz.
- ASC ayrı cooldown handle'larını çözer; GameAbility başlangıçta yalnız modül çarpanını yakalar. Seviye/haste canlı kalır; modül taşıma çalışan cooldown'u yeniden kısaltmaz. Primary/pasifler dışlanır.
- Üretim kart formatter'ı sayıları JSON'dan çözer. Bağımsız final game build exit 0; hedefli Common E2E 317 kontrol geçti. [Plan §5](MODULE_COMMON_IMPLEMENTATION_PLAN.md#5-doğrulama-sonucu-ve-sınırları) komutları ve görsel kabul sınırını kaydeder. Aşağıdaki eski kod örnekleri güncel API sözleşmesinin yerine geçmez.

## 2026-10-03 fiziksel sistem sözleşmesi

- Oyuncu gemisi şematiğinde sistemler dahil toplam **46** kullanılabilir hücre bulunur. Varsayılan Life Support, Flight Control, Primary Weapon ve Q/E/F/R sistemleri ikişer hücre kaplar; önceki kalkan/afterburner/göstergelerle toplam doluluk **20**, boş alan **26** olur.
- `modules.json` sistem tanımlarının ve bölge kurallarının; `ships.json` şemanın ve varsayılan yerleşimlerin sahibidir. `ModuleLoadout` koşu durumunu, `ShipModuleApplier` uygulanmış sistem/modifier durumunu taşır.
- Life Support'un açık sökme komutu oyuncunun ölüm yoluna gider. Yaşam desteği olmayan mevcut koşu yerleşimiyle yeniden doğuş reddedilir; mevcut respawn başarısızlık yolu çalışır. İlk doğuş, aynı modülü taşıma ve yeniden başlatma filtresi ölüm nedeni değildir. `Actor::Destroy()` eşzamanlı bildirim yaydığı için yeni geminin dinleyicileri bağlanmadan yok edilmesi kullanılmaz.
- Flight Control durumu oyuncunun A/S/D hareket taleplerini kapatır; W ve fareye dönüş etkilenmez.
- Primary ve Q/E/F/R modülleri mevcut yetenek örneklerinin kullanılabilirliğini kontrol eder. Sökme/takma yeteneği yeniden grant ederek seviye, cooldown veya ammo sıfırlamaz. Aktif kullanım kapatılır; daha önce dünyaya bırakılmış atışlar normal ömrünü sürdürür.
- Varsayılan ability slot eşlemesinin tek sahibi `gameplay/ability/loadout/DefaultAbilityLoadout.cpp` olarak kalır. Modül ekranı takılı slotun gerçek yeteneğini okur.
- Bu fiziksel sistem genişlemesinin 2026-10-03 doğrulama sınırında test çalıştırılmaması kararı verilmişti. Common modül kapanışında hedefli gerçek-gemi E2E ve `VSLANG=1033` ile `LightYearsGame` derlemesi ayrıca kullanılır; derleme tek başına oyun içi kabul değildir. Güncel kanıt uygulama planı §4 P7'dedir.

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

Uncommon P5, yukarıdaki ilk sürüm modeline `ModuleRulePackage::statusRules` ekler. Electric stack değer tablosu, Cryo donma, Thermal+Cryo süre bonusu ve donma yayılımı ayrı tipli listelerdir; ana ve yan özellik aynı loader/level resolver yolunu kullanır. `ShipModuleApplier` çözülmüş değerleri kendi `CombatRuntime`'ına kopyalar ve sökme/teardown temizler. `DamageTypeSystem` vuruşun sahibinden bu değerleri okur; global status denge dosyası korunur. Hedefte uygulanan Electric profilinin değer snapshot'u effect üzerinde, donma beklemesi hedefin süreli effect'i üzerinde yaşar; actor registry veya süreç-geneli durum eklenmez. [P5 sözleşmesi ve kanıt sınırı](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md#p5--status-kuralı-geçersiz-kılma-doğrudan-codex), [tipli sözleşme](../LightYearsGame/include/gameplay/module/ModuleDefinition.h).

**P8 sonrası eksik yanların tamamlanması (2026-10-08):** Genel `durationBonuses`, koşullu Thermal+Cryo listesinden ayrıdır. O4 kendi Ignite/Cryo/Electric/Kinetic uygulamasına ve Frozen süresine +1 sn ekler; freeze cooldown'u uzatmaz. Aynı tür yenilemede süre güncel taban + sahip bonusudur; birikmez. Suppressed/generated uygulama modül bonusunu taşımaz. Değerler JSON'dan gelir.

O5'in geçmişi kaynağın `CombatRuntime`'ına ait `ModuleFirstHitHistory` içinde yaşar: hedefin monoton `GetUniqueID()` anahtarı, weak actor ve generation korumalı RAII claim. Hasar hazırlanırken rezerve edilir, yalnız `appliedDamage > 0` olduğunda ve hasar olaylarından önce kesinleşir; engellenen/sıfır hasar rezervasyonu düşer. Hasar bağlamı claim'e yalnız weak referans taşır; bir olay kaydının kopyalanması kaynak işlemin rezervasyon ömrünü uzatmaz. Nested hit bonusu çoğaltamaz. İlgisiz rebuild ve sök/tak geçmişi korur; expired hedefler Tick'te, runtime Clear'da tüm geçmiş temizlenir. Yeni engine registry'si yoktur.

`ModuleOwnerAttributeRule` hedef/işlem alanları mevcut Owner/Add varsayılanını korur; Ship için yalnız HealthThreshold + ShieldRegen/Multiply desteklenir. Modifier temizliği kendi attribute sisteminde yapılır; Owner karşılaştırmasının hariç listesi Ship handle'larını içermez. `movementFadeRules` aynı modülün ana hız bonusunu 1,5 sn söndürür, ayrı Attack Speed üretmez; sökme/ölüm anında temizlenir.

Koşu içi Gold'un tek sahibi `Player` içindeki `RunGold` cüzdanıdır. Gemi applier'ına weak cüzdan verilir; G1 ve Hunt Bounty ayrı `AwardGold` eylemleriyle sahibin normal düşman öldürmesine +1 verir. Gold gemi yenilenmesinde korunur, yeni koşu sıfırlamasında yeni cüzdanla 0 olur. Mağaza, temel düşüş, harcama ve kalıcılık bu sözleşmeye dahil değildir. [Sahip](../LightYearsGame/src/player/Player.cpp), [reaktif runtime](../LightYearsGame/src/gameplay/module/ModuleReactiveRuntime.cpp).

Uncommon P6, `generatedRules` altında Ricochet, güdümlü salvo, gecikmeli patlama, iz mayını ve koşullu ölüm patlamasını ayrı tiplerle çözer. `ShipModuleApplier` kendi `ModuleGeneratedRuntime` üyesini besler; observer RAII tutamaçlıdır. Bekleyen aktörler modül instance'ına ait generation lease'e yalnız weak erişir; sökme, seviye/kural değişimi ve yıkım bunları iptal eder. Aktörlere erişim weak_ptr, bütçe kimliği geminin monotonic unique ID'si + lease generation'dır. Aynı kurallarla yeniden çözümleme mevcut işleri korur.

`DamageHitGroup`, aynı Primary atışının tüm mermileri/hedefleri veya tek ability kullanımının gecikmeli aktörleri arasında paylaşılır. Sürekli beam her damage step için tek bütçe açar; bütün muzzles bunu paylaşır. First-hit hasarı ve ölümün MaxHealth/status/location snapshot'u değer olarak taşınır. Üretilen damage hem `skipModuleDamageRules` hem `moduleTriggerPolicy=Suppressed` taşır; aktarılmış Ignite'ın sonraki tick'leri de suppression'ı korur. Snapshot'tan hesaplanan damage `AlreadyResolved` kaynak ölçeklemesini, düz mayın hasarı normal kaynak ölçeklemesini kullanır. Hedefin normal kalkan/zırh çözümü korunur. Mayın mesafesi MovementComponent'in gerçek tick mesafesidir; portal/teleport yol sayılmaz.

Üç somut aktör ailesi mevcut typed `World::SpawnActor` yolunu kullanır. Profiller `presentation/ability/moduleGenerated/` altında ve `PresentationProfileRegistry<ConcreteProfile>` ile kayıtlıdır; genel actor tanımına ayrı sunum alanı eklenmez. [Runtime sahibi](../LightYearsGame/src/gameplay/module/generated/ModuleGeneratedRuntime.cpp), [kanonik değerler](ABILITY_SCALING_CATALOG.md#uncommon-p6-üretilen-hasar), [P6 fazı](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md#p6--üretilen-varlıklar-doğrudan-codex).

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

`.` şema dışında kalan hücredir. Yükleyici şunları doğrular: legend dışında karakter yok, sistemler dahil toplam 46 kullanılabilir hücre, varsayılan modüller kurallara uygun yerleşiyor. Üstteki JSON ilk tasarımdaki biçim örneğidir; güncel yerleşimler `ships.json` içindedir.

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
- **Uygulandı (adım 10):** `presentation/hud/modules/` altında bir ViewModel + Presenter + View + `ModuleScreenHUDController`. **Tab** açar/kapatır (`GameLevel::DispatchEvent` yalnız oyun duraklatılmamışken `HUDController::HandleEvent` çağırır); oyun durmaz. Açıkken fare ekrana gider; ana silah klavyeden (Space) ateşlendiği için tıklamalar ateş etmez, WASD/yetenek tuşları çalışmaya devam eder (bu yüzden döndürme R değil **sağ tık / T / tekerlek**). Sol tık: envanterden/yerleşik modülü al, ızgaraya bırak (kurulum veya taşıma), envanter alanına bırak ya da **Del** ile kaldır; sürükle-bırak da çalışır. Önizleme yeşil/kırmızı `CanPlace`, etiket yalnız bölge bonusu varsa (+%25/+%10). Presenter oyun durumunu tek yönlü okur; komutlar yalnız `Player::TryInstallModule/TryMoveModule/TryUninstallModule` ile gider. Yeniden başlatılan modül kararır, süre ve ilerleme çubuğu gösterir. Şematiksiz gemi "no module slots" mesajı görür. Test: `LightYearsModuleScreen`.

**Uncommon P7 kart sözleşmesi (2026-10-08):** Presenter gerçek `ModulePlacement::level`, mevcut şekilden hücre merdiveni ve `sideFeatureId` seçimini salt okunur `ModuleEntryView`'a taşır. Kilit/uygunluk mevcut `IsSideFeatureUsableAtLevel` ve `IsLargestCellCountVariant` kurallarıyla belirlenir; kart yeni bir gameplay kararı üretmez. `ModuleLevelTable::ValueAtLevel` runtime resolver ve kart numeric projeksiyonunun ortak tablo okuma sahibidir. `gameplay/module/ModuleDescriptionValues` tipli numeric anahtarları JSON template doğrulamasına ve `ModuleDescriptionFormatter`'a verir; metin biçimlendirmesi sunumda, isteğe bağlı yan açıklamalar JSON/loader sözleşmesindedir.

`ModuleCardTextLayout` kullanılan SFML fontunun gerçek genişliğiyle UTF-8 kod noktalarından satır üretir. Tam kart ana açıklama için üç, yan açıklama için iki satır taşır; envanter tek satırlık özeti sığmıyorsa üç noktayla kısaltır ve üzerine gelince tam kart gösterir. Mevcut hologram/grid ve komut yolu korunur. E2E sahibi: [UncommonModuleP7E2E.cpp](../LightYearsGame/tests/UncommonModuleP7E2E.cpp); faz/kanıt: [P7](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md#p7--sunum-doğrudan-codex). Bu geometri doğrulaması kullanıcı görsel kabulünün yerine geçmez.

## 8. Tamamlanan ön temizlik (2026-10-01)

1. **Eski attachment sistemi kaldırıldı:** dosyalar, CMake kayıtları, `weapons.json` alanları, yetenek config'lerindeki `attachmentCapabilities`, `Attachment.*` tag/ID şeması, `GameAbility`/ASC/resolver kancaları ve ilgili testler. `GetResolvedDamageTags()` artık parametresiz, `AbilityActionAttributeResolver::ResolveAttributes` damage tag parametresi almıyor.
2. **Yetenek kategorileri:** `Ability.Control` silindi; Cryo Bola, Frost Maelstrom, Gravity Anomaly, Null Pulse → `Ability.Utility.*` (config'ler `gameConfigs/ability/utility/`). Scorch Drive ve Lance Drive → `Ability.Movement.*` (config'ler `gameConfigs/ability/movement/`).
3. **Doğrulama:** Tam build exit 0 (yalnız Faz 1'den kalan `GasLiteCoreTests.cpp` `shieldAffinity` hatası hariç). `GasLiteCoreTests.cpp`'deki o iki satır geçici olarak devre dışı bırakılıp tüm suite koşuldu: 41 testten 39'u geçti. Kalan iki hata Faz 1 değişikliklerine ait: `LightYearsContent` 4 gemi bekliyor (roster artık 9), `LightYearsGasLiteCore` telemetri testi ham 5 hasar bekliyor (Fighter artık Armor 2). Geçici değişiklik geri alındı.

## 9. Önceki uygulama adımları (2026-10-03 tarihsel anlık görüntü)

Bu 11 adımlı tablo fiziksel sistem/modül temelinin ilk uygulama sırasını kaydeder. Common11 fazlarının güncel durumu veya kapsamı değildir; güncel faz/commit kaydı [uygulama planı §4](MODULE_COMMON_IMPLEMENTATION_PLAN.md#4-fazlar-sol-yönetir-luna-uygular) altındadır.

**Tarihsel durum (2026-10-03):** adım 1-2 `343c785`, 3-4 `403ae1a`, 5-6 `a3044a5`, 7 `3daba62`, 8 `1844223` ile tamamlandı; adım 9 görsel kabul bekliyordu; 10-11 başlamamıştı. Bu durum satırı güncel faz ilerlemesini göstermez.

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
