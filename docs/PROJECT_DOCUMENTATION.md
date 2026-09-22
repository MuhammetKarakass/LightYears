# LightYears — Sistem Referansı

> 20 Eylül 2026 çalışma ağacı karşılaştırması: [7 Eylül 2026 proje durum raporu](<vault/06 - Status and Plans/2026-09-07 Project Status Review.md>) tarihsel bir source audit'tir. Runtime başlangıç slotlarının tek canonical sahibi [`DefaultAbilityLoadout.cpp`](../LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp)'dir; bu belge güncel slot tablosunu kopyalamaz. Çalışma ağacı dirty olduğundan bu doküman güncellemesinde build/test çalıştırılmadı; tarihsel teknik kararlar ve doğrulanmamış test iddiaları güncel runtime kanıtı değildir.


## Dash implementation

`Ability.Movement.Dash.Basic`, kendi behavior contract'ı ve tarihsel Ability3/F
slot profili olan ayrı bir ability behavior sınıfıdır:
`gameplay/ability/dash/DashAbility`. Slot binding default loadout'tan ayrıdır.
`AbilitySystem`, davranışı
`AbilityBehaviorRegistry` üzerinden üretir; game adaptörü
`GameAbilityActionExecutor` yalnızca
ortak action türlerini çalıştırır. Dash validation, start/end event ve state tag
cleanup bu sınıfta kalır. `DashMovementController` ability ile gemi hareketi
arasındaki dar kontrattır; `MovementComponent` input-or-mouse yönünü ve fiziksel
hareketi uygular. `PlayerSpaceShip` içinde Dash davranışı bulunmaz.

Base mesafe 260'dır. Yatay ve dikey movement rating ortalaması `1-exp(-rating/20)`
eğrisiyle en fazla +%50 mesafeye dönüşür. AttackPower, AttackSpeed, EnergyPower,
Luck, Critical ve ability level mesafeyi değiştirmez. Level 2-5 yalnızca cooldown'u
2.0'dan 1.88/1.76/1.64/1.52 saniyeye indirir. Seviye başına düşüş taban
cooldown progression'da her seviye için authored sabit `-0.12` değeridir;
otomatik yüzde kuralı değildir. Ability Haste merkezi cooldown çarpanı
üzerinden en son uygulanır.

Dash impulse hızı `resolvedDistance / 0.24` olarak sabittir ve mevcut velocity
üzerine eklenir:

~~~text
dashVelocity = preservedVelocity + normalizedDirection * (resolvedDistance / duration)
postDashVelocity = preservedVelocity
~~~

Bu nedenle Dash öncesindeki yatay/dikey momentum eksiksiz korunur; Dash bittiğinde
gemi aynı velocity ile normal thrust'a döner. Hareket actor offset yolundan
gittiği için arena boundary kurallarını atlamaz. Lifecycle tag/eventleri
`State.Ability.Dash.Active`, `Event.Ability.Dash.Start` ve `Event.Ability.Dash.End`'dir.

Dash velocity kısa süreli bir displacement impulse kabul edilir ve kameranın
speed zoom / movement look-ahead girdisine verilmez. Arena kamerası Dash
boyunca son normal velocity girdisini korur. Kamera merkezi ayrıca geminin her
Dash-frame displacement'ı kadar taşınarak mevcut kamera–gemi offset'ini korur;
cursor look-ahead çalışmaya devam eder. Dash'in `0.15` kamera oranı normal
speed/afterburner zoom kompozisyonunun üzerine göreli olarak eklenir ve mevcut
zoom smoothing hattıyla girip çıkar. Böylece kamera Dash başında yaklaşmaz,
mevcut hız ve arena sınırı davranışını ezmez, ani zoom darbesi üretmez ve
geminin gerisinde kalıp sonradan yetişmez.

> Amaç: Bu dosya projenin güncel teknik ve oyun tasarımı referansıdır. Kodda bir
> sistem veya sayı değiştirildiğinde, ilgili bölüm ve
> [Balance & Roadmap Notebook](BALANCE_AND_ROADMAP_NOTEBOOK.md) birlikte
> güncellenmelidir.
> Güncel teknik gerçekler bu belgede tutulur; JSON envanteri
> [Current Implementation Catalog](CURRENT_IMPLEMENTATION_CATALOG.md)'a, tarihli
> karar ve roadmap ise [Balance & Roadmap Notebook](BALANCE_AND_ROADMAP_NOTEBOOK.md)'a
> aittir. Etkilenen belgeler arasında bağlantı kurun, aynı tabloyu çoğaltmayın.
>
> Kaynak anlık görüntüsü: 7 Eylül 2026 tarihsel kaydı. Bu bölüm, çalışma
> ağacındaki o tarihteki teknik sınırları açıklar; önerilen fikirler yalnızca notebook dosyasında
> tutulur. JSON kayıt sayıları için [Current Implementation Catalog](CURRENT_IMPLEMENTATION_CATALOG.md)
> tek envanterdir.

## 0. Runtime değişiklikleri (7 Eylül 2026 tarihsel kaynak anlık görüntüsü)

Bu anlık görüntüde C++ identity/behavior/schema katmanı ile JSON balance katmanı
birlikte çalışır. `abilities.json`, `effects.json` ve
`damage_status_balance.json` yüklenmişse shipped runtime değerleri JSON'dan gelir;
damage-status balance dosyası eksik veya geçersizse bootstrap başarısız olur ve
C++ shipped numeric fallback'i kullanılmaz. C++ tarafı ability kimliğini, behavior
türünü, actor türünü, presentation profile tipini ve validasyon sözleşmesini
sahibi olmaya devam eder.

- `sas::AbilityRuntimeBinding`, ability'nin content tanımından bağımsız runtime
  ekipman slotunu taşır. Runtime başlangıç slotlarının canonical sahibi
  [`DefaultAbilityLoadout.cpp`](../LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp)'dir;
  bu bölüm slot eşlemesini tekrar etmez.
- `abilities.json` 55, `effects.json` 19, `weapons.json` 10, `ships.json` 4
  (1 player + 3 enemy) ve `attachments.json` 2 kayıt içerir. Bu sayılar
  registration envanteridir;
  aktif slot, acquisition UI veya uçtan uca oynanış kanıtı değildir. Tam ID
  listesi [Current Implementation Catalog](CURRENT_IMPLEMENTATION_CATALOG.md)
  içindedir.
- Relay Prism, farenin konumunda ability projectile yakalayıp lineage korumalı
  dost clone'lara dönüştüren fizik dışı bir query actor'dür. Execution Drive,
  kill-confirmed olayından stack alır; temporary AttackPower effect'i ve düşük
  canlı hedefe chase movement modifier'ı birlikte yönetir.
- Area telegraph presentation, typed profil sınırını koruyarak sabit/takip,
  zamanlı/external progress ve completion feedback fazlarına ayrılmıştır. Hull
  Shock gerçek alan yarıçapını aynı radial resolver'dan türetir.
- Shield Harvest'in temporary overshield'ı kalıcı ship shield ve Barrier
  effect'inden ayrı bir runtime katmanıdır.
- Ion Storm, cursor'a giden hasarsız projectile'ın hedefte 4 saniyelik Electric
  alan açtığı bir delivery/field family'sidir. Alanın düzensiz sınırı cast başına
  bir kez üretilir; aynı `IonStormBoundary` hem gameplay hedef filtresinde hem de
  presentation çiziminde kullanılır. 250 inner core, 250–335 dış sınır, 0.25 sn
  tick ve 16 tick sabittir; Common.Damage 6 + `Owner.AttackPower × 0.12` ile
  çözülür ve seviyeler yalnız damage/cooldown'u geliştirir. Presentation tek
  renkli tek dolu düzensiz şekildir; iç dolgu katmanı ve iç enerji çizgileri yoktur.
- Time Slip, player'ı gerçek zamanda bırakıp `HostileGameplay` actor domain'ini
  seçici olarak 0.35× yavaşlatan temporal ability'dir. Enemy gemileri ve owner'
  ından türeyen hostile actor'lar ve tüm player/hostile projectile'lar ortak
  `ProjectileGameplay` domain'inde aynı 0.35× çarpanı kullanır; player primary
  fire cadence'i ayrıca 0.35× olurken movement, diğer ability cooldown'ları,
  camera ve UI gerçek zamanda kalır. Süre L1'de 3.0 sn'dir,
  level başına +0.10 sn ve cooldown başına -0.20 sn ilerler; `EnergyPower` bonusu
  `max(0, EnergyPower - 50) × 0.0015` ile uygulanır.

Bu bölüm, tarihsel migration notlarının yerine geçmez; eski bölümlerdeki
"varsayılan loadout" ve "son doğrulama" ifadeleriyle çelişirse bu bölüm ile
runtime source-of-truth önceliklidir. Enemy/boss C++ ShipConfig weapon
literal'ları shipped `weapons.json` kayıtlarına dahil değildir. Input gönderimi
de grant edilmiş weapon/ability bulunduğunu tek başına kanıtlamaz.

Fighter arena test loadout'unun canonical kaynağı
[`DefaultAbilityLoadout.cpp`](../LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp)'dir.
Bu belge slot listesini kopyalamaz. Kaynakta tanımlanan seçim; Cryo alan kontrolü,
kümelenmiş enemy'ler arasında plague yayılımı, alan patlama hasarı ve savunma
senaryosunu aynı arena akışında denemek için kullanılır; güncel exact girişler
source owner dosyasından okunmalıdır.

## Durum anahtarı

| İşaret | Anlamı | Dokümantasyonda kullanım biçimi |
| --- | --- | --- |
| ✅ Uygulandı | Config, runtime ve çağrı yolu kodda mevcut | Mevcut değeri değiştirmeden önce test hedefini yaz |
| 🟡 Mevcut, varsayılan akışta değil | Kod/config mevcut; fakat uygulamanın mevcut başlangıç dünyasında doğrudan açılmıyor | LevelOne, alternatif silahlar gibi içerikler |
| ⚙️ Runtime hazır | Sistem ve API hazır; content/loadout seçimi henüz otomatik akışa bağlanmamış olabilir | Attachment ve alternatif loadout gibi alanlar |
| 🧪 Ölçüm bekliyor | Kod durumu değil; dengesi için ölçülmüş playtest kaydı eksik | Notebook’a hedef ve sonucu gir |

Eksiksiz mevcut içerik, çağrı yolu ve düzenlenebilir alan envanteri için:
[Current Implementation Catalog](CURRENT_IMPLEMENTATION_CATALOG.md).

## Hızlı yönlendirme

| Konu | Kaynak odak noktası | Not alınacak yer |
| --- | --- | --- |
| Genel mimari | LightYearsEngine, SpaceAbilitySystem ve LightYearsGame | Bu dosya, §1 |
| Mevcut içerik / durum envanteri | Tüm config ve runtime bağlantıları | Current Implementation Catalog |
| Hasar, zırh, kritik, statüler | gameplay/combat, gameplay/damage, gameplay/effects | §2 ve Notebook §4 |
| Attribute / ability scaling | gameplay/attributes, gameplay/ability | §3 ve Notebook §2 |
| Silahlar ve ilerleme | gameConfigs/Weapon*, gameplay/weapon | §4 ve Notebook §3 |
| Gemi, enerji, shield, XP | gameConfigs/Ship*, gameplay/progression | §5 ve Notebook §4 |
| Kamera | framework/camera/CameraManager | §6 ve Notebook §5 |
| Arena / sınır | level/Arena* | §7 ve Notebook §6 |
| Level, dalga ve sunum | level, enemy, VFX, widget | §8 |
| Yeni özellik ekleme | Bu dosya, §9 | Notebook §7–§9 |

## 1. Proje haritası

LightYears üç build katmanından oluşur:

| Katman | Sorumluluk | Önemli dizinler |
| --- | --- | --- |
| LightYearsEngine | Yeniden kullanılabilir oyun çatısı: uygulama, dünya, aktör, fizik, kamera, ses, shader, widget | LightYearsEngine/include/framework, LightYearsEngine/src/framework |
| SpaceAbilitySystem | Statik gameplay-system kütüphanesi; aşamalı olarak taşınan generic attribute, ability ve effect çekirdeği | SpaceAbilitySystem/include, SpaceAbilitySystem/src |
| LightYearsGameplayCore | LightYearsGame ile engine arasındaki statik oyun çekirdeği; gameplay contract, combat/ability integration ve ortak game runtime kodu | LightYearsGame gameplay core target ve ilgili include/src dizinleri |
| LightYearsGame | Oyuna özgü gemiler, combat, yetenekler, silahlar, level akışı, UI ve VFX | LightYearsGame/include, LightYearsGame/src |

Ana çalışma akışı:

