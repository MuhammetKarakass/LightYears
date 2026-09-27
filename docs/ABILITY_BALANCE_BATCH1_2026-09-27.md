# Ability catalog implementation — batch 1

## Change contract

Source: [ability catalog](ABILITY_SCALING_CATALOG.md), rows 1–9. User requested
Luna max implementation agents, orchestrator review and commits, then a stop after
the first nine abilities. No work on row 10 or later is authorized in this batch.

Objective: align these nine abilities' authored values, fallback contracts and
runtime behavior with their catalog entries. Preserve unspecified behavior.

Ownership:
- Luna 01–03: HullShock, GlacialPressure, OrbitalDrones family files and first-three E2E.
- Luna 04–06: SunBeam Strike, Rocket, InfernoSpray family files and second-three E2E.
- Luna 07–09: OverdriveCore, ExecutionDrive, MineLayer family files and third-three E2E.
- Codex orchestrator: shared authored JSON and its progression steps, required
  registration/CMake/E2E dispatch, integration review, verification and commits.

Required shared boundary: `AttributeSystem::GetCurrentValueExcludingModifiers`
provides a read-only calculation using the existing modifier math. Overdrive uses
its active boost effect's committed handles to exclude its own bonus while retaining
external Add/Multiply/Override/clamp semantics; it must not temporarily mutate the
owner's attributes or broadcast events. Acceptance includes an overlapping boost
with an external multiplier through the real ability path.

Invariants: no loadout changes; no changes to other ability records; JSON remains
the shipped numeric owner; canonical IDs and typed family presentation remain;
registry/lifecycle ownership rules apply. Existing level limits are retained
unless the catalog explicitly defines a longer sequence (Glacial/Orbital L25).
HullShock's existing 14 upgrade steps and unspecified HP coefficient are retained.

Proof: inspect complete diff; build game and GasLite targets; execute real-runtime
E2E scenarios and relevant existing E2E regressions with JSON artifacts. Unit
tests are not the gameplay acceptance mechanism (existing user preference).

## Status

Completed and verified. Initial Git working tree was clean at `0b18b0c` on
`codex/luna-callback-lifetime-checkpoint`. The first nine abilities are the stop
boundary; no implementation for row 10 or later was started.
Existing unrelated Luna callback/beam audit risks remain outside this package.

## Implementation decisions

- The catalog's diminishing cooldown sequence is authored as additive level steps
  in JSON, using the existing progression resolver. No new generic progression
  schema or second runtime formula owner was introduced. The first reduction is
  `0.175 + 0.025 * BaseCD`; each four upgrades advance its tier, with the specified
  20% floor. Orbital's separate tier sequence stops at its seven-second floor.
- Existing maximum levels remain HullShock/Rocket/ExecutionDrive/MineLayer L15
  and SunBeam/OverdriveCore L5. GlacialPressure and OrbitalDrones extend to L25.
  InfernoSpray has no new progression. HullShock's ambiguous “14 seviye” retains
  its existing fourteen upgrades; its unspecified HP coefficient remains 0.30.
- Glacial collision uses both MaxHealth values and the current pushed target's
  Cryo stacks. Pair deduplication uses unique object IDs. When both actors were
  pushed, the existing strongest-segment selection is preserved.
- Orbital contact tracking uses unique IDs and weak references. An early reentry
  during the 0.5-second gate does not later deal damage while contact continues;
  another exit and reentry is required.
- ExecutionDrive's numeric fallback mirrors the new buff attributes and fourteen
  level steps. SunBeam, Rocket, Overdrive and MineLayer keep their existing
  schema-only fallback pattern. Shared damage/status curves are unchanged.
- Canonical project and implementation catalog descriptions were updated where
  this batch made them stale. Runtime slot mappings are unchanged.

## Verification

Executed after all agent sources were frozen, from `D:\LightYears` in the VS 2022
Developer environment:

```text
cmake --build D:\LightYears\build --target LightYearsGame LightYearsGasLiteTests LightYearsContinuousBeamWallE2ETests --parallel 6
ctest --test-dir build -R 'LightYearsAbilityBalance-.*-E2E|LightYearsAbilityContentRegistrationE2E|LightYearsAbilityLoaderPublicLoadE2E' --output-on-failure
git diff --check
```

Build exit 0; CTest exit 0, **5/5 passed** in 2.98 seconds; diff check passed.
GasLite was compiled but unit tests were not executed, respecting the user's
E2E-only preference. Existing assertion constants were aligned where required;
they are not claimed as runtime acceptance evidence.

The three new suites report **40/40 assertions**:

- [First three artifact](../build/e2e-artifacts/ability-balance-first.json):
  16 checks; HullShock damage/progression, Glacial push/collision/Cryo transfer,
  Orbital formation, duration, orbit speed and contact reentry gate.
- [Second three artifact](../build/e2e-artifacts/ability-balance-second.json):
  8 checks; SunBeam damage/cleanup, Rocket L15 damage and armor mitigation before
  its new Kinetic stacks, held Inferno damage/Ignite/cleanup, typed profiles.
- [Third three artifact](../build/e2e-artifacts/ability-balance-third.json):
  16 checks; Overdrive counts/damage/AS effect/post-active cooldown, including
  an overlapping same-ID boost and external Multiply; ExecutionDrive snapshot
  and cleanup; Mine L15 damage, fixed count, cooldown tiers and stun refresh.

[Final CTest log](../build/balance-batch1-final-ctest.log) and
[build log](../build/balance-batch1-full-build.log) are local ignored artifacts.
These automated runtime cases do not establish visual playtest acceptance or
repo-wide safety. Early fixture failures were corrected (collision masks, valid
ship sprites/health, mouse-world application context, no-op level changes and
cumulative projectile counting); no gameplay assertion was removed to obtain a pass.

A semantic comparison against the initial JSON confirmed exactly eight records
changed, with all 55 IDs preserved. Inferno already matched the catalog, and all
46 abilities outside this batch retain their original records. The default
loadout and shared damage/status balance file are unchanged.
