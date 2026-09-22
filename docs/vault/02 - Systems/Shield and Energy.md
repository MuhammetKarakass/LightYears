---
type: system
verified_on: 2026-09-07
verification: source-review-only
verified_worktree_state: dirty
---

# Shield and Energy

`LightYearsGame/src/gameplay/ship/ShipRuntime.cpp`, `InitializeFromShipDefinition`/`RecalculateAttributes`, owner MaxHealth/EnergyPower değişimlerinden ship shield/afterburner attribute'larını türetir. EnergyPower bir owner statıdır; tüketilen miktar `EnergyComponent` içindeki afterburner rezervidir. Ability'lerin tamamının buradan enerji harcadığı varsayılmamalıdır.

`SpaceShip::UpdateRegeneration` health, temporary overshield, normal shield ve energy günceller. `PlayerMovementComponent` afterburner isteği/intensity, tüketim ve recharge bloklamasını yönetir. Aynı recharge izni normal shield ve energy'ye verilir; temporary overshield decay ayrı tick edilir. `EnergyComponent::Consume` gecikmeyi yeniler; `Tick` önce gecikmeyi tüketir, kalan dt ile yeniler.

`ShieldComponent::AbsorbDamage` hasarı multiplier üzerinden shield miktarına çevirip emilen özgün hasarı geri verir. Normal shield, effect Barrier ve geçici overshield farklı katmanlardır. `GrantTemporaryOvershield` maksimumun üzerindeki kısmı `TemporaryOvercapLedger` içinde source-id ile izler; hold/decay ve hasarla tüketim mevcut. Health tarafında da overcap API'si bulunur.

Sayılar JSON/ship definition tuning'idir; nihai denge değildir. Capacity değişimi, üst üste overcap kaynakları ve ölüm/temporal recovery kombinasyonlarının tamamı çalıştırılmadı. [[Shield System]] · [[Damage and Shield Resolution Flow]] · [[Temporal Runtime]].
