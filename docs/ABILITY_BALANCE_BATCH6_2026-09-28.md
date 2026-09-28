# Ability balance batch 6 — catalog entries 47–50

## Change contract

- **Objective:** implement or verify ZeroDrag, GravityAnomaly, NullPulse, and FrostMaelstrom against their catalog rows.
- **Owners:** family runtime/fallback contracts and the four records in `LightYearsGame/assets/content/data/abilities.json`. Shipped numeric balance belongs to JSON; the family validators define structural requirements.
- **Invariants:** preserve IDs, default loadout, presentation and registry wiring, unrelated ability records, and the final five old-progression abilities (#51–55).
- **Proof:** focused diff and JSON structural review followed by one final `LightYearsGame` build. GasLite/E2E runs are outside this user-requested batch verification.

## Result

ZeroDrag (#47) already matches its 12-second cooldown, 5-second duration, and fourteen +0.1 duration/−0.2 cooldown upgrades. NullPulse (#49) already matches its radius 500, base damage 10, 1-second stun with a nonlinear Energy Power bonus capped at 0.65 seconds, and fourteen +2 damage/−0.25 cooldown upgrades.

GravityAnomaly (#48) now gains both field radius +5 and cast range +5 per level, alongside cooldown −0.1, duration +0.03, and slow magnitude +0.005. The catalog's runtime prose listed radius growth while its progression list omitted it; the progression list is now explicit. Shipped JSON no longer adds pull strength or projectile speed per level. The ability validator accepts exactly those five attributes consistently across fourteen steps while keeping the JSON as the numeric owner. Its existing MaxHealth scaling remains radius +0.2 and duration +0.0025 per point.

FrostMaelstrom (#50) already matches its Cryo tick-damage behavior, AttackPower and excess EnergyPower scaling, radius scaling, six-second active time, and fourteen −0.2 cooldown upgrades. Its fallback actor definition now provides the duration and damage required by its own validator, matching shipped JSON.

## Verification and limit

- `abilities.json` still has 55 records; only `Ability.Control.GravityAnomaly.Basic` changed in this batch.
- `git diff --check` passed.
- `cmake --build D:\LightYears\build --target LightYearsGame --parallel 6` succeeded with the MSVC environment (`VSLANG=1033`, exit 0). The catalog and setting-contract objects were explicitly rebuilt because this checkout previously had incomplete Ninja header dependency records. Local log: `build/balance-batch6-game.log`.
- GasLite/E2E were not run under the user's batch instruction. Compilation and JSON structure do not establish gameplay acceptance.
