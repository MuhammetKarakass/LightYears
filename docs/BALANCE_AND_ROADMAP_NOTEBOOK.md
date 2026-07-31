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
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 1 | Kök CMake'e statik `SpaceAbilitySystem` target'ı eklendi. Generic `GameplayAttribute`, `AttributeSystem`, modifier/scaling tipleri ve `AttributeMath` `SpaceAbilitySystem/include/attributes` ile `src/attributes` altına, `sas` namespace'ine taşındı. Owner/ship/common attribute ID kataloğu oyun içeriği olarak `LightYearsGame/include/gameplay/attributes/AttributeIds.h` içinde bırakıldı. | Uygulandı; toplu doğrulama bekliyor | Taşıma Attribute → Ability → Effect sırasıyla yürütülecek. Bu aşamada davranış veya denge formülü değiştirilmedi; test/build kullanıcı kararı gereği kütüphane taşıması tamamlanınca tek seferde çalıştırılacak. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 2A | Ability taşımasının ilk diliminde `sas::AbilityHandle` ile slot, activation, lifetime, action phase, end reason ve targeting/spawn/direction policy enum'ları `SpaceAbilitySystem/include/abilities` altına alındı. Aktif oyun definition'ı `gameplay/ability/content/GameAbilityDefinition.h` altında SAS tiplerini kullanır; silah/action payload'ı, UI alanları ve ability content'i LightYearsGame'de kalır. Eski `gameplay/ability/legacy/AbilityStructs.h` karşılaştırma için silinmedi ve aktif include zincirinden çıkarıldı. | Uygulandı; toplu doğrulama bekliyor | Runtime `AbilitySystem`/`AbilityInstance`/`AbilityExecutor` henüz taşınmadı. Geçiş alias'ları sonraki Ability dilimlerinde açık `sas::` kullanımlarına dönüştürülecek; build/test tüm kütüphane tamamlanınca çalıştırılacak. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 2B | Pointer ile game-owned `AbilityDefinition` taşıyan eski snapshot kontratı yerine `sas::AbilityRuntimeSnapshot` eklendi. Yeni snapshot handle, ability ID, slot, level, active/cooldown/duration/charge değerlerini sahiplenir ve LightYearsGame tiplerine bağımlı değildir. `AbilityInstance::BuildSnapshot`, `AbilitySystem::BuildSnapshots` ve ilgili test kaynağı yeni SAS tipine geçirildi. | Uygulandı; toplu doğrulama bekliyor | Eski pointer tabanlı şekil `gameplay/ability/legacy/AbilityRuntimeSnapshot.h` içinde karşılaştırma amacıyla korunur. Execution/Instance/System implementasyonları weapon, attachment ve Effect bağımlılıkları ayrılmadan taşınmayacak; build/test taşıma sonunda yapılacak. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 2C | Generic `sas::AbilityDefinition` kimlik, slot/lifecycle, owner-tag koşulları ve attribute/scaling alanlarını sahiplenir. `ly::GameAbilityDefinition` bu tabanı action/trigger/level, scrap, damage/attachment, behavior ve UI content'iyle genişletir; mevcut `ly::AbilityDefinition` adı geçiş alias'ıdır. Generic definition doğrulaması SAS `.cpp` kaynağına taşındı. `sas::AbilityEvent` tag/source/target/magnitude çekirdeğini, oyun `ly::AbilityEvent` ise yalnız `DamageContext` uzantısını taşır. | Uygulandı; toplu doğrulama bekliyor | Eski definition, event ve validation şekilleri legacy/önceki dosyalarda karşılaştırma için korunur. Runtime execution/instance/system halen game-owned adaptördür; build/test taşıma sonunda yapılacak. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 2D | `sas::AbilityRuntimeState` level, input edge, active state, cooldown, active duration ve charge state'ini; clamp, activation begin/end, cooldown/duration tick ve cooldown reduction operasyonlarıyla birlikte SAS'a taşır. `AbilityInstance` owner-tag, behavior, weapon, attachment ve notification adaptörü olarak bu state'i compose eder. | Uygulandı; toplu doğrulama bekliyor | Önceki dağınık state alanları `gameplay/ability/legacy/AbilityRuntimeState.h` içinde karşılaştırma için korunur. Davranış formülü değiştirilmedi; build/test tüm taşıma sonunda yapılacak. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 2E | Ability runtime altyapısı grup halinde ayrıldı: generic `sas::FactoryRegistry`, tekrarlı action zamanlaması için `sas::AbilityActionScheduler`, event-trigger süreleri için `sas::AbilityCooldownTracker` ve handle/instance/ID/slot/passive indekslerini sahiplenen `sas::AbilityCollection` eklendi. Oyun `AbilityBehaviorRegistry`, `AbilityExecutor` ve `AbilitySystem` bu mekanizmaları kullanır. | Uygulandı; toplu doğrulama bekliyor | Weapon lifecycle, action payload dispatch, actor spawn, DamageContext, attachment ve presentation LightYearsGame adaptöründe kalır. Önceki registry/execution/storage şekilleri `gameplay/ability/legacy` altında korunur; build/test taşıma tamamlanınca çalıştırılacak. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 3A | Effect handle, duration/stack policy, immutable definition, resolved spec gövdesi, generic definition validation, runtime snapshot ve mutable duration/stack/modifier-handle/runtime-attribute state `sas/effects` altına taşındı. `ActiveGameplayEffect` SAS runtime state'ini genişletir; oyun spec'i source ability upgrade bilgisini ekler. | Uygulandı; toplu doğrulama bekliyor | Behavior hook registry, DamageContext, Actor/source context, visual pointer/registry ve shipped content LightYearsGame'de kalır. Eski definition/spec/active state şekilleri `gameplay/effects/legacy` altında korunur; lifecycle container taşıması sonraki Effect grubudur. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 3B | `sas::GameplayEffectCollection<ActiveEffect>` active-effect value storage, handle allocation, handle lookup ve index erase/reset mekaniklerini SAS'a taşır. `GameplayEffectSystem` ayrı vector ve `mNextHandleId` yerine bu collection'ı compose eder. | Uygulandı; toplu doğrulama bekliyor | Modifier/tag/behavior/visual cleanup sırası ve stacking eşleşme kuralı oyun adaptöründe kalır. Eski list+sayaç storage şekli `gameplay/effects/legacy/GameplayEffectSystemStorage.h` içinde korunur. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 3C | `sas::GameplayEffectBehaviorRegistry<Hooks>` typed hook depolamasını; `GameplayEffectBindings` application tag gate, instant base modifier, active modifier handle ve granted-tag bağlama/sökme mekaniklerini SAS'a taşır. | Uygulandı; toplu doğrulama bekliyor | Hook callback imzalarındaki Actor/DamageContext ve refresh/stack/remove/visual çağrı sırası LightYearsGame adaptöründe kalır. Önceki hook map ve system helper sahipliği legacy dosyalarda korunur. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 2F | `sas::AbilityLifecycleOrchestrator`, input activation/toggle/release, duration expiry ve instant completion kararlarını SAS'a taşır. Aktif oyun kodundaki Ability handle/policy/snapshot geçiş alias'ları kaldırıldı; `GameAbilityDefinition` oyun uzantısı olarak açıkça kullanılır. | Uygulandı ve doğrulandı | Behavior, action payload, weapon, actor, attachment, damage ve presentation dispatch'i bilinçli olarak LightYearsGame adaptöründe kalır. |
| 2026-07-29 | SpaceAbilitySystem kütüphane ayrımı — Aşama 3D | `sas::GameplayEffectLifecycleOrchestrator`, instant/active ayrımı, source-scope stacking hedefi, refresh/stack kararı, duration refresh uygunluğu ve expiry tick kararlarını SAS'a taşır. Aktif Effect handle/policy/snapshot alias'ları kaldırıldı. | Uygulandı ve doğrulandı | Actor/DamageContext hook çağrıları, modifier/tag bağlama sırası ve visual spawn/cleanup oyun adaptöründe kalır. |
| 2026-07-29 | SpaceAbilitySystem geçiş kapanışı | Eski Attribute/Ability/Effect şekilleri geçiş denetimi için ilgili `legacy/` klasörlerine alınmış ve aktif CMake hedeflerinden çıkarılmıştı. Aktif oyun kaynakları SAS tiplerini `sas::` ile açık kullanır. Debug configure/build ile `SpaceAbilitySystem`, `LightYearsGame` ve `LightYearsGasLiteTests` başarıyla üretildi; CTest `LightYearsGasLiteCore` ve `LightYearsEngineLifetime` testlerini 2/2 geçirdi. | Tamamlandı | Statik kütüphane sınırı tamamlandı. Gelecekte mod/script desteği seçilirse mevcut SAS çekirdeğinin üzerine ayrı adapter/runtime katmanı eklenebilir. |
| 2026-07-30 | SpaceAbilitySystem legacy temizliği | Aktif include, kaynak, CMake veya runtime bağlantısı bulunmadığı doğrulanan Attribute/Ability/Effect karşılaştırma kopyaları kaldırıldı: 21 dosya ve altı boş `legacy` klasörü silindi. | Tamamlandı ve doğrulandı | Aktif sistem yalnız `SpaceAbilitySystem` çekirdeğini ve `LightYearsGame` oyun adaptörlerini kullanır. Silme sonrası Debug ve Release konfigürasyonlarında `SpaceAbilitySystem`, `LightYearsGame` ve `LightYearsGasLiteTests` üretildi; CTest iki konfigürasyonda da 2/2 geçti. |
| 2026-07-30 | SpaceAbilitySystem sahiplik denetimi — runtime sınırı | LightYearsGame Attribute/Ability/Effect dosyaları header ve implementasyon seviyesinde MCP bağımlılık grafiğiyle yeniden sınıflandırıldı. Generic `AbilityBehavior`, `AbilityRuntimeEntry`/snapshot, grant/trigger kuralları ve attribute scaling/list yardımcıları SAS'a; Effect runtime entry/context/source-scope, behavior result, catalog doğrulama ve collection lookup mekanizmaları SAS'a taşındı. Game-owned `AbilityBehavior.cpp` kaldırıldı. | Tamamlandı ve doğrulandı | Attribute tarafında yalnız ship/common ID content kataloğu oyunda kalır. AbilitySystem/Instance/Executor ile EffectSystem/behavior/validation dosyalarında kalan gövdeler Actor, weapon, attachment, DamageContext, presentation veya shipped content bağımlılıkları nedeniyle oyun adaptörleridir. Debug/Release build ve CTest iki konfigürasyonda da 2/2 geçti. |
| 2026-07-30 | SpaceAbilitySystem sahiplik denetimi — component/runtime orchestration | “Actor veya World kullanıyorsa game’de kalır” ölçütü kaldırıldı. `sas::GameplayEffectRuntimeSystem` apply/instant/stack/refresh/tick/expiry/remove, modifier/tag cleanup ve active storage akışını; `sas::GameplayEffectBehaviorRuntime` typed behavior dispatch’ini sahiplenir. `ActiveGameplayEffect` SAS runtime entry alias’ına indirildi ve visual handle-sidecar olarak game presentation adaptörüne ayrıldı. `sas::AbilityRuntimeSystem` grant/remove/slot/passive/tick/clear/snapshot akışını; `sas::AbilityExecution` ve `AbilityExecutionLifecycle` action container ile phase orchestration’ını sahiplenir. | Tamamlandı ve doğrulandı | LightYearsGame’de kalan `AbilitySystem`, `GameplayEffectSystem` ve behavior başlıkları engine/content callback adaptörleridir. FireWeapon, SpawnActor, ApplyEffect dispatch’i; DamageContext phase’i; Barrier/Gravity davranışları ve presentation game-owned kalır. Behavior registry `.cpp` tekrarları kaldırıldı. Debug ve Release build başarılı; CTest iki konfigürasyonda da 2/2 geçti. |
| 2026-07-30 | SpaceAbilitySystem sahiplik düzeltmesi — ability instance/event/registry | `sas::GameplayAbilityInstance` input, activation/end, cooldown/duration tick, level ve snapshot yaşam döngüsünü; `sas::AbilityBehaviorRegistry` typed factory storage'ını sahiplenir. `sas::AbilityEvent` Actor/DamageContext bağımlılığı olmadan typed opaque source/target/context taşır. Eski `GameAbilityInstance`, `GameAbilityBehaviorRegistry`, `GameAbilityEvent` ve `GameAbilityExecution` dosyaları kaldırıldı. | Uygulandı; toplu doğrulama bekliyor | Game tarafında `GameAbilityDefinition`, shipped content ve Actor/weapon/attachment/action uygulamaları kalır. Kullanıcı kararı gereği build/test tüm taşıma sonunda çalıştırılacak. |
| 2026-07-30 | Unreal-benzeri tek component erişimi | `LightYearsAbilitySystemComponent`, doğrudan `sas::AbilitySystemComponent` türevi yapıldı. `CombatRuntime` ayrı core component + facade taşımayı bıraktı; yalnız bu component'a sahip. `GameAbilitySystemBinding`, `ConfiguredGameAbility` ve behavior/action binding başlıkları kaldırıldı. Behavior context ve game action runtime tek `GameAbility` content sınıfında toplandı. | Uygulandı; toplu doğrulama bekliyor | Actor ve tüm combat kullanıcıları ability, effect, attribute, tag ve event API'sine `GetAbilitySystemComponent()` üzerinden ulaşır. Build/test taşıma sonunda yapılacak. |
| 2026-07-30 | Component-owned game content kurulumu | Ability/effect bootstrap, game effect context tipleri ve game-specific effect validation `LightYearsAbilitySystemComponent` içine alındı. `GameAbilityContentRegistration`, `GameEffectBehaviorTypes`, `GameGameplayEffectContent` ve `GameGameplayEffectValidation` dosyaları kaldırıldı. | Uygulandı; toplu doğrulama bekliyor | `GameAbilityDefinition` gerçek weapon/action/level/UI içeriği olduğu için oyunda kalır. Uygulama başlangıcı tek `RegisterGameContent()` çağrısı yapar. |
| 2026-07-30 | SAS component erişim interface'i | SAS-owned `AbilitySystemInterface`, owner nesnelerinin ilişkili `sas::AbilitySystemComponent` örneğini `GetAbilitySystemComponent()` üzerinden sunması için eklendi. `Combatant` bu dar kontratı uygular; mevcut component sahipliği `CombatRuntime` içinde değişmeden kalır. | Uygulandı; toplu doğrulama bekliyor | Yeni registry veya binding katmanı eklenmedi. Mevcut combat çağrıları aynı erişim fonksiyonunu kullanmaya devam eder. |
| 2026-07-30 | Component ve SAS gereksiz yüzey denetimi | `LightYearsAbilitySystemComponent` self-pointer'ı, ikinci runtime referansı, yeniden-adlandırma wrapper'ları ve effect hook alias/çevirici struct'ı kaldırıldı. Ability notification, catalog null/duplicate kontrolü ve toplu cooldown azaltma SAS'a taşındı. Effect attribute/tag zorunlu sahipliği reference yapıldı; tekrarlanan initialize/refresh callback'leri ve çağrılmayan ham runtime/active-list getter'ları kaldırıldı. | Tamamlandı ve doğrulandı | Game tarafında yalnız owner/content/attachment ile `Actor`/`DamageContext` effect specialization'ı kaldı. Debug/Release build başarılı; CTest iki konfigürasyonda da 2/2 geçti. |
| 2026-07-30 | Statik link ve son entegrasyon doğrulaması | `SpaceAbilitySystem` CMake `STATIC` hedefi, `LightYearsGame` ve `LightYearsGasLiteTests` link zinciri doğrulandı. Eski `CombatRuntime::GetAttributes/GetAbilities`, self `GetComponent`, doğrudan AbilityEvent source/target alanı ve eksik SAS runtime-context namespace/include kullanımları yeni component/typed API'ye geçirildi. | Tamamlandı ve doğrulandı | Debug ve Release `SpaceAbilitySystem.lib`, `LightYearsGame.exe` ve `LightYearsGasLiteTests.exe` üretildi. `LightYearsGasLiteCore` ile `LightYearsEngineLifetime` iki konfigürasyonda da 2/2 geçti. |
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
