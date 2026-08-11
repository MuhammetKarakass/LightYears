# LightYears — Mevcut Uygulama Kataloğu

Bu belge, şu anda kaynak kodunda bulunan sistemlerin ve content tanımlarının
doldurulabilir envanteridir. Bir satırdaki “Mevcut değer” koddan gelir.
“Hedef / karar” ve “Test sonucu” kolonları ekip tarafından değiştirilebilir.

Ana matematik açıklamaları için
[Project Documentation](PROJECT_DOCUMENTATION.md), gelecek planı için
[Balance & Roadmap Notebook](BALANCE_AND_ROADMAP_NOTEBOOK.md) kullanılır.

## 1. Durum ve düzenleme rehberi

| Durum | Anlamı | Ne yapılabilir? |
| --- | --- | --- |
| ✅ Uygulandı | Runtime ve config/çağrı yolu mevcut | Değeri değiştir, test satırı ve karar günlüğünü doldur |
| 🟡 İçerik mevcut | Kod/config var, ancak mevcut uygulama başlangıcında doğrudan kullanılmıyor | Content akışına bağla veya playtest sahnesinde etkinleştir |
| ⚙️ Altyapı hazır | Generic runtime/API var; otomatik drop, seçim veya UI akışı olmayabilir | Yeni content eklemeden önce capability ve çağrı yolunu kontrol et |
| 🧪 Ölçüm bekliyor | Çalışma durumu değil; sayısal denge ölçümü kayda geçmemiş | Hedef metrik ve ölçülen sonucu yaz |

### Her değişiklikte doldurulacak minimum kayıt

| Alan | Yazılacak bilgi |
| --- | --- |
| Mevcut değer | Değişiklikten önce config/runtime değeri |
| Hedef değer | Yapmak istediğin yeni sayı veya davranış |
| Sebep | Oyuncu deneyimi / build çeşitliliği / bug / performans |
| Test | Hedef, level, attachment, arena ve ölçüm süresi |
| Sonuç | Ölçülen değer ve kabul/red kararı |
| Kaynak | Değiştirilen dosya ve tanım ID’si |

## 2. Mevcut başlangıç akışı

| Akış noktası | Mevcut durum | Kaynak | Değiştirilebilir not |
| --- | --- | --- | --- |
| Uygulama başlangıç dünyası | ✅ ArenaTestLevel yükleniyor | LightYearsGame/src/gameFramework/GameApplication.cpp | Başlangıç test arena; normal LevelOne varsayılan boot yolu değil |
| Ana menüden oyun | 🟡 MainMenuLevel, StartGame ile LevelOne yükler | LightYearsGame/src/level/MainMenuLevel.cpp | Başlangıç dünyası MainMenuLevel olursa erişilir |
| Oyuncu spawnı | ✅ Player, Ship_Player_Fighter ile spawn olur | LightYearsGame/src/player/Player.cpp | Player ship ID / respawn politikasına göre değiştirilebilir |
| Başlangıç loadout | ✅ BasicRapidLaser + GravityAnomaly + NullPulse + Dash + OverdriveCore | LightYearsGame/include/gameConfigs/ship/ShipConfig.h; LightYearsGame/src/player/PlayerSpaceShip.cpp | Null Pulse varsayılan Ability2/E grant'idir; InfernoSpray shipped/cataloguedır ancak default grant listesinde değildir |
| İlk hasar koruması | ✅ Oyuncu 2 sn invulnerable başlar | LightYearsGame/src/player/PlayerSpaceShip.cpp | mInvulnerabilityTime ile değiştirilebilir |

## 3. Sistem envanteri

