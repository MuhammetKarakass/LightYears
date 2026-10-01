# Birincil Silahlar Uygulama Planı (Faz 2)

**Tarih:** 2026-10-02 · **Branch:** `codex/luna-callback-lifetime-checkpoint` · **Kaynak tasarım:** [Light_Years_Ships_Primary_Weapons_Combined.md](Light_Years_Ships_Primary_Weapons_Combined.md) · **Önceki faz:** [SHIP_ROSTER_PLAN.md](SHIP_ROSTER_PLAN.md)

Bu plan altı oyuncu silahını tasarım dokümanındaki davranışa getirir. Uygulayıcılar için: Sol orkestrasyonu yürütür, her adımı bir Luna uygular, derler, test eder ve commitler. Adımlar sırayla yapılır; bir adımın build ve hedefli testleri geçmeden sonrakine geçilmez. `orchestrator.py` kullanılmaz. Bölüm 6'daki kararların hepsi **2026-10-02'de kullanıcı onayıyla önerildiği gibi kapandı**; plan uygulamaya hazır.

---

## 0. Mevcut durum (kaynakta doğrulandı)

| Konu | Durum | Kaynak |
|---|---|---|
| Silah tipleri | `ProjectileStandard`, `ProjectileShotgun` (pellet), `ArcElectric`, `BeamContinuous`, `WaveExpanding`; özellik: `Heat` | `include/gameConfigs/combat/WeaponStructs.h` |
| Formül modeli | `Common.Damage` taban + seviye başı `Add` + `scalingRules` (sahip stat × katsayı) + seviye başı `scalingRules` katsayı artışı. `L` tabanlı doğrusal formüllerin hepsi bu modelle yazılabilir (Fighter bugün böyle). | `weapons.json`, `WeaponProgressionProfile` |
| Empowered | `empoweredShot` + `PrimaryWeapon.Empowered.{BonusDamage, EverySuccessfulShots, FinalMagazineRounds}`. Sayaç **başarılı atış** sayar, isabet değil. Yalnız `ProjectileStandard` işleyicisi `shotMetadata.isEmpowered` kullanıyor; doğrulayıcı empowered'ı yalnız "interval projectile" silahlara izin veriyor. | `PrimaryWeaponExecutionSystem.cpp:419-441`, `PrimaryWeaponDefinitionValidator.cpp:481-530` |
| Status'lar | Cryo yavaşlatma %4/8/12/20, Electric hasar alımı %3/6/9/16, Kinetic ArmorPen %6/12/18/30, Thermal (Ignite). Değerler tasarımla **aynı**. Süre: Electric 4 s, diğerleri 5 s; politika `Stack` + `DecayAfterDuration` (her stack süre dolunca tek tek düşer). | `damage_status_balance.json`, `effects.json`, `DamageTypeSystem.cpp` |
| Stack uygulama | Silah `Damage.ElectricStacks` / `Damage.KineticStacks` / `Damage.Cryo.BuildupPerHit` / `Damage.IgniteStacks` taşıyorsa **her vuruş** stack ekler. Kinetic: mevcut stack'lerin ArmorPen'i vuruşa uygulanır, yeni stack vuruştan **sonra** eklenir (tasarımla aynı). | `DamageTypeSystem.cpp:198-377` |
| Heat | Işın hasarı ısıyla **doğrusal** artar: `1 + (DamageMultiplierAtMaxHeat − 1) × heatRatio`. Heat gain/capacity/dissipation/overheat mevcut. Hasar tipi her zaman Energy (kalkana ×1,5). | `ContinuousBeamWeaponHandler.cpp:177-189`, `HeatWeaponFeatureHandler.cpp` |
| Global tick | Periyodik hasar 0,25 s (`CombatTick::Interval`); Heat Laser bu tick'e zaten bağlı. | `gameConfigs/combat/CombatTick.h` |
| Yetenek olayı | `AbilitySystemComponent::onAbilityActivated(AbilityHandle)` var. Birincil silah da bir yetenek (PrimaryFire yuvası) — Electric şarjı için hariç tutulmalı. | `SpaceAbilitySystem/include/AbilitySystemComponent.h:243` |
| İsabet olayı | `CombatRuntime` salt-okunur `onDamageDealt` delegesi taşıyor (modül sisteminde eklendi). | commit `3daba62` |
| Modül sistemi | Modüller birincil silaha `Common.*` modifier'ları uygular (ASC'deki revizyonlu liste, attribute resolver PrimaryFire dalı). Yeni formüller `Common.Damage` / `Common.FireRate` üzerinden kalmalı ki modüller etkilemeye devam etsin. | commit `1844223` |
| **Düşmanlar** | `Weapon.Projectile.EnemyTwinBladeScatter.Basic` **pellet shotgun** tipini, `Weapon.Wave.EnemyHexagonCryoPulse.Basic` **dalga** tipini kullanıyor. Ironclad Protocol yeteneği `Weapon.Projectile.IroncladMinigun.Basic` (standart, Kinetic) kullanıyor. | `weapons.json`, `enemy_combat_profiles.json` |
| Geçici | Dual'ın AttackSpeed katsayısı 0,12 (Faz 1 K4); `GasLiteCoreTests` bu değeri bekliyor. | commit `ce9f3ab` |

