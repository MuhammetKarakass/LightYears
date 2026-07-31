# Light Years Roguelite Design Document

## 1. Purpose

Light Years is currently a vertical space shooter built on a custom C++17/SFML engine. The goal is to evolve it into a roguelite space action game with chapter-based node progression, biome-themed encounters, ship building, persistent meta progression, and eventually open arena/twin-stick combat.

This document defines the current design direction before implementation. It is intentionally written as a living design/technical document: specific numbers can change during prototyping, but the hierarchy and system ownership should remain stable unless we deliberately revise them.

## 2. Current Project Assessment

The current codebase is not a pure data-oriented ECS. It is an Actor-based custom engine with partial component-style composition.

Current core architecture:

- `Application` owns the SFML window, main loop, world switching, global/game timers, physics stepping, audio updates, and cleanup cycles.
- `World` owns actors, pending actor spawning, HUD overlays, stage sequencing, and event dispatch.
- `Actor` is the main gameplay object base class. It owns transform, sprite, optional physics body, collision layer/mask, dynamic point lights, and tick/render behavior.
- `GameStage` models timed or wave-based gameplay phases.
- `HealthComponent`, `ShipRuntime`, `CombatRuntime`, `AbilitySystem`, primary-weapon handlers, and presentation components provide component-like behavior through composition.
- `PlayerManager` and singleton managers carry player/audio/assets/timer state.

Strong points:

- The main loop, world lifecycle, actor spawning, timers, audio, physics, HUD, and asset loading already exist.
- The game already has ships, enemies, bullets, pickups, health, shield, boss phases, asteroid hazards, particle explosions, parallax-style backgrounds, and shader-based point lights.
- `GameStage` can be reused as the basis for encounter recipes.
- SFML is sufficient for the next version if we add a better camera/render pipeline layer.

Weak points:

- `LevelOne` and several enemy behaviors still carry vertical-shmup assumptions. The current bootstrap world is `ArenaTestLevel`, where camera follow, world/screen-space separation, thrust/drift movement, and arena-boundary feedback are already present.
- `LevelOne` is a hardcoded linear sequence, not a flexible run progression system.
- There is no persistent profile, run-state, chapter-state, node graph state, biome definition system, or currency model.
- Visual rendering works but lacks a centralized render pipeline. Shader logic is split between `AssetManager`, `ShaderManager`, and actor-level light rendering.
- Content definitions are partly data-driven, but many encounter and spawn behaviors are hardcoded in stage classes.

Design conclusion:

The existing engine should be evolved, not replaced. The roguelite layer should be modeled above combat worlds as run/chapter/node state, while combat entities remain actors.

## 3. Target Game Summary

Light Years will become a roguelite space action game where each run is made of sequential Chapters. Each Chapter contains a branching node graph. The player travels between Nodes, chooses risk/reward routes, fights encounters, earns run-specific and meta currencies, improves weapons, and eventually reaches a Boss Node. Defeating the boss completes the current Chapter and opens the next Chapter.

Important terminology:

- Use `Bölüm` or `Chapter` for the largest progression unit inside a run.
- Use `Node Kümesi` or `Biyom Bölgesi` for a group of nearby nodes sharing a biome theme.
- Use `Node` for a single point/event in a Chapter.
- Do not model `Biome` as a separate hierarchy level above/below Chapter. A biome is a theme/flavor layer applied to node clusters inside a Chapter.

## 4. Core Game Loop

High-level loop:

1. Player starts a run from the main menu.
2. Player selects an available ship.
3. A Chapter is generated with roughly 12 Nodes.
4. Nodes are grouped into visible biome regions.
5. Player starts at the Chapter entry Node.
6. Player travels to connected Nodes.
7. Each travel action increases Threat.
8. The selected Node opens an encounter, shop, stat choice, skill modifier choice, or boss fight.
9. On completion, the player returns to the Chapter graph.
10. Player may revisit previous Nodes.
11. At Threat Level above 3, revisited/old Nodes can spawn enemies again.
12. When the player reaches and defeats the Boss Node, the Chapter closes.
13. A new Chapter opens with a new node graph, biome distribution, and higher baseline challenge.
14. A run ends when the player dies or completes the final planned Chapter.
15. Meta rewards are applied after the run.

The first playable milestone should not attempt the full loop. The first target should be a single Chapter with basic node travel, simple Threat increase, one or two encounter types, and a Boss Node.

## 5. Design Pillars

### Meaningful Route Choice

The player should constantly make small but meaningful route decisions. A short path to the boss reduces Threat but gives fewer rewards. A longer route creates stronger builds but raises danger.

### Build Expression Through Ships And Weapons

Each ship should feel different from the start through stat differences, one passive ability, and one unique weapon. During a run, weapon-linked modifiers and upgrades create build identity.

Primary weapon direction:

