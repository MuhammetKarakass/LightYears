# LightYears — Balance & Roadmap Notebook

## Uygulanan ability: Dash / Ability.Dash.Basic

| Alan | Not |
| --- | --- |
| Statü | Uygulandı |
| Slot ve input | Ability3 / F |
| Activation / lifetime | OnPressed / 0.24 sn Duration |
| Cooldown / duration / charge | L1-L5: 2.0 / 1.88 / 1.76 / 1.64 / 1.52 sn; taban cooldown'un level başına %6'sı düşer; 1 charge |
| Davranış | Girdi yönü normalize edilir; girdi yoksa mouse aim kullanılır; ikisi de geçersizse dash başlamaz |
| Mesafe | Base 260; yatay ve dikey movement rating ortalaması ile diminishing ve en fazla +%50 |
| Momentum | Dash öncesi velocity’nin X/Y bileşenleri tamamen korunur; impulse bunun üzerine eklenir ve bitişte aynı velocity devam eder |
| Damage / savunma / enerji | Damage, invulnerability ve energy cost yok |
| Kamera | Dash impulse speed zoom’a verilmez; son normal velocity ve kamera–gemi follow offset'i korunur; mevcut speed/afterburner zoom hedefinin üzerine +%15 göreli zoom-out, zoom velocity sürekliliği olan kritik sönümlü smoothing ile uygulanırken cursor look-ahead çalışır |
| Lifecycle | State.Ability.Dashing, Event.Ability.Dash.Start ve Event.Ability.Dash.End |
| Kod sınırı | gameplay/ability/dash/DashAbility; dar köprü DashMovementController; fizik MovementComponent |
| Sonrası | L6 Evolve ertelendi; ayrı behavior/actor gerekiyorsa aynı `dash/` vertical slice'ında ele alınacak |

## Uygulanan ability: Rocket / Ability.Rocket.Basic

| Alan | Not |
| --- | --- |
| Statü | Uygulandı |
| Slot ve input | Ability4 / R |
| Activation / lifetime | OnPressed / Instant; tek Rocket projectile actor üretir |
| Cooldown / charge | 7.0 sn taban cooldown; 1 charge; AbilityHaste final cooldown'u merkezi eğri üzerinden azaltır |
| Davranış | Owner önünde spawn olur, mouse aim yönünde gider, ilk uygun çarpışmada patlar; range sonunda sadece cleanup olur |
| Damage tag / crit | Kinetic; merkezi damage pipeline crit ve armor penetration davranışını çözer |
| Base attribute'lar | Damage 55; projectile speed 1000; range 1100; explosion radius 55; projectile count 1 |
| Owner scaling | ResolvedDamage = level ile gelişmiş base damage + AttackPower × 1.25 |
| Level 2–15 | Her level +4 damage, -0.12 sn cooldown, +1 explosion radius; speed ve range bilerek sabittir |
| Kod sınırı | `gameplay/ability/rocket/RocketAbility`, `RocketProjectileActor`, `gameConfigs/ability/RocketConfig.h` |
| Sonrası | L6 ve L15 evolve seçimleri ertelendi; Basic Rocket'te homing, split, multi-rocket veya elemental davranış yok |

## Uygulanan ability: Gravity Anomaly / Ability.GravityAnomaly.Basic

| Alan | Not |
| --- | --- |
| Statü | Uygulandı; player varsayılan loadout'unda Shield'in yerine grant edilir |
| Slot ve input | Ability1 / Q; OnPressed, cursor hedefi |
| Cooldown / charge | 8.0 sn taban cooldown; 1 charge; AbilityHaste final cooldown'u merkezi eğri üzerinden azaltır |
| Delivery | Owner önünde spawn olan projectile cursor'a gider; hedef 900 menzile clamp edilir, homing/collision ile patlama yoktur |
| Field | Projectile hedefe ulaşınca 2.5 sn, 220 radius alan üretir; projectile ve pickup'lar hariç caster/player/enemy Combatant'ları etkiler |
| Movement / effect | İçeride her hedefe field-source scoped, 2 sn refresh-duration `Effect.GravityAnomaly.Inside` uygulanır; içeride süre sürekli 2 sn'ye resetlenir. Çıkışta/field bitiminde pull kapanır, `%20 MovementSlow` ve visual 2 sn daha sürer. Pull velocity'ye `500 * (1-d/radius)^2 * dt` ekler |
| Damage | Damage tag, DamageContext, crit ve hasar uygulaması yok |
| Owner scaling | MaxHealth: radius +0.20 x MaxHealth, duration +0.0025 x MaxHealth; diğer resolved değerler değişmez |
| Level 2-15 | Her level: cooldown -0.10 sn, duration +0.03 sn, radius +2, pull +10, slow +0.005, projectile speed +25, cast range +5. L15: 6.6 sn / 2.92 sn / 248 / 640 / %27 / 2350 / 970 |
| Kod sınırı | `gameplay/ability/gravityAnomaly/`, `gameplay/effects/gravityAnomaly/`, `gameConfigs/ability/GravityAnomalyConfig.h`, typed presentation ve effect visual aile klasörleri |
| Presentation | `RegisterGameAbilityPresentationContent()` projectile ve field için ayrı typed profile kaydeder; field world halkaları/inward particles çizer, hedef üzerindeki effect visual ayrı registry kaydıyla oluşur |
| Sonrası | Value-only evolve mevcut typed profile tipinde yeni kayıt olur; yapısal evolve aynı ailede ayrı profile/actor/handler alır |

