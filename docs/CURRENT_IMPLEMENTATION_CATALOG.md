# LightYears — Mevcut Uygulama Kataloğu

> **20 Eylül 2026 doküman tutarlılık güncellemesi:** Bu katalog çalışma ağacındaki
> dirty implementation'ı açıklar. Sayısal shipped ability/effect değerleri
> `LightYearsGame/assets/content/data/*.json` kaynaklıdır; C++ fallback
> definition'ları behavior, typed actor ve presentation sözleşmesini korur.
> Runtime başlangıç slotlarının tek canonical sahibi
> [`DefaultAbilityLoadout.cpp`](../LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp)'dir;
> bu katalog complete loadout tablosunu tekrar etmez. `docs/vault/00 - Runtime
> Snapshot.md` tarihsel bir snapshot'tır.
> “Test sonucu” kolonundaki başarı kayıtları tarihsel olabilir; 20 Eylül 2026
> docs-only denetiminde build/test çalıştırılmadı. Güncel CMake'de 8 CTest kaydı
> vardır; bu katalog güncel bir pass kanıtı olarak kullanılmamalıdır.

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
| Başlangıç loadout | ✅ Kaynak sahibi `DefaultAbilityLoadout.cpp` | `gameplay/ability/loadout/DefaultAbilityLoadout.cpp`; `player/PlayerSpaceShip.cpp` | Exact slot ataması source owner dosyasından okunur; katalog bunu tekrar etmez. Shipped catalog'daki diğer ability'ler sonradan uygun slota takılabilir |
| İlk hasar koruması | ✅ Oyuncu 2 sn invulnerable başlar | LightYearsGame/src/player/PlayerSpaceShip.cpp | mInvulnerabilityTime ile değiştirilebilir |

## 3. Sistem envanteri

### 14 Ağustos 2026 ability runtime kartları

| ID | Durum | Runtime davranışı | Kaynak |
| --- | --- | --- | --- |
| `Ability.Defense.ShieldHarvest.Basic` | ✅ | 1.5 s alan sayımı; 700 radius içinde her rakip için 40 temporary overshield; 5 s hold, 100/s decay | JSON; `ability/shieldHarvest/` |
| `Ability.Offense.HullShock.Basic` | ✅ | Hold-to-charge Electric burst; 300→600 radius, 30 base damage, `MaxHealth×0.30`; shared telegraph radial curve | JSON; `ability/hullShock/` |
| `Ability.Offense.OrbitalDrones.Basic` | ✅ | 6 s boyunca 4 drone, 500 orbit radius, 18 Kinetic contact damage ve 0.5 s aynı-hedef beklemesi | JSON; `ability/orbitalDrones/` |
| `Ability.Offense.ExecutionDrive.Basic` | ✅ | +10 AttackPower effect; kill başına +3 stack ve duration refresh; düşük canlı hedefe chase movement | JSON; `ability/executionDrive/` |
| `Ability.Utility.RelayPrism.Basic` | ✅ | MouseWorld capture volume; uygun ability projectile'i %15 transfer + `AttackPower×0.25` hasarlı 4+Luck clone'a dönüştürür | JSON; `ability/relayPrism/` |
| `Ability.Utility.EchoProtocol.Basic` | ✅ | Kayıtlı saldırı echo utility family’si; 0.60 base power ve çoklu owner-stat scale katsayıları | JSON; `ability/echoProtocol/` |
| `Ability.Offense.MineLayer.Basic` | ✅ | 3 Energy mine; 8 s ömür, 30 damage, stun/knockback; `AttackPower×0.75` | JSON; `ability/mineLayer/` |
| `Ability.Offense.RailBurst.Basic` | ✅ | High-speed swept Energy projectile; farklı hedefleri deler, same-target guard; Crit pierce loss azaltır | JSON; `ability/railBurst/` |
| `Ability.Offense.CrescentReaver.Basic` | ✅ | Mouse-directed Kinetic ricochet; AttackPower damage ve Luck bounce scaling | JSON; `ability/crescentReaver/` |
| `Ability.Movement.EnergySpear.Basic` | ✅ | Held-charge hareket/temas Energy saldırısı; distance ve charge multiplier | JSON; `ability/energySpear/` |
| `Ability.Offense.ScorchDrive.Basic` | ✅ | Thermal fire-segment izi, özel sabit per-tick burn ve MaxHealth lifetime scale | JSON; `ability/scorchDrive/` |
| `Ability.Offense.IonStorm.Basic` | ✅ | Cursor'a giden hasarsız projectile; hedefte cast başına sabit düzensiz sınır üreten 4 s Electric alan, 0.25 s tick ve 16 tick; görsel tek renkli tek dolu düzensiz şekildir | JSON; `ability/ionStorm/`; typed Ion Storm presentation |
| `sas::AbilityRuntimeBinding` | ✅ | Tanımın content slotundan bağımsız, current equipment slotunu taşır | `SpaceAbilitySystem/include/abilities/AbilityRuntimeBinding.h` |

Önceki tablolar tarihsel ayrıntı içerebilir. Güncel sayısal değerler için
`abilities.json`, bu kartlar ve vault [[00 - Runtime Snapshot]] birlikte
okunmalıdır.

