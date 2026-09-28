# Ability balance batch 7 — catalog entries 51–55

## Change contract

- **Objective:** implement CryoBola, RelayPrism, VoidGate, EchoProtocol, and FoldspaceArena against the revised catalog rows #51–55.
- **Scope:** those five ability families, their shipped JSON records, and the relay/replay integration required to preserve their behavior. The user explicitly limited this batch to #51–55 while allowing necessary cross-family Echo output declarations. The existing uncommitted catalog revision is source input and is not staged by this batch.
- **Owners:** `abilities.json` for numeric content and progression; family C++ for behavior, fallback, and validation. Existing shared projectile/ability contracts remain the integration boundary.
- **Invariants:** preserve ability IDs and loadout, discrete status stacks/projectile counts/geometry, typed presentation/registry wiring, and unrelated authored balance.
- **Proof:** focused source/diff and JSON structural review followed by one final `LightYearsGame` build. GasLite/E2E are not part of the user's requested batch verification.

## Result

The five families now follow the revised catalog formulas and 14-step progression. CryoBola applies Energy Power damage once and distinguishes the primary hit from its area hit. RelayPrism transfers a level- and Energy Power-scaled fraction while preserving projectile-specific shield, field, and shotgun behavior. VoidGate uses the revised placement and active durations. EchoProtocol applies its revised replay power without replaying itself. FoldspaceArena scales damage and arena duration, and starts its cancel window after arena formation.

Replayable ability declarations now include verified continuous damage, healing, shielding, movement, and control outputs from other families. Projectile counts and bounce counts remain discrete and are excluded. This metadata is the necessary cross-family connection for EchoProtocol; the other families' underlying balance and progression were not redesigned in this batch.

## Verification and limit

`git diff --check` passed. `cmake --build D:\LightYears\build --target LightYearsGame --parallel 6` completed with exit 0 using the Visual Studio 2022 x64 environment. The build log is `build/balance-batch7-game.log`. JSON syntax was parsed by the update script; no GasLite or gameplay test was run under the user's time preference. Compilation alone does not establish gameplay acceptance.