Bu dosya, uygulanmış sistem referansından ayrı tutulmuş yaşayan çalışma
notudur. Buradaki “Fikir” ve “Plan” maddeleri kodda var kabul edilmez.
Uygulama tamamlandığında sonucu
[Project Documentation](PROJECT_DOCUMENTATION.md) içine taşıyın.

Yeni bir satırı doldurmadan önce
[Current Implementation Catalog](CURRENT_IMPLEMENTATION_CATALOG.md) içinden
mevcut değer, kaynak dosyası ve kod durumunu kopyalayın. Böylece “hedef
değer” ile “şu an çalışan değer” birbirine karışmaz.

## Kullanım kuralları

| Statü | Anlamı |
| --- | --- |
| Fikir | Araştırılacak; uygulanmamış |
| Deney | Oyun içinde veya testte ölçülüyor |
| Onaylı | Tasarım kararı verildi, geliştirme bekliyor |
| Uygulandı | Kodda mevcut; ana referansa taşınmalı |
| Reddedildi | Bilerek yapılmayacak; gerekçe korunur |

Her denge kaydı mümkünse şu dört alanı içermelidir: hedef davranış, değişen
değer, ölçüm / test sahnesi, sonuç.

Zorunlu kayıt kuralı: Uygulanmış her değişiklik (denge, kod, konfigürasyon,
roadmap veya dokümantasyon) aynı
değişiklik setinde **BALANCE_AND_ROADMAP_NOTEBOOK.md**,
**CURRENT_IMPLEMENTATION_CATALOG.md** ve **PROJECT_DOCUMENTATION.md**
dosyalarına yazılır. Bu üç kayıt güncellenmeden değişiklik tamamlanmış sayılmaz.

## 1. Karar günlüğü