- Every ship owns a unique primary weapon as part of its ship definition.
- The primary weapon is the ship's default combat identity, not a temporary pickup.
- The weapon fires continuously while the player holds fire. It should feel cooldown-free, but technically uses fire-rate attributes such as `shotsPerSecond`.
- Core weapon variables include fire rate, damage, projectile speed, projectile size/collision radius, area radius, lifetime/range, spread, and projectile count.
- Weapon variables should be compatible with the GAS-Lite attribute model so ship upgrades, run modifiers, skill nodes, and future effects can change current values without replacing the weapon class.

### Biomes Change The Feel Of Encounters

Biomes should not be cosmetic only. A biome affects enemy pool, music, reward pool, visual palette, atmosphere, hazard tendencies, and node generation bias.

### Readable Intensity

Combat can become intense, but the player must understand what is happening. Enemy silhouettes, projectile colors, threat warnings, boundary warnings, boss telegraphs, and reward choices must stay readable.

## 6. Chapter System

A Chapter is the top-level unit of progression inside a run.

Chapter properties:

- Contains one complete node graph.
- Contains approximately 12 Nodes in the first target design.
- Has one clear entry/start Node.
- Has one Boss Node that ends the Chapter.
- Can contain multiple biome regions.
- Has baseline difficulty parameters.
- Has generation rules for node type distribution and biome clustering.

Chapter completion:

- Defeating the Boss Node completes the Chapter.
- Once the boss is defeated, the current Chapter closes.
- The player does not continue exploring that Chapter after boss completion.
- A new Chapter opens after completion.

Initial Chapter scope:

- Start Node.
- Several combat Nodes.
- At least one reward/stat Node.
- At least one skill modifier Node.
- Optional shop Node.
- One Boss Node.
- Two biome regions maximum for the first version.

## 7. Biome System

A Biome is not a separate progression layer. It is a theme and content ruleset assigned to a group of Nodes inside a Chapter.

Biome controls:

- Enemy pool.
- Music.
- Reward pool.
- Node type generation tendency.
- Visual palette and atmosphere.
- Hazard tendency.
- Optional combat rules or environmental modifiers.

Biome region behavior:

- A Chapter can have multiple biome regions.
- Each region owns a cluster of connected Nodes.
- Boundaries are clear at the Node cluster level.
- There is no gradual biome transition in the first version.
- Node visuals should make biome ownership obvious.

Example biomes:

### Nebula

Theme:

- Blue/purple palette.
- Energy fog, glowing particles, softer visibility.

Gameplay tendency:

- Projectile-heavy enemies.
- Shield and energy upgrade rewards appear more often.
- Possible future modifier: reduced long-range visibility or distorted enemy silhouettes.

### Asteroid Belt

Theme:

- Dense asteroid fields, dusty space, heavier debris.

Gameplay tendency:

- Higher asteroid/hazard density.
- Collision damage matters more.
- Armor, max HP, and durability rewards appear more often.

### Solar Ruins

Theme:

- Burnt orange/gold palette, broken megastructures, solar flares.

Gameplay tendency:

- Higher elite enemy rate.
- Fire/burn style weapon modifiers appear more often.
- More aggressive encounter pacing.

## 8. Node Graph System

The Chapter graph is a branching progression structure. It should not be a single straight line.

Graph rules:

- Nodes are connected by routes.
- The player can choose between multiple valid paths.
- Old Nodes can be revisited.
- The graph leads generally toward the Boss Node, but the player can take side routes.
- Side routes offer rewards, shops, stat growth, or skill modifier opportunities.
- Longer routes increase Threat and risk.

Node visual states:

- Unvisited.
- Available.
- Current.
- Completed.
- Revisited.
- Locked/unreachable.
- Boss.
- Special encounter.
- Biome ownership indicator.

Node types:

- Combat Node: standard fight, usually grants a reward.
- Elite Combat Node: harder fight, better reward.
- Shop Node: spend Gold for items or upgrades.
- Stat Growth Node: improve ship/player stats for the current run.
- Skill Modifier Node: improve weapon-linked modifiers using Scrap or node reward rules.
- Pure Combat Node: fight with no direct reward or upgrade. Lower Threat increase.
- Boss Node: final fight of the Chapter.

Node travel:

- Threat increases only when travelling between Nodes.
- Threat does not increase from combat duration by default.
- The exact travel UI and path cost presentation can be iterated later.

Revisiting old Nodes:

- Revisit is allowed.
- Completed Nodes are not automatically safe forever.
- At Threat Level above 3, old/completed Nodes can spawn enemies again.
- Revisited Nodes may have reduced/no reward unless explicitly designed otherwise.

## 9. Threat System

Threat is the run pressure system for a Chapter.

Core rule:

- Threat increases only when the player travels between Nodes.

Threat range:

- Threat Level is 1 to 5.
- Internally it can be stored as a float and displayed as a level.

Initial simple curve:

- Start at Threat value `1.0`.
- Travelling to a pure/no-reward combat Node adds `+0.3`.
- Travelling to a reward-bearing Node adds `+0.6`.
- Elite, shop, stat, skill, and boss travel costs can be tuned later.
- Displayed Threat Level is clamped to 1-5.

