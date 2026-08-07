# LightYears — Sistem Referansı

## Dash implementation

`Ability.Movement.Dash.Basic`, Ability3/F üzerinde çalışan ayrı bir ability behavior
sınıfıdır: `gameplay/ability/dash/DashAbility`. `AbilitySystem`, davranışı
`AbilityBehaviorRegistry` üzerinden üretir; game adaptörü
`GameAbilityActionExecutor` yalnızca
ortak action türlerini çalıştırır. Dash validation, start/end event ve state tag
cleanup bu sınıfta kalır. `DashMovementController` ability ile gemi hareketi
arasındaki dar kontrattır; `MovementComponent` input-or-mouse yönünü ve fiziksel
hareketi uygular. `PlayerSpaceShip` içinde Dash davranışı bulunmaz.

Base mesafe 260'dır. Yatay ve dikey movement rating ortalaması `1-exp(-rating/20)`
eğrisiyle en fazla +%50 mesafeye dönüşür. AttackPower, AttackSpeed, EnergyMax,
Luck, Critical ve ability level mesafeyi değiştirmez. Level 2-5 yalnızca cooldown'u
2.0'dan 1.88/1.76/1.64/1.52 saniyeye indirir. Seviye başına düşüş taban
cooldown'un `%6`'sıdır; böylece taban cooldown tuning'i progression'ı geçersiz
hâle getirmez. Ability Haste merkezi cooldown çarpanı üzerinden en son uygulanır.

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
> Uygulanan her değişiklik aynı değişiklik setinde bu belgeye,
> [Current Implementation Catalog](CURRENT_IMPLEMENTATION_CATALOG.md)'a ve
> Balance & Roadmap Notebook'a kaydedilir.
>
> Kaynak anlık görüntüsü: 24 Temmuz 2026. Bu belge, çalışma ağacındaki mevcut
> sistemleri açıklar; önerilen fikirler yalnızca notebook dosyasında tutulur.

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
- AbilitySystem ve GameplayEffectSystem tick/uygulama/damage yolları
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
- LightYearsGame/src/gameplay/effects/GameplayEffectBehavior.cpp
- LightYearsGame/src/gameplay/effects/BarrierEffectBehavior.cpp
- LightYearsGame/src/spaceShip/SpaceShip.cpp
- SpaceAbilitySystem/include/attributes/AttributeMath.h

### 2.1 Hasar çözüm sırası

Bir combat hedefi için sıra aşağıdaki gibidir:

~~~text
Base hit
  -> kaynak kritik zar atışı
  -> hedefin PreMitigation effect'leri (Electric)
  -> standart effect'ler (Barrier)
  -> zırh ve armor penetration
  -> damage type status effect'leri
  -> gemi shield component'i
  -> health
  -> damage taken / damage dealt olayları
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
criticalDamage = baseDamage * max(1, criticalDamageMultiplier)
~~~

- DamagePayload varsayılan olarak kritik destekler ve kritik çarpanı 2.0’dır.
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
`HasGameplayAttribute`/base-list üretimi ve runtime snapshot construction da
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

### 2.4 Zırh

Zırh azaltma eğrisi:

~~~text
ArmorScale = 50 / ln(2) ~= 72.1348
armorReduction = 1 - exp(-armor / ArmorScale)
effectiveArmor = armorReduction * (1 - armorPenetration)
postArmorDamage = preArmorDamage * (1 - effectiveArmor)
~~~

- Armor penetration 0 ile 0.25 aralığına clamp edilir.
- Kinetic hasarın varsayılan armor penetration değeri 0.10’dur.
- Bu işlem Barrier effect’inden sonra, geminin kalıcı shield
  component’inden önce yapılır.

### 2.5 Elektrik, Barrier ve gemi shield’ı

**Electric status**, PreMitigation aşamasındadır:

~~~text
remainingDamage = remainingDamage * (1 + multiplierPerStack * stackCount)
~~~

Electric’in ilk üç stack’i yalnızca buildup/visual bilgisidir; hasarı değiştirmez.
Varsayılan payload 3 sn sürer, dört stack’te dolar ve yalnızca 4/4 stack’te
uygulanır. Stack başına 0.04 ile tam-stack çarpanı **1.16**’dır.