**Temel kural:** Bütün yeni mekanikler **silah tanımında açılan (opt-in)** özelliklerdir. Mevcut tipler ve davranışlar, açıkça değiştirilmeyen silahlar (düşman silahları, Ironclad minigun) için **aynen** kalır. Pellet shotgun ve dalga tipi düşmanlar için yerinde durur.

---

## 1. Hedef silah verileri (L1 referans değerleri testlerde kullanılır)

`L` = silah seviyesi, sınırsız. Formüller tasarım dokümanından; değerler hedef gemi L1 statlarıyla (Fighter AP 39 EP 35; Breacher AP 45 EP 34 HP 290; Interceptor AP 41 EP 28; Conductor AP 27 EP 41; Aegis EP 43; Cryo AP 27 EP 40).

| Silah | Normal hasar formülü | Ateş hızı | L1 normal vuruş | Empowered | L1 empowered |
|---|---|---|---:|---|---:|
| Rapid Laser | `10 + 5(L−1) + AP×0.41` | 4/s, şarjör 48 / 2 s | 25,99 | her 6 başarılı (bkz. K1); bonus mevcut (2 + EP×0.10, kritik garantili) | 25,99 + 5,5 |
| Rapid Shotgun | `38 + 11(L−1) + AP×(0.60 + 0.04(L−1))` | 1,75/s | 65,0 | her 3. tetik; bonus `EP×0.15 + MaxHP×0.03` | 78,8 |
| Dual Kinetic | `3 + 2(L−1) + AP×(0.20 + 0.02(L−1))` | `7 × (1 + AS%)`, şarjör 64 | 11,2 | her 8 başarılı (K1); bonus `5 + 2(L−1) + AP×0.08 + EP×(0.25 + 0.04(L−1))`; +1 Kinetic stack | 26,48 |
| Electric Launcher | `20 + 2(L−1) + Baskın×(0.40 + 0.02(L−1)) + İkincil×(0.10 + 0.01(L−1))`, Baskın = max(AP, EP) | 2/s; şarjlıyken 4/s | 39,1 | yetenek kullanımı → 2 şarj (yenilenir, birikmez); bonus `3 + (L−1) + EP×(0.08 + 0.02(L−1))`; +1 Electric (zincir hedefleri dahil) | 45,38 |
| Heat Laser | tick başı `13 + 2(L−1) + EP×(0.19 + 0.02(L−1))`, **AP yok** | 0,25 s tick (4/s) | 21,17 / tick | ısı bölgesi: 0–35 % ×1,00 · 35–75 % ×1,15 · 75 %+ ×1,40 ve Energy (kalkana ×1,5) | 29,64 / tick (kalkan 44,46) |
| Cryo Projector | `20 + 2(L−1) + AP×(0.30 + 0.02(L−1))` | 2,5/s; Whiteout'ta ×2 | 28,1 | Whiteout'ta dönüşümlü; bonus `5 + 2(L−1) + EP×(0.07 + 0.01(L−1))`; hedef vuruştan önce 4 Cryo taşıyorsa ×1,15; +1 Cryo | 35,9 (41,3) |