| Tarih | Alan | Karar / değişiklik | Statü | Neden / sonuç |
| --- | --- | --- | --- | --- |
| 2026-07-24 | Elemental hasar | Thermal, Cryo ve Electric yalnızca 4. vuruşta tam stack’e ulaşır; ara stack’ler görünür fakat combat etkisi üretmez. Kinetic penetration ve tüm elemental payoff değerleri düşürüldü. | Uygulandı | Sık kullanılan silahlarda sürekli ara-stack kazancı ile Electric’in tüm kaynaklardan %50 vulnerability vermesi kaldırıldı. GasLiteCoreTests eşik, değer ve süre bitişini doğrular. |
| 2026-07-24 | Cryo slow sustain | Tamamlanmış Cryo slow, sonraki her Cryo isabetinde 1.5 sn olarak yenilenir; şiddet %25’te kalır. | Uygulandı | Cryo’nun dört-vuruş payoff’ı sürekli ateşte zayıf kalmamalı; magnitude stacklenmediği için etki kontrollü kalır. |
| 2026-07-24 | Runtime mimarisi | ShipRuntime eklendi. Shield/afterburner türetmeleri ve ship attribute'ları CombatRuntime'dan SpaceShip sahipliğindeki ShipRuntime'a taşındı. | Uygulandı | CombatRuntime yalnızca combat attribute, effect ve ability akışında kalır. Bu sınır ileride EncounterRuntime ve EnemyRuntime gibi bağımsız runtime'ların eklenmesini kolaylaştırır. |
| 2026-07-24 | Ability mimarisi | Generic core, ortak ability actor altyapısı ve ability-family vertical slice sınırı sabitlendi. Dash, Shield ve SunBeam ayrı behavior sınıflarıdır; config'leri aile bazında ayrıdır. | Uygulandı | Yeni ability/evolve aynı aile klasöründe büyür. Somut actor yalnız ilgili ailede kalır; ancak birden fazla aile gerçekten paylaştığında `ability/actors` altına alınır. Core somut ability tipine branch etmez. |
| 2026-07-24 | Ability presentation mimarisi | Global `VisualConfig.h` ve `AbilityVisualStructs.h` kaldırıldı. Actor config'i tek `presentationProfileId` taşır; Rocket ve SunBeam feature-local concrete profile'larını typed registry üzerinden çözer. Shield visual içeriği de feature-local definition/ID/content dosyalarına ayrıldı. | Uygulandı | Yeni ability ve value-only evolve aynı aile altında yeni typed profile kaydeder. Yapısal olarak farklı evolve base profile'a optional alan/flag yığmaz; aynı ailede ayrı profile ve actor/handler alır. Bağlayıcı kontrat `PROJECT_DOCUMENTATION.md` §3.0.1 içindedir. |
| 2026-07-24 | Rocket Basic | Ability.Rocket.Basic ayrı Rocket behavior ve projectile actor olarak eklendi. Damage/radius/cooldown normal level ilerlemesinde gelişir; projectile speed ve maksimum range delivery kimliği olarak sabit kalır. | Uygulandı | Rocket cursor menzil içindeyse cursor konumunda, cursor uzaktaysa 1100 maksimum menzilde patlar; telegraph gerçek clamp edilmiş patlama noktasını gösterir. Çarpışma daha önce gerçekleşirse alan içindeki her uygun hedefe bir kez merkezi Kinetic DamageContext uygulanır. Gelecek evolve'lar aynı `rocket/` ailesine behavior/actor bileşimi olarak eklenir. |
| 2026-07-24 | Dash momentum ve kamera kontrolü | Dash öncesi velocity tamamen korunup Dash impulse üzerine eklendi. Kısa süreli impulse speed zoom’dan çıkarıldı; kamera merkezi Dash displacement’ıyla birlikte taşınıyor. Dash, o andaki normal speed/afterburner kamera hedefinin üzerine +%15 göreli zoom-out ekliyor. | Uygulandı | Gemi Dash sonrasında hızını kaybetmez. Kamera–gemi offset'i korunur; Dash başında yaklaşma olmaz. Göreli katman kritik sönümlü zoom hattıyla girip çıkar; hedef kapanınca zoom hızı bir karede tersine dönmez. Momentum, kamera offset, göreli zoom ve zoom-velocity regresyon testleri eklendi. |
| 2026-07-24 | Dash tuning güvenliği | Dash cooldown progression sabit `-0.3` yerine taban cooldown'un level başına %6'sı olarak tanımlandı; player ability grant hataları sebebiyle loglanıyor. | Uygulandı | Taban cooldown 1 sn yapılınca L5'in negatif cooldown üretip Dash'in hiç grant edilmemesi düzeltildi. Yapısal testler geçerli sayısal tuning değişikliklerini sabit eski değerler yüzünden reddetmez. |
| 2026-07-24 | Primary weapon scaling | Rapid Laser 1.0 AP/1.0 AS korunurken Shotgun 0.75/0.50, Dual Kinetic 0.45/1.0, Electric 0.85/0.60, Beam 0.75 AP + 0.50 EnergyMax ve Cryo 0.75/0.50 olarak ayarlandı. | Uygulandı | Tekrarlanan ana silahlarda erken/orta oyun büyümesi indirildi; Dual'ın iki muzzle toplamı L50'de Rapid Laser'ın altında kaldı. Production runtime testleri L1/10/25/50 değerlerini doğrular. |
| 2026-07-24 | Electric / Beam special scaling | Electric Luck artık hasar scale'ı değil, normal zincirlerden sonra tek ek uygun zincir için merkezi combat Luck ile en fazla %35 şanstır. Beam AttackSpeed yalnızca 50% heat sonrası heat gain'i azaltır; 75–100% aralığında doygun eğriyle en fazla %35'tir. | Uygulandı | Electric hedef tekrarını ve hedef yokken proc'u engeller. Beam AttackSpeed doğrudan DPS/tick/fire rate eklemez, overheat'i kaldırmaz; sadece yüksek heat penceresini uzatır. |
| 2026-07-25 | Gravity Anomaly Basic | Gravity Anomaly player loadout'unda Ability1/Q'ya taşındı ve bu slotta önceki Shield grant'inin yerini aldı. Projectile cursor'a gider, sabit hedefte damage'siz field oluşturur. İçeride slow süresi sürekli 2 sn'ye yenilenir; alan terkinde veya field bitiminde pull hemen kapanır, slow/visual 2 sn sonra normal effect expiry ile temizlenir. | Uygulandı | Ayrı field'lar source scope ile birbirinin slow/pull effect'ini silmez. GasLiteCoreTests loadout/slot, clamp, lifecycle, target filtreleme, pull, exit/destroy tail, hareket slow, multi-field expiry, level/MaxHealth scale ve visual cleanup'ı doğrular. |
| 2026-07-25 | Yeniden kullanılabilir gameplay effect mimarisi | Shipped effect'ler merkezi katalogda immutable `GameplayEffectDefinition` olarak tutulur; her ability, weapon/status, enemy, reward veya area uygulaması kaynağa özel `GameplayEffectSpec` üretir; hedef mutable `ActiveGameplayEffect` sahiplenir. Barrier, Ignite, Electric ve Gravity davranışları generic hook registry üzerinden çalışır. Alan yaşam döngüsü `AreaGameplayEffectApplicator` ile ortaklaştırıldı. | Uygulandı | Yeni slow, burn, haste, attack-speed veya benzeri effect için yeni bir effect system sınıfı yazılmaz. Data/modifier yeterliyse yalnız katalog tanımı; özel tick/damage gerekirse kayıtlı hook; feature'a özgü context gerekiyorsa typed runtime context eklenir. Core effect ID'lerine branch etmez. Katalog, spec izolasyonu, geçersiz behavior/visual/action reddi ve Debug/Release yaşam döngüsü testleri eklendi. |
| 2026-07-27 | Engine diagnostics | Ortak `LY_ASSERT`/`LY_VERIFY`, CORE/GAME seviyeli structured logging ve RAII scope/counter profiler eklendi. Entry point başlangıç/kapanış sahipliğini üstlendi; application, world, ability, effect ve Gravity Anomaly kritik yolları instrument edildi. | Uygulandı | Debug invariant ihlalleri kaynak konumuyla durur; beklenen Release kontrolleri `LY_VERIFY` ile çalışmaya devam eder. Release profiler/assert maliyeti compile-out edilir, Warning+ loglar ve `LightYears.log` hata izi kalır. |
| YYYY-AA-GG |  |  | Fikir |  |
| YYYY-AA-GG |  |  |  |  |
| YYYY-AA-GG |  |  |  |  |

