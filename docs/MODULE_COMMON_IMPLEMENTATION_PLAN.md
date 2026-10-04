# Common Modüller — Sol/Luna Uygulama Planı

**Tarih:** 2026-10-04 · **Durum:** P0–P7 tamamlandı; faz kapanış commit’i bu belgeyle kaydedilir · **Tasarım:** [MODULE_SYSTEM_DESIGN.md](MODULE_SYSTEM_DESIGN.md), [MODULE_SYSTEM_ARCHITECTURE.md](MODULE_SYSTEM_ARCHITECTURE.md)

Bu belge mevcut Common modül setinin (10 çekirdek + P6'da uygulanabilirliği doğrulanıp eklenen opsiyonel M11) uygulama ve davranış sözleşmesidir. Kimlikler ve ayarlanabilir değerler için güncel kaynak `modules.json` ve §2'dir. P0 başlangıcı ve P1–P6 commit'leri §4'te kayıtlıdır; P7 aynı belge/kart/E2E kapanış commit'idir.

---

## 0. Kapsam

| Dahil | Hariç (bu plan değil) |
|---|---|
| 10 çekirdek Common modül (M01–M10) | Uncommon ve üstü modüller |
| Opsiyonel M11 Dolu Şarjör (P6 spike'ı uygulanabilirliği doğruladı; şu an şarjörlü üç Primary içerikte çalışır) | Bağlantı modülleri (Yükseltici, Röle, Ayna, Rezonatör, Yankılayıcı) |
| Tetik/zamanlı buff çalışma zamanı | Çok boyutlu sürümler, Aşırı Yük satırı (bu aşamada tek temel sürüm) |
| Giden hasar kuralı kancası | Nadirlik, kalite ve Scrap geliştirmesi |
| Cooldown kural yolu, bölge etkisi | Slot varyantları / Unique sınıfı (Yakıt Hattı, Q varyantları) |
| Modül açıklama metni ve modül ekranında gösterimi | Yetenek evrimi, loot/düşme, envanter sınırı |

Toparlanma Hattı Common'dan çıkarıldı (Uncommon'a kalır) ve bu planda yok.

---

## 1. Başlangıç durumu (P0 öncesi kaynak anlık görüntüsü, 2026-10-03)

Bu tablo uygulama başlamadan önceki anlık görüntüdür; P1–P6 sonuçlarını veya bugünkü çalışma ağacını tarif etmez. Güncel modül değerleri §2 ve `modules.json`'dadır.

| Konu | Bulgu | Kaynak |
|---|---|---|
| Modül veri modeli | `ModuleDefinition` yalnız `attributeModifiers` (Owner/Ship/PrimaryWeapon) ve `abilityModifiers` taşıyor. **Tetik, koşul, süre, hasar kuralı alanı yok** | `include/gameplay/module/ModuleDefinition.h` |
| Uygulama | `ShipModuleApplier` tüm modifier handle'larını tam yeniden uygular; reboot ve savaş penceresi var | `include/gameplay/module/ShipModuleApplier.h` |
| Çözümleme | `ResolvedModuleEffects` = owner/ship/primary modifier listeleri + `ScopedAbilityRule` + `enabledSystems` | `include/gameplay/module/ModuleResolver.h` |
| Olay yolu | `LightYearsAbilitySystemComponent::onGameplayEvent` ability lifecycle olaylarında yayınlanıyor. `DamageTaken`, `DamageDealt` (kaynağın ASC'sine), `KillConfirmed` olayları `CombatRuntime::NotifyDamageResolved` içinde üretiliyor. Normal `HandleGameplayEvent` yolunun da `onGameplayEvent`'i yayınladığı **Luna doğrulayacak** | `CombatRuntime.cpp:508-565`, `LightYearsAbilitySystemComponent.cpp` |
| Kalkan kırılması | **Hazır olay yok.** `ShieldComponent::onShieldDamaged(absorbed, shield, max)` var; kırılma = önceki kalkan > 0 ve yeni kalkan == 0 | `ShieldComponent.cpp:80-107` |
| Hasar uygulama | Tek giriş `ApplyCombatDamage(...)` (kritik, giden hasar çarpanı, `DamageContext` kurulumu, `ReceiveDamage`) | `src/gameplay/combat/Combatant.cpp:50-157` |
| Kaynak ayrımı | `DamageContext.sourceAbilityTags` ile Primary/Offense ayrımı yapılabilir. Hasar türü `damageTags` içinde (`Damage.Type.Energy` vb.) | `DamageContext.h` |
| Savaş durumu | `PlayerSpaceShip::IsInCombat()` (3 sn pencere) mevcut | `PlayerSpaceShip.h:33` |
| Geçici hareket çarpanı | `ShipRuntimeModifiers::Set(sourceId, ShipRuntimeModifier{movementSpeedMultiplier,...})` kaynak kimlikli, zaten var | `ShipRuntimeModifiers.h` |
| **Cooldown yolu** | `GameAbility::ResolveCooldownDuration()` yalnız `mDefinition.attributeModifiers` ve `AbilityHaste` kullanıyor. **`ScopedAbilityRule` cooldown'a ulaşmıyor** (`ApplyScopedAbilityModifiers` yalnız `AbilityActionAttributeResolver` içinde çağrılıyor). Bölgesel Soğutucu için yeni yol şart | `GameAbility.cpp`, `AbilityActionAttributeResolver.cpp:61` |
| Şarjör durumu | `PrimaryWeaponRuntimeState::magazineState{roundsRemaining, reloadRemaining}` var | `PrimaryWeaponHandler.h:22-37` |
| Mevcut içerik | `modules.json` içinde `ArmorPlate`, `EnergyCell`, `FocusedBarrel`, `ThrustBooster` zaten var (çok boyutlu). Bunlar M01–M04 ile aynı fikir | `assets/content/data/modules.json` |

---

## 2. Modül şartnamesi

Değerler **başlangıç**tır (kullanıcının listesinden). Hepsi `modules.json`'da ayarlanır. "Yeni teknoloji" sütunu hangi plan fazının modülü açtığını gösterir.

| # | Kimlik (`Module.<Aile>.<Ad>.Basic`) | Ad | Aile | Şekil | Etki | Mekanizma | Faz |
|---|---|---|---|---|---|---|---|
| M01 | `Defense.HeavyArmorPlate` | Ağır Zırh Plakası | Defense | 3 (I) | `Owner.Armor` +30 | Sade owner modifier | P1 |
| M02 | `Utility.EnergyCoil` | Enerji Bobini | Utility | 2 | `Owner.Energy.Power` +24 | Sade owner modifier | P1 |
| M03 | `Primary.FocusedBarrel` | Odaklı Namlu | Primary | 2 | Ana silah `Common.Damage` ×1,10 | Mevcut PrimaryWeapon modifier | P1 |
| M04 | `Movement.ThrustBooster` | İtki Artırıcı | Movement | 2 | `Owner.MoveSpeedHorizontal/Vertical` +2,2 rating | Mevcut owner modifier | P1 |
| M05 | `Offense.AttackRhythm` | Saldırı Ritmi | Offense | 2 | Offense yeteneği kullanınca 3 sn `Owner.AttackSpeed` +15 rating. Stack yok, süre yenilenir | Tetik → zamanlı owner buff | P3 |
| M06 | `Defense.ShatterCurrent` | Kırılma Akımı | Defense | 2 | Kalkan kırılınca 5 sn boyunca her Primary atışa **ek Energy hasar bileşeni** (atış hasarının %30'u) | Tetik → zamanlı hasar kuralı | P4 |
| M07 | `Offense.EnergyFeedback` | Enerji Geri Beslemesi | Offense | 1 | Energy hasarı verince 4 sn `Energy.Power` +16. Stack yok, süre yenilenir | Tetik (hasar türü) → zamanlı buff | P3 |
| M08 | `Movement.EscapeReflex` | Kaçış Refleksi | Movement | 2 | Hasar alınca 2 sn hareket hızı ×1,25. Stack yok, süre yenilenir | Tetik → zamanlı gemi çarpanı | P3 |
| M09 | `Offense.FinishingBlow` | Son Darbe | Offense | 2 | Canı ≤ %25 olan düşmana verdiğin hasar ×1,20 | Giden hasar kuralı (koşullu) | P4 |
| M10 | `Utility.ZoneCooler` | Bölgesel Soğutucu | Utility | 3 (I) | Bulunduğu bölgenin ailesinin cooldown'u ×0,79. Gövde'de tüm aktif yetenek cooldown'ları ×0,90 | Konuma bağlı kural + yeni cooldown yolu | P5 |
| M11 (opsiyonel) | `Primary.FullMagazine` | Dolu Şarjör | Primary | 2 | Tam şarjörün ilk Primary atışı ×1,40 hasar; şu an yalnız şarjör kullanan üç Primary içerikte etkin | Atış çözümlemesinde şarjör koşulu | P6 |

### Mekanizma ayrıntıları

- **M05:** `Event.Ability.Activated` olayı ve `GameplayTags::Ability::Offense` filtresi 3 saniyelik `Owner.AttackSpeed` +15 rating buff'ını yeniler; yığılma yoktur. Primary etiketi taşıyan aktivasyonlar bu Offense tetiğini açmaz. Beş ayrık oyuncu silahı bu rating'i mevcut cadence veya FireRate ölçeklemesinde kullanır; Continuous Heat Laser için DPS eşlemesi eklenmez. Bu owner rating yoludur; `Common.FireRate` Primary modifier'ı veya beam DPS eşlemesi değildir.
- **M06:** Kalkan kırılması = `ShieldComponent::onShieldDamaged` içinde önceki kalkan > 0 ve yeni kalkan == 0. Ek bileşen: `Damage.Type.Energy`, `criticalPolicy = Disabled`, atış hasarının oranı kadar. Ek bileşenin kendisi yeni ek bileşen doğurmaz (özyineleme koruması, §3.4). Ek bileşen normal `DamageDealt` olayını üretir; bu M07 ile zincir kurar ve **bilerek serbest**.
- **M07:** Tetik yalnız kaynağın ASC'sine gelen `Event.Source.DamageDealt` olayıdır; `payloadTags` içinde `Damage.Type.Energy` aranır.
- **M08:** Tetik = `Event.Owner.DamageTaken` (`appliedDamage > 0`). Etki `ShipRuntimeModifiers::Set` ile, kaynak kimliği modül örneğine özel.
- **M09:** Hasar anında hedef sağlık oranı kontrol edilir. Hedefin `HealthComponent` sağlık oranı `ApplyCombatDamage` içinde okunur; çarpan hasar kritik çarpanından önce uygulanır.
- **M10:** Modülün tüm hücreleri tek bölgede olmalıdır; bölgeye göre cooldown kuralı seçilir: Kanat→Offense, Çekirdek→Defense, Motor→Movement, Sensör→Utility ×0,79; Gövde→Primary hariç etkin yetenekler ×0,90. Burun'da cooldown kuralı ve atış hızı bonusu yoktur (kullanıcı kararı, 2026-10-03). Etkiler bölge bonusuyla ayrıca ölçeklenmez.
- **M11:** P6 spike'ı, tam şarjör koşulunun Primary atış çözümlemesinde mevcut şarjör durumu ile uygulanabildiğini doğruladı. Etki mevcut `weapons.json` şarjörleriyle Fighter Rapid Laser (48), Dual Kinetic Blaster (64) ve Cone Rapid Shotgun (18) üzerinde geçerlidir; şarjörsüz silahlar için koşul oluşmaz.

---

## 3. Mimari

### 3.1 Veri modeli (tipli sözleşmeler)

`ModuleSizeVariant`, birbirinden ayrı `ModuleTriggerBuff`, `ModuleDamageRule` ve `ModuleZoneEffect` listelerini taşır. Tam alan/default/enum sözleşmesinin sahibi [ModuleDefinition.h](../LightYearsGame/include/gameplay/module/ModuleDefinition.h)'dır; burada ikinci bir C++ tanımı tutulmaz.

- Tetikler `AbilityActivated`, `ShieldBroken`, `DamageTaken`, `SourceDamageDealt` olarak ayrılır. Buff süre, olay etiketi filtresi, modifier listesi ve hareket çarpanı taşır.
- Hasar koşulları `PrimaryShotOnly`, `TargetHealthAtMost`, `FullMagazineShot` olarak ayrılır. Ek bileşen türü/oranı ile `bonusCriticalPolicy` açık alanlardır. `FullMagazineShot` yalnız çarpan destekler; ek bileşen verisi yükleyicide reddedilir.
- Cooldown kapsamları `FamilyOfZone` ve `AllActiveNonPrimary`'dir. Bölge etkileri açık, geçerli bölge etiketleriyle yazılır; boş etiketli fallback yoktur.
- Loader bilinmeyen seçicileri/etiketleri, geçersiz aralıkları, negatif süreleri ve sonlu olmayan değerleri reddeder. Sayısal kart yer tutucuları aynı veriden çözülür.

### 3.2 Çözümleme

`ModuleResolver` yerleşimi yan etkisiz ve kararlı sırada çözer. `ResolvedModuleEffects` modifier/sistem listelerine ek olarak tetik, hasar ve cooldown kurallarını taşır. Tetik ve hasar girişleri `(instanceId, ruleIndex)` ile izlenir; cooldown girişleri örnek, bölge, kapsam, aile ve çarpanı taşır. Tam şeklin tek bölgeye sığması bölge seçiminin şartıdır. `ModulePlacementRules::CanPlace` aynı kimliğin ikinci takılı örneğini `DuplicateModule` ile reddeder; kendi örneğini taşıma ve envanter kopyası korunur.

### 3.3 Çalışma zamanı: `ModuleTriggerRuntime`

- Sahibi `PlayerSpaceShip`; `ShipModuleApplier` çözülmüş kuralları `SetRules` ile eşitler. ASC `onGameplayEvent` ve kalkan `onShieldDamaged` abonelik handle'ları runtime'ın destructor'ında çözülür.
- Buff anahtarı `(instanceId, ruleIndex)`'dir. Yeniden tetikleme süreyi yeniler, yığılmaz. Değişmeyen kuralların aktif süresi korunur; sökme/reboot ilgili pencereyi temizler.
- M05/M07 owner attribute handle'ları, M08 örneğe/kural indeksine özel `ShipRuntimeModifiers` kaynağı kullanır. Kalkanın pozitif hasar emip sıfıra inmesi CombatRuntime'daki `ShieldBroken` hasar penceresini açar; doğrudan kalkan değerini sıfırlama bunu tetiklemez.
- Callback içinde kural değişmesi ertelenir; handle temizliği ve olay snapshot'ları reentrant sökme/takma akışını korur. Zamanlayıcılar gerçek gemi/combat tick yolunda ilerler. Yeni bir görsel buff sistemi bu fazların kapsamı değildir.

### 3.4 Giden hasar kuralı (CombatRuntime)

- Applier, `CombatRuntime::RegisterOutgoingDamageRules()` ile move-only `OutgoingDamageRuleRegistration` token'ını sahiplenir; `ReplaceRules` ile listeleri eşitler. Token zayıf kayıt durumu ve generation guard ile eski kaydın yeni kaydı silmesini engeller.
- `ApplyCombatDamage` hedefin vuruştan önceki sağlık oranından M09 çarpanını ve Primary etiketli vuruş için M06 bonus planını kopyalar. Ana hasar mevcut source-wide/kritik yolundan geçer.
- Bonus ayrı Energy payload'ı, `DamagePayload::skipModuleDamageRules=true` ve `DamageCriticalPolicy::Disabled` ile uygulanır. Yeni bonus doğurmaz; normal SourceDamageDealt bildirimi M07'yi tetikleyebilir. Kaynak kimliği/etiketleri callback öncesi kopyalanır, callback sonrası aktör ömrü kontrol edilir.
- M11 genel impact kurallarında uygulanmaz. `ResolveFullMagazineShotMultiplier()` aynı kayıt listesinden atış çarpanını okur; `PrimaryWeaponExecutionSystem::FireOnce` bunu mühimmat tüketiminden önce `PrimaryWeaponShotMetadata::shotDamageMultiplier` içinde yakalar. Ortak primary hasar çözümü bir kez uygular; Shotgun falloff bundan sonra gelir. Eski mermi, modül sökülse veya şarjör değişse de snapshot'ını korur.

### 3.5 Cooldown yolu (ASC)

- ASC `AddScopedAbilityCooldownRule` / `RemoveScopedAbilityCooldownRule` handle'larıyla ayrı etiket süzgeçli kuralları tutar. `GetScopedAbilityCooldownMultiplier` yalnız aktif loadout slotlarını çözer; Primary ve pasifler dışarıdadır. Yeni bir cooldown önbelleği veya revizyon yolu eklenmez.
- `GameAbility::ResolveCooldownDurationForStart` başlangıç anındaki modül çarpanını yakalar. OnActivation için SAS başlangıç kancası, OnAbilityEnd için GameAbility bitiş yolu kullanılır.
- Devam eden cooldown aynı modül çarpanını korur; taşıma/sökme kalan süreyi geriye dönük yeniden kısaltmaz. Seviye ve AbilityHaste'in mevcut canlı çözümlemesi korunur; tüm süre dondurulmaz.
- `ScopedAbilityRule` aksiyon değerlerinin sahibi olarak kalır. Modül cooldown handle'ları applier sökülünce/destructor'da temizlenir.

### 3.6 Kullanıcıya gösterim

- `modules.json` `description` alanı modül ekranının envanter satırında/tutulan modül bilgisinde gösterilir (mevcut `ModuleScreenView/Presenter`).
- Türkçe karakterlerin yazı tipi desteği daha önce şüpheli bulundu; kart metni için aynı sınırlama geçerli, **görsel kabul kullanıcıda**.

---

## 4. Fazlar (Sol yönetir, Luna uygular)

**Çalışma kuralları:**
- Aynı anda **tek yazıcı ajan**. Salt okunur denetimler paralel olabilir.
- Her faz bir **commit** ile kapanır; yalnız o fazın dosyaları stage edilir. Başka oturumun dirty dosyalarına dokunulmaz.
- Her faz sonunda Sol bağımsız olarak derler ve diff'i okur; ajan raporuna güvenilmez.
- Windows derleme komutu (PowerShell): `cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul && set VSLANG=1033 && cmake --build D:\LightYears\build --target LightYearsGame --parallel 4'`

### P0 — Ön kontrol (Sol)
- `git status` ve son commit'ler: motor refactor'ının durumu (GameplayEffect sadeleştirmesi, ActorComponent, Physics→World) kayda geçirilir.
- Doğrulama: mevcut E2E tercihiyle hedefli Common senaryosu kullanıldı; Burun kuralı ve kopya örnek kuralı 2026-10-03 kullanıcı kararıyla kapandı.
- **Çıktı:** başlangıç commit'i ve P1–P2'nin güvenle başlayabileceği onayı. P3 ve sonrası, refactor ASC/`PlayerSpaceShip`/`CombatRuntime`'ı değiştiriyorsa **refactor bitene kadar bekler** veya onun üstüne rebase edilir.

**Doğrulanmış faz sırası:** P0 `aac98b5` (başlangıç); P1 `51c531c`; P2 `9d39eba`; P3 `55e5b08`; P4 `0579f29`; P5 `35effde`; P6 `396bb78`. P7 bu belge, kart ve E2E kapanış commit'iyle kaydedilir (kendi hash'i belge içinde yinelenmez).

### P1 — Sade modüller ve içerik uzlaştırma (Luna A)
- Dosyalar: `assets/content/data/modules.json`, `ModuleLoader.cpp`, `ModuleDefinition.h` (yalnız `description`), modül ekranı sunumu.
- Yap: M01–M04'ü tek temel sürümle tanımla. Mevcut `ArmorPlate`, `EnergyCell` benzerleri **birleştirilir/yeniden adlandırılır**, kopya bırakılmaz. Eski 15 modülden M01–M11 listesinde olmayanlar (AttackMatrix, OverloadCore, PiercingTip vb.) **dokunulmadan** kalır, "geçici" olarak not edilir.
- `description` alanı ve yükleyici doğrulaması.
- Test hibe anahtarı (`kGrantAllModulesForTesting`) yeni kimlikleri verir.
- **Kabul:** build exit 0; oyun açılışında `modules.json` yüklenir; M01–M04 Tab ekranında görünür ve takılınca ilgili öznitelik değişir (EP, Armor, hasar, hız).

### P2 — Veri modeli ve çözümleme (Luna A, P1'den sonra)
- Dosyalar: `ModuleDefinition.h`, `ModuleLoader.cpp`, `ModuleResolver.{h,cpp}`, `modules.json` şema.
- Yap: modül kimliği başına tek takılı örnek kuralını yerleştirme doğrulamasında uygula (doğrudan sahibi ön kontrolde belirlenip izinli dosyalara eklenir); ikinci örnek reddedilir, farklı kimlikler etkilenmez. §3.1 tipleri, JSON alanları, doğrulama, `ResolvedModuleEffects` genişlemesi, örnek kural tanımları (M05–M10 için JSON iskeleti, henüz çalışma zamanı yok).
- **Kabul:** aynı modül kimliğinden ikinci yerleşim reddedilir; geçerli farklı kimlik yerleşimleri korunur. Build exit 0; geçerli ve geçersiz kural JSON'ları beklenen şekilde kabul/ret edilir; mevcut davranış aynı kalır (varsayılan yerleşimlerde kalkan/afterburner değerleri değişmez).

### P3 — Tetik çalışma zamanı ve zamanlı buff'lar (Luna B)
- Dosyalar: yeni `ModuleTriggerRuntime.{h,cpp}`, `PlayerSpaceShip.{h,cpp}`, `ShipModuleApplier.cpp`, kalkan kırılma tespiti.
- Yap: §3.3. M05, M07, M08'i canlandır.
- **Kabul (senaryo):** Offense yeteneği → 3 sn içinde atış hızı artar, tekrar kullanımda süre yenilenir, yığılmaz. Energy hasarı verilince 4 sn EP artar. Hasar alınca 2 sn hız çarpanı. Modül sökülünce buff anında kalkar. 10 tur tak/sök sonrası handle sayısı sabit.

### P4 — Giden hasar kuralları (Luna B/C)
- Dosyalar: `CombatRuntime.{h,cpp}`, `Combatant.cpp`, `DamageContext.h` (bonus bayrağı), `ModuleTriggerRuntime` (kalkan kırılma penceresi).
- Yap: §3.4. M09 (koşullu çarpan) ve M06 (ek Energy bileşeni, kalkan kırılma penceresi) canlanır.
- **Kabul (senaryo):** %25 altı hedefe hasar ×1,20, üstündeki hedefe değişmez. Kalkan kırılınca 5 sn boyunca her Primary atış ek Energy bileşeni üretir; bileşen bileşen doğurmaz; bileşen M07'yi tetikler; 5 sn sonra biter. Kritik ve mevcut hasar akışı ek kural yokken **birebir aynı**.

### P5 — Cooldown yolu ve bölge etkisi (Luna C)
- Dosyalar: `LightYearsAbilitySystemComponent.{h,cpp}`, `GameAbility.cpp`, `ModuleResolver.cpp`.
- Yap: §3.5 ve M10. Bölgeye göre etki seçimi.
- **Kabul (senaryo):** Kanat'a takılı M10 Offense yeteneklerinin cooldown'unu ×0,79 yapar, başka aileleri etkilemez. Gövde'de Primary hariç tüm yetenekler ×0,90. Burun'da ana silah atış hızı değişmez. Yeniden yerleştirme etkiyi bölgeye göre değiştirir. Çalışan cooldown geriye dönük değişmez.

### P6 — Dolu Şarjör spike'ı ve M11 (Luna D, opsiyonel)
- **Tamamlandı (`396bb78`):** atış çözümlemesi tam şarjör koşulunu destekliyor; M11 ×1,40 ilk atış hasarıyla eklendi. Mevcut `weapons.json` şarjörlerini kullanan üç oyuncu Primary silahına uygulanır; şarjörsüz içerik değiştirilmedi.

### P7 — Sunum, dokümantasyon, kapanış (Luna A + Sol)
- Üretim presenter biçimlendiricisiyle 11 Common kartın dinamik açıklaması gerçek 9 px gövde fontunda ölçülür; her satır 376 px alanına sığar.
- Common E2E gerçek PrimaryFire çözümlemesinde M03'ü, owner yatay/dikey hız değerlerinde bölge etkisiyle M04'ü doğrular.
- `MODULE_SYSTEM_DESIGN.md` ve `MODULE_SYSTEM_ARCHITECTURE.md` güncel Common kaynağına bağlanır; eski nadirlik/16 modül taslağı ve önceki adım tablosu tarihsel olarak işaretlenir.
- Bağımsız son oyun build'i exit 0; Common E2E 1/1 ve 317 kontrol geçti. Kartların en genişi 368 px; kullanılabilir satır 376 px. P7 bu belgeyle commit edilir.

### Bağımlılık özeti
```
P0 → P1 → P2 ─┬→ P3 ─→ P4
              ├→ P5          (P3/P4/P5, refactor durumuna bağlı olarak sıraya girer)
              └→ P6 (spike)  → P7
```
P3, P4, P5 aynı dosyalara kısmen dokunduğu için **seri** koşar. Okuma tabanlı denetimler (örn. P6 spike'ı, silahların `Owner.AttackSpeed`'e duyarlılığı) paralel yapılabilir.

---

## 5. Doğrulama sonucu ve sınırları

2026-10-04: final diff incelemesi ve `git diff --check` temiz. `LightYearsGame` bağımsız build exit 0; hedefli `LightYearsCommonModuleE2E` 1/1, **317 kontrol** geçti. Aynı üretim formatter'ı ve gerçek Body/9 px fontla tüm 11 açıklama 376 px alana sığar (en geniş 368 px). E2E gerçek gemi, loadout, ASC, FireOnce, projectile/impact ve cooldown yollarını çalıştırır; girdiler/sonuçlar [yerel JSON artefaktında](../build-tests/e2e-artifacts/common-modules.json) tutulur. Artefakt Git'e eklenmez; [test kaynağı](../LightYearsGame/tests/CommonModuleE2ETests.cpp) tekrarlanabilirdir.

PowerShell oyun build komutu:

```powershell
cmd /c '"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul && set VSLANG=1033 && cmake --build D:\LightYears\build --target LightYearsGame --parallel 4'
```

Ayrı `build-tests` ağacı `BUILD_TESTING=ON` ile yapılandırılmıştır; ana `build` ağacı OFF olarak korunur. Test build komutu aynı vcvars64/VSLANG hazırlığından sonra `cmake --build D:\LightYears\build-tests --target LightYearsCommonModuleE2ETests --parallel 4`; koşum `ctest --test-dir D:\LightYears\build-tests -R '^LightYearsCommonModuleE2E$' --output-on-failure`.

Tam eski suite bu kapanışta koşulmadı; bilinen eski içerik beklentileri bu kapsamda onarılmadı. Derleme ve font geometrisi oyun içi görsel kabul değildir. Manuel takip: Tab'da Common kartları/elde tutulan modülü gözle kontrol et; Offense sonrası hız, Energy hasarı sonrası EP, alınan hasar sonrası kaçış hızını dene; kalkan kırılması/ilk tam şarjör atışı ve Soğutucu'nun Kanat/Gövde/Burun taşınmasını gözle. Kullanıcıdan oyun içi kabul bu çalışma sırasında alınmadı.

---

## 6. Riskler ve kararların güncel durumu

| Konu | Kapanış durumu |
|---|---|
| Motor refactor çakışması | Başlangıç `aac98b5` üzerinde refactor tamamlanmıştı; Common fazları bunun üstünde seri uygulandı. |
| Test modu | Mevcut E2E tercihi kullanıldı; yeni unit test veya tam suite koşumu eklenmedi. Yeni kullanıcı yanıtı varsayılmadı. |
| Burun ve tek kimlik | Kullanıcının 2026-10-03 kararı: Burun atış hızı bonusu yok; kimlik başına yalnız bir takılı örnek. Envanter kopyası yasaklanmadı. |
| M10 değerleri | Açık zoneEffect çarpanları sabit; ayrıca bölge güç bonusu uygulanmaz. |
| Energy bonus zinciri | Kritiksiz, özyinelemesiz ayrı bileşen; M07 SourceDamageDealt zinciri serbest. |
| Başlangıç dengesi | JSON başlangıç sayıları korundu; daha geniş denge/loot/geliştirme ayrı kapsamdır. |
| AttackSpeed | Beş ayrık oyuncu silahının mevcut owner-rating yolu doğrulandı; Continuous Heat Laser DPS eşlemesi eklenmedi. |
| Kartlar | Aynı fontla genişlik doğrulandı; Türkçe glyph görünümü ve oyun içi his kullanıcı kabulünü bekler. |
| M11 | P6 uygulanabilirliği doğruladı ve mevcut üç şarjörlü Primary'ye uyguladı; şarjörsüz silahlara yeni kaynak sistemi eklenmedi. |

---

## 7. Sol/Luna devir protokolü

**Sol (orkestratör):** fazı sırayla başlatır, tek yazıcıyı garanti eder, her ajan çıktısını bağımsız build + diff ile doğrular, commit'i kendisi atar, doğrulanmamış hiçbir şeyi kanıt saymaz.

**Her Luna görevinin metni şunları içerir:**
1. Hedef modül(ler) ve §2 şartnamesi satırları.
2. İlgili §3 bölümü ve **izinli dosya listesi**; listede olmayan dosya değiştirilmez.
3. Hâlihazırda dirty olan, dokunulmayacak dosyalar (başka oturum işleri).
4. Proje kuralları: kompakt C++17, tipli kural struct'ları, weak_ptr/benzersiz kimlik, RAII kayıt, tek yönlü gameplay→sunum, JSON/C++ sözleşme uyumu, `ContentId`/`GameplayTag`/`AttributeId` doğru tür seçimi.
5. Kabul ölçütü ve yürütülecek komutlar.
6. Rapor biçimi: değişen dosyalar (tam liste), build sonucu, senaryo sonuçları, sapmalar, açık sorular.

**Sol'un faz sonu inceleme listesi:** handle/abonelik sızıntısı, özyineleme koruması, modülsüz yolun değişmemesi, mevcut modifier revizyon yolları ve cooldown başlangıç çarpanı, JSON/C++ uyumu, yalnız izinli dosyaların değişmesi.