~~~text
Application
  -> World / GameLevel
    -> Actor BeginPlay / Tick / Render
      -> SpaceShip
        -> CombatRuntime (combat attribute + effect + ability)
          -> PrimaryWeaponExecutionSystem / AbilityActor
            -> ApplyCombatDamage
        -> ShipRuntime (shield + afterburner + ship attribute'ları)
~~~

Önemli ayrım:

- Tanımların şeması ve davranış bağlantıları `LightYearsGame/include/gameConfigs`
  altında C++ tarafında kalır; silah, player ship ve ability gameplay değerleri
  `LightYearsGame/assets/content/data/*.json` dosyalarından yüklenir.
- Generic SAS tipleri `sas` namespace'indedir. Attribute çekirdeği ile Ability
  handle/policy sözleşmeleri SAS'a aittir; oyun-özel attribute ID'leri, ability
  definition/action içeriği, gemi formülleri ve presentation `LightYearsGame`
  içinde kalır.
- Çalışma anı davranışı LightYearsGame/src/gameplay altında çözülür.
- Bir denge değeri değiştirilirken sadece config değil, onu çözen runtime da
  kontrol edilmelidir.

### Temel yaşam döngüsü

- World, aktörleri üretir, BeginPlay çağırır, Tick eder ve yok edilmesi
  işaretlenen aktörleri temizler.
- Actor sahnedeki temel varlıktır; SpaceShip, AbilityWorldActor, projectile,
  boundary indicator ve VFX aktörleri bunun üstüne kurulur.
- GameLevel, HUD, pause/game over, oyuncu ve düşman ödül bağlantılarını
  yönetir.
- ArenaLevel, GameLevel üstüne serbest hareket, kamera girdileri, arena sınırı
  ve respawn sistemini ekler.

### 1.1 Engine diagnostics: assert, logging ve profiling

Engine seviyesindeki ortak tanılama API'si
`LightYearsEngine/include/framework/debug/` altında üç parçadan oluşur:

| Parça | Kullanım | Debug | Release |
| --- | --- | --- | --- |
| Assert | `LY_ASSERT`, `LY_CORE_ASSERT`, `LY_DEBUG_BREAK` | Başarısız koşulu `FATAL` olarak loglar; debugger bağlıysa break, değilse abort üretir | Derleme dışıdır; koşul değerlendirilmez |
| Verify | `LY_VERIFY` | Assert gibi durur | Koşulu her zaman değerlendirir; başarısızlıkta `ERROR` loglayıp devam eder |
| Logging | `LY_CORE_*`, `LY_GAME_*` | TRACE ve üstü | Varsayılan WARNING ve üstü |
| Profiling | `LY_PROFILE_SCOPE`, `LY_PROFILE_FUNCTION`, `LY_PROFILE_FRAME`, `LY_PROFILE_COUNTER` | Süre ve sayaç aggregate'leri toplar | Derleme dışıdır |

Logging satırı saat, `CORE`/`GAME` kanalı, seviye, mesaj ve kaynak
dosya/satırını taşır. `EntryPoint.cpp` logger'ı `LightYears.log` hedefiyle,
profiler'ı da uygulama oluşturulmadan önce başlatır; normal kapanışta ve
yakalanan exception yollarında profiler/logger açıkça kapatılır. Kritik hata
mesajları kapanmadan önce flush edilir. `LY_*_FATAL` yalnız log seviyesidir;
tek başına programı durdurmaz. Programı durdurması gereken invariant için
`LY_ASSERT` kullanılır.

Profiler isim bazında çağrı sayısı, toplam/minimum/maksimum mikrosaniye
değerlerini aggregate eder ve kapanışta en pahalı scope'ların özetini logging
kanalına gönderir. Şu kritik yollar başlangıç kapsamına alınmıştır:

- Application frame/tick/render ve shutdown
- World tick/clean/render
- AbilitySystemComponent ve typed GameplayEffectRuntimeSystem tick/uygulama/damage yolları
- Gravity Anomaly hedef keşfi
- Mevcut actor/bullet/particle sayaçları (`PerfMonitor`) profiler counter'ları

Makrolar `framework/Core.h` üzerinden engine/game koduna açılır. Assert
ifadesinde side effect bulunmamalıdır; Release'te de çalışması gereken kontrol
`LY_VERIFY` veya normal hata yönetimi olmalıdır. Kullanıcı girdisi, dosya veya
network gibi beklenen runtime hataları assert değil, doğrulama ve log ile ele
alınır.

Derleme politikası `LightYearsEngine/CMakeLists.txt` içinde
`LY_ENABLE_LOGGING`, `LY_ENABLE_ASSERTS` ve `LY_ENABLE_PROFILING` tanımlarıyla
merkezidir. Engine lifetime testleri structured log kaydını, seviye filtresini,
`LY_VERIFY` değerlendirmesini, Debug profiler aggregate/counter üretimini ve
Release no-op kontratını kapsar.

## 2. Hasar ve savunma matematiği

İlgili kaynaklar:

- LightYearsGame/src/gameplay/combat/Combatant.cpp
- LightYearsGame/src/gameplay/combat/CombatRuntime.cpp
- LightYearsGame/src/gameplay/damage/DamageTypeSystem.cpp
- LightYearsGame/src/gameplay/effects/LightYearsEffectBehaviorRuntime.cpp
- LightYearsGame/src/gameplay/effects/content/barrier/BarrierEffectBehavior.cpp
- LightYearsGame/src/spaceShip/SpaceShip.cpp
- SpaceAbilitySystem/include/attributes/AttributeMath.h

### 2.1 Hasar çözüm sırası

Bir combat hedefi için sıra aşağıdaki gibidir:

~~~text
Base hit
  -> kaynak kritik zar atışı
  -> incoming effect'ler ve damage type status uygulaması
  -> gemi shield / overshield component'i
  -> kalan hull hasarına zırh ve armor penetration
  -> health
  -> resolved damage olayları
~~~

Başlangıçta DamageContext içinde originalDamage ve remainingDamage, kritik
uygulandıktan sonraki aynı değerdir. Context ayrıca absorbedDamage,
mitigatedDamage, appliedDamage, damage tag’leri ve DamagePayload taşır.

### 2.2 Kritik vuruş

Kritik olasılığı:

~~~text
S(rating, scale) = clamp(1 - exp(-rating / scale), 0, 1)
criticalChance = 1 - (1 - baseCriticalChance) * (1 - S(criticalRating, 100))
~~~

- Mevcut varsayılan baseCriticalChance: 0.
- Zar başarılıysa hasar:

~~~text
criticalDamage = baseDamage * max(1, Owner.CriticalDamage)
~~~

- `DamageCriticalPolicy` tek kritik otoritesidir: `Random` kaynak owner'ın CriticalChance eğrisini kullanır, `Guaranteed` şansı atlayıp kritik uygular, `Disabled` hiç kritik uygulamaz.
- `Owner.CriticalDamage` tek kritik çarpanı otoritesidir. Başlangıç değeri `1.5`tir; `1.5 = 150%` toplam hasar, `2.0 = 200%` toplam hasar demektir. Upgrade/effect modifier'ları bu owner attribute'a uygulanır; payload ayrı bir çarpan taşımaz.
- Kritik ve varsa weapon'a özgü `ceil` rounding Armor'dan önce çözülür; rounding genel ability/DOT kuralı değildir.
- Aynı doygun eğri combat luck için ölçek 100, loot luck için ölçek 300 ile
  kullanılır.

### 2.3 Attribute çözümlenmesi

Attribute çekirdeğinin public yüzeyi `sas` namespace'indedir:
`sas::GameplayAttribute`, `sas::AttributeModifier`,
`sas::AttributeScalingRule`, `sas::AttributeSystem` ve
`sas::AttributeMath`. Header'lar `SpaceAbilitySystem/include/attributes`,
uygulama dosyası `SpaceAbilitySystem/src/attributes` altındadır.
`OwnerAttributeIds`, `ShipAttributeIds` ve `CommonAttributeIds` oyun content
kataloğu oldukları için `LightYearsGame/include/gameplay/attributes/AttributeIds.h`
içinde `ly` namespace'inde kalır.

Ability taşımasının 2A diliminde yalnızca bağımsız temel sözleşmeler SAS
sahipliğine alınmıştır. `sas::AbilityHandle` ile slot, activation, lifetime,
action phase, end reason ve hedef/spawn/yön policy enum'ları
`SpaceAbilitySystem/include/abilities` altındadır. Oyuna özgü
`AbilityDefinition`, variant action payload'ları, weapon bağımlılığı, UI
metadata'sı ve content construction
`LightYearsGame/include/gameplay/ability/content/GameAbilityDefinition.h`
altında kalır. Aktif kod SAS tiplerini açık `sas::` adlarıyla kullanır;
geçiş alias'ları ve karşılaştırma şemaları doğrulama tamamlandıktan sonra
2026-07-30 tarihinde kaldırılmıştır.

Ability 2B dilimi runtime'ın salt-okunur veri sınırını ayırır.
`sas::AbilityRuntimeSnapshot`, bir game-owned `AbilityDefinition*` saklamak
yerine `abilityId` ve `sas::AbilitySlot` değerlerini doğrudan taşır; level,
active/cooldown/duration ve charge alanları da SAS sözleşmesindedir.
`sas::GameplayAbilityInstance::BuildSnapshot()` ile
`sas::AbilityRuntimeSystem::BuildSnapshots()` bu
tipi döndürür. Önceki pointer tabanlı karşılaştırma şekli geçiş doğrulandıktan
sonra kaldırılmıştır.

Ability 2C diliminde definition ve event ortak gövdeleri ayrılmıştır.
`sas::AbilityDefinition`; kimlik, slot/activation/lifetime, cooldown/duration/
charge, owner tag koşulları ve generic attribute/scaling listelerini taşır.
`ly::GameAbilityDefinition` bu tabanı action/trigger/level progression, scrap,
damage/attachment, behavior ID ve UI metadata'sıyla genişletir. ID, negatif
değer, duration ve passive-lifetime kontrolleri
`sas::ValidateAbilityDefinition()` içinde SAS kaynak kodunda çalışır; action,
weapon, behavior ve game content doğrulaması LightYearsGame'de devam eder.

`sas::AbilityEvent`; `GameplayTag`, magnitude ve type-safe opaque
source/target/context referanslarını taşır. Actor ile `DamageContext` doğrudan
SAS tipine dahil edilmez; oyun bağlama katmanı `SetSource`, `SetTarget` ve
`SetContext` ile bunları olaya ekler, ilgili somut tip üzerinden geri alır.
Önceki `ly::GameAbilityEvent` uzantısı kaldırılmıştır.

Ability 2D diliminde mutable fakat oyundan bağımsız runtime state
`sas::AbilityRuntimeState` altında toplanmıştır. Sınıf level clamp/set, held
input ve pressed-edge takibi, activation begin/end, active duration tick,
cooldown clamp/tick, charge tüketim/yenileme ve cooldown reduction işlemlerini
sahiplenir. 2026-07-30 tarihli son sahiplik düzeltmesinde input değerlendirme,
activation/end, cooldown/duration tick, level değişimi ve snapshot üretimi
`sas::GameplayAbilityInstance` tabanında birleştirilmiştir. Oyundaki
`GameAbility` yalnız owner tag kontrolü, somut behavior/action
çağrıları ile weapon/attachment bağlamasını uygular.

Ability 2E diliminde kalan tekrar kullanılabilir runtime mekanikleri grup halinde
SAS'a alınmıştır. `sas::AbilityBehaviorRegistry<Behavior, Key, Hash>` typed
behavior factory depolama/üretim kuralını sahiplenir. Oyun tarafındaki
`RegisterGameAbilityBehaviors()` yalnız shipped somut behavior factory'lerini
kaydeder. `sas::AbilityActionScheduler`,
`RepeatedAbilityActionState` üzerindeki interval ve maximum-execution
ilerlemesini sahiplenir. `sas::AbilityCooldownTracker` event-trigger internal
cooldown anahtarlarının başlatma/tick/expiry akışını yönetir.
`sas::AbilityCollection<sas::GameplayAbilityInstance>` ise handle üretimi, instance sahipliği,
string ID/slot lookup ve passive handle listesini tek tutarlı container altında
toplar. Weapon lifecycle, variant action dispatch,
actor/effect/damage/attachment bağları LightYearsGame'de kalır. Önceki registry,
execution runtime ve system storage karşılaştırmaları kaldırılmıştır.

2026-07-30 sahiplik denetiminde bu sınır yeniden daraltılmıştır.
`sas::AbilityBehavior<Definition, Context>` varsayılan Validate/Activate/Tick/End
kontratını, `sas::AbilityRuntimeEntry<Definition, Execution>` handle/base
definition/resolved definition/runtime state/execution sahipliğini taşır.
`sas::ValidateAbilityGrant`, passive kapasite ve lifetime kabulünü;
`sas::AbilityTrigger` ile trigger eşleştirme/cooldown anahtarı üretimi de
oyundan bağımsız kararları sahiplenir. `ly::GameAbilityBehavior` artık SAS
template'inin game context alias'ıdır. Eski `GameAbilityInstance`,
`GameAbilityBehaviorRegistry`, `GameAbilityEvent` ve `GameAbilityExecution`
dosyaları kaldırılmıştır. Game tarafında kalan `LightYearsAbilitySystemComponent`,
`GameAbility` ve `GameAbilityActionExecutor` Actor, weapon, attachment,
effect ve concrete content çağrılarını bağlayan adaptörlerdir.
Fonksiyon bazlı ikinci denetimde attribute scaling-rule uygulaması,
`HasAttribute`/base-list üretimi ve runtime snapshot construction da
SAS'a alınmıştır; executor içindeki kalan çözümleme kodu attachment, weapon,
game attribute ID veya World/Actor hedefleme bağımlılığı taşır.

Son component yüzeyi denetiminde `LightYearsAbilitySystemComponent` içindeki
`mComponent = this` self-alias'ı, ikinci runtime referansı, null kontrolleri ve
SAS API'sini yalnız yeniden adlandıran grant/remove/level/input/tag/event
wrapper'ları kaldırılmıştır. Ability notification zinciri, toplu cooldown
azaltma ve catalog null/duplicate denetimi `sas::AbilitySystemComponent` ile
SAS validation API'sine alınmıştır. Effect behavior tarafında yalnız
`Actor`/`DamageContext`/incoming-damage phase somutlaştırması game component'ta
kalır; kullanılmayan handler alias'ları ve initialize/refresh callback
tekrarları kaldırılmıştır.

Bir attribute üzerindeki modifier sırası:

~~~text
value = (baseValue + tüm Add modifier toplamı) * tüm Multiply modifier çarpımı
override varsa value = en yüksek öncelikli Override değeri
value = clamp(value, minValue, maxValue)
~~~

Eşit öncelikli Override değerlerinde sistemde son dolaşılan değer kazanır;
aynı attribute için çakışan override üretmekten kaçının.

Ability veya silah scale kuralı için:

~~~text
Add:       value = value + sourceAttribute * coefficient
Multiply:  value = value * (1 + sourceAttribute * coefficient)
Override:  value = sourceAttribute * coefficient
~~~

Çözüm sırası: definition modifier’ları → attachment modifier’ları →
ability scaling rule’ları → weapon scaling rule’ları → attribute min/max
clamp. Bu nedenle aynı statın hem ability hem weapon scale’ında olması
birikimli çalışır.

Seviye ilerlemesi ve scaling kuralı birikimi (Phase 3B.3):
`AbilityLevelStep` ve `PrimaryWeaponLevelStep` generic `scalingRules` (`std::vector<sas::AttributeScalingRule>`) taşır. `GameAbility::RebuildDefinitionForLevel()` seçilen seviyeye kadar tüm level step scaling rule'larını `mDefinition.scalingRules` içerisine biriktirir. Add operasyonları için taban silah katsayısı ile seviye katsayıları toplanır (`sas::ApplyAttributeScalings`).
Ability JSON loader hem `progression.repeat.scalingRules` hem de `progression.levels[].scalingRules` alanlarını aynı ortak attribute JSON parser üzerinden okur. Weapon JSON `progression.rules[].reward.scalingRules` alanını aynı parser'a yönlendirir; iki içerik türü runtime'da `AbilityLevelStep` kontratında birleşir.
Level yeniden kurulduğunda `GameAbility` içindeki monotonik `mConfigurationRevision` artırılır (`GetConfigurationRevision()`). `FireWeaponActionRuntime` resolved attribute cache'inde bu revision'ı denetler; level değiştiğinde resolved stat'lar hemen yenilenir, ancak şarjördeki mermi adedi (`roundsRemaining`), aktif reload (`reloadRemaining`) ve atış kadansı (`successfulFireCount`) sıfırlanmaz.

### 2.4 Zırh

Zırh azaltma formülleri (Phase 3A.2 hyperbolic diminishing-return modeli):

~~~text
SafeArmor = max(0, Armor)
ArmorDamageReduction = SafeArmor / (SafeArmor + 100)
FinalDamageMultiplier = 100 / (SafeArmor + 100)
DamageReduction = 1 - FinalDamageMultiplier
~~~

Runtime negatif Armor için max(0, Armor) kullanır; negatif Armor ilave bonus hasar üretmez (0 damage reduction).

Armor penetration kombinasyonu:

~~~text
effectiveReduction = ArmorDamageReduction * (1 - armorPenetration)
finalDamage = incomingDamage * (1 - effectiveReduction)
~~~

Runtime, çok yüksek finite Armor değerlerinde `1 - reduction` çıkarımının
float hassasiyetiyle sıfıra yuvarlanmasını önlemek için eşdeğer hesabı doğrudan
multiplier üzerinden yapar:

~~~text
finalArmorDamageMultiplier = FinalDamageMultiplier
    + armorPenetration * (1 - FinalDamageMultiplier)
finalDamage = incomingDamage * finalArmorDamageMultiplier
~~~

Referans breakpoint'ler (100 raw incoming damage):
- 50 Armor   ≈ %33.333 DR (final damage ≈ 66.6667)
- 100 Armor  = %50.0 DR (final damage = 50.0)
- 200 Armor  ≈ %66.667 DR (final damage ≈ 33.3333)
- 300 Armor  = %75.0 DR (final damage = 25.0)
- 500 Armor  ≈ %83.333 DR (final damage ≈ 16.6667)
- 1000 Armor ≈ %90.909 DR (final damage ≈ 9.0909)

Hard cap yoktur; sonlu Armor değerlerinde reduction hiçbir zaman %100'e ulaşmaz, eğri asimptotik olarak yaklaşır.

Pipeline sırasındaki yeri:
- Kritik vuruş (Crit) hesabı Armor'dan önce çözülür.
- Armor mitigation, incoming effect'lerden ve geminin kalıcı shield/overshield emiliminden sonra yalnızca kalan hull hasarına uygulanır. Shield-only hit'lerde Armor sonucu değiştirmez; shield'ı aşan kaynak hasarı ortak `CombatRuntime::ApplyHullDamageMitigation` hesabından geçer.
- DamagePayload armor penetration'ı 0 ile 1 aralığında clamp eder. Kinetic status penetration'ı hit başlamadan önceki mevcut stack snapshot'ından eklenir; `Kinetic Bore` kaydındaki `0.10` granted bonus ve Kinetic tag koşulundaki `+0.05` source bonusu buna eklenebilir. Eski genel silah/ability `0.10` tabanları kullanılmaz.

### 2.5 Elektrik, Barrier ve gemi shield’ı

**Electric status**, PreMitigation aşamasındadır:

~~~text
remainingDamage = remainingDamage * (1 + ElectricTakenMultiplier[stack])
~~~

Electric ilk stack’ten itibaren etkilidir. Stack tablosu
[`damage_status_balance.json`](../LightYearsGame/assets/content/data/damage_status_balance.json)
dosyasından typed `DamageStatusBalanceLoader` ve
`DamageStatusBalanceCatalog` hattıyla tek kaynaktan çözülür: `%3`, `%6`, `%9`,
`%16`; tam güç süresi 4 saniyedir. Dört seçili status için uygulama yeniden
geldiğinde stack eklenir ve tam süre yenilenir. Süre dolunca ilk stack düşer,
sonra her saniyede bir stack azalır; sıfırda effect kaldırılır. Bu decay policy
generic `Stack` effect'lerine otomatik uygulanmaz. Policy tanımı JSON'dan gelir;
`GameplayEffectRuntimeState` effect instance'ında tam süre ve decay aralığını
tutar. Stack düşüşü önce `stackChanged`, ardından normal `changed` bildirimiyle
attribute, görsel ve HUD tüketicilerine yayınlanır. Lifecycle sözleşmesi
`SpaceAbilitySystemLifecycleTests` ile izole olarak doğrulanır.

**Barrier effect** kapasite tabanlı geçici kalkan effect’idir:

~~~text
shieldCapacitySpent = min(capacity, incomingDamage * absorptionRatio * shieldDamageMultiplier)
sourceDamageAbsorbed = shieldCapacitySpent / (absorptionRatio * shieldDamageMultiplier)
~~~

Barrier kırılırsa effect kaldırılır ve Event.Owner.BarrierBroken olayı
gönderilir. Basic Barrier: 30 kapasite, 1.0 absorption ratio, 6/sn
rejenerasyon, 1.5 sn temel rejenerasyon gecikmesi, 5 sn effect süresi.

**Gemi shield component’i** incoming effect’lerden sonra ve hull Armor’ından
önce çalışır; aynı shieldDamageMultiplier mantığını kullanır. Energy hasarının
1.50 çarpanı, aynı kaynak hasarı için daha fazla shield kapasitesi harcatır;
kalkan kısmen kırılırsa kalan kaynak hasarı önce hull Armor’ına, sonra health’e
doğru biçimde taşınır. Shield kapasitesi kaybı ile emilen kaynak hasarı
`DamageContext` içinde ayrı anlamlarını korur.

### 2.6 Hasar türleri

`DamageTypeSystem::BuildPayload`, türleri tek bir global öncelik sırasına göre
seçmez; tanınan damage türlerini ve source attribute'larını bağımsız olarak
payload'a işler. `DamageContext.h` güvenli yapısal default'ları sağlar (Energy
dışı shield multiplier 1, armor penetration 0, Kinetic uygulaması 1 stack,
diğer status uygulamaları 0, crit açık ve çarpan 2). `BuildPayload` Energy
tag'i için `1.50` shield multiplier tabanını tek çözüm yolunda kurar; explicit
source değeri daha düşükse tabanı düşüremez, daha yüksek bir source bonusu
korunur. Hibrit veya çoklu tag davranışı source profile ve çağrı yoluyla
belirlenir.

| Tür | Varsayılan davranış | Denge etkisi |
| --- | --- | --- |
| Photonic | Özel payload yok | Nötr başlangıç hasarı |
| Energy | Shield damage x1.50; regen delay source attribute/profile tarafından verilir | Shield karşıtı; hull hasarı 1.50 ile çarpılmaz |
| Kinetic | Hit başına varsayılan 1 stack, özel payload ile 1–4; armor penetration `%6 / %12 / %18 / %30`; 5 sn | Önceden mevcut stack yalnız mevcut Kinetic hull hasarına etki eder; yeni stack sonraki vuruşa kalır |
| Thermal | 1–4 stack; `1 / 2 / 3 / 5` hasar/sn; 5 sn | İlk stack’ten itibaren zamanla hasar |
| Cryo | 1–4 stack; slow `%4 / %8 / %12 / %20`; 5 sn | İlk stack’ten itibaren hareket kırma |
| Electric | 1–4 stack; alınan hasar `+%3 / +%6 / +%9 / +%16`; 4 sn | Hedefi sonraki hasara kontrollü biçimde açık bırakma |

Status effect’leri incoming effect aşamasında, `remainingDamage` pozitifse
uygulanır. Barrier veya başka bir incoming effect hasarı tamamen emerse status
uygulanmaz. Shield emiliminden sonra kalan kaynak hasarı ortak
`CombatRuntime::ApplyHullDamageMitigation` hesabına gider; Armor shield-only
hit'leri değiştirmez. Thermal’in snapshot tabanlı özel periodic Burn modu
(`BurnDamagePerTick`) korunur ve canonical Thermal DPS ile çift uygulanmaz.
Kinetic status uygulaması bu erken aşamada mevcut stack sayısını snapshot'lar;
aynı hit'in eklediği stack o hit'in Armor penetration hesabına girmez.

### 2.7 Hasar örneği

100 hasarlık bir Energy vuruşu, 20 kapasiteli bir Barrier, 50 kapasiteli bir
ship shield ve 100 Armor’a karşı:

~~~text
Barrier kapasite harcaması = min(20, 100 * 1.0 * 1.50) = 20
Barrier’ın emdiği kaynak hasar = 20 / 1.50 = 13.333
Barrier sonrası kaynak hasar = 86.667
Ship shield’ın emdiği kaynak hasar = 50 / 1.50 = 33.333
Hull’a kalan kaynak hasar = 53.333
100 Armor sonrası hull hasarı = 53.333 * 0.50 = 26.667
~~~

Kritik bu zincirin başında uygulanır; `absorbedDamage` Barrier ve ship
shield’ın emdiği kaynak hasarını, `mitigatedDamage` ise Armor azaltımını
taşır.

## 3. Ability, effect ve attachment sistemi

### 3.0 Ability yerleşim kontratı

### 3.0.A Proje geneli gameplay tag sözleşmesi

`LightYearsGame/include/gameplay/tags/GameplayTagSchema.h`, bütün gameplay
sistemlerinin ortak tag dilini tanımlar. Bu bir evrensel leaf-tag registry'si
değildir: yalnız domain köklerini, biçim doğrulamasını ve paylaşılan davranış
taglerini sahiplenir. Ability, weapon, effect, damage ve attachment aileleri
kendi leaf taglerini kendi feature/config klasöründe tanımlamaya devam eder.

| Alan | Zorunlu tag ailesi | Sahibi |
| --- | --- | --- |
| Ability sınıflandırması | `Ability.<Category>` ve `Ability.<Category>.<Family>` | Feature-local contract/config |
| Ability behavior | `GameAbilityBehavior.<Family>` | Feature-local behavior contract |
| Ability lifecycle | `State.Ability.<Family>.<State>` ve `Event.Ability.<Family>.<Event>`; dinlenen dış olaylar `Event.<Producer>.<Event>` | Feature-local behavior contract |
| Ability actor | `AbilityActor.<Family>.<Role>` | Feature-local actor contract |
| Effect runtime state | `State.Effect.<Family>.<State>` | Feature-local effect contract/config |
| Attribute ID | `Common.*`, `Ability.<Category>.<Family>.*`, `<Owner>...` / `<AbilityActor>...` | İlgili sistemin AttributeId kataloğu ve family-local contract |
| Effect behavior | `EffectBehavior.<Family>[.<Behavior>]` | Feature-local effect behavior |
| Primary weapon | `PrimaryWeapon.<Family>.<Type>` / `PrimaryWeapon.Feature.<Feature>` | Weapon handler/feature |
| Damage | `Damage.Type.<Type>` | Damage type schema |
| Attachment capability | `Attachment.Capability.<Capability>` | Attachment schema |

Bu tablo yalnız runtime'da sorgulanan veya grant edilen gameplay taglerini
gösterir. `Ability.<Category>.<Family>.<Variant>` ve
`Attachment.<Family>.<Name>.<Variant>` biçimindeki somut kayıt kimlikleri
`GameplayTag` değil, `ContentIdSchema` tarafından doğrulanan string content
ID'leridir.

Numeric gameplay değerlerinin kimliği `sas::AttributeId`'dir. AttributeId
string-backed ve opaque bir SAS tipidir; `GameplayTag` hiyerarşisi, tag
conversion'ı veya `MatchesTag()` semantiği taşımaz. `GameplayTag` yalnızca
semantic gameplay bilgisi için kullanılır. AttributeId namespace ve contract
doğrulaması LightYearsGame katmanına aittir; SAS generic AttributeId'nin game
namespace'lerini bilmez.

Canonical AttributeId adı `Owner.*`, `Ship.*`, `Common.*`, `Damage.*`,
`Effect.*`, `PrimaryWeapon.*`, `AbilityActor.*` veya feature tarafından
sahiplenilen eşdeğer bir namespace ile başlar; eski `Attribute.*` prefix'i
JSON loader'larında ve `GameplayTagSchema` içinde reddedilir. `AttributeIdSchema`
yalnızca lexical biçim ve namespace sınırı yardımcılarını sağlar, kayıtlı tüm
attribute'ları listelemez. Hangi ID'lerin geçerli olduğu ilgili game catalog,
weapon handler veya ability actor contract'ında kalır. Lookup API'leri de bu
ayrımdan sonra `FindAttribute`, `FindAttributeValue` ve `HasAttribute` olarak
adlandırılmıştır; davranış ve modifier/scaling sırası değişmemiştir.

Non-primary bir ability'nin özel numeric değerleri, content ID'sinden türetilen
exact family namespace'inde tutulur. Örneğin
`Ability.Movement.PhaseDrift.Basic` yalnız `Common.*` ve
`Ability.Movement.PhaseDrift.*` base attribute'larını taşıyabilir;
`Ability.Offense.OverdriveCore.*` veya benzer-prefix
`Ability.Movement.PhaseDriftExtended.*` değerleri reddedilir.
`GameAbilityDefinitionValidator` aynı sahiplik kontrolünü definition
modifier'larına, scaling target'larına ve level-progression modifier'larına da
uygular. `Ability.*` hedefi ayrıca ability'nin base attribute listesinde declare
edilmiş olmalıdır. Scaling source'ları (`Owner.*`, `Ship.*`) ile consumer-owned
`Effect.*` ve `AbilityActor.*` hedefleri bu family kuralına tabi değildir; kendi
consumer kontratlarında doğrulanır. PrimaryFire tanımlarının kimliği `Weapon.*`
olduğu için ability family namespace'i yoktur ve `Ability.*` değer/hedef taşıması
yasaktır; `Common.*` kullanımı geçerliliğini korur.

`State.ActionLock.AbilityActivation`,
`State.ActionLock.PrimaryWeaponFire`,
`State.ActionLock.MovementInput` ve
`State.ActionLock.ExternalMovement` kayıtlı ortak kilitlerdir. Bir mekanik bu
taglerden gerekli olanını owner'a geçici olarak verir; `GameAbility::CanActivateContent`
ilk tag ile normal ability aktivasyonlarını, ikinci tag ile PrimaryFire
aktivasyonunu engeller. `MovementInput` yalnız oyuncu tarafından verilen hareket
komutunu engeller; `ExternalMovement` ise push, pull ve benzeri dünya kuvvetlerini
de engelleyen nadir hard-stasis kilididir.

Odaklanma (focus) bir feature'ın yeniden tanımlayacağı serbest bir tag paketi
değildir. `ability::ApplyFocusActionLocks()` her odaklanma penceresinde
AbilityActivation + PrimaryWeaponFire + MovementInput kilitlerini birlikte
uygular. Böylece gemi nişan yönünü değiştirebilir ve dış kuvvetlerle sürüklenebilir,
ama hareket input'u veremez, ateş edemez veya başka ability başlatamaz. Focus
ability boyunca sürerse yalnız `RemoveFocusActionLocks()` ile temizlenir; bir
devam actor'ına devredilirse aynı lock referansları actor tarafından bir kez
temizlenir. Overdrive gibi bir ability kendi local firing state'ini UI ve
lifecycle için ayrıca kullanabilir; global input/activation engeli için yeni
feature-özel block tag üretilmez.

Bir gameplay tag ancak domain'i, producer'ı ve consumer'ı açıksa eklenir.
Parent tag doğrudan grant edilmez; parent sorgusu yalnız bilinçli grup sorgusu
olarak kullanılır. Yeni content doğrulaması ability, effect, ability actor,
primary weapon, attachment JSON loader'ı ve ship progression giriş noktalarında
bu şemayı çağırır.

Her ability ailesinin `gameplay/ability/<family>/<Family>Contracts.h` dosyası
feature-local ID, setting, direction/payload ve state/event/actor parçalarını
sahiplenir. Ability dispatch selector'ı ayrı `AbilityBehaviorType`/registry
sınırındadır; bütün aile contract'larının aynı zorunlu alan kümesini taşıdığı
varsayılmaz. Actor role'ü altında onun definition ID'si, type tag'i ve
attribute ID'leri birlikte kalır.
Bir effect ID'si veya dış event, onu üreten effect/owner sisteminin contract'ında
tanımlanır; ability yalnız onu tüketir. Bu nedenle boş `State`/`Event`/`Actor`
struct'ları açılmaz ve feature'lar shared action-lock tag'i yeniden tanımlamaz.

`State` ve `Event` alanları yalnızca ilgili ability davranışı veya onun
data-driven action/trigger akışı state tag'ini gerçekten grant/remove ediyor,
event yayıyor ya da bu event'i contract üzerinden tüketiyorsa eklenir. Instant
ability olması tek başına State/Event gerektirmez; aynı şekilde duration ability
olması da otomatik olarak State/Event gerektirmez. State veya event sahipliği
başka bir effect/combat sistemindeyse ability contract'ına kopyalanmaz.

`Setting` ve `Setting::Contract` yalnızca behavior'ın JSON `settings` nesnesinden
okuduğu özel anahtarlar için eklenir. Standart ability alanları için setting
anahtarı oluşturulmaz ve boş contract kaydedilmez. Mevcut shipped ability
verisinde bu durum Dash ve InfernoSpray olmak üzere 8 ability'den 2'si için
geçerlidir.

Ability'ye özel effect ID, behavior tag'i ve effect state tag'i yalnızca o
ability ailesinin effect lifecycle'ına aitse ability contract'ındaki `Effect`
alanında tutulur. Birden fazla kaynak tarafından kullanılan veya bağımsız
effect behavior/lifecycle'ı olan effect'ler `gameConfigs/combat/` altındaki
effect schema'sına aittir.

Not: Üretilmiş `PrimaryFire` ability tanımı, `Ability.Primary` / `Ability.Offense`
sınıflandırmalarına ek olarak somut `PrimaryWeapon.<Family>.<Type>` tagini aynı
`abilityTags` listesinde taşır. Bu, weapon tipini ayrı bir runtime alanına
kopyalamadan consumer'ların filtrelemesine izin veren tek istisnadır.

Kimlik ve contract kuralları:

- Kalıcı content kayıtları `inline constexpr char ...Id[]` ile tanımlanır ve
  loader tarafından doğrulanır: `Ability.<Category>.<Family>.<Variant>`,
  `Effect.<Family>.<Variant>`, `Actor.Ability.<Family>.<Role>.<Variant>`,
  `AttributeProfile.<Family>.<Role>.<Variant>`,
  `Presentation.Ability.<Family>.<Role>.<Variant>`,
  `Weapon.<Family>.<Name>.<Variant>`, `Ship.<Faction>.<Name>.<Variant>`,
  `Attachment.<Family>.<Name>.<Variant>` ve
  `Visual.Effect.<Family>.<VariantPath>`.
- Sorgulanan veya grant edilen semantic değerler `GameplayTag` olur. Numeric
  gameplay değerlerinin kimliği `sas::AttributeId` olur. Struct alanları ve rol
  bildiren semantic sabitler `...Tag` ile biter: `FamilyTag`, `TypeTag`,
  `FeatureTag`. Ability behavior dispatch'i `AbilityBehaviorType` enum'udur;
  effect `behaviorKey` yalnız `GameplayEffectDefinition` sınırında kullanılır.
  `DamageTypeSchema::Thermal` gibi türü
  enclosing schema tarafından açık olan leaf sabitler kısa kalabilir.
- `...Id` daima catalog/registry kaydı olan string kimliktir. Attachment kayıt
  kimliği content ID'dir; yalnız `Attachment.Capability.*` değerleri tagdir.
- `Effect.<Family>.<Variant>` yalnız content ID'dir; aktif effect varlığını
  taşıyan semantic tagler `State.Effect.<Family>.<State>` altında kalır.
- Ability family contract'ları yalnız gerçekten kullandıkları ID, setting,
  payload, state/event/actor ve effect alanlarını taşır. Dispatch selector'ları
  registry/enum sınırında kalır; JSON loader family'yi bilmez, kayıtlı behavior
  registry üzerinden onu çözer. JSON numeric-setting contract'ı
  `Setting::Contract` içinde kalır.
- Aileye özel actor attribute'ları daima
  `AbilityActor.<Family>.<Role>.<Name>` biçimindeki AttributeId adlarıdır ve ilgili
  `Actor::<Role>` contract'ında tanımlanır. `CommonAttributeIds` değerleri
  contract'ta yeniden adlandırılmaz. Bir actor başka bir role ait değerleri
  yalnızca onları üretilecek actor'a iletmek için tüketiyorsa (Gravity Anomaly
  projectile -> field gibi), handler iki role ait dar kökleri açıkça bildirir;
  aile kökü tek başına yetki vermez.
