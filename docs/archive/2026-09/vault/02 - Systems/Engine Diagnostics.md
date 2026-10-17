---
type: system
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
source_files:
  - VegaEngine/include/framework/debug/Assert.h
  - VegaEngine/include/framework/debug/Log.h
  - VegaEngine/include/framework/debug/Profiler.h
  - VegaEngine/src/framework/debug/Assert.cpp
  - VegaEngine/src/framework/debug/Log.cpp
  - VegaEngine/src/framework/debug/Profiler.cpp
  - VegaEngine/src/EntryPoint.cpp
  - VegaEngine/tests/EngineLifetimeTests.cpp
symbols:
  - vega::debug::Log
  - vega::debug::Profiler
  - vega::debug::ProfileScope
  - VEGA_ASSERT
  - VEGA_VERIFY
  - VEGA_CORE_INFO
  - VEGA_GAME_INFO
  - VEGA_PROFILE_SCOPE
related:
  - "[[Application Startup Flow]]"
  - "[[Performance Monitoring]]"
  - "[[Engine Runtime Services]]"
---

# Engine Diagnostics

## Amaç

Engine ve game kodunun aynı hata kaydı, invariant kontrolü ve CPU süre ölçüm
kontratını kullanmasını sağlar. Makrolar `framework/Core.h` üzerinden erişilir;
uygulama sahipliği `EntryPoint.cpp` içindedir.

## Logging

Logging iki kanal (`CORE`, `GAME`) ve altı seviye (`TRACE`, `DEBUG`, `INFO`,
`WARN`, `ERROR`, `FATAL`) sunar. Her kayıt zaman, kanal, seviye, mesaj ve kaynak
dosya/satırını taşır. Console çıktısına ek olarak oyun başlangıcında çalışma
dizinindeki `LightYears.log` dosyası açılır.

```cpp
VEGA_CORE_INFO("Loaded %d assets", assetCount);
VEGA_GAME_WARN("Ability grant rejected: %s", reason.c_str());
```

Debug varsayılan eşiği `TRACE`, Release varsayılan eşiği `WARN`dır. Error ve
Fatal kayıtları hemen flush edilir. `FATAL` bir log seviyesidir; tek başına
uygulamayı durdurmaz.

## Assert ve verify

```cpp
VEGA_ASSERT(index < effects.size(), "Invalid effect index: %zu", index);
VEGA_VERIFY(TryRegisterContent(), "Content registration failed");
```

- `VEGA_ASSERT` Debug invariant'ları içindir. Başarısızlık `FATAL` kaydı üretir;
  debugger bağlıysa break, değilse abort eder.
- `VEGA_ASSERT` Release'te compile-out edilir ve koşulu değerlendirmez. Bu yüzden
  assert ifadesinde side effect bulunmaz.
- `VEGA_VERIFY` her build'de koşulu değerlendirir. Debug'ta assert gibi durur;
  Release'te `ERROR` loglayıp devam eder.
- Kullanıcı girdisi, dosya/network hatası veya beklenen content rejection
  normal kontrol akışı ve logging ile ele alınır; assert programcı
  invariant'ına ayrılır.

## Profiling

```cpp
VEGA_PROFILE_FUNCTION();
VEGA_PROFILE_SCOPE("GravityAnomaly.TargetDiscovery");
VEGA_PROFILE_COUNTER("Actors", activeActorCount);
```

Debug'ta RAII scope süreleri çağrı sayısı, toplam, minimum ve maksimum
mikrosaniye olarak aggregate edilir. Application kapanışında özet loglanır.
Release'te profiler makroları compile-out edilir.

Instrument edilen ilk kritik yollar:

- Application frame, tick, render ve shutdown
- World tick, cleanup ve render
- `sas::AbilitySystemComponent` ability/effect tick
- `sas::GameplayEffectRuntimeSystem` apply, tick ve incoming damage
- Gravity Anomaly target discovery
- Actor, bullet ve particle sayaçları

## Başlangıç ve kapanış

`EntryPoint.cpp`, application oluşturulmadan önce `Log::Initialize` ve
`Profiler::Initialize` çağırır. Normal çıkışta ve yakalanan exception
yollarında profiler önce özetlenip kapatılır, ardından logger flush edilip
kapatılır.

## Build matrisi

| Özellik | Debug | Release |
|---|---|---|
| Logging | Açık, TRACE+ | Açık, WARN+ |
| Assert | Açık | Compile-out |
| Verify | Değerlendirir, başarısızsa durur | Değerlendirir, ERROR loglar |
| Profiling | Açık | Compile-out |

`VegaEngineLifetimeTests`, structured log alanlarını, seviye filtresini,
verify değerlendirmesini ve profiler scope/counter kontratını doğrular.
