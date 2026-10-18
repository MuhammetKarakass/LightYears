# Engine request: sparse construction-property variants

Date: 2026-10-19. Read-only engine: Vega e6ed855.

Stage 2a tried an unselected type base (`base.type=PlayerSpaceShip`, no Ship)
and Fighter/Interceptor `base.prefab` variants containing only their Ship ID.
The two-ship editor scenario placed both and ran Apply/Save/reopen successfully,
but its strict game postcheck rejected the resulting documents:

- Fighter Apply/Save omitted Ship, leaving only Render Enabled=false. The
  selector equalled the factory's default, but the game requires an explicit
  catalog choice in the authored chain.
- Interceptor Save retained Ship and Render Enabled=false but wrote
  SpriteComponent Texture and HealthComponent Max Health snapshots. These values
  belong to ships.json and must follow catalog reloads.

Evidence from the disposable build-tree asset copy is preserved in
`build/w10-ships-variant-fighter.prefab.json`,
`build/w10-ships-variant-interceptor.prefab.json` and
`build/w10-ships-variant-attempt.log`. The strict validator was not loosened.
Stage 2a ships flat type prefabs instead; no Vega sources were edited.

Requested behavior: when diffing a variant against its parent, realize that
parent with the descendant's construction selectors before capturing defaults.
Retain an explicitly authored construction selector even when equal to the
factory default. Apply, Apply All and scratch Save must share this rule, keeping
Ship plus intentional Render Enabled and no constructor snapshots. Keep ordinary
authored parent overrides and component occurrence semantics intact.

Proposed API: no new game-facing API is required if the existing
`PrefabLibrary::TypeDefaults`/`Diff` authoring paths can preserve selector intent
and construct a parent comparison with merged selectors. Vega owns the exact
implementation and its generic two-kind editor coverage. The game should adopt
that engine commit and rerun the existing two-ship sparse postcheck before
converting shipped assets to variants.