- Somut shipped ability'nin ID family segmenti behavior contract'ı ile
  uyumlu olmak zorundadır; yalnız generic `GameAbilityBehavior.Configured` ile
  tanımlanan content-only test/prototype ability'ler bu eşleşmeden muaftır.
  Owner activation koşulları yalnızca kalıcı effect/status/ability/
  ability-state/action-lock domainlerinden seçilir; `Event.*` geçici olduğu için
  koşul olarak kullanılamaz.
- Weapon content ID family segmenti `PrimaryWeapon.<Family>.<Type>` family
  segmentiyle eşleşir. Ability actor definition, actor type ve presentation
  profile aynı `<Family>.<Role>` çiftini kullanır.
- Runtime'da geçici/test amaçlı oluşturulan weapon definition boş `weaponId`
  kullanabilir. Shipped weapon catalog kayıtlarında ID zorunludur ve loader
  tarafından doğrulanır.

Content sahiplik matrisi:

| Content | C++ sahipliği | JSON sahipliği |
| --- | --- | --- |
| Ability | Behavior, structural actions, tag/actor contract ve fallback skeleton | Cooldown/duration/charge, progression, scaling, effect spec, actor/attribute değerleri ve numeric settings |
| Effect | Typed behavior/presentation skeleton | Policy, stacking, granted/application tagleri ve source-owned olmayan numeric değerler |
| Damage status balance | `DamageStatusBalance` tipi, typed loader/catalog ve semantic validation | `damage_status_balance.json` içindeki Energy, Cryo, Electric, Thermal ve Kinetic sayısal değerleri |
| Weapon | Handler/feature type contractları | Tam weapon definition, presentation, progression ve balance |
| Attachment | Capability/event/condition yorumlama kodu | Tam attachment definition ve balance |
| Ship | Runtime struct ve presentation base | Kimlik, gameplay değerleri, primary weapon referansı ve progression |
| Ability presentation | Concrete typed profile yapısı ve registration | Şimdilik JSON sahibi değildir; profile değerleri feature-local C++ content'tir |