**Barrier effect** kapasite tabanlı geçici kalkan effect’idir:

~~~text
shieldCapacitySpent = min(capacity, incomingDamage * absorptionRatio * shieldDamageMultiplier)
sourceDamageAbsorbed = shieldCapacitySpent / (absorptionRatio * shieldDamageMultiplier)
~~~

Barrier kırılırsa effect kaldırılır ve Event.Owner.BarrierBroken olayı
gönderilir. Basic Barrier: 30 kapasite, 1.0 absorption ratio, 6/sn
rejenerasyon, 1.5 sn temel rejenerasyon gecikmesi, 5 sn effect süresi.

**Gemi shield component’i** Barrier’dan sonra çalışır ve aynı
shieldDamageMultiplier mantığını kullanır. Energy hasarının 1.25 çarpanı,
aynı kaynak hasarı için daha fazla shield kapasitesi harcatır; kalkan
kısmen kırılırsa kalan kaynak hasarı health’e doğru biçimde taşınır.

### 2.6 Hasar türleri

DamageTypeSystem, damage tag listesinde aşağıdaki sırayla bulunan ilk türün
varsayılan payload’ını oluşturur: Energy → Kinetic → Thermal → Cryo →
Electric. Birden fazla tür tag’i verildiğinde payload davranışı ilk eşleşen
türden gelir; bu nedenle hibrit hasar tasarlanmadan önce bu öncelik özellikle
değiştirilmelidir.

| Tür | Varsayılan davranış | Denge etkisi |
| --- | --- | --- |
| Photonic | Özel payload yok | Nötr başlangıç hasarı |
| Energy | Shield damage x1.25; shield regen delay +0.75 sn | Shield karşıtı |
| Kinetic | Armor penetration 0.10 | Zırhlı hedef karşıtı; sık atışta düşük penetration |
| Thermal | 1 Ignite/hit; 1 DPS/stack; 3 sn; en çok 4 stack; yalnızca 4/4’te 4 DPS | Zamanla hasar; dört vuruşluk payoff |
| Cryo | 1 buildup/hit; 4 birikimde %25 slow; 1.5 sn | Kontrol / hareket kırma; tamamlandıktan sonraki her Cryo hit slow süresini yeniler, şiddeti artırmaz |
| Electric | 1 stack/hit; stack başına x0.04; 3 sn; en çok 4; yalnızca 4/4’te x1.16 | Hedefi sonraki hasara kontrollü biçimde açık bırakma |

Status effect’leri yalnızca armor işlemi sonrasında remainingDamage pozitifse
uygulanır. Bir hedef Barrier/zırh tarafından tamamen korunmuşsa status
uygulanmaz.

### 2.7 Hasar örneği

100 hasarlık bir Energy vuruşu, %50 armor reduction, %25 penetration ve 20
kapasiteli bir Barrier’a karşı:

~~~text
Barrier kapasite harcaması = min(20, 100 * 1.0 * 1.25) = 20
Barrier’ın emdiği kaynak hasar = 20 / 1.25 = 16
Barrier sonrası kaynak hasar = 84
effectiveArmor = 0.50 * (1 - 0.25) = 0.375
health/shield'e kalan = 84 * (1 - 0.375) = 52.5
~~~

Gemi shield kapasitesi ayrıca varsa 52.5 üzerinden devreye girer. Kritik,
bu zincirin başında uygulanır.

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
| Attribute | `Attribute.<Owner>...` | İlgili sistemin attribute ID kataloğu |
| Effect behavior | `EffectBehavior.<Family>[.<Behavior>]` | Feature-local effect behavior |
| Primary weapon | `PrimaryWeapon.<Family>.<Type>` / `PrimaryWeapon.Feature.<Feature>` | Weapon handler/feature |
| Damage | `Damage.Type.<Type>` | Damage type schema |
| Attachment capability | `Attachment.Capability.<Capability>` | Attachment schema |

