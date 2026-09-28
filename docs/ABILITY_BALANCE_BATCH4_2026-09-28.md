# Ability balance batch 4 — catalog entries 29–37

## Change contract

- **Objective:** apply the authored balance and runtime behavior for EmberSwarm, Shield, ShieldHarvest, DirectionalBarrier, Cryostasis, ReturnProtocol, TemporalRecall, TimeSlip, and CrystalBarricade.
- **Allowed owners:** these nine ability families, their `abilities.json` records, and this report. Shared runtime wiring is allowed only where an ability cannot use the established family path.
- **Canonical data owner:** `LightYearsGame/assets/content/data/abilities.json`; family-specific C++ owns behavior and fallback values. The default slot map remains owned by `DefaultAbilityLoadout.cpp` and is outside this batch.
- **Invariants:** preserve IDs, other 46 ability records, upgrade costs, typed presentation registration, and lifetime/registry contracts. The catalog's last five old-progression abilities remain untouched.
- **Proof:** review the final diff and JSON record delta, then build `LightYearsGame` once after integration. The user waived GasLite/E2E test runs for this batch; compilation alone does not establish gameplay acceptance.

## Result

| Catalog | Result |
|---|---|
| 29 EmberSwarm | `Common.Damage` now resolves `6 + 2 × (L-1) + EnergyPower × (0.08 + 0.01 × (L-1))` once per cast and feeds each drone pulse. Thermal/Ignite delivery remains one stack per successful pulse. Three drones retain their staggered 0.25-second pulse clocks. Target selection uses 750 acquisition, 1000 retention, under-cap Ignite priority, fewer stacks, fewer assigned drones, distance, then unique ID. |
| 30 Shield | The effect capacity is `60 + 10 × (L-1) + MaxHealth × (0.20 + 0.01 × (L-1))`, with five seconds of duration and 10-second base cooldown. Removed Armor contribution, regeneration, and the break thrust reward. The existing one-stack RefreshDuration barrier policy replaces current capacity on recast. |
| 31 ShieldHarvest | After the 1.5-second focus, a single query at the owner's current position captures enemies within 700. Each eligible target gets one Energy pulse (`10 + 2 × (L-1) + EnergyPower × 0.05`) and a 0.25-second stun subject to control resistance. Each contributes `20 + 5 × (L-1) + EnergyPower × (0.10 + 0.01 × (L-1))` shield. The existing temporary-overshield grant fills missing normal shield first, then tracks excess with five seconds of grace and 100/s decay. |
| 32 DirectionalBarrier | Existing duration, movement multiplier, and 14 progression steps match the catalog; no runtime edit needed. |
| 33 Cryostasis | Removed the catalog-extraneous +0.25/level Afterburner recovery bonus from JSON and fallback. Existing base recovery and listed damage/ice/health/break formulas remain. |
| 34 ReturnProtocol | Existing reflection formula and 14 progression steps match the catalog; no runtime edit needed. |
| 35 TemporalRecall | Existing recovery/overshield formula and 14 progression steps match the catalog; no runtime edit needed. |
| 36 TimeSlip | Existing slow/time formulas and 14 progression steps match the catalog. Its cross-frame modifier key now uses the owner's unique object ID instead of its address. |
| 37 CrystalBarricade | Removed the catalog-extraneous +0.04/level ricochet modifier from JSON. Existing actor-level +3 contact damage progression and wall formula remain. |

EmberSwarm, Shield, and ShieldHarvest use the catalog's 14-step global diminishing cooldown progression. The other six retain their cataloged linear progression. The nine abilities retain 14 upgrade costs each; Shield receives the same 60 Scrap cost used by the other abilities because its old record had no progression.

## Verification and limit

- Parsed `abilities.json`: 55 records remain; exactly five records changed (EmberSwarm, Shield, ShieldHarvest, Cryostasis, CrystalBarricade). The other 50 records are structurally identical to the pre-batch baseline.
- `git diff --check`: passed.
- `LightYearsGame` build: exit 0 with MSVC and `VSLANG=1033`. Ninja held zero recorded dependencies for `AbilityCatalog.cpp`, so its object was rebuilt explicitly and the game relinked; that final build also exited 0. Logs: `build/balance-batch4-game.log` and `build/balance-batch4-game-final.log` (local build artifacts).
- GasLite/E2E tests were not run under the user's batch instruction. Compilation and JSON structure do not prove gameplay behavior or content-loader acceptance.
