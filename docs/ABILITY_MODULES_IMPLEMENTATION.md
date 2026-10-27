# Ability modules — implementation and review

## Change contract

Objective: implement Timing Protocol, Autocast Protocol, Element Core and Fortune
Protocol through the existing Ultimate Protocol placement, resolution and activation
path. Worktree: `D:\LightYears-ability`, branch `ability-modules`.

Allowed owners: module content/loading/placement/resolution and developer acquisition;
ability activation, input, aim, invocation snapshots and delayed hit payloads; the
damage conversion boundary and its existing type readers; ability/module HUD;
focused E2E registration; this implementation record and catalogue status.

Canonical owners:

- `modules.json` owns all module numbers, element acquisition pool and English text.
- `ModuleDefinition` / `ModuleLoaderAbility` own typed module rules and validation.
- `ModuleLoadout` owns instance slot/element identity and acquisition RNG.
- `ShipModuleApplier` binds resolved slot rules to the ability-layer mechanics.
- `AbilityActivationPolicy` is ASC-owned and implements reusable timing input,
  automatic activation, aim overrides, random activation power and feedback state.
- `GameAbility` captures main-effect/cooldown/conversion snapshots before behavior
  activation. `AbilityUseHistory` / `AbilityInvocationRuntime` retain recorded power.
- `gameplay/damage/DamageTypeOverride` converts hit tags and status payloads before
  outgoing and incoming damage-type readers. `DamageHitGroup` carries the snapshot.
- Existing ability actors carry the same hit group through delayed deliveries;
  custom Energy Spear, Nano Plague, Orbital Drones and Scorch Drive paths do so too.
- `EpicOffenseRuntime` counts converted loadout types for Monochrome Core.

Invariants: no rarity, family, side feature, progression or zone bonus; one cell;
one ability module per key; instance binding survives movement/copy/respawn;
`ResolveMainEffectMultiplier` combines power sources additively; existing loadout,
content numbers, manual aim and behavior outside a carrying slot are preserved.
No engine source changes and no git index/branch/commit operations.

Proof: `AbilityModulesE2E.cpp` uses real ships, authored abilities, loadouts, the ASC,
projectile damage, status effects, Player respawn and HUD models/rendering. The focused
regression list below covers the directly affected boundaries. Compilation alone is
not gameplay evidence.

## Decisions / Ambiguities

1. Timing's large window is centered at 70% of the 1.2-second bar: **0.640–1.040 s**.
   The small window is **0.765–0.915 s**. Endpoints are inclusive. At or after the
   duration, timeout is a miss; the small result replaces the large result.
2. Each key has an independent bar. Twin Rack's first/second banks also have
   independent bars and feedback. Another key, another bank or an unrelated normal
   key never cancels a pending bar. A module attached to a key applies to both banks.
3. Eligibility is checked at the first press and again before delayed activation.
   Losing eligibility, changing/removing the source module, replacing the ability,
   death, possession or portal transit cancels that pending bar without a queued
   later cast. Cooldown/blocked first presses follow the existing input path.
4. Timing's second press is the ability's normal held input thereafter. Timeout
   activates with the current physical held state, so an already released hold
   ability follows its normal release behavior. This does not create an automatic
   full-charge rule for Timing.
5. Manual Autocast use receives the same x0.75 as automatic use. Only automatic
   activations override aim. The nearest target's point/direction is snapshotted;
   the ship itself is not rotated. Defense's damage-only cast with no enemy uses
   ship-forward aim. Channeled aim retains this activation's override.
6. Autocast range uses resolved ability range/length/maximum-distance attributes
   first, then referenced actor data, then ability/actor effect radii (excluding
   collision radii). `Common.Range` takes precedence within a record. A behavior
   can supply its canonical range; Closed Circuit reads `maximumDeliveryRange`.
   No spatial range data means the JSON default **600 units**. All actor references
   are derived from authored content, including custom behavior spawns. Existing
   ability scaling/scoped modifiers are resolved before the query.
7. Defense's one-second window counts actual positive damage received, including
   shield damage. Dealing damage, rejected/invulnerable hits and zero damage do not
   open it. The window is not consumed by one defensive cast.
