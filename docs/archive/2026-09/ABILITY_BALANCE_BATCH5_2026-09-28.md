# Ability balance batch 5 — catalog entries 38–46

## Change contract

- **Objective:** implement or verify ClosedCircuit, IroncladProtocol, TemporalConvergence, ShieldGraft, ReclaimerProtocol, Dash, PhaseDrift, EnergySpear, and VectorSync against their authored catalog rows.
- **Allowed owners:** these nine ability families, their records in `LightYearsGame/assets/content/data/abilities.json`, and this report. Shared code changes require a concrete integration need.
- **Canonical owners:** `abilities.json` for shipped numeric data, family C++ for behavior and fallback contracts. Runtime slot mapping remains owned only by `DefaultAbilityLoadout.cpp`.
- **Invariants:** preserve IDs, unrelated ability records, presentation/registry/lifetime contracts, and the final five old-progression abilities outside this batch.
- **Proof:** focused diff and JSON structural review followed by one final `LightYearsGame` build. The user waived GasLite/E2E runs; compilation is not gameplay acceptance.

## Result

The shipped records and family runtime for ClosedCircuit (#38), TemporalConvergence (#40), ShieldGraft (#41), ReclaimerProtocol (#42), PhaseDrift (#44), and VectorSync (#46) already match their catalog formulas and progression. IroncladProtocol (#39) also matches its 8-damage minigun, reduction, movement, duration, and level progression. During review the catalog's Photonic label was found to disagree with the weapon's Kinetic tag; the user explicitly confirmed **Kinetic**, so catalog row #39 was corrected and the weapon's existing Kinetic content was preserved.

Dash (#43) now starts at a 9-second cooldown and computes dash distance from the ship's movement-speed attributes:

`BaseDashDistance × [1 + (DirectionWeightedMoveSpeed / 100 × (0.80 + 0.10 × (Level - 1)))]`.

Horizontal and vertical ratings use the absolute dash-direction components as weights. Current velocity does not feed the distance scale. The ability sends the resolved distance through a small opt-in movement request field so the old generic sigmoid does not scale it again. The progression has 24 upgrade steps (L2–L25): four each at cooldown deltas `-0.40`, `-0.30`, `-0.20`, `-0.10`, `-0.08`, and `-0.06`; each step adds `+0.10` to the MoveSpeed coefficient. This treats the catalog's L1–L4 notation as four-step groups after the L1 baseline. The original four Scrap costs remain `40/50/65/80`; twenty new steps use the project's common 60-Scrap cost.

EnergySpear (#45) already matches its damage, charge, distance, and progression values. Its traversal actor now tracks targets by `GetUniqueID()` across frames, preventing a newly spawned actor at a reused address from being incorrectly skipped.

## Verification and limit

- Parsed `abilities.json`: 55 records remain; only `Ability.Movement.Dash.Basic` differs from the pre-batch baseline. Dash has 24 level steps and 24 costs; level-25 cooldown is 4.44 seconds.
- Confirmed `Weapon.Projectile.IroncladMinigun.Basic` remains tagged `Damage.Type.Kinetic`, following the user's correction.
- `git diff --check` passed. `LightYearsGame` built with MSVC and `VSLANG=1033` (exit 0). The `AbilityCatalog.cpp` object was explicitly rebuilt to include changed headers despite the checkout's incomplete Ninja header-dependency record. Local build log: `build/balance-batch5-game.log`.
- GasLite/E2E were not run under the user's batch instruction. Compilation and structural JSON checks do not establish gameplay or content-loader acceptance.