Possible first mapping:

- `1.0 - 1.99`: Threat Level 1.
- `2.0 - 2.99`: Threat Level 2.
- `3.0 - 3.99`: Threat Level 3.
- `4.0 - 4.99`: Threat Level 4.
- `5.0+`: Threat Level 5.

Threat effects:

- Enemy health/damage/spawn count can scale lightly with Threat.
- At Threat Level above 3, completed/revisited Nodes can spawn enemies again.
- Higher Threat can increase elite chance, hazard density, or encounter pacing.

Open tuning question:

- Should Threat reset per Chapter, partially carry over, or convert into next Chapter baseline difficulty? First implementation should reset per Chapter unless later playtests suggest otherwise.

## 10. Currency System

The game will start with three currencies. The exact count can change later if complexity becomes too high.

### Stardust

Purpose:

- Meta progression.
- Ship development between runs.
- Unlocking or improving permanent ship options.

Persistence:

- Permanent profile currency.
- Earned from run completion, boss kills, achievements, or milestone rewards.

### Gold

Purpose:

- Run-internal economy.
- Used in shops to buy items, temporary upgrades, healing, or build tools.

Persistence:

- Run-only currency.
- Usually lost at run end.

### Scrap

Purpose:

- Skill/weapon modifier improvement during a run.
- Used to upgrade weapon-linked modifiers.

Persistence:

- First version should treat Scrap as run-only.
- Future versions can allow some Scrap conversion into Stardust if desired.

Design caution:

- Stardust should not make the game feel like mandatory grind.
- Gold and Scrap should create route/build choices inside a run.

## 11. Ship Building System

First version recommendation:

- Do not start with a fully modular ship construction system.
- Start with selectable ship archetypes.

Each ship should have:

- Base stat differences.
- One passive ability.
- One unique weapon.
- Different starting health/speed/fire behavior.

Primary weapon model:

- A ship's unique weapon is defined by data through a `PrimaryWeaponDefinition`.
- `PrimaryWeaponDefinition` should hold projectile data, firing pattern data, muzzle offsets, sound/VFX hooks, and a GAS-Lite-friendly weapon attribute group.
- A generic primary-fire ability recipe owns a `FireWeaponAction`; runtime firing state lives in ability execution.
- `PlayerSpaceShip` should not contain weapon timing, spread, projectile-count, or modifier application logic.
- Future vertical or arena weapons should use the same ability/action/effect model, with new reusable delivery implementations added when a weapon is not projectile-based.

Ability slot direction:

- The player's primary weapon is fixed by the selected ship and is not replaced by run drops.
- Run drops should build the player's active ability set around the fixed primary weapon.
- First target ability set size is 4 active ability slots.
- The player should be able to compose those slots freely from compatible run drops.
- Early example pool: 2 shield-type abilities and 2 offensive abilities, such as beam and rocket.
- Shield examples can be different defensive behaviors, not just stronger/weaker versions of the same button.
- Offensive examples can include projectile, rocket, beam, burst laser, or other delivery types.
- This system should support player choice during a run: the player can decide whether to build more defensive, more offensive, or a mixed skill set.
- Primary weapon upgrades can still exist, but they modify the fixed primary weapon rather than replacing it.

Active abilities:

- Active abilities are part of the future skill-slot system.
- They should share the GAS-Lite activation pipeline with primary fire and compose reusable actions, effects, registered gameplay actors, and family-level visuals instead of adding a lifecycle class per ability.
- The first skill-slot implementation should stay small: activate, cooldown/duration, optional charges, and GameplayEffect/attribute hooks.
- Runtime activation should use recipe data, one generic `AbilityInstance`, reusable action variants, and gameplay effects.
- Avoid one large tag/enum-driven activator that contains every behavior. Tags describe metadata, events, modifier targets, UI, and blocking rules; reusable actions/effects/actors implement behavior.

Why this approach:

- It gives strong identity early.
- It is easier to balance than full modular construction.
- It fits the existing `ShipDefinition`, generic combat runtime, and `PlayerSpaceShip` input model.
- It leaves room for future modular systems without blocking the first roguelite milestone.

Example ship archetypes:

### Fighter

- Balanced movement and health.
- Passive: small fire-rate bonus after avoiding damage.
- Unique weapon: focused forward laser or fast pulse cannon.

### Bulwark

- Higher health, lower speed.
- Passive: shield effects last longer.
- Unique weapon: slower heavy projectile or close-range spread.

### Wraith

- High speed, lower health.
- Passive: bonus damage shortly after entering combat or after a dodge-like movement event.
- Unique weapon: angled or piercing shots.

## 12. Skill Modifier System

First version:

- Skill development is split between fixed primary weapon upgrades and run-dropped active skills.
- The ship's primary weapon stays fixed for the run unless a very explicit future system says otherwise.
- Active skill drops fill a limited skill set, initially 4 slots.
- The player should choose and replace skills during a run to shape the build.
- First example pool: two shield variants and two offensive variants such as beam and rocket.

