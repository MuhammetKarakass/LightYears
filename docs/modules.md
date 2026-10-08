# Modules

**Tarih:** 2026-10-05 · **Amaç:** Oyunda veri olarak tanımlı modülleri ve tasarım aşamasındaki modül fikirlerini tek yerde izlemek. · **Durum:** Canlı oyun verisi ile planlar ayrılmıştır.

Modül kurallarının sahibi [Modül Sistemi Tasarımı](MODULE_SYSTEM_DESIGN.md); 11 standart Common modülün tam değer ve davranış sözleşmesinin sahibi [Common uygulama planı §2](MODULE_COMMON_IMPLEMENTATION_PLAN.md#2-modül-şartnamesi), çalıştırılan içerik kaynağı [modules.json](../LightYearsGame/assets/content/data/modules.json)'dır. Bu katalog fikir/status izleme ve hızlı bakış içindir.

## Durum etiketleri

| Durum | Anlamı |
|---|---|
| **Implemented · Common** | Standart Common içeriğinde ve uygulama planında yer alıyor. |
| **System · default** | JSON'da geminin takılı sistem modülü olarak tanımlı. |
| **System · variant** | JSON'da sistem modülü varyantı olarak tanımlı. Kod/veri kaydı bulunması, ödül havuzunda edinilebilir olduğunu tek başına göstermez. |
| **Defined · review** | JSON tanımı var; güncel Common setinin dışında. Edinilebilirlik ve güncel denge ayrıca doğrulanmadı. |
| **Planned** | Fikir/modül yönü listeye alındı; değer, şekil, nadirlik veya runtime uygulaması hâlâ açık olabilir. Oyun içi değildir. |
| **Reworking** | Beğenilen bir fikir var; kullanıcı geri bildirimiyle tetik ya da davranış yeniden tarif ediliyor. Oyun içi değildir. |
| **Review** | Önceki öneri; kullanıcıdan seçim veya düzeltme gelmedi. Oyun içi değildir. |
| **Rejected** | Kullanıcı öneriyi reddetti; kapsam dışı tutulur. |

`■■` iki hücreli yatay domino, `■■■` üç hücreli yatay I, `■` tek hücredir. L şekli iki satırda gösterilir. Mevcut veri şekilleri 90° döndürülebilir. Planlarda şekil belirtilmemişse `Açık` yazar. Önerilerde belirtilen sayılar kullanıcının önerisi veya ilk fikir taslağıdır; durum hücresi uygulama ya da nihai denge onayı anlamına gelmez.

## Oyunda tanımlı modüller

`modules.json` içinde **34 kayıt** vardır: 11 varsayılan sistem, 3 sistem varyantı, 11 standart Common ve Common seti dışında 9 kayıt. Aşağıdaki “tanımlı” statüsü JSON'daki varlığı gösterir; dağıtım/loot havuzunda çıkma kanıtı değildir.

### Varsayılan sistemler · 11

| Modül | Şekil | Aile / sistem rolü | Durum |
|---|---:|---|---|
| Standard Shield Generator | `■■` | Defense · Shield | System · default |
| Standard Afterburner | `■■` | Movement · Afterburner | System · default |
| Health Readout | `■` | Utility · HealthReadout | System · default |
| Damage Readout | `■` | Utility · DamageReadout | System · default |
| Life Support Unit | `■■` | Utility · LifeSupport | System · default |
| Flight Control Unit | `■■` | Movement · FlightControl | System · default |
| Primary Weapon | `■■` | Primary · PrimaryWeapon | System · default |
| Q Ability Slot | `■■` | Utility · AbilitySlot1 | System · default |
| E Ability Slot | `■■` | Utility · AbilitySlot2 | System · default |
| F Ability Slot | `■■` | Utility · AbilitySlot3 | System · default |
| R Ability Slot | `■■` | Utility · AbilitySlot4 | System · default |

### Sistem varyantları · 3

| Modül | Şekil | Etki | Durum |
|---|---:|---|---|
| Fast-Charge Generator | `■■` | Shield capacity ×0.80; shield regeneration ×1.50 | System · variant |
| Heavy Generator | `■■■` | Shield capacity +60 | System · variant |
| Extended Afterburner | `L` · 3 hücre | Afterburner capacity +50 | System · variant |

### Standart Common · 11

| Modül | Aile | Şekil | Etki | Durum |
|---|---|---:|---|---|
| Heavy Armor Plate | Defense | `■■■` | Armor +30 | Implemented · Common |
| Assault Core | Offense | `■■` | Attack Power +15 | Implemented · Common |
| Focused Barrel | Primary | `■■` | Primary weapon damage ×1.10 | Implemented · Common |
| Thrust Booster | Movement | `■■` | Horizontal and vertical movement rating +2.2 | Implemented · Common |
| Attack Rhythm | Offense | `■■` | On Offense use, Attack Speed +15 for 3 sec; does not stack, duration refreshes | Implemented · Common |
| Shatter Current | Defense | `■■` | When the shield breaks, Primary hits deal additional Energy damage equal to 30% of hit damage for 5 sec | Implemented · Common |
| Energy Feedback | Offense | `■` | On dealing Energy damage, Energy Power +16 for 4 sec; does not stack, duration refreshes | Implemented · Common |
| Escape Reflex | Movement | `■■` | On taking damage, movement speed ×1.25 for 2 sec; does not stack, duration refreshes | Implemented · Common |
| Finishing Blow | Offense | `■■` | Damage you deal ×1.20 while the target is at 25% health or below | Implemented · Common |
| Zone Cooler | Utility | `■■■` | In a single-family zone, that family's ability cooldown ×0.79; in Hull, active non-Primary abilities ×0.90; no bonus in Nose | Implemented · Common |
| Full Magazine | Primary | `■■` | The first 3 successful Primary shots in each magazine deal ×1.30 damage | Implemented · Common |

Standart Common listesinde Energy Coil yerine Assault Core bulunur. **Energy Coil JSON tanımında korunmuştur**; aşağıdaki Common dışı tanımlar bölümündedir.

### Common dışı JSON tanımları · 9

Bu tanımlar içerik dosyasında duruyor; 11 standart Common listesine dahil değiller. Şeklin satır dizilimi özgün hücre koordinatlarına göre gösterilir.

| Modül | Aile | Şekil | JSON etkisi | Durum |
|---|---|---:|---|---|
| Piercing Tip | Primary | `■■` | Primary projectile Piercing Count +1 | Defined · review |
| Shot Accelerator | Primary | Domino (2) / L (3) | 2 cells: fire rate ×1.10; 3-cell L: ×1.14 | Defined · review |
| Attack Matrix | Offense | Domino (2) / I (3) | 2 cells: ability damage ×1.12; 3-cell I: ×1.16 | Defined · review |
| Overload Core | Offense | `■■`<br>`■■` | Ability damage ×1.25; cooldown ×1.10 | Defined · review |
| Hull Reinforcement | Defense | Domino (2) / L (3) | 2 cells: max health +40; 3-cell L: +55 | Defined · review |
| Maneuver Thruster | Movement | `■` | Horizontal movement rating +0.36 | Defined · review |
| Cooling Circuit | Utility | Domino (2) / L (3) | 2 cells: Ability Haste +8; 3-cell L: +11 | Defined · review |
| Energy Coil | Utility | `■■` | Energy Power +24 | Defined · review |
| Targeting Computer | Utility | Single (1) / domino (2) | 1 cell: Critical Chance +3; 2 cells: +5 | Defined · review |

## Tasarım kuyruğu

Aşağıdaki kayıtlar kullanıcı geri bildirimini ve şu anki tasarım durumunu tutar. P2 ve P3 kapsamındaki dilimler `modules.json` ve runtime'a eklendi; diğer kayıtlar tasarım kuyruğudur.

### Planned / Reworking modül havuzu

Kullanıcının beğendiği fikirler, ilettiği taslaklar ve 2026-10-05/06 toparlama turlarının sonucu aynı havuzda tutulur. P2/P3 runtime dilimleri satırlarda belirtilir; kalan özellikler ve modüller henüz oyun içi değildir. Uygulama planı: [MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md). Bu ilk sürümdür; değerler tablo biçimli ve genişletilebilir, seviye sayısı, yan özellik ağırlıkları ve yükseltme harcaması henüz karar değildir. Uncommon set **18 modül**: 7 tek boyutlu, 11 boyut merdivenli. Rare 6 modül.

**Kurallar (kullanıcı, 2026-10-05/06):**
- Uncommon yeni varlık (mermi, mayın vb.) üretebilir; ama az üretir ve az vurur.
- Düz stat modülü olabilir; değeri nadirliğine uygun olmalı. Sayılar Common ×1,3 ölçütüyle yazılmış **öneridir**, onaylı denge değildir.
- Modüller gelişecek. Gelişim sütunu, seviye başına artan parametreyi ve örnek 3 adımlık değerleri gösterir. Seviye sayısı ve formül karara bağlanmadı.
- Üretilen mermi, patlama ve sekme başka modülü tetiklemez. Hasar sırası: modül çarpanları, sonra kritik.
- Birimler: kritik şansı yüzde puanıdır (Targeting Computer +3/+5). Kritik hasar çarpanı `Owner.CriticalDamage`, tabanı 1,5. Attack Speed rating'i yaklaşık %1 atış hızıdır (`GDD.md`). Hız tavanı gemiye göre 480–590 birim/sn.
- Ana özelliklere denge amaçlı üst sınır konmaz (kullanıcı, 2026-10-06); başlangıç değerleri küçük tutulur, güç gelişimle artar. Mekanik/teknik sınırlar (Electric stack üst sınırı, kalkanın maksimumu, performans için mayın sayısı) bundan ayrıdır.
- Bazı modüller tek boyutlu (yalnız 1, yalnız 2 veya yalnız 3 hücre) ve yan özellik taşımaz; yan özellik yalnız boyut merdiveni olan modüllerde bulunur.

#### Boyut merdiveni ve yan özellik sistemi (tasarım yönü, kurallar öneri)

- **Merdivenler:** 1-2-3, 1-2, 1-3, 2-3. Boyut hem yükseltmeyle hem loot'ta rastgele değişir.
- **Ana özellik (A) sabittir.** Boyutla sayısal olarak büyüyebilir; belirtilmeyen boyutlarda aynı kalır.
- **Yan özellik, modülün en büyük boyutunda açılır.** Gücü, o boyuta eklenen hücre sayısıyla ölçeklenir.
- **Yan özellik rastgeledir, hep seçilen bir şey değildir:** bir loot'ta A modülü B yan özellikle, başka loot'ta C yan özellikle düşebilir. Her modülün 5 olası yan özelliği vardır: 2 kendine özel + ortak havuzlardan 3.
- **Düşerken** yan özellik atanır ve sabittir; küçük boyutta düşse bile kart "kilitli" gösterir, büyütünce açılır. **Yükseltmede** özel bir harcamayla havuzdan 3 rastgele teklif görülüp biri seçilebilir (öneri; harcama biçimi açık).
- Common'da yan özellik yoktur; Rare'de iki yan özellik olabilir (öneri). Yan özelliklerin güç düzeyi eşit tutulur.
- Kayıt: modül kimliği, boyut seviyesi ve yan özellik kimliği saklanır; yeniden yüklemede değişmez.
- **Yan özelliklerin gelişimi (öneri, karar bekliyor):** modül seviyesiyle yan özellik de büyür, ana özelliğin yaklaşık yarı hızında. Ortak havuz özelliklerinin 3 adımlık değerleri bir kez tanımlanır; özel yan özelliklerin değerleri modül başına yazılır.

#### Ortak yan özellik havuzları (öneri değerler)

| Havuz | Kimlik | Özellik |
|---|---|---|
| Offense (O) | O1 | Attack Power +8 |
| | O2 | Critical Chance +8% |
| | O3 | On kill, Attack Speed +10 for 2 sec (does not stack; duration refreshes) |
| | O4 | Your applied status durations +1 sec |
| | O5 | Your first hit against each target ×1.15 |
| Defense (D) | D1 | MaxHealth +25 |
| | D2 | On taking damage, Armor +12 for 2 sec (does not stack; duration refreshes) |
| | D3 | On kill, restore health equal to 1% of MaxHealth |
| | D4 | Shield regeneration rate ×1.15 |
| | D5 | Max Shield +8% |
| General (G) | G1 | On kill, +1 Gold |
| | G2 | Ability Haste +4 |
| | G3 | Afterburner capacity +10% |
| | G4 | Horizontal and vertical movement rating +0.8 |
| | G5 | Luck +1 |

#### Uncommon · tek boyutlu (7, yan özellik yok)

| Modül | Aile · boyut | Kart metni ve kural | Gelişim (öneri) | Durum |
|---|---|---|---|---|
| Photonic Lens | Primary · `■` | *You deal more damage to targets without a status effect.* All damage you deal to a target with no status effects ×1.15. Status is checked before each hit. | ×1.15 → ×1.18 → ×1.21 | P4 runtime · tüm hasar, vuruş öncesi status kontrolü (kullanıcı, 2026-10-07) |
| Reflection Shield | Defense · `■■` | *Your shield reflects attacks after a Defense ability.* When a Defense ability is used, for 5 sec, 35% of the damage absorbed by your shield is reflected as Energy damage to the nearest enemy; cannot critically hit; inactive while no shield is available; duration refreshes on reuse. Reflected damage does not trigger other modules, including Energy Feedback (user, 2026-10-07). | 35% → 40% → 45% | P4 runtime · kullanıcı 5 sn Defense penceresini ve zincirleme korumasını istedi |
| Health Bridge | Defense · `■■` · komşuluk | *Your health becomes strength for adjacent abilities.* Each direct damage hit from up to 2 adjacent Q/E/F/R slot modules deals additional damage equal to 3% of MaxHealth (at 250 health, +7.5 per hit). No effect without an adjacent module. | 3% → 3.5% → 4% | P4 runtime · yeteneğin kendi hasar türü; DoT tick'leri hariç (kullanıcı, 2026-10-07) |
| Lucky Cast | Utility · `■■` | *Your abilities sometimes refresh instantly.* Luck +1. When a Q/E/F/R ability is used, Luck × 5% chance to reduce that ability's cooldown by 50%; the chance naturally stops at 100% (no separate cap). Excluding Primary, each use rolls separately. | 5% → 6% → 7% / Luck | P3 runtime · kullanıcı onayladı · %50 tavanı 2026-10-06'da kaldırıldı; Luck seviye başına +1 büyüdüğü için ihtimal ilerledikçe %100'e ulaşabilir, değerler gelişimle ayarlanacak |
| Frost Lock | Utility · `■■` | *Targets freeze at 4 Cryo stacks.* A target reaching 4 Cryo stacks freezes for 1 sec and loses its stacks (slow is removed); 8 sec cooldown per target. | Freeze 1.0 → 1.3 → 1.6 sec; cooldown 8 → 7 → 6 sec | Planned · kullanıcı onayladı · donma yalnız hareketi mi saldırıyı da mı kesiyor, boss/elit direnci: açık |
| Arc Extender | Offense · `■■■` | *Electric can reach 5 stacks.* Your applied Electric stack limit increases from 4 to 5. Incoming damage bonus +3%/6%/9%/**13%/19%** (previously, the 4th stack gave +16%). Duration and refresh rules are unchanged. | 5th-stack bonus +19% → +21% → +23% | Planned · kullanıcı onayladı |
| Double Feed Core | Utility · `■■■` | *Grants Attack Power and Energy Power.* Attack Power +15, Energy Power +23. | +15/+23 → +18/+28 → +21/+33 | Reworking · sayı (taslak +12/+18 Common seviyesindeydi) |

#### Uncommon · merdivenli (11, yan özellikli)

Sütunlar: A boyuta göre; özel yan özellikler (2); ortak havuzdan uygun olanlar (3). Yan özellik her satırda en büyük boyutta açılır. Her modülün olası yan özellik sayısı 5.

| Modül | Aile · merdiven | A: ana özellik | Özel yan özellikler | Ortak havuzdan | Gelişim (A, öneri) | Durum |
|---|---|---|---|---|---|---|
| Glass Cannon Core | Offense · 1-3 | *Attack Power rises, hull health falls.* 1c: AP +12, MaxHealth −6% · 3c: AP +34, MaxHealth −16% (after other bonuses) | Critical Chance +10% while health ≤50% · half the MaxHealth penalty returns as Max Shield +6% | O2, O4, O5 | AP +17% / level; penalty is fixed | Planned · kullanıcı onayladı · takılınca can oranı korunur mu: açık |
| Loot Shield | Defense · 1-2-3 | *Kills replenish your shield.* On each kill, gain shield equal to 1c: 2% of Max Shield; 2c/3c: 3%; cannot exceed maximum | On kill, restore 12% of Afterburner capacity *(Hunt Fuel)* · convert 50% of overflow shield into health | D1, D2, D4 | 1c 2% → 2.5% → 3%; 2c/3c 3% → 4% → 5% | P3 runtime · kullanıcı beğendi |
| Sharp Scale | Offense · 2-3 | *Critical hits hit harder and happen more often.* Critical Damage +0.20 (1.5 → 1.7), Critical Chance +15% (user requested +15–20%) | On critical hit, AS +15 for 2 sec *(Critical Rhythm)* · Critical Chance +2% per point of total Luck *(Luck Lens)* | O1, O3, O5 | Critical Damage +0.20 → +0.25 → +0.30 | P4 runtime · Critical Rhythm ve Luck Lens dahil; denge oynayınca ayarlanır |
| Scatter Core | Offense · 2-3 | *The ability fires 3 small guided missiles.* When an Offense ability is used, fire 3 guided missiles at the 3 nearest different enemies; each deals 15% of the triggering ability's first hit (45% total). Does not apply status or critically hit | Once per use, 0.8 sec after the first hit, a small-area explosion deals 25% of that hit *(Aftershock)* · 4th missile | O1, O3, O4 | 15% → 18% → 21% per missile | Planned · fikir beğenildi · <3 hedefte mermi nereye, mermi hızı/ömrü, patlama yarıçapı: açık |
| Thermal Shock Coil | Offense · 2-3 | *Deal more damage to burning and frozen targets.* While a target has at least 1 Thermal and 1 Cryo stack, all damage you deal to it ×1.22. Status is checked before the hit | Thermal and Cryo duration +2 sec on these targets · on their death, Thermal explosion equal to 3% of MaxHealth | O1, O2, O5 | ×1.22 → ×1.27 → ×1.32 | P4 ana runtime · süre yan özelliği status fazında; ölüm patlaması üretilen hasar fazında açık |
| Ricochet Tip | Primary · 2-3 | *A Primary hit bounces once.* A Primary hit bounces once to another nearby enemy for 25% damage; no chained bounces | The bounce transfers one of the target's statuses · second bounce deals 15% | O2, O4, O5 | 25% → 30% → 35% | Planned · önceki Rare adayı, zayıflatılıp Uncommon'a · menzil (öneri 250 birim); sürekli/çok hedefli silahlarda atış başına en çok bir sekme: açık |
| Cold Retort | Defense · 2-3 | *Slow enemies that hit you.* When you take damage, apply 1 Cryo stack to the 2 nearest enemies within 300 units; 2 sec cooldown | Enemies with a status effect deal ×0.88 damage to you *(Jarring Effect)* · enemies frozen by Cryo spread 1 Cryo stack to nearby enemies | D2, D3, D5 | Target count 2 → 3 → 4 | P4 runtime (A + Jarring Effect) · donma yayılımı status fazında açık |
| Grit Your Teeth | Defense · 2-3 | *Take less damage when you're in danger.* While hull health ≤35%, damage you take ×0.80; cannot revive from death | At the same threshold, +35 to whichever is higher: Attack Power or Energy Power (AP on a tie; use values before this bonus) *(Critical Circuit)* · shield regeneration ×2 at the same threshold | D1, D3, D4 | ×0.80 → ×0.77 → ×0.74 | P3 runtime (A + Critical Circuit; kalkan yenileme henüz yok) · kullanıcı onayladı |
| Hunt Clock | Utility · 2-3 | *Kills speed up abilities.* Each kill reduces pending cooldowns on active Q/E/F/R abilities by 0.25 sec; excludes Primary; cooldown cannot go below 0 | +1 Gold on kill *(Hunt Bounty)* · every 5 kills, reduce the cooldown of the ability with the longest cooldown by 20% | G2, G3, G5 | 0.25 → 0.30 → 0.35 sec | P3 runtime (Gold hariç) · saniyede 1 sn sınırı 2026-10-06'da kaldırıldı; yoğun kalabalıkta cooldown'lar çok hızlı düşebilir, değerler gelişimle ayarlanacak |
| Movement Rhythm | Movement · 2-3 | *The faster you move, the faster you fire.* Gain +3.5 Attack Speed rating per 100 units of current movement speed (~+17–21 at top speed; the ship's speed cap is the natural limit). 0 while stationary | Horizontal and vertical rating +2.9 *(Advanced Thrust)* · when speed drops, the AS bonus fades over 1.5 sec instead of disappearing immediately | G2, G3, G5 | 3.5 → 4.0 → 4.5 / 100 speed | P3 runtime (A + Advanced Thrust; sönme yan özelliği onaysız) · kullanıcı beğendi, hıza orantılı istedi · Afterburner dahil gerçek hızın tamamı sayılır (kullanıcı, 2026-10-07) · "Afterburner açıkken ×1,5" yan özelliği beğenilmedi, yerine "1,5 sn sönme" önerildi |
| Trail Mines | Movement · 2-3 | *Leave mines behind after a movement ability.* When a Movement ability is used, drop a mine for every 50 units traveled; up to 12 mines at once; each mine explodes in a small area after a 1 sec fuse; no mines are dropped while stationary. Low damage | Mines apply 1 Cryo stack · limit 12 → 16 and fuse 0.7 sec | G1, G3, G4 | Max mines 12 → 14 → 16; damage value open | Reworking · "4–5 sn" bırakma penceresi mi başlangıç gecikmesi mi, mayın hasarı: açık. 10 birimlik aralık ~200 mayın eder (hız tavanı 480–590 birim/sn); 50 önerildi |

#### Yan özellik olarak birleşenler (artık ayrı modül değil)

| Eski modül | Nereye girdi |
|---|---|
| Hunt Fuel | Loot Shield · private side feature |
| Hunt Bounty Processor | Hunt Clock · private side feature |
| Advanced Thrust Block | Movement Rhythm · private side feature |
| Critical Circuit | Grit Your Teeth · private side feature |
| Critical Rhythm | Sharp Scale · private side feature |
| Luck Lens | Sharp Scale · private side feature |
| Aftershock | Scatter Core · private side feature |
| Jarring Effect | Cold Retort · private side feature |

#### Rare (6)

| Modül | Aile · şekil | Kart metni ve kural | Gelişim | Durum |
|---|---|---|---|---|
| Ember Accumulation | Offense · `■■` | When Thermal reaches 4 stacks, the target explodes once instead of taking Ignite damage over time: 130% of that stack's total 5 sec DoT; stacks are cleared. | Açık | Planned · kullanıcı Rare için uygun buldu |
| Gold Plating | Defense · `■■` | +1 Armor per 20 Gold, up to ~+30 (draft +15; too low for Rare). When Gold is spent, the bonus is recalculated from current Gold. | Açık | Reworking · sayı; Gold oranı ekonomiye göre |
| Armor Conversion | Defense · `■■` | *Your Armor becomes Attack or Energy Power.* Add 40% of your Armor to whichever is higher: Attack Power or Energy Power (using values before this bonus). At 80 Armor, about +32. | 40% → 45% → 50% | Planned · kullanıcı esnek (AP/EP) ve Rare istedi |
| Shatter Shock | Defense · `■■` | *Your shield bursts when it breaks.* When the shield breaks, enemies within 250 units are pushed 150 units, take damage equal to 15% of MaxHealth, and gain 1 Cryo stack; 20% of max shield is restored immediately; 20 sec cooldown. Extra features (Cryo stacks, shield restoration) are intended for a higher tier and remain proposals. | Damage 15% → 19% → 23%; restore 20% → 25% → 30%; cooldown 20 → 17 → 14 sec | Planned · kullanıcı üst tier istedi · Rare/Epic ayrımı açık |
| Status Resonator | Utility · `■■` | *Your applied statuses last longer.* Status effects you apply last ×1.4 (Kinetic/Electric/Cryo 6 → 8.4 sec; Thermal 5 → 7 sec). Stacks, DPS, and effects are unchanged; total Ignite damage increases by ~40%. | ×1.4 → ×1.5 → ×1.6 | Planned · Uncommon'dan Rare'e taşındı (2026-10-06) |
| Energy Coupler | Offense · `■■` · komşuluk | *Part of an adjacent ability's damage becomes Energy damage.* Convert 25% of direct damage from an adjacent Q/E/F/R slot module to Energy damage; total damage stays the same. If adjacent to multiple slots, convert 25% for each; the effects do not stack. Synergizes with Energy Feedback. | 25% → 30% → 35% | Reworking · Uncommon'dan Rare'e taşındı · dönüşen kısım eski türün status'unu uygulamaya devam ediyor mu: açık |

#### Açık / ertelenen

| Modül | Durum |
|---|---|
| Heavy Hull Reinforcement | Defense · `■■■` Armor +50, movement −1/−1. Düz stat takası olduğundan Armor Conversion (Rare) ile birleştirilmesi öneri; karar yok. |
| Layered Defense | Defense · `■■` proposal: while shield > 0, Armor +35; bonus is removed when the shield breaks. Karar yok; savunma sayısı yeterli olduğundan çıkarılabilir. |
| Giant Slayer | Offense · `■■` proposal: all damage you deal to a target whose MaxHealth is at least twice your MaxHealth ×1.15 (progression ×1.15 → ×1.20 → ×1.25). Karar yok. |
| Circle Spirit | Offense · `■■` proposal: damage you deal increases by 2% for each enemy within 300 units, capped at 20% (progression 2% → 2.5% → 3%; cap 20% → 25% → 30%). Karar yok. |
| Chain Link | Utility · `■■` adjacency proposal: when one of two adjacent Q/E/F/R slots is used, reduce the other's cooldown by 0.5 sec (progression 0.5 → 0.7 → 0.9 sec). Karar yok. |
| Level Refresh | Defense · `■■` proposal: on level up, restore health equal to 25% of MaxHealth and 50% of Max Shield. Altı öldürmede seviye sistemi geçici test amaçlı olduğundan riskli; karar yok. |
| Blood Bond, Drift Armor | Defense yedek önerileri; karar yok. |
| Impact Tail | Effect undefined; awaiting a one-sentence description from the user. |
| Rapid Condenser | Ertelendi: charge'lı yetenek sayısı sayılacak; azsa ikinci bir etki gerekir ya da elenir. |

**Komşuluk kuralı:** Energy Coupler ve Health Bridge ortak kural kullanır: hücre kenarı paylaşan Q/E/F/R slot modülleri komşudur; bir domino en çok 2 slota değer. Primary slotu komşuluk dışıdır.

**Ortak sınır notu:** Scatter Core (Aftershock yan özelliğiyle) ve Attack Rhythm aynı "Offense kullanınca" tetiğindedir; Sharp Scale'in Critical Rhythm yan özelliği ile Attack Rhythm ikisi de zamanlı Attack Speed verir. Birlikte takılırsa tek yetenek şişer. Ortak sınır/bütçe kararı açık.

### Geri bildirimi bekleyen önceki fikirler

| Modül | Önceki taslak | Durum |
|---|---|---|
| Void Tip | After Offense, the first Primary shot deals area damage around its target | Review |
| Critical Handoff | A Primary critical hit reduces pending Offense cooldown | Review |
| Retaliation Sequence | On taking damage, fire guided missiles at the attacker | Review |
| Emergency Egress | On shield break, clear slows and grant brief immunity | Review |
| Momentum Battery | Charge accumulated based on Afterburner duration | Review |
| Hunter's Mark | Mark a target with consecutive Primary hits; consume the mark with Offense | Review |

### Reddedilen fikirler

| Modül | Durum |
|---|---|
| Magazine Residue | Rejected |
| Crossfeed | Rejected |
| Condenser Barrel | Rejected |
| Staggered Loading | Rejected |
| Recoil Engine | Rejected |
| Emergency Power Transfer | Rejected |
| Overflow Charge | Rejected |
| Fixed Position | Rejected |
| Impact Buffer | Rejected |
| Luck Core | Rejected (2026-10-05, "Luck +2" çok sıradan bulundu; yerine Lucky Cast) |
| Recovery Line | Rejected (2026-10-05, kullanıcı beğenmedi; yerine başka defansif modüller aranıyor) |
| Counterpush | Rejected (2026-10-05, kullanıcı beğenmedi) |
| Impact Shell | Rejected (2026-10-05, kullanıcı beğenmedi) |
| Last Breath Cell | Rejected (2026-10-05, kullanıcı beğenmedi) |
| Leaky Armor | Rejected (2026-10-05, Afterburner ile sürekli kullanılabildiği için fazla güçlü bulundu) |

## Kaynaklar ve sınırlar

- Çalıştırılan modül kayıtları: [modules.json](../LightYearsGame/assets/content/data/modules.json).
- Common etki değerleri ve doğrulama geçmişi: [Common uygulama planı](MODULE_COMMON_IMPLEMENTATION_PLAN.md).
- Genel modül yerleşimi/nadirlik ilkeleri: [Modül Sistemi Tasarımı](MODULE_SYSTEM_DESIGN.md).
- Planned/Reworking fikirleri, kullanıcının 2026-10-05 ilettiği taslaklar ve geri bildirimlerdir. Bunları oyun davranışı, loot, son değer veya kesin rarity olarak sunma.