Yeni shipped ability dikey dilim olarak eklenir. Applicable kayıt noktaları:

1. Family contract ve C++ structural fallback.
2. `abilities.json` kaydı.
3. Behavior composition kaydı.
4. Actor handler ve typed presentation profile kaydı gerekiyorsa bunların ikisi.
5. Numeric `Setting::Contract` varsa settings composition kaydı.
6. Built-in catalog, CMake source listesi ve shipped validation testi.

Taslak ability yalnız contract ve schema testinden oluşabilir. Boş config,
kaydedilmeyen presentation ID veya yarım actor/profile yüzeyi açılmaz. Null
Pulse dikey dilimi bu kuralın tam uygulamasıdır: contract, JSON, ayrı behavior,
yeniden kullanılabilir projectile sınıflandırması, kontrol effect'leri, typed
presentation profile ve runtime testleri birlikte kaydedilmiştir.

Dosyalar yalnız satır sayısı nedeniyle bölünmez. Ayrıştırma için en az iki ayrı
değişim nedeni veya başka consumer tarafından yeniden kullanılan bağımsız bir
sorumluluk gerekir. Loader'a özel saf JSON parse/materialization yardımcıları
private/internal kalır; public registry, service veya feature ancak bağımsız bir
runtime kontratı varsa açılır.

Kod tarafındaki tek terim **ability**'dir; aynı kavram için `skill` adlı paralel
bir klasör veya runtime katmanı açılmaz.

```text
gameplay/ability/
|-- LightYearsAbilitySystemComponent / GameAbility / GameAbilityActionExecutor
|-- actors/
|   |-- AbilityWorldActor
|   |-- AbilityActorRegistry
|   `-- AreaTelegraphActor
|-- dash/
|   |-- DashAbility
|   |-- DashMovementController
|   `-- DashMovementMath
|-- gravityAnomaly/
|   |-- GravityAnomalyAbility
|   |-- GravityAnomalyProjectileActor
|   `-- GravityAnomalyFieldActor
|-- nullPulse/
|   |-- NullPulseAbility
|   |-- NullPulseTargetQuery
|   `-- NullPulseVisualActor
|-- rocket/
|   |-- RocketAbility
|   `-- RocketProjectileActor
|-- shield/
|   `-- ShieldAbility
`-- sunBeam/
    |-- SunBeamAbility
    |-- SunBeamActorBase / SunBeamStrikeActor
    `-- SunBeamVisual
```

Yerleştirme kuralları:

- Ability kökü yalnızca ability'lerden bağımsız lifecycle, registry, execution
  ve event altyapısını içerir.
- `actors/`, ability kimliğinden bağımsız ve yeniden kullanılabilir world-actor
  altyapısı içindir. Buradaki sınıf somut ability ID'si, config'i veya hasar
  kuralını bilmez. `AreaTelegraphActor` ortak telegraph yaşam döngüsüdür;
  SunBeam gibi tek bir aileye ait somut actor burada tutulmaz.
- Her shipped ability kendi behavior sınıfına ve kendi aile klasörüne sahiptir.
  O ability'ye özgü config, runtime yardımcıları, actor ve visual sınıfları aynı
  vertical slice içinde kalır.
- `gameConfigs/ability/<category>/<Ability>Config.h` dosyaları shipped sayısal
  tuning kaynağı değildir. Bu dosyalarda yalnız ID/tag, behavior/action, actor
  type, presentation ve attribute/schema kontratı tutulur; cooldown, damage,
  duration, range, progression ve effect spec sayıları JSON'dan gelir.
- Her görsel actor olmak zorunda değildir. Bağımsız world konumu, tick,
  collision veya lifetime gerektiren nesne actor olur; salt çizim/sunum
  feature-local visual/helper olarak kalır.
- Bir Dash evolve'u bitişte alan telegraph'ı ve hasar üretiyorsa ayrı behavior
  sınıfı ile hasar/çarpışma actor'u `dash/` içinde yaşar; generic
  `AreaTelegraphActor` altyapısını kullanabilir. Yalnız ability kimliğinden
  bağımsız parçalar `actors/` altına taşınır.
- Yeni ability eklemek core'da `if/switch` açmayı gerektirmez. Yeni behavior
  `AbilityBehaviorRegistry`'ye kaydedilir; mevcut C++ config'i
  `gameConfigs/ability/<category>/<Ability>Config.h` içinde tutulur ve
  `AbilityCatalog.h` shipped kataloğa ekler.

### 3.0.0 Content kaynak sınırı ve planlanan harici veri geçişi

Mevcut durum: weapon, player ship, ability, gameplay effect policy/contract ve
damage status balance değerleri
`LightYearsGame/assets/content/data/*.json` dosyalarından runtime'da yüklenir.
Şemalar, validation, behavior/action tanımları ve typed presentation profilleri
C++ tarafında kalır. `attachments.json` iki kayıt içerir; `GameAbility` içinde
attachment equip, modifier merge/condition ve event yolları vardır. Ancak
`GameContentBootstrap` attachments JSON yüklemiyor ve acquisition UI bu
denetimde doğrulanmadı; attachment'ı tam shipped content akışı olarak
belgelemeyin.

Hedef hibrit sınır aşağıdaki gibidir:

| Katman | Sahiplik |
| --- | --- |
| Harici content (aktif JSON) | Silah, player ship ve ability kaynaklarına ait denge değerleri; gameplay effect policy/contract kayıtları; damage status balance değerleri; ID/reference alanları |
| CSV | Yalnız editör/balance import-export ve analiz; runtime'ın otoriter kaynağı değil |
| C++ | Şemalar, parse/semantic validation, gameplay tag eşlemesi, behavior ve weapon handler'ları, action türleri, typed presentation profile türleri/registration ve tüm mutable runtime state |

İlk geçişte yalnız value/reference ağırlıklı content taşınır. Mevcut JSON kaydı
behavior veya presentation seçebilir; fakat bu ID'lerin C++ registry'de
kayıtlı/uyumlu olması validation ile zorunlu kılınır. `AbilityActorDefinition`
ve §3.0.1'deki typed presentation sözleşmesi değişmez. Böylece ability,
attachment/evolve, gemi ve düşman sayıları artarken denge verisi dışarı alınır;
oyun davranışı ile visual type güvenliği C++ tarafında kalır.

#### Ability `baseId` ve evolve/variant kuralı

Bir ability varyantı `baseId` ile başka bir JSON ability kaydından türeyebilir.
Object alanları recursive olarak birleştirilir; varyantta yazılan scalar değerler
ve object içindeki alanlar base değerin üzerine çıkar, array alanları ise
varyant tarafından tamamen değiştirilir. Böylece yalnız değişen cooldown,
setting veya progression değerini yazmak yeterlidir. Kalıtım zinciri döngüye
giremez ve eksik `baseId` reddedilir.

Value-only varyant actor listesini tekrar üretmez; base ability'nin C++ behavior
ve actor/presentation bağlantılarını kullanır. Yapısal olarak yeni actor veya
presentation gerektiren evolve, aynı ability ailesinde ayrı C++ actor/profile/
handler ve benzersiz actor ID'leriyle tanımlanmalıdır.

#### Sayısal veri sahipliği

Bir sayısal değeri, onu üreten gameplay kaynağı sahiplenir. Weapon hit/status
değerleri `weapons.json` içindeki `Damage.*` alanlarında; ability'nin
uyguladığı effect değerleri `abilities.json` içindeki `effectSpecs` alanında;
ability actor alan değerleri actor kaydında veya ability-local
`attributeProfiles` içinde tutulur. `effects.json` çoğu source-parameterized
kayıt için magnitude, duration, stack limiti veya runtime attribute base value
tutmaz; yalnız effect ID, behavior, duration/stacking politikası, tag, visual ve
source-scope sözleşmesini taşır. İstisna olarak dört
`sourceParameterized=false` effect kaydı policy ile uyumlu sabit duration
içerir: `Effect.Immunity.Movement.Slow` (5 sn) ve
`Effect.PhaseDrift.MovementBoost`, `Effect.PhaseDrift.ShieldRecovery`,
`Effect.PhaseDrift.AfterburnerRecovery` (6 sn). Source-parameterized kayıtlar
sayısal alanları reddeder; bu nedenle effects JSON için “hiç numeric içermez”
ifadesi yanlıştır.

`sourceParameterized: true` olan bir effect kaydına `duration`, `maxStacks`,
`modifiers` veya `attributes` eklenmesi loader tarafından reddedilir. Damage tag
yalnız davranış kimliğidir; gizli sayısal varsayılan üretmez. C++ config
tanımları yalnız eşleşen JSON kaydının typed behavior/action/actor/presentation
iskeletini ve loader/test uyumluluğunu sağlar; loader sayısal alanları bu
tanımlardan runtime'a taşımaz. Shipped runtime için sayısal değerlerin tek
kaynağı JSON'dır.

Startup sırasında weapon, ship, ability ve effect catalog'larının herhangi biri
yüklenemezse `GameContentBootstrap::Register()` başarısız olur ve `GameApplication` oyun
dünyasını yüklemeden `QuitApplication()` ile kapanır. Effect catalog, loader'ın
tip/behavior çözümlemek için kullandığı C++ fallback kayıtlarını JSON'da eksik
olan yeni kayıtlar olarak eklemez; JSON'da bulunmayan effect runtime'da yoktur.

### 3.0.1 Ability presentation profile kontratı

Bu bölüm ability presentation mimarisinin bağlayıcı kaydıdır. Yeni ability ve
evolve geliştirmeleri bu kontrata uymalıdır.

Özet kurallar:

- `AbilityActorDefinition` yalnızca tek bir `presentationProfileId` taşır.
- Visual, telegraph, explosion, trail veya evolve için generic actor struct'a
  ayrı ID/alan eklenmez.
- Her ability ailesi presentation ID, definition, typed profile ve shipped
  registration içeriğini `presentation/ability/<family>/` altında sahiplenir.
- Profil çözümü `PresentationProfileRegistry<ConcreteProfile>` ile yapılır;
  global `VisualConfig.h`, global `AbilityVisualStructs.h`, `std::any` veya
  catch-all visual variant kullanılmaz.
- Yalnız gerçekten paylaşılan primitive'ler
  `presentation/ability/common/` altına alınır.
- Aynı bileşenleri kullanan evolve yeni değer profili kaydeder. Yapısal olarak
  farklı evolve, base profile'a optional alanlar/flag'ler yığmak yerine aynı
  ability ailesinde ayrı feature-local profile ve actor/handler alır.
- Shipped presentation content, actor validation ve spawn'dan önce
  `RegisterGameAbilityPresentationContent()` üzerinden kaydedilir.

Gravity Anomaly, aynı aile altında ayrı
`GravityAnomalyProjectilePresentationProfile` ve
`GravityAnomalyFieldPresentationProfile` kayıtları kullanır. Bu iki profil
`RegisterGameAbilityPresentationContent()` içinde actor validation/spawn'dan
önce kaydedilir. Field world presentation'ı ile hedefe bağlı
`GravityAnomalyEffectVisual` ayrı registry/content yoludur.

### 3.1 AbilityDefinition

Temel şema:

| Alan | Anlamı |
| --- | --- |
| slot | PrimaryFire veya Ability1–Ability4 |
| activationPolicy | OnPressed, WhileHeld, Toggle, Passive, GameplayEvent |
| lifetimePolicy | Instant, Duration, WhileInputHeld, UntilCancelled |
| cooldown / duration / maxCharges | Yaşam ve kaynak zamanlaması |
| behaviorType | Ability behavior registry dispatch enum'u; semantic gameplay tag değildir |
| actions | Effect uygulama, actor spawn, weapon fire, impulse, event yayma |
| triggers | Event tabanlı, cooldown/required/blocked tag filtreli eylemler |
| levelProgression | Level 2’den başlayarak eklenen modifier, upgrade, action, trigger |
| scalingRules | Sahip attribute’undan bu ability attribute’una scale |
| damageTags | DamagePayload kimliği |
| attachmentCapabilities / slotCapacity | Uyumlu attachment seti |

`sas::GameplayAbilityInstance` level yükseltirken base definition’ı yeniden kurar ve Level 2
ile mevcut level arasındaki her progression adımını ekler. Dolayısıyla
progression modifier’ları birikimlidir; bir level step’i “toplam değer” değil
“o levelde eklenecek fark” olarak yazılmalıdır.

#### Runtime sahipliği ve veri sınırı

```text
SpaceShip
|-- HealthComponent
|-- ShipRuntime (movement, energy, kalıcı shield ve ship attribute’ları)
`-- CombatRuntime
    |-- sas::AttributeSystem
    |-- sas::AbilitySystemComponent
    `-- sas::GameplayEffectRuntimeSystem<GameplayEffectSpec, ActiveGameplayEffect>
```

`CombatRuntime`, owner-local combat attribute’larını, tag’lerini, effect’lerini
ve ability’lerini sahiplenir; HUD widget’ları ve visual actor’lar gameplay
state sahibi değildir. Shipped sayısal tanımlar
`LightYearsGame/assets/content/data/*.json` içinden materialize edilir; C++
catalog'ları structural/fallback contract, behavior ve typed presentation
sahipliğini korur. Mutable state ise
`sas::GameplayAbilityInstance`, `sas::AbilityExecution`,
`ActiveGameplayEffect`, primary weapon runtime state’i veya spawn edilmiş world
actor’da kalır.

