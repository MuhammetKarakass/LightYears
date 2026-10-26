# Modül kataloğu

**Son karar güncellemesi: 2026-10-24; kaynak denetimi: 2026-10-23.** Bu dosya seçilmiş modül tasarımlarının sahibidir. Canlı içerik [modules.json](../LightYearsGame/assets/content/data/modules.json) ve [module_side_features.json](../LightYearsGame/assets/content/data/module_side_features.json); sistem kuralları [MODULE_SYSTEM_DESIGN.md](MODULE_SYSTEM_DESIGN.md), teknik akış [MODULE_SYSTEM_ARCHITECTURE.md](MODULE_SYSTEM_ARCHITECTURE.md) içindedir. Tasarımın belgede seçilmiş olması runtime uygulaması veya oyuncu kabulü değildir. Son seçimler bu sohbet ile **Suggest epic modules beyond utility**, **Epic modül seviyesini tasarla**, **Adjust weapon and module balance** ve önceki modül düzenleme sohbetlerinden karşılaştırıldı; [karar kaynakları](MODULE_DESIGN_DECISIONS.md) ayrı tutulur.

## İçindekiler

- [Sayım ve okuma kuralları](#sayım-ve-okuma-kuralları)
- [Common](#common-modüller--13), [Uncommon](#uncommon-modüller--26), [Rare](#rare-modüller--23)
- [Epic modüller](#epic-modüller--17)
- [Legendary modüller](#legendary-modüller--4)
- [Nadirlik dışı özel eşya](#nadirlik-dışı-özel-eşya--1)
- [Yan özellikler](#yan-özellikler)
- [Sistem modülleri](#sistem-modülleri)
- [Açık/ertelenen tasarımlar, nadirliğe göre](#açık-ve-ertelenen-tasarımlar--nadirliğe-göre)
- [Eski JSON tanımları](#eski-json-tanımları--9)
- [Karar kaynakları ve kaldırılanlar](MODULE_DESIGN_DECISIONS.md)
- [Uygulama durumu](#uygulama-durumu-ve-kalan-işler)

<a id="sayısal-envanter--2026-10-13"></a>
<a id="sayısal-envanter--2026-10-14"></a>
<a id="sayısal-envanter--2026-10-15"></a>

## Sayım ve okuma kuralları

**83 seçilmiş ana modül:** 70 canlı ana tanım, 13 planlı ana tasarım. Common13, Uncommon26, Rare23, Epic17, Legendary4. Courier Hold ve Ultimate Protocol nadirlik dışıdır, sayıma dahil değil (Ultimate Protocol 2026-10-25 nadirliksiz yetenek modülü oldu). Resonant Barrel'in ailesi yerleşime göre değiştiği için "Ailesiz" sütununda sayılır. Hunt Clock dört ayrı içerik olarak sayılır; aile karşılaştırmasında ayrı seri sütunundadır. Sistemler, eski JSON tanımları, yan özellikler ve seçilmemiş adaylar ana sayıma dahil değildir.

**Canlı JSON kaydı:** 70 ana + 14 sistem + 9 eski tanım = 93. Bu, loot/denge/oynanış kabulü değildir. JSON sayımı bu denetimde yeniden okundu; runtime/E2E kanıtları 2026-10-13–14 tarihlidir.

- **Uygulandı / Implemented:** canlı eski/yürürlükteki davranış mevcut. **Planlı / Planned:** seçilmiş fakat uygulanmamış tasarım. **Geçiş bekliyor:** canlı davranış ile seçilmiş son yön farklı; ikisi aynı kabul edilmez.
- **Öneri:** sayı veya ayrıntı henüz seçilmedi. **Açık:** seçilmiş tasarımın tamamlanacak parametresi. Bunlar modülün ertelendiği anlamına gelmez.
- **İnceleme:** mekanik henüz seçilmedi. **Depoda/ertelendi:** aktif havuz dışında tutuldu. **Kaldırıldı/reddedildi:** güncel tasarımda kullanılmaz.
- Aynı modül kimliğinden yalnız bir takılı örnek olabilir; envanter kopyası ayrı konudur.
- Boyut seviye değildir. Sabit boyutta yan yok; merdivenin maksimum boyutunda rastgele atanmış tek yan açılır. Spare Capacitor yansız merdiven istisnasıdır. `■` bir hücredir.
- Offense/Defense genelde 2–3; Primary/Utility genelde 1–2 hücredir. Movement genelde 1–2, fakat 3 hücre seçeneği Utility’den daha yaygındır. İstisnalar geçerlidir; nadirlik otomatik boyut büyütmez. Üç hücreli seçenekler genelde L, iki hücre domino, tek hücre karedir; gerçek hücre koordinatları içerik JSON’undadır. Genel şekil yönü mevcut özel şekilleri kendiliğinden değiştirmez.

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Hunt Clock serisi | Yetenek modülü | Ailesiz | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Common | 2 | 4 | 4 | 2 | 1 | 0 | 0 | 0 | 13 |
| Uncommon | 4 | 5 | 6 | 3 | 4 | 4 | 0 | 0 | 26 |
| Rare | 0 | 6 | 7 | 5 | 5 | 0 | 0 | 0 | 23 |
| Epic | 2 | 4 | 3 | 4 | 3 | 0 | 0 | 1 | 17 |
| Legendary | 1 | 0 | 0 | 1 | 1 | 0 | 1 | 0 | 4 |
| **Toplam** | **9** | **19** | **20** | **15** | **14** | **4** | **1** | **1** | **83** |

### Canlı ana tanımlar · 70

Hunt Clock serisi aile karşılaştırmasında ayrı sütundur; dört sürüm toplamda dahildir. Bu tablo seçilen 84 tasarımın canlı alt kümesidir; açık geçişler ayrıca işaretlenir.

| Nadirlik | Primary | Offense | Defense | Movement | Utility | Hunt Clock | Toplam |
|---|---:|---:|---:|---:|---:|---:|---:|
| Common | 2 | 4 | 4 | 2 | 1 | 0 | 13 |
| Uncommon | 3 | 5 | 6 | 3 | 3 | 4 | 24 |
| Rare | 0 | 6 | 6 | 4 | 3 | 0 | 19 |
| Epic | 2 | 3 | 3 | 3 | 3 | 0 | 14 |
| **Toplam** | **7** | **18** | **19** | **12** | **10** | **4** | **70** |

Canlı Rare Utility: Triple Core, Spare Capacitor, Release Valve. Danger Pay/Interest Ledger ile seçilen hedef 5'e ulaşacak. Zone Overclocker ve Misfit Wiring 2026-10-24'te tasarımdan ve JSON'dan çıkarıldı.

## Common modüller · 13

### Sabit boyutlu · 13

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Uygulama / açık sınır |
|---|---|---|---|---|---|
| Heavy Armor Plate | Defense · 3 hücre · sabit | Armor +30. | Armor +30 → +34 → +38 (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Assault Core | Offense · 2 hücre · sabit | Attack Power +15. | AP +15 → +17 → +19 (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Focused Barrel | Primary · 2 hücre · sabit | Toplam Primary hasarı ×1,30. | ×1,30 → ×1,34 → ×1,38 (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Thrust Booster | Movement · 2 hücre · sabit | Yatay ve dikey hareket puanı +2,2. | +2,2 → +2,5 → +2,8 (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Attack Rhythm | Offense · 2 hücre · sabit | Offense kullanımında 3 sn Attack Speed +%15; birikmez, tekrar kullanım süreyi yeniler. | AS +%15 → %17 → %19 (3 sn sabit) (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Shatter Current | Defense · 2 hücre · sabit | Kalkan kırıldıktan sonra 5 sn Primary isabetlerine, isabet hasarının %30’u kadar ek Energy hasarı ekler; ek hasar kritik vurmaz. | %30 → %34 → %38 (5 sn sabit) (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Energy Feedback | Offense · 1 hücre · sabit | Energy hasarı verince 4 sn Energy Power +16; birikmez, süre yenilenir. | EP +16 → +18 → +20 (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Escape Reflex | Movement · 2 hücre · sabit | Yalnız oyuncu hasar alınca 2 sn hareket hızı ×1,25; oyuncunun verdiği hasar tetiklemez. Birikmez, süre yenilenir. | ×1,25 → ×1,28 → ×1,31 (2 sn sabit) (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Finishing Blow | Offense · 2 hücre · sabit | Vuruştan önce canı %25 veya altında olan hedefe hasar ×1,20. | ×1,20 → ×1,23 → ×1,26 (%25 eşik sabit) (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Zone Cooler | Utility · 2 hücre · sabit | Tek aile bölgesinde o ailenin yetenek cooldown’u ×0,79; Gövde’de aktif non-Primary yetenekler ×0,90. Burun’da etkisiz; kendi ailesi Utility. | Aile bölgesi ×0,79 → 0,77 → 0,75; Gövde ×0,90 → 0,89 → 0,88; etkin seviyeyle ölçeklenir (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Full Magazine | Primary · 2 hücre · sabit | Her şarjörde ilk 3 başarılı Primary atışının hasarı ×1,30. | ×1,30 → ×1,34 → ×1,38 (ilk 3 atış sabit) (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Barrier Recharge | Defense · 2 hücre · sabit | Defense kullanımında MaxShield’in %10’u kadar eksik kalkan doldurur; maksimumu aşmaz. | MaxShield %10 → %11,5 → %13 (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Brace Reflex | Defense · 2 hücre · sabit | Defense kullanımında 4 sn Armor +25; birikmez, süre yenilenir. | Armor +25 → +28 → +31 (4 sn sabit) (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |

## Uncommon modüller · 26

### Sabit boyutlu · 10

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Uygulama / açık sınır |
|---|---|---|---|---|---|
| Photonic Lens | Primary · 1 hücre · sabit | Primary, vuruştan önce hiçbir statüsü olmayan hedefe ×1,50 hasar verir; her isabette yeniden kontrol edilir. | ×1.50 → ×1.53 → ×1.56 | Yok (sabit boyut). | Uygulandı. |
| Reflection Shield | Defense · 2 hücre · sabit | Defense kullanımından sonra 5 sn, kalkanın emdiği hasarın %35/%40/%45’ini yakındaki düşmana yansıtır; kritik ve yeni modül tetikleri yok. **Seçilmiş geçiş:** Energy yerine Electric hasarı ve Electric stack üretimi. | Yansıma %35 → %40 → %45; yeni Electric stack miktarı/uygulama sınırı seçilmedi. | Yok (sabit boyut). | Uygulandı (2026-10-24): Electric hasar; yansıyan vuruş hedefe 1 Electric stack verir (ajan yer tutucusu). Yansıma, Impact Redirector azaltması öncesi kalkana gelen ham hasardan hesaplanır (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Health Bridge | Defense · 2 hücre · sabit · komşuluk | Kenar komşusu en fazla iki Q/E/F/R yeteneğinin her doğrudan vuruşuna oyuncu MaxHealth’inin %3’ü kadar aynı türde ek hasar verir; DoT hariç. Komşuluk yoksa etkisiz. | 3% → 3.5% → 4% | Yok (sabit boyut). | Uygulandı. |
| Frost Lock | Utility · 2 hücre · sabit | Normal hedef 4 Cryo stack’e ulaşınca 1 sn hareket ve saldırı kilitlenir; stack’ler ve yavaşlatma silinir. Hedef başına 8 sn bekleme; elit/miniboss/boss donmaya bağışık. | Donma 1,0 → 1,2 → 1,4 sn; bekleme her seviyede 7 sn (karar 2026-10-24, uygulama Sol'a bekliyor) | Yok (sabit boyut). | Uygulandı. |
| Double Feed Core | Utility · 2 hücre · sabit | Attack Power +15 ve Energy Power +23. | +15/+23 → +18/+28 → +21/+33 | Yok (sabit boyut). | Uygulandı. |
| Omni Thrusters | Movement · 2 hücre · sabit | İleri, geri ve yan itki eşitlenir. İleri itki ve hız tavanı %30 azalır; geri/yan aynı düzeye gelir. Önceki örnekte 650/190/270 → 455/455/455. | İleri cezası seviye başına 1–2 puan azalır (öneri 1,5): −30% → −28.5% → −27% (Fighter 455 → 465 → 475) | Yok (sabit boyut). | Uygulandı. |
| Stripped Hull | Utility · 1 hücre · sabit | Her boş gemi hücresi hasar bonusu verir; tavan yok. Uncommon; kabul edilen güç hücre başına %1,5/%1,75/%2. Erken boş gemi güçlü, doldukça bonus düşer; boş-hücre build’i geç oyunda da korunabilir. | Hücre başına %1,5 → %1,75 → %2 (kullanıcı onayı,2026-10-14). | Yok (sabit boyut). | Uygulandı. |
| Status Primer | Primary · 2 hücre · sabit | Primary isabeti hedefin en çok stack’li statüsüne +1 stack verir; hedef başına en fazla 1 sn’de bir. Statü yoksa etkisiz. | Hedef başına aralık 1,0 → 0,9 → 0,8 sn (karar 2026-10-24, uygulama Sol'a bekliyor). | Yok (sabit boyut). | Uygulandı. |
| Survey Relay | Utility · 1 hücre · sabit | Rota haritasında sonraki iki ödülün modül ailelerini gösterir. | Görülen düğüm sayısı; değerler açık. | Yok (sabit boyut). | Planlı; rota ödüllerinin modül ailelerini sağlayan veri sahibi yok. |
| Kritik Senkronizasyon | Primary · 1 hücre · sabit | Her 3 kritik Primary isabeti, kalan beklemesi en uzun Q/E/F/R yeteneğinden süre düşer; bütün aileler, komşuluk yok. Atış başına tek sayım (saçma/sekme çoğaltmaz); ışında kritik adımlar en çok 0,25 sn'de bir sayılır. | İade 1,0 → 1,2 → 1,4 sn | Yok (sabit boyut). | Planlı (karar 2026-10-24, uygulama Sol'a bekliyor). |

### Boyut merdivenli · 16

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Uygulama / açık sınır |
|---|---|---|---|---|---|
| Arc Extender | Offense · 1/2 hücre | Oyuncunun uyguladığı Electric sınırı 4’ten 5 stack’e çıkar. L1 gelen hasar bonusları stack sırasıyla %3/%6/%9/%13/%19; normal 4.stack’in %16 değeri yerine bu profil kullanılır. Süre/yenileme kuralları korunur. | 5th-stack bonus +19% → +21% → +23% | Özel Arc Discharge/Status Resonator; ortak O1/O2/O5. Maksimum2 hücrede tek yan. | Uygulandı. |
| Glass Cannon Core | Offense · 1/3 hücre | 1 hücrede AP +12 ve MaxHealth −%6; 3 hücrede AP +34 ve MaxHealth −%16. Can cezası diğer bonuslardan sonra uygulanır. | AP +17% / level; penalty is fixed | Critical Chance +10% while health ≤50% · half the MaxHealth penalty returns as Max Shield +6% · Ortak: O2, O4, O5 | Uygulandı. AP büyümesi bileşik (+%17/seviye) büyüme türüyle L4/L5'e uzar (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Loot Shield | Defense · 1/2/3 hücre | Her öldürme 1 hücrede MaxShield’in %2’sini, 2/3 hücrede %3’ünü doldurur; maksimumu aşmaz. | 1c 2% → 2.5% → 3%; 2c/3c 3% → 4% → 5% | On kill, damage taken ×0.90 for 2 sec; refreshes, does not stack *(Kill Ward)* · convert 50% of overflow shield into health · Ortak: D1, D2, D4 | Uygulandı. |
| Sharp Scale | Offense · 2/3 hücre | Critical Damage +0,20 (1,50→1,70) ve Critical Chance +%15. | Critical Damage +0.20 → +0.25 → +0.30 | On critical hit, AS +15% for 2 sec *(Critical Rhythm)* · Critical Chance +2% per point of total Luck *(Luck Lens)* · Ortak: O1, O3, O5 | Uygulandı. |
| Scatter Core | Offense · 2/3 hücre | Her Offense kullanımının ilk başarılı isabeti 3 güdümlü mermi çıkarır; farklı yakın hedefler öncelikli, hedef azsa aynı hedef tekrar seçilir. Her mermi o isabetin %15’i kadar hasar verir; hız600/sn, ömür3sn, statü/kritik yok. | 15% → 18% → 21% per missile | Once per use, 0.8 sec after first hit, a 100-radius explosion deals 25% of that hit *(Aftershock)* · 4th missile (may reuse a target) · Ortak: O1, O3, O4 | Uygulandı. |
| Thermal Shock Coil | Offense · 2/3 hücre | Vuruştan önce en az bir Thermal ve bir Cryo stack’i olan hedefe oyuncunun verdiği hasar ×1,30. | ×1.30 → ×1.35 → ×1.40 | Thermal and Cryo duration +2 sec · on their death, 400-radius Thermal explosion equal to 15% of target MaxHealth · Ortak: O1, O2, O5 | Uygulandı. |
| Ricochet Tip | Primary · 2/3 hücre | Her Primary atışının ilk başarılı isabeti 450 birimdeki başka düşmana uygulanan hasarın %25’iyle seker. Hedef yoksa rastgele yönde 450 menzilli gerçek mermi çıkar. Kaynak mermiyle doku, görsel ölçek ve gerçek hız aynı; hasar temas anında. Tek atışın pellet/hedefleri bir temel sekmeyi paylaşır; sürekli beam’de bir hasar adımı esas alınır. | 25% → 30% → 35% | Transfer one damage status stack · one additional bounce deals 15% · Ortak: O2, O4, O5 | Uygulandı. |
| Grit Your Teeth | Defense · 2/3 hücre | Gövde canı ≤%35 iken alınan hasar ×0,80; ölümden diriltmez. | ×0.80 → ×0.77 → ×0.74 | At the same threshold, +35 to whichever is higher: Attack Power or Energy Power (AP on a tie; use values before this bonus) *(Critical Circuit)* · shield regeneration ×2 at the same threshold · Ortak: D1, D3, D4 | Uygulandı. |
| Hunt Clock: Offense | Offense · 2/3 hücre | Her öldürme yalnız Offense yeteneklerinin kalan cooldown’undan %5 iade eder; Primary hariç. | 5% → 6% → 7% | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold · Ortak: G2,G3,Lucky Cast (Luck+1,%3/Luck ihtimal,%25 kullanılmış bekleme iadesi) | Uygulandı. |
| Hunt Clock: Defense | Defense · 2/3 hücre | Her öldürme yalnız Defense yeteneklerinin kalan cooldown’undan %5 iade eder; Primary hariç. | 5% → 6% → 7% | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold · Ortak: G2,G3,Lucky Cast (Luck+1,%3/Luck ihtimal,%25 kullanılmış bekleme iadesi) | Uygulandı. |
| Hunt Clock: Utility | Utility · 2/3 hücre | Her öldürme yalnız Utility yeteneklerinin kalan cooldown’undan %5 iade eder; Primary hariç. | 5% → 6% → 7% | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold · Ortak: G2,G3,Lucky Cast (Luck+1,%3/Luck ihtimal,%25 kullanılmış bekleme iadesi) | Uygulandı. |
| Hunt Clock: Movement | Movement · 2/3 hücre | Her öldürme yalnız Movement yeteneklerinin kalan cooldown’undan %5 iade eder; Primary hariç. | 5% → 6% → 7% | Every fifth kill refunds another 20% from the longest remaining cooldown in the same family · Hunt Bounty +1 Gold · Ortak: G2,G3,Lucky Cast (Luck+1,%3/Luck ihtimal,%25 kullanılmış bekleme iadesi) | Uygulandı. |
| Movement Rhythm | Movement · 2/3 hücre | Gerçek hız200’de AS+%3,5; 300’de+%7; 400’de+%10,5. Ara hızlarda alt basamak korunur; 200 altında sıfır. Afterburner dahil gerçek hız kullanılır. | 3.5 → 4.0 → 4.5 per complete 100-speed step, starting at 200 | Horizontal and vertical rating +2.9 *(Advanced Thrust)* · when speed drops, the AS bonus fades over 1.5 sec instead of disappearing immediately · Ortak: G2, G3, G5 | Uygulandı. |
| Trail Mines | Movement · 2/3 hücre | Başarılı Movement kullanımından sonra5sn, gerçekten gidilen her50birimde bir mayın bırakır; dururken/teleportta bırakmaz. Fünye1sn, yarıçap100; L1 Kinetic hasar30+%30AP, AP bırakılırken alınır. | Damage 30 + 30% AP → 35 + 35% AP → 40 + 40% AP (L1 kullanıcı; L2/L3 ajan gelişim seçimi); active limit 12 → 14 → 16 | +1 Cryo stack · active limit +4 and fuse 0.7 sec · Ortak: G1, G3, G4 | Uygulandı. |
| Overshield Layer | Defense · 2/3 hücre | Defense kullanınca MaxShield’in %20’si kadar tavan üstü geçici kalkan; 6sn içinde azalarak kaybolur. | Geçici kalkan MaxShield %20 → %23 → %26; 6 sn sabit (karar 2026-10-24, uygulama Sol'a bekliyor). | Özel: Shield Rush (tavan üstü geçici kalkan varken hareket hızı+%20) / Reserve Shell (aynı koşulda alınan hasar×0,90). Ortak D1/D2/D4; beş seçenek havuzu kabul. Maksimum3 hücrede tek yan açılır. | Uygulandı. |
| Armor Conversion | Defense · 2/3 hücre | Armor’un %40’ı bonus öncesi AP/EP’den yüksek olana eklenir; eşitlikte AP. Bağımsız ana modül. | Armor dönüşümü %40 → %45 → %50; kabul edilen seviye tablosu. | Özel: Rapid Alloy (Armor’un %10’u kadar AS yüzde puanı;80Armor→+%8 AS) / Balanced Output (ana dönüşüm bonusunun %25’i düşük AP/EP’ye, geri besleme yok). Ortak D1/D2/D3; havuz kabul. | Uygulandı. |

## Rare modüller · 23

### Sabit boyutlu · 10

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Uygulama / açık sınır |
|---|---|---|---|---|---|
| Odometer | Movement · 1 hücre · sabit | Koşuda gidilen her5000birim için bu koşu boyunca kalıcı +%1 hasar ve +%1 hareket hızı; tavan yok. Stack’ler modülde tutulur. | Stack başına değer +1% → +1.2% → +1.4% (mesafe sabit) | Yok (sabit boyut). | Uygulandı; gerçek uçuş mesafesini sayar, rota sistemi gerektirmez. |
| Silent Running | Movement · 1 hücre · sabit | Rota noktaları arasında seyahat ederken kazanılan Threat ×0,75. | Threat ×0,75 → 0,70 → 0,65 (kullanıcı onaylı, 2026-10-10) | Yok (sabit boyut). | Planlı; seyahat Threat artışının sahibi yok. |
| Momentum Shield | Movement · 2 hücre · sabit | Normal kalkan yenilenmesini kapatır; yenilenme gerçek hızla artar. L1 azami hızda normal yenilenmenin ×0,50’si, 200birim/sn altında sıfır. Afterburner sırasında çalışır; hasar sonrası gecikme yok. | Max hızdaki çarpan ×0.5 → ×0.7 → ×0.9; hız <200 ise yenilenme yok (kullanıcı, 2026-10-12) | Yok (sabit boyut). | Uygulandı · Rare. Continuous Field Generator ile birlikte takılamaz; uyuşmazlık ekranda gösterilir. |
| Continuous Field Generator | Defense · 3 hücre · sabit | Kalkan yenileme gecikmesini kaldırır; hasar alınırken de normal yenilenmenin %20’siyle sürekli doldurur. | Yenilenme normalin %20 → 35 → 45'i (üç seviye kullanıcı düzeltmesi, 2026-10-13) | Yok (sabit boyut). | Uygulandı. Kalkan-kırılma tetiği kalkan %25 dolunca yeniden kurulur; sıfır etrafında tekrar tetiklenmez. |
| Impact Redirector | Defense · 3 hücre · sabit | Kalkan varken Armor’un hasar azaltmasının %50’si kalkan hasarına da uygulanır. | Armor'un kalkana etkisi %50 → 60 → 70 (kullanıcı onaylı, 2026-10-10) | Yok (sabit boyut). | Uygulandı. Kalkanın daha az hasar emmesi Reflection Shield yansımasını da azaltır; bu etkileşimin tasarım kabulü açık. |
| Interest Ledger | Utility · 1 hücre · sabit | Her nokta sonunda eldeki Gold miktarının %5 kadarı eklenir; üst sınır yok. | Seviye başına +2 veya +3 yüzde puanı kullanıcı önerisi (2026-10-24). +2: %5/%7/%9; +3: %5/%8/%11. Ajan tercihi +2; katsayı henüz seçilmedi. | Yok (sabit boyut). | Planlı; nokta tamamlanma/ödül akışı yok. Gelişim seçenekleri runtime değildir. Seviye başına +2 puan seçildi: %5/7/9 (karar 2026-10-24). |
| Release Valve | Utility · 2 hücre · sabit | Utility kullanınca 600–900 yarıçap halkasında Energy hasarı; iç bölge boş, itme yok. L1 hasarı 50 +0,40×EP. | Kullanıcı seçimi (2026-10-24): sonraki seviye hasar formülü = önceki seviye formülü ×1,05 +5; %5 hem tabanı hem EP katsayısını büyütür, +5 yalnız tabana eklenir. L1/L2/L3: 50 +0,40×EP / 57,5 +0,42×EP / 65,375 +0,441×EP. EP katsayısı sabit kalmaz; her seviyede göreli %5 büyür, +5 yüzde puanı değil. | Yok (sabit boyut). | Uygulandı (2026-10-24): taban ve EP katsayısı tabloları 50/57,5/65,375 ve 0,40/0,42/0,441. Bölge seviyesiyle L4/L5'e çıkınca bileşik formül yerine doğrusal uzar (taban +7,875, katsayı +0,021/seviye); bu fark açık. Taban ve katsayı bileşik büyüme türüyle (×1,05 +5) L4/L5'e uzar (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Polar Shield | Defense · 2 hücre · sabit | Normal kalkan ön/arka %50/%50 bağımsız havuz; saldırı yönündeki havuz tükenirse gövde açılır. Yeni yön: seviyeye bağlı ek toplam normal kapasite, sonra %50/%50 paylaşım; geçici overshield değildir. Yenilenme daha eksik havuza, geçici katman ortak. | Kullanıcı seviye başına %3–%5 düşündü. Ajan önerisi L1/L2/L3 +%5/+%10/+%15; alternatif +%3/+%6/+%9. Bonus öncesi MaxShield200 için öneri toplam210/220/230, ön/arka105/110/115. Katsayı ve genel MaxShield/havuz kapasitesi uygulama ayrımı açık. | Yok (sabit boyut). | Uygulandı (2026-10-24): toplam normal kapasite ×1,05/1,10/1,15 (ajan önerisi), sonra %50/%50. Katsayı kesin değil. Kapasite +%4/8/12 seçildi (karar 2026-10-24, uygulama Sol'a bekliyor); önceki +%5/10/15 yer tutucusu geçersiz. |
| Shield Rupture | Offense · 2 hücre · sabit | Düşmanın kalkanını hasarla kırmak, yakın zamanda kaybettirilen kalkan miktarına bağlı Energy alan patlaması üretir; patlama başka modül patlaması tetiklemez. | Açık; eski Epic taslağındaki kaybedilen kalkanın %50’si kadar hasar öneridir, Rare gücü henüz seçilmedi. | Yok; sabit2/yansız kullanıcı seçimi (2026-10-24). | Uygulandı (2026-10-24): kalkanı sıfırlayan vuruş, son 3 sn'de sahibin kaybettirdiği kalkanın %50'si kadar Energy patlaması, yarıçap 150; seviye gelişimi yok. %50 eski öneri, yarıçap ve 3 sn pencere ajan yer tutucusu. Yarıçap 250, hasar kaybettirilen kalkanın %50/55/60'ı, 3 sn pencere (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Refleks Devresi | Defense · 2 hücre · sabit | Hasar alınca kalan bütün Q/E/F/R beklemelerinin bir oranı iade edilir; 3 sn iç bekleme; otomatik kullanım yok. | İade %15 → %18 → %21 | Yok (sabit boyut). | Planlı (karar 2026-10-24, uygulama Sol'a bekliyor). |

### Boyut merdivenli · 13

| Modül | Aile · boyut | Ana etki | Gelişim | Yan seçenekleri | Uygulama / açık sınır |
|---|---|---|---|---|---|
| Triple Core | Utility · 2/3 hücre merdiveni | En az bir boş Q/E/F/R slotunda kalan yeteneklerin doğrudan hasarı ×1,25; bir/iki/üç boş slotta etkin. Ana güç boyutlar arasında korunur; 3 hücrede tek yan açılır. | Çarpan ×1,25 → 1,30 → 1,35 korunur. | Maksimum boyutta tek yan; havuz henüz seçilmedi. Yedek Zamanlayıcı özel yan adayıdır, taşıyıcı/güç ayrı seçimdir. | Uygulandı (2026-10-24): 2/3 merdiveni, 3 hücre L. Yan havuzu henüz boş. Özel yanlar: Yedek Zamanlayıcı (her boş Q/E/F/R slotu bekleme yenilenmesini +%15 hızlandırır), Odak Ateşi (boş slot varken çarpanın yarısı Primary'ye de); ortak O1/G2/Sakin Nefes (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Gold Plating | Defense · 2/3 hücre merdiveni | Mevcut Gold Armor verir; harcama sonrası hesaplanır. 10 Gold başına +1 Armor; tavan60/75/90. Ana güç boyutlar arasında korunur; 3 hücrede tek yan açılır. | 60 → 75 → 90 tavan;10 Gold başına +1 Armor korunur. | Maksimum boyutta tek yan. Altın İskelet özel yan mekaniği seçildi: Gold Plating kaynaklı bonus Armor üzerinden MaxHealth. 1:1 oran öneri. İkinci özel Kasa İskontosu: Gold harcama maliyetini azaltır; %10 öneri. Üç ortak henüz seçilmedi. Darphane Kalkanı reddedildi. | Uygulandı (2026-10-24): 2/3 merdiveni, 3 hücre L; Altın İskelet (Gilded Frame) 1:1 uygulandı. Kasa İskontosu mağaza olmadığı için uygulanmadı. Ortak D1/G1/Uyumlu Yuva; Altın İskelet 1:1 onaylı (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Spare Capacitor | Utility · 1/2/3 hücre · komşuluk | 1/2/3 hücre sırasıyla1/2/3 komşu Q/E/F/R yeteneğine ek charge verir. Ek charge miktarı modül seviyesinde+1/+2/+3; cooldown değişmez, aynı yeteneğe parçalar üzerinden yığılmaz. | Komşu yetenek sayısı boyutla 1 → 2 → 3; verilen charge sayısı seviye ile artar, boyutla artmaz; boyut yalnız kaç yeteneğe verileceğini artırır (kullanıcı, 2026-10-10; her seviyede +1 charge: L1 +1, L2 +2, L3 +3; parçalar yığılmaz, bir yetenek bu modülden en çok seviyesinin charge'ını alır) | **Yok — kullanıcı kararı, 2026-10-13.** Boyut merdiveni yan açmaz; özel/ortak yan almaz. | Uygulandı; hiçbir özel/ortak yan veya F1 almaz. Boyut yalnız komşu yetenek sayısını artırır. |
| Ember Accumulation | Offense · 2/3 hücre | Thermal 4 stack’e ulaşınca stack’ler tüketilir; Ignite DoT yerine hedefe, tam temel DoT bütçesinin %130/%140/%150’si kadar tek Thermal patlaması vurur. Bekleyen DoT iptal olur; ana patlama alan hasarı vermez. | Patlama %130 → %140 → %150; bütün boyutlarda aynı. Mevcut formül temel Thermal süresi × süre çarpanını kullanır; O4’ün toplamsal süre bonusunu içermez. | Area Blast: 200 yarıçapta diğer hedeflere ana patlamanın %60’ı, stack vermez. Smoldering Remains: 150 yarıçapta 3 sn, saniyede 1 Thermal stack. Ortak O4/O5/F1. | Uygulandı. Stack tüketimi Thermal Shock Coil koşulunu bozabilir; bu çatışma kabul edildi. O4’ün +1,5 sn’sinden ek patlama hasarı çıktığı iddiası kaldırıldı. |
| Kinetic Breaker | Offense · 2/3 hücre | Hedefte4 Kinetic stack varken sonraki doğrudan vuruş dört stack’i tüketir ve hedef Armor’unu tamamen yok sayar. | Zırh yok sayan vuruşa hasar çarpanı ×1,10 → 1,15 → 1,20 (+5 puan/seviye, tüm boyutlarda aynı; kullanıcı onaylı 2026-10-10; çarpan özgün kart metnine eklenmiş yeni parçadır) | Hair Trigger: normal düşmanda 3 stack, elit/boss’ta 4. Armor Rend: zırhı yok sayan vuruştan sonra 4 sn hedef Armor −%25. Ortak R5/O2/F1. | Uygulandı. |
| Giant Slayer | Offense · 2/3 hücre | Hedefin gövde MaxHealth’i oyuncunun en az1,5katıysa oran arttıkça hasar bonusu artar; üst sınır yok. Mevcut taslak çarpan ×(1,10+0,05×(oran−1,5)); oran1,5/3/10 için ×1,10/1,175/1,525. | Her seviyede +0,05 (kullanıcı, 2026-10-10); Mevcut yorum: taban çarpan ×1,10 → 1,15 → 1,20, eğim 0,05/oran sabit; ana özellik tüm boyutlarda aynı | Toppler: hedef MaxHealth ≥2,5× oyuncu MaxHealth ise ilk vuruş sonrası 2 sn %30 yavaşlatma; boss’ta %15. Underdog: oyuncudan büyük MaxHealth’li düşmandan hasar ×0,92. Ortak R3/O5/F1. | Uygulandı. Oran eğimi ve seviye başına +0,05’in taban çarpana uygulanması mevcut yorum; nihai formül kabulü açık. Formül (taban büyür, eğim 0,05 sabit, tavansız) kabul edildi; ortak havuzda R3 yerine O1 (karar 2026-10-24). |
| Trophy Core | Offense · 2/3 hücre · komşuluk | Komşu Q/E/F/R yeteneğiyle öldürme koşu boyunca kalıcı hasar biriktirir: normal+%0,4, elit+%3, boss+%10. Stack’ler modüldedir; farklı yeteneğin yanına taşınınca bonus da taşınır. | Normal öldürme başına %0,40 → 0,45 → 0,50 (+0,05/seviye, kullanıcı onaylı 2026-10-10); elit ve boss oranı korunur (öneri: elit %3 → 3,4 → 3,75, boss %10 → 11,25 → 12,5); tüm boyutlarda aynı | Bounty Mark: her 25 stack’te komşu cooldown’una toplamsal %5 → %4 → %3 → %2 → %1, sonra %1 azalma; mutlak 1 sn tabanı. Trophy Wall: stack’ler gemideki tüm Offense’a yarı değerle yansır. Ortak O3/G1/O1; F1 yok. | Uygulandı. Sökülünce stack’ler mevcut uygulamada korunur; bunun tasarım kabulü açık. Trophy Core parçalanmaz. Elit %3/3,4/3,75 ve boss %10/11,25/12,5 kabul; stack'lerin modülde korunması kabul (karar 2026-10-24). |
| Dying Star | Offense · 2/3 hücre | 300 birim içindeki uygun öldürmeler yıldızı yükler; 10 yükte oyuncu çevresinde Thermal patlama ve vurulanlara 4 Ignite stack, ardından yükler sıfırlanır. Mevcut yarıçap 350. | Mevcut hasar 60/69/78 + 0,40 × patlama anındaki EP. Tasarım yönü seviye başına +%15; placeholder tablo şimdilik korunur. Ölüm sayısı ve yarıçap boyut/seviye ile değişmez. | Core Collapse: patlamadan önce 1 sn çekim; boss muaf, elit çekilir. Red Giant: ölüm sayma yarıçapı 300 → 450. Ortak O3/D3/F1. | Uygulandı. 60/69/78 taban hasar placeholder olarak kullanıcı kararıyla korunuyor; her seviyede tam +%15 değildir. Taban bileşik +%15 seçildi: 60/69/79,35 + 0,40×EP (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Escape Velocity | Movement · 2/3 hücre | Azami hızın %90’ının üstünde kesintisiz2sn uçuş sonik durumu açar: Primary projectile hızı×2 ve+1 delme. Hız düşünce kapanır; Afterglow ayrı yanıdır. | Sonik durumda Primary hasarı +10% → +15% → +20% | Afterglow: yavaşlayınca sonik durum 1,5 sn daha sürer. Slipstream Control: sonikken dönüş kabiliyeti +%25; durum bitince/çıkarılınca temizlenir. Ortak G4/R6/F1. | Uygulandı; Sonic Boom yerine Slipstream Control seçildi. Beam/cone gibi projectile olmayan silahların özel karşılığı ayrıca netleştirilecek. Mermisiz silahlarda sonik durum hasar bonusu + ışın uzunluğu/koni menzili +%30 verir (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Danger Pay | Utility · 1/2 hücre (boyut merdiveni) | Threat≥3 iken Scrap ve Gold kazancı L1/L2/L3 +%25/+%30/+%35. | Kullanıcı seçimi (2026-10-24): seviye başına +5 yüzde puanı; L1/L2/L3 Gold ve Scrap kazancı +%25/+%30/+%35. Hazard Dividend +10 yüzde puanıyla +%35/+%40/+%45 (bölge bonusu öncesi). | Özel: Hazard Dividend (ana orana +10 yüzde puanı) / Hazard Cover (Threat≥3 iken alınan hasar×0,92). Ortak G1/G5/Four Corners; havuz kabul. | Planlı; Threat koşulu ve Gold/Scrap ödül akışı yok. |
| Tempered Hull | Defense · 2/3 hücre | Defense yeteneği her kullanıldığında bu koşu boyunca kalıcı Armor +1; toplam bir üst sınırı vardır. | Armor tavanı L1/L2/L3 +30/+40/+50 kabul. | Özel: Bastion Slam (%150 birikmiş bonus Armor Energy hasarı,300 yarıçap,5sn bekleme) / Field Medic (Defense kullanımında 3sn toplam %12 MaxHealth iyileşme,30sn bekleme; bekleme boyunca yenilenmez). Ortak D1/D3/D4; havuz kabul. | Uygulandı. |
| Speed Lance | Movement · 2/3 hücre | Movement kullanımından sonraki 1sn pencerede en yüksek hız ölçülür, pencere sonunda otomatik delici Energy mızrağı çıkar; hasar=hız×0,4; menzil900, hız1800/sn (uçuş0,5sn). Delme ana özellik, yan gerektirmez. | Ana hız katsayısı0,4; seçilmemiş seviye büyümesi yok. | Özel: Echo Lance (0,4sn sonra aynı yönde %50 hasarlı ikinci delici mızrak,ilk hız örneğini kullanır) / Charged Lance (ilk hedefe2 Electric stack,sonraki hedeflere yok). Ortak G2/G3/G4; havuz kabul. Maksimum3 hücrede tek yan açılır. | Uygulandı. Hız katsayısı 0,40 → 0,45 → 0,50 (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Shield Battery | Defense · 2/3 hücre merdiveni | Kalkan doluyken yenilenme ek rezerv biriktirir; gelen hasar önce rezervi tüketir. | Açık; eski Epic taslağındaki MaxShield %40 rezerv tavanı öneridir, Rare için henüz seçilmedi. | 2 hücrede ana rezerv, 3 hücrede aynı ana güç + tek yan; kullanıcı seçimi (2026-10-24). İlk özel yan Kapasitör Odağı seçildi: mevcut rezervden EP; %25 oran öneri. İkinci özel Kesintisiz Depolama seçildi: normal kalkan doluyken rezerv hasar altında da dolar; normal depolama hızının %25 oranı öneri. Üç ortak açık; Parazit Filtresi reddedildi. | Uygulandı (2026-10-24): rezerv tavanı MaxShield %30/35/40 (ajan yer tutucusu); Kapasitör Odağı %25 ve Kesintisiz Depolama %25 (öneri). Birden fazla Battery tek rezerv kullanır (en büyük tavan). Rezervin HUD göstergesi yok. Ortak D4/D5/Uyumlu Yuva; Kapasitör Odağı %25 ve Kesintisiz Depolama %25 onaylı; rezerv kalkan çubuğunun ucunda ek parça olarak gösterilir (karar 2026-10-24, uygulama Sol'a bekliyor). |

## Epic modüller · 17

**2026-10-24: Pathfinder Array dışındaki 14 Epic uygulandı**; sayıların bir kısmı öneri veya ajan yer tutucusudur. Epic normal nadirlik kurallarını izler; güçlü stat, koşul, tetik veya kısmi davranış değişimi içerebilir. Nadirlik ve ana mekanik seçimi, bütün sayıların veya boyutların onaylandığı anlamına gelmez. **Öneri** diye işaretlenen değerler kesin denge değildir. Boyut ve yan havuzu henüz kapanmamış kayıtlar aşağıda belirtilir.

### Primary · 2

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Turret Mount | 2/3 hücre; 3 hücre L | Primary menzildeki en yakın düşmana geminin yönünden bağımsız otomatik nişan alır. 2 hücre ana etki, 3 hücre aynı ana etki + tek yan; boyut/şekil kullanıcı seçimi. | L1 hasar×0,80; ×0,80/×0,83/×0,86 gelişimi hâlâ öneri. İnfaz Önceliği özel yanı seçildi (2026-10-24): en düşük can yüzdeli geçerli hedef önceliği. Odak Kilidi ve Ağır Hedef Önceliği reddedildi; ikinci özel/üç ortak açık. **Uygulandı (2026-10-24):** ×0,80/0,83/0,86 öneriyle. Menzilde hedef yoksa ileri ateşler; beam/koni gibi mermi olmayan silahlarda nişan etkisiz. Broadside ile birlikte takılamaz. İkinci özel Ayrık Ateş (her 4. Primary atışı ikinci hedefe de gider); ortak O2/O3/Gövde Bağı; ışın/koni silahlarda da nişan alır (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Broadside Mount | Sabit 3 hücre · L | Primary öne ateş etmek yerine iki yandan ateş eder; L şekli kullanıcı seçimi (2026-10-24). | Kullanıcı seviye başına %2–%5 düşündü. Ajan önerisi her yan için hasar katsayısı ×0,70/×0,75/×0,80 (+5 yüzde puanı). Başlangıç %70 ve gelişim henüz seçilmedi; silah türü uyarlaması açık. Sabit boyutta yan yok. **Uygulandı (2026-10-24):** ×0,70/0,75/0,80 öneriyle; yalnız mermi silahlarında (Standard/Shotgun) iki yana atar, diğer silahlarda etkisiz. Turret ile birlikte takılamaz. Işın/koni silahlarda da iki yana çift çıkar, her biri katsayıyla (karar 2026-10-24, uygulama Sol'a bekliyor). |

### Offense · 4

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Twin Reactor | Sabit 2 hücre; yan yok | AP/EP eşitlemesi korunur; kendi bonusu öncesi yüksek değer M üzerinden iki stat eşitlenir. Her iki stat M+5×L olur; yüzde büyüme yok. | Kullanıcı düzeltmesi (2026-10-24): L1/L2/L3 düz bonus +5/+10/+15 AP ve EP. Bonus öncesi AP100/EP40 örneğinde sonuç105/105,110/110,115/115. Önceki %3 önerisi geçersiz; kendi bonusundan tekrar eşitleme yapılmaz. Diğer dönüşümlerle işlem sırası açık. **Uygulandı (2026-10-24):** eşitleme statik Add değiştiricilerden sonra çözülür; AP/EP'de Multiply değiştirici varsa tam eşitlenmez. |
| Prism Core | 2/3 hücre; büyükte tek yan | Her Offense kullanımı Thermal → Cryo → Electric → Kinetic döngüsünde ilerler; isabetler kendi statüsüne ek olarak o kullanımın statüsünü uygular. Alternating Current bu taşıyıcı altında birleştirildi; ayrı ana modül değildir. | Kabul edilen başlangıç gelişimi: ek stack1; yalnız Prism kaynaklı ek hasar statüsü süreleri L1/L2/L3 ×1,00/×1,15/×1,30. Frozen/stun otomatik kapsam değildir. Storm Capacitor ve Alternating Current özel yanları seçili. Alternating farklı hasar türünde vuruşa ekstra hasar verme etkisini korur; yanın seviye gelişimi yoktur, sabit oranı açık; yeni iki tip uygulama tasarımı değildir. Storm gücü, ortak havuz ve Frost Lock/Arc Discharge etkileşimleri açık. **Uygulandı (2026-10-24)** iki yanla. Storm Capacitor yer tutucuları: 4 yük, 500 yarıçapta 3 hedef, hedef başına 2 Electric stack + 15 hasar. Alternating Current ×1,20 yer tutucu. Ortak O4/R5/Gövde Bağı; Prism stack'leri Frost Lock ve Arc Discharge'ı tam tetikler; Storm Capacitor hasarı tetikleyen Primary isabetinin %50'si (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Blood Contract | 2/3 hücre; büyükte tek yan | Offense kullanımı mevcut canın %6’sını harcar; kalkanı atlar, en az1 can bırakır. Yeteneğinin hasar verdiği düşman4sn içinde kim öldürürse öldürsün MaxHealth’in %1’i iyileşir. Eksik canla Offense hasarı doğrusal artar; cooldown değişmez. | Kabul edilen çarpanlar L1×1,20/×1,70; L2×1,25/×1,75; L3×1,30/×1,80. İki özel mekanik seçildi (2026-10-24): Pıhtı Zırhı (ödenen can üzerinden geçici Armor), Alacak Takibi (tahsilat penceresi uzatma). ×3/3sn ve4→7sn sayıları öneri; üç ortak açık. **Uygulandı (2026-10-24)** iki yanla: Pıhtı Zırhı ödenen can ×3 Armor/3 sn, yenilenir birikmez; Alacak Takibi 7 sn. Pıhtı Zırhı ödenen can ×2 Armor 4 sn, Alacak Takibi 6 sn; ortak D3/D1/Ertelenmiş Hasar (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Monochrome Core | Sabit 2 hücre; yan yok | Aynı hasar türünde aktif Q/E/F/R yeteneği sayısına göre o türde verilen hasar artar: 2 yetenek ×1,25, 3 ×1,50, 4 ×1,75. | Seviye tabanı büyütür: 2'li ×1,25/1,30/1,35; her ek yetenek +0,25. Planlı (karar 2026-10-24, uygulama Sol'a bekliyor). |

### Defense · 3

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Last Stand | Sabit 2 hücre; yan yok | Ölümcül hasarda diriltir; modül L1/L2/L3 hak kapasitesi1/1/2. Son hak bitince bozuk/etkisiz kalır; MaxShield cezası yok. Dirilme canı L1/L2/L3 %40/%45/%50. | Kullanıcı son seçimi (2026-10-24): diriliş sonrası stat bonusu bütün seviyelerde sabit15sn. Bütün savaş statlarına +%20 ve tam stat listesi hâlâ öneri/açık. Eski10/15/20sn gelişimi geçersiz. Kullanılmış haklar korunur; L3 yalnız yeni hakkı verir. Etkin bölge seviyesinin hak kapasitesine etkisi açık. **Uygulandı (2026-10-24):** diriliş sonrası 15 sn AP/EP/Attack Speed/Armor ×1,20 (stat listesi ajan yer tutucusu); harcanan haklar korunur, yeni diriliş bonusu yeniden başlatır. Bonus statları AP/EP/Attack Speed/MaxHealth ×1,20 (Armor yerine); haklar basamaklı büyüme (L1–2: 1, L3–4: 2, L5: 3) (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Vital Alloy | Sabit 3 hücre · L; yan yok | MaxHealth’in bir oranını bonus Armor olarak verir; can ve Armor tüketmez. | Kullanıcı boyut/gelişimi kabul etti (2026-10-24): L1/L2/L3 %15/%18/%21 dönüşüm;400MaxHealth→Armor60/72/84. Çözümleme sırası açık; Armor Conversion/Rapid Alloy ile kombo. **Uygulandı (2026-10-24).** |
| Acil Durum Protokolü | 2/3 hücre; 3 hücre L, büyükte tek yan | Epic Defense ailesi; yerleştiği bölgeden bağımsız, gövde canı≤%30iken bütün Q/E/F/R bekleme yenilenmesi hızlanır. Komşuluk/Defense bölgesi/Defense yeteneği sınırlaması yok. %30 üstünde biter, anlık reset değil. | Kullanıcı 2026-10-24'te sabit3/yansız yerine2/3 merdivenini seçti. İki boyutta ana etki aynı; maksimum3 hücrede tek yan açılır. İki özel yan kabul edildi: can≤%30 iken Karşı Baskı yetenek hasarı+%15; Tahliye Rotası hareket hızı+%20. Üç ortak açık. Ana gelişim L1/L2/L3 +%75/+%100/+%125 yenilenme (×1,75/×2,00/×2,25) korunur; yanlar seviye ile artmaz. Eski1/2/3-parçalı model geri gelmedi. **Uygulandı (2026-10-24)**; birden fazla kaynakta tür başına en güçlü çarpan geçerli. Ana gelişim ×1,50/1,60/1,70 seçildi (önceki ×1,75/2,00/2,25 geçersiz); ortak G2/D6/Ertelenmiş Hasar (karar 2026-10-24, uygulama Sol'a bekliyor). |

### Movement · 4

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Phase Slip | 2 hücre | Afterburner başlangıcında 0,35 sn düşman mermileri gemiden geçer; bu sırada Primary ve yetenek kullanılamaz. Ardından ilk Primary atışı alan patlaması yapar. Bekleme 6 sn. Geçilen her mermi patlamaya +%25 hasar ve +15 yarıçap ekler; sınır yok. | Kullanıcı mevcut tasarımı kabul etti (2026-10-24). Mevcut taban patlama ×2 hasar/120 yarıçap korunur. Gelişim patlama hasarını/yarıçapını artırma yönünde; sayısal gelişim henüz seçilmedi. Faz süresi ve bekleme sabit. Sabit boyutta yan yok. **Kısmen uygulandı (2026-10-24):** faz 0,35 sn boyunca bütün gelen hasarı engeller (mermiler geçmez, tüketilir); ardından ilk Primary isabetinde ×2 hasar/120 yarıçap patlama, 6 sn bekleme. Geçilen mermi başına +%25/+15 büyüme uygulanmadı. Fazda yalnız düşman mermileri gemiden geçer ve yoluna devam eder (ışın/çarpışma/alan vurur); geçen mermi başına +%25 hasar/+15 yarıçap; patlama ×2,0/2,25/2,5 ve yarıçap 120/130/140 (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Assault Vector | Sabit 2 hücre; yan yok | Yakındaki düşmana doğru gerçek hareket hasar; ondan uzaklaşma Armor verir. Yanlamasına veya çok düşük hızda iki bonus da yoktur. | Güncel tasarım: taban +%15 hasar/+20 Armor; seviye başına +3 yüzde puanı hasar ve +5 Armor. L1/L2/L3 +%15/%18/%21 ve+20/25/30 Armor. Hasar bonusu=15+3×(L−1) yüzde puanı; Armor=20+5×(L−1). Hedef seçimi ve yön/hız eşikleri açık. **Uygulandı (2026-10-24)**; yer tutucular: 900 menzilde en yakın düşman, yön kosinüsü ≥0,5, hız ≥100; hasar bonusu bütün giden hasara. Yön ayrımı yarı yarıya: düşmana doğru 180° hasar, ters 180° Armor, nötr bölge yok (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Pathfinder Array | 1/2 hücre; büyükte tek yan | Haritada yakın iki nokta arasında yeni rota bağlantısı açar; farklı ödüle ulaşma veya karşılaşmayı dolaşma olanağı sağlar. | Bölüm başına kullanım=1+floor((L−1)/2); L1/L2/L3 1/1/2. İki boyutta ana etki aynı. İki özel yan: İzci Payı (geçişten sonraki ilk karşılaşma Gold/Scrap+%20), İlk Temas (geçişten sonraki ilk karşılaşma boyunca tüm statlara sabit bonus; miktar/tam kapsam açık). Yanlar seviye ile artmaz. Yakınlık, Threat maliyeti, rota sistemi entegrasyonu ve üç ortak yan açık. Planned; runtime yok. |
| Momentum Drive | 1/2 hücre; büyükte tek yan | Yatay/dikey hareket puanı +3; takılı Movement yeteneği başına +1. Diğer takılı Movement ana modülü başına +0,25 puan. Movement kullanımında yalnız bu modülün toplam puan katkısı 5sn boyunca ×2; gerçek hız azalan getiri formülünden çözülür. | Taban puan=3+0,5×(L−1); L1/L2/L3 +3/3,5/4. Yetenek başına+1 ve modül başına+0,25 sabit. Kendisi, sistem/slot kabukları ve parçalar ayrı modül sayılmaz. Tekrar kullanım çiftleme süresini yeniler, çarpanı biriktirmez. İki boyutta ana etki aynı; maksimum2 hücrede tek yan açılır, henüz seçilmiş yan yok. **Kısmen uygulandı (2026-10-24)**: takılı Movement yeteneği başına +1 uygulanmadı (yetenek ailesi sorgusu yok). Movement yeteneği başına +1 uygulanacak; özel yanlar Hız Yankısı (katlanma sürerken Movement kullanımı süreyi +2 sn uzatır, en çok 10 sn) ve Akış Kalkanı (katlanma sürerken alınan hasar ×0,85); ortak G4/R6/Ertelenmiş Hasar (karar 2026-10-24, uygulama Sol'a bekliyor). |

### Utility · 5

| Modül | Boyut | Ana tasarım | Gelişim ve tamamlanacak kararlar |
|---|---|---|---|
| Cascade Reactor | Sabit 2 hücre; yan yok | Farklı ailelerden yetenekleri art arda kullanmak sonraki yeteneğin gücünü artırır. İlk kullanım zinciri başlatır ve bonus almaz; sonraki farklı aileli kullanımlar sırasıyla1/2/3 adım bonusunu alır. En fazla3 adım; iki aile dönüşümlü ilerleyebilir. | Pencere5sn; her uygun kullanımda yenilenir. Aynı aile tekrarı veya pencere aşımı zinciri sıfırlar. Adım başına20+2×(L−1) yüzde puanı: L1 +%20/40/60; L2 +%22/44/66; L3 +%24/48/72. Seviye yalnız gücü artırır; pencere ve adım tavanı sabit. Etkilenen sayısal güç değerlerinin hasar/iyileşme/kalkan kapsamı ve Combo Sequencer ile ayrışma/birleştirme açık. **Uygulandı (2026-10-24)**; güç kapsamı yer tutucu olarak yalnız yetenek hasarı. Bonus ana etki miktarını büyütür: hasar, iyileşme, kalkan, Movement mesafe/hız, Utility etki gücü; süre/yarıçap/menzil/bekleme değişmez (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Confluence Core | Sabit 1 hücre; yan yok | Her ailedeki takılı ana modül sayısı o ailenin stat katkısını çarpar. Offense AP/EP, Defense MaxHealth, Primary Attack Speed, Movement hareket puanı, Utility bekleme yenilenme hızı verir. Aynı aileyi yalnız bir kez sayan önceki model geçersiz. | Modül başına: Offense AP/EP2+(L−1); Defense MaxHealth10+5×(L−1); Primary AS +[1+0,5×(L−1)]%; Movement hareket puanı0,25+0,10×(L−1); Utility bekleme yenilenmesi +[1+0,5×(L−1)]%. Örnek L1:5 Offense ve4 Defense →AP+10/EP+10/MaxHealth+40. Kendisi, sistem/slot kabukları ve parçalar ayrı modül sayılmaz; ayrı seri/ailesiz kayıtların kapsamı açık. **Uygulandı (2026-10-24)**: Primary AS ateş hızına, Utility yenilenme Utility cooldown çarpanına, hareket puanı yatay/dikey hareket değerine eşlendi. Ailesine göre sayar (Hunt Clock serisi kendi ailesiyle); ailesiz ve Yetenek modülü sınıfı sayılmaz (karar 2026-10-24). |
| Field Mastery | 2/3 hücre; 3 hücre L, büyükte tek yan | Her Utility yeteneği ayrı kullanım sayacıyla koşu boyunca kalıcı +1 seviye kazanır. Takılıyken Utility Scrap ile geliştirilemez. Önceden satın alınan ve modülle kazanılan seviyeler sökünce korunur. Modülün ayrı +2 tavanı yoktur; yeteneğin kendi azami seviyesi geçerlidir. | İki boyutta ana etki aynı;3 hücrede tek yan açılır. Bilgi Aktarımı özel yanı seçildi: bu modülle Utility yeteneğine seviye kazandırınca diğer takılı Utility yeteneklerinin kullanım sayaçlarına+2. Sabit yan; seviye ile artmaz. İkinci özel ve üç ortak açık. Ana eşik15 →14 →13 →12 korunur. Sök-tak koruması, sayaç aktarımının tavan/zincir işlemesi ve kazanılan her seviyede gereken kullanımın1 azalması fikrinin modül eşiğiyle birleşimi açık. **Uygulandı (2026-10-24)**; Scrap kilidi oynanış katmanında, kullanım sayaçları respawn'da sıfırlanır (kazanılan seviyeler korunur). İkinci özel Ustalık Taşması (azami seviyedeki Utility yeteneğinin her 15 kullanımı koşu boyunca Utility etkilerine +%5); ortak G2/G5/Gövde Bağı; sayaçlar koşu boyunca korunur, sökülünce dondurulur (karar 2026-10-24, uygulama Sol'a bekliyor). |
| Resonant Barrel | Sabit 2 hücre; yan yok; aile yerleştiği bölgeye göre | Başarılı Primary atışı (atış başına tek) aile bölgesinde o ailenin yeteneklerine, Gövde'de bütün ailelere bekleme iadesi verir; Burun'da etkisiz. | Aile bölgesinde 0,25/0,30/0,35 sn, Gövde'de 0,10/0,12/0,14 sn; Zone Cooler gibi etkin seviyeyle ölçeklenir. Planlı (karar 2026-10-24, uygulama Sol'a bekliyor). |

## Legendary modüller · 4

Tamamı **planlı tasarım**. Legendary oynanış kimliğini tamamen değiştirir. Her modül tek sabit boyutludur; **yan özellik ve hiçbir bölge bonusu almaz**. Birden fazla kategoriyi etkileyebilir. Seviye/loot ve birlikte takılma kuralları henüz belirlenmiş değildir.

| Modül | Aile · sabit boyut | Ana tasarım | Tamamlanacak kararlar |
|---|---|---|---|
| Second Hardpoint | Primary · 3 hücre | İkinci Primary Weapon takılmasına izin verir. Aynı anda yalnız biri ateş eder; tuşla aralarında geçilir. | Geçiş tuşu, geçiş beklemesi, şarjör durumu ve gelişim açık. Fare tekerleği öneri; Ctrl Twin Rack için ayrılmıştır. |
| Twin Rack | Utility · 3 hücre | Her Q/E/F/R slotu iki yetenek taşır; toplam 8. Normal tuş ilkini, Ctrl+tuş ikinciyi kullanır. Her yeteneğin cooldown’u ayrıdır; aynı slottaki iki yetenek aynı ailedeyse küçük set bonusu alır. | Set bonusunun +%10 hasar olması öneri. Gelişim açık. Shift Afterburner için korunur. |
| Blink Drive | Movement · **Legendary** · sabit 2 hücre | Afterburner yerine 3 kısa ışınlanma charge’ı verir; dolum Afterburner kapasitesinden türetilir. | Işınlanma menzili, dolum hesabı ve gelişim açık. Legendary kararı 2026-10-23 düzeltmesiyle güncellendi. |
| Combo Sequencer | Yetenek modülü · boyut açık | Ardışık yetenek kullanımıyla güçlenen seri; Cascade Reactor'dan ayrışmak için Legendary olarak yeniden tasarlanacak (karar 2026-10-24). | Yeni mekanik Legendary turunda. |

## Nadirlik dışı özel eşya · 1

Ana modül sayımına dahil değildir (karar 2026-10-24). Combo Sequencer Legendary'ye taşındı. Ultimate Protocol 2026-10-25'te nadirliksiz yetenek modülü oldu.

| Eşya | Sınıf · boyut | Seçilmiş yön | Tamamlanacak kararlar |
|---|---|---|---|
| Ultimate Protocol | Yetenek modülü · 1 hücre · nadirlik/yan/seviye yok | Bir yetenek tuşunun (Q/W/E/R, hangisi olursa) standart slot modülünün yerine takılır; o tuştaki yetenek ultimate olur: ana etki ×1,6, bekleme ×2, sabit değerler, bölgeden etkilenmez. Birden fazla tuşa takılabilir; sınırlama yok (oyuncu serbest). Cascade çarpanıyla çarpılır; tavanlı etkiler kendi tavanında kalır (Barrier 180°). Ana etkisi olmayan yetenekte (Void Gate) yalnız bekleme uzar. | Edinim mağaza/rota sistemiyle gelir. **Uygulandı (2026-10-26)**: slot bağı takıldığı standart slot hücresinden seçilir, taşımada korunur; geliştirici envanteri 4 kopya verir. |
| Courier Hold | Nadirliksiz özel eşya · boyut açık | Mağazada satılan mühürlü kargo gemide yer kaplar; sonraki bölüme ulaştırınca güçlü ödül verir. Geliştirilemez. | Mağaza ve rota ödül sistemiyle tasarlanır: ödül, hücre sayısı, ölüm/kargo kaybı. |

## Yan özellikler

### Seçilmiş yan mekanikler ve taşıyıcıları

| Özellik | Yeni yön | Güç / açık noktalar |
|---|---|---|
| Lucky Cast | Hunt Clock dört sürümü ortak yanı;G5 yerine | Luck+1; Q/E/F/R kullanımında Luck başına%3 ihtimalle kullanılan yeteneğin beklemesinin%25’i iade. Primary hariç. Yan seviyeleri bu tur artırılmadı. |
| Status Resonator | Arc Extender özel yanı | Uygulanan hasar statü süreleri×1,20; Frozen dahil, donma beklemesi hariç. O4 Arc havuzunda değil; süre artışı Thermal toplam DoT/Ember referansına yansır. |
| Arc Discharge | Arc Extender özel yanı | Maksimum2 hücrede açılır. Hedef5 Electric’e ulaşınca350 yarıçapta2 en yakın düşmana2stack; hedef başına4sn bekleme. Kaynak korunur; yayılma hasarsız ve zincirsiz. Yeni seviyeler seçilmedi. |
| Bastion Slam | Tempered Hull özel yanı; bağımsız modül değil | Tempered maksimum3 hücrede açılır. Defense kullanımında biriken bonus Armor’dan %150 Energy hasarı,300 yarıçap uygulandı. Alternatif özel Field Medic ve ortak D1/D3/D4 seçildi; 5sn bekleme ve runtime uygulandı; seçilmemiş seviye büyümesi yok. |
| Four Corners | Rastgele atanan ortak Utility yanı; ayrı ana modül değil | Dört Q/E/F/R farklı ailedeyse güç artışı. Uygun taşıyıcının ortak yan havuzundan rastgele atanır; Confluence Core’a sabitlenmez, Spare Capacitor’a atanmaz. ×1,10 kabul (eski ana ×1,15); Danger Pay ortak havuzu kabul. Hasarsız etki kapsamı/diğer taşıyıcılar açık; uygulanmadı. |
| Field Medic | Tempered Hull özel yanı | Defense kullanımında 3sn toplam %12 MaxHealth iyileşme;30sn bekleme tetikte başlar. Bekleme boyunca Defense kullanmak iyileşmeyi yenilemez veya süreyi sıfırlamaz. %4 eski taslak reddedildi; Uncommon %8 önerisi seçilmedi. |
| Storm Capacitor | Prism Core özel yan tasarımı; bağımsız Utility ana modül değil | Yetenek kullanımlarıyla biriken yük, sonraki uygun Primary isabetinde yıldırım/Electric olarak boşalır. Yan rolü seçildi (2026-10-23). Eski 4 yük/3 hedef/2 stack bağımsız Epic paketi aynen aktarılmaz; yan gücü, yük/tetik kapsamı, hedefler, gelişim ve taşıyıcının boyut merdiveni/diğer yan havuzu açık. Uygulandı (2026-10-24); sayılar yer tutucu. |
| Alternating Current | Prism Core özel yanı; bağımsız Epic değil | Eski ana etki doğrudan yana taşındı: aynı düşmana önceki doğrudan yetenek vuruşundan farklı hasar türüyle vurunca o vuruşun hasarı artar. Yan özellik seviye ile artmaz; eski ana modülün ×1,40/×1,45/×1,50 gelişimi yana taşınmaz. Sabit ekstra hasar oranı henüz seçilmedi; DoT tickleri sırayı değiştirmez. Yeni iki tip uygulama mekaniği değildir. Kullanıcı düzeltmesi (2026-10-24). Uygulandı (2026-10-24); ×1,20 yer tutucu. |

### Onaylı özel havuzlar

Boyut merdiveninde maksimum boyutta havuzdan tek yan açılır; eksik seçenekler kendiliğinden doldurulmaz. Uygulama durumu ana katalogdadır.

| Taşıyıcı | Özel seçenek 1 | Özel seçenek 2 | Kabul edilen üç ortak |
|---|---|---|---|
| Pathfinder Array | İzci Payı: açılan bağlantıdan geçtikten sonra ilk tamamlanan karşılaşmanın Gold/Scrap ödülü+%20; bağlantı başına tek ödül. | İlk Temas: açılan bağlantıdan sonraki ilk karşılaşma boyunca tüm statlara bonus; miktar ve uygulanacak stat listesi açık. Eski8sn/+20Armor taslağının yerini aldı. | Üç ortak açık. Bonuslar sabit, seviye ile artmaz; maksimum2 hücrede tek yan açılır. |
| Field Mastery | Bilgi Aktarımı: Field Mastery ile Utility yeteneğine seviye kazandırınca diğer takılı Utility yeteneklerinin kullanım sayaçlarına+2. | Açık. | Üç ortak açık. Bonus sabit, seviye ile artmaz; maksimum3 hücrede tek yan açılır. İkinci özel Ustalık Taşması seçildi. Ortak G2/G5/Gövde Bağı (karar 2026-10-24). |
| Acil Durum Protokolü | Karşı Baskı: gövde canı≤%30 iken yetenek hasarı+%15. | Tahliye Rotası: gövde canı≤%30 iken hareket hızı+%20. | Üç ortak açık. Özel bonuslar sabit, seviye ile artmaz; maksimum3 hücrede havuzdan tek yan açılır. Ortak G2/D6/Ertelenmiş Hasar seçildi (karar 2026-10-24). |
| Prism Core | Storm Capacitor: yük biriktirip uygun Primary isabetinde Electric boşaltma; yan sayıları açık. | Alternating Current: farklı hasar türünde vuruşa ekstra hasar. Yan seviye ile artmaz; sabit bonus oranı açık. | Açık; 3 hücrede bu havuzdan tek yan açılır. Ortak O4/R5/Gövde Bağı seçildi (karar 2026-10-24). |
| Overshield Layer | Shield Rush: tavan üstü geçici kalkan varken hareket hızı+%20; kalkan bitince biter, birikmez. | Reserve Shell: tavan üstü geçici kalkan varken alınan hasar×0,90. | D1 MaxHealth+25; D2 hasar alınca Armor+12/2sn; D4 regen×1,15. |
| Armor Conversion | Rapid Alloy: toplam Armor’un %10’u kadar AS yüzde puanı;80 Armor→+%8 AS, sürekli hesaplanır, Armor tüketilmez. | Balanced Output: ana bonusun %25’i düşük AP/EP’ye; eşitlikte ana AP/yan EP, geri besleme yok. | D1 MaxHealth+25; D2 hasar alınca Armor+12/2sn; D3 öldürünce %1 MaxHealth. |
| Tempered Hull | Bastion Slam: Defense kullanımında birikmiş bonus Armor’un %150’si Energy hasarı,300 yarıçap. 5sn ayrı bekleme kabul edildi. | Field Medic: Defense kullanımında 3sn toplam %12 MaxHealth,30sn bekleme; beklemede yenilenmez. | D1 MaxHealth+45; D3 öldürünce %1,75 MaxHealth; D4 regen×1,25. |
| Danger Pay | Hazard Dividend: Threat≥3 iken ana ekonomi oranına +10 yüzde puanı; ana gelişimle L1/L2/L3 +%35/+%40/+%45 (bölge bonusu öncesi). | Hazard Cover: aynı koşulda alınan hasar×0,92. | G1 öldürünce+2Gold; G5 Luck+2; Four Corners: dört Q/E/F/R farklı ailedeyse güç+%10, hasarsız etki kapsamı açık. |
| Speed Lance | Echo Lance: ilk mızraktan0,4sn sonra aynı yönde %50 hasarlı ikinci delici mızrak; ilk hız örneği, yeni kullanım sayılmaz. | Charged Lance: ilk hedefe2 Electric stack; sonraki delinen hedeflere stack yok. | G2 AbilityHaste+7; G3 Afterburner kapasitesi+%18; G4 yatay/dikey hareket rating+1,4. |
| Gold Plating | Altın İskelet: yalnız bu modülün Gold kaynaklı bonus Armor değerine bağlı MaxHealth. Mekanik kullanıcı seçimi (2026-10-24); 1 Armor → 1 MaxHealth oranı öneri. Gold harcamada yeniden hesaplanır, diğer Armor sayılmaz. | Kasa İskontosu: Gold harcama maliyetlerini azaltır. Mekanik kullanıcı seçimi (2026-10-24); ×0,90 maliyet (%10 indirim) öneri. Harcama türleri, fiyat/yuvarlama ve diğer indirimlerle birleşim açık. Darphane Kalkanı reddedildi. | Seçilmedi. D2/D3/D4 önceki ajan önerisi; iki özel seçili, ortak havuz tamamlanmadı. Ortak D1/G1/Uyumlu Yuva seçildi (karar 2026-10-24). |
| Shield Battery | Kapasitör Odağı: mevcut rezerv miktarına bağlı EP; mekanik kullanıcı seçimi (2026-10-24). %25 oranı öneri: mevcut rezerv80 → EP+20. Rezerv harcandıkça bonus azalır; rezerv tavanı veya normal kalkan sayılmaz. | Kesintisiz Depolama: normal kalkan doluyken rezerv hasar alsa da dolmaya devam eder. Mekanik kullanıcı seçimi (2026-10-24); normal depolama hızının %25 oranı öneri. Normal kalkanın hasar sonrası beklemesini kaldırmaz. | Henüz seçilmedi; iki özel mekanik seçili, ortak havuz açık. Ortak D4/D5/Uyumlu Yuva seçildi; iki %25 oranı onaylı (karar 2026-10-24). |
| Turret Mount | İnfaz Önceliği: otomatik nişan en düşük can yüzdeli hedefe öncelik verir; mekanik seçildi (2026-10-24). | Açık. Odak Kilidi/Ağır Hedef Önceliği reddedildi. | Henüz seçilmedi. Ayrık Ateş seçildi (her 4. Primary atışı ikinci hedefe de gider). Ortak O2/O3/Gövde Bağı (karar 2026-10-24). |
| Blood Contract | Pıhtı Zırhı: ödenen cana bağlı geçici Armor. Mekanik seçildi; maliyet×3 ve3sn öneri. Birikmeme/yenileme ayrıntısı açık. | Alacak Takibi: ölümden iyileşme kazanma penceresi uzar. Mekanik seçildi;4→7sn öneri. | Henüz seçilmedi; iki özel mekanik seçili. Pıhtı Zırhı ×2/4 sn, Alacak Takibi 6 sn; ortak D3/D1/Ertelenmiş Hasar (karar 2026-10-24). |
| Triple Core | Yedek Zamanlayıcı: her boş Q/E/F/R slotu takılı yeteneklerin bekleme yenilenmesini +%15 hızlandırır. | Odak Ateşi: boş slot varken Triple Core hasar çarpanının yarısı Primary'ye de uygulanır. | O1/G2/Sakin Nefes (karar 2026-10-24). |
| Momentum Drive | Hız Yankısı: katlanma sürerken Movement kullanımı süreyi +2 sn uzatır, en çok 10 sn. | Akış Kalkanı: katlanma sürerken alınan hasar ×0,85. | G4/R6/Ertelenmiş Hasar (karar 2026-10-24). |

### Kurallar ve ortak havuzlar

- Common’da yan yok. Sabit boyutlu modülde yan yok. Uncommon/Rare merdiveninde maksimum boyutta 1 yan; normal merdiven havuzunda 2 özel + 3 ortak = 5 seçenek hedeflenir. Epic de normal nadirlik sözleşmesini izler; henüz seçilmemiş havuzlar otomatik doldurulmaz.
- **Spare Capacitor istisnası:** 1/2/3 boyut merdiveni yalnız ana charge özelliğini büyütür; maksimum boyutta da yan taşımaz.
- Yan düşerken rastgele atanır; küçük boyutta kilitli, büyüyünce açılır. Kayıt/yükleme ve yükseltme harcaması/yeniden seçim arayüzü henüz uygulanmış değildir.
- Üretilen mermi/patlama/sekme başka modül tetiklemez. Modül çarpanları sonra kritik sırası korunur; özel üretilen hasar sözleşmeleri kendi satırlarında.
- Ana güce sırf denge için üst sınır koymama kullanıcı yönü geçerlidir; belirtilen Tempered/Gold gibi özel tavan kararları ve teknik/stack sınırları ayrı değerlendirilir.
- F1 yalnız onu izin listesine alan Rare/üstü normal modüllerde; Legendary yan almaz: maksimum boyut tek hücrelere parçalanır, bitişik parçalar otomatik bağlanır; yerleşim seviyesini en zayıf konumdaki parça belirler (hepsi Gövde’de +2, biri eşleşmeyen bölgede +0), komşuluk parça başına. Trophy Core parçalanmaz. Spare için canlı F1 izni kaldırıldı; F1 dahil yan yoktur.
- **Yerleşim seviyesi (kullanıcı, 2026-10-24):** bölge bonusu yüzde değil seviyedir. Modül kendi aile bölgesinde +1, Gövde’de +2 seviye çalışır; başka ailenin bölgesinde +0, ceza yok. Ayrıntı ve istisnalar [sistem kurallarında](MODULE_SYSTEM_DESIGN.md#3-bölgeler-ve-aileler). Etkin seviye = modül seviyesi + yerleşim; gelişim yeteneklerdeki gibi seviye başına formülle tutulur, L4/L5 ayrıca yazılmaz.
- **Büyüme türü (karar 2026-10-24):** seviye tabloları doğrusal (varsayılan), bileşik (her seviye ×oran) veya basamaklı (her N seviyede +1) olabilir; L4/L5 türe göre uzar. Bölge seviyesiyle doğal sınırı aşan değerler kabul edilir (yatırımın ödülü).
- Health Bridge/Spare/Trophy komşuluğu kenar paylaşan Q/E/F/R kabuklarıdır; Primary hariç. Yeni tasarımlarda komşuluk istisna kalır.

#### Ortak havuzlar

| Havuz | Kimlik | Özellik | Runtime durumu |
|---|---|---|---|
| Offense (O) | O1 | Attack Power +8 | Implemented |
| | O2 | Critical Chance +8% | Implemented |
| | O3 | Attack Speed +4% (sabit, tetiksiz) | **Kullanıcı kararı (2026-10-10):** süreli "öldürünce AS" kaldırılıyor çünkü zamanlı AS veren modül/yan özellik zaten var (Attack Rhythm, Critical Rhythm). Kodda uygulandı: `Shared.O3` artık sabit Owner.AttackSpeed +4 (3 seviyede de aynı), adı "Offensive O3". Sharp Scale/Scatter Core izin listelerinde O3 bulunur |
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
| Fragment (F) · yalnız Rare ve üstü | F1 | *Fragmentable:* at max size the module splits into single cells (3 → 1+1+1, 2 → 1+1) that can be placed anywhere on the grid; fragments placed side by side auto-connect. The placement level bonus comes from the worst-placed fragment (all in Gövde → +2; any fragment in a non-matching zone → +0). Adjacency modules count adjacency from every fragment with no extra cap | Yerleşim/çözümleme çekirdeği ve JSON anahtarı mevcut; ekran ve oyuncu kabulü ayrı doğrulanır. |

#### Rare ortak değerler ve Rare’e özel yanlar

| Kimlik | Uncommon (L1) | Rare (canlı L1) | Not |
|---|---|---|---|
| O1 | AP +8 | AP +14 | |
| O2 | Crit +8% | Crit +14% | Kritik güçlü: Targeting Computer +3/+5 ile birlikte düşünülmeli |
| O3 | AS +4% | AS +7% | Yeni sabit tanım |
| O4 | Statü süresi +1 sn | +1,5 sn | ×1,75 = 1,75; Rare JSON değeri +1,5 sn; Ember’ın toplamsal süreyi patlama hasarına katması ayrı konudur |
| O5 | İlk vuruş ×1,15 | ×1,25 | |
| D1 | MaxHealth +25 | +45 | |
| D2 | Armor +12 (2 sn) | Armor +20 (2 sn) | |
| D3 | Öldürünce %1 can | %1,75 | |
| D4 | Kalkan yenilenme ×1,15 | ×1,25 | |
| D5 | Max Shield +8% | +14% | |
| G1 | +1 Gold | +2 Gold | Koşu cüzdanına Gold ekleme uygulanmış; mağaza/ödül ekonomisi ayrı kapsam. |
| G2 | Ability Haste +4 | +7 | |
| G3 | Afterburner +10% | +18% | |
| G4 | Hareket derecesi +0,8 | +1,4 | |
| G5 | Luck +1 | Luck +2 | Luck ayrık; Lucky Cast ve Luck Lens'e çarpan olur, dikkat |

| Kimlik | Özellik | Durum |
|---|---|---|
| R1 | *Guarded Strike:* Defense yeteneği kullanınca 4 sn Primary hasarı ×1,15 | **Implemented · dalga 2** (`RareSideFeaturesE2E`; canlı mermi dalga 4) · Kabul (kullanıcı, 2026-10-10); sayı öneri Genel havuza taşındı: Uncommon ×1,09 / Rare ×1,15 / Epic ×1,21 (karar 2026-10-24). |
| R2 | *Reactive Plating:* kalkan kırılınca 4 sn Armor +30 (yeniden kurulum eşiği %25) | **Implemented · dalga 2** · Kabul (kullanıcı, 2026-10-10); sayılar onaylı (4 sn, Armor +30) Genel havuza taşındı: Armor +17 / +30 / +43 (karar 2026-10-24). |
| R3 | *Boss Hunter:* miniboss ve boss'a verdiğin hasar ×1,10 | Uygulandı; 2026-10-10 kayıtlarında boss sınıflı hedef eksikti. Güncel boss içerik kabulü bu belge denetiminde doğrulanmadı. · Kabul (kullanıcı, 2026-10-10); sayı onaylı **Kaldırıldı (2026-10-24).** |
| R4 | *Full Charge:* kalkan tamamen doluyken verdiğin hasar ×1,08 | **Implemented · dalga 1–2** · Kabul (kullanıcı, 2026-10-10); sayı onaylı · Momentum Shield ve Continuous Field Generator ile etkileşir **Kaldırıldı (2026-10-24).** |
| R5 | *Status Weaver:* hedefte 2 veya daha fazla farklı statü varsa verdiğin hasar ×1,20 | **Implemented · dalga 1–2** · Kabul; kullanıcı ×1,20 veya ×1,25 dedi, ×1,20 seçildi (Claude): Thermal Shock Coil'in Thermal+Cryo ×1,30 ana özelliğini geçmesin Genel havuza taşındı: ×1,11 / ×1,20 / ×1,29 (karar 2026-10-24). |
| R6 | *Fleet Guard:* Movement yeteneği kullanınca 3 sn aldığın hasar ×0,85 | **Implemented · dalga 1–2** (gelen hasar penceresi, `RareDamageConditionsE2E`; `RareSideFeaturesE2E`) · Kabul (kullanıcı, 2026-10-10); sayılar onaylı Genel havuza taşındı: ×0,91 / ×0,85 / ×0,79 (karar 2026-10-24). |

**Nadirliğe göre ortak yanlar (karar 2026-10-24):** genel O/D/G (ve taşınan R1/R2/R5/R6) havuzu bütün nadirliklerde kullanılır; değer Uncommon taban, Rare ×1,75, Epic ×2,5. Nadirliğe özel yalnız dört ortak yan vardır: Rare'e özel **Sakin Nefes** (3 sn hasar alınmazsa bütün bekleme yenilenmesi +%15; hasar alınca biter) ve **Uyumlu Yuva** (modül her yerde aile bölgesindeymiş gibi en az +1 seviye; Gövde +2 aynı); Epic'e özel **Ertelenmiş Hasar** (alınan her vuruşun %30'u sonraki 3 sn'ye yayılır) ve **Gövde Bağı** (modül her yerde Gövde'deymiş gibi +2 seviye). Bunlar havuzlara seçilerek konur; havuz 2 özel + 3 ortak = 5. Uygulama Sol'a bekliyor. Rare ortak değerler ×1,75 ölçek kararına ve ayrı Rare kimliklerine dayanır. O/D/G kodlarının Rare modüllerde Rare değerleri kullanılır. Z1/Z2 2026-10-24’te kaldırıldı; F1 yerleşim çekirdeği mevcut; bu, bütün oyuncu akışının kabul edildiği anlamına gelmez. Yan seviye büyümesi ve yeni yanların nadirlik ölçeği ayrıca açık.

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

## Açık ve ertelenen tasarımlar · nadirliğe göre

Burada yalnız seçilmemiş/ertelenmiş fikirler ve taşıyıcısı açık mekanikler bulunur. Seçilmiş modülün açık boyutu veya katsayısı, yukarıdaki kendi satırındadır. **Önerilen nadirlik kesin karar değildir.** Aşağıdaki hiçbir kayıt 84 ana modül sayısına eklenmez. Kaldırılmış/reddedilmiş eski tasarımlar [karar geçmişindedir](MODULE_DESIGN_DECISIONS.md#kaldırılan-ve-reddedilen-kayıtlar).

### Common · seçilmemiş öneriler

| Kayıt | Taslak | Durum |
|---|---|---|
| Cooling Circuit · Common sürümü | Utility; 2 hücre, Ability Haste +8. | Seçilmedi; eski JSON sürümünün otomatik yeniden sınıflandırılması değil. |
| Energy Coil · Common sürümü | Utility; 2 hücre, EP +18. | Seçilmedi; eski JSON’daki EP +24 ile karıştırılmaz. |

### Uncommon · seçilmemiş veya ertelenmiş

| Kayıt | Taslak | Durum |
|---|---|---|
| Scavenger Net | Utility; ödülsüz savaşlardan Scrap. Önceki öneri 1 hücre, normal Scrap’in %40’ı. | Kullanıcı erteledi; Uncommon ve oran öneri. Bölüm/ödül sistemiyle yeniden ele alınacak. |

### Rare · seçilmemiş yön

| Kayıt | Taslak | Durum |
|---|---|---|
| Harmony Core · ad önerisi | Q/E/F/R setindeki her farklı aile için Haste ve AS; Primary veya modül ailesi sayılmaz. Öneri aile başına Haste+5/AS+%5. | Aktif yetenek ailesi çeşitliliği kullanıcı fikri; Rare, ad ve sayılar ajan önerisi. Confluence modül ailelerini sayar; bu tasarım yetenek ailelerini sayar. Ana modül kabulü yok. |

### Epic · seçilmemiş veya ertelenmiş adaylar

| Kayıt | Taslak | Durum |
|---|---|---|
| Retreat Beacon | Utility; savaştan önceki noktaya geri çekilme, ödül almama, Threat artırmama. | Kullanıcı erteledi. Epic, 2 hücre ve bölüm başına bir kullanım taslak. |
| Monoculture · yan adayı | En az üç Q/E/F/R aynı ailedeyse bekleme azaltma. | Depoda; gelecekte Epic yanı olabilir. Eski Rare ana rolü aktif değil; Triple Core bağlantısı iptal, taşıyıcı/güç seçilmedi. Tek fikir, yeni ana modül değil. |
| Armored Drive | Armor’dan itki/hız tavanı; 100 Armor → +%35 öneri. | Seçilmemiş Epic Movement taslağı. |
| Mobile Charging | Normalde kilitlenen şarj hazırlığı sırasında sınırlı hareket; %60 itki öneri. | Seçilmemiş Epic Movement taslağı; Energy Spear örneği, Cryostasis gibi sabitlenme otomatik kapsanmaz. |
| Shaped Charge | Offense patlamasının merkezinde ek hasar; +%60 öneri. | Seçilmedi; açık ret yok. |
| Shield Gate | Kalkanı kıran vuruşun gövdeye taşmasını engeller; yeniden kurulum önerisi %50 kalkan ve 8 sn. | Seçilmedi; açık ret yok. |
| Scar Tissue | Ağır düşman vuruşundan sağ çıkınca koşu boyunca MaxHealth artışı. | Seçilmedi; sayılar taslak, açık ret yok. |
| Launch Vector | Atış yönündeki gemi hızından Offense projectile hız/hasar katkısı. | Seçilmedi; Primary ve projectile olmayan yetenek kapsamı değildir. |
| Twin Step | Movement kullanımında diğer Movement yeteneklerinin başlamış beklemelerini azaltır; %50 öneri. | Seçilmedi; kendi cooldown’u/charge ekleme değildir. |

<a id="kabul-edilen-hasar-tipi-mekanikleri--taşıyıcı-kararı-bekleyenler"></a>

### Nadirliği veya taşıyıcısı belirlenmemiş

| Kayıt | Mekanik | Durum |
|---|---|---|
| Heat Sink | Kalkanın emdiği hasardan ısı birikir; sonraki Defense kullanımında Thermal boşalma. | Mekanik kabul edildi. Mevcut modüle ana/yan olarak bağlanacak; taşıyıcı, nadirlik, katsayılar, menzil, Ignite, tüketim ve sök-tak kuralları açık. Bağımsız modül onayı yok. |
| Blackglass Circuit | Q/E/F/R sonrası başarılı Primary atışı ek Energy verir; son kullanılan yeteneğin charge yenilenmesini kısmen geri alır. | Utility önerisi; seçilmedi. Nadirlik, iade oranı, atış/beam tek tüketim sınırı açık. |

Diğer onaysız ilk taslaklar ve değiştirilmiş fikirler [kaynak denetiminde](MODULE_DESIGN_DECISIONS.md#seçilmemiş-eski-öneriler) korunur; burada bulunmaması kabul veya ret anlamına gelmez. Önceki öneri turlarındaki Primary/Utility/summon kapsam sınırları o turlara aittir; kullanıcının daha sonra sunduğu yeni fikirleri yasaklamaz.

## Eski JSON tanımları · 9

Bu kayıtlar içerik dosyasında durur; seçilmiş 84 ana modüle dahil değildir. Yeni bir nadirliğe atanmış sayılmazlar.

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

## Uygulama durumu ve kalan işler

2026-10-23 kaynak kontrolünde JSON’da **93 kayıt** var (2026-10-24 sonrası): seçilmiş 70 ana, 14 sistem ve 9 eski tanım. JSON varlığı tek başına oyun/loot/görsel kabulü değildir. Önceki runtime kanıtları [uygulama belgesinde](MODULE_IMPLEMENTATION_EVIDENCE.md) tarihlidir; 2026-10-24 bölge seviyesi ve Rare/Uncommon/Epic uygulaması: tam derleme ve modül/Epic/editör içerik testlerinden 38/38 geçti.

| Kapsam | Mevcut durum / kalan sınır |
|---|---|
| Reflection Shield | Electric hasar ve yansıma başına 1 Electric stack uygulandı (2026-10-24); stack sayısı yer tutucu. |
| Survey Relay | Planlı; rota ödüllerinin modül ailesi verisi eksik. |
| Silent Running | Planlı; seyahat Threat artışının sahibi eksik. |
| Danger Pay | Planlı; Threat koşulu ve Gold/Scrap ödül akışı eksik. |
| Interest Ledger | Planlı; nokta tamamlanma olayı eksik. |
| Shield Battery / Shield Rupture | Uygulandı (2026-10-24); rezerv tavanı, patlama yarıçapı ve pencere yer tutucu, ortak yan havuzu yok. |
| Epic17 / Legendary4 | Pathfinder dışındaki 14 Epic uygulandı (2026-10-24); Phase Slip ve Momentum Drive kısmen. Resonant Barrel ve Monochrome Core 2026-10-25 uygulandı; Legendary dördü planlı. Ultimate Protocol 2026-10-25 Epic sayımından çıktı (nadirliksiz yetenek modülü). 2026-10-24 kararlarının uygulaması Sol'a bekliyor. Alternating Current 2026-10-24'te Prism Core yanına taşındı; Storm Capacitor da ana sayım dışında Prism Core yanıdır. |
| Yan özellikler | Yeni seviyeler için seçilmemiş büyüme eklenmez. Four Corners’ın hasarsız etkileri ve planlı yolculuk yanlarının sistem bağlantıları açık. |
| Ekonomi ve oyuncu akışı | Koşu Gold cüzdanı/harcama API’si var; mağaza, loot, yükseltme harcaması/yeniden seçim, modül kalıcılığı ve Rare sunumu ayrı kapsam. |

[GalaxyRoute](../LightYearsGame/include/gameplay/galaxy/GalaxyRoute.h) yıldız/durak verisi sağlar; ödül/Threat/mağaza sözleşmesi yerine geçmez. [F1 yerleşim çekirdeği](../LightYearsGame/src/gameplay/module/ModulePlacementRules.cpp) ve [parça loadout API’si](../LightYearsGame/src/gameplay/module/ModuleLoadout.cpp) mevcuttur; çekirdek kod bütün ekran/edinme akışının kabulü değildir.

Önceki 2026-10-14 kanıtı Game build exit0 ve yedi focused E2E’de 1517 geçen kontrol içerir. Güncel düzenlemenin kanıtı Markdown/bağlantı/sayım ve kaynak eşleştirme kontrolleridir. Uygulama planları: [Common](MODULE_COMMON_IMPLEMENTATION_PLAN.md), [Uncommon](MODULE_UNCOMMON_IMPLEMENTATION_PLAN.md), [Rare](MODULE_RARE_IMPLEMENTATION_PLAN.md). Eski planların dalga ve sayımları tarihli kayıt, güncel seçim sahibi bu katalogdur.