Modifier examples:

- Three-way shot gains piercing.
- Laser damage increases but cooldown gets longer.
- Projectiles split after travelling a distance.
- Frontal weapon gains burn effect.
- Shield pickup causes a shockwave when depleted.
- Bullets gain extra damage against elite enemies.

Modifier acquisition:

- Skill Modifier Nodes.
- Elite rewards.
- Shop purchases using Gold.
- Upgrade using Scrap.
- Run drops that grant or replace active skills.

Data model direction:

- Weapon modifiers should be data-driven enough to avoid hardcoding every build path into `PlayerSpaceShip`.
- Primary-weapon attributes should receive run modifiers through GAS-Lite style effects/modifiers; do not add hardcoded firing paths to `PlayerSpaceShip`.
- Active skills should use the same GAS-Lite activation model as primary fire: common activation/cooldown/duration/state handling plus composable actions, effects, and registered actor families for shield, dash, boost, rocket, beam, and similar skills.

## 13. Meta Progression

Meta progression lives outside a single run.

Permanent profile should eventually track:

- Unlocked ships.
- Ship upgrade levels.
- Available upgrade/modifier pools.
- Stardust amount.
- Major Chapter or boss milestones.
- Potential cosmetic unlocks.

Run-only state should track:

- Current Chapter.
- Current Node.
- Completed/revisited Nodes.
- Threat value.
- Current Gold and Scrap.
- Current ship.
- Current health/lives if lives remain part of the design.
- Current weapon state and modifiers.
- Temporary stat upgrades.

Recommendation:

- Build `ProfileState` and `RunState` as separate models.
- Avoid putting all persistent/meta data into `PlayerManager`.
- `PlayerManager` can remain responsible for the current combat player, but not the entire roguelite profile.

### 13.1 Hangar And Ship Constellation

The main menu should include a meta progression area where the player spends Stardust on ships. This should be presented as a Hangar-style screen, not as a generic upgrade list.

Reference direction:

- Shape of Dreams uses Stardust as a permanent progression currency.
- Its progression identity comes from constellation/star-style upgrade planning.
- It combines broad/shared upgrade categories with character-specific progression.
- It allows build/loadout choice instead of simply activating every purchased upgrade at once.
- It supports resets/refunds, which is important because build experimentation should not feel permanently punishing.

Light Years adaptation:

- The menu has a `Hangar` screen.
- Inside Hangar, each ship has a `Ship Constellation` or `Star Grid`.
- Stardust is spent on permanent ship development.
- The player can buy Stars, upgrade Stars, and equip a limited number of Stars before a run.
- Owned Stars are permanent unlocks.
- Equipped Stars are the active pre-run meta build.
- Not every owned Star should be active at the same time. This keeps choices meaningful and prevents meta progression from becoming only flat stat inflation.

Recommended Hangar sections:

- `Ship Select`: choose the ship for the next run.
- `Ship Overview`: stats, passive, unique weapon, mastery level.
- `Ship Constellation`: spend Stardust and equip Stars.
- `Loadouts`: save multiple Star setups per ship.
- `Unlocks`: view locked ships, unlock conditions, and required Stardust or achievements.

### 13.1.1 Hangar Visual Target V1

The first Hangar version should aim for a 2.5D ship presentation rather than real 3D rendering.

Hangar V1 visual target:

- 2.5D sprite presentation.
- Glow around ship body and engines.
- Parallax hangar background.
- Mouse-reactive ship tilt.
- Shader highlight on ship surface.
- Engine particles.

Rationale:

- This gives the player a stronger sense of owning and inspecting their ship without requiring a full 3D renderer.
- It fits the current SFML-based rendering stack.
- It can reuse existing sprites, point lights, particle effects, and shader infrastructure.
- It keeps implementation scope reasonable while still making the Hangar feel more premium than a flat menu.

Future visual upgrades:

- Normal-map based fake lighting for stronger 3D surface feel.
- Pre-rendered 3D sprite sheets from Blender for rotating ship previews.
- Real OpenGL 3D preview only if the Hangar later needs true model inspection.

### 13.2 Star Categories

The first implementation should use a small number of clear categories.

Recommended shared categories:

- `Destruction`: damage, fire rate, projectile behavior, elite/boss damage.
- `Survival`: max HP, shield strength, recovery, out-of-bounds grace improvements.
- `Engineering`: weapon handling, cooldowns, pickup efficiency, Scrap/Gold economy.
- `Navigation`: movement, acceleration, drift control, camera comfort, arena boundary tolerance.

Recommended ship-specific categories:

- Unique weapon upgrades.
- Passive ability upgrades.
- Ship identity modifiers.
- Tradeoff Stars that change playstyle.

Examples:

- Fighter Star: `Afterburn Volley` - unique weapon fires one extra mini-shot after sustained firing.
- Fighter Star: `Clean Vector` - acceleration improves after not taking damage for a short time.
- Bulwark Star: `Reinforced Hull` - max HP increases, but acceleration slightly decreases.
- Bulwark Star: `Shield Capacitor` - shield pickups grant extra shield duration.
- Wraith Star: `Phase Spark` - first shot after a high-speed turn deals bonus damage.