`GameAbilityActionExecutor` yalnızca yeniden kullanılabilir action türlerini
çalıştırır: `ApplyEffectAction`, `SpawnActorAction`, `FireWeaponAction`,
`ApplyImpulseAction` ve `EmitGameplayEventAction`. Silah ailesine özgü hedef
seçimi, projectile/beam delivery veya chain davranışı weapon handler’larında
kalmalıdır. Ability'ye özgü validation, activation, tick ve cleanup ise SAS
generic behavior kontratını genişleten ilgili oyun behavior sınıfında kalır;
generic core somut Dash, Shield veya SunBeam sınıfını include etmez.

`FireWeaponAction`ın begin/tick/end lifecycle'ı
`gameplay/ability/actions/FireWeaponActionRuntime` tarafından yürütülür.
`GameAbilityActionExecutor` yalnız action phase dispatch ve genel scheduler
sahibidir; shared scaling/attachment değer çözümü ise
`AbilityActionAttributeResolver` içinde tutulur.

### 3.2 Mevcut shipped ability content

Bu alt bölüm shipped ability content kayıtlarını ve base contract değerlerini
açıklar. Runtime başlangıç slotları ayrı bir sözleşmedir ve
[`DefaultAbilityLoadout.cpp`](../LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp)
tarafından sahiplenilir; aşağıdaki content kayıtları güncel slot eşlemesi olarak
yorumlanmamalıdır.

| Ability ID | Cooldown | Duration | Max charges | Progression |
| --- | ---: | ---: | ---: | --- |
| `Ability.Defense.ShieldGraft.Basic` | 12.0 s | 0.0 s | 1 | 14 repeat step; conversion +0.01, cooldown -0.20 |
| `Ability.Utility.RelayPrism.Basic` | 10.0 s | 4.0 s | 1 | 14 level step; transfer +0.01, cooldown -0.20 |
| `Ability.Offense.GlacialPressure.Basic` | 12.0 s | 1.0 s | 1 | 14 repeat step; initial damage +1, collision +4, cooldown -0.25 |
| `Ability.Defense.IroncladProtocol.Basic` | 22.0 s | 10.0 s | 1 | 14 repeat step; minigun damage +2, cooldown -0.35 |

Bu tablo yalnız JSON base/progression kaydını gösterir; resolved owner scaling,
presentation ve uçtan uca oynanış kanıtı değildir. 55 kaydın tam ID envanteri
ve kapsam sınırı [Current Implementation Catalog](CURRENT_IMPLEMENTATION_CATALOG.md)
içindedir. Ability definition'ında `behaviorType` dar C++ registry dispatch
enum'udur; effect definition'ında `behaviorKey` kullanılır. Bunlar semantic
gameplay tag değildir; content ID ve runtime semantic tag ayrımı
[IDENTITY_AND_CONTRACT_RULES](IDENTITY_AND_CONTRACT_RULES.md)
ile korunur.

SunBeam strike zamanları: telegraph 0.5 sn, arrival 0.2 sn, impact delay
0.05 sn, impact visual 0.22 sn. Bunlar `SunBeamConfig.h` içindeki
`ActorStrikeBasic` tanımındadır.

### 3.3 Attachment kataloğu

Attachment host/capability koşullarını geçtikten sonra ability veya primary
weapon üzerinde çözülür. Koşullu modifier’lar damage tag veya attribute
durumuna göre uygulanır.

| Attachment | Hedef | Temel etki | Koşullu / olay etkisi |
| --- | --- | --- | --- |
| Thermal Converter | Ability, PrimaryWeapon | JSON'da kayıtlı; Thermal capability yolu | Equip/modifier/event runtime yolu mevcut; acquisition ve bootstrap JSON akışı doğrulanmadı |
| Kinetic Bore | Ability, PrimaryWeapon | `Damage.ArmorPenetration` +0.10; Kinetic damage tag koşulunda +0.05 | Equip/modifier runtime yolu mevcut; acquisition ve bootstrap JSON akışı doğrulanmadı |

Bu denetimde `attachments.json`, `Attachment.Thermal.Converter.Basic` ve
`Attachment.Kinetic.Bore.Basic` kayıtlarını içerir. Kinetic Bore'un JSON kaydı
`Damage.ArmorPenetration` için `+0.10` granted değer ve Kinetic damage tag
koşulunda `+0.05` modifier taşır. `GameContentBootstrap` attachments JSON
yüklemiyor ve acquisition UI doğrulanmadı; kayıt varlığı tam shipped attachment
veya otomatik drop akışını tek başına kanıtlamaz. `GameAbility::TryEquipAttachment`
ve attachment modifier/event yolları runtime altyapısını gösterir.

### 3.4 Effect, event, UI ve cleanup kontratı

Effect verisi üç ayrı yaşam katmanına sahiptir:

```text
GameplayEffectDefinition (immutable shipped content)
    -> GameplayEffectSpec (kaynağa göre çözümlenmiş uygulama payload'u)
        -> ActiveGameplayEffect (hedefe ait mutable runtime state)
```