| Sistem | Durum | Çalışan davranış | Kaynak / düzenleme noktası | Denge veya test notu |
| --- | --- | --- | --- | --- |
| Ability.Control.NullPulse.Basic | ✅ | Evet, varsayılan Ability2 / E | 11 sn cooldown; Instant; 1 charge; self-centered radius 500 (+200); JSON'da `settings` boş, runtime attribute'ları `attributes` bölümünde | Energy damage bir kez uygulanır; `IsProjectileActor()` işaretli actor'lar temizlenir. Normal/elite/miniboss Stun, boss kısa Stagger/interrupt alır. L1 damage 10, her level +2; cooldown her level -0.25; stun `1.0 + 0.65×(1-exp(-BonusEnergyMax/50))` ile en fazla 1.65 sn; ability level stun/radius/cleanup kuralını değiştirmez; crit yok | [x] Content loader, behavior validation, projectile/persistent actor ayrımı, hasar, Stun/Stagger effect, EnergyMax formülü, typed profile/visual self-cleanup ve CTest |
| SpaceAbilitySystem statik kütüphanesi | ✅ Doğrulandı | Attribute, Ability ve Effect generic çekirdeği ile lifecycle kararları `sas` namespace'inde; engine/content/presentation entegrasyonları oyun adaptörlerinde | SpaceAbilitySystem/CMakeLists.txt; SpaceAbilitySystem/include/{attributes,abilities,effects}; SpaceAbilitySystem/src/{attributes,abilities,effects} | Debug/Release `SpaceAbilitySystem.lib`, oyun ve GAS test executable'ları üretildi; CTest iki konfigürasyonda 2/2 geçti |
| sas::AttributeSystem | ✅ Doğrulandı | `GameplayAttribute`, modifier, scaling rule, lookup map, handle map ve delegate kimlikleri `sas::AttributeId` kullanır; Add, Multiply, Override, min/max clamp ve scaling sırası korunur | SpaceAbilitySystem/include/attributes/AttributeSystem.h; SpaceAbilitySystem/include/attributes/GameplayAttribute.h; SpaceAbilitySystem/src/attributes/AttributeSystem.cpp | Davranış test edilmedi; test çalıştırılmadı |
| sas::AttributeId | Uygulandı | Numeric gameplay attribute kimliği için string-backed, opaque API; equality, validity ve hash desteği; SAS lookup/delegate/spec mutator yolları ve game loader'ları kullanır | SpaceAbilitySystem/include/attributes/AttributeId.h; SpaceAbilitySystem/include/attributes/GameplayAttribute.h; SpaceAbilitySystem/include/effects/GameplayEffectSpec.h | Lookup API'leri `FindAttribute`, `FindAttributeValue`, `HasAttribute`; test çalıştırılmadı |
| ly::AttributeIdSchema | Uygulandı | Canonical AttributeId biçimi, legacy `Attribute.` prefix reddi ve namespace sınırı yardımcıları; shipped ID kataloğu tutmaz | LightYearsGame/include/gameplay/attributes/AttributeIdSchema.h | Feature/catalog sahipliği korunur; test çalıştırılmadı |
| sas::AttributeMath | ✅ Doğrulandı | Crit, luck, armor için doygun üstel eğriler | SpaceAbilitySystem/include/attributes/AttributeMath.h | Rating/cooldown/movement regresyonları core test içinde geçti |
| Oyun attribute ID kataloğu | Uygulandı | Owner, ship, ortak, weapon, damage, effect, attachment ve ability actor numeric katalog sabitleri `sas::AttributeId`; canonical format `Owner.*`, `Common.*`, `AbilityActor.*` vb. | LightYearsGame/include/gameplay/attributes/AttributeIds.h; gameConfigs/*; gameplay/ability/*Contracts.h | Semantic audit tamamlandı; `CurrentRuntimeValue` semantic runtime key ve `MinCancelDuration` numeric setting olarak ayrık kaldı |
| Proje geneli gameplay tag sözleşmesi | ✅ | Domain kökleri/biçim doğrulaması ile iki ortak action lock; numeric `Attribute.*` tag biçimi reddedilir, feature leaf'leri yerel kalır | LightYearsGame/include/gameplay/tags/GameplayTagSchema.h; ability/effect/actor/weapon/attachment ve ship progression validation | Attribute API rename tamamlandı; semantic attribute bilgisi schema'ya taşınmadı; test çalıştırılmadı |
| SAS Ability temel sözleşmeleri | ✅ Doğrulandı | `sas::AbilityHandle` ve slot/lifecycle/action/targeting policy enum'ları; aktif kod açık `sas::` tiplerini kullanır | SpaceAbilitySystem/include/abilities/{AbilityHandle,AbilityPolicies,AbilityTypes}.h | Legacy karşılaştırma dosyaları doğrulama sonrası silindi |
| sas::AbilityRuntimeSnapshot | ✅ Doğrulandı | Game-owned definition pointer'ı olmadan ID, slot, level, active/cooldown/duration/charge durumunu taşır | SpaceAbilitySystem/include/abilities/AbilityRuntimeSnapshot.h | Eski pointer tabanlı karşılaştırma kaldırıldı |
| sas::AbilityDefinition + validation | ✅ Doğrulandı | Kimlik, slot/lifecycle, owner tag koşulları ve generic attribute/scaling verisi; ID/sayı/duration/passive ile catalog null/duplicate doğrulaması | SpaceAbilitySystem/include/abilities/{AbilityDefinition,AbilityDefinitionValidation}.h; SpaceAbilitySystem/src/abilities/AbilityDefinitionValidation.cpp | Action/weapon/UI/content doğrulaması LightYearsGame'de kalır; catalog testleri Debug/Release geçti |
| sas::AbilityEvent | ✅ Doğrulandı | Tag/magnitude ile typed opaque source/target/context referanslarını SAS taşır; Actor ve `DamageContext` oyun binding'i tarafından bağlanır | SpaceAbilitySystem/include/abilities/AbilityEvent.h | Oyun çağrıları typed `Set/GetSource`, `Set/GetTarget` ve context API'sine geçirildi |
| sas::GameplayAbilityInstance + runtime state | ✅ Doğrulandı | Input/activation/end/level/cooldown/duration/charge/snapshot yaşam döngüsü SAS'a aittir | SpaceAbilitySystem/include/abilities/{GameplayAbilityInstance,AbilityRuntimeState}.h | Game tarafındaki `GameAbility` yalnız concrete behavior/action, weapon ve attachment içeriğidir; lifecycle testleri geçti |
| LightYearsAbilitySystemComponent | ✅ Doğrulandı | `sas::AbilitySystemComponent` türevi tek oyun component'ı; Actor erişimi SAS-owned `AbilitySystemInterface::GetAbilitySystemComponent()` kontratından yapılır. Game subclass yalnız owner, shipped content/validation, attachment endpoint'leri ve `Actor`/`DamageContext` effect specialization'ını taşır | SpaceAbilitySystem/include/{AbilitySystemInterface,AbilitySystemComponent}.h; LightYearsGame/include/gameplay/ability/LightYearsAbilitySystemComponent.h | Self-facade ve eski CombatRuntime erişimleri olmadan oyun/test hedefleri linklendi |
| SAS Ability runtime mekanikleri | ✅ Doğrulandı | `AbilityBehaviorRegistry`, `GameplayAbilityInstance`, `AbilityRuntimeSystem`, `AbilityExecution`, scheduler/trigger, component notification zinciri ve toplu cooldown azaltma framework sahipliğindedir | SpaceAbilitySystem/include/abilities; SpaceAbilitySystem/include/AbilitySystemComponent.h; SpaceAbilitySystem/src/AbilitySystemComponent.cpp | Debug/Release build ve GAS runtime testleri geçti |
| SAS Effect çekirdeği | ✅ Doğrulandı | Handle/policy/definition/spec/validation/state yanında `GameplayEffectRuntimeSystem` tam active-effect lifecycle’ını, `GameplayEffectBehaviorRuntime` typed hook dispatch’ini sahiplenir | SpaceAbilitySystem/include/effects; SpaceAbilitySystem/src/effects | Typed runtime context namespace/include bağlantıları düzeltildi; Debug/Release effect testleri geçti |
| sas::GameplayEffectCollection | ✅ Doğrulandı | Active-effect storage, handle allocator/lookup/index lookup, predicate lookup, index erase ve reset | SpaceAbilitySystem/include/effects/GameplayEffectCollection.h | Lifecycle policy kararları SAS orchestrator'ında; callback/visual cleanup adaptörü oyunda |
| SAS Effect registry, bindings ve lifecycle | ✅ Doğrulandı | Typed hook storage; tag gate; modifier/tag binding; instant/refresh/stack/duration kararları; zorunlu attribute/tag sahipliği reference kontratıyla tutulur | SpaceAbilitySystem/include/effects; SpaceAbilitySystem/src/effects | Actor/DamageContext callback'leri ve visual orchestration game-owned; Debug/Release lifecycle testleri geçti |
| Oyun ability definition/content sınırı | ✅ Tamamlandı | `ly::GameAbilityDefinition`, SAS definition tabanını action/trigger/level, silah, damage/attachment, behavior ve UI alanlarıyla genişletir | LightYearsGame/include/gameplay/ability/content/GameAbilityDefinition.h | Geçiş alias'ı ve eski `AbilityStructs.h` kaldırıldı |
| Engine diagnostics | ✅ | CORE/GAME kanallı structured logging, Debug assert/verify ve scope/counter profiler | LightYearsEngine/include/framework/debug/*; src/framework/debug/*; EntryPoint.cpp | Debug lifetime testleri log/filter/verify/profile kontratını doğrular; Release'te assert/profiling compile-out, logging Warning+ kalır |
| CombatRuntime | ✅ | Attribute, effect, ability ve incoming damage akışını birleştirir | gameplay/combat/CombatRuntime.* | Damage sırası §6’da kayıtlı |
| DamageTypeSystem | ✅ | 6 tür tag’i, payload override ve status uygulama | gameplay/damage/DamageTypeSystem.* | Hibrit tag önceliği tasarım kararı gerektirir |
| HealthComponent | ✅ | Health clamp, damage/heal eventleri | gameplay/HealthComponent.* | Max health değişince yüzde koruma açık |
| ShieldComponent | ✅ | Kalıcı gemi shield’ı, gecikme ve regen | gameplay/ShieldComponent.* | Energy hasarı shield kapasitesini daha hızlı tüketir |
| EnergyComponent | ✅ | Afterburner enerjisi, gecikme ve regen | gameplay/EnergyComponent.* | Consume ve recharge aynı frame davranışı test edilebilir |
| ShipRuntime | ✅ | Ship attribute'ları ile shield/afterburner türetmelerini owner attribute'lardan çözer | gameplay/ship/ShipRuntime.* | SpaceShip sahiplenir; CombatRuntime'dan ayrı tutulur |
| GameplayEffectSystem game adaptörü | ✅ Doğrulandı | SAS runtime component’ini Actor event, incoming `DamageContext` ve presentation sidecar callback’lerine bağlar | gameplay/effects/GameplayEffectSystem.* | Active storage/lifecycle bu sınıfta değil SAS’tadır |
| AbilitySystem game adaptörü | ✅ Doğrulandı | SAS runtime component’ini Actor, trigger execution, attachment ve game delegate’lerine bağlar | gameplay/ability/AbilitySystem.* | Grant/remove/slot/tick/snapshot sahipliği SAS’tadır |
| AbilityBehaviorRegistry adaptörü | ✅ Doğrulandı | SAS `FactoryRegistry`’yi game behavior ID’lerine bağlayan header-only typed adaptör | gameplay/ability/AbilityBehaviorRegistry.h | Tekrarlayan game `.cpp` kaldırıldı; somut ability kayıtları game-owned |
| Shared ability actors | ✅ | Ortak world actor registry, lifecycle ve alan telegraph altyapısı | gameplay/ability/actors/* | Yalnız bir ability'ye özgü actor kendi aile klasöründe kalır |
| AttachmentLoadout | ⚙️ | Host/capability doğrular, static/conditional modifier ve event uygular | gameplay/attachment/* | Katalog var; varsayılan oyuncuya otomatik attachment verilmiyor |
| PrimaryWeaponExecution | ✅ | Type handler, runtime state, interval fire ve feature yönetimi | gameplay/weapon/PrimaryWeaponExecutionSystem.* | Handler validator’ı yeni config için zorunlu |
| Weapon handlers | ✅ | Standard, Shotgun, Electric Arc, Continuous Beam, Expanding Wave | gameplay/weapon/handlers/* | Tümü registry’de built-in olarak kayıtlı |
| Heat feature | ✅ | Heat gain curve, dissipation, cooldown ve runtime heat değeri | gameplay/weapon/features/HeatWeaponFeatureHandler.cpp | Beam sustained DPS test edilmeli |
| ShipProgression | ✅ | XP, level ve attribute modifier yeniden kurma | gameplay/progression/ShipProgression.* | Wave XP temposu playtest ile ölçülmeli |
| ThrustDrift movement | ✅ | Thrust, strafe, damping, aim ve afterburner | spaceShip/SpaceShip.cpp; player/PlayerSpaceShip.cpp | ArenaTest oyuncuyu bu moda geçirir |
| CameraManager | ✅ | Follow, cursor/velocity look-ahead, speed zoom, mutlak/göreli ek zoom, bounds ve shake; Dash impulse speed zoom’dan ayrılır ve follow offset korunur | framework/camera/CameraManager.*; level/ArenaLevel.cpp | Her arena preset’i ölçülmeli |
| ArenaBoundarySystem | ✅ | Margin dışı sayaç, HUD warning ve ölüm cezası | level/ArenaBoundarySystem.* | Grace time ve ceza hedef mücadele temposuna göre ayarlanır |
| ArenaTestLevel | ✅ | 6000×3000 arena, respawn ve kamera girdileri | level/ArenaTestLevel.cpp | Şu an uygulamanın başlangıç seviyesi |
| LevelOne akışı | 🟡 | Normal stages, boss ve infinite stage zinciri | level/LevelOne.cpp | Başlangıç app akışında doğrudan yüklenmiyor |
| Boss fazları | 🟡 | HP eşiklerinde silah ve hareket değişimi | enemy/LevelOneBoss.cpp | LevelOne açıldığında etkin |
| HUD / warning | ✅ | Gameplay HUD, shield/effect görünümü, arena uyarısı | widget/*; presentation/* | Tooltip’te resolved stat gösterimi ayrı iş |
| Otomatik test runner | ✅ | Core gameplay sistemleri için GasLiteCoreTests | LightYearsGame/tests/GasLiteCoreTests.cpp | SAS sınır doğrulaması sonunda Debug/Release çalıştırılır |

## 4. Aktif ability ve effect içeriği

### Content kaynak durumu

| Konu | Mevcut durum | Planlanan sınır |
| --- | --- | --- |
| Otoriter shipped content | Weapon/ability sayısal değerleri owner JSON'da; `effects.json` yalnız policy/contract; player gemisi JSON'da | Düşman/boss silahları, attachment runtime ve diğer content türleri daha sonra bağlanacak |
| JSON / CSV runtime loader | ✅ JSON loader + domain loader/catalog + temel semantic validation | CSV yalnızca balance import/export ve analiz için kullanılacak |
| C++'da kalacak alanlar | Şema, tag, behavior/action/weapon handler, typed presentation ve runtime state | Aynı sahiplik korunacak; JSON yalnız kayıtlı C++ ID'lerine referans verecek |
| CSV kullanımı | ❌ Yok | Balance import/export ve analiz; doğrudan oyun runtime kaynağı değil |

Bu tablo mevcut runtime durumunu açıklar. JSON kaynağı yüklendiğinde ilgili
catalog yalnızca JSON kayıtlarını içerir. C++ fallback tanımları yeni kayıt
eklemek için kullanılmaz; loader bunları yalnızca JSON kaydının eşleşen
behavior/action/actor/presentation iskeletini çözmek için kullanır. JSON
yükleme başarısızsa catalog başarısız kalır ve uygulama oyun dünyasını başlatmaz.

Effect sahiplik kuralı: `sourceParameterized` effect kayıtları sayısal alan
taşıyamaz. Weapon status değerleri `Damage.*`, ability effect
değerleri `effectSpecs`, paylaşılan actor değerleri ability-local
`attributeProfiles` üzerinden runtime `GameplayEffectSpec` üretir. Damage tag
yalnız davranış seçer; sayı üretmez. Loader lint ve catalog validation eksik ya
da çift kaynaklı kayıtları başlangıçta reddeder.

Startup strictness: `GameContentBootstrap::Register()` herhangi bir JSON catalog'u
yüklenemezse `false` döner ve `GameApplication` `QuitApplication()` çağırarak
oyun dünyasını yüklemeden kapanır. Effect C++ fallback kayıtları yalnız JSON
kaydının behavior/policy iskeletini parse etmeye yarar; JSON'da olmayan effect'i
katalogda otomatik olarak yaşatmaz.

Ability varyantları `baseId` ile JSON içindeki başka bir ability kaydından
türeyebilir. Object alanları recursive merge edilir, array alanları varyantta
verilmişse tamamen değiştirilir. Value-only varyantlar base actor listesini
çoğaltmaz; behavior ve actor/presentation bağlantısı base ability'den gelir.
Yeni actor/presentation gerektiren yapısal evolve'lar ayrı C++ actor/profile/
handler ve benzersiz actor ID'si gerektirir.

### Ability klasör kontratı

| Katman | İçerik | Kural |
| --- | --- | --- |
| `gameplay/ability/` | AbilitySystem/Instance adaptörü, behavior kaydı, executor ve DamageContext event uzantısı | Actor, weapon, attachment, damage, presentation ve shipped content entegrasyonu |
| `gameplay/ability/actors/` | AbilityWorldActor, AbilityActorRegistry, AreaTelegraphActor | Ability ID/config/hasar kuralı bilmeyen, yeniden kullanılabilir world-actor altyapısı |
| `gameplay/ability/dash/` | DashAbility, DashMovementController, DashMovementMath | Dash'e özgü behavior, hareket kontratı ve evolve parçaları |
| `gameplay/ability/gravityAnomaly/` | GravityAnomalyAbility, ProjectileActor, FieldActor | Gravity Anomaly validation, cursor delivery, source-scoped field lifecycle ve gelecek evolve parçaları |
| `gameplay/ability/nullPulse/` | NullPulseAbility, NullPulseTargetQuery, NullPulseVisualActor | Self-centered projectile cleanup, enemy damage/control response ve feature-local pulse presentation |
| `gameplay/ability/rocket/` | RocketAbility, RocketProjectileActor | Rocket'e özgü validation, projectile delivery, patlama ve gelecek evolve parçaları |
| `gameplay/ability/shield/` | ShieldAbility | Shield'e özgü behavior |
| `gameplay/ability/sunBeam/` | SunBeamAbility, actor ve visual sınıfları | SunBeam'e özgü tüm runtime parçaları |
| `gameplay/ability/content/` | GameAbilityDefinition | SAS-owned temel tipleri kullanan game-owned definition, action payload ve UI metadata |
| `gameConfigs/ability/` | AbilityActorStructs, AbilityCatalog ve kategoriye ayrılmış aile config'leri | `functional/DashConfig`, `defensive/ShieldConfig`, `offensive/{SunBeam,GravityAnomaly,Rocket,InfernoSpray}Config`; bu config'ler yalnız ID/tag, behavior/action, actor type, presentation ve schema kontratını taşır; sayısal tuning JSON'dadır |
| `presentation/ability/<family>/` | Stable presentation ID, concrete typed profile ve shipped content registration | Her aile kendi visual/telegraph/explosion paketini sahiplenir; global visual config yok |
| `presentation/ability/common/` | AreaTelegraphVisualDefinition gibi gerçekten paylaşılan primitive'ler | Benzer alanlar tek başına ortaklaştırma gerekçesi değildir |

Yeni ability, kendi klasöründe ayrı bir behavior sınıfı ve config alır.
Bağımsız world lifetime/tick/collision gerektirmeyen bir görsel için actor
oluşturulmaz. Evolve yeni davranış getiriyorsa aynı ability ailesi içinde ayrı
behavior/actor olur ve ortak `AreaTelegraphActor` gibi altyapıyı compose eder;
yeni bir top-level klasör katmanı oluşturmaz.

Ability actor config'i yalnızca tek `presentationProfileId` taşır. Handler bu
ID'yi `PresentationProfileRegistry<ConcreteProfile>` üzerinden çözer. Yeni
ability/evolve eklerken `VisualConfig.h`, `AbilityVisualStructs.h`, paralel
visual/telegraph ID alanları veya catch-all visual struct oluşturulmaz. Ayrıntılı
kurallar `PROJECT_DOCUMENTATION.md` içindeki **3.0.1 Ability presentation
profile kontratı** bölümündedir.

### 4.1 Ability kayıtları

| ID | Durum | Oyuncuda varsayılan mı? | Mevcut config | Runtime matematiği | Hedef / test notu |
| --- | --- | --- | --- | --- | --- |
| Ability.Defense.Shield.Basic | ✅ | Hayır; shipped içerik, PlayerSpaceShip varsayılan grant listesinde değil | 8 sn cooldown; 5 sn duration; Basic Barrier uygular | Barrier capacity = 30 + 0.20×MaxHealth + 50×Armor | [ ] Shield uptime ve break event test edilecek |
| Ability.Offense.SunBeam.Strike.Basic | ✅ | Evet, Ability2 / E | 1 sn cooldown; MouseWorld spawn | Base damage 40; radius 96; L2–5 her sefer +8 damage ve +8 radius | [ ] Tek hedef ve area hasarı ölçülecek |
| Ability.Movement.Dash.Basic | ✅ | Evet, Ability3 / F | 2 sn cooldown; 0.24 sn duration; 1 charge; mevcut kamera mesafesine +%15 göreli zoom-out | Base 260 mesafe; mevcut velocity tamamen korunur ve Dash impulse üzerine eklenir; kamera velocity ile follow offset'i korur; göreli zoom kritik sönümlü kamera hattıyla girip çıkar; L2-L5 taban cooldown'un her seferinde %6'sını düşürerek 1.88/1.76/1.64/1.52 olur | [x] Katalog, tuning-safe cooldown, yön, tam momentum, kamera offset/göreli zoom ve zoom-velocity sürekliliği, lifecycle ve cleanup core testleri |
| Ability.Offense.Rocket.Basic | ✅ | Evet, Ability4 / R | 7 sn cooldown; Instant; 1 charge; mouse aim yönünde tek projectile; cursor yakındaysa cursor'da, uzaktaysa 1100 maksimum menzilde patlar | Damage 55 + AttackPower×1.25; Kinetic; radius 55; speed 1000 ve maksimum range 1100 sabit. L2-L15: +4 damage, -0.12 sn cooldown, +1 radius | [x] Katalog/actor validation, cursor hedef mesafesi ve telegraph, max-range clamp, tek spawn, yön, owner scaling, haste, Kinetic AoE tek-vuruş, L1-L15 sabit delivery ve cleanup core testleri |
| Ability.Control.GravityAnomaly.Basic | ✅ | Evet, Ability1 / Q; bu slotta Shield'in varsayılan grant'inin yerini alır | 8 sn cooldown; Instant; 1 charge; 900 cast range, 2000 projectile speed; hedefte 2.5 sn / 220 radius field | Damage yok. Field caster/player/enemy Combatant'larına source-scoped `Effect.GravityAnomaly.Inside.Basic` uygular: içeride 2 sn'ye yenilenen %20 movement slow ve `500×(1-d/radius)^2×dt` velocity pull. Çıkış/field bitiminde pull kesilir, slow 2 sn sürer. MaxHealth yalnız radius (+0.20) ve duration'ı (+0.0025) scale eder. L2-L15: -0.10 cooldown, +0.03 duration, +2 radius, +10 pull, +0.005 slow, +25 speed, +5 range | [x] Typed profile/actor validation, varsayılan Ability1/Q loadout, clamp ve lifecycle, target filtreleme, pull/center güvenliği, gerçek movement slow, 2 sn exit/destroy tail, field-source expiry cleanup, L15/MaxHealth scaling ve effect visual cleanup |
| PrimaryFire üretilmiş ability | ✅ | Evet, Space | Weapon definition’dan slot/action oluşturulur | Level ve scaling weapon profile’dan gelir | [ ] Her silah için ayrı card doldur |
| BossThreeWayBlaster ability | 🟡 | Boss LevelOne içinde | Ability1’e fire weapon olarak atanır | 3 pellet, 60° spread, 0.5 FireRate | [ ] Boss phase test |
| BossFrontalSweep ability | 🟡 | Boss LevelOne içinde | Ability2’ye atanır | 8 muzzle, 0.33 FireRate | [ ] Boss phase 3 test |
| BossLastStageSideBlaster ability | 🟡 | Boss LevelOne içinde | Ability3’e atanır | 2 muzzle, 2 FireRate | [ ] Boss phase 4 test |

### 4.2 Effect ve status kayıtları

| ID / status | Durum | Mevcut değer | Matematik / davranış | Hedef / test notu |
| --- | --- | --- | --- | --- |
| Effect.Barrier.Basic | ✅ | Capacity 30; ratio 1; regen 6/sn; delay 1.5 sn; duration 5 sn | Kaynak hasar emilimi = capacitySpent / (ratio×shieldMultiplier) | [ ] Armor ile etkileşim doğrulanacak |
| Effect.Test.BarrierBreak.ThrustBoost | ✅ | 2 sn; horizontal +0.20; vertical +0.25 | BarrierBroken trigger ile self’e uygulanır | [ ] “Test” ID’si üretim adlandırmasına taşınacak mı? |
| Effect.GravityAnomaly.Inside.Basic | ✅ | 2 sn duration, refresh-duration, source-scoped; field başına %20 slow | `AreaGameplayEffectApplicator` hedef içerideyken süreyi sürekli 2 sn'ye resetler. Çıkışta veya field bitiminde pull hemen kapanır; slow ve target visual kalan 2 sn sürer. Ayrı field source scope'ları birbirini korur. | [x] Multi-field izolasyonu, enter/leave/destroy tail, expiry cleanup, gerçek movement slow ve target-following visual doğrulandı |
| Thermal / Ignite | ✅ | 1 DPS, 3 sn, en fazla 4 stack | Ara stack sadece görünür; 4. stack’te 4 DPS DOT çalışır | [ ] Crit/DOT kuralı kayda geçir |
| Cryo / Buildup + Slow | ✅ | 1 buildup/hit; 4 gereken; 2.5 sn buildup; %25 slow / 1.5 sn | 4. hit birikimi tüketir; aktif slow sonraki Cryo hit’lerinde 1.5 sn’ye yenilenir; `MovementSlow` Add(+0.25) shared gerçek hareket çarpanını kullanır | [ ] Boss ve normal düşman hız testi |
| Electric | ✅ | 0.04 taken-damage / stack; 3 sn; max 4 | Ara stack etkisiz; tam stack incomingDamage × 1.16, armor öncesi | [ ] Max stack burst testi |

## 5. Damage türleri ve mevcut matematik

| Tür | Durum | Varsayılan payload | Kaynak | Değiştirilebilir hedef / test |
| --- | --- | --- | --- | --- |
| Photonic | ✅ | Özel numeric payload yok | DamageTypeSystem.cpp | [ ] Nötr baseline DPS tanımla |
| Energy | ✅ | shieldDamageMultiplier 1.25; regen delay +0.75 sn | DamageTypeSystem.cpp | [ ] Shield karşıtı time-to-break ölç |
| Kinetic | ✅ | armorPenetration 0.10 | DamageTypeSystem.cpp | [ ] Armor eşiği karşılaştır |
| Thermal | ✅ | 1 Ignite/hit, 1 DPS/stack, 3 sn, max 4; yalnızca tam stack hasar verir | EffectConfig.h; DamageTypeSystem.cpp; registered tick hook | [ ] DOT üst üste binme temposu |
| Cryo | ✅ | 1/4 buildup; %25 slow; 1.5 sn; aktifken Cryo hit ile refresh | DamageTypeSystem.cpp | [x] Slow refresh davranışı GasLiteCoreTests’te doğrulandı |
| Electric | ✅ | 1 stack/hit, +%4/stack, 3 sn, max 4; yalnızca tam stack x1.16 | EffectConfig.h; DamageTypeSystem.cpp; registered PreMitigation hook | [ ] Çoklu kaynak stack davranışı |

### Hasar parametre değişiklik tablosu

| Parametre | Mevcut değer / formül | Kaynak | Hedef değer | Ölçülen sonuç | Karar |
| --- | --- | --- | --- | --- | --- |
| Crit rating scale | 100 | AttributeMath.h |  |  |  |
| Crit multiplier | 2.0 varsayılan payload | DamageContext.h |  |  |  |
| Armor scale | 50 / ln(2) ≈ 72.1348 | AttributeMath.h |  |  |  |
| Armor penetration üst sınırı | 0.25 | DamageTypeSystem.cpp |  |  |  |
| Shield damage multiplier | Tür/attribute’a bağlı; Energy 1.25 | DamageTypeSystem.cpp |  |  |  |
| Electric stack çarpanı | 0.04 taban / 0.05 attachment üstü | DamageTypeSystem.cpp |  |  |  |
| Cryo slow üst sınırı | 0.30 | DamageTypeSystem.cpp |  |  |  |

### Pipeline kontrol kartı

| Aşama | Mevcut işlem | Kontrol / not |
| --- | --- | --- |
| 1 | Kaynak crit zarı ve criticalDamageMultiplier | payload.canCrit false ise atlanır |
| 2 | Electric PreMitigation effect | Yalnızca 4/4 stack, damage’i x1.16 büyütür |
| 3 | Barrier effect | Geçici effect kapasitesi harcanır |
| 4 | Armor ve penetration | Hasar azaltımı doygun eğriden gelir |
| 5 | Thermal/Cryo/Electric status üretimi | remainingDamage > 0 ise |
| 6 | Geminin ShieldComponent’i | Kalıcı shield capacity’si harcanır |
| 7 | HealthComponent | Kalan hasar health’e gider |
| 8 | Damage eventleri | DamageTaken, DamageDealt, Ignite attachment eventleri |

## 6. Oyuncu silah kataloğu

| Silah ID | Durum | Varsayılan loadout | Tür / damage type | Mevcut temel değer | Mevcut scaling | Hedef / test notu |
| --- | --- | --- | --- | --- | --- | --- |
| Weapon.Projectile.FighterRapidLaser.Basic | ✅ | Evet | Standard / Photonic | 8 damage, 8 fire rate, 1100 speed, 1600 range, 1 muzzle | Damage +1.0 AttackPower; FireRate +1.0 AttackSpeed | Runtime L1/10/25/50 single-target DPS verified |
| Weapon.Projectile.RapidShotgun.Basic | ⚙️ | Hayır | Shotgun / Thermal | 10 damage, 2.5 fire rate, 3 pellet, 8° spread, 400 range | Damage +0.75 AP; FireRate +0.5 AS | Runtime AP/AS, three-pellet same-target aggregate and Ignite profile verified |
| Weapon.Projectile.DualKineticBlaster.Basic | ⚙️ | Hayır | Standard / Kinetic | 3.5 damage, 12 fire rate, 2 muzzle, 3400 speed, 650 range | Damage +0.45 AP; FireRate +1.0 AS | Runtime two-muzzle DPS verified; L50 remains below Rapid Laser |
| Weapon.Arc.ElectricLauncher.Basic | ⚙️ | Hayır | Arc / Electric | 15 damage, 2.8 fire rate, 850 first range, 3 extra chain, 250 chain range, x0.72 | Damage +0.85 AP; FireRate +0.60 AS; Luck has no direct damage scaling | Runtime 1/2/4/5 target, falloff, no-duplicate and capped bonus-chain chance verified |
| Weapon.Beam.ContinuousHeatLaser.Basic | ⚙️ | Hayır | Continuous Beam / Energy | 28 DPS, 950 range, 26 width, 38 heat/sn, cap 100, cool 25/sn | Damage +0.75 AP +0.50 EnergyMax; AttackSpeed only reduces high-heat gain | Runtime AP/Energy, heat zones, overheat and 10/30s sustained DPS verified |
| Weapon.Wave.CryoProjector.Basic | ⚙️ | Hayır | Expanding Wave / Cryo | 7 damage, 1.8 fire rate, 780 range, 850 speed, width 80→260 | Damage +0.75 AP; FireRate +0.5 AS | Runtime L1/10/25/50 damage/rate and four-hit slow tempo verified |

### Silah progression kaydı

| Silah | Durum | L2 | L3 | L4 | Scrap 2/3/4 | Ölçülen L4 DPS | Karar |
| --- | --- | --- | --- | --- | --- | --- |
| BasicRapidLaser | ✅ | +1 FR | +2 D, +100 speed | +2 D, +1 FR, +100 range | 40 / 50 / 65 |  |  |
| DualKineticBlaster | ✅ | +1.5 FR | +1 D | +100 range, +1 FR | 40 / 50 / 65 |  |  |
| ElectricArcLauncher | ✅ | +4 D | +1 chain, +80 range | +0.4 FR, +0.08 chain factor | 40 / 50 / 65 |  |  |
| ContinuousHeatLaser | ✅ | +6 D | +150 range | +0.20 max-heat multiplier | 40 / 50 / 65 |  |  |
| CryoWaveProjector | ✅ | +2 D, +30 width | +100 range, +0.5 buildup duration | +0.3 FR, +2 D | 40 / 50 / 65 |  |  |

### Silaha özel hesap alanları

| Mekanik | Mevcut matematik | Değer girilecek alan |
| --- | --- | --- |
| Standard projectile | Teorik tek hedef DPS = damage × fireRate × muzzleCount × hitRate | HitRate: ___ ; DPS: ___ |
| Shotgun | sameTargetMultiplier = max(floor, 1 - reduction×(pelletHitCount-1)) | Pellet hit sayısı: ___ ; DPS: ___ |
| Arc | totalDamage = baseDamage × Σ(chainMultiplier^i), i=0..chainCount | Hedef sayısı: ___ ; toplam: ___ |
| Beam | DPS = baseDPS × (1+(maxHeatMultiplier-1)×heatRatio) | Ortalama heat ratio: ___ ; sustained DPS: ___ |
| Wave | Etkin alan, width’in initial→max büyümesi ile değişir | Ortalama hedef/hit: ___ ; DPS: ___ |

### Primary weapon runtime balance verification (2026-07-24)

`GasLiteCoreTests` resolves the production weapon definitions through
`LightYearsAbilitySystemComponent`, `GameAbilityActionExecutor` and
`ShipProgression` at ship levels 1, 10, 25 and 50. Values below
assume weapon ability level 1, no attachment, no crit, no armor and every shot connects.
Shotgun values are three pellets on one target; Beam values are base single-target DPS
before its heat multiplier.

| Weapon | L1 | L10 | L25 | L50 | Contract verified |
| --- | ---: | ---: | ---: | ---: | --- |
| Rapid Laser | 64.00 | 1513.00 | 7168.00 | 25593.00 | Damage x rate, 1 muzzle |
| Rapid Shotgun | 60.00 | 1188.60 | 5985.60 | 22080.60 | 3 x x0.80 same-target pellet aggregate |
| Dual Kinetic | 84.00 | 1677.90 | 7250.40 | 24637.90 | Damage x rate x 2 muzzles; L50 < Rapid Laser |
| Electric Arc (one target) | 42.00 | 687.57 | 3415.92 | 12553.17 | 3 normal chains remain x0.72 falloff |
| Continuous Heat Laser | 28.00 | 93.25 | 202.00 | 383.25 | AttackSpeed does not add fire rate, tick rate or direct DPS |
| Cryo Wave | 12.60 | 426.83 | 2332.20 | 8882.83 | Four-hit slow timing: 1.667 / 0.476 / 0.217 / 0.114 sec |

Electric normal-chain total multipliers are 1.00 (one target), 1.72 (two targets), and
2.611648 (four targets). A fifth valid target can only receive one extra chain after
the normal four-target sequence. Its chance is `0.35 * CombatRuntime::GetCombatLuckFactor()`;
the central diminishing curve approaches, but does not exceed, 35 percent. No eligible
next target means no roll, and the struck-target set prevents duplicates.

For the Beam, AttackSpeed has no heat-gain effect through 50% heat, ramps from 50% to
75%, and applies up to a 35% diminishing-return heat-gain reduction from 75% to 100%.
It neither removes overheat nor changes direct Beam damage/ticks; the suite verifies a
finite overheat cycle and sustained damage over 10 and 30 seconds.

## 7. Düşman ve boss silah kataloğu

| Silah ID | Durum | Damage / FireRate | Muzzle | Önemli davranış | Hedef / test |
| --- | --- | --- | --- | --- | --- |
| EnemyVanguardBlaster | 🟡 | 10 / 1.1 | 1 | Standard, speed 500, range 1900 | [ ] Vanguard TTK |
| EnemyVanguardEliteBlaster | 🟡 | 10 / 1.35 | 1 | Standard, speed 500, range 1900 | [ ] Elite spike |
| EnemyTwinBladeDualBlaster | 🟡 | 10 / 1.0 | 2 | Standard, speed 400 | [ ] Cross-fire overlap |
| EnemyHexagonRadialBlaster | 🟡 | 10 / 1.35 | 6 | Altı yön radial | [ ] Dodge window |
| EnemyUFOTriBlaster | 🟡 | 10 / 1.1 | 3 | 60° / -60° / 180° | [ ] Arena pressure |
| BossBaseDualBlaster | 🟡 | 10 / 2.0 | 2 | Boss primary fire | [ ] Phase 1–4 baseline |
| BossThreeWayBlaster | 🟡 | 10 / 0.5 | 3 pellet | 60° shotgun spread | [ ] Phase 1–4 spread |
| BossFrontalSweep | 🟡 | 10 / 0.33 | 8 | Frontal fan | [ ] Phase 3 dodge |
| BossLastStageSideBlaster | 🟡 | 10 / 2.0 | 2 | Side weapon | [ ] Phase 4 pressure |

## 8. Ship kataloğu

| Gemi ID | Durum | HP | Collision | Score / Ship XP | Silah | Loot olasılıkları | Hedef / test |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Ship_Player_Fighter | ✅ | 100 | 25 | 0 / 0 | Weapon.Projectile.FighterRapidLaser.Basic | Yok | [ ] Varsayılan baseline |
| Ship_Enemy_Vanguard | 🟡 | 60 | 50 | 10 / 10 | EnemyVanguardBlaster | Health .20, Life .05, Shield .08 | [ ] Wave TTK |
| Ship_Enemy_Vanguard_Elite | 🟡 | 200 | 75 | 25 / 25 | EnemyVanguardEliteBlaster | Health .20, Life .01, Shield .08 | [ ] Elite reward |
| Ship_Enemy_TwinBlade | 🟡 | 60 | 50 | 20 / 20 | EnemyTwinBladeDualBlaster | Health .25, Life .05, Shield .08 | [ ] Dual pressure |
| Ship_Enemy_Hexagon | 🟡 | 100 | 60 | 30 / 30 | EnemyHexagonRadialBlaster | Health .30, Life .05, Shield .10 | [ ] Radial safety |
| Ship_Enemy_UFO | 🟡 | 80 | 80 | 40 / 40 | EnemyUFOTriBlaster | Health .30, Life .08, Shield .10 | [ ] Mobility / reward |
| LevelOne Boss | 🟡 | 4000 | 200 | 1000 / 1000 | BossBaseDualBlaster + phase silahları | Yok | [ ] 4 faz TTK |

### Player Fighter — doldurulabilir hareket/enerji kartı

| Parametre | Mevcut değer | Hedef değer | Ölçüm / sonuç |
| --- | --- | --- | --- |
| Forward / reverse / strafe thrust | 650 / 190 / 270 |  |  |
| Max speed / damping | 520 / 0.36 |  |  |
| Turn speed / responsiveness | 400 / 5 |  |  |
| Input responsiveness / mouse dead zone | 12 / 32 |  |  |
| Base shield / full recharge / delay | 100 / 5.5 sn / 4 sn |  |  |
| Afterburner capacity / full recharge / delay | 50 / 8 sn / 1.5 sn |  |  |
| Afterburner speed / acceleration | 1.55 / 1.80 |  |  |
| Energy drain | 16.5 / sn |  |  |
| Ramp up / down / maneuverability | .22 sn / .45 sn / .90 |  |  |
| MaxEnergy → shield / capacity | .5 / 1.0 |  |  |
| XP base / exponent | 100 / 1.25 |  |  |

### Afterburner matematiği

~~~text
requestedEnergy = drainPerSecond * deltaTime
draining = EnergyComponent.Consume(requestedEnergy) > 0
intensityStep = clamp(deltaTime / selectedRampDuration, 0, 1)
intensity -> 1 while draining, otherwise -> 0
speedCapMultiplier = lerp(1, afterburnerSpeedMultiplier, intensity)
accelerationMultiplier = lerp(1, afterburnerAccelerationMultiplier, intensity)
turnCapability = lerp(1, afterburnerManeuverabilityMultiplier, intensity)
~~~

## 9. Attachment kataloğu

Tüm attachment tanımları mevcut ve runtime resolver tarafından desteklenir.
Varsayılan Player Fighter loadout’ında hiçbiri otomatik equip edilmez.

| Attachment | Durum | Host / capability | Mevcut etki | Doldurulacak test |
| --- | --- | --- | --- | --- |
| Thermal Converter | ⚙️ | Ability/Primary; Damage | Thermal ise Damage x1.20; Ignite sonrası non-primary cooldown -0.4 | [ ] Ignite proc döngüsü |
| Energy Coupler | ⚙️ | Ability/Primary; Damage | Energy ise Damage x1.15; shield x1.25, delay +.75 | [ ] Shield break |
| Kinetic Bore | ⚙️ | Ability/Primary; Damage | Kinetic ise +.05 penetration; base 0.10 | [ ] Armor breakpoint |
| Cryo Conduit | ⚙️ | Ability/Primary; Damage | Cryo slowdown +.05; 4-hit, %25 / 1.5 sn profile verir; aktif slow Cryo hit ile refresh olur | [ ] Slow uptime |
| Electric Conduit | ⚙️ | Ability/Primary; Damage | Electric stack multiplier +.01; 3 sn / max 4 | [ ] Burst combo |
| Heavy Capacitor | ⚙️ | Ability; Damage+Cooldown | Damage x1.40, Cooldown x1.25 | [ ] Net value |
| Emergency Salvo | ⚙️ | Primary; FireRate+Projectile | FireRate < 4 ise +1 projectile | [ ] Eşik davranışı |

## 10. Arena, kamera ve level içeriği

| İçerik | Durum | Mevcut değer / davranış | Hedef değer | Test sonucu |
| --- | --- | --- | --- | --- |
| ArenaTest legal bounds | ✅ | 6000 × 3000 |  |  |
| Arena margin / grace | ✅ | 20 / 5 sn |  |  |
| Boundary visual | ✅ | DebugRectangle; cyan normal, kırmızı warning, 4 px |  |  |
| Arena respawn | ✅ | Merkez spawn; 1 sn delay; game over on fail |  |  |
| Kamera base zoom | ✅ | 1.55 |  |  |
| Cursor look-ahead | ✅ | strength .45; dead zone 380; max 600 |  |  |
| Velocity look-ahead | ✅ | strength .75; max 320 |  |  |
| Hız zoom’u | ✅ | speed 500’de +.32 max zoom-out; Dash impulse hariç |  | Dash sırasında son normal velocity korunur |
| Dash follow kompanzasyonu | ✅ | Kamera merkezi her Dash frame’inde target displacement kadar taşınır |  | Kamera–gemi offset regresyon testi geçti |
| Dash göreli zoom | ✅ | Normal speed + afterburner kamera hedefinin üzerine +%15; zoom velocity sürekliliği olan kritik sönümlü smoothing; bounds clamp gerçek smoothed view boyutunu kullanır |  | Düşük/yüksek hız additive hedef, hedef kapanışında yönün aniden tersine dönmemesi, smooth recovery ve arena sınırı regresyonları geçti |
| Kamera smoothing | ✅ | position 2.75 üstel; zoom 3.2 kritik sönümlü; look-ahead 1.65 üstel |  | Zoom hedef geçişlerinde velocity korunur |
| World bounds padding | ✅ | 200 |  |  |
| Camera shake | ✅ | sinüs ofset + kalan sürenin karesiyle azalma |  |  |
| LevelOne stage zinciri | 🟡 | Wait → Vanguard → TwinBlade → Hexagon → UFO → Chaos → Boss → Infinite |  |  |
| Infinite stage | 🟡 | enemy=min(5+2w,25); interval=max(1.5-.075w,.5); diff=1+.25×w/5 |  |  |
| Boss phases | 🟡 | <75% stage2; <50% stage3; <25% stage4 |  |  |

## 11. Eklenecek içerik için boş satırlar

### Yeni ability

| ID | Durum | Slot | Base değer | Scaling | Level / scrap | Runtime dosyası | Test sonucu |
| --- | --- | --- | --- | --- | --- | --- | --- |
|  | Fikir |  |  |  |  |  |  |
|  | Fikir |  |  |  |  |  |  |

### Yeni silah

| ID | Durum | Type / damage type | Damage / rate | Özel matematik | Scaling | Config dosyası | Test sonucu |
| --- | --- | --- | --- | --- | --- | --- | --- |
|  | Fikir |  |  |  |  |  |  |
|  | Fikir |  |  |  |  |  |  |

### Yeni gemi

| ID | Durum | HP / armor | Weapon | Reward / XP | Hareket / enerji farkı | Config dosyası | Test sonucu |
| --- | --- | --- | --- | --- | --- | --- |
|  | Fikir |  |  |  |  |  |  |
|  | Fikir |  |  |  |  |  |  |

### Yeni arena / level

| ID | Durum | Bounds / grace | Kamera preset | Boundary cezası | Wave / encounter | Kaynak | Test sonucu |
| --- | --- | --- | --- | --- | --- | --- | --- |
|  | Fikir |  |  |  |  |  |  |
|  | Fikir |  |  |  |  |  |  |

## 12. Bu kataloğu güncelleme kontrol listesi

- [ ] Yeni config ID’si, ilgili “mevcut içerik” tablosuna eklendi.
- [ ] Durumu Uygulandı / İçerik mevcut / Runtime hazır olarak yazıldı.
- [ ] Base değer ve scaling kuralı kaydedildi.
- [ ] Etkilenen damage type, status, attachment ve level progression yazıldı.
- [ ] Varsayılan başlangıç loadout’ına bağlı olup olmadığı belirtildi.
- [ ] En az bir test veya playtest sonucu yazıldı.
- [ ] Ana referanstaki matematik, kod değişmişse güncellendi.
- [ ] Uygulanan her değişiklik için BALANCE_AND_ROADMAP_NOTEBOOK.md ve
      PROJECT_DOCUMENTATION.md aynı değişiklik setinde güncellendi.