| Sistem | Durum | Çalışan davranış | Kaynak / düzenleme noktası | Denge veya test notu |
| --- | --- | --- | --- | --- |
| Ability.Control.NullPulse.Basic | ✅ | 11 sn cooldown, Instant, 1 charge; radius 500 (+200); Energy damage, projectile cleanup ve Stun/Stagger uygular; Stun yeni/aktif ability ve primary fire yürütmesini, outgoing damage ve trigger aksiyonlarını bloklar. Slot binding bu kartta tutulmaz. | `assets/content/data/abilities.json`; `gameplay/ability/nullPulse/`; `presentation/ability/nullPulse/` | [x] Loader, behavior validation, damage/control, projectile ayrımı, EnergyPower scale, typed visual cleanup ve CTest |
| Ability.Offense.OverdriveCore.Basic | ✅ | 1 sn launch + 5 sn AttackSpeed boost; 8 homing Kinetic projectile; same-target decay 0.90, CriticalChance-scaled boost ve 2–5 progression. Slot binding bu kartta tutulmaz. | `assets/content/data/abilities.json`; `gameplay/ability/overdriveCore/`; `presentation/ability/overdriveCore/` | [x] Loader, actor/profile validation, homing/damage, gerçek ship/dummy, boost state/effect ve CTest |
| Ability.Movement.PhaseDrift.Basic | ✅ | Shipped, varsayılan loadout'ta değil; 14 sn cooldown, 6 sn duration; cleanse, damage/collision protection, movement/recovery boost, break-on-action ve typed aura | `assets/content/data/abilities.json`; `gameplay/ability/phaseDrift/`; `presentation/ability/phaseDrift/` | [ ] Loader var; runtime lifecycle, cleanse/protection, collision restoration, break-on-action ve visual cleanup testleri eklenmeli |
| Ability.Offense.IonStorm.Basic | ✅ | Shipped, varsayılan loadout'ta değil; 900 menzil ve 2000 hızla cursor'a ulaşan hasarsız projectile; hedefte 250 inner core, 250–335 arasında cast-stable düzensiz Electric alan; 4 sn, 0.25 sn aralık, 16 tick; Common.Damage 6 + AttackPower×0.12; L2–L15 +1 damage / -0.20 sn cooldown | `assets/content/data/abilities.json`; `gameplay/ability/ionStorm/`; `presentation/ability/ionStorm/` | [x] Loader, behavior/actor/profile registration, common boundary resolver ve content/runtime başlangıç doğrulaması |
| SpaceAbilitySystem statik kütüphanesi | ✅ Doğrulandı | Attribute, Ability ve Effect generic çekirdeği ile lifecycle kararları `sas` namespace'inde; engine/content/presentation entegrasyonları oyun adaptörlerinde | SpaceAbilitySystem/CMakeLists.txt; SpaceAbilitySystem/include/{attributes,abilities,effects}; SpaceAbilitySystem/src/{attributes,abilities,effects} | Tarihsel kayıt: Debug/Release `SpaceAbilitySystem.lib` ve test executable'ları üretildi; iki konfigürasyonda 2/2 geçti. Bu turda yeniden çalıştırılmadı |
| sas::AttributeSystem | ✅ Doğrulandı | `GameplayAttribute`, modifier, scaling rule, lookup map, handle map ve delegate kimlikleri `sas::AttributeId` kullanır; Add, Multiply, Override, min/max clamp ve scaling sırası korunur | SpaceAbilitySystem/include/attributes/AttributeSystem.h; SpaceAbilitySystem/include/attributes/GameplayAttribute.h; SpaceAbilitySystem/src/attributes/AttributeSystem.cpp | Davranış test edilmedi; test çalıştırılmadı |
| sas::AttributeId | Uygulandı | Numeric gameplay attribute kimliği için string-backed, opaque API; equality, validity ve hash desteği; SAS lookup/delegate/spec mutator yolları ve game loader'ları kullanır | SpaceAbilitySystem/include/attributes/AttributeId.h; SpaceAbilitySystem/include/attributes/GameplayAttribute.h; SpaceAbilitySystem/include/effects/GameplayEffectSpec.h | Lookup API'leri `FindAttribute`, `FindAttributeValue`, `HasAttribute`; test çalıştırılmadı |
| ly::AttributeIdSchema + ability family ownership | ✅ Doğrulandı | Canonical AttributeId biçimi, legacy `Attribute.` prefix reddi ve nokta-segment namespace sınırı; non-primary ability base değerleri ve `Ability.*` modifier/scaling/level target'ları exact content-ID family namespace'inde ve declare edilmiş olmalıdır | LightYearsGame/include/gameplay/attributes/AttributeIdSchema.h; LightYearsGame/src/gameplay/ability/validation/GameAbilityDefinitionValidator.cpp | `Common.*` açık kalır; scaling source ile `Effect.*`/`AbilityActor.*` consumer-owned hedefleri ayrıdır; Configured ve PrimaryFire edge case'leri dahil GasLiteCoreTests geçti |
| sas::AttributeMath | ✅ Doğrulandı | Crit, luck, armor için doygun üstel eğriler | SpaceAbilitySystem/include/attributes/AttributeMath.h | Rating/cooldown/movement regresyonları core test içinde geçti |
| Oyun attribute ID kataloğu | Uygulandı | Owner, ship, ortak, weapon, damage, effect, attachment ve ability actor numeric katalog sabitleri `sas::AttributeId`; canonical format `Owner.*`, `Common.*`, `AbilityActor.*` vb. | LightYearsGame/include/gameplay/attributes/AttributeIds.h; gameConfigs/*; gameplay/ability/*Contracts.h | Semantic audit tamamlandı; `CurrentRuntimeValue` semantic runtime key ve `MinCancelDuration` numeric setting olarak ayrık kaldı |
| Proje geneli gameplay tag sözleşmesi | ✅ | Domain kökleri/biçim doğrulaması ile iki ortak action lock; numeric `Attribute.*` tag biçimi reddedilir, feature leaf'leri yerel kalır | LightYearsGame/include/gameplay/tags/GameplayTagSchema.h; ability/effect/actor/weapon/attachment ve ship progression validation | Attribute API rename tamamlandı; semantic attribute bilgisi schema'ya taşınmadı; test çalıştırılmadı |
| SAS Ability temel sözleşmeleri | ✅ Doğrulandı | `sas::AbilityHandle` ve slot/lifecycle/action/targeting policy enum'ları; aktif kod açık `sas::` tiplerini kullanır | SpaceAbilitySystem/include/abilities/{AbilityHandle,AbilityPolicies,AbilityTypes}.h | Legacy karşılaştırma dosyaları doğrulama sonrası silindi |
| sas::AbilityRuntimeSnapshot | ✅ Doğrulandı | Game-owned definition pointer'ı olmadan ID, slot, level, active/cooldown/duration/charge durumunu taşır | SpaceAbilitySystem/include/abilities/AbilityRuntimeSnapshot.h | Eski pointer tabanlı karşılaştırma kaldırıldı |
| sas::AbilityDefinition + validation | ✅ Doğrulandı | Kimlik, slot/lifecycle, owner tag koşulları ve generic attribute/scaling verisi; ID/sayı/duration/passive ile catalog null/duplicate doğrulaması | SpaceAbilitySystem/include/abilities/{AbilityDefinition,AbilityDefinitionValidation}.h; SpaceAbilitySystem/src/abilities/AbilityDefinitionValidation.cpp | Action/weapon/UI/content doğrulaması LightYearsGame'de kalır; tarihsel catalog testleri Debug/Release geçti, bu turda yeniden çalıştırılmadı |
| sas::AbilityEvent | ✅ Doğrulandı | Tag/magnitude ile typed opaque source/target/context referanslarını SAS taşır; Actor ve `DamageContext` oyun binding'i tarafından bağlanır | SpaceAbilitySystem/include/abilities/AbilityEvent.h | Oyun çağrıları typed `Set/GetSource`, `Set/GetTarget` ve context API'sine geçirildi |
| sas::GameplayAbilityInstance + runtime state | ✅ Doğrulandı | Input/activation/end/level/cooldown/duration/charge/snapshot yaşam döngüsü SAS'a aittir | SpaceAbilitySystem/include/abilities/{GameplayAbilityInstance,AbilityRuntimeState}.h | Game tarafındaki `GameAbility` yalnız concrete behavior/action, weapon ve attachment içeriğidir; tarihsel lifecycle testleri geçti, bu turda yeniden çalıştırılmadı |
| LightYearsAbilitySystemComponent | ✅ Doğrulandı | `sas::AbilitySystemComponent` türevi tek oyun component'ı; Actor erişimi SAS-owned `AbilitySystemInterface::GetAbilitySystemComponent()` kontratından yapılır. Game subclass yalnız owner, shipped content/validation, attachment endpoint'leri ve `Actor`/`DamageContext` effect specialization'ını taşır | SpaceAbilitySystem/include/{AbilitySystemInterface,AbilitySystemComponent}.h; LightYearsGame/include/gameplay/ability/LightYearsAbilitySystemComponent.h | Self-facade ve eski CombatRuntime erişimleri olmadan oyun/test hedefleri linklendi |
| SAS Ability runtime mekanikleri | ✅ Doğrulandı | `AbilityBehaviorRegistry`, `GameplayAbilityInstance`, `AbilityRuntimeSystem`, `AbilityExecution`, scheduler/trigger, component notification zinciri ve toplu cooldown azaltma framework sahipliğindedir | SpaceAbilitySystem/include/abilities; SpaceAbilitySystem/include/AbilitySystemComponent.h; SpaceAbilitySystem/src/AbilitySystemComponent.cpp | Tarihsel Debug/Release build ve GAS runtime testleri geçti; bu turda yeniden çalıştırılmadı |
| SAS Effect çekirdeği | ✅ Doğrulandı | Handle/policy/definition/spec/validation/state yanında `GameplayEffectRuntimeSystem` tam active-effect lifecycle’ını, `GameplayEffectBehaviorRuntime` typed hook dispatch’ini sahiplenir | SpaceAbilitySystem/include/effects; SpaceAbilitySystem/src/effects | Typed runtime context namespace/include bağlantıları düzeltildi; tarihsel Debug/Release effect testleri geçti, bu turda yeniden çalıştırılmadı |
| sas::GameplayEffectCollection | ✅ Doğrulandı | Active-effect storage, handle allocator/lookup/index lookup, predicate lookup, index erase ve reset | SpaceAbilitySystem/include/effects/GameplayEffectCollection.h | Lifecycle policy kararları SAS orchestrator'ında; callback/visual cleanup adaptörü oyunda |
| SAS Effect registry, bindings ve lifecycle | ✅ Doğrulandı | Typed hook storage; tag gate; modifier/tag binding; instant/refresh/stack/duration ve opt-in stack-decay kararları; zorunlu attribute/tag sahipliği reference kontratıyla tutulur | SpaceAbilitySystem/include/effects; SpaceAbilitySystem/src/effects | `SpaceAbilitySystemLifecycleTests` geçti; full damage gameplay suite bu turda özellikle çalıştırılmadı |
| Oyun ability definition/content sınırı | ✅ Tamamlandı | `ly::GameAbilityDefinition`, SAS definition tabanını action/trigger/level, silah, damage/attachment, behavior ve UI alanlarıyla genişletir | LightYearsGame/include/gameplay/ability/content/GameAbilityDefinition.h | Geçiş alias'ı ve eski `AbilityStructs.h` kaldırıldı |
| Engine diagnostics | ✅ | CORE/GAME kanallı structured logging, Debug assert/verify ve scope/counter profiler | LightYearsEngine/include/framework/debug/*; src/framework/debug/*; EntryPoint.cpp | Debug lifetime testleri log/filter/verify/profile kontratını doğrular; Release'te assert/profiling compile-out, logging Warning+ kalır |
| CombatRuntime | ✅ | Attribute, effect, ability ve incoming damage akışını birleştirir | gameplay/combat/CombatRuntime.* | Damage sırası §6’da kayıtlı |
| DamageTypeSystem | ✅ | 6 tür tag’i, payload override ve status uygulama | gameplay/damage/DamageTypeSystem.* | Hibrit tag önceliği tasarım kararı gerektirir |
| HealthComponent | ✅ | Health clamp, damage/heal eventleri | gameplay/HealthComponent.* | Max health değişince yüzde koruma açık |
| ShieldComponent | ✅ | Kalıcı gemi shield’ı, gecikme ve regen | gameplay/ShieldComponent.* | Energy hasarı shield kapasitesini daha hızlı tüketir |
| EnergyComponent | ✅ | Afterburner enerjisi, gecikme ve regen | gameplay/EnergyComponent.* | Consume ve recharge aynı frame davranışı test edilebilir |
| ShipRuntime | ✅ | Ship attribute'ları ile shield/afterburner türetmelerini owner attribute'lardan çözer | gameplay/ship/ShipRuntime.* | SpaceShip sahiplenir; CombatRuntime'dan ayrı tutulur |
| GameplayEffectSystem game adaptörü | ✅ Doğrulandı | `sas::AbilitySystemComponent` effect facade'ını Actor event, incoming `DamageContext` ve presentation callback’lerine bağlar | SpaceAbilitySystem/include/AbilitySystemComponent.h; LightYearsGame/src/gameplay/combat/CombatRuntime.cpp; LightYearsGame/src/gameplay/effects/LightYearsEffectBehaviorRuntime.cpp | Active storage/lifecycle `sas::GameplayEffectRuntimeSystem` içindedir |
| AbilitySystem game adaptörü | ✅ Doğrulandı | `LightYearsAbilitySystemComponent` SAS component'ini Actor, trigger execution, attachment ve game delegate’lerine bağlar | SpaceAbilitySystem/include/AbilitySystemComponent.h; LightYearsGame/include/gameplay/ability/LightYearsAbilitySystemComponent.h | Grant/remove/slot/tick/snapshot sahipliği SAS runtime'ındadır |
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
| LevelOne akışı | 🟡 | Wait → Vanguard → TwinBlade → Hexagon → Chaos → Infinite geçici zinciri | level/LevelOne.cpp | Boss/UFO legacy girişleri kaldırıldı; başlangıç app akışında doğrudan yüklenmiyor |
| HUD / warning | ✅ | Gameplay HUD, shield/effect görünümü, arena uyarısı | widget/*; presentation/* | Tooltip’te resolved stat gösterimi ayrı iş |
| Otomatik test runner | ✅ | Core gameplay sistemleri için GasLiteCoreTests | LightYearsGame/tests/GasLiteCoreTests.cpp | SAS sınır doğrulaması sonunda Debug/Release çalıştırılır |

## 4. Aktif ability ve effect içeriği

### Content kaynak durumu

| Konu | Mevcut durum | Planlanan sınır |
| --- | --- | --- |
| Otoriter shipped content | Weapon/ability/effect/ship sayısal değerleri owner JSON'da; JSON'da 1 attachment kaydı var. C++ fallback attachment tanımları ayrıca `AttachmentConfig.h` içinde tutulur. | Düşman/boss content, attachment bootstrap ve acquisition akışı ayrı bağlanacak; attachment runtime equip/resolve zaten mevcut |
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
| `gameplay/ability/overdriveCore/` | OverdriveCoreAbility, ProjectileActor, VisualActor | Multi-rocket hedef tahsisi, homing projectile, same-target decay, AttackSpeed boost ve presentation cleanup |
| `gameplay/ability/phaseDrift/` | PhaseDriftAbility, PhaseDriftVisualActor | Cleanse, geçici damage/collision protection, ship runtime modifier'ları, break-on-action ve aura lifecycle |
| `gameplay/ability/glacialPressure/` | GlacialPressureAbility, GlacialPressureTelegraphActor | Beş segmentli cone focus, Cryo buildup, MaxHealth tabanlı vektörel push, push collision ve ortak Stun effect |
| `gameplay/ability/rocket/` | RocketAbility, RocketProjectileActor | Rocket'e özgü validation, projectile delivery, patlama ve gelecek evolve parçaları |
| `gameplay/ability/shield/` | ShieldAbility | Shield'e özgü behavior |
| `gameplay/ability/sunBeam/` | SunBeamAbility, actor ve visual sınıfları | SunBeam'e özgü tüm runtime parçaları |
| `gameplay/ability/content/` | GameAbilityDefinition | SAS-owned temel tipleri kullanan game-owned definition, action payload ve UI metadata |
| `gameConfigs/ability/` | AbilityActorStructs, AbilityCatalog ve kategoriye ayrılmış aile config'leri | `movement/{Dash,PhaseDrift}Config`, `control/{GravityAnomaly,NullPulse}Config`, `defensive/{Shield,ShieldHarvest,DirectionalBarrier}Config`, `offensive/{HullShock,SunBeam,Rocket,InfernoSpray,OverdriveCore,OrbitalDrones,ExecutionDrive,GlacialPressure}Config`; bu config'ler yalnız ID/tag, behavior/action, actor type, presentation ve schema kontratını taşır; sayısal tuning JSON'dadır |
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

> Bu envanterdeki eski “Oyuncuda varsayılan mı?” slot notları ayrı bir source
> audit'i yapılana kadar tarihsel kabul edilmelidir. Güncel runtime slotlarının
> tek kaynağı [`DefaultAbilityLoadout.cpp`](../LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp)'dir;
> bu tablo content/runtime kartlarını taşır, binding tablosu değildir.
> Bu envanterdeki eski “Oyuncuda varsayılan mı?” slot notları 20 Eylül 2026
> source review'ünde tarihsel olarak korundu. Güncel runtime slotlarının tek
> kaynağı [`DefaultAbilityLoadout.cpp`](../LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp)'dir;
> bu tablo content/runtime kartlarını taşır, binding tablosu değildir.

| ID | Durum | Tarihsel oyuncu-slot notu | Mevcut config | Runtime matematiği | Hedef / test notu |
| --- | --- | --- | --- | --- | --- |
| Ability.Defense.Shield.Basic | ✅ | Hayır; shipped içerik, PlayerSpaceShip varsayılan grant listesinde değil | 8 sn cooldown; 5 sn duration; Basic Barrier uygular | Barrier capacity = 30 + 0.20×MaxHealth + 50×Armor | [ ] Shield uptime ve break event test edilecek |
| Ability.Offense.SunBeam.Strike.Basic | ✅ | Evet, Ability2 / E | 1 sn cooldown; MouseWorld spawn | Base damage 40; radius 96; L2–5 her sefer +8 damage ve +8 radius | [ ] Tek hedef ve area hasarı ölçülecek |
| Ability.Movement.Dash.Basic | ✅ | Evet, Ability3 / F | 2 sn cooldown; 0.24 sn duration; 1 charge; mevcut kamera mesafesine +%15 göreli zoom-out | Base 260 mesafe; mevcut velocity tamamen korunur ve Dash impulse üzerine eklenir; kamera velocity ile follow offset'i korur; göreli zoom kritik sönümlü kamera hattıyla girip çıkar; L2-L5 taban cooldown'un her seferinde %6'sını düşürerek 1.88/1.76/1.64/1.52 olur | [x] Katalog, tuning-safe cooldown, yön, tam momentum, kamera offset/göreli zoom ve zoom-velocity sürekliliği, lifecycle ve cleanup core testleri |
| Ability.Offense.Rocket.Basic | ✅ | Evet, Ability4 / R | 7 sn cooldown; Instant; 1 charge; mouse aim yönünde tek projectile; cursor yakındaysa cursor'da, uzaktaysa 1100 maksimum menzilde patlar | Damage 55 + AttackPower×1.25; Kinetic; radius 55; speed 1000 ve maksimum range 1100 sabit. L2-L15: +4 damage, -0.12 sn cooldown, +1 radius | [x] Katalog/actor validation, cursor hedef mesafesi ve telegraph, max-range clamp, tek spawn, yön, owner scaling, haste, Kinetic AoE tek-vuruş, L1-L15 sabit delivery ve cleanup core testleri |
| Ability.Control.GravityAnomaly.Basic | ✅ | Evet, Ability1 / Q; bu slotta Shield'in varsayılan grant'inin yerini alır | 8 sn cooldown; Instant; 1 charge; 900 cast range, 2000 projectile speed; hedefte 2.5 sn / 220 radius field | Damage yok. Field caster/player/enemy Combatant'larına source-scoped `Effect.GravityAnomaly.Inside.Basic` uygular: içeride 2 sn'ye yenilenen %20 movement slow ve `500×(1-d/radius)^2×dt` velocity pull. Çıkış/field bitiminde pull kesilir, slow 2 sn sürer. MaxHealth yalnız radius (+0.20) ve duration'ı (+0.0025) scale eder. L2-L15: -0.10 cooldown, +0.03 duration, +2 radius, +10 pull, +0.005 slow, +25 speed, +5 range | [x] Typed profile/actor validation, varsayılan Ability1/Q loadout, clamp ve lifecycle, target filtreleme, pull/center güvenliği, gerçek movement slow, 2 sn exit/destroy tail, field-source expiry cleanup, L15/MaxHealth scaling ve effect visual cleanup |
| Ability.Offense.GlacialPressure.Basic | ✅ | Evet, Ability4 / R; Rail Burst yerine varsayılan | 12 sn cooldown; 1 sn focus; 700 uzunlukta, 22° yarı açılı, 5 segmentli cone; focus sonrası 1,5 sn impulse penceresi | Uzaklığa göre hasar/push/Cryo stack azalır: taban push mesafeleri 800/680/560/440/320; her segment 140 birimdir. İlk itiş Stun'u 1,5 sn, gemi çarpışması Stun'u 2 sn, birinci segmentte push sonrası 2 sn daha refresh olur. Başlangıç kuvveti bu mesafeleri exponential deceleration ile kat edecek biçimde hesaplanır. Çarpışma hasarı hem gerçek hareket hızına göre %25–%100 quadratic ölçeklenir, hem de merkeze uzaklıkla %100/%90/%80/%73,33/%66,67 azalır; swept collision ortak damage + Cryo uygular. Cooldown focus ve impulse penceresi tamamlandıktan sonra başlar | [ ] Push mesafesi, collision ve 4-stack Cryo runtime ölçümü |
| PrimaryFire üretilmiş ability | ✅ | Evet, Space | Weapon definition’dan slot/action oluşturulur | Level ve scaling weapon profile’dan gelir | [ ] Her silah için ayrı card doldur |

### 4.2 Effect ve status kayıtları

| ID / status | Durum | Mevcut değer | Matematik / davranış | Hedef / test notu |
| --- | --- | --- | --- | --- |
| Effect.Barrier.Basic | ✅ | Capacity 30; ratio 1; regen 6/sn; delay 1.5 sn; duration 5 sn | Kaynak hasar emilimi = capacitySpent / (ratio×shieldMultiplier) | [x] Shield-only Armor etkisizliği, hull taşması, shield’sız summon ve Energy 1.50 doğrulandı |
| Effect.Test.BarrierBreak.ThrustBoost | ✅ | 2 sn; horizontal +0.20; vertical +0.25 | BarrierBroken trigger ile self’e uygulanır | [ ] “Test” ID’si üretim adlandırmasına taşınacak mı? |
| Effect.GravityAnomaly.Inside.Basic | ✅ | 2 sn duration, refresh-duration, source-scoped; field başına %20 slow | `AreaGameplayEffectApplicator` hedef içerideyken süreyi sürekli 2 sn'ye resetler. Çıkışta veya field bitiminde pull hemen kapanır; slow ve target visual kalan 2 sn sürer. Ayrı field source scope'ları birbirini korur. | [x] Multi-field izolasyonu, enter/leave/destroy tail, expiry cleanup, gerçek movement slow ve target-following visual doğrulandı |
| Thermal / Ignite | ✅ | 1–4 stack; 1 / 2 / 3 / 5 hasar/sn; 5 sn | İlk stack çalışır; süre bitince ilk stack, sonra her saniye bir stack azalır; `BurnDamagePerTick` özel modu canonical DPS ile çiftlenmez | [x] GasLite canonical DPS, decay, refresh ve özel burn ayrımı |
| Cryo / Slowed | ✅ | 1–4 stack; %4 / %8 / %12 / %20 slow; 5 sn | Tek yetkili stack effect'i `Cryo.Slowed`; ilk stack çalışır, yeniden uygulama stack ekler ve süreyi yeniler, decay modifier'ı tabloya göre senkronize eder | [x] GasLite slow tablosu, decay ve reapply |
| Electric | ✅ | 1–4 stack; +%3 / +%6 / +%9 / +%16 alınan hasar; 4 sn | PreMitigation aşamasında mevcut stack'ler vuruşa uygulanır; o vuruşun yeni stack'i sonraki vuruşu etkiler; decay sırasında stack düşer | [x] GasLite taken-damage tablosu, sıra ve decay |
| Kinetic | ✅ | 1–4 stack; %6 / %12 / %18 / %30 Armor penetration; 5 sn | Önceden mevcut stack hit başında snapshot'lanır ve yalnız Kinetic hull hasarına eklenir; yeni stack sonraki hit'te kullanılır; shield'a uygulanmaz | [x] Deterministic payload/Armor sırası testleri eklendi; tam GasLite koşusu mevcut Orbital Drones ön-koşulunda duruyor |

## 5. Damage türleri ve mevcut matematik

| Tür | Durum | Varsayılan payload | Kaynak | Değiştirilebilir hedef / test |
| --- | --- | --- | --- | --- |
| Photonic | ✅ | Özel numeric payload yok | DamageTypeSystem.cpp | [ ] Nötr baseline DPS tanımla |
| Energy | ✅ | `BuildPayload` tabanı shieldDamageMultiplier 1.50; regen delay source attribute/profile'dan | DamageTypeSystem.cpp; `DamageStatusBalance` | [x] Legacy 1.0/1.25 explicit değerlerin tabanı düşürmediği ve shield overflow doğrulandı |
| Kinetic | ✅ | Varsayılan hit 1 stack; özel payload 1–4; stack başına %6 / %12 / %18 / %30; 5 sn | EffectConfig.h; DamageTypeSystem.cpp; ortak hull Armor çözümü | [x] Yeni stack'in aynı hit'e girmemesi, 4 stack=%30 ve Energy izolasyonu için testler eklendi |
| Thermal | ✅ | 1–4 stack; 1 / 2 / 3 / 5 hasar/sn; 5 sn | `DamageStatusBalance`; `DamageTypeSystem.cpp`; registered tick hook; özel `BurnDamagePerTick` yolu canonical tabloyla çarpılmaz | [x] Runtime ve içerik değerleri doğrulandı |
| Cryo | ✅ | 1–4 stack; %4 / %8 / %12 / %20 slow; 5 sn | `DamageStatusBalance`; `DamageTypeSystem.cpp`; tek `Cryo.Slowed` effect'i ve stack-change senkronizasyonu | [x] Runtime ve içerik değerleri doğrulandı |
| Electric | ✅ | 1–4 stack; +%3 / +%6 / +%9 / +%16; 4 sn | `DamageStatusBalance`; `DamageTypeSystem.cpp`; registered PreMitigation hook | [x] Runtime ve içerik değerleri doğrulandı |

### Hasar parametre değişiklik tablosu

| Parametre | Mevcut değer / formül | Kaynak | Hedef değer | Ölçülen sonuç | Karar |
| --- | --- | --- | --- | --- | --- |
| Crit rating scale | 100 | AttributeMath.h |  |  |  |
| Crit multiplier | 1.5 varsayılan payload; ileride `1.5 + BonusCriticalDamage` | DamageContext.h |  |  |  |
| Armor scale | 50 / ln(2) ≈ 72.1348 | AttributeMath.h |  |  |  |
| Armor penetration üst sınırı | 0.25 | DamageTypeSystem.cpp |  |  |  |
| Shield damage multiplier | Tür/attribute’a bağlı; Energy 1.50 | DamageTypeSystem.cpp |  |  |  |
| Damage status stack tabloları | Cryo %4/%8/%12/%20; Electric +%3/+%6/+%9/+%16; Thermal 1/2/3/5 DPS; Kinetic %6/%12/%18/%30 penetration | DamageTypeConfig.h | JSON/C++ tek sahibi |  |  |
| Damage status süreleri | Cryo/Thermal/Kinetic 5 sn; Electric 4 sn; decay sonrası 1 sn/stack | DamageTypeConfig.h; GameplayEffectRuntimeState.cpp | Seçili dört Stack effect'i |  |  |

### Pipeline kontrol kartı

| Aşama | Mevcut işlem | Kontrol / not |
| --- | --- | --- |
| 1 | `DamageCriticalPolicy`: Random zar, Guaranteed veya Disabled; ardından `Owner.CriticalDamage` | Crit policy tek karar noktasıdır; global çarpan owner attribute'unda, başlangıç 1.5x |
| 2 | Electric PreMitigation effect | İlk stack’ten itibaren canonical tablo çarpanını uygular |
| 3 | Barrier effect | Geçici effect kapasitesi harcanır |
| 4 | Thermal/Cryo/Electric/Kinetic status üretimi | Incoming effect aşamasında, remainingDamage > 0 ise; seçili dört policy ASC içinde |
| 5 | Geminin ShieldComponent’i | Kalıcı shield capacity’si harcanır; Energy source/capacity ayrımı korunur |
| 6 | Armor ve penetration | Yalnız shield sonrası kalan hull hasarı, ortak `ApplyHullDamageMitigation` hesabından geçer |
| 7 | HealthComponent | Kalan hull hasarı health’e gider |
| 8 | Damage eventleri | DamageTaken, DamageDealt, Ignite attachment eventleri |

## 6. Oyuncu silah kataloğu

| Silah ID | Durum | Varsayılan loadout | Tür / damage type | Mevcut temel değer | Mevcut scaling | Hedef / test notu |
| --- | --- | --- | --- | --- | --- | --- |
| Weapon.Projectile.FighterRapidLaser.Basic | ✅ | Evet | Standard / Photonic | 12 damage (L15: 152), 4 fire rate, 1100 speed, 1600 range, 1 muzzle, Mag 48, Reload 2.0s; 6th/final-6 empowered; MaxLevel 15 | Normal damage: BaseDamage + AP × APRatio (+0.40 L1, +1.10 L15); Empowered damage: NormalRaw + EmpoweredBaseDamage (+2 L1, +16 L15) + EP × EPRatio (+0.10 L1, +0.52 L15), guaranteed 1.5x crit & ceil; Cadence: OwnerAttackSpeedPercentage (FR × (1+AS/100)); Reload = 2.0s / (1+AS/100) | L1 (AP 35, EP 35): normal = 26, empowered = 48, mag total = 1534, sustained DPS ≈ 109.6; L15 (AP 77, EP 63): normal = 237, empowered = 429, mag total = 13872 (35 × 237 + 13 × 429), sustained cycle DPS ≈ 990.86 (12s fire + 2s reload = 14s); scrap costs placeholder; diğer silahlar değişmedi |
| Weapon.Projectile.RapidShotgun.Basic | ⚙️ | Hayır | Shotgun / Thermal | 10 damage, 2.5 fire rate, 3 pellet, 8° spread, 400 range | Damage +0.75 AP; FireRate +0.5 AS | Runtime AP/AS, impact-time progressive same-target falloff and Ignite profile verified |
| Weapon.Projectile.DualKineticBlaster.Basic | ⚙️ | Hayır | Standard / Kinetic | 3.5 damage, 12 fire rate, 2 muzzle, 3400 speed, 650 range | Damage +0.45 AP; FireRate +1.0 AS | Runtime two-muzzle DPS verified; L50 remains below Rapid Laser |
| Weapon.Arc.ElectricLauncher.Basic | ⚙️ | Hayır | Arc / Electric | 15 damage, 2.8 fire rate, 850 first range, 3 extra chain, 250 chain range, x0.72 | Damage +0.85 AP; FireRate +0.60 AS; Luck has no direct damage scaling | Runtime 1/2/4/5 target, falloff, no-duplicate and capped bonus-chain chance verified |
| Weapon.Beam.ContinuousHeatLaser.Basic | ⚙️ | Hayır | Continuous Beam / Energy | 28 DPS, 950 range, 26 width, 38 heat/sn, cap 100, cool 25/sn | Damage +0.75 AP +0.50 EnergyPower; AttackSpeed only reduces high-heat gain | Runtime AP/Energy, heat zones, overheat and 10/30s sustained DPS verified |
| Weapon.Wave.CryoProjector.Basic | ⚙️ | Hayır | Expanding Wave / Cryo | 7 damage, 1.8 fire rate, 780 range, 850 speed, width 80→260 | Damage +0.75 AP; FireRate +0.5 AS | Runtime L1/10/25/50 damage/rate and four-hit slow tempo verified |

### Silah progression kaydı

| Silah | Durum | L2 | L3 | L4 | Scrap 2/3/4 | Ölçülen L4 DPS | Karar |
| --- | --- | --- | --- | --- | --- | --- |
| BasicRapidLaser | ✅ | L2-L15 her seviye: +10 Damage, +1 Empowered Damage, +0.05 AP scaling, +0.03 EP scaling (L15: Base 152 D, 1.10 AP, 16 Empowered D, 0.52 EP) | 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 105 (placeholder) | L15 (AP 77, EP 63): normal = 237, empowered = 429; 48 mermilik mag (35 normal, 13 empowered) toplam 13872 hasar; 14 sn döngüde sustained DPS ≈ 990.86; diğer silahlar Phase 3B.3 kapsamında değiştirilmedi |
| DualKineticBlaster | ✅ | +1.5 FR | +1 D | +100 range, +1 FR | 40 / 50 / 65 |  |  |
| ElectricArcLauncher | ✅ | +4 D | +1 chain, +80 range | +0.4 FR, +0.08 chain factor | 40 / 50 / 65 |  |  |
| ContinuousHeatLaser | ✅ | +6 D | +150 range | +0.20 max-heat multiplier | 40 / 50 / 65 |  |  |
| CryoWaveProjector | ✅ | +2 D, +30 width | +100 range, +0.5 buildup duration | +0.3 FR, +2 D | 40 / 50 / 65 |  |  |

### Silaha özel hesap alanları

| Mekanik | Mevcut matematik | Değer girilecek alan |
| --- | --- | --- |
| Standard projectile | Teorik tek hedef DPS = damage × fireRate × muzzleCount × hitRate | HitRate: ___ ; DPS: ___ |
| Shotgun | nthPelletMultiplier = max(floor, 1 - reduction×priorHitsOnTarget) | Pellet hit sayısı: ___ ; DPS: ___ |
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
| Rapid Shotgun | 67.50 | 1337.18 | 6733.80 | 24840.68 | x1.00 + x0.90 + x0.80 progressive same-target pellet toplamı |
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
| Weapon.Projectile.EnemyVanguardPulse.Basic | ✅ | 15 / 1.2 | 1 | ApproachGunner temporary profile; Standard / Energy; speed 700, range 1200; maxLevel 1, AP/EP yok | [x] Runtime ve loader testleri geçti; foundation geçici |
| Weapon.Projectile.EnemyTwinBladeScatter.Basic | ✅ | 8 / 0.8 | 3 pellet | StrafeSkirmisher temporary profile; Shotgun / Kinetic; spread 12°, range 500; maxLevel 1, AP/EP yok | [x] Runtime ve loader testleri geçti; foundation geçici |
| Weapon.Wave.EnemyHexagonCryoPulse.Basic | ✅ | 10 / 0.6 | 1 wave | RangeKeeper temporary profile; Expanding Wave / Cryo; width 60→200, speed 500, range 600; maxLevel 1, AP/EP yok | [x] Runtime ve loader testleri geçti; foundation geçici |

## 8. Ship kataloğu

| Gemi ID | Durum | HP | Collision | Score / Ship XP | Silah | Loot olasılıkları | Hedef / test |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Ship_Player_Fighter | ✅ | 250 | 25 | 0 / 0 | Weapon.Projectile.FighterRapidLaser.Basic | Yok | [ ] Varsayılan baseline |
| Ship.Enemy.ApproachGunner.Basic | ✅ | 60 | 50 | 10 / 10 | `EnemyCombat.ApproachGunner.Basic` → EnemyVanguardPulse | Health .20, Life .05, Shield .08 | [x] Generic EnemyActor/chassis content |
| Ship.Enemy.StrafeSkirmisher.Basic | ✅ | 60 | 50 | 20 / 20 | `EnemyCombat.StrafeSkirmisher.Basic` → EnemyTwinBladeScatter | Health .25, Life .05, Shield .08 | [x] Generic EnemyActor/chassis content |
| Ship.Enemy.RangeKeeper.Basic | ✅ | 100 | 60 | 30 / 30 | `EnemyCombat.RangeKeeper.Basic` → EnemyHexagonCryoPulse | Health .30, Life .05, Shield .10 | [x] Generic EnemyActor/chassis content |

### Player Fighter — doldurulabilir hareket/enerji kartı

| Parametre | Mevcut değer | Hedef değer | Ölçüm / sonuç |
| --- | --- | --- | --- |
| Forward / reverse / strafe thrust | 650 / 190 / 270 |  |  |
| Max speed / damping | 520 / 0.36 |  |  |
| Turn speed / responsiveness | 400 / 5 |  |  |
| Input responsiveness / mouse dead zone | 12 / 32 |  |  |
| EnergyPower / ReactorBudget | 35 / 70 |  |  |
| Base shield / full recharge / delay | 115 / 6 sn / 6 sn |  |  |
| Afterburner base capacity / full recharge / delay | 65 / 6 sn / 1.5 sn |  |  |
| Afterburner speed / acceleration | 1.55 / 1.80 |  |  |
| Energy drain | 33 / sn |  |  |
| Ramp up / down / maneuverability | .22 sn / .45 sn / .90 |  |  |
| Reactor affinity / final shield / final capacity | 50 / 50 / 150 / 100 |  |  |
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

C++ fallback kataloğunda 7 attachment tanımı ve runtime resolver desteği vardır.
Shipped JSON kataloğunda şu an 1 attachment kaydı bulunur; bootstrap JSON'u henüz
yüklemediği için fallback tanımları ile shipped content aynı şey değildir.
Varsayılan Player Fighter loadout’ında hiçbiri otomatik equip edilmez.

| Attachment | Durum | Host / capability | Mevcut etki | Doldurulacak test |
| --- | --- | --- | --- | --- |
| Thermal Converter | ⚙️ | Ability/Primary; Damage | Thermal ise Damage x1.20; Ignite sonrası non-primary cooldown -0.4 | [ ] Ignite proc döngüsü |
| Energy Coupler | ⚙️ | Ability/Primary; Damage | Energy ise Damage x1.15; shield x1.50, delay +.75 | [ ] Shield break |
| Kinetic Bore | ⚙️ | Ability/Primary; Damage | JSON'da +0.10 granted `Damage.ArmorPenetration` ve Kinetic koşulunda +0.05 extra; status tablosu %6/%12/%18/%30 ve 5 sn decay policy'den ayrı source bonusudur | [ ] Armor breakpoint |
| Cryo Conduit | ⚙️ | Ability/Primary; Damage | Cryo hit stack başına canonical %4/%8/%12/%20 slow ve 5 sn decay policy kullanır | [ ] Slow uptime |
| Electric Conduit | ⚙️ | Ability/Primary; Damage | Electric canonical alınan hasar tablosu +%3/+%6/+%9/+%16 ve 4 sn decay policy kullanır | [ ] Burst combo |
| Heavy Capacitor | ⚙️ | Ability; Damage+Cooldown | Damage x1.40, Cooldown x1.25 | [ ] Net value |
| Emergency Salvo | ⚙️ | Primary; FireRate+Projectile | FireRate < 4 ise +1 projectile | [ ] Eşik davranışı |

## 10. Arena, kamera ve level içeriği

> **Geçici test UI notu (16 Ağustos 2026):** Gemilerin sağ üstündeki hasar
> sayıları ve HUD hız değeri yalnızca mevcut combat akışını test etmek için
> eklenmiştir. Son UI/presentation çalışmasında yeniden tasarlanacak ve hasar
> bildirimi `SpaceShip` API'sinde kalmayacak; bu bağlantı geçici bir test
> köprüsüdür.

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
| LevelOne stage zinciri | 🟡 | Wait → Vanguard → TwinBlade → Hexagon → Chaos |  | Legacy stage yolu; `InfiniteStage` kaldırıldı, ana giriş ArenaTestLevel |
| Infinite stage | ❌ | `InfiniteStage` kaldırıldı |  | Endless encounter sorumluluğu `EncounterWaveRuntime` içinde |
| Arena encounter runtime | ✅ | Endless cycle; 3 template, başlangıç hedefi 3 enemy, wave başına +1; her 3 wave'de level-up ve level-up dalgasında bilinçli -2 telafi; level cap 15, active cap 24, deterministic seed | Encounter kompozisyonu ve sayılar vertical-slice test kararıdır | Lifecycle, progression, snapshot ve restart testleri yerel GasLite suite içinde geçti |
| Arena encounter lifecycle | ✅ | `Idle / Running / Completed / Failed`; playerless start fail, player death owned enemy cleanup, restart fresh player + encounter | Completion reward/victory akışı sonra | Runtime lifecycle testi geçti |
| Encounter snapshot | ✅ | `EncounterWaveSnapshot`: state, wave sayıları, planned/spawned/alive/remaining enemy, inter-wave süre, enemy level | `EncounterHUDController` read-only kontratı tüketiyor | Snapshot kontrat ve HUD entegrasyon yolu mevcut |

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

## 9. Düşman muharebe profilleri (Enemy Combat Profiles)

Kaynak dosya: `LightYearsGame/assets/content/data/enemy_combat_profiles.json`  
Sözleşme ve runtime: `include/gameplay/enemy/EnemyCombatProfile.h`, `EnemyRuntime.h`

- `EnemyRuntime`: Atomik initialization ve kontrollü transactional rollback sağlar. Preflight ile harici slot çakışmaları mutation öncesi engellenir. Move/copy semantiği kapatılmıştır. Enemy-owned level growth, deterministic variation, source-owned shield contribution ve encounter outgoing-damage modifier'ını da temizler.
- `EnemyActor`: Generic profile-backed actor; önce `EnemyRuntime`, sonra base `SpaceShip`, sonra `EnemyBehaviorRuntime` initialize edilir. UFO, boss ve `EnemySpaceShip` legacy aktörleri kaldırılmıştır.
- `DummyEnemy`: Arena test target'ı olarak profile dışı, doğrudan `SpaceShip` tabanlıdır.
- `EnemyBehaviorRuntime`: Şimdiki basit Approach/HoldRange/Strafe movement/fire policy katmanıdır; Advanced Enemy AI sonraki aşamadır.
- LevelOne, ChaosStage ve InfiniteStage geçici stage/wave akışlarıdır; üç generic enemy profili provisional vertical-slice içeriğidir. Enemy AP/EP varsayılanı 0'dır. Author edilmiş base damage, enemy profile level growth ve sınırlı variation ile birleşen tek encounter outgoing multiplier üzerinden ölçeklenir.

| Profil ID | Durum | Birincil Silah | Power Scaling | Yetenekler | Aktör Eşleşmesi | Not |
| --- | --- | --- | --- | --- | --- | --- |
| `EnemyCombat.ApproachGunner.Basic` | ✅ | `Weapon.Projectile.EnemyVanguardPulse.Basic` | Disabled | [] | `Enemy.ApproachGunner.Basic` / `EnemyBehavior.ApproachGunner.Basic` | [x] Generic EnemyActor loadout ve PrimaryFire; Energy hasar tipi |
| `EnemyCombat.StrafeSkirmisher.Basic` | ✅ | `Weapon.Projectile.EnemyTwinBladeScatter.Basic` | Disabled | [] | `Enemy.StrafeSkirmisher.Basic` / `EnemyBehavior.StrafeSkirmisher.Basic` | [x] Generic EnemyActor loadout ve PrimaryFire; Kinetic hasar tipi |
| `EnemyCombat.RangeKeeper.Basic` | ✅ | `Weapon.Wave.EnemyHexagonCryoPulse.Basic` | Disabled | [] | `Enemy.RangeKeeper.Basic` / `EnemyBehavior.RangeKeeper.Basic` | [x] Generic EnemyActor loadout ve PrimaryFire; Cryo hasar tipi |