Bu tablo yalnız runtime'da sorgulanan veya grant edilen gameplay taglerini
gösterir. `Ability.<Category>.<Family>.<Variant>` ve
`Attachment.<Family>.<Name>.<Variant>` biçimindeki somut kayıt kimlikleri
`GameplayTag` değil, `ContentIdSchema` tarafından doğrulanan string content
ID'leridir.

`State.ActionLock.AbilityActivation` ve
`State.ActionLock.PrimaryWeaponFire` iki kayıtlı ortak kilittir. Bir mekanik
bu taglerden birini owner'a geçici olarak verir; `GameAbility::CanActivateContent`
tüm normal ability aktivasyonlarını ilk tag ile, PrimaryFire aktivasyonunu ikinci
tag ile engeller. Overdrive gibi bir ability kendi local firing state'ini UI ve
lifecycle için ayrıca kullanabilir; global input/activation engeli için yeni
feature-özel block tag üretilmez.

Bir gameplay tag ancak domain'i, producer'ı ve consumer'ı açıksa eklenir.
Parent tag doğrudan grant edilmez; parent sorgusu yalnız bilinçli grup sorgusu
olarak kullanılır. Yeni content doğrulaması ability, effect, ability actor,
primary weapon, attachment JSON loader'ı ve ship progression giriş noktalarında
bu şemayı çağırır.

Her ability ailesinin `gameplay/ability/<family>/<Family>Contracts.h` dosyası
aynı contract yüzeyini kullanır: zorunlu `AbilityId`, `CategoryTag`, `FamilyTag`
ve `BehaviorTag`; varsa `State`, `Event` ve `Actor` alt alanları. Actor role'ü
altında onun definition ID'si, type tag'i ve attribute tag'leri birlikte kalır.
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
verisinde bu durum Dash ve InfernoSpray olmak üzere 6 ability'den 2'si için
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
- Sorgulanan veya grant edilen semantic değerler `GameplayTag` olur. Struct
  alanları ve rol bildiren sabitler `...Tag` ile biter: `behaviorTag`,
  `FamilyTag`, `TypeTag`, `FeatureTag`. `DamageTypeSchema::Thermal` gibi türü
  enclosing schema tarafından açık olan leaf sabitler kısa kalabilir.
- `...Id` daima catalog/registry kaydı olan string kimliktir. Attachment kayıt
  kimliği content ID'dir; yalnız `Attachment.Capability.*` değerleri tagdir.
- `Effect.<Family>.<Variant>` yalnız content ID'dir; aktif effect varlığını
  taşıyan semantic tagler `State.Effect.<Family>.<State>` altında kalır.
- Her ability family contract'ında üst seviyede yalnızca `AbilityId`,
  `CategoryTag`, `FamilyTag`, `BehaviorTag` bulunur. İsteğe bağlı veriler
  sahipliğine göre `State`, `Event`, `Actor`, `Effect` ve `Setting` altında
  gruplanır. JSON numeric-setting contract'ı `Setting::Contract` içinde kalır;
  loader family'yi bilmez, behavior registry üzerinden onu çözer.
- Aileye özel actor attribute'ları daima
  `Attribute.AbilityActor.<Family>.<Role>.<Name>` biçimindedir ve ilgili
  `Actor::<Role>` contract'ında tanımlanır. `CommonAttributeIds` değerleri
  contract'ta yeniden adlandırılmaz. Bir actor başka bir role ait değerleri
  yalnızca onları üretilecek actor'a iletmek için tüketiyorsa (Gravity Anomaly
  projectile -> field gibi), handler iki role ait dar kökleri açıkça bildirir;
  aile kökü tek başına yetki vermez.
- Somut shipped ability'nin ID family segmenti, `BehaviorTag`in son segmentiyle
  aynı olmak zorundadır; yalnız generic `GameAbilityBehavior.Configured` ile
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
kaydedilmeyen presentation ID veya yarım actor/profile yüzeyi açılmaz. Overdrive
Core taslağı bu nedenle yalnız contract ve schema testi olarak tutulur; behavior,
JSON ve actor/profile bir sonraki dikey dilimde birlikte eklenir.

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