## 2. Ability tasarım kartları

Her yeni veya değişen ability için bir kart kopyalayın.

### Ability: [Ad / ID]

| Alan | Not |
| --- | --- |
| Statü | Fikir / Deney / Onaylı / Uygulandı |
| Slot ve input |  |
| Activation / lifetime |  |
| Cooldown / duration / charge |  |
| Hedef oyuncu davranışı |  |
| Counterplay |  |
| Damage tag / element |  |
| Action ve trigger özeti |  |
| Base attribute’lar |  |
| Owner scaling |  |
| Level 2–N ödülleri |  |
| Scrap maliyeti |  |
| Attachment slot/capability |  |
| VFX / SFX / HUD ihtiyacı |  |
| Kod kaynağı / mevcut durum |  |
| Değişiklikten önceki runtime değer |  |
| Hedef runtime değer |  |
| Ölçüm senaryosu |  |
| Sonuç / karar |  |

Notlar:

-

## 3. Weapon denge kartları

### Weapon: [Ad / ID]

| Alan | Değer / not |
| --- | --- |
| Statü | Fikir / Deney / Onaylı / Uygulandı |
| Type tag |  |
| Damage type tag |  |
| Base damage |  |
| Fire rate veya DPS |  |
| Muzzle sayısı |  |
| Range / speed / lifetime |  |
| Özel mechanic | Heat / chain / shotgun / wave / pierce |
| Owner scaling |  |
| Attachment uyumu |  |
| Level progression ve scrap |  |
| Kod kaynağı / mevcut durum |  |
| Değişiklikten önceki runtime değer |  |
| Hedef runtime değer |  |
| Tek hedef teorik DPS |  |
| Çok hedef teorik DPS |  |
| Ölçülen DPS |  |
| Test hedefi | HP / armor / shield / element status |
| Risk / counterplay |  |
| Son karar |  |

