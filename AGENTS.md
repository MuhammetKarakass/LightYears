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
