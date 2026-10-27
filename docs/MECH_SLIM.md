# MECH-SLIM implementation record

Scope: `D:\LightYears-mech`, branch `mechanics-slim`, based on `game-main` `c4f0a5bb`.
This implements only the two decisions in [MECHANICS_LAYER_PLAN.md](MECHANICS_LAYER_PLAN.md), section 6.

## Change contract

- Objective: move owner death-prevention dispatch and deferred damage to shared owners, preserving shipped behavior.
- Allowed files: the shared combat list/runtime, SpaceShip and PlayerSpaceShip integration, module death-prevention clients, deferred-damage contracts/loader/applier, CMake, the requested new test, and this task's documentation/logs.
- Canonical owners: `CombatRuntime` owns both lethal-hit lists; `SpaceShip` owns deferred damage; module runtimes own their registrations, resolved values, charges, and policy.
- Invariants: Parasite Hull before Last Stand; unchanged revive charge spending, sink and bonus; unchanged Neural Hijacker eligibility/rewards; unchanged deferred-damage validation, selection, arithmetic, source attribution, debt lifetime and tick order. No engine edits or existing test expectation edits.
- Proof: inspect the diff, build `LightYearsGame` and the five selected test targets with `VSLANG=1033`, then run only the five named CTest cases once after the final build.

## Part 1: owner death prevention

`LethalHitInterceptorList` is reused for both roles, with integer priorities (lower runs first). Existing source-side registrations default to priority zero and retain their existing order. Equal priorities retain registration order. Each entry has a monotonically allocated registration ID; token reset removes only that ID, so a stale token cannot erase a newer entry. Tokens hold weak state references, so they can safely outlive the list.

`CombatRuntime::GetOwnerDeathPreventions()` exposes a separate target-side instance next to `GetLethalHitInterceptors()`. `SpaceShip::ResolveLethalDamage` resolves the target instance first; any non-None result ends dispatch. Only if it declines does the source-side list run. The list skips entries removed during an earlier callback, preventing a copied callback from reaching a receiver whose token was reset.

Module policy priorities are declared once in `module/rules/OwnerDeathPreventionPriorities.h`: Parasite Hull = 100, Last Stand = 200. No module identifiers or policy enter the shared list.

`LegendaryParasiteHullRuntime` owns its registration. `Replace` keeps the existing registration, cooldown and possession while installed; empty rules clear it, and `Clear`/destruction reset the token. `ShipModuleApplier`, the existing owner of Last Stand placement/charge state and `TryReviveOwner`, owns the revive registration. Rebuild resets and replaces that token only when an active placement has revive rules, excluding rebooting placements. Shutdown disconnects both prevention callbacks before other teardown callbacks, preserving the old shutdown guard. The member tokens also remove their entries on destruction. `TryReviveOwner` itself is unchanged.

`ShipModuleApplier::TryPreventOwnerDeath` and the player override disappear. There was no player-specific dispatch left; the feature-specific callbacks now supply that behavior through the base path.

### Existing order dependency

Before this change, a dying player never consulted its attacker's source list: it tried Parasite Hull, then Last Stand. An enemy receiving a lethal player hit consulted the player's source list, letting Neural Hijacker return ConvertedKill and award a kill while keeping the captured enemy alive. These are distinct directions of damage, not two competing player-module callbacks on one hit.

Now owner prevention still wins before any source interception. A surviving player therefore cannot also produce a source-side capture. If the player cannot be saved, the source list is consulted, but the shipped Neural Hijacker rejects the player because it requires an `EnemyActor` target. Enemy deaths still reach the player's source list, with the same eligibility, capture and reward logic. Generic source callbacks on player targets become reachable as explicitly requested by the shared base contract; no shipped such callback changes its result.

## Part 2: deferred damage

`gameplay/damage/DeferredDamageSpec.h` owns the value-only `{ fraction, seconds }` contract. `gameplay/damage/DeferredDamage.h/.cpp` owns the mechanic and its pending debts. Neither directly includes module headers. `SpaceShip` includes and owns `DeferredDamage`, retaining `GetDeferredDamageRuntime()` for its callers. `SetSpec` replaces `SetRule`.

The module definition and resolver carry the shared spec directly. The side-feature loader still parses the same JSON fields and performs exactly the same validation; the resolver still selects the rule with the greatest fraction. The applier pushes the resolved spec. Existing debts continue at their captured rates when the spec changes or the module is removed.

The old `module/rules/ModuleDeferredDamageRuntime.h/.cpp` and `ModuleDeferredDamageRule` definition are removed. CMake already discovers all runtime sources recursively, so the moved source remains in `LightYearsGameLib` (linked into `LightYearsGame`) without a hand-maintained runtime source entry. CMake explicitly registers the new test target and case.

## Changed files

Paths below are relative to `LightYearsGame/`, except the documentation entries.

