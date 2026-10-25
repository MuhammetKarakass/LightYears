# Tarihsel katalog — 2026-10-23 kaynak denetimi öncesi

Bu dosya eski ifadeleri de koruyan anlık kopyadır. Güncel sahip [modules.md](../../modules.md).

# Modül kataloğu

**Son düzenleme: 2026-10-23.** Bu dosya seçilmiş modül tasarımlarının sahibidir. Canlı içerik [modules.json](../../../LightYearsGame/assets/content/data/modules.json) ve [module_side_features.json](../../../LightYearsGame/assets/content/data/module_side_features.json); sistem kuralları [MODULE_SYSTEM_DESIGN.md](../../MODULE_SYSTEM_DESIGN.md), teknik akış [MODULE_SYSTEM_ARCHITECTURE.md](../../MODULE_SYSTEM_ARCHITECTURE.md) içindedir. Tasarımın belgede seçilmiş olması runtime uygulaması veya oyuncu kabulü değildir.

## İçindekiler

- [Sayım ve okuma kuralları](#sayım-ve-okuma-kuralları)
- [Common, Uncommon ve Rare](#common-uncommon-ve-rare)
- [Epic modüller](#epic-modüller--15)
- [Legendary modüller](#legendary-modüller--3)
- [Seçilmiş, nadirliği belirlenmemiş modüller](#seçilmiş-nadirliği-belirlenmemiş-modüller--3)
- [Yan özellikler](#yan-özellikler)
- [Sistem modülleri](#sistem-modülleri)
- [Seçilmiş mekanikler ve değerlendirme kayıtları](#seçilmiş-mekanikler-ve-değerlendirme-kayıtları)
- [Uygulama durumu](#uygulama-durumu)

<a id="sayısal-envanter--2026-10-13"></a>
<a id="sayısal-envanter--2026-10-14"></a>
<a id="sayısal-envanter--2026-10-15"></a>

## Sayım ve okuma kuralları

**83 seçilmiş ana modül:** 56 uygulanmış, 27 planlı. Common13, Uncommon25, Rare24, Epic15, Legendary3; Combo Sequencer, Ultimate Protocol ve Courier Hold’un nadirliği henüz belirlenmedi. Hunt Clock dört ayrı içerik olarak sayılır; aile karşılaştırmasında ayrı seri sütunundadır. Sistemler, eski JSON tanımları, yan özellikler ve seçilmemiş adaylar ana sayıma dahil değildir.

**Canlı JSON kaydı:** 56 ana + 14 sistem + 9 eski tanım = 79. Bu, loot/denge/oynanış kabulü değildir. Sayım kayıtları 2026-10-13–15 uygulama dilimine dayanır; tarihli kanıtlar ayrı belgededir.

- **Implemented:** belgelenmiş runtime uygulaması. **Planned:** seçilmiş fakat uygulanmamış tasarım. **Geçiş bekliyor:** canlı kayıtla seçilen yön farklı.
- **Öneri:** sayı veya ayrıntı henüz seçilmedi. **Açık:** seçilmiş tasarımın tamamlanacak parametresi. Bunlar modülün ertelendiği anlamına gelmez.
- **İnceleme:** mekanik henüz seçilmedi. **Depoda/ertelendi:** aktif havuz dışında tutuldu. **Kaldırıldı/reddedildi:** güncel tasarımda kullanılmaz.
- Boyut seviye değildir. Sabit boyutta yan yok; merdivenin maksimum boyutunda rastgele atanmış tek yan açılır. Spare Capacitor yansız merdiven istisnasıdır. `■` bir hücredir.
- Offense/Defense genelde 2–3; Primary/Utility genelde 1–2 hücredir. Movement genelde 1–2, fakat 3 hücre seçeneği Utility’den daha yaygındır. İstisnalar geçerlidir; nadirlik otomatik boyut büyütmez. Üç hücreli seçenekler genelde L, iki hücre domino, tek hücre karedir; özel şekiller kendi satırında korunur.

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Hunt Clock serisi | Yetenek modülü | Ailesiz | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Common | 2 | 4 | 4 | 2 | 1 | 0 | 0 | 0 | 13 |
| Uncommon | 3 | 5 | 6 | 3 | 4 | 4 | 0 | 0 | 25 |
| Rare | 0 | 6 | 6 | 5 | 7 | 0 | 0 | 0 | 24 |
| Epic | 2 | 4 | 2 | 4 | 3 | 0 | 0 | 0 | 15 |
| Legendary | 1 | 0 | 0 | 1 | 1 | 0 | 0 | 0 | 3 |
| Epic / Legendary açık | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 1 |
| Nadirlik açık | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 1 | 2 |
| **Toplam** | **8** | **19** | **18** | **15** | **16** | **4** | **2** | **1** | **83** |

### Uygulanan ana modüller · 56

Hunt Clock serisi aile karşılaştırmasında ayrı sütundur; dört sürüm toplamda dahildir. Bu tablo seçilen 83 tasarım tablosunun yerine geçmez.

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Hunt Clock | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|
| Common | 2 | 4 | 4 | 2 | 1 | 0 | 13 |
| Uncommon | 3 | 5 | 6 | 3 | 3 | 4 | 24 |
| Rare | 0 | 5 | 5 | 4 | 5 | 0 | 19 |
| **Toplam** | **5** | **14** | **15** | **9** | **9** | **4** | **56** |

Canlı Rare Utility: Zone Overclocker, Misfit Wiring, Triple Core, Spare Capacitor, Release Valve. Danger Pay/Interest Ledger ile seçilen hedef7'ye ulaşacak.

## Common, Uncommon ve Rare

### Common · 13

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Durum |
|---|---|---|---|---|---|
| Heavy Armor Plate | Defense · `■■■` | Armor +30 | Tek değer. | Yok. | Implemented · Common |
| Assault Core | Offense · `■■` | Attack Power +15 | Tek değer. | Yok. | Implemented · Common |
| Focused Barrel | Primary · `■■` | Primary damage +30% | Tek değer. | Yok. | Implemented · Common |
| Thrust Booster | Movement · `■■` | Horizontal and vertical movement rating +2.2 | Tek değer. | Yok. | Implemented · Common |
| Attack Rhythm | Offense · `■■` | On Offense use, Attack Speed +15% for 3 sec; does not stack, duration refreshes | Tek değer. | Yok. | Implemented · Common |
| Shatter Current | Defense · `■■` | When the shield breaks, Primary hits deal additional Energy damage equal to 30% of hit damage for 5 sec | Tek değer. | Yok. | Implemented · Common |
| Energy Feedback | Offense · `■` | On dealing Energy damage, Energy Power +16 for 4 sec; does not stack, duration refreshes | Tek değer. | Yok. | Implemented · Common |
| Escape Reflex | Movement · `■■` | On taking damage, movement speed ×1.25 for 2 sec; does not stack, duration refreshes | Tek değer. | Yok. | Implemented · Common |
| Finishing Blow | Offense · `■■` | Damage you deal ×1.20 while the target is at 25% health or below | Tek değer. | Yok. | Implemented · Common |
| Zone Cooler | Utility · sabit2 hücre | In a single-family zone, that family's ability cooldown ×0.79; in Hull, active non-Primary abilities ×0.90; no bonus in Nose | Tek değer. | Yok. | Implemented · tasarım geçişi ilk paket (2026-10-13); Game build+ModulePlanTransitionE2E212 kontrol. Oyuncu oynanış/görsel kabulü değildir. |
| Full Magazine | Primary · `■■` | The first 3 successful Primary shots in each magazine deal ×1.30 damage | Tek değer. | Yok. | Implemented · Common |
| Barrier Recharge | Defense · sabit 2 hücre | Defense kullanınca MaxShield’in %10’u kadar eksik kalkan doldurulur; maksimumu aşmaz. | Common; tek değer. | Yok (sabit boyut; kullanıcı onayı, 2026-10-13). | Implemented · tasarım geçişi ilk paket (2026-10-13); Game build+ModulePlanTransitionE2E212 kontrol. Oyuncu oynanış/görsel kabulü değildir. |
| Brace Reflex | Defense · sabit 2 hücre | Defense kullanınca 4sn Armor+25; birikmez, süre yenilenir. | Common; tek değer. | Yok (sabit boyut; kullanıcı onayı, 2026-10-13). | Implemented · tasarım geçişi ilk paket (2026-10-13); Game build+ModulePlanTransitionE2E212 kontrol. Oyuncu oynanış/görsel kabulü değildir. |

### Uncommon · 25

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Durum |
|---|---|---|---|---|---|
| Photonic Lens | Primary · `■` | *Primary hits deal more damage to targets without a status effect.* Primary damage to a target with no status effects ×1.50. Status is checked before each hit. | ×1.50 → ×1.53 → ×1.56 | Yok (sabit boyut). | Implemented · Uncommon · P4; yalnız Primary, vuruş öncesi status kontrolü |
| Reflection Shield | Defense · `■■` | *Your shield reflects attacks after a Defense ability.* When a Defense ability is used, for 5 sec, 35% of the damage absorbed by your shield is reflected as Energy damage to the nearest enemy; cannot critically hit; inactive while no shield is available; duration refreshes on reuse. Reflected damage does not trigger other modules, including Energy Feedback (user, 2026-10-07). | 35% → 40% → 45% | Yok (sabit boyut). | Implemented · Uncommon · P4; 5 sn Defense penceresi, Energy Feedback dahil zincirleme yok |
| Health Bridge | Defense · `■■` · komşuluk | *Your health becomes strength for adjacent abilities.* Each direct damage hit from up to 2 adjacent Q/E/F/R slot modules deals additional damage equal to 3% of MaxHealth (at 250 health, +7.5 per hit). No effect without an adjacent module. | 3% → 3.5% → 4% | Yok (sabit boyut). | Implemented · Uncommon · P4; yeteneğin kendi hasar türü, DoT tick hariç |
| Frost Lock | Utility · `■■` | *Targets freeze at 4 Cryo stacks.* A normal target reaching 4 Cryo stacks cannot move or attack for 1 sec and loses its stacks (slow is removed); 8 sec cooldown per target. Elites and bosses are immune to the freeze. | Freeze 1.0 → 1.3 → 1.6 sec; cooldown 8 → 7 → 6 sec | Yok (sabit boyut). | Implemented · Uncommon · P5; hareket+saldırı kesilir, elit/miniboss/boss muaf |
| Arc Extender | Offense · 1/2 hücre (boyut merdiveni) | *Electric can reach 5 stacks.* Your applied Electric stack limit increases from 4 to 5. Incoming damage bonus +3%/6%/9%/**13%/19%** (previously, the 4th stack gave +16%). Duration and refresh rules are unchanged. | 5th-stack bonus +19% → +21% → +23% | Özel Arc Discharge/Status Resonator; ortak O1/O2/O5. Maksimum2 hücrede tek yan. | Implemented · tasarım geçişi ilk paket (2026-10-13); Game build+ModulePlanTransitionE2E212 kontrol. Oyuncu oynanış/görsel kabulü değildir. |
| Double Feed Core | Utility · `■■` | *Grants Attack Power and Energy Power.* Attack Power +15, Energy Power +23. | +15/+23 → +18/+28 → +21/+33 | Yok (sabit boyut). | Implemented · Uncommon · P2; seviye tablosu JSON'da, nihai denge ayrı |
| Omni Thrusters | Movement · `■■` | *Same speed in every direction.* Forward, reverse, and strafe thrust all become equal: thrust and top speed −30%, and reverse and strafe are set to that same value (e.g. Fighter 650/190/270 → 455/455/455). | İleri cezası seviye başına 1–2 puan azalır (öneri 1,5): −30% → −28.5% → −27% (Fighter 455 → 465 → 475) | Yok (sabit boyut). | **Implemented · Uncommon (2026-10-13)** · kullanıcı tanımı (2026-10-08): üç yön eşit (~450/450/450); seviye artışı %5 değil 1–2 puan |
| Glass Cannon Core | Offense · 1-3 | *Attack Power rises, hull health falls.* 1c: AP +12, MaxHealth −6% · 3c: AP +34, MaxHealth −16% (after other bonuses) | AP +17% / level; penalty is fixed | Critical Chance +10% while health ≤50% · half the MaxHealth penalty returns as Max Shield +6% · Ortak: O2, O4, O5 | Implemented · Uncommon · P2 A + Shield Return + Desperate Aim; O4/O5 dahil. P3 can sayısını korur, gerekirse son azami değere keser |
| Loot Shield | Defense · 1-2-3 | *Kills replenish your shield.* On each kill, gain shield equal to 1c: 2% of Max Shield; 2c/3c: 3%; cannot exceed maximum | 1c 2% → 2.5% → 3%; 2c/3c 3% → 4% → 5% | On kill, damage taken ×0.90 for 2 sec; refreshes, does not stack *(Kill Ward)* · convert 50% of overflow shield into health · Ortak: D1, D2, D4 | Implemented · Uncommon · P3; Overflow Recovery + Kill Ward mevcut; Hunt Fuel yerine Kill Ward kullanıcı onayıyla eklendi (2026-10-13) |
| Sharp Scale | Offense · 2-3 | *Critical hits hit harder and happen more often.* Critical Damage +0.20 (1.5 → 1.7), Critical Chance +15% (user requested +15–20%) | Critical Damage +0.20 → +0.25 → +0.30 | On critical hit, AS +15% for 2 sec *(Critical Rhythm)* · Critical Chance +2% per point of total Luck *(Luck Lens)* · Ortak: O1, O3, O5 | Implemented · Uncommon · P2 A + P4 Critical Rhythm/Luck Lens; O5 dahil |
| Scatter Core | Offense · 2-3 | *The first hit fires small guided missiles.* First successful hit of each Offense use fires 3 missiles, preferring distinct nearest enemies; each deals 15% of that hit. With fewer targets, remaining missiles reuse available targets; speed 600 units/s, lifetime 3 sec. No status or critical hit | 15% → 18% → 21% per missile | Once per use, 0.8 sec after first hit, a 100-radius explosion deals 25% of that hit *(Aftershock)* · 4th missile (may reuse a target) · Ortak: O1, O3, O4 | Implemented · Uncommon · P6; Aftershock/Fourth Missile ve O4 dahil; generated hasar başka modülü tetiklemez |
| Thermal Shock Coil | Offense · 2-3 | *Deal more damage to burning and frozen targets.* While a target has at least 1 Thermal and 1 Cryo stack, all damage you deal to it ×1.30. Status is checked before the hit | ×1.30 → ×1.35 → ×1.40 | Thermal and Cryo duration +2 sec · on their death, 400-radius Thermal explosion equal to 15% of target MaxHealth · Ortak: O1, O2, O5 | Implemented · Uncommon · P4 A + P5 Lingering Shock + P6 Death Burst; O5 dahil |
| Ricochet Tip | Primary · 2-3 | *A Primary hit bounces once.* First successful Primary hit bounces to the nearest other enemy within 450 units for 25% of applied damage; if none is nearby, emits a random straight projectile travelling up to 450 units; same texture, visual scale and actual speed as the source Primary projectile; damage lands on contact; one base bounce per shot, shared by all pellets/targets (continuous beam: one damage step) | 25% → 30% → 35% | Transfer one damage status stack · one additional bounce deals 15% · Ortak: O2, O4, O5 | Implemented · Uncommon · P6; Status Transfer/Second Bounce ve O4/O5 dahil |
| Grit Your Teeth | Defense · 2-3 | *Take less damage when you're in danger.* While hull health ≤35%, damage you take ×0.80; cannot revive from death | ×0.80 → ×0.77 → ×0.74 | At the same threshold, +35 to whichever is higher: Attack Power or Energy Power (AP on a tie; use values before this bonus) *(Critical Circuit)* · shield regeneration ×2 at the same threshold · Ortak: D1, D3, D4 | Implemented · Uncommon · P3 A + Critical Circuit + Emergency Recharge |
| Hunt Clock: Offense | Offense · 2-3 | Each kill refunds 5% of remaining Offense ability cooldowns; Primary excluded. | 5% → 6% → 7% | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold · Ortak: G2,G3,Lucky Cast (Luck+1,%3/Luck ihtimal,%25 kullanılmış bekleme iadesi) | Implemented · Uncommon; family filter and percentage refund |
| Hunt Clock: Defense | Defense · 2-3 | Each kill refunds 5% of remaining Defense ability cooldowns; Primary excluded. | 5% → 6% → 7% | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold · Ortak: G2,G3,Lucky Cast (Luck+1,%3/Luck ihtimal,%25 kullanılmış bekleme iadesi) | Implemented · Uncommon; family filter and percentage refund |
| Hunt Clock: Utility | Utility · 2-3 | Each kill refunds 5% of remaining Utility ability cooldowns; Primary excluded. | 5% → 6% → 7% | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold · Ortak: G2,G3,Lucky Cast (Luck+1,%3/Luck ihtimal,%25 kullanılmış bekleme iadesi) | Implemented · Uncommon; family filter and percentage refund |
| Hunt Clock: Movement | Movement · 2-3 | Each kill refunds 5% of remaining Movement ability cooldowns; Primary excluded. | 5% → 6% → 7% | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold · Ortak: G2,G3,Lucky Cast (Luck+1,%3/Luck ihtimal,%25 kullanılmış bekleme iadesi) | Implemented · Uncommon; family filter and percentage refund |
| Movement Rhythm | Movement · 2-3 | *The faster you move, the faster you fire.* Gain +3.5% Attack Speed at 200 actual speed, +7% at 300, +10.5% at 400; intermediate speeds keep the lower step. Zero below 200 | 3.5 → 4.0 → 4.5 per complete 100-speed step, starting at 200 | Horizontal and vertical rating +2.9 *(Advanced Thrust)* · when speed drops, the AS bonus fades over 1.5 sec instead of disappearing immediately · Ortak: G2, G3, G5 | Implemented · Uncommon · P3 A + Advanced Thrust + Lingering Rhythm; Afterburner dahil gerçek hız sayılır |
| Trail Mines | Movement · 2-3 | *Leave mines after a movement ability.* For 5 sec after a successful Movement use, drop a mine every 50 units actually traveled; no stationary or teleport mines. Fuse 1 sec, radius 100, Kinetic damage 30 + 30% Attack Power at L1 (AP snapshotted when dropped) | Damage 30 + 30% AP → 35 + 35% AP → 40 + 40% AP (L1 kullanıcı; L2/L3 ajan gelişim seçimi); active limit 12 → 14 → 16 | +1 Cryo stack · active limit +4 and fuse 0.7 sec · Ortak: G1, G3, G4 | Implemented · Uncommon · P6; Cryo Mines/Quick Fuse ve koşu içi G1 dahil |
| Stripped Hull | Utility · `■` | Her boş gemi hücresi hasar bonusu verir; tavan yok. Uncommon; kabul edilen güç hücre başına %1,5/%1,75/%2. Erken boş gemi güçlü, doldukça bonus düşer; boş-hücre build’i geç oyunda da korunabilir. | Hücre başına %1,5 → %1,75 → %2 (kullanıcı onayı,2026-10-14). | Yok (sabit boyut). | Implemented · 2026-10-14; Game build ve ModuleGoldHullE2E geçti. Yeni yanlarda seçilmemiş L2/L3 büyümesi eklenmedi. Eski Rare ContentId sabit, nadirlik Uncommon. |
| Overshield Layer | Defense · 2/3 hücre (boyut merdiveni) | Defense kullanınca MaxShield’in %20’si kadar tavan üstü geçici kalkan; 6sn içinde azalarak kaybolur. | Açık | Özel: Shield Rush (tavan üstü geçici kalkan varken hareket hızı+%20) / Reserve Shell (aynı koşulda alınan hasar×0,90). Ortak D1/D2/D4; beş seçenek havuzu kabul. Maksimum3 hücrede tek yan açılır. | Implemented · ikinci paket (2026-10-13); Game build+ModuleDefenseLayerE2E227kontrol; önceki paket212kontrol tekrar geçti. Yeni yanlar L1 kabul gücünü L2/L3te korur; yeni büyüme seçilmedi. OyuncuQA değildir. |
| Status Primer | Primary · sabit 2 hücre | Primary isabeti hedefin en çok stack’li statüsüne +1 stack verir; hedef başına en fazla 1 sn’de bir. Statü yoksa etkisiz. | Açık. | Yok (sabit boyut). | Implemented · ikinci paket (2026-10-13); Game build+ModuleDefenseLayerE2E227kontrol; önceki paket212kontrol tekrar geçti. Yeni yanlar L1 kabul gücünü L2/L3te korur; yeni büyüme seçilmedi. OyuncuQA değildir. |
| Armor Conversion | Defense · 2/3 hücre (boyut merdiveni) | Armor’un %40’ı bonus öncesi AP/EP’den yüksek olana eklenir; eşitlikte AP. Bağımsız ana modül. | %40/%45/%50; son yanıtın söylenmeyenleri kabul kapsamına alındı. | Özel: Rapid Alloy (Armor’un %10’u kadar AS yüzde puanı;80Armor→+%8 AS) / Balanced Output (ana dönüşüm bonusunun %25’i düşük AP/EP’ye, geri besleme yok). Ortak D1/D2/D3; havuz kabul. | Implemented · ikinci paket (2026-10-13); Game build+ModuleDefenseLayerE2E227kontrol; önceki paket212kontrol tekrar geçti. Yeni yanlar L1 kabul gücünü L2/L3te korur; yeni büyüme seçilmedi. OyuncuQA değildir. |
| Survey Relay | Utility · sabit 1 hücre | Rota haritasında sonraki iki ödülün modül ailelerini gösterir. | Görülen düğüm sayısı; değerler açık. | Yok (sabit boyut). | Planned · sistem bağımlılığı: Sonraki rota ödüllerinin modül aileleri için veri sahibi yok; GalaxyRoute yalnız yıldız/durak verisi taşıyor. |

### Rare · 24

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Durum |
|---|---|---|---|---|---|
| Zone Overclocker | Utility · `■` | *Supercharge one zone.* Other modules in the zone where this module is placed get a +50% zone bonus instead of +25%. No effect of its own. | 50% → 55% → 60% | Yok (sabit boyut). | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcı beğendi; tek hücre (2026-10-08; esin Last Epoch Idol Altar) · Z1 yan özelliğinden ayrışsın diye +45'ten +50'ye güçlendirildi (kullanıcı: ana buff + yan nerf) · Gövde'deki +%10'a etkisi: uygulanan varsayım ölçektir (Hull %10 → %20), kullanıcı onayı bekler |
| Odometer | Movement · `■` | *The farther you travel, the stronger you get.* Every 5,000 units traveled during the run permanently grants +1% damage and +1% movement speed for the rest of the run; no cap. Stacks are stored in the module. | Stack başına değer +1% → +1.2% → +1.4% (mesafe sabit) | Yok (sabit boyut). | **Implemented · Rare · dalga 3 (2026-10-10)** (mesafe sayacı oyunda çalışır; rota/yolculuk sistemi gerekmez) · kullanıcı beğendi; seviye ile birim başına % artar (2026-10-08) · oyunun yolculuk hissine bağlı · kullanıcı iyi buldu, Rare'de kalır (2026-10-10); yolculuk/rota sistemi yok: bağımlılık borcu |
| Silent Running | Movement · `■` | *Travel quietly between fights.* Threat gained while traveling between route nodes ×0.75. | Threat ×0,75 → 0,70 → 0,65 (kullanıcı onaylı, 2026-10-10) | Yok (sabit boyut). | Planned · sistem bağımlılığı: Threat artışını yazan seyahat sahibi yok. |
| Misfit Wiring | Utility · `■` | *Modules outside their own zone work better.* Modules placed in a zone that does not match their family get +25% to their main value (normally no bonus, no penalty) — as good as their own zone. | 25% → 30% → 35% | Yok (sabit boyut). | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcı beğendi (2026-10-08) · Z2'den ayrışsın diye +20'den +25'e güçlendirildi · Zone Overclocker ile bilinçli zıtlık · kullanıcı (2026-10-10): 1 hücre, sabit boyutlu · gemi genelinde, yalnız ana paket; Hull ve Ability Bay aile bölgesi sayılmadığı için Misfit'e tabi değildir |
| Momentum Shield | Movement · `■■` | *Your shield charges from speed, not time.* The shield no longer regenerates normally. Instead it recharges in proportion to your current movement speed: at max speed ×0.5 the normal regeneration rate at level 1; zero below 200 units/sec. Works during Afterburner. No recharge delay after damage. | Max hızdaki çarpan ×0.5 → ×0.7 → ×0.9; hız <200 ise yenilenme yok (kullanıcı, 2026-10-12) | Yok (sabit boyut). | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcı fikri (2026-10-08); Rare/Epic açık, Claude önerisi Rare (Continuous Field Generator ile aynı ağırlıkta kural değişimi) · Continuous Field Generator ile aynı anda takılamaz; modül ekranı uyuşmazlık uyarısıyla iki modülü işaretler (kullanıcı, 2026-10-08) |
| Triple Core | Utility · `■■` | *Leave one slot empty to empower the others.* If at least one of Q/E/F/R is empty, direct damage of every remaining ability ×1.25; active with one, two or three empty slots. | Çarpan ×1,25 → 1,30 → 1,35 (×1,18'den yükseltildi; kullanıcı onaylı, 2026-10-10) | Yok (sabit boyut). | **Implemented · Rare · dalga 3 (2026-10-10; reboot sayımı dalga 4, `RareStaticDamageRulesE2E`)** · kullanıcının 2026-10-08 listesi · kullanıcı Rare'de iyi buldu (2026-10-10) · Claude notu: boşaltılan yetenek hasar veriyorsa 3×1.25 < 4×1.0, yani değer boş slota ne koyulduğuna bağlı; gerçek hasar dağılımıyla ölçülmeli |
| Continuous Field Generator | Defense · `■■■` | *Your shield never stops recharging.* Removes the shield recharge delay; the shield recharges continuously, even while taking damage, at 20% of the normal recharge rate. | Yenilenme normalin %20 → 35 → 45'i (üç seviye kullanıcı düzeltmesi, 2026-10-13) | Yok (sabit boyut). | **Implemented · Rare · dalga 2 (2026-10-10)** · kalkan-kırılma tetikleri (ör. Shatter Current) kalkan %25 dolduktan sonra yeniden kurulur, 0↔az döngüsü tekrar tetiklemez (kullanıcı, 2026-10-08: eşik Max Shield'in %25'i) · Grit/D4/Fast-Charge çarpanlarıyla test |
| Impact Redirector | Defense · `■■■` | *Armor also protects your shield.* While shield > 0, damage to the shield also benefits from 50% of Armor's damage reduction. | Armor'un kalkana etkisi %50 → 60 → 70 (kullanıcı onaylı, 2026-10-10) | Yok (sabit boyut). | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcının 2026-10-08 listesi · `armorShare`: kalkan harcaması f = 1 − pay × (1 − Armor çarpanı) ile ölçeklenir; yan etki: kalkan yansıması (Reflection Shield) aynı oranda azalır, onay bekler |
| Spare Capacitor | Utility · `■` / `■■` / `■■■` (1-2-3) · komşuluk | *Adjacent abilities gain a charge.* 1 cell: 1 adjacent Q/E/F/R ability gains +1 charge; 2 cells: 2 adjacent abilities; 3 cells: 3 adjacent abilities at once. Cooldowns are unchanged. | Komşu yetenek sayısı boyutla 1 → 2 → 3; verilen charge sayısı seviye ile artar, boyutla artmaz; boyut yalnız kaç yeteneğe verileceğini artırır (kullanıcı, 2026-10-10; her seviyede +1 charge: L1 +1, L2 +2, L3 +3; parçalar yığılmaz, bir yetenek bu modülden en çok seviyesinin charge'ını alır) | **Yok — kullanıcı kararı, 2026-10-13.** Boyut merdiveni yan açmaz; özel/ortak yan almaz. | Implemented · tasarım geçişi ilk paket (2026-10-13); Game build+ModulePlanTransitionE2E212 kontrol. Oyuncu oynanış/görsel kabulü değildir. Ana charge1/2/3 ve komşuluk korunur; Trickle/F1 dahil yan izinleri kaldırıldı. |
| Ember Accumulation | Offense · `■■` (2-3) | When Thermal reaches 4 stacks, the target explodes once instead of taking Ignite damage over time: 130% of that stack's total 5 sec DoT; stacks are cleared. | Patlama %130 → 140 → 150 (+10 puan/seviye, kullanıcı onaylı 2026-10-10; tüm boyutlarda aynı) | *Area Blast:* patlama 200 birim çevreye de vurur, yan hedefe ana patlamanın %60'ı (oran onaylı, 2026-10-10) · *Smoldering Remains:* patlama yerinde 3 sn kor alanı, saniyede 1 Thermal stack · Ortak: O4, O5, F1 · **Implemented · dalga 5-7** (Area Blast, Smoldering Remains; `RareWave5ContentE2E`) · Area Blast kullanıcı fikri, Smoldering Remains kabul; Chain Ember kaldırıldı (kullanıcı, 2026-10-10). O4 Ember DoT'unu 5 → 6 sn yapar, patlama ~%20 büyür | **Implemented · Rare · dalga 4 (çekirdek, `RareStatusRulesE2E`), dalga 5-7 (ladder, `RareLadderModulesE2E`)** · kullanıcı Rare için uygun buldu · stack sıfırlama Thermal Shock Coil koşulunu bozar; kullanıcı bu çatışmayı kabul etti (2026-10-08) · ana patlama yalnız hedefe vurur (katalog metni hedefin patladığını söyler, alan belirtmez); alan hasarı yan özellik olarak 200 birim önerildi (kullanıcı, 2026-10-10) |
| Kinetic Breaker | Offense · `■■` (2-3) | *Spend Kinetic stacks to pierce Armor.* While the target has 4 Kinetic stacks, your next direct hit consumes all 4 stacks and ignores the target's Armor completely. | Zırh yok sayan vuruşa hasar çarpanı ×1,10 → 1,15 → 1,20 (+5 puan/seviye, tüm boyutlarda aynı; kullanıcı onaylı 2026-10-10; çarpan özgün kart metnine eklenmiş yeni parçadır) | *Hair Trigger:* normal düşmanda 3 stack'te tetiklenir (elit/boss 4) · *Armor Rend:* zırh yok sayan vuruştan sonra hedef 4 sn %25 daha az Armor'a sahip · Ortak: R5, O2, F1 · **Implemented · dalga 5-7** (Hair Trigger, Armor Rend) · Shrapnel Burst ve Quick Load reddedildi; yenileri kabul · değeri düşman Armor'una bağlı | **Implemented · Rare · dalga 4 (çekirdek, `RareStatusRulesE2E`), dalga 5-7 (Hair Trigger, Armor Rend)** · kullanıcı beğendi (2026-10-08) · değeri düşman Armor'una bağlı; düşman Armor değerleri ayarlanacak |
| Giant Slayer | Offense · `■■` (2-3) | *The bigger they are, the harder they fall.* Against a target whose hull MaxHealth is at least 1.5× your hull MaxHealth, damage you deal is increased; the larger the gap, the larger the bonus. Lower threshold 1.5×, no upper cap. Öneri formül: ×(1.10 + 0.05 × (oran − 1.5)) → 1.5× = ×1.10, 3× = ×1.175, 10× = ×1.525. | Her seviyede +0,05 (kullanıcı, 2026-10-10); Claude yorumu: taban çarpan ×1,10 → 1,15 → 1,20, eğim 0,05/oran sabit; ana özellik tüm boyutlarda aynı | *Toppler:* hull MaxHealth'i seninkinin ≥2,5 katı hedef ilk vuruştan sonra 2 sn %30 yavaşlar; boss'ta %15 (kullanıcı, 2026-10-10) · *Underdog:* seninkinden büyük MaxHealth'li düşmanlar sana ×0,92 hasar verir · Ortak: R3, O5, F1 · **Implemented · dalga 5-7** (Toppler, Underdog) · Kabul | **Implemented · Rare · dalga 4 (oran tabanlı hasar, `RareRatioRulesE2E`), dalga 5-7 (Toppler, Underdog)** · kullanıcı yeniden tanımladı (2026-10-08): elit/boss %8 kalkan/Armor atlama yerine hull MaxHealth oranına bağlı hasar; alt sınır 1.5×, üst sınır yok · formül eğimi açık |
| Trophy Core | Offense · `■■` (2-3) · komşuluk | *Kills make the adjacent ability stronger for the rest of the run.* Each enemy killed by the adjacent Q/E/F/R ability permanently adds +0.4% damage for the run (elite +3%, boss +10%). Stacks are stored in the module: move the module next to another ability and the bonus moves with it. | Normal öldürme başına %0,40 → 0,45 → 0,50 (+0,05/seviye, kullanıcı onaylı 2026-10-10); elit ve boss oranı korunur (öneri: elit %3 → 3,4 → 3,75, boss %10 → 11,25 → 12,5); tüm boyutlarda aynı | *Bounty Mark:* her 25 stack'te komşu yeteneğin bekleme süresi kalıcı kısalır; kazanç adım başına azalır (kullanıcı, 2026-10-10; sayılar onaylı: %5 → 4 → 3 → 2 → 1, sonra %1 sabit, toplamsal; mutlak 1 sn tabanı geçerli) · *Trophy Wall:* stack'ler gemideki tüm Offense yeteneklere yarı değerde yansır · Ortak: O3, G1, O1 · **Implemented · dalga 5-7** (Bounty Mark, Trophy Wall) · Kabul · Trophy Core parçalanmaz (kullanıcı, 2026-10-10): F1 çıkarıldı, üçüncü ortak O1 (kullanıcı, 2026-10-10) | **Implemented · Rare · dalga 5-7 (kill sayacı, `RareTrophyCoreMechanicsE2E`; Bounty Mark, Trophy Wall)** · kullanıcı beğendi (2026-10-08; esin Shape of Dreams Divine Faith); aile Utility değil Offense (kullanıcı) · komşuluk kullanıcı kararıyla korunur · modül sökülünce stack'ler korunur mu: öneri olarak korunur, yeniden takılınca etkin; kod bunu yapar ve `RareTrophyCoreMechanicsE2E` sınar (kullanıcı onayı ayrıca açık) |
| Dying Star | Offense · `■■■` (2-3) | *Kills feed a star that goes supernova.* Each enemy that dies within 300 units of you charges the star; at 10 charges it explodes in a large Thermal blast around you and applies 4 Ignite stacks to every enemy hit; charges reset. | Patlama hasarı her seviyede +%15 (kullanıcı onaylı, 2026-10-10); ölüm sayısı (10) ve yarıçap tüm boyutlarda ve seviyelerde sabit (yarıçap öneri 350, mutlak hasar denge testiyle) | *Core Collapse:* patlamadan önceki 1 sn düşmanlar merkeze çekilir; boss muaf, elit çekilir (kullanıcı, 2026-10-10) · *Red Giant:* şarj için ölüm yarıçapı 300 → 450 · Ortak: O3, D3, F1 · **Implemented · dalga 5-7** (Core Collapse, Red Giant) · Kabul (kullanıcı, 2026-10-10) · Binary Star ve Stellar Wind reddedildi | **Implemented · Rare · dalga 5-7 (KillConfirmed şarj, patlama, Core Collapse, Red Giant; `RareDyingStarMechanicsE2E`)** · kullanıcı Süpernova yönünü seçti (2026-10-08; esin Nova Drift Dying Star; can harcayan aura sürümü bırakıldı) · **AÇIK / PLACEHOLDER:** taban hasar JSON 60/69/78 (2. seviye +%15, 3. seviye +%13; kart +%15 diyor); yarıçap 350 JSON; **KARAR (kullanıcı, 2026-10-12):** 60/69/78 placeholder şimdilik kalır, denge koşusunda ayarlanır (güncel ölçek, kullanıcı 2026-10-12: taban 60/69/78 + 0,40 × Energy Power; patlama anındaki canlı EP) |
| Escape Velocity | Movement · `■■` (2-3) | *Reach top speed and your shots go sonic.* After 2 sec of uninterrupted flight above 90% of max speed, the ship goes sonic: Primary projectiles travel twice as fast and pierce one target. Ends when you slow down. | Sonik durumda Primary hasarı +10% → +15% → +20% | *Afterglow:* yavaşlayınca sonik durum 1,5 sn sürer · *Slipstream Control:* sonik durumda dönüş kabiliyeti +%25 · Ortak: G4, R6, F1 · **Implemented · dalga 5-7** (Afterglow); **Implemented · Slipstream Control (2026-10-12):** Sonic Boom kaldırıldı; sonik durumda dönüş kabiliyeti ×1,25; yavaşlama/çıkarma/ölümde modifier temizlenir | **Implemented · Rare · dalga 4 (sustainedSpeedRules, `RareSustainedSpeedE2E`), Afterglow dalga 5-7** · kullanıcı beğendi (2026-10-08) · **Slipstream Control (kullanıcı, 2026-10-12):** Sonic Boom yerine sonik durumda +%25 dönüş kabiliyeti |
| Danger Pay | Utility · 1/2 hücre (boyut merdiveni) | Threat≥3 iken Scrap ve Gold kazancı+%25. | Açık | Özel: Hazard Dividend (+10 yüzde puanı,%25→%35), Hazard Cover (Threat koşulunda alınan hasar×0,92). Ortak G1/G5/Four Corners; havuz kabul. | Planned · sistem bağımlılığı: Threat sahibi ve koşullu Gold/Scrap ödül akışı yok. |
| Interest Ledger | Utility · sabit 1 hücre | Her nokta sonunda eldeki Gold’un %5’i eklenir; üst sınır yok. | Açık | Yok (sabit boyut; kullanıcı onayı, 2026-10-13). | Planned · sistem bağımlılığı: Nokta tamamlanma olayı/ödül akışı yok. |
| Release Valve | Utility · sabit 2 hücre | Utility kullanınca 600–900 yarıçap arası halka bir anlık Energy hasarı verir: %40 EP. İç bölge boş, itme yok. | Açık | Yok (sabit boyut; kullanıcı onayı, 2026-10-13). | Implemented · 2026-10-14; Game build ve ModuleReleaseValveE2E geçti. Yeni yanlarda seçilmemiş L2/L3 büyümesi eklenmedi. |
| Tempered Hull | Defense · 2/3 hücre (boyut merdiveni) | *Every defensive cast hardens you for the rest of the run.* Defense yeteneği her kullanıldığında bu koşu boyunca kalıcı Armor +1; toplam bir üst sınırı vardır. | Armor tavanı L1/L2/L3 +30/+40/+50 kabul. | Özel: Bastion Slam (%150 birikmiş bonus Armor Energy hasarı,300 yarıçap,5sn bekleme) / Field Medic (Defense kullanımında 3sn toplam %12 MaxHealth iyileşme,30sn bekleme; bekleme boyunca yenilenmez). Ortak D1/D3/D4; havuz kabul. | Implemented · 2026-10-14; Game build ve ModuleTemperedHullE2E geçti. Yeni yanlarda seçilmemiş L2/L3 büyümesi eklenmedi. |
| Speed Lance | Movement · 2/3 hücre (boyut merdiveni) | Movement kullanımından sonraki 1sn pencerede en yüksek hız ölçülür, pencere sonunda otomatik delici Energy mızrağı çıkar; hasar=hız×0,4; menzil900, hız1800/sn (uçuş0,5sn). Delme ana özellik, yan gerektirmez. | Ana hız katsayısı0,4; seçilmemiş seviye büyümesi yok. | Özel: Echo Lance (0,4sn sonra aynı yönde %50 hasarlı ikinci delici mızrak,ilk hız örneğini kullanır) / Charged Lance (ilk hedefe2 Electric stack,sonraki hedeflere yok). Ortak G2/G3/G4; havuz kabul. Maksimum3 hücrede tek yan açılır. | Implemented · 2026-10-14; Game build ve ModuleSpeedLanceE2E geçti. Yeni yanlarda seçilmemiş L2/L3 büyümesi eklenmedi. |
| Polar Shield | Defense · sabit 2 hücre | Kalkan kapasitesi %50 ön/%50 arka bağımsız havuza bölünür. Saldırı yönündeki havuz tükenirse gövde açılır; toplam kapasite artmaz. Yenileme daha eksik havuza, geçici katman ortak. | Normal kapasite50/50; seviye büyümesi seçilmedi. | Yok (sabit boyut). Crossfeed/Rear Guard ve ortak yan havuzu kullanılmaz. | Implemented · 2026-10-14; Game build ve ModulePolarShieldE2E geçti. Yeni yanlarda seçilmemiş L2/L3 büyümesi eklenmedi. |
| Gold Plating | Defense · sabit 3 hücre | Mevcut Gold Armor verir; harcama sonrası yeniden hesaplanır. Kabul edilen Rare dengesi: 10 Gold başına +1 Armor; tavan 60/75/90.  | 60 → 75 → 90 tavan;10 Gold başına +1 Armor. | Yok (sabit boyut). | Implemented · 2026-10-14; Game build ve ModuleGoldHullE2E geçti. Yeni yanlarda seçilmemiş L2/L3 büyümesi eklenmedi. |
| Shield Battery | Defense · boyut açık | Kalkan doluyken yenilenme ek rezerv biriktirir; gelen hasar önce rezervi tüketir. | Açık; eski Epic taslağındaki MaxShield %40 rezerv tavanı öneridir, Rare için henüz seçilmedi. | Boyut ve yan havuzu açık. | Planned · Rare kullanıcı seçimi (2026-10-23). Kalkan kırılması/yenilenme etkileşimleri, rezerv tavanı, gelişim ve boyut ayrıca dengelenecek; runtime yok. |
| Shield Rupture | Offense · boyut açık | Düşmanın kalkanını hasarla kırmak, yakın zamanda kaybettirilen kalkan miktarına bağlı Energy alan patlaması üretir; patlama başka modül patlaması tetiklemez. | Açık; eski Epic taslağındaki kaybedilen kalkanın %50’si kadar hasar öneridir, Rare gücü henüz seçilmedi. | Boyut ve yan havuzu açık. | Planned · Rare kullanıcı seçimi (2026-10-23). Yarıçap, geçmiş penceresi, kalkan katmanları, kaynak sahipliği, gelişim ve boyut açık; runtime yok. |

## Epic modüller · 15

Tamamı **planlı tasarım**. Epic normal nadirlik kurallarını izler; güçlü stat, koşul, tetik veya kısmi davranış değişimi içerebilir. Nadirlik ve ana mekanik seçimi, bütün sayıların veya boyutların onaylandığı anlamına gelmez. **Öneri** diye işaretlenen değerler kesin denge değildir. Boyut ve yan havuzu henüz kapanmamış kayıtlar aşağıda belirtilir.

### Primary · 2

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Turret Mount | 3 hücre | Primary menzildeki en yakın düşmana geminin yönünden bağımsız otomatik nişan alır; hasar ×0,80. | Gelişim ve yan sözleşmesi açık. Omni Thrusters ile sinerji. |
| Broadside Mount | Sabit 3 hücre | Primary öne ateş etmek yerine iki yandan ateş eder. | Her yanda %70 hasar öneri. Silah türlerine uyarlama ve gelişim açık; sabit boyutta yan yok. |

### Offense · 4

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Twin Reactor | Açık | AP ve EP’den düşük olanı yüksek olana eşitler; iki değeri toplamaz. | Gelişim/yan havuzu ve dönüşümlerin çözümleme sırası açık. Kendi bonusundan önceki değerlere bakması öneri. |
| Prism Core | 3 hücre taslağı | Her Offense kullanımı Thermal → Cryo → Electric → Kinetic döngüsünde ilerler; isabetler kendi statüsüne ek olarak o kullanımın statüsünü uygular. | Storm Capacitor özel yan rolü seçildi (2026-10-23); yan gücü, boyut merdiveni ve kalan havuz açık. Ek 1 stack öneri. Gelişim ve Frost Lock/Arc Discharge etkileşimleri açık; Thermal Shock Coil, Status Weaver ve Kinetic Breaker ile sinerji. |
| Blood Contract | 2 hücre taslağı | Offense kullanımı mevcut canın %6’sını harcar; kalkanı atlar, en az 1 can bırakır. Yeteneğinin hasar verdiği düşman 4 sn içinde kim tarafından öldürülürse öldürülsün MaxHealth’in %1’i iyileşir. Eksik canla Offense hasarı doğrusal artar; cooldown değişmez. | Hasar eğrisi tam canda ×1,20 → sıfıra yakın canda ×1,70 öneri. Gelişim ve yanlar açık. Glass Cannon Core ile düşük can kapasitesi playtest konusu; Grit Your Teeth’in %35 eşiğiyle etkileşim izlenecek. |
| Alternating Current | Açık | Aynı düşmana önceki doğrudan yetenek vuruşundan farklı hasar türüyle vurunca hasar artar; DoT tick’leri sırayı değiştirmez. | +%40 öneri. Geçmiş penceresi, Primary/modül hasarı kapsamı, gelişim ve yanlar açık. |

### Defense · 2

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Last Stand | 2 hücre taslağı | Ölümcül hasarda bir kez %50 canla diriltir; sonra modül bozuk ve etkisiz kalır. MaxShield cezası yoktur. | Dirilme canı %50 → %60 → %70. Boyut/yan sözleşmesi açık. Grit Your Teeth’ten farklı olarak ölümden diriltir. |
| Vital Alloy | Açık | MaxHealth’in bir oranını bonus Armor olarak verir; can ve Armor tüketmez. | %15 dönüşüm öneri: 400 MaxHealth → +60 Armor. Çözümleme sırası, gelişim ve yanlar açık; Armor Conversion/Rapid Alloy ile kombo. |

### Movement · 4

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Phase Slip | 2 hücre taslağı | Afterburner başlangıcında 0,35 sn düşman mermileri gemiden geçer; bu sırada Primary ve yetenek kullanılamaz. Ardından ilk Primary atışı alan patlaması yapar. Bekleme 6 sn. Geçilen her mermi patlamaya +%25 hasar ve +15 yarıçap ekler; sınır yok. | Taban patlama ×2 hasar/120 yarıçap öneri. Gelişim patlama hasarını/yarıçapını artırır; faz süresi ve bekleme sabit. Yanlar ve boyut sözleşmesi açık. |
| Assault Vector | Açık | Yakındaki düşmana doğru gerçek hareket hasar; ondan uzaklaşma Armor verir. Yanlamasına veya çok düşük hızda iki bonus da yoktur. | +%30 hasar / +50 Armor öneri. Hedef seçimi, yön/hız eşikleri, gelişim ve yanlar açık. |
| Pathfinder Array | Açık | Bölüm başına bir kez haritada yakın iki nokta arasında yeni rota bağlantısı açar; farklı ödüle ulaşma veya karşılaşmayı dolaşma olanağı sağlar. | Yakınlık, Threat maliyeti, rota sistemi entegrasyonu, gelişim ve yanlar açık. Epic seçildi; bölümde bir kez kullanım ilk taslakta öneri olarak da kaydedildiği için kesin kullanım sınırı ayrıca netleştirilecek. |
| Movement hız modülü (adı açık) | Açık | Yatay/dikey hareket puanı +3; takılı Movement yeteneği başına +1. Ayrıca takılı Movement modülleri arttıkça hareket puanı artar. Movement kullanımında yalnız bu modülün toplam puan katkısı ×2; gerçek hız azalan getiri formülünden çözülür. | Modül başına katsayı, kendini/sistem kabuklarını/parçaları sayma ve gelişim/yanlar açık. 5 sn pencere ve tekrar kullanımda süre yenileme/birikmeme öneri. |

### Utility · 3

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Cascade Reactor | Açık | Farklı ailelerden yetenekleri art arda kullanmak sonraki yeteneğin gücünü artırır; zincir devam ettikçe bonus büyür, pencere kaçınca sıfırlanır. | 3 sn pencere ve %20 → %40 → %60 öneri. Etkilenen güç değerleri, gelişim/yanlar ve Combo Sequencer ile ayrışma/birleştirme açık. |
| Confluence Core | Açık | Her farklı takılı modül ailesi Armor, Attack Speed, hareket hızı, MaxHealth ve Critical Chance dahil geniş stat paketine katkı verir. Aynı aile bir kez sayılır; beş ailede ayrıca ödül yoktur. | Stat miktarları/tam listesi, sistem ve slot kabuklarının sayımı, gelişim ve yanlar açık. Yalnız AP/EP veya beş aileye ek bonus veren eski taslak geçersiz. |
| Field Mastery | Sabit 2 hücre; yan yok | Her Utility yeteneği ayrı kullanım sayacıyla koşu boyunca kalıcı +1 seviye kazanır. Takılıyken Utility Scrap ile geliştirilemez. Önceden satın alınan ve modülle kazanılan seviyeler sökünce korunur. Modülün ayrı +2 tavanı yoktur; yeteneğin kendi azami seviyesi geçerlidir. | Modülün 4 seviyesinde eşik 15 → 14 → 13 → 12. Epic korunur (kullanıcı, 2026-10-23): geç elde edilirse kullanım biriktirme değeri azalacağı için Legendary önerisi alınmadı. Sök-tak koruması açık: kazanılan her seviyede gereken kullanımın 1 azalması fikrinin modül eşiğiyle birleşimi ve yalnız sayacı dolan yeteneğin seviye alması yorumu netleştirilecek. |

## Legendary modüller · 3

Tamamı **planlı tasarım**. Legendary oynanış kimliğini tamamen değiştirir. Her modül tek sabit boyutludur; **yan özellik ve hiçbir bölge bonusu almaz**. Birden fazla kategoriyi etkileyebilir. Seviye/loot ve birlikte takılma kuralları henüz belirlenmiş değildir.

| Modül | Aile · sabit boyut | Ana tasarım | Tamamlanacak kararlar |
|---|---|---|---|
| Second Hardpoint | Primary · 3 hücre | İkinci Primary Weapon takılmasına izin verir. Aynı anda yalnız biri ateş eder; tuşla aralarında geçilir. | Geçiş tuşu, geçiş beklemesi, şarjör durumu ve gelişim açık. Fare tekerleği öneri; Ctrl Twin Rack için ayrılmıştır. |
| Twin Rack | Utility · 3 hücre | Her Q/E/F/R slotu iki yetenek taşır; toplam 8. Normal tuş ilkini, Ctrl+tuş ikinciyi kullanır. Her yeteneğin cooldown’u ayrıdır; aynı slottaki iki yetenek aynı ailedeyse küçük set bonusu alır. | Set bonusunun +%10 hasar olması öneri. Gelişim açık. Shift Afterburner için korunur. |
| Blink Drive | Movement · 2 hücre | Afterburner yerine 3 kısa ışınlanma charge’ı verir; dolum Afterburner kapasitesinden türetilir. | Işınlanma menzili, dolum hesabı ve gelişim açık. Legendary kararı 2026-10-23 düzeltmesiyle güncellendi. |

## Seçilmiş, nadirliği belirlenmemiş modüller · 3

Bu üç tasarım **83 ana modül toplamına dahildir**; Epic15 veya Legendary3 listelerine dahil değildir. Parametresi açık olan seçilmiş bir modül, ertelenmiş ya da reddedilmiş sayılmaz.

| Modül | Sınıf · boyut | Seçilmiş yön | Tamamlanacak kararlar |
|---|---|---|---|
| Combo Sequencer | Yetenek modülü · boyut açık | Ardışık yetenek kullanımıyla güçlenen seri fikri tutuldu; Rare taslağı yeniden tasarlanacak. | Epic/Legendary seçimi ve yeni mekanik açık. Eski 10 sn içinde dört farklı Q/E/F/R → 5 sn ×1,5 cooldown recovery taslağı güncel onay değildir. Cascade Reactor ile ayrışma/birleştirme kararı bekler. |
| Ultimate Protocol | Yetenek modülü · 2 hücre taslağı | Yalnız R slotundaki yeteneği ultimate’e dönüştürür. | Nadirlik, gelişim ve yan sözleşmesi açık. Cooldown ×2 / etki ×1,6 öneri; farklı yeteneklerde etki ayrı çözümlenebilir. |
| Courier Hold | Ailesiz · boyut açık | Mağazada satılan mühürlü kargo gemide yer kaplar; sonraki bölüme ulaştırınca güçlü ödül verir. Geliştirilemez. | Nadirlik, ödül, hücre sayısı, ölüm/kargo kaybı ve yan sözleşmesi açık. |

## Yan özellikler

### Ana modülden yan özelliğe geçenler

| Özellik | Yeni yön | Güç / açık noktalar |
|---|---|---|
| Lucky Cast | Hunt Clock dört sürümü ortak yanı;G5 yerine | Luck+1; Q/E/F/R kullanımında Luck başına%3 ihtimalle kullanılan yeteneğin beklemesinin%25’i iade. Primary hariç. Yan seviyeleri bu tur artırılmadı. |
| Status Resonator | Arc Extender özel yanı | Uygulanan hasar statü süreleri×1,20; Frozen dahil, donma beklemesi hariç. O4 Arc havuzunda değil; süre artışı Thermal toplam DoT/Ember referansına yansır. |
| Arc Discharge | Arc Extender özel yanı | Maksimum2 hücrede açılır. Hedef5 Electric’e ulaşınca350 yarıçapta2 en yakın düşmana2stack; hedef başına4sn bekleme. Kaynak korunur; yayılma hasarsız ve zincirsiz. Yeni seviyeler seçilmedi. |
| Bastion Slam | Tempered Hull özel yanı; bağımsız modül değil | Tempered maksimum3 hücrede açılır. Defense kullanımında biriken bonus Armor’dan %150 Energy hasarı,300 yarıçap uygulandı. Alternatif özel Field Medic ve ortak D1/D3/D4 seçildi; 5sn bekleme ve runtime uygulandı; seçilmemiş seviye büyümesi yok. |
| Four Corners | Rastgele atanan ortak Utility yanı; ayrı ana modül değil | Dört Q/E/F/R farklı ailedeyse güç artışı. Uygun taşıyıcının ortak yan havuzundan rastgele atanır; Confluence Core’a sabitlenmez, Spare Capacitor’a atanmaz. ×1,10 kabul (eski ana ×1,15); Danger Pay ortak havuzu kabul. Hasarsız etki kapsamı/diğer taşıyıcılar açık; uygulanmadı. |
| Field Medic | Tempered Hull özel yanı | Defense kullanımında 3sn toplam %12 MaxHealth iyileşme;30sn bekleme tetikte başlar. Bekleme boyunca Defense kullanmak iyileşmeyi yenilemez veya süreyi sıfırlamaz. %4 eski taslak reddedildi; Uncommon %8 önerisi seçilmedi. |
| Storm Capacitor | Prism Core özel yan tasarımı; bağımsız Utility ana modül değil | Yetenek kullanımlarıyla biriken yük, sonraki uygun Primary isabetinde yıldırım/Electric olarak boşalır. Yan rolü seçildi (2026-10-23). Eski 4 yük/3 hedef/2 stack bağımsız Epic paketi aynen aktarılmaz; yan gücü, yük/tetik kapsamı, hedefler, gelişim ve taşıyıcının boyut merdiveni/diğer yan havuzu açık. Planned; runtime yok. |

### Onaylı özel havuzlar

Boyut merdiveninde maksimum boyutta havuzdan tek yan açılır; eksik seçenekler kendiliğinden doldurulmaz. Uygulama durumu ana katalogdadır.

| Taşıyıcı | Özel seçenek 1 |Özel seçenek 2 | Kabul edilen üç ortak |
|---|---|---|---|
| Overshield Layer | Shield Rush: tavan üstü geçici kalkan varken hareket hızı+%20; kalkan bitince biter, birikmez. | Reserve Shell: tavan üstü geçici kalkan varken alınan hasar×0,90. | D1 MaxHealth+25; D2 hasar alınca Armor+12/2sn; D4 regen×1,15. |
| Armor Conversion | Rapid Alloy: toplam Armor’un %10’u kadar AS yüzde puanı;80 Armor→+%8 AS, sürekli hesaplanır, Armor tüketilmez. | Balanced Output: ana bonusun %25’i düşük AP/EP’ye; eşitlikte ana AP/yan EP, geri besleme yok. | D1 MaxHealth+25; D2 hasar alınca Armor+12/2sn; D3 öldürünce %1 MaxHealth. |
| Tempered Hull | Bastion Slam: Defense kullanımında birikmiş bonus Armor’un %150’si Energy hasarı,300 yarıçap. 5sn ayrı bekleme kabul edildi. | Field Medic: Defense kullanımında 3sn toplam %12 MaxHealth,30sn bekleme; beklemede yenilenmez. | D1 MaxHealth+45; D3 öldürünce %1,75 MaxHealth; D4 regen×1,25. |
| Danger Pay | Hazard Dividend: Threat≥3 iken ana ekonomi oranı+10 yüzde puanı,%25→%35. | Hazard Cover: aynı koşulda alınan hasar×0,92. | G1 öldürünce+2Gold; G5 Luck+2; Four Corners: dört Q/E/F/R farklı ailedeyse güç+%10, hasarsız etki kapsamı açık. |
| Speed Lance | Echo Lance: ilk mızraktan0,4sn sonra aynı yönde %50 hasarlı ikinci delici mızrak; ilk hız örneği, yeni kullanım sayılmaz. | Charged Lance: ilk hedefe2 Electric stack; sonraki delinen hedeflere stack yok. | G2 AbilityHaste+7; G3 Afterburner kapasitesi+%18; G4 yatay/dikey hareket rating+1,4. |

### Kurallar ve ortak havuzlar

- Common’da yan yok. Sabit boyutlu modülde yan yok. Uncommon/Rare merdiveninde maksimum boyutta 1 yan; modül başına 2 özel + 3 ortak = 5 seçenek.
- **Spare Capacitor istisnası:** 1/2/3 boyut merdiveni yalnız ana charge özelliğini büyütür; maksimum boyutta da yan taşımaz.
- Yan düşerken rastgele atanır; küçük boyutta kilitli, büyüyünce açılır. Kayıt/yükleme ve yükseltme harcaması/yeniden seçim arayüzü henüz uygulanmış değildir.
- Üretilen mermi/patlama/sekme başka modül tetiklemez. Modül çarpanları sonra kritik sırası korunur; özel üretilen hasar sözleşmeleri kendi satırlarında.
- Ana güce sırf denge için üst sınır koymama kullanıcı yönü geçerlidir; belirtilen Tempered/Gold gibi özel tavan kararları ve teknik/stack sınırları ayrı değerlendirilir.
- F1 Rare ve üstünde: maksimum boyut tek hücrelere parçalanır, bitişik parçalar otomatik bağlanır; bölge bonusu orantılı, komşuluk parça başına. Trophy Core parçalanmaz. Spare için canlı F1 izni kaldırıldı; F1 dahil yan yoktur.
- Health Bridge/Spare/Trophy komşuluğu kenar paylaşan Q/E/F/R kabuklarıdır; Primary hariç. Yeni tasarımlarda komşuluk istisna kalır.

#### Ortak havuzlar

| Havuz | Kimlik | Özellik | Runtime durumu |
|---|---|---|---|
| Offense (O) | O1 | Attack Power +8 | Implemented |
| | O2 | Critical Chance +8% | Implemented |
| | O3 | Attack Speed +4% (sabit, tetiksiz) | **Kullanıcı kararı (2026-10-10):** süreli "öldürünce AS" kaldırılıyor çünkü zamanlı AS veren modül/yan özellik zaten var (Attack Rhythm, Critical Rhythm). Kodda uygulandı: `Shared.O3` artık sabit Owner.AttackSpeed +4 (3 seviyede de aynı), adı "Offensive O3". Sharp Scale ve Scatter Core O3 kullanır |
| | O4 | Your applied damage status durations +1 sec; includes Frozen, excludes freeze cooldown | Implemented |
| | O5 | Your first successful hit against each target ×1.15 | Implemented · blocked/zero damage does not consume it |
| Defense (D) | D1 | MaxHealth +25 | Implemented |
| | D2 | On taking damage, Armor +12 for 2 sec (does not stack; duration refreshes) | Implemented |
| | D3 | On kill, restore health equal to 1% of MaxHealth | Implemented |
| | D4 | Shield regeneration rate ×1.15 | Implemented |
| | D5 | Max Shield +8% | Implemented |
| | D6 | *Desperate Patch:* when a Defense ability is used while health is at or below 40%, restore health equal to 12% of MaxHealth | Planned · kullanıcı, 2026-10-13 · modül değil yan özellik (Defense havuzu) · %40 eşik ve %12 ajan önerisi |
| General (G) | G1 | On owner kill, +1 Gold for this run | Implemented · resets on new run |
| | G2 | Ability Haste +4 | Implemented |
| | G3 | Afterburner capacity +10% | Implemented |
| | G4 | Horizontal and vertical movement rating +0.8 | Implemented |
| | G5 | Luck +1 | Implemented |
| | G6 | *Homing Beacon:* returning to a completed node adds no Threat | Planned · kullanıcı, 2026-10-13 · yolculuk yan özelliği; yeniden ziyaret kuralı GDD'de açık karar olduğundan onunla birlikte kesinleşir |
| | G7 | *Wayside Repairs:* on arriving at a new node, restore health equal to 6% of MaxHealth (same for every node type) | Planned · kullanıcı beğendi, 2026-10-13 · modül değil yan özellik (ajan önerisi, bir hücreye değmeyecek kadar küçük) · %6 ajan önerisi; nokta türüne göre ikiye katlama istenmedi |
| | G8 | *Quartermaster:* shop prices −15%; one free reroll per shop | Planned · kullanıcı beğendi, 2026-10-13 · modül değil yan özellik (ajan önerisi) · −%15 ve yenileme ajan önerisi |
| | G9 | *Salvage Processor:* dismantling unwanted modules yields more Scrap | Planned · kullanıcı fikri, 2026-10-13 · modül değil yan özellik (ajan önerisi) · parçalama mekaniğinin varlığı, çarpan ve Scrap kaynağı açık |
| Zone (Z) · öneri | Z1 | *Overclocked Seat:* while fully inside its family's zone, this module's zone bonus gets +10 points (+25% → +35%; with Zone Overclocker 50% → 60%). Only this module | Planned · kullanıcı, 2026-10-08 · ana modülden ayrışsın diye zayıflatıldı |
| | Z2 | *Misfit Seat:* while outside its family's zone, this module gets +10% (with Misfit Wiring 25% → 35%). Only this module | Planned · kullanıcı, 2026-10-08 · ana modülden ayrışsın diye zayıflatıldı |
| Fragment (F) · yalnız Rare ve üstü | F1 | *Fragmentable:* at max size the module splits into single cells (3 → 1+1+1, 2 → 1+1) that can be placed anywhere on the grid; fragments placed side by side auto-connect. Zone bonus is proportional to fragments in the matching zone (2 of 3 in zone → 2/3 of the bonus). Adjacency modules count adjacency from every fragment with no extra cap | Çekirdek uygulandı; kullanıcı kararları (2026-10-08) |

#### Rare ortak ölçek ve Rare’e özel yanlar

| Kimlik | Uncommon (L1) | Rare (öneri) | Not |
|---|---|---|---|
| O1 | AP +8 | AP +14 | |
| O2 | Crit +8% | Crit +14% | Kritik güçlü: Targeting Computer +3/+5 ile birlikte düşünülmeli |
| O3 | AS +4% | AS +7% | Yeni sabit tanım |
| O4 | Statü süresi +1 sn | +1,5 sn | ×1,75 = 1,75; Status Resonator ile çakışmasın diye aşağı yuvarlandı |
| O5 | İlk vuruş ×1,15 | ×1,25 | |
| D1 | MaxHealth +25 | +45 | |
| D2 | Armor +12 (2 sn) | Armor +20 (2 sn) | |
| D3 | Öldürünce %1 can | %1,75 | |
| D4 | Kalkan yenilenme ×1,15 | ×1,25 | |
| D5 | Max Shield +8% | +14% | |
| G1 | +1 Gold | +2 Gold | Kullanıcı kararı (2026-10-10); Gold ekonomisi yok |
| G2 | Ability Haste +4 | +7 | |
| G3 | Afterburner +10% | +18% | |
| G4 | Hareket derecesi +0,8 | +1,4 | |
| G5 | Luck +1 | Luck +2 | Luck ayrık; Lucky Cast ve Luck Lens'e çarpan olur, dikkat |

| Kimlik | Özellik | Durum |
|---|---|---|
| R1 | *Guarded Strike:* Defense yeteneği kullanınca 4 sn Primary hasarı ×1,15 | **Implemented · dalga 2** (`RareSideFeaturesE2E`; canlı mermi dalga 4) · Kabul (kullanıcı, 2026-10-10); sayı öneri |
| R2 | *Reactive Plating:* kalkan kırılınca 4 sn Armor +30 (yeniden kurulum eşiği %25) | **Implemented · dalga 2** · Kabul (kullanıcı, 2026-10-10); sayılar onaylı (4 sn, Armor +30) |
| R3 | *Boss Hunter:* miniboss ve boss'a verdiğin hasar ×1,10 | **Implemented · dalga 1–2, fiilen işlemez** (oyunda boss sınıflı düşman yok) · Kabul (kullanıcı, 2026-10-10); sayı onaylı |
| R4 | *Full Charge:* kalkan tamamen doluyken verdiğin hasar ×1,08 | **Implemented · dalga 1–2** · Kabul (kullanıcı, 2026-10-10); sayı onaylı · Momentum Shield ve Continuous Field Generator ile etkileşir |
| R5 | *Status Weaver:* hedefte 2 veya daha fazla farklı statü varsa verdiğin hasar ×1,20 | **Implemented · dalga 1–2** · Kabul; kullanıcı ×1,20 veya ×1,25 dedi, ×1,20 seçildi (Claude): Thermal Shock Coil'in Thermal+Cryo ×1,30 ana özelliğini geçmesin |
| R6 | *Fleet Guard:* Movement yeteneği kullanınca 3 sn aldığın hasar ×0,85 | **Implemented · dalga 1–2** (gelen hasar penceresi, `RareDamageConditionsE2E`; `RareSideFeaturesE2E`) · Kabul (kullanıcı, 2026-10-10); sayılar onaylı |

Rare ortak değerler ×1,75 ölçek kararına ve ayrı Rare kimliklerine dayanır. O/D/G kodlarının Rare modüllerde Rare değerleri kullanılır. Z1/Z2 planlı; F1 çekirdeği uygulanmış, havuz tablosundaki tarihsel Planned ifadesi ilk karar kaydıdır. Yan seviye büyümesi ve yeni yanların nadirlik ölçeği ayrıca açık.

## Sistem modülleri

Bu kayıtlar ana modül sayımına dahil değildir; varsayılan ve varyant sistem kayıtlarıdır.

### Varsayılan sistemler

| Modül | Şekil | Aile / sistem rolü | Durum |
|---|---|---|---|
| Standard Shield Generator | `■■` | Defense · Shield | System · default |
| Standard Afterburner | `■■` | Movement · Afterburner | System · default |
| Health Readout | `■` | Utility · HealthReadout | System · default |
| Damage Readout | `■` | Utility · DamageReadout | System · default |
| Life Support Unit | `■` | Utility · LifeSupport | System · default |
| Flight Control Unit | `■■` | Movement · FlightControl | System · default |
| Primary Weapon | `■■` | Primary · PrimaryWeapon | System · default |
| Q Ability Slot | `■` | Utility · AbilitySlot1 · Ability Bay | System · default |
| E Ability Slot | `■` | Utility · AbilitySlot2 · Ability Bay | System · default |
| F Ability Slot | `■` | Utility · AbilitySlot3 · Ability Bay | System · default |
| R Ability Slot | `■` | Utility · AbilitySlot4 · Ability Bay | System · default |

### Sistem varyantları

| Modül | Şekil | Etki | Durum |
|---|---|---|---|
| Fast-Charge Generator | `■■` | Shield capacity ×0.80; shield regeneration ×1.50 | System · variant |
| Heavy Generator | `■■■` | Shield capacity +60; regeneration +30%; recharge delay −2 sec | System · variant |
| Extended Afterburner | `L` · 3 hücre | Afterburner capacity +50 | System · variant |

## Seçilmiş mekanikler ve değerlendirme kayıtları

Gruplama mevcut veya önceki tasarımın nadirliğine göredir. **Nadirlik önerisi**, kesin seçim değildir; depodaki bir kaydın eski nadirliği onu aktif havuza geri almaz. Bu bölümdeki kayıtlar 84 seçilmiş ana modül toplamına eklenmez. Seçilmiş ana modüllerin açık parametreleri kendi katalog satırlarında kalır.

### Common · inceleme

| Kayıt | Mekanik / seçenek | Durum ve açık kararlar |
|---|---|---|
| Cooling Circuit · yeni Common sürümü | Utility, 2 hücre, Ability Haste +8. | Seçilmemiş öneri; eski JSON sürümünden ayrı değerlendirilir. |
| Energy Coil · yeni Common sürümü | Utility, 2 hücre, Energy Power +18. | Seçilmemiş öneri; eski JSON’daki +24 değeriyle karıştırılmaz. |

### Uncommon · ertelenen ve kaldırılan kayıtlar

| Kayıt | Mekanik / seçenek | Durum ve açık kararlar |
|---|---|---|
| Scavenger Net | Utility; ödülsüz savaşların Scrap vermesi. Önceki taslak 1 hücre, normal savaş Scrap’inin %40’ı. | **Ertelendi.** Uncommon ve %40 öneri; bölüm/ödül tasarımında yeniden değerlendirilecek, şu an uygulanmayacak. |
| Cold Retort | Önceki Uncommon Defense modülü. | **Kaldırıldı.** Bağımsız ana ile Jarring Effect/Frozen Spread yanları silindi; başka taşıyıcıya geçiş kararı yok. |
| Hunt Fuel | Loot Shield özel yan özelliği. | **Kaldırıldı.** Yerine Kill Ward var. |
| Overshield Layer için seçilmeyen yanlar | Overcharge, Emergency Layer, Cryo Insulation. | **Seçilmedi.** Mevcut havuz Shield Rush/Reserve Shell + D1/D2/D4. Stable Reserve reddedildi. |

### Rare · depodaki ve geçersiz eski kayıtlar

| Kayıt | Mekanik / seçenek | Durum ve açık kararlar |
|---|---|---|
| Monoculture · eski bağımsız tasarım | En az üç Q/E/F/R aynı ailedeyse bekleme azaltma. | **Depoda.** Eski Rare ana yönü aktif değil; Triple Core bağlantısı iptal. Gelecekteki Epic yan seçeneği aşağıda ayrı bir rol olarak açıklanır; ikinci bir modül sayılmaz. |
| Polar Shield için reddedilen yanlar | Crossfeed / Rear Guard. | **Reddedildi.** Polar Shield sabit 2 hücre ve yansız. |
| Speed Lance için reddedilen yanlar | Piercing Wake / Slipstream; %60 delme taslağı. | **Reddedildi.** Delme ana özellikte sınırsız ve tam hasarlı; reddedilen %60 değeri ana özelliğe aktarılmadı. |
| Eski Field Medic değeri | Tempered Hull özel yanının önceki %4/3 sn taslağı. | **Reddedildi.** Güncel değer %12/3 sn +30 sn bekleme; yeni taşıyıcı Rare Tempered Hull’dür. Eski değer güncel Uncommon önerisi sayılmaz. |

### Epic · inceleme, ertelenen ve reddedilen adaylar

Bu grupta Epic **adaylığı** gösterilir; adaylar kesin Epic15 listesine dahil değildir.

| Kayıt | Mekanik / seçenek | Durum ve açık kararlar |
|---|---|---|
| Monochrome Core | En az üç aktif Q/E/F/R aynı hasar türündeyse o türün hasarı artar. İlk çarpan önerisi ×1,50. | **İnceleme.** Offense/Epic adayı; kullanıcı “olabilir” dedi, kesin seçim değil. Sayılar, boyut/gelişim ve çok tür hasarlı yetenek sayımı açık. |
| Retreat Beacon | Utility; bölüm başına bir kez savaştan önceki noktaya dönme, ödül almama ve Threat artırmama. Önceki boyut taslağı 2 hücre. | **Ertelendi.** Epic önerisi; savaştan kaçış sistemi yok, şu an eklenmeyecek. |
| Monoculture · gelecekteki yan rolü | Üç aynı aile koşulunu bir Epic modülün yanına taşıma olasılığı. | **Depoda.** Bağımsız Rare kayıtla aynı fikir; yeni ana modül değil. Taşıyıcı ve güç seçilmedi; eski ×0,85 ana değeri yeni yan onayı değildir. |
| Siege Core | Önceki ağır silah/Primary odaklı Epic fikri. | **Reddedildi.** Düşük hız tetiği ve sonraki atış sayımı yönleri bırakıldı. |

#### Epic havuzunu 14–15'e indirme değerlendirmesi · 2026-10-23

**Kullanıcı hedefi:** Epic havuzu 14–15 civarında olsun; kullanıcının daha sonra sunacağı yeni Epic fikirleri de değerlendirilecek. **Son seçim (2026-10-23): Field Mastery Epic kalır; Shield Battery ve Shield Rupture Rare, Storm Capacitor Prism Core özel yan tasarımı olur.** Epic18−3=15, Rare22+2=24, Legendary3 aynı, bağımsız Storm kaydı çıktığı için ana84−1=83. Broadside Mount taşıması ve Cascade ertelemesi seçilmedi. Nadirlik/rol seçimi yeni güç değerlerinin veya runtime uygulamasının onayı değildir.

| Mevcut Epic | Önerilen yön | Gerekçe ve korunacak sınır |
|---|---|---|
| Broadside Mount | Legendary adayı | Öne saldırıyı iki yana taşıyarak geminin çatışma geometrisini ve yönlenmesini belirgin değiştirir. Sabit3/yansız yapısı zaten uygun; Legendary olursa bölge bonusu da kalkar. Güç ve silah türü uyarlaması ayrıca çözülür. |
| Field Mastery | Epic korunur; Legendary önerisi alınmadı | Kullanıcı gerekçesi: geç elde edilirse kullanım biriktirmeye dayalı ilerlemenin değeri azalır. Sabit2/yansız ve mevcut kullanım/ilerleme mekanikleri korunur; açık koruma sözleşmeleri çözülmeden uygulanmış sayılmaz. |
| Shield Battery | Rare seçildi | Tam kalkan sonrası rezerv Rare Defense olarak korunur. Güç, boyut ve gelişim ayrıca dengelenir; eski %40 önerisi kesin Rare gücü değildir. |
| Shield Rupture | Rare seçildi | Kırılma patlaması Rare Offense olarak korunur. Hasar oranı/yarıçap/boyut yeniden değerlendirilir; eski %50 önerisi kesin Rare gücü değildir. |
| Storm Capacitor | Yan rolü seçildi; Prism Core özel yanı | Bağımsız Utility ana kaydı çıkar. Eski4yük/3hedef/2stack paketi yan role aynen taşınmaz; güç, yük/tetik kapsamı ve yıldırım parametreleri açık. Prism'in boyut merdiveni ve diğer yan havuzu da ayrıca seçilir. |

Güncel Epic çekirdek **15**: Primary2, Offense4, Defense2, Movement4, Utility3. Kullanıcının yeni fikirleri geldikten sonra 14–15 hedefi için havuz yeniden karşılaştırılacak; şimdiden yeni silme/taşıma yapılmaz. İlk beşli önerinin13 ve ek Cascade ertelemesinin12 sonucu, önceki koşullu taslaktı; kullanıcı Field Mastery'yi koruduğu ve Broadside kararını vermediği için uygulanmış sonuç değildir. Cascade Reactor/Combo Sequencer ayrışması açık kalır; erteleme veya birleştirme seçilmedi.

### Nadirliği belirlenmemiş mekanikler ve adaylar

Bu kayıtların nadirliği kaynaklarda kesinleşmediği için Common–Legendary gruplarına varsayımla atanmaz.

| Kayıt | Mekanik / seçenek | Durum ve açık kararlar |
|---|---|---|
| Heat Sink | Kalkanın emdiği hasar ısı biriktirir; sonraki Defense kullanımı ısıyı yakındaki düşmanlara Thermal hasar olarak boşaltır. | **Mekanik kabul edildi (2026-10-14).** Mevcut modülün ana/yan mekanizması olacak; taşıyıcı, nadirlik, katsayılar, menzil, Ignite stack’i, tüketim/korunma ve sök-tak kuralları açık. Runtime yok. |
| Blackglass Circuit | Q/E/F/R sonrası başarılı Primary atışı ek Energy verir ve son kullanılan yeteneğin bekleyen charge yenilenmesini kısmen geri alır. | **Utility önerisi.** Nadirlik, oran ve tek tüketim sınırı açık. |
| Critical Handoff | Primary kritik isabeti bekleyen Offense cooldown’unu azaltır. | **İnceleme.** Nadirlik ve sayılar açık. |
| Diğer reddedilen yanlar | Charged Plating, Heavy Caliber, Shield Burst, Stable Reserve. | **Reddedildi.** Güncel nadirlik/taşıyıcı varsayılmadan ret kaydı korunur. |

Hasar tipi yönü Cryo/Kinetic/Electric/Thermal/Energy ilişkilerini mevcut ana/yan mekaniklerle geliştirmektir; her mekanik yeni bağımsız modül değildir. Diğer eski fikirler [2026-10-13 arşivinde](../../archive/2026-10/MODULE_CATALOG_BEFORE_CLEANUP_2026-10-13.md), önceki tam düzen [2026-10-23 kopyasında](../../archive/2026-10/MODULE_CATALOG_BEFORE_REORGANIZATION_2026-10-23.md) korunur. Arşiv güncel statü kaynağı değildir.

### Eski JSON tanımları · 9

| Modül | Aile | Şekil | JSON etkisi | Durum |
|---|---|---|---|---|
| Piercing Tip | Primary | `■■` | Primary projectile Piercing Count +1 | Defined · review |
| Shot Accelerator | Primary | Domino (2) / L (3) | 2 cells: fire rate ×1.10; 3-cell L: ×1.14 | Defined · review |
| Attack Matrix | Offense | Domino (2) / I (3) | 2 cells: ability damage ×1.12; 3-cell I: ×1.16 | Defined · review |
| Overload Core | Offense | `■■`<br>`■■` | Ability damage ×1.25; cooldown ×1.10 | Defined · review |
| Hull Reinforcement | Defense | Domino (2) / L (3) | 2 cells: max health +40; 3-cell L: +55 | Defined · review |
| Maneuver Thruster | Movement | `■` | Horizontal movement rating +1.5 | Defined · review |
| Cooling Circuit | Utility | Domino (2) / L (3) | 2 cells: Ability Haste +8; 3-cell L: +11 | Defined · review |
| Energy Coil | Utility | `■■` | Energy Power +24 | Defined · review |
| Targeting Computer | Utility | Single (1) / domino (2) | 1 cell: Critical Chance +3; 2 cells: +5 | Defined · review |

<a id="kill-ward--2026-10-13"></a>
<a id="boyut-ve-denge-revizyonu--2026-10-13"></a>
<a id="uygulama-kanıtı-ve-kalan-işler"></a>

## Uygulama durumu

2026-10-13–14 kayıtları Common/Uncommon/Rare içinde **56 uygulanmış ana modül** gösterir. Epic, Legendary ve nadirliği belirlenmemiş seçilmiş tasarımlar planlıdır.

| Planlı ana modül | Eksik sistem sınırı |
|---|---|
| Survey Relay | Rota ödüllerinin modül ailesi bilgisi. |
| Silent Running | Seyahat sırasında Threat artışının sahibi. |
| Danger Pay | Threat koşulu ve Gold/Scrap ödül akışı. |
| Interest Ledger | Nokta tamamlanma olayı. |

[GalaxyRoute](../../../LightYearsGame/include/gameplay/galaxy/GalaxyRoute.h) yıldız/durak verisi bu sözleşmeleri tek başına karşılamaz. Loot dağılımı, modül kalıcılığı, yükseltme harcaması/yeniden seçim arayüzü, ekonomi/rota akışları ve Rare sunumu ayrı işlerdir. Danger Pay/Four Corners’ın hasarsız etki kapsamı açık; yeni yanların seçilmemiş L2/L3 büyümeleri eklenmedi, kabul edilen güçleri sabit tutuldu.

Build komutları, test tabloları ve paket geçmişi [Modül uygulama kanıtları](../../MODULE_IMPLEMENTATION_EVIDENCE.md) belgesindedir. 2026-10-14 kaydı Game build exit0 ve yedi focused E2E’de 1517 geçen kontrol içerir; bu düzenlemede yeniden çalıştırılmadı. Test varlığı oyuncunun modülü edinme, denge veya görsel kabul kanıtı değildir. Uygulama planları: [Common](../../MODULE_COMMON_IMPLEMENTATION_PLAN.md), [Uncommon](../../MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md), [Rare](../../MODULE_RARE_IMPLEMENTATION_PLAN.md).