`GameplayEffectDefinition`; ID, duration/stack politikası, tag, behavior,
visual ve source-scope sözleşmesini taşır. Source-parameterized shipped
effect'lerde sayısal denge değeri taşımaz; dört `sourceParameterized=false`
kayıt policy ile uyumlu sabit duration taşır (Movement Slow 5 sn, Phase Drift
üç boost/recovery effect'i 6 sn). Silah, ability, enemy, reward veya
alan üreticisi bu tanımı değiştirmez; `GameplayEffectSpec` kopyası üretip
duration, stack limiti, modifier, runtime attribute ve source-upgrade
değerlerini kendi JSON kaynağından çözer. `ActiveGameplayEffect` ise handle,
remaining duration, stack count, uygulanmış modifier handle'ları, runtime
attribute'lar, visual ve typed runtime context'i sahiplenir. Böylece aynı Cryo
veya hareket hızı effect tanımı farklı kaynak magnitudelarıyla eşzamanlı
kullanıldığında global content mutasyona uğramaz.

Effect 3A taşımasından sonra bu üç katmanın oyundan bağımsız gövdeleri
`SpaceAbilitySystem/include/effects` altındadır. `sas::GameplayEffectHandle`,
duration/stack policy enum'ları, `sas::GameplayEffectDefinition`,
`sas::GameplayEffectSpec`, structural validation,
`sas::GameplayEffectRuntimeSnapshot` ve `sas::GameplayEffectRuntimeState`
kütüphane sınırını oluşturur. Runtime state duration refresh/tick, stack
limit/artışı, modifier handle listesi ve runtime attribute reset/snapshot
işlemlerini sahiplenir. Oyun `GameplayEffectSpec` tipi source ability upgrade
tag'leriyle SAS spec'ini; `ActiveGameplayEffect` ise Actor/source scope, typed
runtime context ve visual reference ile SAS state'ini genişletir. Behavior
registration, DamageContext dispatch, visual registry ve shipped effect content
LightYearsGame'de kalır. Önceki definition/spec/active state karşılaştırmaları
kaldırılmıştır.

Effect 3B diliminde active-effect depolaması da generic hale getirilmiştir.
`sas::GameplayEffectCollection<ActiveEffect>` value storage, handle allocation,
handle lookup, index erase ve reset işlemlerini sahiplenir.
`sas::GameplayEffectRuntimeSystem<Spec, ActiveEffect>` bu collection'ı compose
eder; `sas::AbilitySystemComponent` facade'ı generic runtime'ı oyuna açar.
Modifier/granted-tag bağlama ve cleanup, stacking/source-scope eşleşmesi,
behavior damage dispatch'i ve visual lifecycle callback'leri bu typed runtime
üzerinden yürür. Önceki ayrı
`List<ActiveGameplayEffect>` ve handle sayacı karşılaştırması kaldırılmıştır.

Effect 3C diliminde generic kayıt ve attribute/tag binding mekanikleri ayrılır.
`sas::GameplayEffectBehaviorRegistry<Hooks>` tag ile typed hook değeri arasındaki
duplicate-safe depolama, lookup ve registration sorgusunu sağlar; callback
imzalarını tanımlayan `GameplayEffectBehavior::Hooks` oyun katmanındadır.
`GameplayEffectBindings` required/blocked application tag gate'ini, Instant
effect base modifier uygulamasını, active modifier handle ekleme/sökme ve
granted-tag ekleme/sökme işlemlerini yürütür. `GameplayEffectRuntimeSystem` refresh,
stack ve removal sırasını; behavior callback, pending DamageContext event ve
visual lifecycle orkestrasyonunu korur.

`GameplayEffectRuntimeSystem`; instant, duration ve infinite yaşam politikalarını;
no-stack, refresh-duration, stack ve opt-in stack-decay davranışlarını; modifier/tag ekleme ve
temizliğini; incoming damage hook dispatch'ini ve effect visual yaşam döngüsünü
sahiplenir. Ignite, Electric, Barrier veya Gravity Anomaly ID'lerine branch
etmez. Add-stack, tick ve incoming-damage davranışları
`GameplayEffectBehavior::Hooks` ile kaydedilir. Shipped katalog
`EffectData::GetShippedGameplayEffectDefinitions()` üzerinden bulunur;
`GameContentBootstrap::Register()` behavior ve visual içeriklerini kaydettikten
sonra ID, süre, stack, attribute/modifier ve kayıt referanslarını doğrular.
Ability `ApplyEffectAction` da bilinmeyen veya geçersiz effect ID'si içerirse
grant/catalog doğrulamasında reddedilir.

Bir effect tanımı `sourceScopedApplication` isterse uygulama context'i source
actor, source scope ve typed runtime context taşır. Refresh/stack eşleşmesi bu
scope içinde yapılır; böylece iki Gravity Anomaly field'i aynı hedefte bağımsız
effect handle'ları korur. Ability ailesine özgü per-tick davranış, generic core'a
ID branch eklemek yerine `GameplayEffectBehavior` tick-handler kaydıyla ve
feature-local typed context ile sağlanır. `MovementSlow`, `MovementComponent`
içindeki gerçek displacement çarpanıdır; Cryo ve Gravity Anomaly aynı mekanizmayı
kullanır.

Alan actor'ları şekil ve hedef keşfini kendi feature sınırında tutar;
`AreaGameplayEffectApplicator` hedef başına handle/context takibi, içerideyken
duration refresh, alandan çıkışta grace süresinin devamı ve source-scope
izolasyonunu ortaklaştırır. Gravity Anomaly içeride olduğu her güncellemede slow
süresini tam 2 saniyeye yeniler. Hedef alanı terk ettiğinde veya field bittiğinde
pull anında kapanır; slow ve hedef visual'ı kalan 2 saniye boyunca yaşar, sonra
normal `GameplayEffectRuntimeSystem` cleanup yoluyla kaldırılır.

Barrier kırılması, owner-local event ile tetiklenen reaction örneğidir:

```text
Shield_Basic -> Barrier effect -> incoming DamageContext
-> BarrierBroken event -> trigger action -> temporary thrust effect
```

Event’ler effect damage işlemesinden sonra owner üzerinde senkron çalışır;
network prediction veya global event bus yerine mevcut tek oyunculu runtime
içindir. Effect visual’ları `GameplayEffectVisualRegistry` üzerinden spawn
edilir, yalnızca synchronize edilmiş effect state okur ve effect kaldırılırken
normal cleanup yoluyla yok edilir.

`AbilityUIController`, `sas::AbilityRuntimeSnapshot`; effect HUD ise
`GameplayEffectSnapshot` tüketir. UI, somut ability/effect sınıflarına göre
branch etmez. `CombatRuntime::Tick` aynı frame’de önce effect’leri, sonra
ability’leri tick eder. `CombatRuntime::Clear()` sırasıyla ability’leri iptal
eder, effect’leri normal cleanup ile kaldırır, tag’leri siler ve attribute /
modifier storage’ını temizler; respawn edilmiş gemi yeni runtime alır.

### 3.5 Katalog doğrulaması ve regresyon

Her shipped ability, effect, ability actor ve primary weapon tanımı katalog
üzerinden doğrulanır. `LightYearsGasLiteCore` CTest’i (`LightYearsGasLiteTests`
executable’ı) cooldown/charge, effect stack ve cleanup, Barrier event zinciri,
attribute sırası, weapon handler/feature, actor validation, Sun Beam spawn,
Gravity Anomaly typed profile/target clamp/field lifecycle/source cleanup/pull/
iki saniyelik exit tail/movement slow/scale/effect visual, effect catalog
lookup, Definition/Spec izolasyonu, geçersiz behavior/visual/ability-effect
reddi, damage overflow ve aggregate catalog validation’ı kapsar. Yeni shipped content
ilgili catalog enumerator’ına eklenmeli; aggregate validation hatası ertelenmiş
runtime davranışı değil, geçersiz game data kabul edilmelidir.

2026-07-29 tarihli statik `SpaceAbilitySystem` taşıması Attribute → Ability →
Effect sırasıyla tamamlandı. Ability 2A–2E dilimleri handle/policy,
snapshot, definition/event/validation, runtime state ve
factory/scheduler/collection mekaniklerini; 2F dilimi input/lifetime kararlarını
`sas::AbilityLifecycleOrchestrator` ile SAS'a taşıdı. Effect 3A–3C dilimleri
contract/runtime/collection/registry/binding çekirdeğini; 3D dilimi application,
stack/refresh ve duration expiry kararlarını
`sas::GameplayEffectLifecycleOrchestrator` ile SAS'a taşıdı. Aktif oyun
kaynaklarında geçiş alias'ı kalmadı; game-specific execution payload'ları,
Actor/DamageContext hook'ları, silah/attachment ve presentation adaptörleri
LightYearsGame'de tutuldu. Son sahiplik denetimi generic
`GameplayEffectRuntimeEntry<Spec>` ile source-scope/runtime-state/spec
sahipliğini, generic runtime context'i, behavior event/result kontratını,
catalog null/duplicate doğrulamasını ve collection predicate/index lookup
mekanizmasını da SAS'a taşıdı;
oyun `ActiveGameplayEffect` uzantısı yalnız Actor ve visual bağlantısını ekler.
Legacy karşılaştırma kopyaları, aktif bağlantı
olmadığı ve Debug/Release doğrulamaları geçtiği teyit edildikten sonra
2026-07-30 tarihinde silindi. Son entegrasyon doğrulamasında
`SpaceAbilitySystem` hedefinin CMake `STATIC` kütüphanesi olduğu ve hem
`LightYearsGame` hem `LightYearsGasLiteTests` tarafından linklendiği teyit
edildi. Debug ve Release konfigürasyonlarında `SpaceAbilitySystem.lib`,
`LightYearsGame.exe` ve `LightYearsGasLiteTests.exe` üretildi;
`LightYearsGasLiteCore` ile `LightYearsEngineLifetime` CTest sonucu iki
konfigürasyonda da 2/2 passed oldu.

### 3.6 Enemy Combat Runtime ve Hasar Modeli

Düşman foundation modeli `ly::EnemyActor : public SpaceShip` ve composition tabanlı `ly::EnemyRuntime` (`gameplay/enemy/EnemyRuntime.h`) ile çalışır. `ShipRuntime` aktör taban sınıfı yapılmaz. UFO, boss ve `EnemySpaceShip` legacy aktör sistemi kaldırılmıştır; yeni düşmanlar yalnız generic actor üzerinden üretilir.

#### Mimari ve Sorumluluklar
- `EnemyCombatProfile`: Yalnızca muharebe loadout'unu taşır (`profileId`, çoklu `weapons[]`, `abilities`, `powerScalingPolicy`, enemy progression). Şasi, can, ödül ve hareket bilgileri `ShipDefinition` içinde kalır.
- `EnemyDefinition`: `Enemy.<Role>.<Variant>` kimliğini `ShipDefinition`, `EnemyCombatProfile` ve `EnemyBehaviorProfile` kimliklerine bağlayan orchestration verisidir. Hardcoded Vanguard/TwinBlade/Hexagon listesi içermez.
- `EnemyBehaviorProfile` / `EnemyBehaviorRuntime`: Şimdiki basit, parametrik Approach/HoldRange/Strafe movement ve fire policy katmanıdır; target acquisition, target validity, aim, steering ve PrimaryFire intent üretir. Player singleton kullanmaz; opposing `Combatant` query altyapısını kullanır. Behavior runtime doğrudan ability çalıştırmaz. Advanced Enemy AI, threat/state machine/behavior tree sonraki aşamadır.
- `EnemyActor`: Combat profile'ı önce initialize eder; bu adım başarısızsa actor pending-destroy olur ve base gameplay/collision/spatial/on-spawn/reward akışına girmez. Başarılı actor, behavior intent'lerini `SpaceShip` movement/rotation/input API'lerine uygular.
- `DummyEnemy`: Arena silah/yetenek test target'ıdır; doğrudan `SpaceShip` kullanır ve `EnemyRuntime`, AI, behavior profile veya `EnemyFactory` kullanmaz.
- `EnemyRuntime`:
  - `weapons[]` girdilerini `WeaponContentCatalog` üzerinden çözer ve her weapon'ı belirtilen slotta ortak ability runtime hattına grant eder. Enemy yalnız weapon, yalnız ability, ikisi veya birden fazla weapon/ability taşıyabilir; slot ve passive kapasite contract'ı katalogda doğrulanır.
  - Non-primary yetenekleri `AbilityContentCatalog` üzerinden çözüp belirtilen slot (`Ability1`-`Ability4`) ve level ile `LightYearsAbilitySystemComponent`'a grant eder.
  - Verilen yetenek handle'larını saklar; re-initialization veya destruction sırasında yalnızca kendi verdiği yetenekleri kaldırır (harici buff/modifier/ability kayıtlarına dokunmaz).
  - Level growth ve deterministic variation'dan çözülen Health/Armor modifier'larını owner `AttributeSystem` üzerinde kendi source handle'larıyla uygular; shield katkısını `ShipRuntime` source-owned API'siyle verir.
  - Encounter çarpanını `CombatRuntime` üzerindeki `EnemyRuntime.EncounterDamage` kaynak kimliğine bağlar. Bu değer encounter × level × variation bileşimidir ve outgoing damage sınırında yalnız bir kez uygulanır.
  - `Clear()` çağrısında kendi weapon/ability handle'larını, progression modifier'larını, shield katkısını ve encounter modifier'ını temizler.
  - `CombatRuntime&` referansı ve canlı handle sahipliği nedeniyle move ve copy semantiği kapatılmıştır (`= delete`).
- `EnemyFactory`: `EnemyDefinition` üzerinden şasi, combat ve behavior referanslarını çözer; canonical `EnemySpawnContext { level, variationSeed }` ile generic `EnemyActor` üretir. Eski overload nötr level-1 context sağlar. Arena Dummy spawn'ları bu factory'ye bağlanmaz.

#### Atomik Initialization ve Rollback Kontratı
- `Initialize()` çağrısı mutation öncesinde preflight doğrulaması yapar (`ResolveLoadout` ve `CanReplaceCurrentLoadout`):
  1. Profil katalog kaydı doğrulanır.
  2. Birincil silah tanımı katalogda aranır.
  3. Tüm non-primary yetenek tanımları aranır ve `ContentIdSchema::ValidateAbilityId` doğrulaması yapılır.
  4. Yetenek seviyesi sınırları kontrol edilir (1 ile tanımın `GetMaxLevel()` aralığında olmalıdır).
  5. Slotların kendi aralarında çakışmadığı doğrulanır.
  6. Hedef ASC üzerindeki slotların kullanılabilirliği kontrol edilir: Mevcut `EnemyRuntime`'ın kendi handle'larının işgal ettiği slotlar yeniden kullanıma uygun sayılırken, başka bir sistem tarafından kullanılan hedef slot varsa initialization herhangi bir mutation başlamadan reddedilir.
- Preflight sonrasında kontrollü rollback transaction'ı uygulanır:
  1. Eski profil, çarpan ve owned handle snapshot'ı alınır.
  2. Eski handle'lar kaldırılır (`RemoveAbility` dönüşleri kontrol edilir).
  3. Yeni loadout grant edilir ve seviyeleri ayarlanır (`SetAbilityLevel` dönüşü kontrol edilir).
  4. Herhangi bir adım başarısız olursa bu denemede verilen yeni handle'lar kaldırılır, snapshot'taki eski profil ve loadout yeniden kurulur, eski seviye ve çarpan değerleri geri yüklenir.
  5. Rollback veya cleanup sırasında gerçekten kaldırılamayan handle kalırsa runtime `Faulted` olur ve handle unresolved ledger'ında tutulur; temizlenemeyen handle yoksa `Empty`/`Ready` state korunur. Harici ability'lere ve modifier kayıtlarına dokunulmaz.

#### Outgoing Multiplier ve Sayısal Güvenlik
Çarpanlar tekil hardcoded alanlar yerine `CombatRuntime` üzerindeki kaynak-kimlikli `CombatRuntimeModifier` kayıtları olarak tutulur. Ortak ürün hesabı `gameplay/math/MultiplierMath.h` içindeki düşük seviyeli utility'dedir; Combat ve Ship runtime aynı utility'yi kullanır. Multiplier yalnızca `gameplay/combat/Combatant.cpp` içindeki ortak `ApplyCombatDamage()` sınırında uygulanır:

```text
Weapon/ability authored resolution
-> source outgoing multiplier: resolvedDamage = damage * sourceOutgoingDamageMultiplier
-> crit resolution (Guaranteed / Chance)
-> rounding policy
-> target incoming effects (Barrier, Vulnerability vb.)
-> Barrier / ship shield absorption
-> remaining hull Armor reduction
-> health
```

- Çarpan matematiği sayısal güvenliğe sahiptir:
  - Modifikatör logaritmaları `long double` akümülatörde toplanır; böylece ara taşma ve `unordered_map` dolaşım sırası sonucu değiştirmez.
  - Her çarpan sonlu (`std::isfinite`) ve negatif olmayan ($\ge 0$) olmalıdır. Geçersiz producer değerleri kaydedilmez; conditional resolver'ın geçersiz çıktısı neutral `1.0` olarak yutulmaz, güvenli şekilde `0.0` sonuç üretir.
  - Herhangi bir modifikatör 0 ise toplam çarpan 0'dır.
  - Float taşmasında (overflow) sonuç `std::numeric_limits<float>::max()` değerine güvenli biçimde sınırlandırılır (clamp).
  - Hasar formülünde `damage * sourceOutgoingDamageMultiplier` sonucunun sonlu kalması garanti edilir.
- Multiplier mermi, ışın, expanding wave, ability damage, temas hasarı ve DoT efektlerine ortak ve tek seferlik uygulanır; silahlarda mükerrer (double) scaling yapılmaz.

#### AP/EP Default-Zero Kontratı
- Düşman attribute sisteminde `CombatRuntime::InitializeOwnerAttributes()` varsayılan olarak `AttackPower = 0` ve `EnergyPower = 0` (ayrıca `AttackSpeed = 0`, `CriticalChance = 0`, `CriticalDamage = 1.5`) atar.
- Düşman saldırıları varsayılan olarak AP/EP scaling kullanmaz (`EnemyPowerScalingPolicy::Disabled`).
- `Disabled` politikasına sahip profillerde, ilişkili silah veya yetenek tanımında `OwnerAttributeIds::AttackPower` ya da `OwnerAttributeIds::EnergyPower` kaynaklı scaling kuralı bulunması durumunda katalog doğrulaması (`UsesPowerScaling`) başarısız olur ve profil reddedilir.
- `Allowed` politikası yalnızca ileride özel mekanik gerektiren düşmanlar için ayrılmıştır ve tek başına AP/EP'ye otomatik değer atamaz.

#### Gerçek Düşman Silah Değerleri (Vertical-Slice Geçici Değerler)
`weapons.json` dosyasındaki üç geçici combat profile silahı şu generic arketiplere bağlanır:
- **ApproachGunner** (`Weapon.Projectile.EnemyVanguardPulse.Basic`): Damage 15, FireRate 1.2, Speed 700, Range 1200, `Damage.Type.Energy`.
- **StrafeSkirmisher** (`Weapon.Projectile.EnemyTwinBladeScatter.Basic`): Damage 8, FireRate 0.8, Range 500, 3 pellet, `Damage.Type.Kinetic`.
- **RangeKeeper** (`Weapon.Wave.EnemyHexagonCryoPulse.Basic`): Damage 10, FireRate 0.6, Speed 500, Range 1200, InitialWidth 60 -> MaximumWidth 200, `Damage.Type.Cryo`.
Bu değerler mimari entegrasyonu doğrulamaya yönelik vertical-slice geçici değerlerdir; production balance aşamasında revize edilebilir.

#### Stage ve içerik sınırı

- LevelOne, ChaosStage ve InfiniteStage geçici stage/wave akışlarıdır; wave director bu aşamada yeniden tasarlanmamıştır.
- Üç `Enemy.*.*` tanımı provisional vertical-slice içeriğidir. Düşman saldırılarında authored base damage, encounter multiplier ile çarpılır; enemy AP/EP alanları sistemde bulunur fakat varsayılan değerleri 0'dır.
- Enemy content loader boş listeleri, duplicate ID'leri, eksik referansları, chassis üzerindeki `primaryWeaponId` değerini, unknown JSON alanlarını, geçersiz Strafe interval'ını ve toplamı 1'i aşan reward ağırlıklarını reddeder.

### 3.7 Encounter Wave Runtime ve Arena Lifecycle

`EncounterWaveRuntime`, arena veya gelecekteki encounter tüketicileri için küçük, actor-ownership tabanlı wave state machine'dir. Author edilmiş `EnemyWaveDefinition` girdisini, `EnemyFactory` callback'ini ve `EncounterProgression` ayarını alır; encounter dışı actor'ları izlemez.

```text
Idle -> Spawning -> WaitingForClear -> InterWaveDelay -> Spawning
                                           \-> Completed
                                     \-> Failed
```

- Spawn interval büyük `deltaTime` içinde tüketilir; kaçırılan spawn oluşmaz.
- Son wave temizlenince delay beklenmeden `Completed` olur.
- `EncounterProgression`, ek enemy sayısı, enemy level cap'i ve deterministik seed üretiminden sorumludur; enemy stat formüllerini bilmez.
- `EnemySpawnContext` factory'den actor/runtime'a taşınır. `level >= 1` zorunludur; seed `0` nötr variation anlamına gelir.
- `EncounterWaveSnapshot` yan etkisiz read-only projeksiyondur: state, current/total wave, planned/spawned/alive/remaining enemy sayıları, inter-wave kalan süre ve resolved enemy level taşır. Expired/pending-destroy referansların temizliği `Tick()` içinde kalır.
- `ArenaTestLevel`, ayrı `ArenaEncounterState` (`Idle / Running / Completed / Failed`) ile player death, restart ve owned-enemy cleanup lifecycle'ını sahiplenir. Bu state, wave runtime state'iyle birleştirilmez.
- `PlayerRespawnSystem`, player ship destroy delegate handle'ını sahiplenir ve `Clear()` ile destructor'ında unbind eder. Arena restart önce eski ship'i kontrollü yok eder, sonra PlayerManager reset ve yeni spawn ile fresh encounter başlatır.

`EncounterHUDController` bu snapshot kontratını tüketen mevcut minimal HUD katmanıdır; wave veya enemy matematiğini yeniden hesaplamaz. Reward, victory akışı ve encounter JSON migration hâlâ bu aşamanın dışındadır.

#### Test ve Doğrulama Ayrımı
- `LightYearsGasLiteTests`: Enemy foundation bölümünde runtime loadout, encounter multiplier, atomic initialization/rollback, harici slot çakışması, non-primary ability slotu ve generic `EnemyActor` projectile spawn sözleşmeleri doğrulanır.
- `LightYearsContentTests`: Enemy profil katalog yükleme ve validation contract'ları burada doğrulanır.
- `LightYearsGasLiteTests`: Runtime, lifecycle, behavior ve gerçek projectile akışını doğrular; behavior testleri geçmeden behavior sistemi tamamlandı kabul edilmez.
- Bu bölümün son durumu, değişiklik sonrası toplu build/test turunun sonucuyla birlikte güncellenir.

## 4. Silah konfigürasyonu ve progression

İlgili kaynaklar:

- LightYearsGame/include/gameConfigs/combat/WeaponStructs.h
- LightYearsGame/assets/content/data/weapons.json
- LightYearsGame/src/gameplay/weapon

### 4.1 Silah tanımı kuralı

Her PrimaryWeaponDefinition tam olarak bir yaprak type tag seçer:

- Projectile.Standard
- Projectile.Shotgun
- Arc.Electric
- Beam.Continuous
- Wave.Expanding

Bir type handler, yalnızca kendi attribute root’larını kabul eder. Yeni silah
eklerken önce type handler ve validator uyumluluğu, sonra config tanımı
eklenmelidir. feature tag’leri (örneğin Heat) type’dan bağımsız ek
çalışma zamanı davranışıdır.

### 4.2 Oyuncu silahları — güncel başlangıç değerleri

| Silah | Type / tür | Temel değerler | Owner scaling |
| --- | --- | --- | --- |
| Weapon.Projectile.FighterRapidLaser.Basic | Standard / Photonic | Damage 12, FireRate 4, speed 1100, range 1600, radius 7, 1 muzzle, Mag 48, Reload 2.0s; every 6th and final 6 rounds empowered | Normal damage +0.40 AttackPower; empowered +2 base +0.10 EnergyPower, guaranteed 1.5x crit and final damage ceil; Cadence: OwnerAttackSpeedPercentage (FireRate × (1 + max(0, AS)/100)); Reload = 2.0s / (1 + max(0, AS)/100) |
| Weapon.Projectile.RapidShotgun.Basic | Shotgun / Thermal | Damage 10, FireRate 2.5, speed 3000, range 400, 3 pellet, spread 8°, floor x0.5 | Damage +0.75 AttackPower; FireRate +0.5 AttackSpeed |
| Weapon.Projectile.DualKineticBlaster.Basic | Standard / Kinetic | Damage 3.5, FireRate 12, speed 3400, range 650, radius 6, 2 muzzle | Damage +0.45 AttackPower; FireRate +1.0 AttackSpeed |
| Weapon.Arc.ElectricLauncher.Basic | Arc / Electric | Damage 15, FireRate 2.8, range 850, 3 normal chain, chain range 250, x0.72 / chain | Damage +0.85 AttackPower; FireRate +0.60 AttackSpeed; no direct Luck damage |
| Weapon.Beam.ContinuousHeatLaser.Basic | Continuous beam / Energy | Damage 28 DPS, range 950, width 26, heat 38/sn, cap 100, cool 25/sn, overheat 2.5 sn, max heat x1.75 | Damage +0.75 AttackPower ve +0.50 EnergyPower; AttackSpeed only affects high-heat gain |
| Weapon.Wave.CryoProjector.Basic | Expanding wave / Cryo | Damage 7, FireRate 1.8, range 780, speed 850, width 80→260, thickness 30 | Damage +0.75 AttackPower; FireRate +0.5 AttackSpeed |

Bu değerler ham tanım değerleridir. Level, attachment, owner stat ve status
etkileri uygulandıktan sonraki ekrandaki değer farklı olabilir.

### 4.3 Silaha özel matematik

**Shotgun — aynı hedefe birden fazla pellet**

~~~text
perPelletMultiplier = max(minimumDamageMultiplier,
	                          1 - damageReductionPerAdditionalHit * priorHitsOnTarget)
damagePerPellet = baseDamage * perPelletMultiplier
~~~

Rapid Shotgun’da üç pellet aynı hedefe çarparsa çarpanlar sırasıyla x1.0,
x0.9 ve x0.8 olur; toplam vuruş hasarı `10 + 9 + 8 = 27` olur. Hasar her
pellet impact anında uygulanır; volley'nin son pellet'ini beklemez. Üç pellet
farklı hedefe çarparsa her hedef ilk-hit x1.0 ile 10 alır.

**Electric Arc**

İlk hedef base damage alır. Sonraki her hedefin hasarı:

~~~text
damage[n] = baseDamage * chainDamageMultiplier^n
~~~

Mevcut x0.72 ile dört hedefin teorik toplamı
15 × (1 + 0.72 + 0.72² + 0.72³) olur. Her hedef yalnızca bir kez seçilir,
hedef seçiminde en yakın uygun hedef kullanılır. Normal zincirler tamamlandıktan
sonra tek bir ek zincir ancak ek bir uygun hedef varsa zar atar. Olasılık
`0.35 * CombatRuntime::GetCombatLuckFactor()`'dır; bu merkezi doygun Luck
eğrisini kullanır, doğrudan hasar scale'ı değildir. Aynı hedef tekrar
seçilemez; ek uygun hedef yoksa bonus da yoktur.

**Continuous Heat Laser**

~~~text
heatRatio = clamp(currentHeat / heatCapacity, 0, 1)
instantDPS = baseDPS * (1 + (maxHeatMultiplier - 1) * heatRatio)
tickDamage = instantDPS * deltaTime
~~~

Mevcut tanımda lazer 28 DPS’den başlar, tam ısıda 49 DPS’ye çıkar. Isı
artışı eğrisi 0–50% x1.0, 50–75% x0.5, 75–90% x0.25, 90–100% x0.15’tir.
Tam kapasiteye ulaşınca 2.5 sn cooldown ister ve heat sıfırlanır. AttackSpeed,
Damage, fire rate veya tick rate'i değiştirmez: 0–50% heat'te etkisiz, 50–75%
arasında doygun eğriyle 0→%35 heat-gain reduction'a çıkar, 75–100%'de en
fazla %70 heat-gain reduction sağlar.

### 4.3.1 Primary weapon balance runtime checks (2026-07-24)

> Tarihsel test kaydıdır. 7 Eylül 2026 source review sırasında yeniden
> çalıştırılmadı; aşağıdaki sonuçlar güncel build/test kanıtı değildir.

`GasLiteCoreTests`, production `LightYearsAbilitySystemComponent`/
`GameAbilityActionExecutor` attribute resolution
and current Fighter `ShipProgression` profile at levels 1, 10, 25 and 50. The checks
cover AP/AS/EnergyPower contribution, muzzle and pellet aggregates, Cryo four-hit tempo,
Electric 1/2/4/5 target falloff plus high-Luck proc cap, and Beam heat/overheat with
10- and 30-second sustained windows. All values use weapon level 1, no attachment,
no crit, no armor and 100% hit rate.

| Weapon | L1 / L10 / L25 / L50 single-target theory DPS |
| --- | --- |
| Rapid Laser | 64.00 / 1513.00 / 7168.00 / 25593.00 |
| Rapid Shotgun (3 same-target pellets) | 60.00 / 1188.60 / 5985.60 / 22080.60 |
| Dual Kinetic (2 muzzles) | 84.00 / 1677.90 / 7250.40 / 24637.90 |
| Electric Arc (first target) | 42.00 / 687.57 / 3415.92 / 12553.17 |
| Continuous Heat Laser (pre-heat multiplier) | 28.00 / 93.25 / 202.00 / 383.25 |
| Cryo Wave | 12.60 / 426.83 / 2332.20 / 8882.83 |

### 4.4 Oyuncu silah progression’ı

Tüm bu profillerde maksimum level 4 ve level 2/3/4 scrap bedelleri
40 / 50 / 65’tir.

| Silah | L2 | L3 | L4 |
| --- | --- | --- | --- |
| Basic Rapid Laser | +1 FireRate | +2 Damage, +100 projectile speed | +2 Damage, +1 FireRate, +100 Range |
| Dual Kinetic Blaster | +1.5 FireRate | +1 Damage | +100 Range, +1 FireRate |
| Electric Arc Launcher | +4 Damage | +1 chain, +80 chain range | +0.4 FireRate, +0.08 chain multiplier |
| Continuous Heat Laser | +6 Damage | +150 beam range | +0.20 max heat damage multiplier |
| Cryo Wave Projector | +2 Damage, +30 max width | +100 range, +0.5 cryo buildup duration | +0.3 FireRate, +2 Damage |

### 4.5 Magazine, Cadence ve Empowered Shot Sistemi (Phase 3B.1–3B.2)

Ortak primary weapon altyapısına opsiyonel magazine/reload kontratı, yüzde tabanlı AttackSpeed cadence modeli, generic empowered shot tanımı ve crit/rounding pipeline eklenmiştir. Yalnızca Fighter `BasicRapidLaser` üzerinde etkinleştirilmiştir:
- **Crit Pipeline & Global Çarpan**:
  - Global temel crit çarpanı `2.0` yerine `1.5` olarak standardize edilmiştir.
  - `DamageCriticalPolicy` enum (`Random`, `Guaranteed`, `Disabled`) merkezi damage pipeline'a (`ApplyCombatDamage`) eklenmiştir.
  - `Random` policy mevcut `CriticalChance` zarını kullanır.
  - `Guaranteed` policy zar atmadan kaynağın çözülmüş `Owner.CriticalDamage` çarpanını uygular.
  - Crit yalnız bir kez uygulanır; guaranteed crit olan atışlar random crit ile asla ikinci kez çarpılmaz.
  - `Owner.CriticalDamage` başlangıçta `1.5`tir ve tüm kritik kaynaklar aynı attribute'u kullanır.
- **Damage Rounding Contract (Merkezi Combat Çözümleme Sırası)**:
  - Sıra: `Raw shot damage` → `crit policy` → `ceil if shot requests integer primary damage (roundDamageUp)` → `armor / shield damage pipeline`.
  - Fighter `BasicRapidLaser`, weapon-level `damageRoundingPolicy = CeilFinalDamage` kullanır; normal ve empowered shot'lar aynı tamsayı hasar konvansiyonunu paylaşır.
  - Armor ve shield hesapları float hassasiyetinde kalır.
  - Global olarak bütün ability hasarlarına zorunlu `ceil` uygulanmaz; yalnız shot payload'u talep ederse çalışır.
- **Fighter L1 normal shot**: `NormalRawDamage = 12 + AttackPower × 0.40`. Fighter AP=35 iken normal shot ceil(12 + 14) = `26` hasar verir. FireRate 4.0 atış/saniye, magazine 48 ve base reload 2.0s'dir. Normal shots random crit kullanır (L1'de CriticalChance=0).
- **Generic Empowered-Shot Tanımı**:
  - `PrimaryWeaponEmpoweredShotDefinition` (`everySuccessfulShots`, `finalMagazineRounds`, `bonusBaseDamage`, `bonusDamageScaling`, `guaranteedCritical`) silaha özgü tanımdır. `bonusDamageScaling` yalnız `sourceAttributeId + coefficient` taşır; runtime'ın kullanmadığı generic target/operation alanları yoktur.
  - Validator kuralları: `everySuccessfulShots > 0`, `finalMagazineRounds >= 0`, magazine yoksa `finalMagazineRounds == 0`, `finalMagazineRounds <= magazine.capacity`, bonus değerleri finite, scaling source attribute canonical (`Owner.*`) ve yalnız interval projectile silahlarda desteklenir.