Mevcut durum: weapon, player ship, ability ve gameplay effect değerleri
`LightYearsGame/assets/content/data/*.json` dosyalarından runtime'da yüklenir.
Şemalar, validation, behavior/action tanımları ve typed presentation profilleri
C++ tarafında kalır. Attachment JSON parser'ı vardır ancak attachment mekaniği
henüz runtime'a bağlanmamıştır.

Hedef hibrit sınır aşağıdaki gibidir:

| Katman | Sahiplik |
| --- | --- |
| Harici content (aktif JSON) | Silah, player ship ve ability kaynaklarına ait denge değerleri; gameplay effect policy/contract kayıtları; ID/reference alanları |
| CSV | Yalnız editör/balance import-export ve analiz; runtime'ın otoriter kaynağı değil |
| C++ | Şemalar, parse/semantic validation, gameplay tag eşlemesi, behavior ve weapon handler'ları, action türleri, typed presentation profile türleri/registration ve tüm mutable runtime state |

İlk geçişte yalnız value/reference ağırlıklı content taşınır. Mevcut JSON kaydı
behavior veya presentation seçebilir; fakat bu ID'lerin C++ registry'de
kayıtlı/uyumlu olması validation ile zorunlu kılınır. `AbilityActorDefinition`
ve §3.0.1'deki typed presentation sözleşmesi değişmez. Böylece 30 ability,
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
değerleri `weapons.json` içindeki `Attribute.Damage.*` alanlarında; ability'nin
uyguladığı effect değerleri `abilities.json` içindeki `effectSpecs` alanında;
ability actor alan değerleri actor kaydında veya ability-local
`attributeProfiles` içinde tutulur. `effects.json` magnitude, duration, stack
limiti veya runtime attribute base value tutmaz; yalnız effect ID, behavior,
duration/stacking politikası, tag, visual ve source-scope sözleşmesini taşır.

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
| behaviorTag | Ability ailesine ait behavior factory kaydı |
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
    |-- GameplayEffectSystem
    `-- LightYearsAbilitySystemComponent -> sas::GameplayAbilityInstance (handle başına)
```

`CombatRuntime`, owner-local combat attribute’larını, tag’lerini, effect’lerini
ve ability’lerini sahiplenir; HUD widget’ları ve visual actor’lar gameplay
state sahibi değildir. Saf tanımlar bugün `gameConfigs/ability` ve
`gameConfigs/combat` altındaki C++ catalog'larında bulunur; planlanan JSON
content bu immutable tanımların alternatif kaynağı olacaktır. Mutable state ise
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

### 3.2 Mevcut aktif ability’ler

| Ability | Slot | Çalışma | Başlangıç / scale | Level |
| --- | --- | --- | --- | --- |
| Shield_Basic | Ability1 (Q), varsayılan player grant yok | 8 sn cooldown, 5 sn duration; Basic Barrier uygular | Barrier kapasitesi +0.20 × MaxHealth ve +50 × Armor | Progression tanımlı değil |
| SunBeam_Strike_Basic | Ability2 (E) | MouseWorld konumunda SunBeam strike actor spawn eder | Başlangıç damage 40, radius 96, width 72, length 720 | 2–5: her level +8 Damage, +8 Radius; scrap: 40/50/65/80 |
| Dash_Basic | Ability3 (F) | 2 sn cooldown, 0.24 sn duration; input veya mouse yönünde hareket | Base 260 mesafe; movement rating ile en fazla +%50; mevcut kamera hedefinin üzerine +%15 zoom-out | 2–5: cooldown 1.88/1.76/1.64/1.52 sn |
| Rocket_Basic | Ability4 (R) | 7 sn cooldown; mouse aim yönünde tek projectile, fare konumunda veya daha önce çarpışırsa Kinetic alan patlaması | Base damage 55 + AttackPower×1.25; radius 55; speed 1000; range 1100 üst sınırdır ve level ile değişmez | 2–15: her level +4 Damage, -0.12 sn cooldown, +1 Radius; L6/L15 evolve seçimi ertelendi |
| GravityAnomaly_Basic | Ability1 (Q), varsayılan player grant | 8 sn cooldown; cursor hedefi 900 menzile clamp edilir, projectile yalnız hedefe ulaştığında sabit field üretir | Damage yok. Field: 2.5 sn, radius 220, pull 500, %20 MovementSlow; MaxHealth yalnız radius (+0.20) ve duration'ı (+0.0025) scale eder | 2–15: her level -0.10 cooldown, +0.03 duration, +2 radius, +10 pull, +0.005 slow, +25 speed, +5 range |
| Primary fire | PrimaryFire (Space) | Weapon definition’dan otomatik üretilir | Silahın progression ve scaling rule’ları kullanılır | Silah profiline bağlı |

