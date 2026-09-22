---
type: system
verified_on: 2026-09-07
verification: source-review-only
verified_worktree_state: dirty
---

# Temporal Runtime

## Kaynakta mevcut

`LightYearsEngine/include/framework/SimulationTime.h`: RealTime, HostileGameplay, ProjectileGameplay. `World::TickInternal` actor dt'sini domain ölçeğiyle çarpar; `GetSimulationTimeScale` kaynaklar arasındaki en küçük multiplier'ı seçer, çarpımla birleştirmez. Camera/HUD ve Application timer/physics dt'si aynı yoldan ölçeklenmez.

`LightYearsGame/src/gameplay/ability/timeSlip/TimeSlipAbility.cpp`, `Activate`, hostile ve projectile domain'lerine modifier ekler ve owner primary fire rate'ini ayrıca değiştirir. `End` yalnız kendi source-id modifier'larını kaldırır. Player movement/diğer ability zamanını global olarak yavaşlatmaz. Bu aile kayıtlı fakat güncel default loadout'ta değildir.

`SpaceShip::Tick` temporal geçmişi capture eder; `src/gameplay/temporal/TemporalStateHistory.cpp` geçmiş servisi, `TemporalRecallAbility` focus/rewind/recovery bağlantısı içerir. Temporal Recall'ın bütün recovery ve collision dalları okunmadı; bu paragraf global world rewind veya disk save desteği anlamına gelmez.

## Doğrulama ihtiyacı

Ölüm/iptal/world switch sonrası modifier kalmaması, stacked source kaldırma, büyük dt ve primary cadence/mermi hareket uyumu birlikte test edilmeli. Game timer'ları domain zamanından bağımsızdır; timer tabanlı düşman davranışının da otomatik yavaşladığı söylenemez. [[Movement]] · [[Shield and Energy]] · [[Frame Update Flow]].