- **Empowered Cadence ve Tespiti (`PrimaryWeaponExecutionSystem::FireOnce`)**:
  - Her weapon runtime, reload'dan bağımsız kalıcı başarılı fire/volley sayacı tutar. Periyodik empowered kontrolü yalnız bu sayacın başarılı sonraki değeriyle yapılır; shotgun gibi çok projectile üreten silahlar bir volley'i tek fire olarak sayar.
  - `roundsBeforeFire = roundsRemaining`
  - `shotIndex = capacity - roundsBeforeFire + 1`
  - `periodic = (shotIndex % 6 == 0)`
  - `finalPhase = (roundsBeforeFire <= 6)`
  - `isEmpowered = periodic || finalPhase`
  - Hesap başarılı spawn öncesinde yapılır; spawner başarısızsa mermi veya empowered durumu harcanmaz.
  - Reload sonrası capacity geri geldiği için pattern otomatik olarak baştan başlar.
- **Hasar Hesabı ve AP/EP Ayrımı**:
  - Normal: `raw = 12 + AP × 0.40`; `final = ceil(raw)` (Fighter L1: `26`).
  - Empowered: `raw = NormalRaw + 2 + EP × 0.10`; `crit = guaranteed 1.5x`; `final = ceil(raw × 1.5)`. Fighter L1: `ceil((26 + 2 + 3.5) × 1.5) = ceil(47.25) = 48`.
  - AP artışı hem normal hem empowered hasarın tabanını artırır. EP artışı ise yalnız empowered bonusunu artırır.
- **Atış Örüntüsü (Cadence Pattern)**:
  - Şarjör başında `26 / 26 / 26 / 26 / 26 / 48` döngüsüyle başlar (1–5 normal, 6 empowered).
  - 42. atış periyodik kuraldan, 43–48 arası 6 atış son-altı kuralından empowered olur; 42–48 arasında ardışık 7 empowered atış fired edilir.
  - Magazine toplamı: Tam 48 mermide **35 normal (26 dmg) + 13 empowered (48 dmg)** bulunur.
  - Toplam deterministik şarjör hasarı: `35 × 26 + 13 × 48 = 910 + 624 = 1534`.
  - Sustained cycle DPS: 12 saniye atış + 2 saniye reload = 14 saniye döngü; `1534 / 14 ≈ 109.57` (~109.6 DPS).
- **Hedef TTK Referansları (Fighter L1 vs Sabit Düşman Canı)**:
  - İlk mermi `t=0` anında çıkar; 4 atış/saniye (0.25s aralık).
  - 180 HP düşman: 5 normal (130) + 1 empowered (48) = 178 hasar (2 HP eksik kalır); 7. atışta (26 dmg) ölür. `t = 1.5 saniye` (7 atış).
  - 250 HP düşman: 6 atış (178) + 2 normal (52) = 230 hasar; 9. atışta (26 dmg, toplam 256) ölür. `t = 2.0 saniye` (9 atış).
- **Shot Metadata ve Projectile Presentation**:
  - `PrimaryWeaponShotMetadata` (`isEmpowered`, `criticalPolicy`, `roundFinalDamageUp`) spawner üzerinden mermiye aktarılır ve `proj->IsEmpowered()` / `proj->GetShotMetadata()` ile erişilebilir.
  - Bu aşamada mermi sprite'ı, point light, renk, VFX ve ses efektleri değiştirilmemiştir; sunum katmanı ilerideki aşamalarda bağlanacaktır.
- **Formüller**:
  - `AttackSpeedMultiplier = 1 + max(0, Owner.AttackSpeed) / 100` (PercentageRatingScale = 100)
  - `FinalFireRate = ResolvedWeaponFireRate * AttackSpeedMultiplier`
  - `FireInterval = 1 / FinalFireRate`
  - `FinalReloadTime = BaseReloadTime / AttackSpeedMultiplier`
- **Successful-fire API**: `PrimaryWeaponHandler::FireOnce` atış başarısını `bool` döndürür. Standard projectile / Shotgun / Expanding wave için başarı en az bir aktörün spawn olmasıdır; Electric arc için geçerli world üzerinde arc atışı yapılmasıdır. 1 başarılı volley = 1 ammo tüketir. Başarısız atış ammo tüketmez, cooldown başlatmaz ve `AfterFire` çalıştırmaz.
- **Reload Snapshot & Zamanlama**: Son atış magazine'i boşalttığı anda güncel AS okunup reload duration snapshot'ı alınır; devam eden reload süresi anlık AS değişiminden etkilenmez, sonraki reload yeni AS'yi kullanır. Fire cooldown ve reload paralel ilerler (`NextFireReadyIn = max(FireCooldown, ReloadRemaining)`); reload bitiminde fazladan bir tam fire interval beklenmez.
- **Runtime Sahipliği ve Lifecycle**: Ammo ve reload state `PrimaryWeaponRuntimeState.magazineState` altındadır. Level up, attachment refresh veya input release/repress refill yapmaz. Inactive tick sırasında reload ilerler. Temporary primary override asıl silahın ammo/reload state'ini korur.
- **Uzun Frame ve Event Boundary**: Magazine interval firing event-boundary döngüsüyle işlenir. Simülasyon zamanı reload bitişi ve atış hazır olma anlarına bölünerek deterministik tüketilir; reload sırasında kaçırılmış atış birikmez. Catch-up limiti aşılırsa kalan zaman silaha ait kalıcı `PrimaryWeaponRuntimeState.unprocessedSimulationTime` alanında saklanır. Input bırakılırsa bu borç atış üretmeden cooldown/reload süresine işlenir ve temizlenir; sonraki basış eski frame borcundan burst üretemez. Temporary override runtime'ları kendi borçlarını taşır.
- **Magazine doğrulaması**: JSON `capacity` değeri pozitif ve runtime'ın signed `int` aralığında olmalıdır; `INT_MAX` üzerindeki değerler narrowing uygulanmadan loader tarafından reddedilir.
- **TTK referansı**: İlk başarılı projectile `t=0` anında çıkar. Önceki 28-damage normal-shot TTK referansları bu balance ile geçersizdir; enemy durability/TTK yeniden ayrı bir aşamada normalize edilecektir.

## 5. Gemi, movement, enerji ve progression

### 5.1 ShipDefinition alanları

ShipDefinition bir geminin texture, health, hız başlangıcı, collision damage,
score, bağımsız ship XP reward, explosion, engine mount, loot listesi,
primary weapon, movement, energy ve progression konfigürasyonunu taşır.

shipXPReward verilmemişse scoreAmt değeri kullanılır; score ve progression
ödülü böylece ileride birbirinden bağımsız dengelenebilir.

### 5.2 Player Fighter — mevcut değerler

Mevcut production player-ship kapsamı yalnız Fighter'dır. Breacher,
Interceptor, Conductor, Aegis ve Cryo Controller kimlikleri tasarım notu olarak
korunur; bu aşamada `ShipDefinition`, `ships.json` kaydı, seçim/runtime bağlantısı
veya balance profili oluşturulmayacaktır. Phase 3A çalışmaları, aksi ayrıca
kararlaştırılana kadar Fighter üzerinden ilerler.

| Alan | Değer |
| --- | --- |
| Health (L1) | 250 |
| Base Owner Combat Attributes | AttackPower = 35 (`baseOwnerAttributes`), Armor = 0, Luck = 0, AttackSpeed = 0, CriticalChance = 0, CriticalDamage = 1.5, AbilityHaste = 0, MoveSpeedHorizontal = 0, MoveSpeedVertical = 0 |
| Başlangıç silahı | Weapon.Projectile.FighterRapidLaser.Basic |
| Forward / Reverse / Strafe thrust | 650 / 190 / 270 |
| Angular speed / responsiveness | 400 / 5 |
| Linear damping | 0.36 |
| Max speed | 520 |
| Input responsiveness / mouse dead zone | 12 / 32 |
| EnergyPower / ReactorBudget | 35 / 70 |
| Base shield / affinity / final shield | 115 / 0.50 / 150; full recharge 6 sn; delay 6 sn |
| Base afterburner / affinity / final capacity | 65 / 0.50 / 100; full recharge 6 sn; delay 1.5 sn |
| Afterburner speed / acceleration multiplier | 1.55 / 1.80 |
| Afterburner drain | 33/sn |
| Afterburner ramp up / down / maneuverability | 0.22 sn / 0.45 sn / x0.90 |
| XP başlangıcı / üssü | 100 / 1.25 |

Hareket rating katkısı doğrusal değildir:

~~~text
movementContribution = contributionPerRating * scale *
                       (1 - exp(-max(0, rating) / scale))
~~~

Varsayılan scale 20’dir. Böylece yüksek hareket stack’lerinde marjinal kazanç
azalır.

### 5.3 Enerji türetmeleri

~~~text
reactorBudget = EnergyPower * 2
maxShield = baseMaxShield + reactorBudget * shieldAffinity
afterburnerCapacity = baseAfterburnerCapacity +
                      reactorBudget * afterburnerAffinity
shieldRegenPerSecond = maxShield / shieldFullRechargeDuration
afterburnerRegenPerSecond = afterburnerCapacity / afterburnerFullRechargeDuration
healthRegenPerSecond = maxHealth / 1200
~~~

`shieldAffinity + afterburnerAffinity = 1.0` ship-profile invariant'ıdır.
EnergyPower resource scaling tamamen lineerdir: hard cap veya diminishing
return uygulanmaz. MaxHealth veya EnergyPower değiştiğinde ShipRuntime bu
değerleri yeniden hesaplar. Mevcut resource miktarı kapasite artışında
korunur, kapasite düştüğünde yalnız yeni maksimuma clamp edilir.

> **Aşama 2:** `EnergyPower` tüketilen bir enerji havuzu değil, genel reaktör
> gücü owner attribute'udur. `EnergyPower Reference = 100` yalnız ortak
> referanstır; doğrusal resource formülünü normalize etmez.