### Hızlı silah test günlüğü

| Tarih | Silah | Level | Build / attachment | Hedef | Beklenen | Ölçülen | Karar |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-07-24 | Tüm player primary suite | 1 / 10 / 25 / 50 | Weapon L1, attachment yok, crit/armor yok, %100 hit | Tek hedef; Electric 1/2/4/5 hedef; Beam 10/30 sn | Runtime resolved scaling ve özel mekanik sınırları | GasLiteCoreTests geçti; DPS ve status/heat kontratları doğrulandı | Uygulandı |
| YYYY-AA-GG |  |  |  |  |  |  |  |
| YYYY-AA-GG |  |  |  |  |  |  |  |

## 4. Gemi, savunma ve ekonomi notları

### Ship: [Ad / ID]

| Alan | Değer / not |
| --- | --- |
| Statü |  |
| Health / armor |  |
| Base shield / recharge / delay |  |
| EnergyMax ile türeyen değerler |  |
| Afterburner cap / regen / drain |  |
| Thrust / strafe / max speed / damping |  |
| Başlangıç silahı |  |
| XP base / exponent |  |
| Level growth override’ları |  |
| Score / ship XP / loot |  |
| Rol ve zayıflık |  |
| Kod kaynağı / mevcut durum |  |
| Değişiklikten önceki runtime değer |  |
| Hedef runtime değer |  |
| Test sonucu |  |

### Damage / savunma değişiklikleri

| Tarih | Değişken | Eski | Yeni | Etkilenen türler | Örnek hesap | Test sonucu |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-07-24 | 4-hit elemental payoff | Thermal 3×2 DPS; Cryo 3 hit/%40/2 sn; Electric 5×%10 | Thermal 4×1 DPS/3 sn; Cryo 4 hit/%25/1.5 sn; Electric 4×%4/3 sn | Thermal, Cryo, Electric | Full Thermal = 4 DPS; full Electric = x1.16 | GasLiteCoreTests: ara stack etkisiz, 4. hit etkin, süre bitişi doğrulandı |
| 2026-07-24 | Cryo slow refresh | Aktif slow yenilenmiyordu | Aktif slow, her yeni Cryo hit’inde 1.5 sn’ye tazelenir | Cryo | 1.0 sn + Cryo hit + 0.75 sn sonrasında slow hâlâ aktiftir | GasLiteCoreTests doğrular |
| 2026-07-24 | Kinetic armor penetration | 0.25 taban, Kinetic Bore +0.15 | 0.10 taban, Kinetic Bore +0.05; sistem üst sınırı 0.25 | Kinetic | 50 armor reduction’da effectiveArmor = 0.50×(1-0.10)=0.45 | GasLiteCoreTests taban penetration=0.10 doğrular |
| YYYY-AA-GG | ArmorScale |  |  |  |  |  |
| YYYY-AA-GG | Shield multiplier |  |  |  |  |  |
| YYYY-AA-GG | Crit multiplier |  |  |  |  |  |

Hatırlatma:

- Armor, doygun üstel eğri kullanır; düz yüzde gibi belgelenmemelidir.
- Electric hedefin aldığı hasarı armor öncesinde çarpar.
- Energy, shield kapasite tüketimini artırır.
- Birden fazla damage tag’in payload önceliği kod tarafından belirlenir;
  hibrit tür eklenmeden önce açık tasarım kararı gerekir.

## 5. Kamera notları

| Tarih | Arena / level | baseZoom | Cursor LA | Movement LA | Speed zoom | Smoothing | Test sonucu |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 2026-07-24 | Tüm ArenaLevel türevleri | 1.55 | .45 / 380 / 600 | .75 / 320 | 500’de +.32; Dash impulse hariç; Dash mevcut hedefe göre +%15 | Position 2.75 / zoom 3.2 kritik sönümlü / look-ahead 1.65; Dash follow offset ve göreli zoom korunur; bounds clamp smoothed view ile eşzamanlıdır | Build; kamera offset, düşük/yüksek hız additive zoom, zoom-velocity sürekliliği, smooth recovery ve arena sınırı regresyonları geçti; oyun içi his kontrolü bekliyor |
| YYYY-AA-GG |  |  |  |  |  |  |  |
| YYYY-AA-GG |  |  |  |  |  |  |  |

