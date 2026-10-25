# Rare Modüller — Uygulama Planı

**2026-10-24 notu:** Zone Overclocker, Misfit Wiring, `zoneRules` (AmplifyZone/OutOfZoneMainBonus) ve yüzdeli bölge bonusu kaldırıldı; bölge bonusu artık seviyedir ([MODULE_SYSTEM_DESIGN.md §3](MODULE_SYSTEM_DESIGN.md#3-bölgeler-ve-aileler)). Kayıtsız `RareZoneRulesE2E`, `RareSingleModulesE2E`, `RareFragmentationE2E`, `RareWave3ModulesE2E`, `RareWave4PolishE2E` dosyaları silindi. Aşağıdaki bu konulara ait satırlar tarihli kayıttır.
**Güncel tasarım geçişi (2026-10-13):** Bu belgedeki dalga sayıları tarihli kanıttır; güncel sahip [modules.md](modules.md). İlk yeni paket ZoneCooler2,Arc merdiven/Discharge/Resonator,Lucky yan dönüşümü,Cold/Shatter kaldırma,Spare yansız,Barrier/Brace; Game build ve [212 kontrol](../build/e2e-artifacts/module-plan-transition-2026-10-13.json) geçti. Yeni ana/yan paketleri ayrı; eski ana-modül senaryoları yeni tasarımın kanıtı sayılmaz.

**Tarih:** 2026-10-10 (taslak) · **Son durum:** 2026-10-12 — Dalga 1–7 uygulandı: `modules.json` içinde 17 Rare modül ve 35 yeni Rare yan özellik var; Rare E2E'leri yeşil. F1 Parçalama (R7) ve Rare sunumu (R8) yapılmadı; Silent Running kapsam dışı. Ayrıntı §7.12. Tasarım kullanıcı kararlarıyla büyük ölçüde kapalıdır ([modules.md](modules.md), Rare bölümleri). **Tasarım kaynağı:** [modules.md](modules.md), [Uncommon uygulama planı](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md) (aynı altyapıyı kullanır), [Modül Sistemi Tasarımı](MODULE_SYSTEM_DESIGN.md).

Bu belge, [modules.md](modules.md)'de tasarlanan **18 Rare modülün**, 16 özel ve 15+6 ortak yan özelliğinin, F1 Parçalama'nın ve Rare'e özgü sistem kurallarının uygulama sözleşmesidir. Sayıların sahibi modules.md'deki kartlar, sonra JSON'lardır; bu belge ikinci bir değer listesi tutmaz.

> **Doğrulama notu:** Bu planın ilk taslağındaki varsayımlar R0 ile gerçek koda karşı sınandı (2026-10-10); sonuçlar §2.1 ve §7'dedir. Birkaç varsayım çürüdü (Fleet Guard, Momentum Shield, Spare Capacitor'ın charge yarısı).

---

## 0. İlkeler (Uncommon planından devralınır)

1. **Modül başına sınıf yazılmaz.** Modül = küçük, yeniden kullanılan kural parçalarının veri birleşimi. Yeni kod yalnız gerçekten yeni bir davranış türü için, modülden bağımsız ve değerleri JSON'dan gelen kural parçası olarak yazılır (kullanıcı, 2026-10-06).
2. **Değer farkı JSON'dur.** Rare ortak yan özellikler ayrı Rare kimlikleridir (`Shared.O1.Rare` gibi; kullanıcı, 2026-10-10), kod nadirliğe göre dallanmaz.
3. **Seviye tabloları** açık değer listesidir, Rare'de 3 seviye (L1–L3).
4. **Davranış değişmez kuralı:** modülsüz yol, Common ve Uncommon birebir aynı kalır; mevcut E2E aynen geçmeli.
5. **Doğrulama yalnız gerçek runtime E2E'dir**; her E2E girdi ve sonuçları gösteren tekrarlanabilir artefakt üretir. Unit test yazılmaz (kullanıcı tercihi).
6. **Üretilen hasar başka modül kuralını tetiklemez**; aynı anda üretilen varlıklar için teknik sınır yapılandırılır (Uncommon §3.4).
7. C++ imzaları tek satırda yazılır (`AGENTS.md`). Commit yalnız kullanıcı onayıyla, yalnız faz dosyalarıyla ve atıf satırı olmadan.

---

## 1. Kapsam

| Dahil | Hariç |
|---|---|
| 17 Rare modül (9 tek boyutlu, 8 merdivenli) | **Silent Running** (rota haritası ve Threat sistemi yok; borç) |
| 16 özel yan özellik (8 merdivenli × 2) | Gold Plating, Status Primer, Arc Discharge, Armor Conversion, Status Resonator (Uncommon adayları, ayrı plan) |
| Rare ortak yan özellikler: 15 ayrı Rare kopyası + R1–R6 | Z1/Z2 yan özellikleri (hiçbir Rare listesinde yok) |
| F1 Parçalama (7 merdivenli modülde; Trophy Core hariç) | Epic/Legendary adayları |
| Modül uyuşmazlığı (Momentum Shield ↔ Continuous Field Generator) | Loot dağılımı, yükseltme harcaması ve ekonomisi, kalıcılık |
| Rare kart metni, uyuşmazlık uyarısı, parça yerleşimi sunumu | Gold ekonomisi |

Kapsamdaki modül sayısı 18 Rare − Silent Running = **17**. Tek boyutlular: Zone Overclocker, Misfit Wiring, Stripped Hull, Odometer, Momentum Shield, Omni Thrusters, Triple Core, Continuous Field Generator, Impact Redirector (9, Silent Running bekliyor). Merdivenliler: Ember Accumulation, Shatter Shock, Kinetic Breaker, Giant Slayer, Trophy Core, Dying Star, Escape Velocity, Spare Capacitor (8; Trophy Core parçalanmaz).

---

## 2. Mevcut kural parçaları ve Rare ihtiyacı

Okunan sözleşme: `ModuleRulePackage` şu listeleri taşır: öznitelik değiştirici, yetenek değiştirici, süreli tetik buff'ı, hasar kuralı (çarpan, koşul, `activeWhileTrigger` penceresi, gelen hasar bayrağı), bölge etkisi, olay eylemi (`RestoreShield`, `ReduceLongestCooldown`, `AwardGold`…), sahip öznitelik kuralı (can eşiği, hareket hızı, Luck…), kalkan yansıması, status kuralları (Electric limiti, Cryo donma, süre bonusu, donma yayılımı), üretilen kurallar (sekme, güdümlü mermi, gecikmeli patlama, mayın, ölüm patlaması). Tetikler: yetenek kullanıldı, kalkan kırıldı, hasar alındı, kaynak hasar verdi, öldürme, kritik. Hasar koşulları: yalnız Primary, hedef can ≤, tam şarjör, sahip can ≤, hedef statüsüz, hedef Thermal+Cryo, kaynak statüsü, komşu yetenek doğrudan vuruşu, ilk hedef vuruşu.

### 2.1 R0 sonrası: mevcut parçayla yazılabilenler ve yazılamayanlar (2026-10-10 doğrulandı, ayrıntı §7)

| Özellik | Sonuç (R0) | Not (R0) | Uygulama (2026-10-12) |
|---|---|---|---|
| R1 Guarded Strike | **Yazılabilir** | Giden hasar kuralı: `PrimaryShotOnly` + `activeWhileTrigger` = AbilityActivated, etiket `Ability.Defense`, 4 sn (`ModuleLoader.cpp:393-410`, `CombatRuntime.cpp:355-370`) | **Uygulandı:** `Shared.R1` (dalga 2, `RareSideFeaturesE2E`); canlı mermide çalıştığı dalga 4 probuyla doğrulandı (`RareSustainedSpeedE2E`) |
| R2 Reactive Plating | **Yazılabilir** | `ShieldBroken` triggerBuff + `Owner.Armor`; süre yenilenir, birikmez; modül kaynaklı kalkan kırılması tetiklemez. JSON'da örnek yok | **Uygulandı:** `Shared.R2` (dalga 2, `RareSideFeaturesE2E`), Shatter Shock havuzunda |
| R6 Fleet Guard | **Yazılamaz** | Gelen hasar kuralında `activeWhileTrigger` yükleyicide yasak (`ModuleLoader.cpp:413-414`) ve gelen yol pencere okumaz (`CombatRuntime.cpp:478`) | **Uygulandı:** gelen hasar penceresi (dalga 1, `RareDamageConditionsE2E`), `Shared.R6` (dalga 2, `RareSideFeaturesE2E`: 3 sn, ×0,85) |
| R3 Boss Hunter | **Yeni koşul + veri** | Bağlamda hedef sınıfı yok; ayrıca hiçbir düşmana sınıf atanmıyor (§7.2) | **Kısmen:** koşul `TargetIsBossClass` (dalga 1), `Shared.R3` (dalga 2). Oyunda boss sınıflı düşman yok; fiilen işlemez (§7.12 açık kararlar) |
| R4 Full Charge | **Yeni koşul** | Bağlamda kalkan referansı yok | **Uygulandı:** `OwnerShieldFull` (dalga 1), `Shared.R4` (dalga 2) |
| R5 Status Weaver | **Yeni koşul** | Bağlamda yalnız `targetHasStatus` ve `targetHasThermalAndCryo` bool'ları var | **Uygulandı:** `TargetStatusKindsAtLeast` (dalga 1), `Shared.R5` (dalga 2) |
| Koşulsuz çarpan (Stripped Hull, Odometer) | **Yeni koşul** | Enum'da koşulsuz değer yok, `condition` zorunlu (`ModuleLoader.cpp:327`) | **Uygulandı:** Stripped Hull `Always` + `perEmptyCellMultiplier` (dalga 3); Odometer `runCounterRules` (dalga 3) |
| 15 ortak Rare kopyası | Yazılabilir | Yalnız değer farkı | **Uygulandı:** `Shared.O1–O5.Rare`, `D1–D5.Rare`, `G1–G5.Rare` (dalga 2, `RareSideFeaturesE2E`) |
| Zırhı tamamen yok sayma (Kinetic Breaker) | **Yeni alan** | `armorPenetration=1` nihai çarpanı 1 yapar, ama yalnız Kinetic etiketli hasarda okunur (`DamageTypeSystem.cpp:282-284`) ve modül hasar kuralında alan yok | **Uygulandı:** `statusRules.kineticConsumes` (dalga 4, `RareStatusRulesE2E`) |
| Hedefe süreli Armor −%25 (Armor Rend) | Yazılabilir | Hedef efekt spec'i + modifier (`MakeMovementSlowSpec` kalıbı) | **Uygulandı:** `Module.SideFeature.KineticBreaker.ArmorRend`, yeni efekt `Effect.Status.Damage.Kinetic.ArmorRend` (dalga 4 çekirdeği, dalga 5-7 yan özelliği) |
| Primary mermi hız/delme (Escape Velocity) | Yazılabilir | `PrimaryWeapon` hedefli triggerBuff (Piercing Tip kalıbı); `Delivery.Speed` yalnız 5 silahta tanımlı, `PierceDamageLoss` okunmuyor | **Uygulandı:** `sustainedSpeedRules` (sonik durum, dalga 4, `RareSustainedSpeedE2E`; canlı atış 26 → 29) |
| İtme | API var | `MovementInfluenceService::ApplyImpulse`; modül çalışma zamanından çağrılmıyor | **Uygulandı:** `ownerBurstRules.pushImpulse` (dalga 4, Shatter Shock, `RareOwnerBurstE2E`); birim sorunu açık (§7.12) |
| Momentum Shield | **Yazılamaz** | `MovementSpeed` kaynağı Ship/Multiply'a kapalı (`ModuleLoader.cpp:507-512`); regen'e hız girmiyor | **Uygulandı:** `shieldRules` `SpeedScaled` (dalga 2, `RareSingleModulesE2E`, `RareShieldRulesE2E`) |
| Continuous Field Generator | Kısmen | Gecikme sıfırlama ve kalkan regen bloğu ShieldComponent'te var, modül yolu yok | **Uygulandı:** `shieldRules` `Continuous` + `breakRearmFraction` 0,25 (dalga 1-2) |
| Impact Redirector | **Yeni adım** | Armor kalkandan SONRA, yalnız kalan hasara uygulanıyor (`SpaceShip.cpp:385-397`) | **Uygulandı:** `shieldRules.armorShare` (dalga 1-2, `RareShieldRulesE2E`) |
| Omni Thrusters | **Yeni modül hedefi** | `thrustBonus` üç yönü birlikte çarpar; `SetDirectionalThrustSync` (geri/yan = ileri) var ama yalnız VectorSync ability'sine bağlı | **Uygulandı:** `movementRules` (dalga 3, `RareMovementRulesE2E`) |
| Spare Capacitor | **Büyük yeni iş** | §7.3 | **Uygulandı:** `chargeRules`, çok charge modeli, GrantCharge (R-Ön charge geri yükleme; dalga 5-7 `RareSpareCapacitorMechanicsE2E`). Overflow Transfer uygulanmadı (§7.12) |
| Trickle Charge, periyodik tetik | **Yeni** | Periyodik tetik türü ve charge ekleme API'si yok | **Uygulandı:** `Module.SideFeature.SpareCapacitor.TrickleCharge` (dalga 5-7, `RareSpareCapacitorMechanicsE2E`) |

### 2.2 Yeni, yeniden kullanılabilir kural parçaları

| Parça | İhtiyaç duyanlar |
|---|---|
| **Yeni hasar koşulları:** hedef boss/miniboss mu; sahip kalkanı dolu mu; hedefte ≥2 farklı statü; koşulsuz tüm hasar; hedef MaxHealth oranı ≥ eşik (oranla değişen çarpan); Q/E/F/R yeteneğinin doğrudan vuruşu; tam bir yetenek slotu boş mu | R3 Boss Hunter · R4 Full Charge · R5 Status Weaver · Stripped Hull · Giant Slayer · Triple Core |
| **Çözümleyici bölge kuralları:** bölgedeki diğer modüllerin bölge bonusunu yükselt; kendi ailesi dışındaki modüle ana değer bonusu; boş hücre sayısından çarpan | Zone Overclocker · Misfit Wiring · Stripped Hull |
| **Kalkan yenileme modu:** normal yenilenmeyi kapat, yenileme hızını hıza bağla veya sürekli sabit oran uygula, gecikmeyi kaldır; kalkan-kırılma tetiğini kalkan %25 dolunca yeniden kur | Momentum Shield · Continuous Field Generator |
| **Modül uyuşmazlığı:** tanımda çatışan modül kimlikleri; yerleştirmede reddet veya uyar, ekran iki modülü işaretler | Momentum Shield ↔ Continuous Field Generator |
| **Koşu sayacı:** modül örneğinde saklı, koşu boyunca kalıcı sayaç (söküp takınca korunur); mesafe ve öldürme kaynakları | Odometer · Trophy Core (+ Bounty Mark, Trophy Wall) · Dying Star (kalıcı değil, şarj sayacı) |
| **Hareket kuralı:** ileri/geri/yan itkiyi eşitle; sürekli yüksek hızda sayaç ve "sonik" durumu (Primary mermi hızı ×, delme) | Omni Thrusters · Escape Velocity |
| **Status eşiği patlaması:** hedefte stack eşiğinde patla ve stack'leri sil; stack tüket ve sonraki vuruşta zırhı yok say; hedefte süreli zırh azaltma | Ember Accumulation · Kinetic Breaker (+ Hair Trigger, Armor Rend) |
| **Sahip merkezli patlama (OwnerBurst):** sahip etrafında yarıçap, sahip MaxHealth oranı, itme, ikinci gecikmeli dalga, çekme; boss/elit muafiyeti | Shatter Shock (+ Stagger, Aftershock Wave) · Dying Star (+ Core Collapse) |
| **Alan etkisi:** süreli kor alanı, saniyede bir stack | Ember (Smoldering Remains) |
| **Yetenek charge kuralı:** komşu yeteneğe seviye başına charge, taşma aktarımı, periyodik charge | Spare Capacitor (+ Overflow Transfer, Trickle Charge) |
| **F1 Parçalama** (yerleştirme altyapısı, §4 R7) | 7 merdivenli modül |

Aynı parçalar ana ve yan özellikte kullanılabilir. Üretilen hasar, patlama ve sekme başka modülü tetiklemez.

---

## 3. Mimari kararlar

- **Veri modeli:** `ModuleRarity::Rare` zaten var. Rare ortak yan özellikler `module_side_features.json`'da ayrı kimliklerle (`Shared.O1.Rare` …) eklenir; Rare modüllerin `allowedFeatureIds` listesi bu kimlikleri gösterir. Yükleyici bilinmeyen kimlik, eksik seviye girdisi ve havuzda olmayan özelliği reddetmeye devam eder.
- **Sayaç ve durum:** koşu sayacı ve kalıcı sayaç, modül örneğiyle birlikte saklanır; sökme/takma sayacı silmez. Yeni koşu sıfırlar. Kalıcılık sistemi bu planın kapsamı dışındadır.
- **Uyuşmazlık:** kural tanımda bildirilir (örn. `conflictsWith`); `ModulePlacementRules` yeni modülü reddeder veya uyarı sonucu döndürür; modül ekranı iki modülü işaretler. Yalnız ters sinerji (Ember ↔ Thermal Shock Coil) bu kurala girmez.
- **Parçalama (F1):** bir modül örneği en büyük boyutta birden çok 1 hücrelik yerleşime bölünebilir; yan yana parçalar bağlı sayılır; bölge bonusu orantılı; komşuluk her parçadan sayılır, ek sınır yok. Etkilenen yerler: yerleşim verisi, ızgara kontrolü, kayıt alanları, çözümleyici, sunum. Trophy Core parçalanmaz (F1 listesinde yok); Spare Capacitor'da bir yetenek bu modülden en çok seviyesinin charge'ını alır, parçalar yığılmaz.
- **Boyutla değer:** Rare'de ana özellik her boyutta aynıdır (Uncommon gibi); istisnalar Spare Capacitor (yetenek sayısı) ve Shatter Shock (boyutla hasar).

---

## 4. Fazlar

**Çalışma kuralları:** Uncommon planı §4 geçerli. Aynı anda tek yazıcı; her faz kendi E2E'siyle kapanır, commit yalnız kullanıcı onayıyla. Derleme: `vcvars64` sonrası `VSLANG=1033`, `cmake --build D:\LightYears\build --target …`; ortak `build` klasörü başka oturumlarca da derlenebildiği için sonuçlar yazılırken araç zaman damgası kontrol edilir.

### R0 — Ön kontrol · **Tamamlandı (2026-10-10)** (§7)
- Çalışma ağacı durumu (kirli dosyalar, başka oturumların `ships.json` ve doğrulayıcı değişiklikleri), Common ve Uncommon E2E başlangıç sonuçları.
- §2.1 varsayımlarını gerçek koda karşı doğrula: `activeWhileTrigger` penceresinin gelen/giden hasarda çalışması, etiket eşleşmesi (Defense/Movement), kalkan kırıldı tetiğinin buff taşıması, yetenek charge kapasitesinin öznitelik olarak değiştirilebilirliği, itme (knockback) ve stun yollarının modül kuralından erişilebilirliği, `ArmorPen` yolu, ShieldComponent yenileme/gecikme kancaları.
- Çıktı: bu planın §2'sine doğrulanmış/yanlış işaretleri ve gerekirse faz değişiklikleri.

### R1 — Ortak yan özellikler ve hasar koşulları · **Uygulandı (dalga 1–2; §7.8, §7.9)**
- 15 ayrı Rare kopyası; R1, R2, R6 (mevcut parçalar); yeni koşullar R3 (boss), R4 (kalkan dolu), R5 (≥2 farklı statü).
- Ortak yan özellikler yalnız merdivenli bir modülün havuzuna bağlanınca gerçek yoldan sınanabilir; merdivenli Rare modüller R5–R7'de gelir. Bu yüzden R0, test fixture'ının Rare ortak kopyalarını havuz dışı kurup kurabildiğini doğrular. Kurabiliyorsa R1 kendi başına E2E ile kapanır; kuramıyorsa R1'in kural çalışması R5'in ilk modülüyle birlikte teslim edilir ve R1 yalnız veri ve yükleyici doğrulamasıyla kapanır.
- **Kabul:** Rare kopyaları Uncommon değerlerinden farklı sonuç verir; koşullar vuruş öncesi durumla çalışır; Common/Uncommon sonuçları değişmez.

### R2 — Çözümleyici kuralları · **Uygulandı (dalga 1–4; §7.8, §7.10, §7.11)**
- **Modüller:** Zone Overclocker, Misfit Wiring, Stripped Hull, Triple Core, Impact Redirector.
- **Kabul:** bölge bonusu yükseltme yalnız aynı bölgedeki diğer modüllere; Misfit yalnız ailesi dışındaki bölgede; Stripped Hull boş hücre sayısıyla orantılı (hücre sayısı 28 varsayımı); Triple Core tam bir slot boşken çalışır, iki boşta çalışmaz; Impact Redirector yalnız kalkan > 0 iken.

### R3 — Kalkan yenileme ve uyuşmazlık · **Uygulandı (dalga 1–2; §7.8, §7.9)**
- **Modüller:** Momentum Shield, Continuous Field Generator; uyuşmazlık kuralı ve placement sonucu.
- **Kabul:** kalkan yenileme modları gerçek ShieldComponent yolunda; yeniden kurulum eşiği Max Shield'in %25'i; ikisi aynı anda takılamaz ve uyarı iki modülü işaretler; mevcut kalkan tetikleri (Shatter Current vb.) değişmez.

### R4 — Hareket ve koşu sayacı · **Uygulandı (dalga 3–4; §7.10, §7.11)**
- **Modüller:** Omni Thrusters, Escape Velocity (+ Afterglow, Sonic Boom), Odometer.
- **Kabul:** itki eşitleme gerçek MovementComponent'te; sonik durum 2 sn kesintisiz >%90 hızdan sonra, yavaşlayınca biter; Odometer sayacı söküp takınca korunur, yeni koşuda sıfırlanır.

### R5 — Status ve hasar · **Uygulandı (dalga 4–7; §7.11, §7.12)**
- **Modüller:** Kinetic Breaker (+ Hair Trigger, Armor Rend), Ember Accumulation (+ Area Blast, Smoldering Remains), Giant Slayer (+ Toppler, Underdog).
- **Kabul:** Ember patlaması stack'leri siler ve Thermal Shock Coil koşulunu bozar (kabul edilmiş çatışma); Kinetic Breaker zırhı yok sayan vuruşu stack tüketerek tetikler; Giant Slayer çarpanı hedef/sahip MaxHealth oranına göre.

### R6 — Sahip merkezli patlama, sayaçlı ve charge modülleri · **Uygulandı; Overflow Transfer açık (dalga 4–7; §7.11, §7.12)**
- **Modüller:** Shatter Shock (+ Stagger, Aftershock Wave), Dying Star (+ Core Collapse, Red Giant), Trophy Core (+ Bounty Mark, Trophy Wall), Spare Capacitor (+ Overflow Transfer, Trickle Charge).
- **Kabul:** üretilen hasar başka modülü tetiklemez; elit/boss muafiyetleri; Trophy Core öldürmeleri komşu yeteneğe atfeder, sökülünce stack'ler korunur ve yeniden takılınca etkin olur (öneri); Spare Capacitor seviye başına +1 charge, parça yığmaz.

### R7 — F1 Parçalama · **Çekirdek tamam (2026-10-12; sunum R8'de)**

> Veri modeli (`ModulePlacement.fragmentCells`), yerleşim kuralları, çözümleyici bölge payı, `Player::TryInstallModuleFragments`/`TryMoveModuleFragments`, F1 ortak yan özelliği (`Module.SideFeature.Shared.F1.Rare`, yedi merdivenli modülün havuzunda, Trophy Core'da yok) ve `RareFragmentationE2E` yeşil. Parçalı yerleşim için arayüz akışı yok; sunum `placement.origin/rotation` yerine `GetCoveredCells` kullanmalı. Belirsiz yorumlar: parçaların bağlı olması zorunlu değil, parçalı Spare Capacitor'da komşuluk sınırı dört kabuk, Spare Capacitor +1 çelişkisi kullanıcı kararıyla kapandı (2026-10-12).
- Yerleşim, ızgara, kayıt alanları, çözümleyici ve orantılı bölge bonusu; komşuluk her parçadan.
- **Modüller:** Ember, Shatter Shock, Kinetic Breaker, Giant Slayer, Dying Star, Escape Velocity, Spare Capacitor.
- **Kabul:** parçalar yan yana otomatik bağlanır; bölge bonusu orantılı; Spare Capacitor'da yığma yok; parçalanmayan modüller etkilenmez. En büyük ve riskli faz; son sıraya konur.

### R8 — Sunum ve kapanış · **Yapılmadı (2026-10-12)**
- Rare kart metni (yeni token'lar), seviye ve yan özellik gösterimi, uyuşmazlık uyarısı, parça yerleşimi arayüzü; modules.md statüleri.
- **Görsel kabul kullanıcıdadır.**

### Bağımlılık
```
R0 → R1 → R2 → R3 → R4 → R5 → R6 → R7 → R8
```
R2–R6 farklı dosyalara dokunur ama aynı çözümleyiciyi paylaştığı için seri koşar. R7 diğer fazlardan bağımsız tasarlanabilir ama aynı dosyalara dokunduğu için sona kalır.

---

## 5. Test stratejisi

- Gerçek ASC, loadout, kalkan, hareket ve hasar yolları kullanılır; mock davranışı kanıt sayılmaz.
- Her kural parçası için: tetiklenir, tetiklenmez, süre/yenileme, sökünce temizlik, 10 tur tak/sök sonrası handle sabit.
- Her merdivenli modül için: küçük boyutta yan özellik yok, büyük boyutta var, farklı yan özellikle farklı sonuç, seviye tablosu.
- Tek boyutlu modüllerde yan özellik yoktur; yükleyici bunu reddeder.
- Uyuşmazlık, parçalama ve sayaç korunumu için ayrı senaryolar.

---

## 6. Riskler ve açık kararlar

| Konu | Not |
|---|---|
| Silent Running | Rota haritası ve Threat sistemi yok; modül bu planda uygulanmaz |
| Gold Plating | Uncommon adayı; Gold harcaması/ekonomisi yok |
| Spare Capacitor | Yetenek charge kapasitesinin öznitelik olarak değiştirilebilirliği doğrulanmadı (R0). Seviye başına +1 charge (L3'te üç yeteneğe +3) bu sistemin en güçlü kaynak kuralıdır, dengede ilk izlenecek yer. *[2026-10-12: charge modeli uygulandı (§7.12); charge sayısı kuralı kullanıcı kararıyla kapandı: L1 +1, L2 +2, L3 +3, parçalar yığılmaz.]* |
| Stripped Hull | Boş hücre sayısı 28 (kullanıcı); L3'te +%98. Gemi şeması değişirse güç değişir |
| Ember + Dying Star + Thermal Shock Coil | Aynı Thermal ekseninde zincir ve yeniden tetiklenme riski; kullanıcı çatışmayı bilerek kabul etti, ölçüm gerekir |
| F1 Parçalama | Veri/ızgara/kayıt değişikliği maliyeti kabul edildi; en büyük faz; Trophy Core ve Dying Star etkileşimleri sınanmalı |
| Shatter Shock | MaxHealth oyuncunun; 1 hücre %10, 3 hücre %25 başlangıç oranları Claude önerisi |
| Kinetic Breaker | ×1,10 → 1,20 çarpan özgün kart metninde yoktu, onaylı yeni parça |
| Giant Slayer | "+0,05 / seviye" taban çarpana uygulandı (Claude yorumu) |
| Rare ortak kimlik kopyaları | 15 kopya; değer değişince iki yer güncellenir. Sorun olursa nadirlik başına tabloya geçilir |
| Ortak `build` klasörü | Başka oturumlar aynı klasöre derleyebilir; test sonuçları zaman damgasıyla doğrulanır |
| Rare ana değerleri | Mutlak hasar sayıları (Dying Star, Shatter Shock yüzdeleri) denge testiyle belirlenir |

---

## 7. R0 sonuçları (2026-10-10)

**Yöntem:** Beş salt-okunur Haiku 5.5 ajanı (xhigh) kodu okudu. Ajan raporları güvenilmez sayıldı; kritik iddialar ayrıca doğrudan kodda doğrulandı: gelen hasar kuralında `activeWhile` yasağı, `SetControlTargetClass`'ın yalnız testte çağrılması, `maxCharges`'ın düz `int` olması ve 55 yeteneğin tamamında 1 olması, komşuluk sınırının varsayılan 2 olması, Armor'un kalkandan sonra uygulanması, `ShieldBroken`'ın latch'siz olması, `MovementSpeed` kaynağının Ship/Multiply'a kapalı olması, `ReduceCooldown`'ın charge geri vermemesi. Çalışma zamanında denenmedi; ek olarak derleme/test yapılmadı.

### 7.1 Doğrulanan ve çürüyen varsayımlar
§2.1 tablosu. Özet: yazılabilir olanlar R1, R2, 15 ortak kopya, Armor Rend, Primary mermi hız/delme; yazılamayanlar Fleet Guard, Momentum Shield, Spare Capacitor'ın charge yarısı; kalanı yeni koşul veya yeni hedef ister.

### 7.2 Düşman sınıfı verisi yok
`SpaceShip::GetControlTargetClass()` var ve kullanılıyor (`SpaceShip.cpp:286-303`: Elit stun ×0,6, MiniBoss ×0,3, Boss yalnız kesme), ama hiçbir ship JSON'u `controlTargetClass` taşımıyor ve kod onu okumuyor; atama yalnız testte (`UncommonModuleP5E2E_Status.cpp:119-122`). Oyunda her düşman Normal. Etkilenenler: R3 Boss Hunter, Stagger "elit/boss muaf" (Elit zaten tam muaf değil, ×0,6), Core Collapse boss muafiyeti, Trophy Core elit/boss öldürme değerleri, Dying Star elit/boss şarjı (yok), Frost Lock'un mevcut `normalTargetsOnly` kontrolü fiilen hep geçiyor. **Önkoşul:** düşman sınıfını JSON'dan okuyup atayan küçük bir veri/yükleyici işi.

### 7.3 Spare Capacitor charge mimarisi
- Charge kapasitesi öznitelik değil, `AbilityDefinition::maxCharges` düz `int` (`AbilityDefinition.h:22`); `ScopedAbilityRule` yalnız aksiyon değerlerine uygular, kapasiteye ulaşmaz.
- 55 yeteneğin hepsi `maxCharges = 1`. "+N charge" bu yeteneklere ikinci, üçüncü bir kullanım eklemek demek: kapasite çalışma zamanında değişebilir olmalı ve fazla charge'ın nasıl yenileneceği tanımlanmalı.
- Charge yalnız cooldown 0'a *tikleyerek* inerken veya aktivasyon bitişinde cooldown ≤ 0 iken geri gelir (`AbilityRuntimeState.cpp:100-133`). Charge ekleme API'si ve kazanım olayı yok.
- Komşuluk çözümü varsayılan olarak en çok 2 komşu verir, fazlaysa isme göre alfabetik keser (`ModulePlacementRules.cpp:43-59`). 3 yetenek için sınır kalkmalı ve sıralama geometrik olmalı. Bay tek satır ve kabuklar bitişik olduğu için bir hücre pratikte en çok 1 kabuğa değer; 3 hücrelik bir hat 3 kabuğa değer.
- **Çok charge bugün anlamsız (kod okumasıyla, denenmedi):** `EndActivation` her aktivasyondan sonra cooldown'u charge sayısına bakmadan başlatır (`GameplayAbilityInstance.h:516-525`), `CanActivate` cooldown bitmeden izin vermez, cooldown bitince charge'lar **tam kapasiteye** döner (`AbilityRuntimeState.cpp:128-131`). Yani 3 charge'lı bir yetenek yine kullanımlar arasında cooldown bekler ve her seferinde tam doluya döner; fazladan charge hiçbir şey kazandırmaz. İçerikte `maxCharges > 1` olan yetenek yoktur (yalnız bir test fixture'ı, `AuditFixesE2E.cpp:789`). "Charge" bu projede gerçek bir mekanik olarak hiç çalışmıyor.
- **Karar (kullanıcı, 2026-10-10):** iş büyük olsa da yapılacak; Spare Capacitor charge modelini genişletir. Bu iş `SpaceAbilitySystem` kütüphanesine dokunur ve **önce charge'ın anlamının tanımlanmasını** gerektirir (§7.7). Tek başına faz: R6b.

### 7.8 Dalga 1 uygulama durumu (2026-10-10)

Kullanıcı R1/R2/R3'ün aynı anda yürütülmesini istedi. Paylaşılan dosyalar (`ModuleLoader.cpp`, `ModuleDefinition.h`, `ModuleResolver.cpp`, uygulayıcılar), ortak `build` klasörü ve canlı başka oturum yüzünden tam paralellik güvenli değildi; dosya sahipliği ayrık dört yazıcı (Haiku, high) paralel çalıştı, derleme ve test yöneticide kaldı. Ajanların eşzamanlı düzenlemesi `ModuleDefinition.h:131`'de bozuk bir satır üretti (düzeltildi); bunun dışında tüm proje ilk tam derlemede derlendi.

| Mekanizma | Plan fazı | Durum |
|---|---|---|
| Hasar koşulları `Always`, `OwnerShieldFull`, `TargetIsBossClass`, `TargetStatusKindsAtLeast`; gelen hasar penceresi | R1 | Yeşil: `RareDamageConditionsE2E` 66 kontrol |
| Kalkan `shieldRules` (regenMode Normal/SpeedScaled/Continuous, `breakRearmFraction`, `armorShare`) | R3 | Yeşil: `RareShieldRulesE2E` |
| Uyuşmazlık `conflictsWith`, `ModuleConflict`, çatışan modül işareti için view model verisi | R3 | Yeşil: `ModuleConflictE2E` |
| Bölge kuralları `zoneRules` (AmplifyZone, OutOfZoneMainBonus), `CountEmptyCells` | R2 | Yeşil: `RareZoneRulesE2E` 177 kontrol (tüm oyuncu gemisi şemalarını tarar) |

**Doğrulama (2026-10-10):** tam `ctest` 54 testin 50'si geçti. Düşen dört test dalgadan önce de düşenlerdir (PrimaryWeapon, PlayerShipSelection, WeaponPhase2, ContinuousBeamWallE2E). Yeni artefaktlar: RareDamageConditions 66, RareShieldRules 79, ModuleConflict 44, RareZoneRules 177, ControlTargetClassLoad 21, AbilityChargeRestore 37 kontrol, hepsi `passed=true`. Mevcut Common/Uncommon/ModuleScreen testleri değişmeden geçti.

Kararlar ve açıklar: (1) yeni hasar koşulları Fire fazında değil Impact fazında değerlendirilir (Fire yalnız Fire-faz kuralları işler). (2) Birden çok regenMode kuralı varsa Continuous > SpeedScaled > Normal; `breakRearmFraction` ve `armorShare` tüm kurallar arasında max. (3) `armorShare` kalkan hasar çarpanını `f = 1 − share × (1 − armorÇarpanı)` ile ölçekler; kalkan yansıması (`ModuleShieldReflectionRuntime`) `context.shieldDamage` kullandığı için armorShare altında yansıma hasarı f kadar düşer, tasarım seçimi onay bekler. (4) `armorShare` hesabı `CombatRuntime::ApplyHullDamageMitigation` formülünü kopyalıyor; ortak yardımcıya çıkarılmalı. (5) AmplifyZone Hull gibi farklı taban bonuslu bölgeleri `bonus / 0,25` ile ölçekler (Hull 0,10 → 0,20); kullanıcı "Gövdedeki etkisi açık" demişti, onay bekler. (6) Yan özellik paketine yazılan `shieldRules`/`zoneRules` `ModuleSideFeatureLoader`'da sessizce yok sayılır veya reddedilmez, bu yüzden yan özellikler bu iki kuralı henüz taşıyamaz. (7) `GetZoneBonus` imzası değişmedi, bu yüzden modül ekranı AmplifyZone etkisini göstermez; sunum R8'de. (8) Momentum Shield ve Continuous Field Generator modül tanımları henüz yazılmadı; mekanizmalar test fixture'larıyla sınandı, gerçek `modules.json`'a Rare modül eklenmedi.

### 7.9 Dalga 2 uygulama durumu (2026-10-10)

Gerçek içeriğe ilk Rare girdiler eklendi. İki yazıcı (Haiku, high) ayrık dosyalarda çalıştı.

- **Beş tek boyutlu Rare modül (`modules.json`, yalnız sona ekleme):** Zone Overclocker (`zoneRules` AmplifyZone 0,50/0,55/0,60), Misfit Wiring (OutOfZoneMainBonus 0,25/0,30/0,35, 1 hücre), Momentum Shield (`shieldRules` SpeedScaled 1,5/1,7/1,9), Continuous Field Generator (Continuous 0,35/0,45/0,55, `breakRearmFraction` 0,25), Impact Redirector (`armorShare` 0,5/0,6/0,7). Momentum Shield ↔ Continuous Field Generator `conflictsWith` ile bildirildi. `ModuleDescriptionValues` yeni kural türleri için `zone.*` ve `shield.*` anahtarlarını aldı. E2E: `RareSingleModulesE2E` 414 kontrol (gerçek `modules.json`, fixture yok).
- **Ortak yan özellikler (`module_side_features.json`, yalnız ekleme):** 15 ayrı Rare kopyası (`Shared.O1.Rare` …) ve R1–R6 (`Shared.R1`…`R6`); Uncommon havuzlarına dokunmamak için AYRI havuzlar (`RareOffense`, `RareDefense`, `RareGeneral`, `RareSpecial`). Yan özellik yükleyicisi `shieldRules` taşıyabilir; `zoneRules` taşıyan yan özellik açıkça REDDEDİLİR (çözümleyici yalnız ana varyantın bölge kurallarını okuyor; yan özellikte bölge kuralı şu an gerekli değil). `UncommonModuleCompletionE2E_Content.cpp` yan özellik sayısı beklentisi 37 → 58 güncellendi. E2E: `RareSideFeaturesE2E` 214 kontrol.
- **Doğrulama:** tam `ctest` 56 testte 52 yeşil; düşen dört test dalgadan önce de düşenlerdir. Artefaktlar: RareSingleModules 414, RareSideFeatures 214, RareZoneRules 177, RareShieldRules 79, RareDamageConditions 66, ModuleConflict 44, ChargeRestore 37, ControlTargetClassLoad 21, hepsi `passed=true`. `LightYearsGame.exe` kullanıcının açık oyun penceresi yüzünden bağlanamadı (LNK1168), test ikilileri etkilenmedi.
- **Öngörülen ama bu dalgada çıkmayan etki:** `PlayerShipSelectionTests.cpp:423` `grantable.size() == 15` beklentisi yeni beş Rare modülle 20 olmalı; test zaten dalga öncesinden düşüyordu (şema geçişi), düzeltilmedi. `ModuleSystemTests.cpp` bu build'de derlenmiyor ve beş yeni kimliği 'covered' listesinde bilmiyor.
- **Henüz yapılmayanlar:** Triple Core (koşul: tam bir yetenek slotu boş), Stripped Hull (boş hücre sayısından hasar çarpanı), Omni Thrusters (itki hedefi), Odometer (koşu sayacı); sekiz merdivenli Rare modül; F1 Parçalama; Rare sunumu.

### 7.10 Dalga 3 uygulama durumu (2026-10-10)

Dört kalan tek boyutlu Rare'in mekanizmaları üç ayrık yazıcıyla (Haiku, high) yazıldı; `modules.json`'a modül henüz eklenmedi (mekanizmalar önce fixture E2E'siyle doğrulanır, modüller sonra tek yazıcıyla eklenir).

| Mekanizma | Modül | Durum |
|---|---|---|
| Hareket kuralı `movementRules` (`thrustMultiplier`, `equalizeDirections`; MovementComponent'te modül kaynaklı itki bonusu ve yön eşitleme, VectorSync'ten ayrı) | Omni Thrusters | Yeşil: `RareMovementRulesE2E` 150 kontrol (Fighter 650/190/270 → eşit, L2/L3, sökünce birebir geri, 10 tur handle sabit) |
| Hasar kuralı alanları `perEmptyCellMultiplier` (boş hücre sayısından çarpan, çözümleyicide), `requiredEmptyAbilitySlots` (tam N kabuk boşken), yeni koşul `AbilityDirectHit` | Stripped Hull, Triple Core | `RareStaticDamageRulesE2E` fixture düzeltmesi sürüyor |
| Koşu sayacı `runCounterRules` (`ModulePlacement::runCounter`, mesafe `|hız|×dt`, stack = floor(sayaç/5000), hasar çarpanı `CombatRuntime::SetRuntimeModifier`, hız `ShipRuntimeModifiers`, sök/tak/Move korur, `ResetRunProgression` sıfırlar) | Odometer (ve ileride Trophy Core) | `RareRunCounterE2E` fixture düzeltmesi sürüyor |

**Güncelleme (aynı gün):** üç mekanizmanın fixture E2E'leri yeşil: `RareStaticDamageRulesE2E` 442, `RareMovementRulesE2E` 150, `RareRunCounterE2E` 74 kontrol (ilk koşuda ikisi fixture modül kimliğinin yükleyici şemasına uymaması yüzünden düşmüştü, test hatası). Dört modül gerçek `modules.json`'a eklendi: Triple Core (`AbilityDirectHit`, `requiredEmptyAbilitySlots` 1, çarpan 1,25/1,30/1,35, 2 hücre), Stripped Hull (`Always`, `perEmptyCellMultiplier` 0,025/0,03/0,035, 1 hücre), Omni Thrusters (`thrustMultiplier` 0,70/0,715/0,73, `equalizeDirections`, 2 hücre), Odometer (`runCounterRules` Distance 5000, 1/1,2/1,4%, 1 hücre); `ModuleDescriptionValues`'a `movement.*` ve `runCounter.*` anahtarları eklendi. Gerçek içerik E2E'si `RareWave3ModulesE2E` 544 kontrol yeşil (ilk düşüş testteki bayat modül kimliğiydi: yeniden takınca instanceId değişir; ikincisi Odometer vuruş oranında hedef kalkanının yenilenmesiydi). **Bağımsız doğrulama:** tam `cmake --build` exit 0, tam `ctest` 60 testte 56 yeşil; düşen dört test dalgalardan önce de düşenlerdir. On iki yeni artefakt toplam 2.262 kontrol, hepsi `passed=true`. **Rare tek boyutlu modüller tamam:** 9 modül oyunda (Silent Running hariç).

Kararlar ve açıklar: (1) bir kabuk modülü yeniden başlatılırken (reboot) aktif yerleşimde sayılmaz; o süre boş sayılır, Triple Core kısa süreliğine açılır, Stripped Hull o hücreyi boş sayar. Tasarım kararı gerekir. (2) `perEmptyCellMultiplier` ile `damageMultiplier` birlikte verilemez (yükleyici reddeder, eklenen kısıt). (3) Odometer hasarı `ModuleGeneratedDamage` (sekme, mayın, ölüm patlaması) kaynaklı hasara uygulanmaz; "tüm hasar" tam değil. (4) Mesafe hıza bağlı: zorla hareket ve movement burst eksik sayılabilir, ışınlanma sayılmaz. (5) Sayaç yalnız oyuncu gemisinde. (6) Omni Thrusters birden çok kuralda en düşük çarpanı seçer, `equalizeDirections` herhangi biri true ise true. (7) `ModuleSideFeatureLoader`'daki boş kural kontrolü `movementRules`/`runCounterRules`'ı saymıyor; yan özelliklerde bu kurallar henüz gerekmiyor. (8) `ModuleDescriptionValues` hareket ve sayaç anahtarlarını henüz tanımlamıyor.

### 7.11 Dalga 4 uygulama durumu (2026-10-11)

Kullanıcı kararları: reboot'taki kabuk dolu sayılır; Odometer hasarı üretilen hasara da uygulanır; Odometer ve Impact Redirector sınırlamaları ile Zone Overclocker Hull ölçeği kabul; sıradaki dalga başlasın. Beş ayrık yazıcı (Haiku, high) çalıştı; kod gözle yazıldı ve ilk tam derlemede hatasız derlendi.

| Mekanizma | Modül | Durum |
|---|---|---|
| Reboot sayımı: `Resolve` üçüncü parametre `layoutPlacements`; Odometer çarpanı ölüm patlamasına (yalnız Odometer kaynakları, `ModuleRunCounterRuntime::GetDamageMultiplier`) | Triple Core, Stripped Hull, Odometer | Yeşil: `RareWave4PolishE2E` 108 kontrol |
| `statusRules.thermalBursts`, `statusRules.kineticConsumes` (Ignite DoT yerine patlama = damageFraction × 25 × çıkış çarpanı; Kinetic tüketimi `armorPenetration=1`; yeni efekt `Effect.Status.Damage.Kinetic.ArmorRend`) | Ember Accumulation, Kinetic Breaker | Yeşil: `RareStatusRulesE2E` 153 kontrol |
| `ownerBurstRules` (ShieldBroken tetiği, sahip MaxHealth oranı hasar, itme, stun elit/boss muaf, ikinci dalga; `Effect.Control.Stun.Basic`, `MovementInfluenceService::ApplyImpulse`) | Shatter Shock | Yeşil: `RareOwnerBurstE2E` 144 kontrol |
| Oran tabanlı hasar `TargetMaxHealthRatioAtLeast` + `ratioSlope`, `SourceMaxHealthGreaterThanOwner` (gelen), `targetSlowRules` | Giant Slayer, Underdog, Toppler | Yeşil: `RareRatioRulesE2E` 104 kontrol |
| `sustainedSpeedRules` (2 sn kesintisiz >%90 hız → sonik: Primary mermi hızı ×2, delme +1, tam atış hasarı ×1,10/1,15/1,20; `Afterglow` linger) | Escape Velocity | Yeşil: `RareSustainedSpeedE2E` 102 kontrol (canlı mermi vuruşu 26 → 29) |

**Dalga 4 doğrulaması (2026-10-11):** tam `cmake --build` exit 0, `ctest` 65 testte 61 yeşil; düşen dört test baştan beri düşenlerdir (PrimaryWeapon, PlayerShipSelection, WeaponPhase2, ContinuousBeamWallE2E). On yedi Rare/yeni artefakt ve Common artefaktı toplam 3.205 kontrol, hepsi `passed=true`. `Combatant::ApplyCombatDamage` (Combatant.cpp ~174-188) artık `payload.hitGroup == PrimaryShot` olan canlı mermi vuruşunda plan hesabına `Ability.Primary` katıyor; canlı sonda `PrimaryShotOnly` ×1,5'i yakaladı (`matchedLive true`, oran 1,5). Common **Shatter Current** ve R1 Guarded Strike artık canlı mermide çalışır; yeni canlı Shatter Current kontrolü eklendi.

Dersler: (1) iki test ilk koşuda 2,3333 sapma gösterdi, üretim hatası değil: ortak düşman fixture'ı (`Enemy()`) kalkanı sıfırlıyor ama yenileme gecikmesi koymuyor, hiç hasar almamış düşmanın kalkanı spawn'dan itibaren saniyede ~23,3 yenileniyor. (2) status ajanı yeni bir efekt (`ArmorRend`) ekledi; `ContentLoaderTests.cpp` ve `GasLiteCoreTests.cpp` efekt sayısı beklentisi 17 → 18 güncellendi. (3) Polish ajanı ölüm patlamasına tüm giden çarpanı uyguladı ve P6 sözleşmesini bozdu; yalnız Odometer kaynağına daraltıldı.

**Doğrulandı (2026-10-11):** canlı mermi sondası (`rare-sustained-speed.json`, `primaryShotOnlyLiveProbe`): gerçek bir `PrimaryWeaponProjectileActor` vuruşunda `PrimaryShotOnly` koşullu ×1,5 modül çarpanı EŞLEŞMEDİ (oran 1,0; canlı vuruşun kaynak etiket sayısı 0). Etkilenen gönderilmiş içerik: Common **Shatter Current** (kalkan kırılınca ana silah ek Energy hasarı) ve R1 Guarded Strike. Modül testleri doğrudan etiketli çağrı yaptığı için bunu yakalamamıştı. Düzeltme `Combatant::ApplyCombatDamage` içinde `payload.hitGroup == PrimaryShot` ise yalnız plan hesabına Primary etiketi katmak; ajan uyguluyor. Sonik tam atış çarpanı canlıda doğrulandı (26 → 29, yukarı yuvarlama).

**Önceki şüphe:** gerçek mermi vuruşu `ApplyCombatDamage`'a boş `sourceAbilityTags` geçiriyor olabilir (`PrimaryWeaponProjectileActor.cpp:294-304`); `ResolveOutgoingDamagePlan`'daki `isPrimaryShot` yalnız etiketlere bakıyorsa mevcut `PrimaryShotOnly` modül kuralları (Common, Uncommon, R1 Guarded Strike) canlı atışta eşleşmeyebilir. Testler etiket verdiği için bu yakalanmamış olabilir.

### 7.12 Dalga 5-7 uygulama durumu (2026-10-12)

**Kapsam.** Dalga 1–4'te yüklenen mekanizmaların üzerine kalan sekiz merdivenli Rare modül, 14 özel yan özellik ve üç yeni mekanizma eklendi: kill sayacı (Trophy Core), KillConfirmed şarjlı sahip merkezli patlama ve çekme (Dying Star), modüle ait kor alanı (Smoldering Remains) ile çok charge modeli, GrantCharge ve Spare Capacitor'ın komşu charge kuralı. Bu dalgalar arasında sıra ayrımı kayıt altına alınmadı; aşağıdaki tablo bu yüzden dalga numarası vermez.

| Kalem | Uygulanan | Test (artefakt kontrol sayısı) |
|---|---|---|
| Merdivenli Rare modüller (8) | Ember Accumulation (2-3), Shatter Shock (1-3), Kinetic Breaker (2-3), Giant Slayer (2-3), Trophy Core (2-3), Dying Star (2-3), Escape Velocity (2-3), Spare Capacitor (1-2-3) | `RareLadderModulesE2E` (601), `RareWave5ContentE2E` (531) |
| Özel yan özellikler (14 / 16) | Area Blast, Smoldering Remains (Ember); Stagger, Aftershock Wave (Shatter Shock); Hair Trigger, Armor Rend (Kinetic); Toppler, Underdog (Giant Slayer); Bounty Mark, Trophy Wall (Trophy Core); Core Collapse, Red Giant (Dying Star); Afterglow (Escape Velocity); Trickle Charge (Spare Capacitor) | `RareWave5ContentE2E`, `RareLadderModulesE2E`, `RareRatioRulesE2E` (104), `RareDyingStarMechanicsE2E` (151) |
| Kill sayacı (Trophy Core) | Öldürme stack'i modülde saklanır, komşu yeteneğe atfedilir; ağırlıklar normal 1, elit 7,5, miniboss 7,5, boss 25 (JSON); söküp takınca stack korunur | `RareTrophyCoreMechanicsE2E` (70) |
| KillConfirmed şarjlı patlama (Dying Star) | 300 birim içinde ölen her düşman şarj eder; 10 şarjda 350 birim Thermal patlama, hedeflere 4 Ignite; Core Collapse çekme (1 sn önce), Red Giant şarj yarıçapı 450 | `RareDyingStarMechanicsE2E` (151) |
| Yan özellik modifier birleştirme | Yan özellik modifier'ları (Status Modifiers deseni) | `RareStatusModifiersE2E` (151) |
| Modüle ait kor alanı (Smoldering Remains) | Patlama yerinde 3 sn, 150 birim, saniyede 1 Thermal stack | `RareWave5ContentE2E` |
| Çok charge modeli ve GrantCharge | Kapasite = `maxCharges` + seviye bonusu; GrantCharge `SpaceAbilitySystem` üzerinden; Trickle 20 sn'de en az charge'lı komşuya +1 | `RareSpareCapacitorMechanicsE2E` (426) |
| Spare Capacitor komşu charge | 1/2/3 hücre = 1/2/3 komşu yetenek; seviye başına charge 1/2/3 (JSON `chargePerLevel`) | `RareSpareCapacitorMechanicsE2E` |

Yapılmadı: Sonic Boom (Escape Velocity) ve Overflow Transfer (Spare Capacitor); ikisi de JSON'da yok. (2026-10-12: Sonic Boom dalga 8'de uygulandı, `RareSonicBoomE2E`; Overflow Transfer kullanıcı teyidi bekliyor.) Bkz. açık kararlar.

**Doğrulama.**
- Rare E2E: 20 test (`LightYearsGame/CMakeLists.txt:236-277`), 20 artefakt, toplam **4.701 kontrol, hepsi `passed=true`**. Artefaktlar `build/e2e-artifacts/rare-*.json` (zaman damgası 2026-10-12 18:15). Dağılım: damage-conditions 66, shield-rules 79, zone-rules 177, single-modules 414, side-features 214, static-damage-rules 442, movement-rules 150, run-counter 74, wave3-modules 544, wave4-polish 108, status-rules 153, owner-burst 144, ratio-rules 104, sustained-speed 102, dying-star-mechanics 151, trophy-core-mechanics 70, spare-capacitor-mechanics 426, ladder-modules 601, status-modifiers 151, wave5-content 531.
- Tam `ctest` (kullanıcı raporu, 2026-10-12): 72 testten **68 yeşil**. Düşen dört test baştan beri düşen: PrimaryWeapon, PlayerShipSelection, WeaponPhase2, ContinuousBeamWallE2E.
- Bu belge güncellemesinde derleme ve test yeniden çalıştırılmadı; sayılar artefakt dosyalarından ve kullanıcı raporundan alındı.

**Uygulama sırasında alınan davranış kararları (2026-10-12 durumu: 1 → tablo satır 11, kabul; 2 → tablo satır 10; 3 → tablo satır 9; 4 değişmedi).**
1. Spare Capacitor 3 hücre: varsayılan Fighter'da (iki kabuk) en çok 2 kabuğa değer; Breacher, Interceptor, Conductor, Aegis ve CryoController'da 3 kabuğa değer (`RareSpareCapacitorMechanicsE2E` başlık yorumu).
2. Stagger muafiyeti (elit, miniboss, boss) ana kuralın varsayılanıdır. Oyunda düşman sınıfı atanmadığı için muafiyet şu an yalnız test düşmanlarında devreye girer.
3. MiniBoss kill ağırlığı kartta yoktur; JSON'da elit ile aynı 7,5 kullanıldı.
4. Shatter Shock kart metni JSON'a göre düzeltildi: eski "%15" ifadesi yerine 1 hücre %10/16/22, 3 hücre %25/31/37 (L1/L2/L3).

**Açık kullanıcı kararları ve durumu (2026-10-12 güncellemesi).** Kullanıcının tarihli kararları aşağıdaki *Kullanıcı kararları 2026-10-12* alt bölümünde tutulur; bu tablo yalnız her konunun durumunu gösterir. Satır numaraları kaymıştır; modules.md satırları konu adıyla anılır (eski "203/244" yerine güncel satırlar 204 ve 245).

| # | Konu | Mevcut durum (kod/JSON) | Durum (2026-10-12) |
|---|---|---|---|
| 1 | Sonic Boom (Escape Velocity yan özelliği) | `Module.SideFeature.EscapeVelocity.SonicBoom` dalga 8'de eklendi (`boomRadius` 200, `boomPushImpulse` 75, `boomRearmBelowSpeed` 50) | **Uygulandı (2026-10-12).** İtme Boss dahil tüm sınıfları iter (sınıf muafiyeti yok; istenirse ayrı alan gerekir). |
| 2 | Overflow Transfer (Spare Capacitor) | İçerikte `overflowTransfer` yok. Kod yolu var: yükleyici bayrağı kabul eder, `ModuleReactiveRuntime::GrantTrickle` taşmayı yalnız Trickle verişinde dağıtır | **AÇIK: kullanıcıdan teyit bekleniyor.** "1'deki gibi olacak" yorumu belirsiz bulundu. Uygulanmadı. |
| 3 | Spare Capacitor charge kuralı (satır 203/244 çelişkisi) | `chargePerLevel` [1,2,3]; komşu kapasitesine seviye kadar bonus; parçalar yığılmaz | **Karar verildi (kullanıcı, 2026-10-12):** L1 +1, L2 +2, L3 +3; parçalar yığılmaz. Uygulanan A yorumu doğrulandı. |
| 4 | F1 Parçalama ve parçalı yerleşim arayüzü (R7/R8) | Çekirdek yeşil. Parçalı Spare Capacitor'da komşuluk sayısı 4 kabuk (`kAbilityShellCount = 4`, ModuleResolver.cpp:140, 410-414) | Spare çelişkisi kapandı (3). **Parçalı yerleşim arayüzü açık (R8).** |
| 5 | Rare sunumu (R8) | Yapılmadı | Açık; görsel kabul kullanıcıda. Değişmedi. |
| 6 | Düşman sınıfı atamaları | `controlTargetClass` içeren ship kaydı yok (ships.json, 0 kayıt); yükleyici hazır (R-Ön). Tüm düşmanlar Normal | **Karar verildi (kullanıcı, 2026-10-12):** Elit/MiniBoss/Boss düşmanlar tasarlanana kadar beklenecek. Boss Hunter (R3) ve elit/boss muafiyetleri bu süre boyunca fiilen işlemez. |
| 7 | Shatter Shock itme (`pushImpulse` 150) | Hız birimi. Sönüm r = gemi `linearDamping` (0,70/sn): v(t)=v0·r^t. Kuramsal toplam 150/ln(1/0,7) ≈ 420 birim; d(t)=420·(1−0,7^t) | **Karar verildi: 150 kalır (kullanıcı, 2026-10-12).** Etkin mesafe ~350 birim (~5 sn). "Kart 150 birim" ifadesi güncel değil: modules.md kartında mesafe birimi yok. |
| 8 | Aftershock Wave | `echoFraction` 0 (yankıda hasar yok), `echoPushFraction` 0,5, gecikme 1,5 sn. Yankı yarıçapı ayrı değil (`rule.radius` = 250). Hedefler yankı anında yeniden sorgulanır; ilk dalga hedefi dışlanmaz (`ModuleOwnerBurstRuntime::Wave`) | **Karar verildi: olduğu gibi kalır (kullanıcı, 2026-10-12).** Kod uyumu açık: karar "aynı düşmana iki kez isabet olmaz" der; kodda yankı yalnız 0,5× itme verir ve ilk dalga hedefini dışlamaz. Görev notundaki "73 birim" kodda doğrulanamadı. |
| 9 | PLACEHOLDER değerler | Dying Star hasarı 60/69/78; Core Collapse pullImpulse 400; Smoldering yarıçapı 150; MiniBoss kill ağırlığı 7,5 | **Karar verildi: şimdilik kalır (kullanıcı, 2026-10-12);** dengeleme (balance) koşusunda test edilip ayarlanacak. Ölçek dayanağı aşağıdaki tabloda. |
| 10 | Stagger muafiyeti | Kod varsayılanı `stunImmuneClasses` = Elite, MiniBoss, Boss (ModuleDefinition.h:438). Sınıf atanmadığı için fiilen yalnız test düşmanlarında geçerli | Ayrıca karar verilmedi; kod varsayılanı korunuyor. Sınıf kararı (6) kapsamında beklemede. |
| 11 | Spare Capacitor 3 hücre, Fighter'da 2 kabuk | Fighter'da (iki kabuk) en çok 2 kabuğa değer (§7.12 kararları, madde 1) | **Kabul (kullanıcı, 2026-10-12).** F1 (parçalama) bunu çözer. |

#### Kullanıcı kararları 2026-10-12

Aşağıdaki kararlar kullanıcının verdiği biçimde tutulur; sonraki güncellemelerde değiştirilmez, yalnız yeni tarihli karar eklenir.

1. **Spare Capacitor + F1 (2026-10-12):** Parçalı olsun olmasın L1'de komşu her yetenek +1 charge alır (varsayılan yetenek 1 charge, Q/E/R komşuysa 2/2/2); seviye yükseldikçe +L (L2 +2, L3 +3) ve parçalar yığılmaz. Uygulanan A yorumu doğrulandı. modules.md 203/244 çelişkisi "karar verildi" notuyla kapatıldı; 244'ün "+1" ifadesi L1 anlamındadır.
2. **Overflow Transfer (2026-10-12):** Kullanıcı "1'deki gibi olacak" dedi; yorum belirsiz → AÇIK KARAR, kullanıcıdan teyit bekleniyor. Uygulanmadı.
3. **Shatter Shock itme (2026-10-12):** pushImpulse 150 kalır. Kuramsal toplam mesafe ~420 birim; ancak düşman gemisi kendi itkisiyle hareket eder ve sönüm kuyruğu çok yavaştır; etkin mesafe ~350 birim (~5 sn'de). d(t)=420·(1−0,7^t): 3 sn'de ~276, 5 sn'de ~349. Kart "150 birim" ifadesi bu yüzden güncel değil.
4. **Aftershock Wave (2026-10-12):** Olduğu gibi kalır: ikinci dalga ilk patlamanın vurduğu aynı düşmana değil, sonradan gelen/yarıçap içindeki diğer düşmanlara vurur; aynı düşmana iki kez isabet gerekmez.
5. **PLACEHOLDER değerler (2026-10-12):** Dying Star hasarı (60/69/78), Core Collapse pullImpulse (400), Smoldering yarıçapı (150) ve MiniBoss kill ağırlığı (7,5) şimdilik kalır; dengeleme (balance) koşusunda test edilip ayarlanacak.
6. **Sonic Boom (2026-10-12):** Uygulandı (dalga 8, `RareSonicBoomE2E` yeşil). Sonik duruma girişte sahip merkezli şok dalgası; hasar yok; yarıçap 200; itme 75 (PLACEHOLDER); yeniden kurma: hız mutlak 50'nin altına inince (tam durma gerekmez).
7. **Düşman sınıfı (2026-10-12):** Elit/MiniBoss/Boss düşmanlar tasarlanana kadar beklenecek (kullanıcı onayı). Boss Hunter ve elit/boss muafiyetleri o zamana dek fiilen işlemez.
8. **Spare Capacitor 3 hücre (2026-10-12):** 3 hücreli Spare Capacitor Fighter'da 2 kabuğa değer: kabul. F1 (parçalama) bunu çözer.

#### Değerlerin dayanağı ve ölçek tabanı

Her Rare sayısal değerin kaynağı JSON (`modules.json`, `module_side_features.json`, `damage_status_balance.json`, `ships.json`) ve kodda çözümlendiği yerdir; "Nereden ölçeklenir" sütunu gerçek kod yolunu verir. Durum: **kart** = modules.md kart metninde açıkça var; **placeholder** = kullanıcı kararıyla geçici; **türetilmiş** = başka değerlerden hesaplanır veya öneri ölçektir. "Koddan doğrulanamadı" yazılan ayrıntı kodda bulunamamıştır. Sayılar ön hesaptır; denge koşusunda ölçülecektir.

| Değer | Nereden ölçeklenir | Seviye / boyut ölçeği | Durum |
|---|---|---|---|
| Shatter Shock hasarı, 1 hücre (%10/16/22) | Sahibin MaxHealth'i × oran (`ModuleOwnerBurstRuntime::Detonate`). MaxHealth = gemi `health` (Fighter 250, Breacher 290, Interceptor 200, Conductor 220, Aegis 270, CryoController 250) + `Owner.MaxHealth` değiştiricileri (D1 +45; `SpaceShip.cpp:160-165`). Fighter ön hesap: 25 / 40 / 55 birim | Seviye: JSON dizisi. Boyut: yalnız 1 hücre dizisini seçer | kart |
| Shatter Shock hasarı, 3 hücre (%25/31/37) | Aynı mekanizma; Fighter ön hesap: 62,5 / 77,5 / 92,5 birim | Boyut: 3 hücre dizisi | kart |
| Üretilen hasarın giden çarpanı | `AlreadyResolved` (`Combatant.cpp:194`): sahibin genel giden çarpanı uygulanmaz. Odometer'ın patlamaya ayrı uygulanması (§7.11) bu belgede ayrıca izlenmedi | — | türetilmiş (kod) |
| Shatter Shock yarıçapı 250, bekleme 20 sn | JSON `radius`, `cooldownSeconds`; sabit | Seviyeyle değişmez | kart |
| Shatter Shock itme 150 | `PushAway` → `ApplyImpulse`: hız v0 = 150, sönüm r = gemi linearDamping 0,70/sn. Toplam ≈ 150/ln(1/0,7) ≈ 420 birim; 3 sn ≈ 276, 5 sn ≈ 349 | Seviyeyle değişmez | türetilmiş (karar: 150 kalır, 2026-10-12) |
| Stagger stun 1 sn | `Effect.Control.Stun.Basic`; varsayılan muaf Elite/MiniBoss/Boss (`ModuleDefinition.h:438`) | Seviyeyle değişmez | kart (muafiyet fiilen yok, karar 7) |
| Aftershock gecikme 1,5 sn, itme %50 | `echoDelaySeconds` 1,5; `echoPushFraction` 0,5; `echoFraction` 0 (hasar yok). Yankı yarıçapı = 250 (ayrı değil) | Sabit | kart |
| Dying Star ölüm yükü 10, şarj yarıçapı 300 | `KillConfirmed`: kaynak sahip; hedef konumunun sahibe uzaklığı ≤ chargeRadius (`ModuleOwnerBurstRuntime::OnGameplayEvent`) | Red Giant: 300 → 450 (`ownerBurstModifiers.chargeRadius`) | kart |
| Dying Star patlama yarıçapı 350 | JSON `radius`, sahip merkezli | Sabit | placeholder (modules.md "öneri 350"; kullanıcı listesinde yok, denge testine dahil) |
| Dying Star hasarı 60/69/78 | Düz hasar: `damageFractionOfMaxHealth` 0 olduğu için `rule.damage`. Hiçbir istatistikten ölçeklenmez (MaxHealth, güç, Odometer yok). Seviye: tabandan +%15 toplamsal (60 → 69 → 78; bileşik olsaydı L3 79,35). Kart "+%15" der; JSON L3'te +%13 | Seviye tablosu; boyuttan bağımsız | placeholder (karar 5: şimdilik kalır) |
| Dying Star Ignite 4 stack | `igniteStacks` 4. Thermal DoT: 4 stack için `damage_status_balance.json` Thermal değeri 5,0/sn, süre 5 sn ⇒ toplam 25 (zırh öncesi) | Sabit | kart (4 stack); DPS türetilmiş |
| Core Collapse çekme 400 | `Pull` patlama anında, sahip merkezli, yarıçap 350; sönüm 0,70 ile 1 sn'de ≈ 336 birim (400·0,3/0,357) | Sabit; boss muaf (`pullImmuneClasses` varsayılanı Boss); elit çekilir | placeholder (karar 5) |
| Core Collapse gecikme 1 sn | `pullLeadSeconds` 1; ana dalga bu süre sonra yeniden sorgulanır (`ScheduleWave`) | Sabit | kart |
| Ember patlaması %130/140/150 | `damageFraction` × `ThermalDamagePerSecond(4)` (5,0) × Thermal süre (5 sn) = oran × 25 (`DamageTypeSystem.cpp`, `ApplyThermalBurst`). L1 32,5; L2 35; L3 37,5 (zırh öncesi) | Seviye JSON; boyuttan bağımsız | kart (oran); taban türetilmiş |
| Area Blast 200 birim, %60 | `areaRadius` 200; `areaDamageFraction` 0,6 × ana patlama | Sabit | kart |
| Smoldering Remains 3 sn, 1 stack/sn, yarıçap 150 | `smolderSeconds` 3; `smolderStacksPerSecond` 1; `smolderRadius` 150 (`ModuleReactiveRuntime::SpawnSmolderingField`) | Sabit | kart (3 sn, 1/sn); yarıçap 150 placeholder (karar 5) |
| Kinetic Breaker çarpanı 1,10/1,15/1,20 | `kineticConsumes.damageMultiplier`; zırh tamamen yok sayılır (`armorPenetration = 1`, `ApplyKineticConsume`) | Seviye JSON; boyuttan bağımsız | kart (onaylı yeni parça) |
| Kinetic eşiği 4 (Hair Trigger: normal hedefte 3) | `stackThreshold` 4; `normalTargetThreshold` 3 (yalnız Normal sınıf) | Sabit | kart |
| Armor Rend −%25, 4 sn | `rendFraction` 0,25; `rendSeconds` 4; Armor çarpanı 0,75 | Hedef zırhından bağımsız sabit yüzde | kart |
| Giant Slayer taban çarpanı 1,10/1,15/1,20 | `damageMultiplier` (seviye tablosu) | Seviye JSON | kart |
| Giant Slayer eğimi 0,05 | `ratioSlope`: çarpan += 0,05 × (oran − 1,5) (`CombatRuntime.cpp:417-420`); oran = hedef MaxHealth / sahip MaxHealth (`CombatRuntime.cpp:383`). Üst sınır yok | Eğim seviyeyle değişmez | türetilmiş (öneri eğim; modules.md "Claude yorumu") |
| Giant Slayer / Toppler / Underdog tetik erişilebilirliği | Mevcut düşmanlar: ApproachGunner 60, StrafeSkirmisher 60, RangeKeeper 100 (ships.json). Oyuncu gemileri 200–290; oran en fazla ~0,5. 1,5 ve 2,5 eşiklerine ve "düşman > sahip" koşuluna ulaşılamıyor. Spawn sırasında düşman sağlığına ölçek uygulanıp uygulanmadığı koddan doğrulanamadı | — | türetilmiş (JSON); tetik fiilen yok |
| Toppler yavaşlatma | `minRatio` 2,5; `slowFraction` 0,30; `bossSlowFraction` 0,15; `seconds` 2 (`ModuleTargetSlowRuntime`) | Sabit | kart (tetik şu an erişilemez) |
| Underdog gelen ×0,92 | `SourceMaxHealthGreaterThanOwner` gelen hasar kuralı | Sabit | kart (tetik şu an erişilemez) |
| Trophy Core normal öldürme +%0,40/0,45/0,50 | `damagePerStack` × ağırlık; bonus = Σ(ağırlık × damagePerStack); komşu yeteneğe çarpan 1 + bonus (`ShipModuleApplier::OnOwnerKill`). Komşu sınırı JSON'da 3 | Seviye JSON; boyuttan bağımsız; sökünce stack korunur | kart |
| Trophy elit/boss ağırlığı 7,5 / 25 → %3 / %10 (L1) | `killWeights` (`modules.json`); L3'te elit %3,75, boss %12,5 | Seviye ile yalnız damagePerStack artar | kart (elit %3, boss %10); miniboss 7,5 placeholder (karar 5) |
| Trophy Wall: stack'in %50'si | `killStackDamageRules` fraction 0,5; yalnız Offense ailesi (`ModuleResolver.cpp:583`) | Sabit | kart |
| Bounty Mark basamakları | `killStackCooldownRules`: her 25 stack'te indirim [%5, %4, %3, %2], sonrası %1 (tail); min 1 sn; çarpan = 1 − indirim (`GetKillStackCooldownReduction`, `ModuleResolver.cpp:575`). Elit 7,5, boss 25 stack sayılır | Seviye ölçeği yok | kart |
| Odometer: 5000 birim = 1 stack; stack başına +%1/1,2/1,4 hasar ve hız | `unitsPerStack` 5000; stack = floor(mesafe / 5000) (`GetModuleRunCounterStacks`); mesafe = hız büyüklüğü × dt; çarpan = 1 + değer × stack (`ModuleRunCounterRuntime.h`). 5000 birim ≈ 9,6 sn tepe hızda (Fighter maxSpeed 520; hız ile aynı birim varsayımı) | Seviye JSON; boyuttan bağımsız | kart (5000 ve yüzde); 9,6 sn türetilmiş |
| Stripped Hull: boş hücre başına +%2,5/3/3,5 | `perEmptyCellMultiplier` (`ModuleResolver.cpp:539`): çarpan = 1 + oran × boş hücre. Boş = şemada kilitsiz ve bölgeli hücre eksi modül kapsamı (`CountEmptyCells`, :457-476). Varsayılan kurulumda 40 hücreli gemiler (Breacher, Interceptor, Conductor, Aegis, CryoController) 28; Fighter 26 (38 oynanabilir hücre; 16 küçük harfli kilitli hücre sayılmaz). Kendi hücresi dolu sayılır; Fighter L3 ≈ 26 × 3,5 = +%91 | Seviye JSON; kurulumla değişir | kart (oran); "28" kullanıcı varsayımı: 40 hücreli gemilerde doğru, Fighter'da değil |
| Triple Core çarpanı 1,25/1,30/1,35 | `damageMultiplier`; koşul `AbilityDirectHit` ve tam 1 Q/E/F/R slotu boş (`requiredEmptyAbilitySlots` 1) | Seviye JSON; boyuttan bağımsız | kart metni ×1,18 güncel değil; değer onaylı (2026-10-10) |
| Omni Thrusters ileri itki ×0,70/0,715/0,73 | `thrustMultiplier` ve `equalizeDirections` (`movementRules`). Fighter 650/190/270 → L1 455/455/455; L2 464,75; L3 474,5 | Seviye JSON | kart |
| Momentum Shield regen ×1,5/1,7/1,9 (tepe hızda) | Normal regen × `speedMultiplierAtMax` × (hız / canlı hız tavanı); gecikme yok; duran gemide 0 (`SpaceShip.cpp:240-262`) | Seviye JSON | kart (sayılar öneri) |
| Continuous Field Generator regen %35/45/55 | Normal regen × `continuousRateFraction`; gecikme yok (`SpaceShip.cpp:261`) | Seviye JSON | kart |
| CFG yeniden kurulum %25 | `breakRearmFraction` 0,25: kırılma tetiği kilitlenir; Max Shield'in %25'i dolmadan yeniden tetiklenmez (`ModuleTriggerRuntime.cpp:118-145`) | Sabit | kart (kullanıcı, 2026-10-08) |
| Impact Redirector kalkan Armor payı %50/60/70 | `armorShare`; kalkan harcaması f = 1 − pay × (1 − hull çarpanı) (`SpaceShip.cpp:400-407, 427-430`); yalnız kalkan > 0 | Seviye JSON | kart (onaylı) |
| Zone Overclocker bölge bonusu %50/55/60 | `AmplifyZone`; yükseltici = bonus / 0,25 (`AmplifierScale`, `kReferenceZoneBonus`, `ModuleDefinition.h:141`). Gövde (%10) → %20 | Seviye JSON | kart (%50 vb.); gövde %20 uygulanan varsayım, onay bekliyor |
| Misfit Wiring aile dışı ana değer +%25/30/35 | `OutOfZoneMainBonus` (`LayoutOutOfZoneMainBonus`) | Seviye JSON | kart |
| Sonik eşik %90 hız, 2 sn; sonra 1,5 sn linger | `speedFraction` 0,9; `holdSeconds` 2; `lingerSeconds` 1,5; hız tavanı canlı tavan (`ModuleReactiveRuntime::TickSustainedSpeed`) | Sabit | kart |
| Sonik Primary mermi hızı ×2, delme +1 | `primarySpeedMultiplier` 2; `primaryPierceBonus` 1 (`AddPrimaryWeaponModifier`); canlı ölçüm 26 → 29 (dalga 4) | Sabit | kart |
| Sonik tam atış hasarı ×1,10/1,15/1,20 | `primaryDamageMultiplier` (PrimaryShotOnly) | Seviye JSON | kart |
| Sonic Boom: yarıçap 200, itme 75, yeniden kurma hızı < 50 | Karar (2026-10-12); dalga 8'de uygulandı; itme 75 PLACEHOLDER | — | placeholder (karar 6) |
| Spare Capacitor komşu charge +1/+2/+3 (L1/L2/L3) | `chargePerLevel` [1,2,3] (`ModuleResolver.cpp:414`); komşu sayısı 1/2/3 hücre; parçalıysa 4 kabuk (`kAbilityShellCount`); yığılmaz | Seviye ile artar; boyut yalnız komşu sayısını artırır | kart (karar 1) |
| Trickle Charge +1 / 20 sn | `trickleSeconds` 20; `trickleCharges` 1; en az charge'lı komşuya (`GrantTrickle`) | Sabit | kart |
| Ortak Rare O1–O5, D1–D5, G1–G5 | Uncommon (L1) değeri × 1,75, yuvarlama. Eklemeli değerler: AP 8 → 14, Crit 8 → 14, AS 4 → 7, Haste 4 → 7, Hareket 0,8 → 1,4, D1 25 → 45 (43,75 yuvarlandı), D3 %1 → %1,75, Luck 1 → 2 (1,75 yuvarlandı). Çarpanlar: (çarpan − 1) × 1,75 (D5 1,08 → 1,14; G3 1,10 → 1,18; O5 1,15 → 1,25 yuvarlandı). Süre: O4 1 → 1,5 (1,75 yuvarlandı). Kaynak: modules.md ölçek kararı (2026-10-10) | Seviye JSON; Rare kimlikleri ayrı (`*.Rare`) | türetilmiş (öneri ölçek; kullanıcı ×1,75'i onayladı) |
| R1 Guarded Strike ×1,15, 4 sn | `damageRules` PrimaryShotOnly; `activeWhileTrigger` AbilityActivated (Defense); `activeDurationSeconds` 4 | Sabit | kart (sayı öneri) |
| R2 Reactive Plating Armor +30, 4 sn | `triggerBuffs` ShieldBroken; süre 4 sn | Sabit | kart (onaylı) |
| R3 Boss Hunter ×1,10; R4 Full Charge ×1,08; R5 Status Weaver ×1,20 | `TargetIsBossClass`; `OwnerShieldFull`; `TargetStatusKindsAtLeast` 2 | Sabit | kart (onaylı); R3 fiilen yok (karar 6) |
| R6 Fleet Guard gelen ×0,85, 3 sn | Gelen hasar kuralı; `activeWhileTrigger` AbilityActivated (Movement); süre 3 sn | Sabit | kart (onaylı) |
| F1 Parçalama | `fragmentation` bayrağı; sayısal değer yok | — | kart |

#### Bilinen belge-kod tutarsızlıkları (2026-10-12)

- Triple Core kart metni "×1,18" diyor; JSON 1,25/1,30/1,35 (onaylı değer). Kart metni güncellenmeli (modules.md satır 202).
- Dying Star kart metni "+%15" diyor; JSON 60 → 69 (+%15) → 78 (+%13).
- Giant Slayer, Toppler ve Underdog mevcut düşman sağlığı değerleriyle tetiklenemiyor (yukarıdaki satır).
- Trophy Core ve Spare Capacitor komşuluk sınırı: modules.md satır 333 "en çok 2 slot" diyor. Trophy Core JSON'da `adjacentAbilityLimit` 3 (Bounty Mark da 3); Spare Capacitor'da sınır boyutla gelir (1/2/3 hücre).
- Stripped Hull "28" boş hücre varsayımı yalnız 40 hücreli gemilerde varsayılan kurulumda doğrudur; Fighter'da 26.
- Shatter Shock itme: "kart 150 birim" ifadesi modules.md kartında yok; birim yalnız plan notlarında geçiyordu.
- Aftershock: karar (hedef dışlaması) ile kod uyumsuz (karar tablosu, satır 8).
- Overflow Transfer: "kodda yok" notu içerik için doğru; yükleyici bayrağı ve Trickle taşma yolu koddadır.
- Sonic Boom: dalga 8'de uygulandı; belge güncellemesi sırasında henüz yoktu.

### 7.7 Çok charge semantiği (kullanıcı onayladı, 2026-10-10)

**Kullanıcı tarifi:** League of Legends'taki gibi (örnek Heimerdinger Q): yetenek 3 kez atılabilir; bir kez atınca 2 charge durur, bir yandan cooldown işler ve charge'lar tek tek yenilenerek 3'e çıkar; üç atışın ardı ardına spam edilmemesi için kullanımlar arasında kısa bir bekleme vardır.
Her kullanım bir charge harcar; cooldown, charge kapasitenin altına düşünce çalışır ve her tamamlanışta **bir** charge geri verir; charge > 0 iken yeni kullanım cooldown'u beklemez (yalnız kısa bir kullanım aralığı olabilir). `maxCharges = 1` için bu davranış bugünküyle aynıdır, yani 55 yetenekte hiçbir şey değişmez. Aynı ortak yardımcı hem `TickCooldown` hem `ReduceCooldown` için "cooldown 0'a ulaştı → bir charge geri ver" kuralını uygular; böylece §7.4'teki kilit de kapanır. Kapasiteye modül bonusu `AbilityRuntimeState`'te çalışma zamanı alanı olarak eklenir (tanım `maxCharges` + bonus); sınırsız (`maxCharges <= 0`) yetenekler etkilenmez. Etkilenen yerler: `AbilityRuntimeState`, `GameplayAbilityInstance` (cooldown başlatma, `CanActivate`), `GameAbility` (ResolveCooldownDuration, iade yolları), ASC bonus kapasite API'si, modül uygulayıcısı.

**Netleşen kurallar:** (1) Tek cooldown zamanlayıcısı, charge < kapasite iken çalışır; tamamlanınca bir charge geri verir ve charge hâlâ eksikse yeniden başlar. (2) Her kullanım bir charge harcar; charge varken kullanım cooldown'u beklemez. (3) Kullanımlar arasında **kısa kullanım aralığı** vardır (anti-spam); süre yetenek başına JSON'dan gelir, varsayılan öneri 0,5 sn, kullanıcı değeri vermedi. (4) `maxCharges = 1` için davranış bugünküyle aynıdır. (5) Cooldown'u azaltan her yol (Tick, `ReduceCooldown`, iade) aynı "sıfıra ulaştı → bir charge ver" yardımcısını kullanır. Açık: kullanım aralığının süresi, cooldown azaltmanın tek charge dönüşüne mi yoksa kalan zamana mı uygulandığı (öneri: aktif zamanlayıcıya).

### 7.4 Mevcut içerikte olası gizli kusur: cooldown sıfırlanınca charge geri gelmiyor
`AbilityRuntimeState::ReduceCooldown` cooldown'u sıfıra kırpar ama charge'ı geri vermez (`:146-155`); `TickCooldown` cooldown ≤ 0 iken erken döner ve `UpdateCooldown` yalnız `IsOnCooldown()` iken çalışır (`GameplayAbilityInstance.h:449-460`). `maxCharges = 1` olan bir yetenekte kullanım charge'ı 0'a indirir; bir modül kalan cooldown'u tam sıfıra indirirse (Hunt Clock −0,25 sn "sıfırın altına inmez", `ReduceAllCooldowns` amount ≥ kalan, oran 1,0 olan `ReduceLongestCooldown`/`RefundUsedCooldown`) yetenek `charge = 0, cooldown = 0` durumunda kalır ve `CanActivate` onu bir daha etkinleştirmez. Mevcut bir test yorumu bunu bilinen davranış olarak anıyor (`GasLiteCoreTests.cpp:6861-6862`). **Kod okumasıyla doğrulandı, çalışma zamanında denenmedi.** Bu gönderilmiş Uncommon içeriği (Hunt Clock) için de geçerli olabilir; ayrı bir E2E ile kanıtlanmalı.

### 7.5 Diğer bulgular
- **Bölge kuralları:** `GetZoneBonus` tek yerleşime bakar (`ModuleResolver.cpp:200-207`); Zone Overclocker diğer yerleşimleri görmek için imza değişikliği ister (presenter + ~16 test çağrısı). Hull bölgesi bonusu %25 değil **%10**, AbilityBay 0. `ModuleZoneEffect` kullanılamaz (çarpan (0,1] ile sınırlı ve yalnız cooldown üretir). Misfit Wiring'in bonusu hem ana hem yan pakete gider; "yalnız ana değer" için ayrı parametre. Boş hücre sayısı çalışma zamanında hazır değil, `kTotalCellCapacity − kapsanan hücre` olarak türetilir (40 − …).
- **Uyuşmazlık:** doğru nokta `ModulePlacementRules::CanPlace` (DuplicateModule satırının yanı, `:105-110`); `ModulePlacementFailure` düz enum, çatışan modül kimliğini taşıyamaz; presenter yalnız metin yazar, iki modülü işaretlemek için view modeline çatışan `instanceId` eklenmeli.
- **F1 Parçalama: BÜYÜK.** `ModulePlacement` tek `origin`+`rotation`+`variant` taşır. Etkilenenler: `ModuleLoadout` (Install/Move/Uninstall), `ModulePlacementRules:110` (aynı modül kimliği iki kez), `ModuleResolver:209-252`, `ShipModuleApplier:113-132`, view modeli ve presenter. Kayıt/serileştirme yok (loadout yalnız bellekte), bu yüzden kayıt göçü gerekmez.
- **Kalkan:** `ShieldBroken` latch'siz, kırıl → az dol → kırıl her seferinde yeni olay üretir (`ModuleTriggerRuntime.cpp:116-124`); %25 yeniden kurulum için ShieldComponent'e dolum izleme ve tetik runtime'ına `onShieldChanged` aboneliği gerekir. Regen oranı `SpaceShip::UpdateRegeneration`'da hesaplanır (hız bağlamak için kanca), duran gemide çarpan 0 olunca modifier silinip regen %100 olur (`ModuleReactiveRuntime.cpp:199`).
- **Hareket:** "Omni Thrusters" için iki hazır parça var: `thrustBonus` ve `SetDirectionalThrustSync` (geri/yan = ileri). İleri −%30 + eşitleme = 650/190/270 → 455/455/455, ama ikisi de modül hedefi değil. Sonik durum için kesintisiz yüksek hız sayacı yok; uygun yer `ModuleReactiveRuntime::UpdateAttributes`.
- **Ember:** kanca `DamageTypeSystem.cpp:454-462`; Ignite'ın birikmiş hasarı yalnız son stack bitince teslim edildiği için stack silmeden önce boşaltılmalı. **Kinetic Breaker:** kanca `ApplyStatusEffects` (`:422`), mitigasyondan önce.
- **Üretilen kural:** kill patlaması hedef konumunda çalışır, sahip merkezli değil; `ApplyModuleGeneratedAreaDamage` yeniden kullanılabilir ama aktif lease ister. Çekme yok. "Sahibin 300 birim yakınında herhangi bir ölüm" için dünya düzeyinde ölüm yayını yok.
- **Kill atfı:** olay kaynak yetenek kimliğini taşır, slot `GetAbilityById(...)->slot` ile türetilir (Q/E/F/R eşlemesi doğrulanmadı); kill yolunda komşuluk filtresi yok.
- **Sayaç:** `ModulePlacement`'ta sayaç alanı yok; sök/tak alan kopyalar (kalıcılık korunur); `Player::ResetRunProgression` sıfırlar; `RunGold` deseni yeniden kullanılabilir.
- **Test fixture:** `Fixture::Install` havuz dışı ortak özellik kuramaz; atlama yalnız testte `rules.lookup`'ı değiştirilmiş kopya `ModuleDefinition` ile mümkün.
- **Rare kodda özel dal yok:** `ModuleRarity::Rare` yalnız enum ve ayrıştırma; `assets/content/data` içinde Rare kayıt yok. *[R0 anının bulgusu (2026-10-10). Güncel durum: 17 Rare kayıt `modules.json`'da, 35 Rare yan özellik `module_side_features.json`'da; bkz. §7.9–§7.12.]*

### 7.6 Faz düzeltmeleri
- **R-Ön (yeni, R1'den önce) — tamamlandı 2026-10-10:** düşman sınıfı verisi (§7.2) ve §7.4 kusurunun E2E ile kanıtı ve düzeltmesi. (A) `ShipLoader` gemi JSON'undan opsiyonel `controlTargetClass` okur (Normal/Elite/MiniBoss/Boss, yoksa Normal, bilinmeyen değer hata); içerik JSON'u değişmedi, oyunda hâlâ Elit/MiniBoss/Boss düşman yok. E2E: `ControlTargetClassLoadE2E` (21 kontrol). (B) Kusur kanıtlandı (ability düzeyinde ve gönderilmiş Hunt Clock modülüyle gerçek öldürmede cooldown sıfıra inince charge 0 kalıyordu) ve düzeltildi: `AbilityRuntimeState::ReduceCooldown(amount, maxCharges)` cooldown 0'a ulaşınca `TickCooldown` ile aynı kuralla charge'ı geri verir. E2E: `AbilityChargeRestoreE2E` (37 kontrol, 3 senaryo). Tam paket 46/50; kalan 4 düşüş (PrimaryWeapon, PlayerShipSelection, WeaponPhase2, ContinuousBeamWallE2E) bu işten önce de düşenlerdir. Çok charge semantiği (§7.7) henüz uygulanmadı.
- **R1:** yalnız R1, R2, 15 kopya ve yeni koşullar (boss, kalkan dolu, ≥2 statü, koşulsuz, MaxHealth oranı); **R6 Fleet Guard** ayrılır: gelen hasar kuralına pencere desteği (yükleyici + `CombatRuntime:478`) ya da başka biçim gerekir.
- **R3:** Momentum Shield için `UpdateRegeneration` kancası; CFG için gecikme sıfırlama modül yolu ve %25 yeniden kurulum; Impact Redirector için kalkan öncesi Armor adımı (çift sayımdan kaçınarak); uyuşmazlık.
- **R4:** Omni Thrusters için modül hedefi (`thrustBonus`, eşitleme bayrağı); sonik sayaç; Odometer sayacı (`RunGold` deseni).
- **R6a:** Shatter Shock, Dying Star, Trophy Core. **R6b:** Spare Capacitor — charge modeli genişletilir (kullanıcı kararı, §7.3); önce §7.7 semantiği onaylanır.
- **R7 (F1)** değişmez: büyük, son sıra.

---

## 8. Sonuç

**Durum (2026-10-12):** R0 ve R-Ön tamamlandı; R1–R6 dalga 1–7 ile uygulandı. Oyun içeriğinde 17 Rare modül (9 tek boyutlu, 8 merdivenli) ve 35 yeni Rare yan özellik var. Rare E2E'leri 20 test, 4.701 kontrol, hepsi geçti (§7.12). Tam `ctest` 72 testten 68 yeşil; düşen dört test baştan beri düşen. Kullanıcı kararları (2026-10-12) §7.12'de işlendi.

**Açık kalanlar (2026-10-12):** F1 Parçalama çekirdeği tamam ama parçalı yerleşim arayüzü yok (R8); Rare sunumu (R8) yapılmadı; Silent Running kapsam dışı (rota/Threat borcu); Sonic Boom dalga 8'de uygulandı; Overflow Transfer kullanıcı teyidi bekliyor; düşman sınıfı verisi oyunda atanmıyor (elit/MiniBoss/Boss tasarımı bekleniyor); PLACEHOLDER değerler (Dying Star 60/69/78, Core Collapse 400, Smoldering 150, MiniBoss 7,5) denge koşusunu bekliyor; Aftershock kararı ile kod uyumu (yankı hedef dışlaması) açık. Kullanıcı onayı olmadan yeni faz başlatılmaz. R8 sunumu için kullanıcı kararı gerekir.

## Güncel düzeltme — 2026-10-12 gece

Kullanıcı oynanış hatalarını bildirdi. Momentum katsayıları 0,5/0,7/0,9; hız <200 iken yenilenme yok, Afterburner engelinden bağımsız. CFG L1 %30 (L2/L3 %45/%55). Omni itiş ile normal hız tavanını birlikte düşürür. Inferno Spray ortak hasar sınırını ve kaynak yetenek kimliğini kullanır; Stripped Hull, Triple Core ve Opening Hit bu yola uygulanır. Kinetic Breaker yalnız Direct enum'una bağlı değildir: periyodik olmayan teslimatlar kabul edilir, modül/suppressed hasarı tüketmez. Dual Kinetic Blaster mevcut normal Photonic/güçlendirilmiş Kinetic düzenini korur (kullanıcının açık seçimi). Dying Star ham patlama = 60/69/78 + 0,40 × Energy Power; şarj tamamlanınca bu tutar hesaplanır, Core Collapse gecikmesi varsa tutar o dalgaya taşınır. Sonic Boom kodu/içeriği/testi kaldırıldı; Slipstream Control sonik durumda dönüş kabiliyeti ×1,25 verir. Bu bölüm eski Sonic Boom ve sabit Dying Star hasarı kayıtlarının yerine geçen güncel karardır. [Sayısal sözleşme](ABILITY_SCALING_CATALOG.md), [modül kataloğu](modules.md).