### 13.3 Equipped Stars And Loadouts

To avoid passive power creep, purchased Stars should not all be active automatically.

Proposed first model:

- Each ship has a limited number of Star slots.
- Slots are divided into shared and ship-specific slots.
- Early ships start with few slots.
- Stardust upgrades can unlock additional slots gradually.
- Each ship can have multiple saved loadouts.

Example first version:

- 3 shared Star slots.
- 2 ship-specific Star slots.
- 1 flexible slot unlocked later.
- 3 saved loadouts per ship.

This gives the player a reason to return to the Hangar before a run and prepare for different strategies.

### 13.4 Ship Mastery

Each ship should have a mastery track separate from Stardust spending.

Mastery can increase by:

- Completing Nodes with that ship.
- Defeating Chapter bosses.
- Finishing runs.
- Clearing higher Threat encounters.

Mastery rewards can include:

- Small Stardust bundles.
- New Star unlock availability.
- New ship-specific modifier options.
- Cosmetic variants.
- New loadout slots.

Important constraint:

- Mastery should unlock options more than raw power.
- Stardust should be the main spending currency.

### 13.5 Stardust Earning

Stardust sources should reward both success and experimentation.

Recommended sources:

- Chapter boss defeat.
- Run completion.
- First-time biome discovery.
- First-time ship milestone.
- Ship mastery level rewards.
- Challenge objectives.
- Rare relic/artifact appraisal after a run.

Avoid:

- Making Stardust income so low that one failed run feels wasted.
- Making Stardust upgrades mandatory before the game becomes enjoyable.
- Making all early Stars tiny filler nodes.

Early progression should give the player visible choices quickly. A good first target is that the player can buy or upgrade at least one meaningful Star after an average early run.

### 13.6 Respec And Experimentation

The Hangar should support experimentation.

Recommended model:

- Unequipping Stars is free.
- Changing loadouts is free.
- Refunding purchased Stars can cost a small Stardust fee or use a limited reset item.
- First reset for each ship should be free.

Rationale:

- Build experimentation is central to the genre.
- Permanent mistakes discourage trying new ships.
- A light refund cost preserves commitment without trapping the player.

## 14. Arena/Twin-Stick Direction

The long-term combat target is not a 1280x720 bounded arena. The window/viewport can be 1280x720 or user-configurable later, but the combat area itself is larger than the visible screen.

Example:

- Viewport/window: 1280x720 initially.
- Arena/world area: approximately 3000x2000 or larger.
- Camera: follows the player using `sf::View`.
- Player: free 2D movement with acceleration and damping.
- Enemies/hazards: can enter from any direction.
- Biome atmosphere: applies across the encounter area.

Arena boundary behavior:

- The arena has legal bounds.
- The player is allowed to leave the legal area briefly.
- Leaving triggers an `Unauthorized Region` warning.
- If the player remains outside for 5 seconds, the ship is destroyed or receives lethal damage.
- Returning to the legal area clears or resets the warning timer.

Initial arena systems:

- `ArenaDefinition`: size, legal bounds, boundary grace time, visual theme.
- `CameraSystem`: follows player, applies view, handles shake/zoom later.
- `ArenaBoundarySystem`: checks out-of-bounds state and broadcasts warning/death events.
- `SpawnDirector`: spawns enemies around the player while respecting arena bounds and safe distances.

## 15. Visual And Technical Approach

Technology recommendation:

- Continue with SFML.
- Do not switch engine/library for the current transformation.
- Add a thin custom render/shader pipeline on top of SFML.

Why SFML remains sufficient:

- `sf::View` supports camera-follow arena rendering.
- `sf::RenderTexture` can support post-process passes.
- Existing actors, sprites, shaders, particles, and audio are SFML-based.
- Switching technology now would rewrite working combat systems without solving the core design problem.

Needed improvements:

- Add a world-space vs screen-space render distinction.
- Render actors with the camera view.
- Render HUD with the default view.
- Add render texture pass support for biome post-processing.
- Centralize shader ownership and usage.
- Add biome visual themes that drive palette, background layers, particles, and shader uniforms.

First visual pipeline target:

1. World actors render to a world target or directly under world view.
2. Optional biome post-process shader applies tint/vignette/noise.
3. HUD renders in screen-space.
4. Warning overlays, node graph UI, and menus stay screen-space.

## 16. Architecture Plan

The roguelite hierarchy should be outside the actor/ECS-like combat layer.

Recommended ownership:

- `RunState`: owns current run data.
- `ChapterState`: owns the current Chapter graph and Threat.
- `NodeGraphState`: owns Nodes, edges, biome regions, and visited/completed state.
- `BiomeDefinition`: data definition for biome content and visuals.
- `NodeDefinition`: data definition for a Node.
- `EncounterDefinition`: data passed into combat to spawn enemies, hazards, rewards, and objectives.

