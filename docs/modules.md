# Modules

**Güncel kararlar: 2026-10-15.** Bu belge seçilen modül tasarımlarının sahibidir. Çalışan içerik [modules.json](../LightYearsGame/assets/content/data/modules.json) ve [module_side_features.json](../LightYearsGame/assets/content/data/module_side_features.json); genel kurallar [Modül Sistemi Tasarımı](MODULE_SYSTEM_DESIGN.md). **Belgedeki karar oyun içinde uygulanmış sayılmaz.** Önceki seçenekler ve tarihli sonuçlar [temizlik öncesi arşivde](archive/2026-10/MODULE_CATALOG_BEFORE_CLEANUP_2026-10-13.md); arşiv güncel karar kaynağı değildir.

## Kesinleşen güncel yön

| Konu | Güncel karar | Uygulama sınırı |
|---|---|---|
| Rare Utility | **7 bağımsız modül; Monoculture depoda.** | Aşağıdaki seçilen tasarım sayımı; canlı JSON henüz aynı dağılımda değil. |
| Lucky Cast | Hunt Clock dört sürümünün ortak yanı; G5 yerine | Luck+1, Luck başına%3 ihtimalle kullanılan Q/E/F/R beklemesinin%25’i iade; Primary hariç. |
| Cold Retort | Bağımsız aktif havuzdan kaldırıldı | Canlı ana ve iki özel yan kayıt kaldırıldı; başka taşıyıcı seçilmedi. |
| Status Resonator | Arc Extender ikinci özel yanı | Uygulanan hasar statü süreleri×1,20; Frozen dahil, donma beklemesi hariç. |
| Arc Extender / Arc Discharge | 1/2 merdiveni; Arc Discharge/Status Resonator özel, O1/O2/O5 ortak | 5 Electric’te350 yarıçapta2 yakın düşmana2stack; hedef başına4sn, zincir yok. |
| Monoculture | **Depoda; gelecekte Epic yan adayı olabilir** | Triple Core bağlantısı reddedildi; aktif ana/yan havuzunda değil. Taşıyıcı ve güç seçilmedi. |
| Tempered Hull / Bastion Slam | Tempered Hull 2/3 merdiveni; Bastion Slam özel yanı | Defense başına +1 Armor,30/40/50 tavan; Bastion5sn ve FieldMedic30sn uygulandı. |
| Stripped Hull | **Uncommon**, tek hücre, bağımsız | Uncommon geçişi ve %1,5/%1,75/%2 uygulandı; ContentId uyumu için eski Rare son eki korunur. |
| Four Corners | **Rastgele atanan ortak Utility yan; Danger Pay havuzunda** | Dört farklı Q/E/F/R ailesinde +%10 güç kabul; hasarsız etkilerin kapsamı açık, runtime yok. |
| Spare Capacitor | **Rare ana modül; yan özellik yok** | 1/2/3 hücre ve charge gelişimi korunur. Canlı özel/ortak yan izinleri kaldırıldı. |
| Danger Pay | **Rare Utility**, bağımsız | Planlı; çalışan Threat sahibi bulunmadı, Gold/Scrap cüzdanı tek başına yeterli değil. |
| Interest Ledger | **Rare Utility**, bağımsız; üst sınır yok | Yan yapma/6’lı Rare Utility seçeneği alınmadı. |
| Gold Plating | **Rare Defense**, bağımsız | 10 Gold/+1 Armor,60/75/90 tavan uygulandı; cüzdan harcaması sonucu güncel Armor yeniden hesaplanır. |
| Armor Conversion | **Uncommon Defense**, bağımsız | Yan yapılmayacak. |
| Polar Shield | **Rare Defense, sabit 2 hücre; yan yok** | Crossfeed/Rear Guard reddedildi; ana ön/arka kalkan %50/%50; yenileme daha eksik havuza, geçici kalkan ortak katman (2026-10-14). |
| Speed Lance | **Rare Movement, 2/3; delici vuruş ana özellik** | 900 menzil/1800 hız,1sn örnek,×0,4 Energy; Echo/Charged özel yanları uygulandı. |
| Survey Relay | **Uncommon Utility, sabit 1 hücre** | Sonraki iki rota ödülünün modül ailelerini gösterir; yan yok. |
| Field Medic | **Tempered Hull özel yanı; %12 MaxHealth/3sn, 30sn bekleme** | Defense kullanımında tetiklenir; bekleme boyunca yeniden kullanım iyileşmeyi yenilemez. Eski %4 taslağı reddedildi; seçilmiş %12/3sn +30sn uygulandı. |
| Shatter Shock | **Depoda; aktif ana veya yan değil** | Ana ve özel yan kayıtları ilk uygulama paketinde kaldırıldı. |
| Epic/Legendary Primary | Turret Mount, Broadside Mount, Second Hardpoint **3 hücrede şimdilik korunur** | Boyut değiştirme önerileri bu tur uygulanmayacak. |
| Hunt Clock | **Dört aile sürümü olan ayrı seri** | Aile çeşitlilik sayımında ayrı; içerik toplamında 4, mevcut aile/bölge kuralları korunur. |
| Yetenek modülü sınıfı | Combo Sequencer ve Ultimate Protocol | Utility değildir; yerleşim/bölge/kabuk sözleşmesi açık. |
| Confluence Core | Her farklı aileden geniş stat paketi; **beş ailede ekstra bonus yok** | Stat miktarları ve sistem kabuklarının sayımı açık. |
| Pathfinder / Silent | Movement / Movement | Planlı içerikler; Status Resonator Arc Extender özel yanı. |

**Boyut karakteri:** Offense/Defense genelde 2–3; Utility/Primary genelde 1–2; Movement genelde 1–2 ama 3 seçenekleri Utility’den daha yaygın. Bu bir genellemedir; gerekçeli istisnalar geçerlidir. Epic veya Legendary olması otomatik boyut artışı gerektirmez.