### 5.4 Runtime sınırları

`CombatRuntime`, combat attribute'ları, damage/effect pipeline'ını ve
ability'leri sahiplenir; ShipDefinition veya ship'e özgü shield/afterburner
değerlerini bilmez. `SpaceShip`, CombatRuntime'ın owner AttributeSystem'ine
bağlanan `ShipRuntime`'ı sahiplenir. ShipRuntime, MaxHealth ve EnergyPower
değiştiğinde shield, health regen ve afterburner kapasite/regen değerlerini
yeniden çözer.

Bu ayrım, gelecekteki `EncounterRuntime` ve `EnemyRuntime` katmanlarının
combat sınırını genişletmeden kendi durumlarını sahiplenebilmesi içindir.
Yeni runtime eklenirken sorumluluk ilgili domain sahibinde kalmalı; yalnızca
combat akışına ait veriler CombatRuntime'a eklenmelidir.

### 5.5 Gemi XP’si

~~~text
xpRequiredForNextLevel = max(1, baseXP * currentLevel^xpExponent)
totalStatBonus = (currentLevel - 1) * perLevel
~~~

Ship level 1’den başlar. `ShipProgressionDefinition`, `baseXP`, `xpExponent`
ve kontrollü `naturalGrowth` listesini taşır. Her liste elemanı doğrudan
`{ attributeId, perLevel }` tanımlar; merkezi base-growth tablosu ve gemiye
özel multiplier katmanı kullanılmaz. Listede bulunmayan attribute büyümez.

İzin verilen doğal büyüme hedefleri MaxHealth, AttackPower, EnergyPower,
Armor, Luck, AttackSpeed, CriticalChance, AbilityHaste,
MoveSpeedHorizontal ve MoveSpeedVertical owner attribute'larıdır. Duplicate,
negatif, non-finite veya bu whitelist dışındaki girdiler content validation'da
reddedilir. Movement için yeni genel attribute oluşturulmaz; iki mevcut eksen
ayrı ayrı tanımlanır.

Fighter'ın eski migration compatibility growth listesi kaldırılmış ve Phase 3A.3
ile resmi production Fighter base profili ve natural-growth değerleri devreye alınmıştır.
Bu aşamada Fighter için L1 base AttackPower 35 değeri kontrollü generic `baseOwnerAttributes`
alanı üzerinden ship profile tarafından verilir; L1'de progression modifier bonusu sıfırdır.

Fighter gerçek natural-growth profili tam beş attribute taşır:
- MaxHealth: +75 / level
- AttackPower: +3 / level
- EnergyPower: +2 / level
- Armor: +2 / level
- Luck: +1 / level

AttackSpeed, CriticalChance, AbilityHaste, MoveSpeedHorizontal ve MoveSpeedVertical
sistem whitelist'inde kalmaya devam eder fakat Fighter JSON/profile listesinde yer almaz
ve Fighter'da doğal büyüme almaz.

Bu değerler mevcut Fighter production değeridir; gelecekte playtest ve telemetri
sonuçlarına göre yeniden tune edilebilir. Bütün gemiler için küresel bir standart
değildir. Production player ship kapsamı yalnız Fighter'dır; Breacher, Interceptor,
Conductor, Aegis ve Cryo Controller ertelenmiştir ve altı gemilik nihai karşılaştırma
yapılmamıştır.

#### Fighter Seviye ve Kaynak Tablosu

| Stat / Kaynak | L1 | L5 | L10 | L15 |
| --- | ---: | ---: | ---: | ---: |
| MaxHealth | 250 | 550 | 925 | 1300 |
| AttackPower | 35 | 47 | 62 | 77 |
| EnergyPower | 35 | 43 | 53 | 63 |
| Armor | 0 | 8 | 18 | 28 |
| Luck | 0 | 4 | 9 | 14 |
| AttackSpeed | 0 | 0 | 0 | 0 |
| CriticalChance | 0 | 0 | 0 | 0 |
| AbilityHaste | 0 | 0 | 0 | 0 |
| MoveSpeed H/V | 0 | 0 | 0 | 0 |
| MaxShield | 150 | 158 | 168 | 178 |
| AfterburnerCapacity | 100 | 108 | 118 | 128 |
| ShieldRegenPerSecond | 25.0 | ~26.3333 | 28.0 | ~29.6667 |
| AfterburnerRegenPerSecond | ~16.6667 | 18.0 | ~19.6667 | ~21.3333 |
| Armor DR | 0% | 8/108 (~7.407%) | 18/118 (~15.254%) | 28/128 (21.875%) |

AttackSpeed, CriticalChance ve AbilityHaste `0..1` fraction veya ekranda
yazılan yüzde değil, ham **rating** birimini kullanır. Ortak rating ölçeği
`100`dür: `2.0` iki rating puanıdır; critical eğrisinde yaklaşık `%1.98`
sonuç verir, `0.02` ise `%2` anlamına gelmez. Her tüketici bu owner rating'i
kendi sözleşmesine göre çözer; mevcut primary weapon scaling'i AttackSpeed'i
FireRate'a doğrudan additive uygular ve bu Phase 3A.1'de yeniden tasarlanmamıştır.

### 5.6 Fighter BasicRapidLaser Seviye (L1–L15) ve Hasar Ölçekleme Modeli (Phase 3B.3)

Fighter başlangıç silahı `Weapon.Projectile.FighterRapidLaser.Basic`, Phase 3B.3 ile maksimum seviye 15'e genişletilmiştir. Seviye artışı FireRate (4.0 atış/sn), şarjör kapasitesi (48 mermi), reload süresi (2.0 sn) ve kadans modunu değiştirmez; yalnızca hasar ve scaling katsayılarını artırır.

#### Hasar ve Yuvarlama Kontratı

~~~text
Normal Raw Damage:      BaseDamage + AttackPower * APRatio
Normal Final Damage:   ceil(Normal Raw Damage)
Empowered Raw Damage:  NormalRaw + EmpoweredBaseDamage + EnergyPower * EPRatio
Empowered Final:       ceil(Empowered Raw Damage * resolved Owner.CriticalDamage)
~~~

Fighter taban `Owner.CriticalDamage` değeri 1.5'tir. Empowered atışlar garantili kritik (`DamageCriticalPolicy::Guaranteed`) uygular ve integer damage convention gereği `ceil` ile yukarı yuvarlanır.

#### Seviye Büyüme Modeli (L2–L15 Recurring Rule)

- Top-level L1: BaseDamage = 12, AP Ratio = 0.40, EmpoweredBaseDamage = 2, EP Ratio = 0.10.
- Her seviye (L2..L15): Common.Damage +10, PrimaryWeapon.Empowered.BonusDamage +1, AP scaling Add +0.05, EP scaling Add +0.03.
- Scrap Maliyetleri (14 kademe geçici placeholder): [40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 105].

#### Seviye Karşılaştırma Tablosu

| Seviye | Base Damage | AP Katsayısı | Empowered Base | EP Katsayısı | Fighter Stat (AP / EP) | Normal Hasar (Raw / Final) | Empowered Hasar (Raw / Final) |
|---|---:|---:|---:|---:|:---:|:---:|:---:|
| L1 | 12 | 0.40 | 2 | 0.10 | 35 / 35 | 26.0 / **26** | 31.5 / **48** |
| L5 | 52 | 0.60 | 6 | 0.22 | 47 / 43 | 80.2 / **81** | 95.66 / **144** |
| L10 | 102 | 0.85 | 11 | 0.37 | 62 / 53 | 154.7 / **155** | 185.31 / **278** |
| L15 | 152 | 1.10 | 16 | 0.52 | 77 / 63 | 236.7 / **237** | 285.46 / **429** |

*Not: L10 tablosunda 185.31 * 1.5 = 277.965 -> ceil 278.*

#### Şarjör ve Sustained Hasar Referansı (L15)

- 48 mermilik şarjör dağılımı: 35 normal atış + 13 empowered atış.
- Şarjör toplam hasarı: `35 × 237 + 13 × 429 = 8295 + 5577 = 13872` hasar.
- Atış süresi: `48 / 4.0 = 12.0` saniye; Reload: `2.0` saniye; Toplam döngü: `14.0` saniye.
- Sustained döngü DPS: `13872 / 14.0 ≈ 990.86 DPS`.
- Scrap maliyetleri geçici placeholder ekonomi değerleridir.
- Diğer birincil silahlar Phase 3B.3 kapsamında değiştirilmemiştir.

## 6. Kamera

İlgili kaynak: LightYearsEngine/include/framework/camera/CameraManager.h ve
LightYearsEngine/src/framework/camera/CameraManager.cpp.

Kamera üç ayrı look-ahead kaynağını işler:

1. Follow target: geminin dünya konumu.
2. Hareket look-ahead: dışarıdan verilen gemi hızı yönünde offset.
3. Cursor look-ahead: mouse world pozisyonuna doğru offset.

İki offset toplanır, combinedLookAheadMaxOffset ile sınırlandırılır, sonra
lookAheadSmoothingSpeed üzerinden üstel yumuşatılır:

~~~text
alpha = 1 - exp(-smoothingSpeed * deltaTime)
current = lerp(current, desired, alpha)
~~~

Pozisyon üstel smoothing kullanır. Zoom ise mevcut zoom hızını frame'ler
arasında koruyan kritik sönümlü bir tepki kullanır. Hedef değiştiğinde zoom
hızı bir karede ters çevrilmez; önce yavaşlar, sonra yeni hedefe döner. Hız
zoom’u lineer veya smoothstep olabilir:

~~~text
composedZoom = baseZoom + speedAlpha * maxSpeedZoomOut + additionalZoomOut
zoom = composedZoom * (1 + relativeAdditionalZoomOut)
~~~

Varsayılan önemli kamera değerleri:

| Ayar | Değer |
| --- | --- |
| baseZoom | 1.55 |
| cursor strength / dead zone / max distance | 0.45 / 380 / 600 |
| movement strength / max distance | 0.75 / 320 |
| combined offset max | 380 |
| speedForMaxZoomOut / maxSpeedZoomOut | 500 / 0.32 |
| position / zoom smoothing | 2.75 / 3.2 |
| world bounds padding | 200 |

CameraManager ayrıca sinüs tabanlı ve zamanla karesel olarak azalan shake
efekti, dünya sınırı clamp’i, external velocity ve gameplay kaynaklı ek zoom
katmanları sunar. Mutlak katman afterburner gibi sabit katkılar içindir; göreli
katman önce hesaplanan mevcut kamera mesafesinin yüzdesini ekler. ArenaLevel
her tick gemi velocity, afterburner zoom, Dash göreli zoom oranı ve mouse world
konumunu bu yöneticide günceller. Gemi Dash durumundayken kısa süreli Dash
velocity yerine son normal external velocity korunur; mouse girdisi ve
follow-target takibi kesilmez. `preserveFollowTargetOffset` aktifken kamera
merkezi target displacement'ıyla aynı miktarda taşınır ve normal position
smoothing kaynaklı Dash gecikmesi oluşmaz. Dash göreli zoom hedefi de genel
`zoomSmoothingSpeed` ve kalıcı zoom velocity üzerinden kritik sönümlü olarak
yumuşatıldığı için farklı arena kamera ayarlarıyla aynı davranış ailesinde
kalır. World-bounds clamp hesabı hedef zoom
yerine o karede yumuşatılmış gerçek view boyutunu kullanır; arena kenarında
zoom ile kamera merkezi birbirinden farklı hızlarda sıçramaz.

## 7. Arena ve boundary

ArenaDefinition:

| Alan | Varsayılan |
| --- | --- |
| size | 3000 × 2000 |
| legalBounds | (0,0,size) |
| outOfBoundsMargin | 20 |
| outOfBoundsTime | 5 sn |
| boundary visual | Açık, DebugRectangle, cyan normal / kırmızı uyarı |

ArenaBoundarySystem, takip edilen aktör legal bounds + margin dışında
kaldığında süre saymaya başlar ve her tick warning event’i gönderir. Aktör
tekrar sınır içine girdiğinde sayaç sıfırlanır. Süre dolduğunda bir defalık
penalty event gönderilir.

ArenaLevel, oyuncu gemisine cezayı uygulanmadan önce invulnerability’yi
kaldırır ve aktif Barrier kapasitesini de hesaba katan öldürücü hasar üretir.
Bu, sınır ihlalinin Barrier ile kaçınılmasını önler.

ArenaTestLevel mevcut test ayarları:

- Boyut: 6000 × 3000
- Margin: 20
- Grace: 5 sn
- Respawn delay: 1 sn
- Spawn konumu: arena merkezi
- Hareket modu: ThrustDrift

## 8. Level, düşman, UI ve sunum

### Level / dalga

- LevelOne normal oyun akışı, arka plan katmanları, oyuncu spawnı, stage ve
  boss akışını taşır.
- InfiniteStage için güncel ölçek:

~~~text
enemyCount = min(5 + wave * 2, 25)
spawnInterval = max(1.5 - wave * 0.075, 0.5)
difficultyMultiplier = 1 + (wave / 5) * 0.25
~~~

- Formasyon geçişi: wave 1–4 V, 5–8 Reserve V, 9–12 Snake, 13–16 Scatter,
  17–20 Zipper, sonrası Scatter.

### Sunum

- Widget çatısı HUD, gauge, button ve text bileşenlerini sağlar.
- GameHUD, ability/effect/health/shield/energy gibi gameplay verilerini
  görünür hale getirir.
- Gameplay warning HUD, arena sınır ihlalinin geri sayımını gösterir.
- Engine tarafında sprite, point light shader, parallax background, audio ve
  particle/VFX altyapısı bulunur.

## 9. Yeni özellik ekleme kontrol listesi

### Yeni ability

1. AbilityDefinition: slot, activation/lifetime, cooldown, action/trigger,
   display ve icon.
2. Kullanacağı actor/effect varsa ilgili config ve runtime handler.
3. Attribute ve scaling rule’ları; damage tag’i.
4. Level progression ve scrap cost’ları.
5. Attachment capability/slot kararları.
6. Feature-local typed presentation profile, stable profile ID ve presentation
   content registration.
7. Missing/unknown profile validation; typed registry isolation; visual actor,
   telegraph, impact ve cleanup testleri.
8. HUD görünümü, test senaryosu ve Notebook’a denge notu.

### Yeni silah

1. Yaprak weapon type veya yeni handler/feature kararını verin.
2. Validator’ın zorunlu attribute’larını karşılayın.
3. PrimaryWeaponDefinition: type, attributes, muzzle, damage tag, scale.
4. ProgressionProfile ve scrap maliyetleri.
5. Runtime test: fire rate, target seçimi, hit, status, cooldown/heat.
6. Notebook’taki silah kartına teorik ve ölçülen DPS’yi yazın.

### Yeni gemi

1. ShipDefinition: health, collision, reward/XP, weapon, movement ve energy.
2. Engine mount, VFX ve collision layer kontrolü.
3. Progression growth override’ları.
4. En az bir normal hedef, zırhlı hedef, shield hedef ve arena hareket testi.

### Yeni arena / level

1. ArenaDefinition ile legal bounds, margin, grace time ve visual ayarlayın.
2. CreateCameraSettings override’ı gerekiyorsa kamera profilini ekleyin.
3. Respawn policy, penalty, HUD warning ve test spawnını belirleyin.
4. Yeni wave/difficulty formülünü Notebook karar kaydına yazın.

## 10. Dokümantasyon bakımı

- Gerçekleşmiş bir değişiklikte bu dosyadaki güncel tabloyu değiştirin.
- Her değişiklik yalnızca etkilediği sahiplik belgesinde kaydedilir. Bir değişiklik
  runtime davranışını, JSON envanterini ve tarihli denge kararını birlikte
  etkiliyorsa ilgili belgeler birbirine bağlantı verir; aynı tabloyu üç kez
  kopyalamayın.
- Henüz karara bağlanmamış fikirleri yalnızca
  [Balance & Roadmap Notebook](BALANCE_AND_ROADMAP_NOTEBOOK.md) içinde
  “Fikir” veya “Deney” statüsünde tutun.
- Bir denge değişikliğinde hedef metrik, ölçüm yöntemi, önce/sonra sonucu ve
  test senaryosu yazılmadan sayı değiştirmeyin.
- Kod yolu değiştiğinde bu dosyadaki kaynak listelerini güncelleyin.
