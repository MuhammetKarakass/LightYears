# Documentation maintenance

Repository-wide engineering and architecture rules are supplied by the active
repository instructions. This file only adds rules for the Markdown tree.

For source discovery, prefer the codebase-memory graph in this order:
`search_graph`, `trace_path`, `get_code_snippet`, `query_graph`, then
`get_architecture`. If the graph is unavailable or insufficient, use `rg` or
file search for literals, configuration, JSON and Markdown.

Ability presentation documentation must continue to describe the binding
[§3.0.1 Ability presentation profile contract](PROJECT_DOCUMENTATION.md#301-ability-presentation-profile-kontratı): one `presentationProfileId` on `AbilityActorDefinition`,
feature-local typed profiles under `presentation/ability/<family>/`,
`PresentationProfileRegistry<ConcreteProfile>`, no global visual struct,
`std::any` or catch-all variant, and registration before validation/spawn.

- Keep runtime facts in `PROJECT_DOCUMENTATION.md` and the current content
  inventory in `CURRENT_IMPLEMENTATION_CATALOG.md`.
- Keep dated experiments, balance decisions, rejected ideas and the roadmap in
  `BALANCE_AND_ROADMAP_NOTEBOOK.md`.
- Keep long-term product direction, chapter/biome/threat/hangar/meta design
  and milestone intent in `Designs.md`.
- Do not copy a complete content table between root documents and vault
  notes. Link to the owner document and keep vault indexes limited to scope,
  source and verification status.
- Preserve historical decisions with an explicit date and status. Do not turn
  a registration count, source reading or old test result into end-to-end
  gameplay evidence.
- Use repository-relative Markdown links and update the vault inventory and
  Existing Documentation Index when a document is moved, renamed or removed.
- Before removing a redundant note, search all Markdown links and frontmatter
  source fields, move any unique useful content to its owner, and leave a
  short replacement link where a vault source path is part of the note
  metadata.
