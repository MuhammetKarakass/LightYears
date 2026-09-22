---
type: system
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/include/framework/PerfMonitor.h
  - LightYearsEngine/include/framework/debug/Profiler.h
  - LightYearsEngine/src/framework/debug/Profiler.cpp
  - LightYearsEngine/src/framework/World.cpp
  - LightYearsEngine/src/VFX/Particle.cpp
  - LightYearsGame/src/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.cpp
symbols:
  - ly::perf::TickAndReport
  - ly::perf::g_activeActors
  - ly::perf::g_bullets
  - ly::perf::g_particles
  - ly::perf::g_disableLights
  - ly::debug::Profiler
  - LY_PROFILE_SCOPE
  - LY_PROFILE_COUNTER
related:
  - "[[Engine Diagnostics]]"
  - "[[Engine Runtime Services]]"
  - "[[Technical Debt and Roadmap]]"
---

# Performance Monitoring

`ly::perf`, canlı actor, bullet ve particle sayılarını `std::atomic<int>` ile
tutan hafif runtime sayaç katmanıdır. `World::SpawnActor` /
`World::CleanCycle`, Particle ctor/dtor ve primary projectile lifecycle bu
sayaçları günceller. `g_disableLights` atomic flag'i Actor light render yolunda
kontrol edilir.

`World::TickInternal` her frame `TickAndReport(dt)` çağırır. Beş saniyede bir
değerler `LY_PROFILE_COUNTER` ile profiler snapshot'ına aktarılır ve
`LY_CORE_INFO` ile structured log olarak raporlanır.

Scope profiler `ly::debug::Profiler` içinde ayrı bir katmandır. Debug
derlemesinde `LY_PROFILE_SCOPE`, `LY_PROFILE_FUNCTION` ve `LY_PROFILE_FRAME`
RAII sürelerini isim bazında çağrı sayısı, toplam, minimum ve maksimum
mikrosaniye olarak aggregate eder. `LY_PROFILE_COUNTER` son sayaç değerini
saklar. Release derlemesinde profiling makroları compile-out edilir.

Mevcut instrumentation kapsamı application frame/tick/render, World
tick/cleanup/render, `sas::AbilitySystemComponent`, typed effect runtime ve Gravity Anomaly
target discovery yollarıdır. Kapanışta en pahalı scope'lar logging sistemine
özetlenir. GPU timing, frame percentile/histogram, allocation byte/count ve
haricî telemetry pipeline henüz kapsamda değildir.

API ve Debug/Release davranışı: [[Engine Diagnostics]].
