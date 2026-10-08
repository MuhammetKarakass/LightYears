# Modules

**Tarih:** 2026-10-08 · **Amaç:** Oyunda veri olarak tanımlı modülleri ve tasarım aşamasındaki modül fikirlerini tek yerde izlemek. · **Durum:** Canlı oyun verisi ile planlar ayrılmıştır; Uncommon ana özellikler ve P8 sonrası yedi eksik yan doğrulandı.

Modül kurallarının sahibi [Modül Sistemi Tasarımı](MODULE_SYSTEM_DESIGN.md); 11 standart Common modülün tam değer ve davranış sözleşmesinin sahibi [Common uygulama planı §2](MODULE_COMMON_IMPLEMENTATION_PLAN.md#2-modül-şartnamesi), çalıştırılan içerik kaynağı [modules.json](../LightYearsGame/assets/content/data/modules.json)'dır. Bu katalog fikir/status izleme ve hızlı bakış içindir.

## Durum etiketleri

| Durum | Anlamı |
|---|---|
| **Implemented · Common** | Standart Common içeriğinde ve uygulama planında yer alıyor. |
| **Implemented · Uncommon** | Ana özellik ve satırda belirtilen yan özellikler canlı içerikte; P8 sonrası yedi eksik yan dahil oyun build ve hedefli gerçek E2E ile doğrulandı; loot/ekonomi, nihai denge ve kullanıcı görsel kabulü anlamına gelmez. |
| **System · default** | JSON'da geminin takılı sistem modülü olarak tanımlı. |
| **System · variant** | JSON'da sistem modülü varyantı olarak tanımlı. Kod/veri kaydı bulunması, ödül havuzunda edinilebilir olduğunu tek başına göstermez. |
| **Defined · review** | JSON tanımı var; güncel Common setinin dışında. Edinilebilirlik ve güncel denge ayrıca doğrulanmadı. |
| **Planned** | Fikir/modül yönü listeye alındı; değer, şekil, nadirlik veya runtime uygulaması hâlâ açık olabilir. Oyun içi değildir. |
| **Reworking** | Beğenilen bir fikir var; kullanıcı geri bildirimiyle tetik ya da davranış yeniden tarif ediliyor. Oyun içi değildir. |
| **Review** | Önceki öneri; kullanıcıdan seçim veya düzeltme gelmedi. Oyun içi değildir. |
| **Rejected** | Kullanıcı öneriyi reddetti; kapsam dışı tutulur. |

`■■` iki hücreli yatay domino, `■■■` üç hücreli yatay I, `■` tek hücredir. L şekli iki satırda gösterilir. Mevcut veri şekilleri 90° döndürülebilir. Planlarda şekil belirtilmemişse `Açık` yazar. Önerilerde belirtilen sayılar kullanıcının önerisi veya ilk fikir taslağıdır; durum hücresi uygulama ya da nihai denge onayı anlamına gelmez.

## Oyunda tanımlı modüller

`modules.json` içinde **52 kayıt** vardır: 11 varsayılan sistem, 3 sistem varyantı, 11 standart Common, 9 eski Common dışı tanım ve 18 Uncommon. Uncommon ana/yan özellik sözleşmeleri aşağıdaki kendi tablolarındadır; güncel doğrulama [P8 sonrası sonuç §9](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md#9-p8-sonrası-eksik-yanlar--2026-10-08) içindedir. Aşağıdaki “tanımlı” statüsü JSON'daki varlığı gösterir; dağıtım/loot havuzunda çıkma kanıtı değildir.

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

Aşağıdaki kayıtlar kullanıcı geri bildirimini, uygulanan Uncommon içeriği ve kalan tasarım kuyruğunu birlikte tutar. 18 Uncommon ana modül P2–P6 ile runtime'a eklendi; P7 sunumu ve P8 kapanışı doğrulandı. Rare/Epic fikirleri ve belirtilen eksik yan özellikler tasarım aşamasındadır.

### İçerik ve tasarım havuzu

Kullanıcının beğendiği fikirler, ilettiği taslaklar ve 2026-10-05/06 toparlama turlarının sonucu aynı havuzda tutulur. Uygulanan ana/yan özellikler ve kalan eksikler satırlarda belirtilir. P8 mevcut runtime diliminin kapanışıdır; tüm tasarım önerilerinin uygulandığı anlamına gelmez. Uygulama planı: [MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md). Bu ilk sürümdür; değerler tablo biçimli ve genişletilebilir, seviye sayısı, yan özellik ağırlıkları ve yükseltme harcaması henüz karar değildir. Uncommon set **18 modül**: 7 tek boyutlu, 11 boyut merdivenli. Rare 23 modül; Epic/Legendary adayı 7.

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
- **Yan özellik rastgeledir, hep seçilen bir şey değildir:** bir loot'ta A modülü B yan özellikle, başka loot'ta C yan özellikle düşebilir. Tasarım hedefi her merdivenli modül için 5 olası yan özelliktir: 2 kendine özel + ortak havuzlardan 3. Mevcut izin listelerinde her merdivenli Uncommon için 2 özel + 3 ortak olmak üzere 5 seçenek vardır.
- **Düşerken** yan özellik atanır ve sabittir; küçük boyutta düşse bile kart "kilitli" gösterir, büyütünce açılır. **Yükseltmede** özel bir harcamayla havuzdan 3 rastgele teklif görülüp biri seçilebilir (öneri; harcama biçimi açık).
- **Parçalama (kullanıcı, 2026-10-08):** Rare ve üstü merdivenli modüllerde ortak yan özellik seçeneği F1. Her zaman tek hücrelere bölünür (kısmi 2+1 seçimi yok); yan yana konan parçalar otomatik bağlanır. Bölge bonusu orantılıdır. Komşuluk modüllerinde parça başına bitişiklik sayılır ve sınır konmaz; oyuncu serbest, oyunu kırabilmesi kabul edilir. Parça yerleşimi için veri/ızgara/kayıt değişikliği maliyeti kabul edildi.
- Common'da yan özellik yoktur. **Rare, Uncommon tasarımını sürdürür (kullanıcı, 2026-10-08):** tek boyutlu Rare'de yan özellik yok; merdivenli Rare'de en büyük boyutta 1 yan özellik, 2 özel + 3 ortak seçenek; şekiller Rare'e uygun seçilir. Önceki "Rare'de iki yan özellik" önerisi geçersiz. Yan özelliklerin güç düzeyi eşit tutulur.
- Kayıt/yükleme tasarım hedefi: modül kimliği, boyut seviyesi ve yan özellik kimliği saklanacak; yeniden yüklemede değişmeyecek. Şimdilik bu alanlar yalnız runtime verisi; kalıcılık sistemi uygulanmadı.
- **Yan özelliklerin gelişimi (öneri, karar bekliyor):** modül seviyesiyle yan özellik de büyür, ana özelliğin yaklaşık yarı hızında. Ortak havuz özelliklerinin 3 adımlık değerleri bir kez tanımlanır; özel yan özelliklerin değerleri modül başına yazılır.

#### Ortak yan özellik havuzları (öneri değerler)

| Havuz | Kimlik | Özellik | Runtime durumu |
|---|---|---|---|
| Offense (O) | O1 | Attack Power +8 | Implemented |
| | O2 | Critical Chance +8% | Implemented |
| | O3 | On kill, Attack Speed +10 for 2 sec (does not stack; duration refreshes) | Implemented |
| | O4 | Your applied damage status durations +1 sec; includes Frozen, excludes freeze cooldown | Implemented |
| | O5 | Your first successful hit against each target ×1.15 | Implemented · blocked/zero damage does not consume it |
| Defense (D) | D1 | MaxHealth +25 | Implemented |
| | D2 | On taking damage, Armor +12 for 2 sec (does not stack; duration refreshes) | Implemented |
| | D3 | On kill, restore health equal to 1% of MaxHealth | Implemented |
| | D4 | Shield regeneration rate ×1.15 | Implemented |
| | D5 | Max Shield +8% | Implemented |
| General (G) | G1 | On owner kill, +1 Gold for this run | Implemented · resets on new run |
| | G2 | Ability Haste +4 | Implemented |
| | G3 | Afterburner capacity +10% | Implemented |
| | G4 | Horizontal and vertical movement rating +0.8 | Implemented |
| | G5 | Luck +1 | Implemented |
| Zone (Z) · öneri | Z1 | *Overclocked Seat:* while fully inside its family's zone, this module's zone bonus gets +10 points (+25% → +35%; with Zone Overclocker 50% → 60%). Only this module | Planned · kullanıcı, 2026-10-08 · ana modülden ayrışsın diye zayıflatıldı |
| | Z2 | *Misfit Seat:* while outside its family's zone, this module gets +10% (with Misfit Wiring 25% → 35%). Only this module | Planned · kullanıcı, 2026-10-08 · ana modülden ayrışsın diye zayıflatıldı |
| Fragment (F) · yalnız Rare ve üstü | F1 | *Fragmentable:* at max size the module splits into single cells (3 → 1+1+1, 2 → 1+1) that can be placed anywhere on the grid; fragments placed side by side auto-connect. Zone bonus is proportional to fragments in the matching zone (2 of 3 in zone → 2/3 of the bonus). Adjacency modules count adjacency from every fragment with no extra cap | Planned · kullanıcı fikri ve kararları (2026-10-08) |


#### Uncommon · tek boyutlu (7, yan özellik yok)

| Modül | Aile · boyut | Kart metni ve kural | Gelişim (öneri) | Durum |
|---|---|---|---|---|
| Photonic Lens | Primary · `■` | *You deal more damage to targets without a status effect.* All damage you deal to a target with no status effects ×1.15. Status is checked before each hit. | ×1.15 → ×1.18 → ×1.21 | Implemented · Uncommon · P4; tüm hasar, vuruş öncesi status kontrolü |
| Reflection Shield | Defense · `■■` | *Your shield reflects attacks after a Defense ability.* When a Defense ability is used, for 5 sec, 35% of the damage absorbed by your shield is reflected as Energy damage to the nearest enemy; cannot critically hit; inactive while no shield is available; duration refreshes on reuse. Reflected damage does not trigger other modules, including Energy Feedback (user, 2026-10-07). | 35% → 40% → 45% | Implemented · Uncommon · P4; 5 sn Defense penceresi, Energy Feedback dahil zincirleme yok |
| Health Bridge | Defense · `■■` · komşuluk | *Your health becomes strength for adjacent abilities.* Each direct damage hit from up to 2 adjacent Q/E/F/R slot modules deals additional damage equal to 3% of MaxHealth (at 250 health, +7.5 per hit). No effect without an adjacent module. | 3% → 3.5% → 4% | Implemented · Uncommon · P4; yeteneğin kendi hasar türü, DoT tick hariç |
| Lucky Cast | Utility · `■■` | *Your abilities sometimes refresh instantly.* Luck +1. When a Q/E/F/R ability is used, Luck × 5% chance to reduce that ability's cooldown by 50%; the chance naturally stops at 100% (no separate cap). Excluding Primary, each use rolls separately. | 5% → 6% → 7% / Luck | Implemented · Uncommon · P3; %50 tavanı kaldırıldı, doğal ihtimal üst sınırı %100 |
| Frost Lock | Utility · `■■` | *Targets freeze at 4 Cryo stacks.* A normal target reaching 4 Cryo stacks cannot move or attack for 1 sec and loses its stacks (slow is removed); 8 sec cooldown per target. Elites and bosses are immune to the freeze. | Freeze 1.0 → 1.3 → 1.6 sec; cooldown 8 → 7 → 6 sec | Implemented · Uncommon · P5; hareket+saldırı kesilir, elit/miniboss/boss muaf |
| Arc Extender | Offense · `■■■` | *Electric can reach 5 stacks.* Your applied Electric stack limit increases from 4 to 5. Incoming damage bonus +3%/6%/9%/**13%/19%** (previously, the 4th stack gave +16%). Duration and refresh rules are unchanged. | 5th-stack bonus +19% → +21% → +23% | Implemented · Uncommon · P5; sahip profili ve mevcut ortak Electric stack modeli |
| Double Feed Core | Utility · `■■■` | *Grants Attack Power and Energy Power.* Attack Power +15, Energy Power +23. | +15/+23 → +18/+28 → +21/+33 | Implemented · Uncommon · P2; seviye tablosu JSON'da, nihai denge ayrı |

#### Uncommon · merdivenli (11, yan özellikli)

Sütunlar: A boyuta göre; özel yan özellikler (2); ortak havuzdan uygun olanlar (3). Yan özellik her satırda en büyük boyutta açılır. Her merdivenli modül için 5 olası yan özellik içeriktedir: 2 özel + 3 ortak.

| Modül | Aile · merdiven | A: ana özellik | Özel yan özellikler | Ortak havuzdan | Gelişim (A, öneri) | Durum |
|---|---|---|---|---|---|---|
| Glass Cannon Core | Offense · 1-3 | *Attack Power rises, hull health falls.* 1c: AP +12, MaxHealth −6% · 3c: AP +34, MaxHealth −16% (after other bonuses) | Critical Chance +10% while health ≤50% · half the MaxHealth penalty returns as Max Shield +6% | O2, O4, O5 | AP +17% / level; penalty is fixed | Implemented · Uncommon · P2 A + Shield Return + Desperate Aim; O4/O5 dahil. P3 can sayısını korur, gerekirse son azami değere keser |
| Loot Shield | Defense · 1-2-3 | *Kills replenish your shield.* On each kill, gain shield equal to 1c: 2% of Max Shield; 2c/3c: 3%; cannot exceed maximum | On kill, restore 12% of Afterburner capacity *(Hunt Fuel)* · convert 50% of overflow shield into health | D1, D2, D4 | 1c 2% → 2.5% → 3%; 2c/3c 3% → 4% → 5% | Implemented · Uncommon · P3; Hunt Fuel ve Overflow Recovery dahil |
| Sharp Scale | Offense · 2-3 | *Critical hits hit harder and happen more often.* Critical Damage +0.20 (1.5 → 1.7), Critical Chance +15% (user requested +15–20%) | On critical hit, AS +15 for 2 sec *(Critical Rhythm)* · Critical Chance +2% per point of total Luck *(Luck Lens)* | O1, O3, O5 | Critical Damage +0.20 → +0.25 → +0.30 | Implemented · Uncommon · P2 A + P4 Critical Rhythm/Luck Lens; O5 dahil |
| Scatter Core | Offense · 2-3 | *The first hit fires small guided missiles.* First successful hit of each Offense use fires up to 3 missiles at distinct nearest enemies; each deals 15% of that hit. Fewer targets produce fewer missiles; speed 600 units/s, lifetime 3 sec. No status or critical hit | Once per use, 0.8 sec after first hit, a 100-radius explosion deals 25% of that hit *(Aftershock)* · 4th distinct missile | O1, O3, O4 | 15% → 18% → 21% per missile | Implemented · Uncommon · P6; Aftershock/Fourth Missile ve O4 dahil; generated hasar başka modülü tetiklemez |
| Thermal Shock Coil | Offense · 2-3 | *Deal more damage to burning and frozen targets.* While a target has at least 1 Thermal and 1 Cryo stack, all damage you deal to it ×1.22. Status is checked before the hit | Thermal and Cryo duration +2 sec · on their death, 100-radius Thermal explosion equal to 3% of target MaxHealth | O1, O2, O5 | ×1.22 → ×1.27 → ×1.32 | Implemented · Uncommon · P4 A + P5 Lingering Shock + P6 Death Burst; O5 dahil |
| Ricochet Tip | Primary · 2-3 | *A Primary hit bounces once.* First successful Primary hit bounces to the nearest other enemy within 300 units for 25% of applied damage; one base bounce per shot, shared by all pellets/targets (continuous beam: one damage step) | Transfer one damage status stack · one additional bounce deals 15% | O2, O4, O5 | 25% → 30% → 35% | Implemented · Uncommon · P6; Status Transfer/Second Bounce ve O4/O5 dahil |
| Cold Retort | Defense · 2-3 | *Slow enemies that hit you.* When you take damage, apply 1 Cryo stack to the 2 nearest enemies within 300 units; 2 sec cooldown | Enemies with a status effect deal ×0.88 damage to you *(Jarring Effect)* · enemies you freeze by Cryo spread 1 Cryo stack to enemies within 400 units | D2, D3, D5 | Target count 2 → 3 → 4 | Implemented · Uncommon · P4 A/Jarring Effect + P5 Frozen Spread (400 birim) |
| Grit Your Teeth | Defense · 2-3 | *Take less damage when you're in danger.* While hull health ≤35%, damage you take ×0.80; cannot revive from death | At the same threshold, +35 to whichever is higher: Attack Power or Energy Power (AP on a tie; use values before this bonus) *(Critical Circuit)* · shield regeneration ×2 at the same threshold | D1, D3, D4 | ×0.80 → ×0.77 → ×0.74 | Implemented · Uncommon · P3 A + Critical Circuit + Emergency Recharge |
| Hunt Clock | Utility · 2-3 | *Kills speed up abilities.* Each kill reduces pending cooldowns on active Q/E/F/R abilities by 0.25 sec; excludes Primary; cooldown cannot go below 0 | +1 Gold on kill *(Hunt Bounty)* · every 5 kills, reduce the cooldown of the ability with the longest cooldown by 20% | G2, G3, G5 | 0.25 → 0.30 → 0.35 sec | Implemented · Uncommon · P3 A + Fifth Kill + Hunt Bounty; koşu içi Gold, saniyelik denge tavanı yok |
| Movement Rhythm | Movement · 2-3 | *The faster you move, the faster you fire.* Gain +3.5 Attack Speed rating per 100 units of current movement speed (~+17–21 at top speed; the ship's speed cap is the natural limit). 0 while stationary | Horizontal and vertical rating +2.9 *(Advanced Thrust)* · when speed drops, the AS bonus fades over 1.5 sec instead of disappearing immediately | G2, G3, G5 | 3.5 → 4.0 → 4.5 / 100 speed | Implemented · Uncommon · P3 A + Advanced Thrust + Lingering Rhythm; Afterburner dahil gerçek hız sayılır |
| Trail Mines | Movement · 2-3 | *Leave mines after a movement ability.* For 5 sec after a successful Movement use, drop a mine every 50 units actually traveled; no stationary or teleport mines. Fuse 1 sec, radius 100, damage 8 at L1 | +1 Cryo stack · active limit +4 and fuse 0.7 sec | G1, G3, G4 | Damage 8 → 10 → 12; active limit 12 → 14 → 16 | Implemented · Uncommon · P6; Cryo Mines/Quick Fuse ve koşu içi G1 dahil |

#### P8 sonrası eksik yanlar — 2026-10-08

Canlı içerik **18 ana Uncommon modül** (7 tek boyutlu, 11 merdivenli) ve **37 yan özellik** taşır: 15 ortak, 22 özel. O4/O5/G1 ve Glass/Grit/Hunt/Movement'in bekleyen dört özel yanı tamamlandı. Her merdiven 2 özel + 3 ortak seçeneğe açıktır; yan özellik yalnız en büyük boyutta çalışır. Sayılar ve ana modüller mevcut JSON sahiplerindedir. Kaynak: [modules.json](../LightYearsGame/assets/content/data/modules.json), [module_side_features.json](../LightYearsGame/assets/content/data/module_side_features.json).

O5, modüllü kaynağın ilk başarılı hasarında hedef başına bir kez çalışır; engellenen/sıfır hasar ve generated/suppressed vuruş tüketmez. Aynı hedefe sök/tak veya ilgisiz rebuild bonusu yeniden açmaz. O4 taban süreye +1 sn ekler; tekrar uygulamada birikmez. Ignite/Cryo/Electric/Kinetic ve Frozen dahildir; freeze cooldown'u hariçtir.

Kullanıcı Gold için koşu içi kapsamı onayladı: başlangıç 0; G1 ve Hunt Bounty takılıyken sahibin normal düşman öldürmesi başına ayrı ayrı +1; gemi yenilenmesinde korunur, yeni koşuda sıfırlanır. Mağaza, temel Gold düşüşü, harcama ve kalıcılık sonraya kalır. Movement'in Lingering Rhythm yan özelliği hız bonusunu 1,5 sn söndürür; ayrı AS eklemez. Glass Shield Return mevcut sabit +%6 Max Shield profilidir; cezanın yarısını her boyut/seviyede dinamik hesaplayan ayrı bir davranış yoktur.

Yeni doğrulama sonucu [uygulama planının güncel sonuç bölümünde](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md#9-p8-sonrası-eksik-yanlar--2026-10-08) tutulur. Loot dağılımı, yükseltme harcaması/arayüzü, kalıcılık ve nihai denge ayrı işlerdir. Test hibesi edinilebilirlik kanıtı değildir. Kullanıcının görsel kabulü ve simetrik gemi şemalarının bonus-bölgesine sığma sorunu ayrı açık kalır; bu çalışma görsel/şema kaynaklarını değiştirmez.

#### Yan özellik olarak birleşenler (artık ayrı modül değil)

| Eski modül | Nereye girdi |
|---|---|
| Hunt Fuel | Loot Shield · private side feature |
| Hunt Bounty Processor | Hunt Clock · implemented private side feature; koşu içi +1 Gold |
| Advanced Thrust Block | Movement Rhythm · private side feature |
| Critical Circuit | Grit Your Teeth · private side feature |
| Critical Rhythm | Sharp Scale · private side feature |
| Luck Lens | Sharp Scale · private side feature |
| Aftershock | Scatter Core · private side feature |
| Jarring Effect | Cold Retort · private side feature |

#### Rare (23)

| Modül | Aile · şekil | Kart metni ve kural | Gelişim | Durum |
|---|---|---|---|---|
| Ember Accumulation | Offense · `■■` | When Thermal reaches 4 stacks, the target explodes once instead of taking Ignite damage over time: 130% of that stack's total 5 sec DoT; stacks are cleared. | Açık | Planned · kullanıcı Rare için uygun buldu · stack sıfırlama Thermal Shock Coil koşulunu bozar; kullanıcı bu çatışmayı kabul etti (2026-10-08) |
| Gold Plating | Defense · `■■` | +1 Armor per 20 Gold, up to ~+30 (draft +15; too low for Rare). When Gold is spent, the bonus is recalculated from current Gold. | Açık | Reworking · sayı; Gold oranı ekonomiye göre |
| Armor Conversion | Defense · `■■` | *Your Armor becomes Attack or Energy Power.* Add 40% of your Armor to whichever is higher: Attack Power or Energy Power (using values before this bonus). At 80 Armor, about +32. | 40% → 45% → 50% | Planned · kullanıcı esnek (AP/EP) ve Rare istedi |
| Shatter Shock | Defense · `■■` | *Your shield bursts when it breaks.* When the shield breaks, enemies within 250 units take damage equal to 15% of MaxHealth and are pushed back; 20 sec cooldown. Cryo stacks and shield restoration were removed from the Rare main effect (2026-10-08). | Damage 15% → 19% → 23%; cooldown 20 → 17 → 14 sec | Planned · itme mesafesi (önceki taslak 150 birim): açık |
| Status Resonator | Utility · `■■` | *Your applied statuses last longer.* Status effects you apply last ×1.4 (Kinetic/Electric/Cryo 6 → 8.4 sec; Thermal 5 → 7 sec). Stacks, DPS, and effects are unchanged; total Ignite damage increases by ~40%. | ×1.4 → ×1.5 → ×1.6 | Planned · Uncommon'dan Rare'e taşındı (2026-10-06) |
| Kinetic Breaker | Offense · `■■` | *Spend Kinetic stacks to pierce Armor.* While the target has 4 Kinetic stacks, your next direct hit consumes all 4 stacks and ignores the target's Armor completely. | Açık | Planned · kullanıcı beğendi (2026-10-08) · değeri düşman Armor'una bağlı; düşman Armor değerleri ayarlanacak |
| Continuous Field Generator | Defense · `■■■` | *Your shield never stops recharging.* Removes the shield recharge delay; the shield recharges continuously, even while taking damage, at ~35% of the normal recharge rate. | Açık | Planned · kalkan-kırılma tetikleri (Shatter Current, Shatter Shock…) kalkan %25 dolduktan sonra yeniden kurulur, 0↔az döngüsü tekrar tetiklemez (kullanıcı, 2026-10-08: eşik Max Shield'in %25'i) · Grit/D4/Fast-Charge çarpanlarıyla test |
| Triple Core | Utility · `■■` | *Leave one slot empty to empower the others.* If exactly one of Q/E/F/R is empty, direct damage of the other three abilities ×1.18. Inactive if two or more slots are empty. | Açık | Planned · kullanıcının 2026-10-08 listesi |
| Impact Redirector | Defense · `■■■` | *Armor also protects your shield.* While shield > 0, damage to the shield also benefits from 50% of Armor's damage reduction. | Açık | Planned · kullanıcının 2026-10-08 listesi |
| Status Primer | Primary · `■■` | *Primary hits feed your strongest status.* A Primary hit adds +1 stack to the status with the most stacks on the target; at most once per target per 1 sec. Does nothing if the target has no status. | Açık | Planned · kullanıcı beğendi; sınır 0,5 değil 1 sn (2026-10-08) |
| Arc Discharge | Offense · `■■` | *Electric jumps to nearby enemies.* When a target reaches 4 Electric stacks, 2 Electric stacks jump to the 2 nearest enemies within 350 units; stacks on the source are kept; 4 sec cooldown per target. | Sıçrama hedef sayısı artar (örn. 2 → 3 → 4) | Planned · kullanıcı beğendi; gelişim sekme/hedef sayısı (2026-10-08) |
| Spare Capacitor | Utility · `■■` · komşuluk | *An adjacent ability gains a charge.* An adjacent Q/E/F/R ability gains +1 charge; its cooldown is unchanged. | Açık | Planned · kullanıcı ceza istemedi: bekleme ×1,25 değil ×1,0 (2026-10-08) · gelişimde ne büyüyeceği açık |
| Giant Slayer | Offense · `■■` | *The bigger they are, the harder they fall.* Against a target whose hull MaxHealth is at least 1.5× your hull MaxHealth, damage you deal is increased; the larger the gap, the larger the bonus. Lower threshold 1.5×, no upper cap. Öneri formül: ×(1.10 + 0.05 × (oran − 1.5)) → 1.5× = ×1.10, 3× = ×1.175, 10× = ×1.525. | Gelişim kullanıcı tarafından sonra verilecek | Planned · kullanıcı yeniden tanımladı (2026-10-08): elit/boss %8 kalkan/Armor atlama yerine hull MaxHealth oranına bağlı hasar; alt sınır 1.5×, üst sınır yok · formül eğimi açık |
| Trophy Core | Offense · `■■` · komşuluk | *Kills make the adjacent ability stronger for the rest of the run.* Each enemy killed by the adjacent Q/E/F/R ability permanently adds +0.4% damage for the run (elite +3%, boss +10%). Stacks are stored in the module: move the module next to another ability and the bonus moves with it. | Stack başına değer | Planned · kullanıcı beğendi (2026-10-08; esin Shape of Dreams Divine Faith); aile Utility değil Offense (kullanıcı) · komşuluk kullanıcı kararıyla korunur · modül sökülünce stack'ler korunur mu: açık (öneri: korunur, yeniden takılınca etkin) |
| Zone Overclocker | Utility · `■` | *Supercharge one zone.* Other modules in the zone where this module is placed get a +50% zone bonus instead of +25%. No effect of its own. | 50% → 55% → 60% | Planned · kullanıcı beğendi; tek hücre (2026-10-08; esin Last Epoch Idol Altar) · Z1 yan özelliğinden ayrışsın diye +45'ten +50'ye güçlendirildi (kullanıcı: ana buff + yan nerf) · Gövde'deki +%10'a etkisi: açık |
| Misfit Wiring | Utility · `■■` | *Modules outside their own zone work better.* Modules placed in a zone that does not match their family get +25% to their main value (normally no bonus, no penalty) — as good as their own zone. | 25% → 30% → 35% | Planned · kullanıcı beğendi (2026-10-08) · Z2'den ayrışsın diye +20'den +25'e güçlendirildi · Zone Overclocker ile bilinçli zıtlık |
| Stripped Hull | Utility · `■` | *Fly light, hit hard.* Damage +2.5% for each empty cell on the ship. | 2.5% → 3% → 3.5% per cell (öneri) | Planned · kullanıcı: Utility, tek hücre, güçlü olmalı (2026-10-08; esin Cobalt Core) · koşu başındaki bedava erken güç kullanıcı tarafından kabul edildi; boş hücre sınırı yok |
| Dying Star | Offense · `■■■` | *Kills feed a star that goes supernova.* Each enemy that dies within 300 units of you charges the star; at 10 charges it explodes in a large Thermal blast around you and applies 4 Ignite stacks to every enemy hit; charges reset. | Patlama hasarı büyür; ölüm sayısı sabit | Planned · kullanıcı Süpernova yönünü seçti (2026-10-08; esin Nova Drift Dying Star; can harcayan aura sürümü bırakıldı) · patlama hasarı/yarıçapı: açık |
| Odometer | Movement · `■` | *The farther you travel, the stronger you get.* Every 5,000 units traveled during the run permanently grants +1% damage and +1% movement speed for the rest of the run; no cap. Stacks are stored in the module. | Stack başına değer +1% → +1.2% → +1.4% (mesafe sabit) | Planned · kullanıcı beğendi; seviye ile birim başına % artar (2026-10-08) · oyunun yolculuk hissine bağlı |
| Escape Velocity | Movement · `■■` | *Reach top speed and your shots go sonic.* After 2 sec of uninterrupted flight above 90% of max speed, the ship goes sonic: Primary projectiles travel twice as fast and pierce one target. Ends when you slow down. | Sonik durumda Primary hasarı +10% → +15% → +20% | Planned · kullanıcı beğendi (2026-10-08) |
| Momentum Shield | Movement · `■■` | *Your shield charges from speed, not time.* The shield no longer regenerates normally. Instead it recharges in proportion to your current movement speed: at max speed ~×1.5 the normal regeneration rate, zero while standing still. No recharge delay after damage. | Max hızdaki çarpan ×1.5 → ×1.7 → ×1.9 (öneri) | Planned · kullanıcı fikri (2026-10-08); Rare/Epic açık, Claude önerisi Rare (Continuous Field Generator ile aynı ağırlıkta kural değişimi) · Continuous Field Generator ile aynı anda takılamaz; modül ekranı uyuşmazlık uyarısıyla iki modülü işaretler (kullanıcı, 2026-10-08) |
| Omni Thrusters | Movement · `■■` | *Same speed in every direction.* Forward, reverse, and strafe thrust all become equal: forward thrust −30%, and reverse and strafe are set to that same value (e.g. Fighter 650/190/270 → 455/455/455). | İleri cezası seviye başına 1–2 puan azalır (öneri 1,5): −30% → −28.5% → −27% (Fighter 455 → 465 → 475) | Planned · kullanıcı tanımı (2026-10-08): üç yön eşit (~450/450/450); seviye artışı %5 değil 1–2 puan |
| Silent Running | Utility · `■` | *Travel quietly between fights.* Threat gained while traveling between route nodes ×0.75. | Açık | Planned · kullanıcı beğendi (2026-10-08) · rota/Threat sistemine bağlı; harita henüz yok |

#### Epic / Legendary adayları (7)

İlk Epic tasarımlarıdır; Epic nadirlik kuralları (yan özellik sayısı, gelişim) henüz karar değildir.

| Modül | Aile · şekil | Kart metni ve kural | Gelişim | Durum |
|---|---|---|---|---|
| Phase Slip | Movement · `■■` | *Slip through fire and answer with an explosive shot.* For the first 0.35 sec after Afterburner starts, enemy projectiles pass through the ship, but you cannot use abilities or fire the Primary weapon. Your first Primary shot after the phase becomes an explosive round that deals area damage around the impact. 6 sec cooldown. | Patlama hasarı/yarıçapı büyür; faz süresi ve bekleme sabit | Planned · kullanıcı faz sırasında ateş/yetenek kilidi ve sonraki merminin patlayıcıya dönüşmesini istedi, Epic önerdi; Claude katıldı (2026-10-08) · patlama hasarı, yarıçap: açık (öneri ×2, 120 birim) · geçilen mermi başına +%25 hasar ve +15 birim yarıçap, sınır yok (kullanıcı, 2026-10-08: %10 riskli yetenek için azdı; sorun çıkarsa sınır sonra eklenir) · temel ×2 hasar/120 birim öneri |
| Last Stand | Defense · `■■` | *Cheat death once.* On lethal damage, revive once with 50% health; the module then burns out (shown broken, no effect) and Max Shield is −30% for the rest of the run. | Dirilme canı 50% → 60% → 70% | Planned · kullanıcı Epic dedi (2026-10-08; esin Nova Drift Last Stand) · Grit Your Teeth ölümden dirilmez kuralıyla ayrı mekanik |
| Combo Sequencer | Utility · Açık | İlk Rare taslağı: four different Q/E/F/R abilities within 10 sec → cooldowns recover ×1.5 for 5 sec. Kullanıcı fikri tuttu ama Rare için değil; Epic/Legendary olarak farklı tasarlanacak (2026-10-08). | Açık | Reworking · yeni tasarım bekliyor |
| Turret Mount | Primary · `■■■` · Epic | *Your primary weapon aims itself.* The Primary weapon automatically aims at the nearest enemy in range, independent of the ship's facing; damage ×0.80. | Açık | Planned · kullanıcı beğendi (2026-10-08) · Omni Thrusters ile sinerji |
| Blink Drive | Movement · `■■` · Epic | *Afterburner becomes short-range teleports.* Afterburner is replaced by 3 short teleport charges; recharge is derived from Afterburner capacity. | Açık | Planned · kullanıcı beğendi (2026-10-08) · menzil/dolum açık |
| Second Hardpoint | Primary · `■■■` · Legendary | *Mount a second primary weapon.* Breaks the one-per-category rule: a second Primary Weapon module can be installed (e.g. Fighter + Breacher shotgun). Both fire at ×0.6, or a key switches between them. | Açık | Planned · kullanıcı beğendi (2026-10-08) · aynı anda mı geçişli mi: açık |
| Twin Rack | Utility · `■■■` · Legendary | *Ability sets, Hwei-style.* Q/E/F/R slots hold more than one ability, organized as sets the player switches between (kullanıcı: LoL Hwei'nin büyü setleri gibi). | Açık | Planned · kullanıcı beğendi (2026-10-08) · girdi Afterburner (LShift) ve Primary (Space) ile çakışmamalı (kullanıcı); kullanıcı kararı: Ctrl basılıyken Q/E/F/R ikinci seti kullanır (Ctrl+Q gibi) · set yapısı ve bekleme kuralları açık |

#### Açık / ertelenen

| Modül | Durum |
|---|---|
| Broadside Mount | **Planned · kullanıcı fikri beğendi (2026-10-08, öneri 2).** Primary: *Your primary weapon fires from both sides of the ship instead of forward.* Öneri: sabit 3 hücre, her yanda normal hasarın %70'i, Epic adayı; sayı/boyut/nadirlik kesinleşmedi. Silah tiplerine uyarlama ve gelişim açık; tek boyutlu seçilirse yan özellik yok. |
| Polar Shield | **Planned · kullanıcı fikri beğendi (2026-10-08, öneri 4).** Defense: *Your shield splits into independent front and rear pools. Hits drain the pool facing the attack; an empty pool exposes the hull.* Öneri: mevcut toplam kapasite eşit bölünür, toplam regen bütçesi paylaşılır; 2–3 hücre, Rare adayı. Sayı/boyut/nadirlik, kırılma tetikleri ve diğer kalkan modülleriyle etkileşim açık. |
| Survey Relay | **Planned · kullanıcı fikri beğendi (2026-10-08, öneri 6).** Utility: *Reveal the module families of upcoming rewards on the route map.* Öneri: sonraki 2 ödül görünür; gelişim görülen düğüm sayısı; 1–2 hücre, Uncommon/Rare açık. Sayı/boyut/nadirlik kesinleşmedi; gelecek rota haritasına bağlı, mevcut arena özelliği değil. |
| Heavy Hull Reinforcement | Defense · `■■■` Armor +50, movement −1/−1. Düz stat takası olduğundan Armor Conversion (Rare) ile birleştirilmesi öneri; karar yok. |
| Layered Defense | Defense · `■■` proposal: while shield > 0, Armor +35; bonus is removed when the shield breaks. Karar yok; savunma sayısı yeterli olduğundan çıkarılabilir. |
| Circle Spirit | Offense · `■■` proposal: damage you deal increases by 2% for each enemy within 300 units, capped at 20% (progression 2% → 2.5% → 3%; cap 20% → 25% → 30%). Nadirlik: **Uncommon adayı** (Claude, kullanıcı rarity seçimini bıraktı, 2026-10-08); devam eden Uncommon fazlarını bozmamak için 18'lik sete henüz alınmadı. |
| Chain Link | Utility · `■■` adjacency proposal: when one of two adjacent Q/E/F/R slots is used, reduce the other's cooldown by 0.5 sec (progression 0.5 → 0.7 → 0.9 sec). Karar yok. |
| Level Refresh | Defense · `■■` proposal: on level up, restore health equal to 25% of MaxHealth and 50% of Max Shield. Altı öldürmede seviye sistemi geçici test amaçlı olduğundan riskli; karar yok. |
| Blood Bond, Drift Armor | Defense yedek önerileri; karar yok. |
| Momentum Strike | Movement · `■■` Rare proposal: within 3 sec after a Movement ability, the first Offense ability deals ×1.30 damage. Kullanıcı emin değil; karar yok. |
| Impact Tail | Effect undefined; awaiting a one-sentence description from the user. |
| Rapid Condenser | Ertelendi: charge'lı yetenek sayısı sayılacak; azsa ikinci bir etki gerekir ya da elenir. |

**Modül uyuşmazlığı (kullanıcı, 2026-10-08):** Aynı kuralı farklı biçimde yeniden yazan modüller aynı anda takılamaz. İkincisi yerleştirilmek istendiğinde modül ekranı uyuşmazlık uyarısı verir ve çatışan iki modülü işaretler. İlk çift: Momentum Shield ↔ Continuous Field Generator. Yalnız ters sinerjisi olan modüller (ör. Ember Accumulation ↔ Thermal Shock Coil) bu kurala girmez; birlikte takılabilirler.

**Komşuluk az kullanılır (kullanıcı, 2026-10-08):** modül sistemi gemi geneli ve bölge temelli çalışır; Shape of Dreams'teki gibi yeteneğe takılan öz mantığı esas değildir. Yeni tasarımlarda komşuluk istisna olmalıdır.

**Komşuluk kuralı:** Health Bridge, Spare Capacitor ve Trophy Core şu kuralı kullanır: hücre kenarı paylaşan Q/E/F/R slot modülleri komşudur; bir domino en çok 2 slota değer. Primary slotu komşuluk dışıdır.

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
| Tension Matrix | Rejected (2026-10-08, Rare aday: Electric 4 → 8 stack; kullanıcı yapılmamasını istedi) |
| Energy Coupler | Rejected (2026-10-08, Rare aday: komşu slot hasarının bir kısmını Energy'ye çevirme; kullanıcı kaldırdı) |
| Precision Processor | Rejected (2026-10-08, Rare aday: kritik RNG yerine yük sayacı; kullanıcı beğenmedi) |
| Overkill Relay | Rejected (2026-10-08, Rare aday: artan öldürme hasarını yakın hedefe aktarma; kullanıcı beğenmedi) |
| Graze Coil, Ram Plating, Shatterpoint, Charged Chamber | Rejected (2026-10-08, Rare adayları: sıyırma→Afterburner, yüksek hızda çarpma hasarı, donmuş hedefte buz kıymığı, bekleyince şarjlı Primary; kullanıcı hiçbirini beğenmedi) |
| Status Reaction, Death Transfer, Afterburner Overflow, Echo Slot | Rejected (2026-10-08, kullanıcı yaratıcı bulmadı; Echo Slot EchoProtocol yeteneğinin modül hali) |
| Gilded Trigger, Afterglow, Autopilot Slot, Cold Preservation, Amplifier Frame | Rejected (2026-10-08, Shape of Dreams/Hades/RoR2 esinli tur; komşuluğa fazla dayandı) |
| Salvo Loader | Rejected (2026-10-08, Primary yeniden dolmaz/Afterburner şarjörü doldurur; kullanıcı kötü buldu) |
| Slipstream Tether, Decoy Beacon, Full Thrusters | Rejected (2026-10-08, Movement Rare adayları: Afterburner'la düşman sürükleme, hologram yem, Motor bölgesi doluysa +1 charge; kullanıcı beğenmedi) |
| Comet Core, Dynamo, Wake Charger | Rejected (2026-10-08, Movement Rare adayları; kullanıcı Escape Velocity'yi seçti, Dynamo yerine kendi hız→kalkan fikrini verdi) |
| Hot Swap Bay, Trade Uplink, Belt Feed, Modular Frame, Shield Drones | Seçilmedi (2026-10-08, sistem değiştiren tur; kullanıcı Silent Running, Turret Mount, Blink Drive, Second Hardpoint, Twin Rack'i seçti) |

## Kaynaklar ve sınırlar

- Çalıştırılan modül kayıtları: [modules.json](../LightYearsGame/assets/content/data/modules.json).
- Common etki değerleri ve doğrulama geçmişi: [Common uygulama planı](MODULE_COMMON_IMPLEMENTATION_PLAN.md).
- Genel modül yerleşimi/nadirlik ilkeleri: [Modül Sistemi Tasarımı](MODULE_SYSTEM_DESIGN.md).
- Planned/Reworking fikirleri, kullanıcının 2026-10-05 ilettiği taslaklar ve geri bildirimlerdir. Bunları oyun davranışı, loot, son değer veya kesin rarity olarak sunma.