Kritik/yuvarlama: Rapid Laser `CeilFinalDamage` ve empowered garantili kritik **korunur**; diğer silahlarda kritik normal (rastgele) kalır.

---

## 2. Adımlar

### Adım 1 — Ortak altyapı A: empowered sinyali tüm işleyicilere

**Dosyalar:** `WeaponStructs.h`, `PrimaryWeaponExecutionSystem.cpp`, `PrimaryWeaponDefinitionValidator.cpp`, işleyiciler (`handlers/*`), `ExpandingWaveWeaponActor.cpp`, `ElectricArcWeaponHandler.cpp`, `PrimaryWeaponProjectileActor.cpp`, `WeaponLoader.cpp`

1. `PrimaryWeaponEmpoweredShotDefinition` genişler:
   - `trigger`: `EverySuccessfulShots` (bugünkü), `EverySuccessfulHits` (K1'e bağlı), `AbilityCastCharges`, `Whiteout`.
   - `statusStacksOnlyWhenEmpowered` (bool, varsayılan false): true ise silahın `Damage.*Stacks` / `Cryo.BuildupPerHit` değerleri **yalnız empowered** vuruşta payload'a girer; normal vuruşta 0.
   - `AbilityCastCharges` için: `chargesPerCast` (int), `chargedFireRate` (float).
2. `shotMetadata.isEmpowered`, `ProjectileStandard` dışındaki bütün işleyicilere (yeni koni, arc + zincirleri, dalga aktörü) taşınır ve `BuildPayload`'a/`ApplyStatusEffects`'e ulaşır.
3. Doğrulayıcı: empowered artık "yalnız interval projectile" ile sınırlı değil; her tip için tetikleyiciyle tutarlılık kuralları (ör. `AbilityCastCharges` şarjörsüz olabilir; `Whiteout` yalnız `FrostMeter` özelliği olan dalga silahında).
4. `EverySuccessfulHits` (K1 "isabet" seçilirse): sayaç `CombatRuntime::onDamageDealt` üzerinden, **bu silahın** vuruşlarıyla artar (yetenek/DoT hasarı saymaz). Bir atışın birden çok isabeti (pierce/zincir/koni) tek sayılır mı — **atış başına en fazla 1** önerilir.
5. Hiçbir mevcut silahın davranışı değişmez (alanların hepsi varsayılan kapalı).

**Kabul:** Build exit 0. Mevcut `LightYearsPrimaryWeapon`, `LightYearsContent`, `LightYearsContinuousBeamWallE2E` ve düşman testleri değişmeden geçer. Yeni birim kontrolleri: her işleyicide empowered bayrağı payload'a ulaşıyor; `statusStacksOnlyWhenEmpowered` normal vuruşta stack 0 veriyor.

### Adım 2 — Ortak altyapı B: stack yaşam kuralı ve AttackSpeed ritmi

1. **Yeni stack yaşam politikası** `ClearAllAfterSourceInactivity` (`GameplayEffectStackLifetimePolicy`):
   - Stack'ler zamanla tek tek düşmez; hedef **aynı kaynaktan** `inactivitySeconds` (6 s) boyunca hiç hasar almazsa hepsi birden silinir.
   - O kaynaktan gelen **her** hasar (stack eklemese de) sayacı sıfırlar. Hasar olayı noktası: `DamageTypeSystem::ApplyStatusEffects` öncesi ya da `CombatRuntime` hasar çözümü — Luna mevcut sahiplik kurallarına uygun olanı seçer.
   - Kapsam K2'ye bağlı (yalnız oyuncu silahları mı, tüm Electric/Cryo mu). `damage_status_balance.json`'a `inactivitySeconds` alanı.
2. **AttackSpeed ritmi:** `OwnerAttackSpeedPercentage` modu Fighter dışındaki mermi silahlarına da açılır (bugün testler bunu yasaklıyor: `PrimaryWeaponMagazineTests` "non-fighter weapons do NOT have … OwnerAttackSpeedPercentage"). Dual bu moda geçer; Faz 1'deki geçici 0,12 katsayısı **kaldırılır**.

**Kabul:** Hedefli test: Electric/Cryo stack'leri 5,9 s'de duruyor, 6 s sessizlikten sonra hepsi siliniyor, araya giren normal vuruş sayacı sıfırlıyor. Kapsam dışı status'lar (Kinetic/Thermal) eski kuralla çalışıyor.

### Adım 3 — Rapid Laser (Fighter)

Yalnız veri: `Common.Damage` 12 → **10**, seviye başı +5 (aynı), AP katsayısı 0,40 → **0,41**, seviye başı AP katsayı artışı (+0,05) **kaldırılır**. Empowered bonus, şarjör, kritik ve yuvarlama aynı. K1 "isabet" ise trigger `EverySuccessfulHits`.

**Kabul:** L1 Fighter normal vuruş ⌈25,99⌉ = 26; L2 silah + L2 gemi değerleri formülle aynı.

### Adım 4 — Rapid Shotgun: anlık koni (yeni tip)

1. Yeni tip `PrimaryWeaponType::ConeBlast` (`PrimaryWeapon.Cone.Blast`), yeni şema `PrimaryWeapon.Cone.{Range, HalfAngleDegrees}` ve işleyici `ConeBlastWeaponHandler`:
   - Tetik başına, burun yönünde `HalfAngleDegrees` (K3: 22,5°) ve `Range` (400) içindeki her geçerli hedef **tam bir kez** hasar alır; hedef sayısıyla hasar düşmez.
   - Aday sorgusu `GetActorsInBounds` + kesin açı/mesafe testi (hedefin sınır kutusu koniyle kesişiyorsa vurulur). Duvar/statik engel kuralı ışınla aynı (`FindFirstStaticGeometryHit`).
   - Görsel: kısa ömürlü koni flaşı aktörü (yeni `ConeBlastVisualActor`), mermi yok.
2. `RapidShotgun` kaydı yeni tipe taşınır: `PelletCount`, `SpreadAngle`, `DamageReductionPerAdditionalHit`, `MinimumDamageMultiplier`, mermi hızı/ömrü kalkar. Hasar tipi Thermal → **Photonic**. Ateş 2,5 → **1,75**. AttackSpeed katsayısı 0,5 kalır (tasarımda yok; K8).
3. Empowered: `EverySuccessfulShots = 3`; `BonusDamage` taban 0 + `scalingRules` EP×0,15 ve **MaxHealth×0,03**. Ignite: K3'e göre — öneri empowered vuruşta hedef başına +1 Ignite (`statusStacksOnlyWhenEmpowered`). Not: Ignite Thermal etiketine bağlı; Photonic silahın Ignite uygulayabilmesi için payload'ın tip kapısı bu silah için açılmalı (Luna: `DamageTypeSystem::BuildPayload` tip kapısını genişletmeden, empowered vuruşa ek bir `Damage.Type.Thermal` etiketi eklemek daha az riskli — tasarım kararını rapora yaz).
4. **Düşman `EnemyTwinBladeScatter` pellet tipinde kalır**; pellet işleyicisi ve testleri değişmez.

**Kabul:** Hedefli E2E: koni içindeki 3 hedef her biri 65 alır (L1 Breacher), koni dışındaki (açı/menzil) hedef almaz, duvar arkasındaki almaz; 3. tetik 78,8 (+Ignite K3). Düşman scatter testleri aynen geçer.

### Adım 5 — Dual Kinetic Blaster

Veri: `Common.Damage` 3,5 → **3**, seviye başı +2; AP 0,45 → **0,20** + seviye başı +0,02; ateş 12 → **7**, `cadenceMode: OwnerAttackSpeedPercentage`; menzil 650; **şarjör 64**, reload K5 (öneri 1,5 s). Eski seviye kuralları (L2 +1,5 ateş, L3 +1 hasar, L4 +100 menzil +1 ateş) **kaldırılır** (K7).
Empowered: trigger K1 (öneri isabet), `EverySuccessful… = 8`; `BonusDamage` taban 5 + seviye başı 2 + AP×0,08 + EP×0,25 + seviye başı EP +0,04; `Damage.KineticStacks = 1`, `statusStacksOnlyWhenEmpowered = true` (normal vuruş ArmorPen **uygulamaz**, mevcut stack'lerin ArmorPen'i ise her Kinetic vuruşa uygulanır — tasarımdaki "normal hits do not apply ArmorPen" ifadesi "stack eklemez" olarak okunur; K9).

**Kabul:** L1 Interceptor: normal 11,2, empowered 26,48; ateş 7 × 1,01 = 7,07/s; L5 gemi AS %5 → 7,35/s; 8. (isabet/atış) sonra hedefte 1 Kinetic stack; normal vuruşlar stack eklemiyor.

### Adım 6 — Electric Launcher

1. **Uyarlamalı ölçekleme:** yeni `scalingRules` operasyonu ya da silah alanı `adaptiveStatScaling { primary: [AP, EP], dominantCoefficient, secondaryCoefficient, perLevelDominant, perLevelSecondary }` — hangisi mevcut `AttributeScalingRule` çözümleyicisine daha az müdahaleyse (Luna gerekçesini rapora yazar). Eşitlikte (AP = EP) baskın = EP.
2. Veri: `Common.Damage` 15 → **20** + seviye başı 2; AP 0,85 sabit katsayı **kalkar**, uyarlamalı kural gelir; ateş 2,8 → **2**; zincir değerleri K8 (öneri mevcut: 3 zincir, 250 menzil, zincir başı ×0,72). Eski seviye kuralları kalkar (K7).
3. Empowered: trigger `AbilityCastCharges`, `chargesPerCast = 2`, `chargedFireRate = 4`; PrimaryFire yuvasından gelen aktivasyon **sayılmaz**; yeni kullanım şarjı 2'ye **yeniler**. `BonusDamage` taban 3 + seviye başı 1 + EP×0,08 + seviye başı EP +0,02. `Damage.ElectricStacks = 1`, `statusStacksOnlyWhenEmpowered = true`; empowered atışın zincir hedefleri de +1 Electric alır, normal atışın zincirleri almaz.

**Kabul:** L1 Conductor: normal 39,1; yetenek sonrası 2 atış 45,38, 0,5 s içinde; üçüncü atış normal ve 2/s; ikinci yetenek kullanımı şarjı 2'ye yeniliyor (4 değil); doğrudan hedef ve zincir hedefleri 1 Electric. AP > EP olan sahip (ör. Fighter) için baskın = AP.

### Adım 7 — Continuous Heat Laser

1. Veri: `Common.Damage` 28 → **13** + seviye başı 2; AP 0,75 **kalkar**; EP 0,5 → **0,19** + seviye başı +0,02. Eski kurallar (L2 +6, L3 +150 menzil, L4 +0,2 max çarpan) kalkar (K7). Menzil 950, genişlik 26, heat gain/capacity/dissipation/overheat **aynı** (K4).
2. **Isı bölgeleri** (doğrusal çarpanın yerine): `heatDamageRegions` listesi (`[{ upToPercent: 35, multiplier: 1.0 }, { upToPercent: 75, multiplier: 1.15 }, { upToPercent: 100, multiplier: 1.40, damageTags: ["Damage.Type.Energy"] }]`). Son bölge Energy etiketiyle kalkana ×1,5 (`energyShieldDamageMultiplier`); altındaki bölgelerin hasar tipi K4 (öneri Photonic). `DamageMultiplierAtMaxHeat` niteliği bu silahtan kalkar.
3. Tick 0,25 s ve hedef başına biriktirme mevcut yapıda kalır.

**Kabul:** L1 Aegis tick: 21,17 (düşük), 24,35 (orta), 29,64 (yüksek; kalkana 44,46); bölge geçişleri %35 ve %75'te; hasar tipi yüksek bölgede Energy, altında K4.

### Adım 8 — Cryo Projector: Frost Meter ve Whiteout

1. Yeni özellik `PrimaryWeaponFeatureType::FrostMeter` (Heat gibi `features/FrostMeterWeaponFeatureHandler`):
   - `MaxFrostMeter = 4`. Normal dalga başına: 2+ farklı düşmana isabet → +1; dalga gelmeden önce Cryo stack taşıyan en az bir hedefe isabet → +1; dalga başına en fazla +2.
   - Meter 4 → **Whiteout** 4 s: ateş hızı ×2, sıra **Empowered → Normal → …** (empowered ile başlar), Whiteout'ta meter dolmaz.
   - Whiteout bitince meter 0, **3 s lockout** (dolmaz).
   - Durum `PrimaryWeaponRuntimeState`'te; HUD göstergesi bu fazın kapsamı dışında (K10), ama değerler sorgulanabilir olmalı.
2. Dalga aktörü dalga başına isabet sonuçlarını (farklı hedef sayısı, "önceden Cryo'lu" hedef var mı) özelliğe bildirir. "Önceden" = dalganın **ilk temasından önceki** durum.
3. Empowered dalga: hedef başına +1 Cryo (`statusStacksOnlyWhenEmpowered`), menzil +250, genişlik K6 (öneri ×1,5: başlangıç 80→120, en fazla 260→390), `BonusDamage` taban 5 + seviye başı 2 + EP×0,07 + seviye başı EP +0,01; vuruştan **önce** 4 Cryo taşıyan hedefe son hasar ×1,15; Cryo stack'leri tüketilmez.
4. Veri: `Common.Damage` 7 → **20** + seviye başı 2; AP 0,75 → **0,30** + seviye başı +0,02; ateş 1,8 → **2,5**; `Damage.Cryo.BuildupPerHit` normal dalgada 0 (yalnız empowered); eski kurallar kalkar (K7).
5. **Düşman `EnemyHexagonCryoPulse` dalga tipinde, özelliksiz kalır**; davranışı değişmez.

**Kabul:** Hedefli test senaryoları: tek hedef → 0; iki hedef → +1; ikisi + biri Cryo'lu → +2; 4'e ulaşınca Whiteout, 4 s'de 20 dalga (10 empowered) taban hızda; Whiteout'ta meter sabit; bitince 0 ve 3 s kilit; empowered L1 35,9, 4 stack'li hedefe 41,3; hexagon düşmanı aynen.

### Adım 9 — Testler ve regresyon

1. Her adım kendi hedefli testini getirir (yeni ya da mevcut aile testleri: `PrimaryWeaponMagazineTests`, `ContinuousBeamWallE2E`, `EnemyCombatFoundationTests`'teki düşman silahları). L1 değerleri Bölüm 1 tablosundan **sayısal** kontrol edilir.
2. `GasLiteCoreTests` ve `PrimaryWeaponMagazineTests` içinde **değiştirilen silahların katsayılarını** doğrudan kontrol eden satırlar (ör. Dual 0,12 / 0,45; Laser 0,40; shotgun pellet; "non-fighter weapons do NOT have OwnerAttackSpeedPercentage") bu işin konusudur ve yeni değerlere güncellenir. Bunun dışındaki eski beklentiler **düzeltilmez, listelenir** (kullanıcı kuralı).
3. Modül uyumu: bir `Common.Damage` modülü takılıyken her yeni silahın normal vuruşu modül oranında artıyor (en az bir silah için).
4. Düşman regresyonu: TwinBlade pellet, Hexagon dalga, Vanguard mermi ve Ironclad minigun davranışları değişmedi.

### Adım 10 — Doküman ve kapanış

- Tasarım dokümanının başına "Faz 2 uygulandı, tarih/commit" notu ve kapanan kararların (K1–K10) sonuçları.
- `docs/PROJECT_DOCUMENTATION.md` silah bölümü; bu planın sonuna "Uygulama sonucu" (her adımın commit'i, komut ve sonuç).

---

## 3. Build ve test

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set VSLANG=1033
cd /d D:\LightYears
cmake -S . -B build
cmake --build build -- -k 0
cd build
ctest -j1
```

- Log'lar `.tmp/logs/` altına. Tam paket seri koşar (43 test, ~45 s); kararlılık için ayrıca bir kez `ctest -j4`.
- Her adım sonunda commit (branch `codex/luna-callback-lifetime-checkpoint`), push yok.

## 4. Sıra ve bağımlılık

1 → 2 ortak altyapı (her silah buna dayanır). 3 (Laser) en küçük adım, altyapıyı doğrular. 4–8 birbirinden bağımsız; önerilen sıra 5 (Dual: altyapıyı en çok kullanır), 6 (Electric), 4 (koni: yeni tip), 7 (Heat), 8 (Cryo: en büyük). 9 her adımda sürekli, 10 en son.

## 5. Bilinen riskler

1. **Status kuralı kapsamı (K2):** Electric/Cryo yaşam kuralı global yapılırsa ChainLightning, IonStorm, CryoBola, FrostMaelstrom gibi yeteneklerin ve Hexagon düşmanının dengesi değişir.
2. **Isabet sayacı (K1):** isabet tabanlı empowered, ıskalayan oyuncu için daha seyrek empowered demek; Rapid Laser'ın bugünkü hissi değişir.
3. **Koni ve tip kapısı:** Photonic koninin Ignite uygulaması, status'ların hasar tipine kapılı yapısıyla çelişir (Adım 4.3).
4. **Hasar düşüşleri:** yeni L1 değerleri bugünkünden belirgin farklı (ör. Interceptor L1 Dual ~263 → ~93 DPS; Aegis L1 Heat Laser bugün ~68–119 DPS (doğrusal ısı ×1→×1,75), yeni ~85–119 DPS). Bu tasarımın istediği sonuç; oyun hissi kullanıcı tarafından denenmeli.
5. **Modül etkileşimi:** empowered bonus ve yeni ölçeklemeler modül modifier'larıyla nasıl birleşiyor (önce/sonra) Faz 1 sonrası açık karar listesinde zaten var; mevcut davranış korunur.

---

## 6. Kullanıcı kararları — kapandı (2026-10-02)

Kullanıcı "devam" diyerek tüm önerileri onayladı. "Öneri" sütunu artık bağlayıcı karardır; plan metnindeki "öneri" ifadeleri bu kararlara göre okunur.

| # | Karar | Karar (onaylı) |
|---|---|---|
| **K1** | Rapid Laser (her 6) ve Dual (her 8) empowered sayacı **atış** mı **isabet** mi? Tasarım "isabet" diyor; kod bugün atış sayıyor. | **İsabet** (atış başına en fazla 1). Dual'ın kimliği "hedefte kalmak". |
| **K2** | Electric/Cryo "6 s sessizlikte hepsi silinir" kuralı yalnız oyuncu silahlarına mı, tüm Electric/Cryo kaynaklarına (yetenekler, düşmanlar) mı? | **Global** (tek status kuralı); yetenek dengesi sonra gözden geçirilir. |
| **K3** | Koni açısı ve shotgun Ignite (önceden "öneri" olarak dokümanda). | 45° toplam (±22,5°), 400 menzil; Ignite yalnız empowered'da hedef başına +1. |
| **K4** | Heat Laser: %75 altı hasar tipi ve mevcut ısı mekaniği (gain 38/s, kapasite 100, soğuma 25/s, aşırı ısınma 2,5 s). | Altı **Photonic**; ısı mekaniği aynen. |
| **K5** | Dual şarjör 64'ün reload süresi (tasarımda yok). | **1,5 s** (7/s ile ~9 s'lik şarjör). |
| **K6** | Cryo empowered dalga boyutu ("daha büyük", sayı yok). | Genişlik **×1,5** (80→120, 260→390); menzil +250 (780→1030). |
| **K7** | Eski seviye "milestone" kuralları (Electric L3 +1 zincir, Heat L3 +150 menzil, Cryo L2 +30 genişlik vb.) | **Kaldır**; tek kaynak tasarım formülleri. |
| **K8** | Tasarımda olmayan sayılar: Electric zincir (3 / 250 / ×0,72), Shotgun AttackSpeed katsayısı 0,5 | **Mevcut değerler korunur.** |
| **K9** | Dual "normal hits do not apply ArmorPen": normal vuruş mevcut stack'lerin ArmorPen'inden yararlanmasın mı? | Normal vuruş **stack eklemez ama mevcut ArmorPen'den yararlanır** (bugünkü status davranışı; aksi tasarımı anlamsızlaştırır). |
| **K10** | Frost Meter / Electric şarj HUD göstergesi | Bu fazda **yok**; değerler runtime'da sorgulanabilir, HUD ayrı iş. |