Combat ownership:

- `World` remains responsible for active actors, HUD, physics, and local combat lifecycle.
- `GameStage` can evolve into encounter stages or be used by encounter directors.
- Enemies, bullets, pickups, hazards, particles, and background objects remain `Actor`-based.
- Generic attribute/ability/effect mechanics live in the static
  `SpaceAbilitySystem` library under the `sas` namespace. Ship-specific
  attribute IDs, combat formulas, balance data, content and presentation remain
  in `LightYearsGame`.

Data flow:

1. Player selects a connected Node.
2. `ChapterState` applies travel cost and updates Threat.
3. Selected Node produces an `EncounterDefinition`.
4. Combat World loads with that definition.
5. Combat resolves success/failure.
6. Rewards and node completion state are applied to `RunState`.
7. Player returns to Chapter graph.

Do not:

- Do not make every Node an `Actor` unless it is only for visual display.
- Do not store permanent progression in combat actors.
- Do not make biome logic dependent on a specific level class like `LevelOne`.

## 17. Systems Impact

### Application

Expected change: medium.

Changes:

- Window size can remain fixed for now but should not define arena size.
- Rendering should support camera/world view and HUD/default view.
- Later, settings can expose resolution and fullscreen/windowed options.

### World

Expected change: medium to large.

Changes:

- Needs camera/view ownership or access to a camera system.
- Needs clean world-space actor rendering and screen-space HUD rendering.
- Needs support for combat worlds generated from encounter definitions.

### Actor

Expected change: medium.

Changes:

- `IsActorOutOfWindow()` should stop being the main despawn rule.
- Arena-based despawn/lifetime rules should replace screen-based assumptions.
- Transform and render behavior can mostly remain.

### PlayerSpaceShip

Expected change: large.

Changes:

- Remove screen-edge movement clamp.
- Add free 2D movement.
- Add acceleration and damping.
- Separate movement direction from aim/fire direction.
- Later support mouse/gamepad twin-stick controls.
- Integrate ship passive and unique weapon definitions.

### Enemy And Boss AI

Expected change: large.

Changes:

- Vertical-entry enemies need arena behaviors.
- Add pursue, strafe, orbit, ranged kite, charge, and hazard-control patterns.
- Boss should be redesigned as an arena boss with movement, telegraphs, and area control.

### Spawn System

Expected change: large.

Changes:

- Replace top-of-screen spawns with player-relative and arena-bounds-aware spawns.
- Spawn enemies outside immediate player range but within active combat space.
- Use biome, Node type, and Threat to select enemy pools and spawn intensity.

### Collision And Physics

Expected change: medium.

Changes:

- Box2D can stay.
- World bounds and cleanup rules need to be defined.
- Physics body lifecycle must remain safe as larger encounter spaces and revisits increase object count.

### HUD/UI

Expected change: medium.

Changes:

- HUD must be screen-space.
- Add Threat display, currencies, current Node context, and Unauthorized Region warning.
- Node graph UI should be a dedicated screen/HUD, not mixed into combat HUD.

### Background And Visuals

Expected change: medium to large.

Changes:

- Current vertical scrolling background should become arena/world-space friendly.
- Add biome-specific background layers, particle ambience, palette, and optional post-process.
- Keep existing point lights and particles but centralize theme control.

## 18. Existing Content Reuse

Can mostly stay:

- Main loop.
- Asset loading.
- Audio manager.
- Timer manager.
- Base actor lifecycle.
- Health component.
- Reward pickup concept.
- Explosion particles.
- Basic shader/light idea.
- HUD widget base classes.

Can be adapted:

- Primary-weapon delivery handlers and projectile actors.
- Enemy base class.
- Asteroid spawner.
- Boss phase concept.
- Infinite/Chaos stage concepts.
- Background layer concept.
- Reward factories.

Needs redesign:

- Player movement.
- Enemy movement and spawn assumptions.
- Boss spatial behavior.
- LevelOne linear stage sequence.
- Screen-bound despawn and movement limits.
- Progression structure.
- Currency and build state.

## 19. Risk Register

### World-Space vs Screen-Space Confusion

Camera support will break UI if HUD rendering is not separated from world rendering. Mouse coordinates, UI clicks, actor positions, and camera transforms need clear conversion rules.

Mitigation:

- Establish world view and UI/default view early.
- Add helper functions for screen-to-world and world-to-screen.

### Vertical Shmup Assumptions

Many systems assume top-down vertical flow.

Mitigation:

- Introduce arena systems before full roguelite progression.
- Convert one enemy at a time.

### Spawn And Despawn

Camera outside does not mean object outside gameplay. Screen-based cleanup will delete valid objects or keep invalid ones.

Mitigation:

- Use lifetime, arena distance, encounter ownership, and player distance rules.

### Boss Redesign Scope

The existing boss is a vertical shmup boss and may not feel good in an arena.

Mitigation:

- Build one simple arena boss prototype before adding multiple Chapters.

### Progression Complexity

