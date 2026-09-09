# Light Years Agent Instructions

## Code discovery

<!-- codebase-memory-mcp:start -->
This project uses `codebase-memory-mcp` to maintain a knowledge graph of the
codebase. Prefer graph tools over grep/glob/file search for code discovery:

1. `search_graph`
2. `trace_path`
3. `get_code_snippet`
4. `query_graph`
5. `get_architecture`

Use grep/file search for literals, configuration values, non-code files, or
when the graph is insufficient.
<!-- codebase-memory-mcp:end -->

## Ability presentation architecture

All new abilities and evolve variants must follow section **3.0.1 Ability
presentation profile kontratı** in
[`docs/PROJECT_DOCUMENTATION.md`](docs/PROJECT_DOCUMENTATION.md).

Mandatory rules:

- Never recreate a global `VisualConfig.h` or `AbilityVisualStructs.h`.
- `AbilityActorDefinition` carries one presentation reference:
  `presentationProfileId`. Do not add separate visual, telegraph, explosion,
  trail, sound, or evolve presentation IDs to the generic actor definition.
- Keep presentation IDs, definitions, profile types, content construction, and
  registration under `presentation/ability/<ability-family>/`.
- Resolve profiles through `PresentationProfileRegistry<ConcreteProfile>`.
  Do not introduce `std::any`, a catch-all variant, or a universal visual
  struct.
- Keep only genuinely reusable presentation primitives under
  `presentation/ability/common/`.
- A value-only evolve registers another profile of the existing family type.
  An evolve with structurally different presentation gets its own feature-local
  profile/actor/handler inside the same ability family instead of adding many
  optional fields and flags to the base family profile.
- Register shipped profiles in `RegisterGameAbilityPresentationContent()`
  before actor-definition validation or spawning.
- Add tests for registration, typed-registry isolation, invalid/missing profile
  rejection, spawn/lifecycle, telegraph cleanup, and impact/explosion behavior
  as applicable.

## Process and Normative Rules

- Priority: correctness/invariants > scope containment > build-and-test evidence > documentation.
- A mandatory change contract before edit: objective, allowed files, canonical owner, invariants, proof. Agents must first inspect only direct owners and the closest existing feature; if that identifies a coherent integration path, they must proceed using that established pattern; they stop/report only when direct owners conflict or no safe integration path exists after focused inspection.
- Do not touch unrelated dirty work; no unrelated rename/refactor; do not silently alter balance/loadout/behavior/visuals/public contracts.
- Default runtime ability slot mapping has one owner only: `LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp`. Per-ability fallback config is not a runtime slot change.
- Follow `docs/IDENTITY_AND_CONTRACT_RULES.md` and do not duplicate `GameplayTag`, `AttributeId`, `ContentId`, enum, or registry-selector sources of truth.
- New runtime behavior/effect/actor/presentation needs all necessary contract/registry/content/CMake wiring. New compiled runtime source must link to `LightYearsGame` and `LightYearsGasLiteTests`, or use their shared source list.
- JSON/config must match C++ contracts and loader validation.
- Before completion: diff inspection; build `LightYearsGame`; for runtime/registry/CMake/content/loadout changes build and execute `LightYearsGasLiteTests`; report exact command/outcome; compilation is not gameplay evidence.
- Fix errors from exact diagnostics using the smallest correction; search old names after rename.
- Follow `docs/AGENTS.md` and never duplicate complete current-loadout tables across documents.

### Flash implementation workflow

- For implementation, bug-fix, and refactoring requests, Codex owns requirements interpretation, architecture, algorithms, state/lifecycle design, class and file boundaries, contracts/tags/JSON/presentation/registration/CMake planning, acceptance criteria, and test strategy.
- Gemini Flash owns implementation and must create or edit every required code, configuration, content, and test file. This includes complete feature classes such as abilities, contracts, runtime actors, typed presentation profiles, registrations, and tests; Flash is not limited to tiny mechanical patches.
- After Codex establishes a coherent design, it automatically launches and manages Flash. The user must not need to prepare prompts, run delegation commands, or repeatedly request continuation.
- Codex delegates coherent bounded packages with resolved design decisions. Do not ask Flash to discover unresolved architecture or redesign the feature from scratch.
- After each package, Codex reviews the scoped diff and integration, runs the required builds and tests, and delegates focused corrections to Flash until the acceptance criteria pass. Flash completion claims are not proof.
- Do not assign a new ability to a runtime slot or report it complete until its required content and behavior tests pass, unless the user explicitly requests otherwise.
- Only one Flash process may edit a workspace at a time.
- A package is one complete feature or one logically failing subsystem, never arbitrary single-file splitting.
- After direct-owner inspection identifies the integration path, Flash must make its first mutation within 45 seconds and must not search transcripts/history/unrelated docs.
- If no mutation occurs in 45 seconds or it repeats inspection, Codex terminates the turn and retries once with a clarified feature decision.
- Corrections are opened only from concrete diff/build/test evidence and cover the failing subsystem, not a one-file command.