SunBeam strike zamanları: telegraph 0.5 sn, arrival 0.2 sn, impact delay
0.05 sn, impact visual 0.22 sn. Bunlar `SunBeamConfig.h` içindeki
`ActorStrikeBasic` tanımındadır.

### 3.3 Attachment kataloğu

Attachment host/capability koşullarını geçtikten sonra ability veya primary
weapon üzerinde çözülür. Koşullu modifier’lar damage tag veya attribute
durumuna göre uygulanır.

| Attachment | Hedef | Temel etki | Koşullu / olay etkisi |
| --- | --- | --- | --- |
| Thermal Converter | Ability, PrimaryWeapon | Ignite sonrası cooldown azaltma değeri 0.4 | Thermal damage: Damage x1.20; Ignite olayı tüm non-primary ability cooldown’larını azaltır |
| Energy Coupler | Ability, PrimaryWeapon | Shield damage x1.25, regen delay +0.75 sn | Energy damage: Damage x1.15 |
| Kinetic Bore | Ability, PrimaryWeapon | Armor penetration 0.10 | Kinetic damage: +0.05 armor penetration |
| Cryo Conduit | Ability, PrimaryWeapon | Cryo 4-hit, %25 / 1.5 sn parametreleri | Cryo slow +0.05 (üst sınır %30) |
| Electric Conduit | Ability, PrimaryWeapon | Electric 0.04/stack, 3 sn, max 4 | Electric stack multiplier +0.01 (üst sınır 0.05/stack) |
| Heavy Capacitor | Ability | Damage x1.40 | Cooldown x1.25; net güçlü fakat daha yavaş active ability |
| Emergency Salvo | PrimaryWeapon | Ek projectile başlangıç değeri 0 | FireRate < 4 ise +1 projectile |

### 3.4 Effect, event, UI ve cleanup kontratı

Effect verisi üç ayrı yaşam katmanına sahiptir:

```text
GameplayEffectDefinition (immutable shipped content)
    -> GameplayEffectSpec (kaynağa göre çözümlenmiş uygulama payload'u)
        -> ActiveGameplayEffect (hedefe ait mutable runtime state)
```

`GameplayEffectDefinition`; ID, duration/stack politikası, tag, behavior,
visual ve source-scope sözleşmesini taşır. Source-parameterized shipped
effect'lerde sayısal denge değeri taşımaz. Silah, ability, enemy, reward veya
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
`ly::GameplayEffectSystem` bu collection'ı compose eder; modifier/granted-tag
bağlama ve cleanup, stacking/source-scope eşleşmesi, behavior damage dispatch'i
ve visual lifecycle sırasını oyun adaptörü olarak korur. Önceki ayrı
`List<ActiveGameplayEffect>` ve handle sayacı karşılaştırması kaldırılmıştır.

Effect 3C diliminde generic kayıt ve attribute/tag binding mekanikleri ayrılır.
`sas::GameplayEffectBehaviorRegistry<Hooks>` tag ile typed hook değeri arasındaki
duplicate-safe depolama, lookup ve registration sorgusunu sağlar; callback
imzalarını tanımlayan `GameplayEffectBehavior::Hooks` oyun katmanındadır.
`GameplayEffectBindings` required/blocked application tag gate'ini, Instant
effect base modifier uygulamasını, active modifier handle ekleme/sökme ve
granted-tag ekleme/sökme işlemlerini yürütür. `GameplayEffectSystem` refresh,
stack ve removal sırasını; behavior callback, pending DamageContext event ve
visual lifecycle orkestrasyonunu korur.