Chapter graph, Threat, currencies, ship progression, and modifiers can expand quickly.

Mitigation:

- First version should use simple numeric Threat, three currencies, one Chapter, limited Node types, and a small ship roster.

## 20. Recommended Implementation Order

### Milestone 1: Arena Foundation

Goal:

- Prove the combat format before building the full roguelite shell.

Work:

- Add larger arena definition.
- Add camera follow using `sf::View`.
- Separate world-space and screen-space rendering.
- Remove player screen clamp.
- Add free movement with acceleration/damping.
- Add Unauthorized Region warning and 5-second death rule.

Success criteria:

- Player can move inside a 3000x2000 style area.
- Camera follows smoothly.
- HUD stays fixed.
- Leaving bounds shows warning and kills after grace period.

### Milestone 2: Arena Encounter Prototype

Goal:

- Make existing combat work in the new arena.

Work:

- Add player-relative spawn director.
- Convert one or two enemies to arena behavior.
- Update bullet direction logic for free aim or ship-facing fire.
- Add basic cleanup rules.

Success criteria:

- Enemies can spawn from multiple directions.
- Combat is playable in a large area.
- Existing rewards and explosions still work.

### Milestone 3: Chapter Graph Prototype

Goal:

- Create the first playable Chapter loop.

Work:

- Add `RunState`, `ChapterState`, `NodeGraphState`.
- Generate a small graph.
- Add Node travel and Threat increase.
- Add revisiting.
- Add boss Node completion closing the Chapter.

Success criteria:

- Player can choose Nodes.
- Threat increases on travel.
- Boss completion opens next Chapter placeholder.

### Milestone 4: Biome Regions

Goal:

- Make node clusters feel distinct.

Work:

- Add biome definitions.
- Assign biome regions to node clusters.
- Use biome to influence enemy pool, music, reward pool, visual palette, and node type bias.

Success criteria:

- Different node clusters clearly belong to different biomes.
- Combat content changes based on biome.

### Milestone 5: Ship And Build Layer

Goal:

- Add roguelite build identity.

Work:

- Add ship archetypes.
- Add one passive and one unique weapon per starting ship.
- Add Gold, Scrap, Stardust.
- Add weapon-linked skill modifiers.
- Add simple shop/stat/skill Nodes.

Success criteria:

- Route choices affect build.
- Different ships feel meaningfully different.
- Run rewards and meta rewards are separated.

## 21. Open Questions

These are intentionally unresolved:

- Should Threat reset every Chapter, partially carry over, or convert into the next Chapter baseline?
- How many Chapters should a full run contain in the first public version?
- Should completed non-boss Nodes always be revisitable, or should some Node types become one-way?
- Should revisited Nodes at high Threat give any reward after enemy respawn?
- How much of Stardust progression should be power vs unlock variety?
- Should Gold and Scrap ever convert into Stardust at run end?
- Should ship passive abilities be purely numeric or behavior-changing from the first version?
- Should the first arena boss be a redesigned version of the current boss or a new boss built for arena combat?
- Should the Unauthorized Region kill instantly at 5 seconds or deal extreme damage over time after 5 seconds?

## 22. Current Design Decisions

Confirmed decisions:

- Use Chapter/Bölüm terminology for the top-level run progression unit.
- Biome is not a hierarchy level; it is a theme/content layer applied to node clusters.
- Node graph is branching, not linear.
- Old Nodes can be revisited.
- Boss Node completion closes the Chapter and opens the next one.
- Threat increases only through Node travel.
- Threat range is 1-5.
- Initial simple travel values: pure/no-reward Node `+0.3`, reward-bearing Node `+0.6`.
- At Threat Level above 3, old/completed Nodes can spawn enemies again.
- Three initial currencies: Stardust, Gold, Scrap.
- Stardust is for meta progression and ship development.
- Gold is for run shops/items/upgrades.
- Scrap is for weapon-linked skill modifier improvement.
- First skill system is weapon modifier based.
- Separate active skill system is postponed.
- Ships start with stat differences, one passive, and one unique weapon.
- Ship active abilities are postponed.
- Biome boundaries are clear by node cluster.
- No gradual biome transition in the first version.
- Long-term combat format is open arena/twin-stick, not vertical shmup.
- Arena is larger than the viewport.
- Viewport/resolution can be configurable later but is not a first concern.
- Player can leave arena bounds briefly.
- Unauthorized Region warning lasts 5 seconds before death/lethal consequence.
- Arena player movement uses a thrust/drift model, not Box2D force-driven movement.
- Arena player W/S input is ship-relative forward/reverse thrust.
- Arena player A/D input uses adaptive screen strafe: when the ship faces mostly vertical, D moves toward screen-right and A moves toward screen-left; when the ship faces mostly horizontal, D moves toward screen-up and A moves toward screen-down; intermediate angles blend smoothly between those directions.
- Arena player mouse input controls ship rotation target through world-space mouse position.
- Legacy vertical movement remains separate through `ShipMovementMode::LegacyVelocity`; arena movement uses `ShipMovementMode::ThrustDrift`.
- `SpaceAbilitySystem` is an internal static library. Migration proceeds
  system-by-system in Attribute → Ability → Effect order; native plugin/DLL or
  scripting support, if selected later, is a separate adapter/runtime layer.
