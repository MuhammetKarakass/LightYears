---
type: architecture
status: active
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
uncommitted_areas:
  - GAS-Lite gameplay-effect core and content registration
  - SpaceAbilitySystem static-library migration (Attribute + Ability 2D runtime-state phase)
  - ability, damage, movement and presentation integrations
  - GasLiteCoreTests and LightYearsGame CMake target
---

# Project Overview

## 20 Eylül 2026 kaynak kontrolü

Güncel durum tamamlanmış oyun değil combat test prototipidir. Ek `LightYearsGameplayCore` statik hedefi `LightYearsGame/CMakeLists.txt` içinde bulunur. Gerçek giriş `ArenaTestLevel`; [[2026-09-07 Project Status Review]] tarihsel kapsamı, [[System Index]] ise güncel sistem indeksini belirler.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


> Güncel çalışma ağacı özeti: [[00 - Runtime Snapshot]] (20 Eylül 2026 doğrulaması; tarihsel snapshot notları ayrıca etiketlidir).
> Sayısal ability/effect değerlerinde `assets/content/data/*.json`, C++ fallback
> config'lerinden önceliklidir.

## Nedir?

Light Years, üç build target'lı bir C++17 oyun projesidir:
`LightYearsEngine` genel runtime/engine araçlarını, statik
`SpaceAbilitySystem` generic gameplay-system çekirdeğini, `LightYearsGame` uzay
gemisi, level ve gameplay içeriğini sağlar. Engine/oyun kodunun baskın
namespace'i `ly`, SAS public API namespace'i `sas`tır.

## Teknoloji ve başlangıç

- Kök `CMakeLists.txt`: CMake 3.31.6, C++17, `LightYearsEngine`,
  `SpaceAbilitySystem` ve `LightYearsGame` alt dizinleri.
- `LightYearsEngine/CMakeLists.txt`: SFML 3.1.0 (graphics/audio/system/window) ile Box2D 3.1.1'i FetchContent ile getirir. Audio device invalidation ve bellek/performance incelemesi için [[Audio System]] ve [[Engine Performance and Memory]] notlarına bakın.
- Giriş noktası: `LightYearsEngine/src/EntryPoint.cpp` içindeki `main`; graph kaydı `LightYears.LightYearsEngine.src.EntryPoint.main`dir. Oyun uygulaması `LightYearsGame/src/gameFramework/GameApplication.cpp` / `GameApplication`dır.

## Runtime yapısı

`ly::Application` (`LightYearsEngine/include/framework/Application.h`) world yaşam döngüsünü yönetir. `ly::World` (`framework/World.h`) `ly::Object` tabanlıdır; sahne/actor tarafında `ly::Actor` (`framework/Actor.h`) ana tabandır. `ly::GameStage` ile game-stage akışı; `GameLevel` ve seviyeler oyun katmanındadır.

Önemli ortak servisler: `AssetManager`, `TimerManager`, `PhysicsSystem`,
`AudioManager`, `ShaderManager`, `CameraManager`. Oyun tarafında
`PlayerManager`, [[CombatRuntime]], `LightYearsAbilitySystemComponent` ve [[AttributeSystem]]
bulunur. `CombatRuntime`, owner `sas::AttributeSystem`,
`GameplayTagContainer`, `sas::AbilitySystemComponent`/[[GameplayEffectSystem]] nesnelerini
value member olarak aynı lifetime sınırında kurar.

Ayrıntılı çekirdek runtime akışı: [[Game Loop]], [[Actor and World System]], [[Object Lifecycle]] ve [[Ownership and Lifetime]]. Uygulama başlangıcı ile frame/lifecycle çağrı zincirleri [[Application Startup Flow]], [[Frame Update Flow]] ve [[Actor Spawn and Destruction Flow]] notlarındadır.

## Doğrulanan gameplay alanları