- Shared combat: `include/gameplay/combat/CombatRuntime.h`, `include/gameplay/combat/LethalHitInterceptors.h`, `src/gameplay/combat/LethalHitInterceptors.cpp`.
- Ship integration: `include/spaceShip/SpaceShip.h`, `src/spaceShip/SpaceShip.cpp`, `include/player/PlayerSpaceShip.h`, `src/player/PlayerSpaceShip.cpp`.
- Module prevention: `include/gameplay/module/ShipModuleApplier.h`, `src/gameplay/module/ShipModuleApplier.cpp`, new `include/gameplay/module/rules/OwnerDeathPreventionPriorities.h`, `include/gameplay/module/rules/legendary/LegendaryParasiteHullRuntime.h`, `src/gameplay/module/rules/legendary/LegendaryParasiteHullRuntime.cpp`.
- Deferred damage: new `include/gameplay/damage/DeferredDamageSpec.h`, moved `include/gameplay/damage/DeferredDamage.h` and `src/gameplay/damage/DeferredDamage.cpp`; removed old `include/gameplay/module/rules/ModuleDeferredDamageRuntime.h` and `src/gameplay/module/rules/ModuleDeferredDamageRuntime.cpp`.
- Module data: `include/gameplay/module/rules/ModuleSideRules.h`, `include/gameplay/module/ModuleDefinition.h`, `include/gameplay/module/ModuleResolver.h`, `src/gameplay/content/ModuleLoaderSideRules.cpp`.
- Verification: `CMakeLists.txt`, new `tests/OwnerDeathPreventionTests.cpp`.
- Documentation: `docs/MECH_SLIM.md`, `docs/MECHANICS_LAYER_PLAN.md` at repository root. Generated logs live under `outputs/`.

## Verification

Build completed successfully (exit 0), including the full first engine/dependency build inside this worktree. No engine source was edited. All five focused CTest cases passed, each run once after that build. No existing test file or expectation was changed. No git add/commit/checkout/reset/stash/push was performed.

Commands, from `D:\LightYears-mech` in PowerShell:

```powershell
$env:VSLANG='1033'
cmd.exe /d /c 'tools\vega-build.cmd --target LightYearsGame LightYearsEpicDefenseModulesE2ETests LightYearsLegendaryTakeoverModulesE2E LightYearsModuleSidePoolsE2ETests LightYearsGasLiteTests LightYearsOwnerDeathPreventionTests --parallel 4 > outputs\mech-slim-build.log 2>&1'
ctest --test-dir build -R '^(LightYearsEpicDefenseModulesE2E|LightYearsLegendaryTakeoverModulesE2E|LightYearsModuleSidePoolsE2E|LightYearsPlayerShipLifetime|LightYearsOwnerDeathPreventionTests)$' --output-on-failure --output-log D:/LightYears-mech/outputs/mech-slim-ctest.log
git diff --check
```

| CTest case | Result | Time |
|---|---|---|
| LightYearsOwnerDeathPreventionTests | Passed | 0.11 s |
| LightYearsPlayerShipLifetime | Passed | 0.81 s |
| LightYearsLegendaryTakeoverModulesE2E | Passed | 1.26 s |
| LightYearsEpicDefenseModulesE2E | Passed | 0.78 s |
| LightYearsModuleSidePoolsE2E | Passed | 1.13 s |

CTest exit 0, total reported real test time 4.42 s. Only the player-lifetime case was run from `LightYearsGasLiteTests`.

Logs: [build](../outputs/mech-slim-build.log), [focused CTest](../outputs/mech-slim-ctest.log), [structural checks](../outputs/mech-slim-structural-checks.log), [diff check](../outputs/mech-slim-diff-check.log).

Structural checks confirmed the deferred source is exactly the original after include/type/member/comment substitutions, `TryReviveOwner` is unchanged after newline normalization, no existing tracked test changed, old runtime names are gone from code, the shared deferred mechanic has no module includes, and the moved source compiles exactly once into `LightYearsGameLib`.

## Ambiguities

1. Reuse the exact existing list type for two separately owned instances rather than add a template or duplicate implementation: their callback and result contracts are identical.
2. Lower integer priority runs first; use 100/200 for the two module clients. Equal priorities preserve existing source registration order.
3. Keep Last Stand's token in `ShipModuleApplier`, its existing runtime state owner, rather than split charge/sink/bonus ownership into another runtime.
4. Preserve `GetDeferredDamageRuntime()` as the ship accessor; name the shared mechanic `DeferredDamage`, its data `DeferredDamageSpec`, and its setter `SetSpec`.
5. Preserve deferred damage's existing module-trigger suppression payload flags: those are damage-pipeline compatibility data, not a dependency on module runtime code.
6. Recheck snapshot entry IDs during resolution so token removal immediately protects a destroyed receiver. This addresses the required RAII lifetime guarantee and is covered by the new test.
7. The requested base contract makes source callbacks reachable for dying player targets after prevention declines. Preserve shipped Neural Hijacker behavior through its unchanged EnemyActor eligibility check.
8. Clear the Parasite Hull runtime at the beginning of applier shutdown, alongside the revive token, so teardown callbacks cannot invoke prevention after the old shutdown guard would have rejected it. Its Clear function makes no owner callbacks.