8. Movement requires nonzero held movement input at activation, uses its normalized
   world direction, and ignores inertial velocity. A started automatic charge keeps
   that direction and charges to full even if movement input is later released.
   Virtual held input survives physical key-up. Existing charge/focus locks do not
   cancel their own already-started charge; locks still prohibit new activations.
9. Automatic activation is checked once per ASC tick after cooldown processing;
   it does not backfill multiple casts after a long frame. Active abilities finish
   their existing lifetime before another automatic activation.
10. Element acquisition is uniform over the five JSON elements and uses a separate
    loadout-owned seedable RNG. The developer inventory explicitly selects each
    element through a validated inventory-only setter. Installed elements cannot
    be rerolled. Moving/copying/respawning keeps the value; reinstalling from cargo
    retains it while choosing the new key just like Ultimate Protocol.
11. Conversion carries the sum of the original hit's applicable damage-status
    stacks into the new type, subject to the destination's existing status rules.
    It does not add a status to an originally status-free hit. Energy removes all
    four status counters. Type-specific shield/armor payload properties are reset
    to the destination type's canonical defaults. Ability-owned field ticks carry
    their conversion too; status DoT ticks and module-generated secondary hits keep
    their own identity. Non-damaging effects are unchanged.
12. Fortune rolls power, samples the clamped Luck probability, then rolls again
    only on success. The ship-owned `std::mt19937` can be seeded through
    `GetActivationPolicy().SetRandomSeed`. Recorded invocations retain the roll
    without rolling again. An activation attempt rejected by the normal gates
    rolls nothing; a behavior that rejects after preparation can consume RNG.
13. Timing feedback lasts **0.9 s**; Fortune feedback lasts **1.2 s**, both from JSON.
    Green means a successful timing window or Fortune >= x1; red/orange means a
    miss or Fortune < x1. Fortune is formatted to two decimals on the ability icon.
    Timing uses a bottom-of-screen bar per visible slot, blue large window, green
    small window and white moving marker.
14. The module screen exposes one **Unique** category for rarity-less items while
    preserving separate code classes. Entries show **Ability module** or
    **Connector Unique** as sub-kind. Element Core displays a name suffix and a
    stored-element tooltip. Standard ability shells retain the Ability category.

## Focused verification

Build from this worktree, with `VSLANG=1033` set by the wrapper after `vcvars64`:

```powershell
cmd /c "set VSLANG=1033&& tools\vega-build.cmd --target LightYearsGame LightYearsAbilityModulesE2E LightYearsAbilityModuleUltimateProtocolE2ETests LightYearsUniqueModulesE2E LightYearsModuleScreenTests LightYearsEpicOffenseModulesE2ETests LightYearsLegendaryPowerModulesE2E LightYearsLegendaryTwinRackE2E LightYearsHUDMigrationTests --parallel 4 > outputs\build-final.log 2>&1"
```

Selected tests:

- New `LightYearsAbilityModulesE2E`: four module contracts and real runtime paths.
- Required `LightYearsAbilityModuleUltimateProtocolE2E`, `LightYearsUniqueModulesE2E`,
  `LightYearsModuleScreen`: existing ability-module, connector and module-screen contracts.
- `LightYearsEpicOffenseModulesE2E`: converted-type readers and the ordinary damage path.
- `LightYearsLegendaryPowerModulesE2E`: Primed/captured additive power integration.
- `LightYearsLegendaryTwinRackE2E`: per-bank input routing and retained instances.
- `LightYearsHUDMigrationAbility`: ordinary ability-bar geometry, revision and lifecycle.

```powershell
ctest --test-dir build -R '^(LightYearsAbilityModulesE2E|LightYearsAbilityModuleUltimateProtocolE2E|LightYearsUniqueModulesE2E|LightYearsModuleScreen|LightYearsEpicOffenseModulesE2E|LightYearsLegendaryPowerModulesE2E|LightYearsLegendaryTwinRackE2E|LightYearsHUDMigrationAbility)$' --output-on-failure --output-log D:/LightYears-ability/outputs/test-focused.log
```

The full CTest suite is intentionally excluded. Results and the complete file
manifest are recorded below after the final run.