### Kamera hipotezleri

- [ ] Yakın dövüş / shotgun arena profilinde daha dar base zoom test et.
- [ ] Hızlı afterburner sırasında zoom-out üst sınırını ölç.
- [ ] Mouse look-ahead dead zone’un nişan kararlılığına etkisini kaydet.
- [ ] Shake yoğunluğunu hit feedback ile motion comfort açısından test et.

## 6. Arena ve level notları

### Arena: [Ad / ID]

| Alan | Değer / not |
| --- | --- |
| Statü |  |
| Legal bounds / boyut |  |
| Margin / grace time |  |
| Boundary visual |  |
| Respawn konumu / policy |  |
| Kamera profili |  |
| Hareket modu |  |
| Sınır cezası |  |
| Spawn / wave kuralları |  |
| Hedef mücadele süresi |  |
| Kod kaynağı / mevcut durum |  |
| Değişiklikten önceki runtime değer |  |
| Hedef runtime değer |  |
| Son playtest sonucu |  |

### Wave / zorluk deneyleri

| Tarih | Level | Wave aralığı | Enemy count | Spawn interval | Difficulty | Sonuç |
| --- | --- | --- | --- | --- | --- | --- |
| YYYY-AA-GG |  |  |  |  |  |  |
| YYYY-AA-GG |  |  |  |  |  |  |

## 7. Önerilen roadmap

Bu liste yalnızca planlama içindir; işaretlenmiş olması implementasyon
anlamına gelmez.

### P0 — ölçülebilir çekirdek denge

- [ ] Tek hedef / çok hedef / shield / armor için tekrar üretilebilir combat
  test sahnesi kur.
- [ ] Her oyuncu silahı için level 1–4 teorik ve ölçülen DPS tablosunu doldur.
- [ ] Damage type önceliği veya gerçek hibrit payload tasarımına karar ver.
- [ ] Tooltip/HUD’da oyuncuya gösterilen değer ile runtime çözülmüş değeri
  karşılaştır.

### P1 — build çeşitliliği

- [ ] Her damage type için en az bir sinerji ve bir karşı oyun senaryosu tanımla.
- [ ] Attachment drop/ekonomi kaynağını ve rarity politikasını belirle.
- [ ] Ability / primary weapon attachment slot kapasitesini build temposuna göre
  test et.
- [ ] Ship progression büyümesini wave zorluğu ile aynı ölçekte kalibre et.

### P2 — içerik ve sunum

- [ ] Arena encounter çekirdeği: oyuncuya göre spawn director, iki düşman
  türünün arena davranışına dönüşümü ve bir arena boss prototipi.
- [ ] Yeni arena boundary visual türleri: EnergyWall veya WarningField.
- [ ] Arena özel kamera preset’leri.
- [ ] Yeni ability actor türleri ve telegraph standardı.
- [ ] Silah/element durum ikonları, combat log veya training HUD.
- [ ] Boss mekaniklerinin ability/effect sistemine veri odaklı taşınması.

### P3 — mühendislik / kalite

- [ ] Damage pipeline, level progression, attachment koşulları ve weapon
  validator için otomatik test kapsamını artır.
- [ ] Config doğrulama hatalarını UI veya başlangıç logunda görünür yap.
- [ ] Denge değerleri için tek bir export/rapor akışı ekle.
- [ ] Playtest telemetrisi: DPS, alınan hasar, ölüm nedeni, out-of-bounds.

## 8. Playtest kayıtları

| Build / commit | Harita | Loadout | Süre | Ölüm nedeni | En güçlü his | Sorun | Takip işi |
| --- | --- | --- | --- | --- | --- | --- | --- |
|  |  |  |  |  |  |  |  |
|  |  |  |  |  |  |  |  |
|  |  |  |  |  |  |  |  |

## 9. Açık sorular

- [ ] Damage type’ler birden fazla tag taşıdığında hangi davranışlar
  birleşmeli, hangileri dışlanmalı?
- [ ] Critical, damage-over-time ve chain damage için istenen kurallar nedir?
- [ ] Barrier, kalıcı shield ve armor arasında hedeflenen savunma kimliği ne?
- [ ] Her arena için farklı out-of-bounds ceza türü gerekli mi?
- [ ] Oyuncunun ekranda görmesi gereken resolved stat’lar hangileri?
- [ ] İleride save/load, build export veya run özeti hangi verileri taşımalı?