`GameplayEffectSystem`; instant, duration ve infinite yaşam politikalarını;
no-stack, refresh-duration ve stack davranışlarını; modifier/tag ekleme ve
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
normal `GameplayEffectSystem` cleanup yoluyla kaldırılır.

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
| Weapon.Projectile.FighterRapidLaser.Basic | Standard / Photonic | Damage 8, FireRate 8, speed 1100, range 1600, radius 7, 1 muzzle | Damage +1.0 AttackPower; FireRate +1.0 AttackSpeed |
| Weapon.Projectile.RapidShotgun.Basic | Shotgun / Thermal | Damage 10, FireRate 2.5, speed 3000, range 400, 3 pellet, spread 8°, floor x0.5 | Damage +0.75 AttackPower; FireRate +0.5 AttackSpeed |
| Weapon.Projectile.DualKineticBlaster.Basic | Standard / Kinetic | Damage 3.5, FireRate 12, speed 3400, range 650, radius 6, 2 muzzle | Damage +0.45 AttackPower; FireRate +1.0 AttackSpeed |
| Weapon.Arc.ElectricLauncher.Basic | Arc / Electric | Damage 15, FireRate 2.8, range 850, 3 normal chain, chain range 250, x0.72 / chain | Damage +0.85 AttackPower; FireRate +0.60 AttackSpeed; no direct Luck damage |
| Weapon.Beam.ContinuousHeatLaser.Basic | Continuous beam / Energy | Damage 28 DPS, range 950, width 26, heat 38/sn, cap 100, cool 25/sn, overheat 2.5 sn, max heat x1.75 | Damage +0.75 AttackPower ve +0.50 EnergyMax; AttackSpeed only affects high-heat gain |
| Weapon.Wave.CryoProjector.Basic | Expanding wave / Cryo | Damage 7, FireRate 1.8, range 780, speed 850, width 80→260, thickness 30 | Damage +0.75 AttackPower; FireRate +0.5 AttackSpeed |

Bu değerler ham tanım değerleridir. Level, attachment, owner stat ve status
etkileri uygulandıktan sonraki ekrandaki değer farklı olabilir.

### 4.3 Silaha özel matematik

**Shotgun — aynı hedefe birden fazla pellet**

~~~text
perPelletMultiplier = max(minimumDamageMultiplier,
                          1 - damageReductionPerAdditionalHit * (hitCount - 1))
damagePerPellet = baseDamage * perPelletMultiplier
~~~

Rapid Shotgun’da üç pellet aynı hedefe çarparsa çarpan x0.8, toplam vuruş
hasarı 3 × 10 × 0.8 = 24 olur; üçü farklı hedefe çarparsa her hedef x1.0
ile 10 alır.

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
fazla %35 reduction uygulanır. Bu overheat'i kaldırmaz; sadece yüksek heat
bölgesine daha uzun süre erişim sağlar.

### 4.3.1 Primary weapon balance runtime checks (2026-07-24)

`GasLiteCoreTests`, production `LightYearsAbilitySystemComponent`/
`GameAbilityActionExecutor` attribute resolution
and current Fighter `ShipProgression` profile at levels 1, 10, 25 and 50. The checks
cover AP/AS/EnergyMax contribution, muzzle and pellet aggregates, Cryo four-hit tempo,
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

## 5. Gemi, movement, enerji ve progression

### 5.1 ShipDefinition alanları

ShipDefinition bir geminin texture, health, hız başlangıcı, collision damage,
score, bağımsız ship XP reward, explosion, engine mount, loot listesi,
primary weapon, movement, energy ve progression konfigürasyonunu taşır.

shipXPReward verilmemişse scoreAmt değeri kullanılır; score ve progression
ödülü böylece ileride birbirinden bağımsız dengelenebilir.

### 5.2 Player Fighter — mevcut değerler