Ability davranış/çalıştırma/sistem sınıfları halen `LightYearsGame/gameplay/ability/`
altında; weapon handler ve yürütme `gameplay/weapon/` altındadır. Ability handle
ve policy sözleşmeleri `SpaceAbilitySystem/include/abilities` altına
taşınmıştır; aynı dizindeki `sas::AbilityRuntimeSnapshot` definition pointer'ı
olmadan salt-okunur runtime state taşır. Generic `sas::AbilityDefinition`,
`sas::AbilityEvent` ve definition validation da bu katmandadır. Oyun
definition/action içeriği
`LightYearsGame/include/gameplay/ability/content/GameAbilityDefinition.h`
altında kalır. Geçiş karşılaştırma kopyaları 2026-07-30 tarihinde silinmiştir.
Generic attribute tipleri
`SpaceAbilitySystem/include/attributes`, uygulaması
`SpaceAbilitySystem/src/attributes` içindedir. Owner/ship/common ID kataloğu
oyun content'i olarak `LightYearsGame/include/gameplay/attributes/AttributeIds.h`
altında kalır.

### Güncel runtime binding ve content sınırı

Ability definition'ın `slot` alanı content'in varsayılan yerini anlatır;
`sas::AbilityRuntimeBinding::slot` ise o instance'ın anlık loadout konumudur.
Bu ayrım, aynı shipped ability'nin acquisition/loadout akışında content kimliği
değişmeden başka bir aktif slota taşınmasını sağlar. Güncel runtime slot binding'inin
canonical sahibi `LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp`'dir;
bu belge complete loadout tablosunu kopyalamaz.

GAS-Lite çekirdeği koddan ayrıca doğrulandı:

- `GameplayTag`, `GameplayTagHash` ve counted `GameplayTagContainer`: `LightYearsEngine/include/framework/Core.h`; [[Gameplay Tag System]].
- `sas::GameplayAttribute`, `sas::AttributeModifier`, `sas::AttributeSystem` ve
  rating eğrileri: `SpaceAbilitySystem/include/attributes/` ve
  `SpaceAbilitySystem/src/attributes/`; [[Attribute System]].
- `sas::AbilityHandle`, `sas::AbilitySlot` ve ability policy enum'ları:
  `SpaceAbilitySystem/include/abilities/`; [[Ability System]].
- `sas::AbilityRuntimeSnapshot`: game definition pointer'ı taşımayan salt-okunur
  runtime gözlem sözleşmesi.
- `sas::AbilityDefinition`, `sas::AbilityEvent` ve
  `sas::ValidateAbilityDefinition`: generic definition/event/validation çekirdeği;
  action/weapon/UI/DamageContext oyun adaptöründe kalır.
- `sas::AbilityRuntimeState`: input edge, level, active/cooldown/duration ve
  charge lifecycle state'i; `GameAbility` game adapter'ı olarak `sas::GameplayAbilityInstance`'ı özelleştirir.
- `sas::AbilityLifecycleOrchestrator`: input/lifetime policy kararları.
- `sas::GameplayEffectLifecycleOrchestrator`: application, stacking/refresh ve
  duration expiry kararları.
- `GameplayEffectDefinition` → `GameplayEffectSpec` → `ActiveGameplayEffect`: `gameConfigs/combat/EffectStructs.h` ve `gameplay/effects/`; [[Gameplay Effect System]].
- Effect'ler aynı owner attribute/tag nesnelerine `CombatRuntime` constructor'ında bağlanır; effect tick'i ability tick'inden önce çalışır.

## Build ve test

Statik hedefler `LightYearsEngine`, `SpaceAbilitySystem` ve `LightYearsGameplayCore`; çalıştırılabilir hedef
`LightYearsGame`dir. Test executable'ları `LightYearsEngineLifetimeTests`, `LightYearsGasLiteTests`,
`LightYearsContentTests` ve koşullu specialized test hedefleridir. CMake'de toplam 8 CTest kaydı vardır;
bu belge güncellemesinde build/test çalıştırılmadı.

## Sonraki dokümantasyon alanları

GAS-Lite çekirdeğinden sonra ilk aday [[Ability System]]'dır. Presentation, weapon ve progression ayrıntıları bu aşamanın kapsamı dışındadır.
