---
type: system
verified_on: 2026-09-07
verification: source-review-only
verified_worktree_state: dirty
---

# Movement

## Mevcut davranış

`LightYearsGame/src/player/PlayerMovementComponent.cpp`, `SetInput`/`ConsumeInput`: klavye polling, mouse aim, Shift afterburner ve ability input schema. `LightYearsGame/src/gameplay/MovementComponent.cpp`, `Tick`: voluntary velocity, movement burst, impulse, acceleration source ve forced movement ayrı değerlendirilir. Focus MovementInput kilidi yeni hareket girdisini keser fakat drift'i korur; ExternalMovement kilidiyle hard stasis hareketi durdurur ve impulse/acceleration kaynaklarını temizler.

`include/gameplay/movement/MovementInfluenceTypes.h` impulse / source-id acceleration / öncelikli forced velocity kontratlarını taşır. Eski bağımsız `ForcedMovementRequest.h` kaldırılmıştır; aynı adlı tip artık bu dosyada yer alır. `MovementPolicyTypes.h` damping ve speed-cap kurallarını attribute değerlerinden ayırır. Policy removal normalize seçeneği geçici hız sınırından normal sınıra dönüş sağlar. İlgili implementation `src/gameplay/movement/MovementPolicyController.cpp` ve `MovementInfluenceController.cpp`.

## Bağlantılar ve sınır

SpaceShip::Tick movement'tan sonra combat ve regeneration çalıştırır. Arena kamera kodu `IsMovementBurstActive` ile burst hızını normal speed zoom girdisinden ayırır. Kamera zoom resolver'ı hâlâ Ability3 slotunu sorgular; diğer slota takılan burst'lerle kamera ayarı ayrıca test edilmeli. Bütün hareket ability kombinasyonları incelenmedi; [[Temporal Runtime]] ve [[Shield and Energy]].

## Risk / öneri

Yeni policy ve influence test dosyaları var; çalıştırılmadı. Forced move + focus + stun + arena boundary ve iptal/ölümde source cleanup kabul senaryoları gerekli. Bu öneri yeni bir movement mimarisi kurma talebi değildir.