**Epic / Legendary sözleşmesi (kullanıcı, 2026-10-14):** Epic diğer normal nadirliklerin kurallarını izler; kimliği tamamen değiştirmesi gerekmez. Legendary kimliği tamamen değiştiren güçlü bir davranıştır; tek sabit boyutu vardır, yan özellik ve hiçbir bölge bonusu almaz, farklı kategorileri etkileyebilir. Tek boyut bir hücre anlamına gelmez. [Kural sahibi](MODULE_SYSTEM_DESIGN.md#61-epic--legendary-ayrımı--kullanıcı-kararı-2026-10-14). Mevcut Epic adaylarının nadirlikleri bu kararla otomatik değiştirilmedi; yeniden değerlendirme ayrı karardır. Legendary satırlarındaki 3 hücre korunur, yan sözleşmeleri aşağıda netleştirilmiştir. Runtime uygulaması değildir.

<a id="sayısal-envanter--2026-10-14"></a>

## Sayısal envanter — 2026-10-15

**Canlı içerik:** JSON’da **79 kayıt**: **56 ana modül** (Common 13, Uncommon 24, Rare 19), 14 sistem ve 9 eski tanım. JSON’da bulunma loot/denge/oynanış kabulü değildir.

**Seçilen tasarım havuzu: 83 ana modül.** 2026-10-14 sadeleştirmesinde 85 kayıttan Lucky Cast, Cold Retort, Status Resonator, Arc Discharge, Monoculture ve Bastion Slam bağımsız aktif listeden çıktı: 85−6=79. Bunlardan dördü aktif yan tasarıma dönüştü; Monoculture depoya alındı, Cold Retort kaldırıldı. Önceden Four Corners/Field Medic yan olmuş ve Shatter bağımsızdan çıkma yönü seçilmişti. Kullanıcı 2026-10-15'te Alternating Current, Shield Rupture, Vital Alloy ve Assault Vector'ü seçti: 79+4=83; Epic 13+4=17. Canlı JSON 56 aktif ana kaydı taşır. Epic dışındaki 60 seçilmiş ana modülün 56’sı uygulandı; dört sistem bağımlısı planlı. Genel seçilmiş havuz 56 canlı + 27 planlı aktif kayıt. Yeni Common önerileri, sistemler/eski tanımlar, yanlar, Review/Rejected ve ertelenenler bu sayımın dışındadır. Hunt Clock’un dört sürümü Uncommon toplamına dahil; ayrı seri sütunu aile karşılaştırması içindir.

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Hunt Clock serisi | Yetenek modülü | Ailesiz | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Common | 2 | 4 | 4 | 2 | 1 | 0 | 0 | 0 | 13 |
| Uncommon | 3 | 5 | 6 | 3 | 4 | 4 | 0 | 0 | 25 |
| Rare | 0 | 5 | 5 | 5 | 7 | 0 | 0 | 0 | 22 |
| Epic | 2 | 5 | 3 | 3 | 4 | 0 | 0 | 0 | 17 |
| Legendary | 1 | 0 | 0 | 0 | 1 | 0 | 0 | 0 | 2 |
| Epic / Legendary açık | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 1 |
| Nadirlik açık | 0 | 0 | 0 | 1 | 0 | 0 | 1 | 1 | 3 |
| **Toplam** | **8** | **19** | **18** | **14** | **17** | **4** | **2** | **1** | **83** |

### Canlı ana modüller · uygulanan 56

Hunt Clock serisi aile karşılaştırmasında ayrı sütundur; dört sürüm toplamda dahildir. Bu tablo seçilen 83 tasarım tablosunun yerine geçmez.

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Hunt Clock | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|
| Common | 2 | 4 | 4 | 2 | 1 | 0 | 13 |
| Uncommon | 3 | 5 | 6 | 3 | 3 | 4 | 24 |
| Rare | 0 | 5 | 5 | 4 | 5 | 0 | 19 |
| **Toplam** | **5** | **14** | **15** | **9** | **9** | **4** | **56** |

Canlı Rare Utility: Zone Overclocker, Misfit Wiring, Triple Core, Spare Capacitor, Release Valve. Danger Pay/Interest Ledger ile seçilen hedef7'ye ulaşacak.

### Rare Utility · seçilen 7

Zone Overclocker, Misfit Wiring, Triple Core, Spare Capacitor, Danger Pay, Interest Ledger, Release Valve. Önceki 8/Monoculture bağımsız seçimi son kullanıcı mesajıyla değişti; Monoculture depoda olduğu için burada sayılmaz. Survey Relay Uncommon olarak seçildi; bu yediye dahil değildir.

<a id="havuz-sadeleştirme-taslağı--2026-10-13"></a>

## Açık kararlar — kesin kararlarla karıştırılmayacak

| Konu | Henüz seçilmemiş yön |
|---|---|
| Storm Capacitor | Offense/Epic aile önerisi açık; mevcut plan Utility/Epic. |
| Common Utility | Az sayıda ekleme yönü var; Cooling Circuit 2 hücre/Haste+8, Energy Coil 2 hücre/EP+18 ajan önerileri. Seçilmedikleri için aktif ana havuz dışında. |
| Field Mastery | Rare/Epic tartışıldı; mevcut Epic Utility korunur. Sök-tak koruma sözleşmesi açık. |
| Açık boyutlar | Twin Reactor, Shield Battery, Cascade Reactor, Confluence Core, Pathfinder Array. Son boyut önerileri kesin seçim değildir. |
| Yeni yan entegrasyonu | Danger Pay havuzu sistem bağımlısı; Four Corners’ın hasarsız etki kapsamı açık. Diğer seçilmiş havuzlar uygulandı. Yeni yanların L2/L3 büyümesi seçilmediği için kabul edilen güçleri sabit tutulur. |

## Kabul edilen hasar tipi mekanikleri — taşıyıcı kararı bekleyenler

Kullanıcı yönü (2026-10-14): Cryo, Kinetic, Electric, Thermal ve Energy ilişkilerini tip başına 5–6 modüle tamamlamak için mevcut modüllerin ana/yan mekanikleri üzerinde çalışılacak; yeni bağımsız modül eklemek genel hedef değildir. Aşağıdaki mekanik kabulü, taşıyıcı veya runtime uygulaması kabulü değildir. Bu kayıtlar ayrı ana modül olarak envantere eklenmez.

| Mekanik | Kabul edilen davranış | Açık kararlar / uygulama |
|---|---|---|
| **Heat Sink** | Kalkanın emdiği hasar ısı biriktirir. Sonraki Defense yeteneği kullanımında biriken ısı yakındaki düşmanlara Thermal hasar olarak boşalır. Mevcut bir modülün ana veya yan mekanizmasına bağlanacak; bağımsız modül ekleme kararı yok. | **Mekanik kabul edildi · kullanıcı, 2026-10-14.** Taşıyıcı, ana/yan konumu, birikim ve boşalma katsayıları, menzil, Ignite stack uygulaması, birikimin tüketim/korunma ve sök-tak kuralları açık. **Planlı; runtime yok.** |

## Ana modül kataloğu

**Durumlar:** Implemented canlı içeriktir; Planned seçilmiş tasarımdır; Geçiş bekliyor canlı kayıt ile yeni kararın farklı olduğunu gösterir. Planlı tablolardaki eski İngilizce kart metni taslak davranışı anlatır. `■` tek hücre; `■■` iki hücre; `■■■` üç hücre. Sabit boyutta yan yok; merdivende en büyük boyutta rastgele atanmış tek yan açılır. Gelişim tablolarında belirtilen öneriler nihai denge onayı değildir.

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

### Rare · 22

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Durum |
|---|---|---|---|---|---|
| Zone Overclocker | Utility · `■` | *Supercharge one zone.* Other modules in the zone where this module is placed get a +50% zone bonus instead of +25%. No effect of its own. | 50% → 55% → 60% | Yok (sabit boyut). | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcı beğendi; tek hücre (2026-10-08; esin Last Epoch Idol Altar) · Z1 yan özelliğinden ayrışsın diye +45'ten +50'ye güçlendirildi (kullanıcı: ana buff + yan nerf) · Gövde'deki +%10'a etkisi: uygulanan varsayım ölçektir (Hull %10 → %20), kullanıcı onayı bekler |
| Odometer | Movement · `■` | *The farther you travel, the stronger you get.* Every 5,000 units traveled during the run permanently grants +1% damage and +1% movement speed for the rest of the run; no cap. Stacks are stored in the module. | Stack başına değer +1% → +1.2% → +1.4% (mesafe sabit) | Yok (sabit boyut). | **Implemented · Rare · dalga 3 (2026-10-10)** (mesafe sayacı oyunda çalışır; rota/yolculuk sistemi gerekmez) · kullanıcı beğendi; seviye ile birim başına % artar (2026-10-08) · oyunun yolculuk hissine bağlı · kullanıcı iyi buldu, Rare'de kalır (2026-10-10); yolculuk/rota sistemi yok: bağımlılık borcu |
| Silent Running | Movement · `■` | *Travel quietly between fights.* Threat gained while traveling between route nodes ×0.75. | Threat ×0,75 → 0,70 → 0,65 (kullanıcı onaylı, 2026-10-10) | Yok (sabit boyut). | Planned · sistem bağımlılığı: Threat artışını yazan seyahat sahibi yok. |
| Misfit Wiring | Utility · `■` | *Modules outside their own zone work better.* Modules placed in a zone that does not match their family get +25% to their main value (normally no bonus, no penalty) — as good as their own zone. | 25% → 30% → 35% | Yok (sabit boyut). | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcı beğendi (2026-10-08) · Z2'den ayrışsın diye +20'den +25'e güçlendirildi · Zone Overclocker ile bilinçli zıtlık · kullanıcı (2026-10-10): 1 hücre, sabit boyutlu · gemi genelinde, yalnız ana paket; Hull ve Ability Bay aile bölgesi sayılmadığı için Misfit'e tabi değildir |
| Momentum Shield | Movement · `■■` | *Your shield charges from speed, not time.* The shield no longer regenerates normally. Instead it recharges in proportion to your current movement speed: at max speed ×0.5 the normal regeneration rate at level 1; zero below 200 units/sec. Works during Afterburner. No recharge delay after damage. | Max hızdaki çarpan ×0.5 → ×0.7 → ×0.9; hız <200 ise yenilenme yok (kullanıcı, 2026-10-12) | Yok (sabit boyut). | **Implemented · Rare · dalga 2 (2026-10-10)** · kullanıcı fikri (2026-10-08); Rare/Epic açık, Claude önerisi Rare (Continuous Field Generator ile aynı ağırlıkta kural değişimi) · Continuous Field Generator ile aynı anda takılamaz; modül ekranı uyuşmazlık uyarısıyla iki modülü işaretler (kullanıcı, 2026-10-08) |
| Triple Core | Utility · `■■` | *Leave one slot empty to empower the others.* If at least one of Q/E/F/R is empty, direct damage of every remaining ability ×1.25; active with one, two or three empty slots. | Çarpan ×1,25 → 1,30 → 1,35 (×1,18'den yükseltildi; kullanıcı onaylı, 2026-10-10) | Yok (sabit boyut). | **Implemented · Rare · dalga 3 (2026-10-10; reboot sayımı dalga 4, `RareStaticDamageRulesE2E`)** · kullanıcının 2026-10-08 listesi · kullanıcı Rare'de iyi buldu (2026-10-10) · Claude notu: boşaltılan yetenek hasar veriyorsa 3×1.25 < 4×1.0, yani değer boş slota ne koyulduğuna bağlı; gerçek hasar dağılımıyla ölçülmeli |
| Continuous Field Generator | Defense · `■■■` | *Your shield never stops recharging.* Removes the shield recharge delay; the shield recharges continuously, even while taking damage, at 20% of the normal recharge rate. | Yenilenme normalin %20 → 35 → 45'i (üç seviye kullanıcı düzeltmesi, 2026-10-13) | Yok (sabit boyut). | **Implemented · Rare · dalga 2 (2026-10-10)** · kalkan-kırılma tetikleri (Shatter Current, Shatter Shock…) kalkan %25 dolduktan sonra yeniden kurulur, 0↔az döngüsü tekrar tetiklemez (kullanıcı, 2026-10-08: eşik Max Shield'in %25'i) · Grit/D4/Fast-Charge çarpanlarıyla test |
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

### Epic · 17

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Durum |
|---|---|---|---|---|---|
| Phase Slip | Movement · `■■` | *Slip through fire and answer with an explosive shot.* For the first 0.35 sec after Afterburner starts, enemy projectiles pass through the ship, but you cannot use abilities or fire the Primary weapon. Your first Primary shot after the phase becomes an explosive round that deals area damage around the impact. 6 sec cooldown. | Patlama hasarı/yarıçapı büyür; faz süresi ve bekleme sabit | Boyut/yan sözleşmesi açık. | Planned · kullanıcı faz sırasında ateş/yetenek kilidi ve sonraki merminin patlayıcıya dönüşmesini istedi, Epic önerdi; Claude katıldı (2026-10-08) · patlama hasarı, yarıçap: açık (öneri ×2, 120 birim) · geçilen mermi başına +%25 hasar ve +15 birim yarıçap, sınır yok (kullanıcı, 2026-10-08: %10 riskli yetenek için azdı; sorun çıkarsa sınır sonra eklenir) · temel ×2 hasar/120 birim öneri |
| Last Stand | Defense · `■■` | *Cheat death once.* On lethal damage, revive once with 50% health; the module then burns out (shown broken, no effect). | Dirilme canı 50% → 60% → 70% | Boyut/yan sözleşmesi açık. | Planned · kullanıcı Epic dedi (2026-10-08; esin Nova Drift Last Stand) · kullanıcı (2026-10-13): Max Shield cezası kaldırıldı; modülün işlevini kaybetmesi yeterli bedel · Grit Your Teeth ölümden dirilmez kuralıyla ayrı mekanik |
| Turret Mount | Primary · `■■■` · Epic | *Your primary weapon aims itself.* The Primary weapon automatically aims at the nearest enemy in range, independent of the ship's facing; damage ×0.80. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı beğendi (2026-10-08) · Omni Thrusters ile sinerji |
| Blink Drive | Movement · `■■` · Epic | *Afterburner becomes short-range teleports.* Afterburner is replaced by 3 short teleport charges; recharge is derived from Afterburner capacity. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı beğendi (2026-10-08) · menzil/dolum açık |
| Shield Battery | Defense · şekil açık · Epic adayı | Kalkan doluyken yenilenme ek rezerv biriktirir; gelen hasar önce rezervi tüketir. Rezerv tavanı önerisi Max Shield'in %40'ı. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı fikri ve Defense ailesini beğendi (2026-10-13); %40 öneri, sayı/boyut/gelişim ve kalkan-kırılma etkileşimleri açık |
| Cascade Reactor | Utility · şekil açık · Epic adayı | Yetenek kullandıktan sonraki 3 sn içinde farklı aileden kullanılan sonraki yetenek güçlenir; zincir sürerse bonus yükselir, pencere kaçarsa sıfırlanır. Öneri %20 → %40 → %60. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı fikri ve Utility ailesini beğendi (2026-10-13); süre/bonuslar öneri, etkilenen güç değerleri, boyut/gelişim açık; Combo Sequencer ile ayrışma veya birleştirme kararı bekler |
| Twin Reactor | Offense · şekil açık · Epic adayı | AP ve EP'den düşük olanı yüksek olana eşitler; birbirine eklemez. Tek güç yatırımını hibrit saldırılara taşır. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı beğendi (2026-10-13); boyut/gelişim açık. Eşitlemenin kendi bonusundan önceki AP/EP tabanına göre çözülmesi öneri; kaynak sırası ve diğer dönüşümlerle etkileşim henüz kesinleşmedi |
| Confluence Core | Utility · şekil açık · Epic adayı | Gemide bulunan her farklı modül ailesi tüm statlardan bonus verir: Armor, Attack Speed, hareket hızı, MaxHealth ve Critical Chance dahil geniş bir stat paketi. Aynı aileden çok modül yalnız bir aile sayılır. Beş aile tamamlanınca ayrıca bir bonus veya yeni etki açılmaz; her aile aynı stat paketine katkı yapar. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı düzeltmesi (2026-10-13): yalnız AP/EP veren paket yerine her şeyden stat veren Epic; eski aile başına AP/EP +8 ve beş ailede Armor +25/hız +%15 taslağı geçersiz. Stat başına miktarlar, tüm stat listesinin teknik kapsamı, boyut/gelişim ve varsayılan sistem/slot kabuklarının aile sayımına katılması açık; sistemleri dışarıda tutmak ajan önerisi, kullanıcı kararı değil |
| Prism Core | Offense · `■■■` · Epic adayı | *Every Offense cast shifts the spectrum.* Her Offense isabeti kendi statüsüne ek olarak sıradaki statüyü uygular (Thermal → Cryo → Electric → Kinetic → başa); her cast sıradaki statüye geçer. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı beğendi (2026-10-13) · Thermal Shock Coil, Status Weaver (R5) ve Kinetic Breaker ile sinerji · eklenen stack sayısı (öneri 1), Frost Lock/Arc Discharge zincirlenmesi ve boyut/gelişim açık |
| Blood Contract | Offense · `■■` · Epic adayı | *Pay hull for power, and fight harder the weaker you are.* Her Offense yeteneği mevcut canın %6'sını harcar (kalkanı atlar, ölümcül olamaz, en az 1 can bırakır). Yeteneğinin hasar verdiği bir düşman, o hasardan sonraki 4 sn içinde ölürse (öldüren kaynak fark etmez) MaxHealth'inin %1'i kadar can kazanırsın. Canın azaldıkça Offense hasarın artar: eksik can oranına doğrusal, tam canda ×1,20 → can sıfıra yakınken ×1,70. Bekleme süresi değişmez. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı kararları (2026-10-13): bedel %6; iade MaxHealth'in %1'i, mevcut can değil; 4 sn penceresinde yeteneğin düşmana hasar vermesi yeter, ölümü kimin yaptığı önemsiz; hasar eğrisi doğrusal (tam canda ×1,20 → ×1,70 öneri değerleri); bekleme ×0,85 kaldırıldı · **açık:** boyut/gelişim ve eğri sayıları (kullanıcı eğriyi onayladı, sayılar ajan önerisi) · Grit Your Teeth'in %35 eşiğiyle çakışmaması için eğri yumuşak olmalı · Glass Cannon Core ile birlikte MaxHealth düşük kalır, %6 bedel daha sert vurur (playtest notu) |
| Storm Capacitor | Utility · `■■` · Epic adayı | *Abilities charge a storm.* Her Q/E/F/R yeteneği kullanımı 1 yük verir (en fazla 4). 4 yükte bir sonraki Primary isabeti yıldırım düşürür: menzildeki 3 rastgele düşmana vurur, her birine 2 Electric stack uygular; yükler sıfırlanır. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı beğendi (2026-10-13) · Electric üretimi ve yeni varlık türü (yıldırım) · Arc Extender ve Arc Discharge ile sinerji · tetik "Primary isabeti" olduğundan silah tipinden bağımsız (beam tick'i, cone, wave dahil) · yıldırım hasarı, menzil, hedef seçimi (rastgele mi en yakın mı), boyut/gelişim ve yük sayısı ajan önerisi/açık |
| Field Mastery | Utility · `■■` **sabit** · Epic adayı | *Practice is progression.* Her Utility yeteneği kendi kullanım sayacını ayrı tutar. Bir Utility yeteneğini her N kez kullandığında o yetenek **kalıcı** olarak (koşu boyunca) +1 seviye kazanır; **üst sınır yoktur**. Bu modül takılıyken Utility yetenekleri **Scrap ile geliştirilemez**; seviye yalnız kullanımla gelir. | Eşik N modül seviyesiyle düşer: 15 → 14 → 13 → 12 | Boyut/yan sözleşmesi açık. | Planned · kullanıcı beğendi (2026-10-13) · kullanıcı kararları: sayaçlar yetenek başına ayrı; modül 2 hücre ve sabit boyutlu (merdiven ve yan özellik yok); **+2 sınırı ve "geçici" kaldırıldı:** seviye kalıcı ve sınırsız; eşik 15 ile başlar, 14 → 13 → 12'ye düşer; modül takılıyken Utility yetenekleri Scrap ile geliştirilemez · **yorum (ajan, doğrulama bekliyor):** kazanılan seviye yalnız sayacı dolan yeteneğe gider · **kullanıcı kararları (2026-10-13, ikinci tur):** yeteneğin kendi azami seviyesi geçerli kalır; modül takılmadan önce Scrap ile alınmış seviyeler korunur; modül sökülünce kazanılmış seviyeler **kalır** ve bunun için bir koruma eklenecek (kullanıcı örneği: her seviye alındığında gereken kullanım sayısı 1 azalır) · **açık:** koruma kuralının tam biçimi (bu azalma modül seviyesine bağlı 15 → 14 → 13 → 12 eşiğinin yerine mi geçer, yoksa onunla birlikte mi çalışır; ajan okuması: yetenek başına her kazanılan seviyede eşik 1 azalır) ve takıp sökmeyi sömürü yapmayacak biçimi · 4 seviyeli gelişim (diğer modüller 3 seviyeli) |
| Broadside Mount | Primary · sabit 3 hücre önerisi | Öne ateş yerine iki yandan ateş; her yanda normal hasarın %70’i öneri. Silah türlerine uyarlama açık. | Açık. | Yok (sabit boyut). | Planned · kullanıcı tarafından tutuldu; 10 Gold/+1 ve60/75/90 tavan seçildi; runtime geçişi sürüyor. |
| Alternating Current | Offense · boyut açık | Aynı düşmana önceki doğrudan yetenek vuruşundan farklı hasar türüyle vurunca hasar artar. DoT tick'leri sırayı değiştirmez. | Açık; başlangıç +%40 ajan önerisi. | Boyut ve yan havuzu açık. | Planned · kullanıcı beğendi (2026-10-15). Mekanik/Epic seçildi; güç, geçmiş penceresi, Primary/modül hasarı kapsamı, boyut ve gelişim henüz kesinleşmedi. Runtime yok. |
| Shield Rupture | Offense · boyut açık | Düşmanın kalkanını hasarla kırınca, ona yakın zamanda kaybettirilen kalkan miktarına bağlı Energy alan patlaması oluşur; modül patlaması yeni modül patlaması tetiklemez. | Açık; başlangıç %50 ajan önerisi. | Boyut ve yan havuzu açık. | Planned · kullanıcı beğendi (2026-10-15). Mekanik/Epic seçildi; oran, yarıçap, geçmiş penceresi, kalkan katmanları, kırılmanın kaynak sahipliği, boyut ve gelişim açık. Runtime yok. |
| Vital Alloy | Defense · boyut açık | MaxHealth'in bir oranı kadar bonus Armor verir; can veya Armor tüketilmez. | Açık; başlangıç %15 (400 MaxHealth → Armor+60) ajan önerisi. | Boyut ve yan havuzu açık. | Planned · kullanıcı beğendi (2026-10-15). Mekanik/Epic seçildi; katsayı, çözümleme sırası, boyut ve gelişim açık. Armor Conversion/Rapid Alloy ile kombo taslağı. Runtime yok. |
| Assault Vector | Movement · boyut açık | Yakındaki düşmana doğru hareket ederken hasar bonusu, ondan uzaklaşırken Armor bonusu verir; yanlamasına veya çok düşük hızda bonus yoktur. | Açık; başlangıç +%30 hasar / Armor+50 ajan önerisi. | Boyut ve yan havuzu açık. | Planned · kullanıcı beğendi (2026-10-15). Mekanik/Epic seçildi; hedef seçimi, yön ve hız eşikleri, bonus miktarları, boyut ve gelişim açık. Runtime yok. |

### Legendary · 2

İki modül de yalnız sabit 3 hücre boyutunda tasarlanır; yan özellik ve aile bölgesi/Gövde bonusu yoktur. Tablodaki gelişim alanının açık olması bu kuralları değiştirmez.

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Durum |
|---|---|---|---|---|---|
| Second Hardpoint | Primary · `■■■` · Legendary | *Mount a second primary weapon.* Breaks the one-per-category rule: a second Primary Weapon module can be installed (e.g. Fighter + Breacher shotgun). Only one weapon fires at a time; a key switches between them. | Açık | Yok; sabit 3 hücre, bölge bonusu yok. | Planned · kullanıcı kararı (2026-10-10): aynı anda değil geçişli · geçiş tuşu açık (öneri fare tekerleği; Ctrl Twin Rack'te) · geçiş beklemesi ve şarjör durumu açık |
| Twin Rack | Utility · `■■■` · Legendary | *Ability sets, Hwei-style.* Each Q/E/F/R slot holds 2 abilities (8 total). Q/E/F/R casts the first set; holding Ctrl casts the slot's second ability (Ctrl+Q). Each ability keeps its own cooldown. If both abilities in a slot share a family, both get a small set bonus. | Açık | Yok; sabit 3 hücre, bölge bonusu yok. | Planned · kullanıcı kararları (2026-10-10): slot başına 2 yetenek, Ctrl ikinci set (Shift=Afterburner ile çakışmasın), aynı aile set bonusu olsun · bonus değeri açık (öneri +10% hasar) |

### Epic / Legendary açık · 1

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Durum |
|---|---|---|---|---|---|
| Combo Sequencer | Yetenek modülü · Açık | İlk Rare taslağı: four different Q/E/F/R abilities within 10 sec → cooldowns recover ×1.5 for 5 sec. Kullanıcı fikri tuttu ama Rare için değil; Epic/Legendary olarak farklı tasarlanacak (2026-10-08). | Açık | Boyut/yan sözleşmesi açık. | Reworking · yeni tasarım bekliyor |

### Nadirlik açık · 3

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Durum |
|---|---|---|---|---|---|
| Pathfinder Array | Movement · şekil açık | *Open a shortcut.* Bölüm başına bir kez haritada birbirine yakın iki nokta arasında yeni bir rota bağlantısı açarsın: ulaşamadığın ödüle geçmek veya tehlikeli karşılaşmayı dolaşmak. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı fikri (2026-10-13) · modül (ajan önerisi, onay bekliyor: bölümde bir kez yapılan anlamlı harita hamlesi) · "yakın" tanımı, Threat maliyeti ve rota haritasına bağımlılık açık |
| Courier Hold | **Ailesiz** · hücre sayısı açık | *Carry sealed cargo to the next chapter.* Birkaç gemi hücresini kaplayan mühürlü kargo taşırsın; sonraki bölüme ulaştırınca güçlü bir ödüle dönüşür. Şimdiki modül alanından vazgeçip geleceğe yatırım yapmaktır. | **Geliştirilemez** | Boyut/yan sözleşmesi açık. | Planned · kullanıcı fikri ve kararı (2026-10-13): ailesiz, geliştirilemez, **mağazada satılır** · ödülün ne olduğu, hücre sayısı ve kargo kaybedilme/ölüm kuralı açık |
| Ultimate Protocol | Yetenek modülü · `■■` | *Your R ability becomes an ultimate.* R slotundaki yeteneğin bekleme süresi ×2, etkisi ×1,6. | Açık | Boyut/yan sözleşmesi açık. | Planned · kullanıcı beğendi, yalnız **tek slot (R)** olsun dedi (2026-10-13) · **kapsam uyarısı (kullanıcı):** bazı yetenekler farklı çalıştığı için "etki ×1,6" yeteneğe göre ayrı çözüm isteyebilir, iş büyüyebilir · çarpanlar ajan önerisi |


## Ana modülden yan özelliğe geçenler

| Özellik | Yeni yön | Güç / açık noktalar |
|---|---|---|
| Lucky Cast | Hunt Clock dört sürümü ortak yanı;G5 yerine | Luck+1; Q/E/F/R kullanımında Luck başına%3 ihtimalle kullanılan yeteneğin beklemesinin%25’i iade. Primary hariç. Yan seviyeleri bu tur artırılmadı. |
| Status Resonator | Arc Extender özel yanı | Uygulanan hasar statü süreleri×1,20; Frozen dahil, donma beklemesi hariç. O4 Arc havuzunda değil; süre artışı Thermal toplam DoT/Ember referansına yansır. |
| Arc Discharge | Arc Extender özel yanı | Maksimum2 hücrede açılır. Hedef5 Electric’e ulaşınca350 yarıçapta2 en yakın düşmana2stack; hedef başına4sn bekleme. Kaynak korunur; yayılma hasarsız ve zincirsiz. Yeni seviyeler seçilmedi. |
| Bastion Slam | Tempered Hull özel yanı; bağımsız modül değil | Tempered maksimum3 hücrede açılır. Defense kullanımında biriken bonus Armor’dan %150 Energy hasarı,300 yarıçap uygulandı. Alternatif özel Field Medic ve ortak D1/D3/D4 seçildi; 5sn bekleme ve runtime uygulandı; seçilmemiş seviye büyümesi yok. |
| Four Corners | Rastgele atanan ortak Utility yanı; ayrı ana modül değil | Dört Q/E/F/R farklı ailedeyse güç artışı. Uygun taşıyıcının ortak yan havuzundan rastgele atanır; Confluence Core’a sabitlenmez, Spare Capacitor’a atanmaz. ×1,10 kabul (eski ana ×1,15); Danger Pay ortak havuzu kabul. Hasarsız etki kapsamı/diğer taşıyıcılar açık; uygulanmadı. |
| Field Medic | Tempered Hull özel yanı | Defense kullanımında 3sn toplam %12 MaxHealth iyileşme;30sn bekleme tetikte başlar. Bekleme boyunca Defense kullanmak iyileşmeyi yenilemez veya süreyi sıfırlamaz. %4 eski taslak reddedildi; Uncommon %8 önerisi seçilmedi. |
| Shatter Shock | Depoda; aktif ana/yan değil | Bağımsız ana ve Stagger/Aftershock Wave özel kayıtları kaldırıldı; tarihsel taslak arşivde. |
| Hunt Fuel | Kaldırıldı; yerine Kill Ward uygulandı | Öldürmeyle Afterburner geri gelmez. Loot Shield: Kill Ward + Overflow Recovery, ortak D1/D2/D4. |

## Onaylı boyutlar ve yan havuzları — 2026-10-13

Son kullanıcı mesajı önceki yanıttaki itiraz edilmeyen önerileri kabul eder; önceki “ajan önerisi” etiketleri bu kapsamda güncellendi. Polar Shield artık sabit2/yansız; Survey Relay Uncommon/sabit1; Speed Lance delici vuruş ana özellik. Üç hücreli sürümlerde genel şekil L; iki hücre domino, tek hücre kare. Boyut seviye değildir; ana güç boyutlar arasında korunur, seviye geliştirir. Maksimum boyutta havuzdan tek yan açılır; eksik özel seçenekler kendiliğinden doldurulmaz. Bu tablo seçilmiş mekanikleri gösterir; uygulanma durumu ana katalog ve aşağıdaki kanıt bölümündedir.

| Taşıyıcı | Özel seçenek 1 | Özel seçenek 2 | Kabul edilen üç ortak |
|---|---|---|---|
| Overshield Layer | Shield Rush: tavan üstü geçici kalkan varken hareket hızı+%20; kalkan bitince biter, birikmez. | Reserve Shell: tavan üstü geçici kalkan varken alınan hasar×0,90. | D1 MaxHealth+25; D2 hasar alınca Armor+12/2sn; D4 regen×1,15. |
| Armor Conversion | Rapid Alloy: toplam Armor’un %10’u kadar AS yüzde puanı;80 Armor→+%8 AS, sürekli hesaplanır, Armor tüketilmez. | Balanced Output: ana bonusun %25’i düşük AP/EP’ye; eşitlikte ana AP/yan EP, geri besleme yok. | D1 MaxHealth+25; D2 hasar alınca Armor+12/2sn; D3 öldürünce %1 MaxHealth. |
| Tempered Hull | Bastion Slam: Defense kullanımında birikmiş bonus Armor’un %150’si Energy hasarı,300 yarıçap. 5sn ayrı bekleme kabul edildi. | Field Medic: Defense kullanımında 3sn toplam %12 MaxHealth,30sn bekleme; beklemede yenilenmez. | D1 MaxHealth+45; D3 öldürünce %1,75 MaxHealth; D4 regen×1,25. |
| Danger Pay | Hazard Dividend: Threat≥3 iken ana ekonomi oranı+10 yüzde puanı,%25→%35. | Hazard Cover: aynı koşulda alınan hasar×0,92. | G1 öldürünce+2Gold; G5 Luck+2; Four Corners: dört Q/E/F/R farklı ailedeyse güç+%10, hasarsız etki kapsamı açık. |
| Speed Lance | Echo Lance: ilk mızraktan0,4sn sonra aynı yönde %50 hasarlı ikinci delici mızrak; ilk hız örneği, yeni kullanım sayılmaz. | Charged Lance: ilk hedefe2 Electric stack; sonraki delinen hedeflere stack yok. | G2 AbilityHaste+7; G3 Afterburner kapasitesi+%18; G4 yatay/dikey hareket rating+1,4. |

**Reddedilen yanlar:** Charged Plating, Crossfeed, Rear Guard, Piercing Wake, Slipstream, Heavy Caliber, Shield Burst, Stable Reserve. Field Medic %4/3sn reddedildi; yeni %12/3sn ve30sn bekleme Tempered Hull için kabul edildi. Polar Shield sabit boyut olduğundan ortak yan da taşımaz. Speed Lance sınırsız,tam hasarlı delme ve hedef başına mızrak başına bir isabet kabul; reddedilen %60 yan kuralı ana özelliğe aktarılmadı. Monoculture depoda, Spare Capacitor yan almaz.

### Overshield Layer — havuz tamamlandı

Shield Rush kullanıcı tarafından kabul edildi: geçici tavan üstü kalkan varken hareket hızı+%20; kalkan bitince bonus biter, ayrı süre/yığılma yok. Özel havuz Shield Rush/Reserve Shell; ortak D1/D2/D4. Maksimum3 hücrede tek yan açılır. Overcharge/Emergency Layer/Cryo Insulation bu modül için seçilmedi; aktif havuzda değiller. Stable Reserve reddedildi. Runtime uygulandı; yeni yanların L2/L3 büyümesi seçilmedi, kabul edilen güç sabit.

## Yan özellik kuralları ve ortak havuzlar

- Common’da yan yok. Sabit boyutlu modülde yan yok. Uncommon/Rare merdiveninde maksimum boyutta 1 yan; modül başına 2 özel + 3 ortak = 5 seçenek.
- **Spare Capacitor istisnası:** 1/2/3 boyut merdiveni yalnız ana charge özelliğini büyütür; maksimum boyutta da yan taşımaz.
- Yan düşerken rastgele atanır; küçük boyutta kilitli, büyüyünce açılır. Kayıt/yükleme ve yükseltme harcaması/yeniden seçim arayüzü henüz uygulanmış değildir.
- Üretilen mermi/patlama/sekme başka modül tetiklemez. Modül çarpanları sonra kritik sırası korunur; özel üretilen hasar sözleşmeleri kendi satırlarında.
- Ana güce sırf denge için üst sınır koymama kullanıcı yönü geçerlidir; belirtilen Tempered/Gold gibi özel tavan kararları ve teknik/stack sınırları ayrı değerlendirilir.
- F1 Rare ve üstünde: maksimum boyut tek hücrelere parçalanır, bitişik parçalar otomatik bağlanır; bölge bonusu orantılı, komşuluk parça başına. Trophy Core parçalanmaz. Spare için canlı F1 izni kaldırıldı; F1 dahil yan yoktur.
- Health Bridge/Spare/Trophy komşuluğu kenar paylaşan Q/E/F/R kabuklarıdır; Primary hariç. Yeni tasarımlarda komşuluk istisna kalır.

### Ortak havuzlar

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

### Rare ortak ölçek ve Rare’e özel yanlar

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

## Sistemler ve eski JSON tanımları

Bu bölüm ana tasarım sayımına dahil değildir. Ability Bay ve güncel gemi yerleşimi sözleşmesinin sahibi [Modül Sistemi Tasarımı](MODULE_SYSTEM_DESIGN.md); runtime şekillerinin sahibi JSON’dır. Dört Q/E/F/R kabuğu Ability Bay’de tek sırada, yalnız birbirleriyle değişir; diğer modüller Bay’e giremez. Kilitli hücreler yerleşime kapalıdır.

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

### Common dışı JSON tanımları

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

## Ertelenenler, inceleme ve ret geçmişi

**Shatter Shock depoda (kullanıcı,2026-10-13).** Aktif yan olarak başka taşıyıcıya atanmadı.

**Monoculture depoda.** En az üç Q/E/F/R yeteneği aynı ailedeyse beklemeleri kısaltma fikri korunur; gelecekte Epic yan özelliği olabilir. Triple Core için uygun bulunmadı, bağlantı iptal. Aktif yan havuzunda veya ana sayımda değildir; taşıyıcı ve güç seçilmedi. Eski ×0,85 ana taslağı yeni yan gücü onayı değildir.

**Cold Retort kullanıcı düzeltmesiyle bağımsız aktif havuzdan kaldırıldı.** Canlı JSON ve Jarring Effect/Frozen Spread yanları kaldırıldı; başka modüle taşınacaklarına dair karar yok. Eski mekanik: hasar alınca 300 birimde 2/3/4 hedefe1 Cryo, 2 sn bekleme; tarihsel ayrıntılar arşivde korunur.

Scavenger Net ve Retreat Beacon kullanıcı kararıyla ertelendi; aktif havuzda değiller. Diğer kararsız/Review/Rejected fikirler ve eski seçenekler [temizlik öncesi arşivde](archive/2026-10/MODULE_CATALOG_BEFORE_CLEANUP_2026-10-13.md). Monochrome Core Review’dır; kesin kabul sayılmaz. Siege Core reddedilmiştir. Arşivdeki öneriler güncel statüleri değiştirmez.

<a id="kill-ward--2026-10-13"></a>
<a id="boyut-ve-denge-revizyonu--2026-10-13"></a>

## Uygulama kanıtı ve kalan işler

**Tamamlayıcı paket —2026-10-14:** Tempered Hull/Bastion/Field Medic, Speed Lance/Echo/Charged, Release Valve, Gold Plating, Polar Shield ve Stripped Hull geçişi uygulandı. Field Medic30sn ve Bastion5sn yeniden Armor hesabıyla sıfırlanmaz; hız mızrağı900 menzil/1800 hızla tüm hedefleri tam hasarla, her hedefe mızrak başına bir kez deler. Release halkasının sınırları600 ve900 dahil; iç/dış bölge hasarsız. Polar normal havuzları bağımsız, geçici katman ortak; kaynaksız/yönü belirlenemeyen hasar normal havuzları oranlı tüketir. Pozitif normal kalkan artışı daha eksik havuzu eşitler; kapasite değişimleri ve sökme toplam kalkan üretmez. Gold bonusu mevcut cüzdan üzerinden tam10’luk dilimlerden, tavanlı hesaplanır; `RunGold::TrySpend` yetersiz bakiyeyi reddeder. Gold Armor, Armor Conversion ve Rapid Alloy’a dahildir; mağaza akışı eklenmedi.

`cmake --build build --target LightYearsGame LightYearsModuleGoldHullE2ETests LightYearsModulePolarShieldE2ETests LightYearsModuleReleaseValveE2ETests LightYearsModuleSpeedLanceE2ETests LightYearsModuleTemperedHullE2ETests LightYearsModuleDefenseLayerE2ETests LightYearsModulePlanTransitionE2ETests --parallel 2` —vcvars64 14.38 sonrası `VSLANG=1033`, `_CL_=/Z7`; exit0. Aşağıdaki yedi focused E2E güncel runtime ile geçti; tam suite çalıştırılmadı. Test artefaktları somut mekanik/entegrasyon kanıtıdır, oyuncu dengesi veya görsel kabulü değildir.

| E2E | Geçen kontrol | Kanıt |
|---|---:|---|
| ModuleGoldHull | 98 | [JSON](../build/e2e-artifacts/ModuleGoldHull-2026-10-14.json) |
| ModulePolarShield | 97 | [JSON](../build/e2e-artifacts/ModulePolarShield-2026-10-14.json) |
| ModuleReleaseValve | 120 | [JSON](../build/e2e-artifacts/ModuleReleaseValve-2026-10-14.json) |
| ModuleSpeedLance | 229 | [JSON](../build/e2e-artifacts/ModuleSpeedLance-2026-10-14.json) |
| ModuleTemperedHull | 534 | [JSON](../build/e2e-artifacts/ModuleTemperedHull-2026-10-14.json) |
| ModuleDefenseLayer | 227 | [JSON](../build/e2e-artifacts/ModuleDefenseLayer-2026-10-14.json) |
| ModulePlanTransition | 212 | [JSON](../build/e2e-artifacts/ModulePlanTransition-2026-10-14.json) |

Toplam **1517** geçen kontrol. İlk test hataları kapalı collision mask, doğrudan maksimum-kalkan fixture ayarı, no-op Apply dönüşü ve float sınır örneklemesiydi; bu düzeltmeler üretim bug iddiası değildir. Son build sırasında başka Ninja ile nesne dosyası çakışmaları ve açık exe link kilidi yaşandı; ayrı test hedefleriyle devam edilip final Game linki tamamlandı.

**Epic dışı kalan4:** Danger Pay—Threat/koşullu Gold+Scrap ödülü; Silent Running—Threat seyahat artışı; Interest Ledger—nokta sonu olayı; Survey Relay—rota ödül ailesi verisi. [GalaxyRoute](../LightYearsGame/include/gameplay/galaxy/GalaxyRoute.h) yıldız ve konak duraklarını taşır, bu oyun sözleşmelerini karşılamaz. Bu modüller için sahte tetik veya yeni Threat/rota sistemi kurulmadı. Epic/Legendary ve nadirliği açık modüller bu uygulama kapsamının dışında planlıdır.

**İkinci paket —2026-10-13:** Armor Conversion2/3+RapidAlloy/BalancedOutput, Overshield2/3+ShieldRush/ReserveShell, StatusPrimer sabit2 uygulandı. Geçici modül kalkanı normal kalkan eksikken de6sn erir; tekrar aynı kaynak yenilenir, sökme başka yeteneğin kalkanına dokunmaz. StatusPrimer eşit stackte deterministik Thermal/Cryo/Electric/Kinetic sırası kullanır; mevcut stack tavanları korunur ve yeni modül tetiklemez. `cmake --build build --target LightYearsGame LightYearsModuleDefenseLayerE2ETests LightYearsModulePlanTransitionE2ETests --parallel 2`(vcvars/VSLANG1033),exit0. [Yeni227kontrol](../build/e2e-artifacts/module-defense-layer-2026-10-13.json) ve [önceki212kontrol](../build/e2e-artifacts/module-plan-transition-after-defense-2026-10-13.json),exit0. Yeni yan seviyeleri kabul edilen başlangıç değerinde sabit; seçilmemiş büyüme eklenmedi. Bastion5sn ve Speed sınırsız/tamhasar delme o paketten sonra uygulandı; güncel kanıt yukarıdadır.

**İlk tasarım geçişi —2026-10-13:** `cmake --build build --target LightYearsGame LightYearsModulePlanTransitionE2ETests --parallel 2`,vcvars64 sonrasıVSLANG=1033,_CL_=/Z7;exit0. `build/LightYearsGame/LightYearsModulePlanTransitionE2ETests.exe build/e2e-artifacts/module-plan-transition-2026-10-13.json`:exit0,[212 kontrol kanıtı](../build/e2e-artifacts/module-plan-transition-2026-10-13.json). Arc üç seviyede hedef/range/cooldown/no-chain/removal/küçükboyut kilidi; dört hasar statüsü/Frozen süresi/donma beklemesi; dört Hunt taşıyıcısında gerçek Lucky iadesi; Defense-only kalkan/Armor/expiry ve canlı katalog dönüşümü. Tam test suite çalıştırılmadı. Eski Lucky/Cold/Shatter ana-modül veya Spare yan beklentili tarihsel E2E senaryoları eski tasarım içindir; bu kanıt onların çalıştığı iddiası değildir.

Önceki tarihli canlı revizyon: Arc Extender 1 hücre; Double Feed 2 hücre; Death Burst 400 yarıçap/%15; Trail L1 30+%30 AP Kinetic (L2/L3 ajan35+%35/40+%40); Hunt Fuel yerine Kill Ward. Omni Uncommon. İlgili Game build ve focused E2E sonuçları:

- [Boyut/mayın/patlama](../build/e2e-artifacts/module-sizing-mines-burst-2026-10-13.json): 379 kontrol.
- [Hunt Fuel kaldırma](../build/e2e-artifacts/loot-shield-no-hunt-fuel-2026-10-13.json): 192 kontrol (Kill Ward eklenmeden önceki tarihli sınır).
- [Kill Ward](../build/e2e-artifacts/loot-shield-kill-ward-2026-10-13.json): 108 kontrol; üç seviye, küçük boyut kilidi, kill/refresh/no-stack/expiry/removal.

İlk tasarım geçişi uygulandı: Zone Cooler2,Arc1/2+Discharge/Resonator+O1/O2/O5,Hunt Clock G5 yerineLucky,Spare yansız,Barrier/BraceCommon. Lucky/Cold/Shatter bağımsız kayıtları ve Cold/Shatter/Spare özel yanları kaldırıldı. Armor/Overshield/StatusPrimer ikinci pakette uygulandı. Tempered/Speed/Polar/Release/Gold ve Stripped geçişi de uygulandı. Epic dışındaki dört bağımlı ana modül aşağıda açık olarak listelenir. Loot,kalıcılık,yükseltme harcaması,ekonomi/rota bağımlıları ve Rare sunumu ayrı işlerdir. Test/JSON varlığı oyuncunun edinme veya görsel kabul kanıtı değildir. Güncel uygulama sonuçları [Uncommon planında](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md) ve [Rare planında](MODULE_RARE_IMPLEMENTATION_PLAN.md).