- The first Ability slice moves only stable handles and policy enums into
  `abilities`. Game-specific definitions, action payloads, weapon data,
  presentation and balance content remain in `LightYearsGame`.
- The next slice keeps read-only runtime observation generic:
  `sas::AbilityRuntimeSnapshot` owns an ability ID and slot instead of pointing
  at the game-specific definition.
- The definition/event slice keeps the event carrier fully SAS-owned:
  `sas::AbilityEvent` stores tags, magnitude, and typed opaque
  source/target/context references. LightYearsGame binds Actor and
  `DamageContext` without defining a second event type. Core definition
  validation runs in SAS, while content-specific validation stays in the game
  adapter.
- Mutable ability lifecycle and orchestration are SAS-owned. Input evaluation,
  activation/end, level changes, active/cooldown/duration counters, charge
  consumption/refill and snapshots live behind `sas::GameplayAbilityInstance`.
  The game `GameAbility` adapter supplies owner-tag gates, concrete
  behavior/action calls, weapons and attachments.
- Reusable ability runtime mechanisms are SAS-owned as typed, game-agnostic
  components: `AbilityBehaviorRegistry` stores typed behavior factories,
  `AbilityActionScheduler`
  advances repeated action intervals/counts, `AbilityCooldownTracker` owns
  event-trigger cooldown timing, and `AbilityCollection` owns handle allocation
  plus instance/ID/slot/passive indexing. LightYearsGame
  keeps shipped concrete behavior registration and all weapon, action payload,
  actor, damage, attachment and presentation adapters.
- The ownership audit additionally places the generic behavior lifecycle
  contract, runtime-entry storage, grant admission rules, and trigger
  matching/cooldown-key policy in SAS. Attribute scaling/list helpers and
  runtime snapshot construction are library-owned as well. The game
  `LightYearsAbilitySystemComponent`, `GameAbility`, and
  `GameAbilityActionExecutor` remain integration adapters only; they must not
  accumulate reusable policy.
- Actor access follows the component-provider pattern through the SAS-owned
  `AbilitySystemInterface`. Game actors implement the interface and expose
  their associated `sas::AbilitySystemComponent` through
  `GetAbilitySystemComponent()`; the component may remain owned by a separate
  game runtime object.
- The SAS component owns ability notification forwarding, passive counts,
  catalog-level structural validation and bulk cooldown reduction. Game
  subclasses must call the inherited component API directly instead of adding
  renamed forwarding methods or retaining a self-pointer facade. Raw ability
  runtime and mutable active-effect container getters are not part of the
  public component surface.
- Effect migration follows the same extension boundary. SAS owns effect
  handle/policies, immutable definition, resolved spec core, structural
  validation, snapshot and duration/stack/runtime-attribute state.
  LightYearsGame extends these with source ability upgrades, Actor/source
  context, DamageContext behavior hooks, visuals and shipped content.
- Active effect storage and the complete apply/stack/refresh/tick/expiry/remove
  orchestration are owned by `sas::GameplayEffectRuntimeSystem`. Modifier/tag
  binding cleanup and source-scope stacking therefore no longer live in the
  game adapter.
- Typed effect hook dispatch is owned by
  `sas::GameplayEffectBehaviorRuntime`. Game code supplies the concrete
  `DamageContext` handler signatures and presentation callbacks. The concrete
  runtime specialization is exposed by the game component; redundant handler
  aliases and default initialize/refresh callbacks are not added.
- Ability input/lifetime decisions are centralized in
  `sas::AbilityLifecycleOrchestrator`; game code supplies behavior, weapon,
  action, actor, attachment, damage and presentation adapters.
- Effect application kind, source-scope stacking match, refresh/stack state and
  duration expiry decisions are centralized in
  `sas::GameplayEffectLifecycleOrchestrator`. Game code retains typed
  Actor/DamageContext hooks and presentation cleanup.
- `sas::AbilityRuntimeSystem` owns grant/remove, handle and slot/passive
  registration, ticking, clearing and snapshots. `sas::AbilityExecution` and
  `sas::AbilityExecutionLifecycle` own the action runtime container and phase
  traversal. Game action handlers retain FireWeapon, ApplyEffect and
  SpawnActor integration.
- Active effects no longer own visual pointers. The game presentation adapter
  stores effect-handle-to-visual sidecars and synchronizes them through SAS
  runtime callbacks.
- The Attribute, Ability and Effect migration is complete at the static-library
  boundary. Active code uses explicit `sas::` contracts. Legacy comparison
  copies were deleted after zero-reference and Debug/Release verification.
- The completed Debug build links the SAS library, game and test executable;
  CTest passes `LightYearsGasLiteCore` and `LightYearsEngineLifetime` (2/2).