| Alan | Değer |
| --- | --- |
| Health | 100 |
| Başlangıç silahı | Weapon.Projectile.FighterRapidLaser.Basic |
| Forward / Reverse / Strafe thrust | 650 / 190 / 270 |
| Angular speed / responsiveness | 400 / 5 |
| Linear damping | 0.36 |
| Max speed | 520 |
| Input responsiveness / mouse dead zone | 12 / 32 |
| Base shield | 100; full recharge 5.5 sn; delay 4 sn |
| Afterburner capacity | 50; full recharge 8 sn; delay 1.5 sn |
| Afterburner speed / acceleration multiplier | 1.55 / 1.80 |
| Afterburner drain | 16.5/sn |
| MaxEnergy → shield | 0.5 shield / MaxEnergy |
| MaxEnergy → afterburner capacity | 1.0 capacity / MaxEnergy |
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
maxShield = baseMaxShield + EnergyMax * maxShieldPerMaxEnergy
afterburnerCapacity = baseAfterburnerCapacity +
                      EnergyMax * afterburnerCapacityPerMaxEnergy
shieldRegenPerSecond = maxShield / shieldFullRechargeDuration
afterburnerRegenPerSecond = afterburnerCapacity / afterburnerFullRechargeDuration
healthRegenPerSecond = maxHealth / 1200
~~~

MaxHealth veya EnergyMax değiştiğinde ShipRuntime bu değerleri yeniden
hesaplar. Health, shield ve energy kapasitesi değişiminde yüzde korunur.

### 5.4 Runtime sınırları

`CombatRuntime`, combat attribute'ları, damage/effect pipeline'ını ve
ability'leri sahiplenir; ShipDefinition veya ship'e özgü shield/afterburner
değerlerini bilmez. `SpaceShip`, CombatRuntime'ın owner AttributeSystem'ine
bağlanan `ShipRuntime`'ı sahiplenir. ShipRuntime, MaxHealth ve EnergyMax
değiştiğinde shield, health regen ve afterburner kapasite/regen değerlerini
yeniden çözer.

Bu ayrım, gelecekteki `EncounterRuntime` ve `EnemyRuntime` katmanlarının
combat sınırını genişletmeden kendi durumlarını sahiplenebilmesi içindir.
Yeni runtime eklenirken sorumluluk ilgili domain sahibinde kalmalı; yalnızca
combat akışına ait veriler CombatRuntime'a eklenmelidir.

### 5.5 Gemi XP’si

~~~text
xpRequiredForNextLevel = max(1, baseXP * currentLevel^xpExponent)
totalStatBonus = completedLevels * baseGrowth * shipGrowthOverride
~~~

Ship level 1’den başlar. Mevcut genel base growth’lar:

| Attribute | Base growth / level |
| --- | --- |
| MaxHealth | 8 |
| EnergyMax | 2 |
| AttackPower | 3 |
| AttackSpeed | 0.5 |
| AbilityHaste | 0.5 |
| MoveSpeedHorizontal / Vertical | 0.2 / 0.2 |
| Armor | 1.5 |
| Luck | 0.3 |
| CriticalChance | 0.35 |

Player Fighter override’ları AttackPower x3, AttackSpeed x2,
CriticalChance x2, MaxHealth x1, Armor x1, EnergyMax x0.5 ve yatay/dikey
movement x0.5’tir. Tanımlanmamış attribute’larda varsayılan büyüme çarpanı
x0.25 kullanılır.

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
- Her uygulanan değişiklikte bu dosyaya ek olarak
  **BALANCE_AND_ROADMAP_NOTEBOOK.md** ve
  **CURRENT_IMPLEMENTATION_CATALOG.md** aynı değişiklik setinde güncellenir;
  üç kayıt tamamlanmadan iş bitti kabul edilmez.
- Henüz karara bağlanmamış fikirleri yalnızca
  [Balance & Roadmap Notebook](BALANCE_AND_ROADMAP_NOTEBOOK.md) içinde
  “Fikir” veya “Deney” statüsünde tutun.
- Bir denge değişikliğinde hedef metrik, ölçüm yöntemi, önce/sonra sonucu ve
  test senaryosu yazılmadan sayı değiştirmeyin.
- Kod yolu değiştiğinde bu dosyadaki kaynak listelerini güncelleyin.
