# Ability System debugger

Open **View > Ability System**, then Play. The panel follows the primary selected
actor; an empty selection falls back to the registered player. An actor without
an ASC displays a message. Pause freezes the cached view, including history and
events; selecting a different actor while paused takes one initial snapshot.

Abilities show runtime bindings (0 = passive/unbound, 1 = primary, 2–5 = slots
1–4), content ID/name, level, state, recharge, charges and passive status. Effects
show source identity, duration, stacks, modifiers, runtime magnitudes/intervals
and granted tags. Attributes show base/current/delta; hover an ID for the last
10 seconds of sampled values. Tag parent counts aggregate their descendants.
Every section has a search filter. Visibility, docking, refresh interval and
section collapse state are saved in the editor layout.

Snapshots refresh every 100 ms by default (20–2000 ms configurable). Closed
panels perform no snapshot work. The event ring holds the last 100 gameplay
events, activations and effect applications/removals per ASC, timestamped in
wall-clock seconds since Play. Starting Play clears the session. Logging is
enabled only by the editor Play session.

**Debug** controls mutate the live ASC through its public API: apply/remove
effects, add/remove counted tags, set levels, reset cooldowns and grant charges.
Mutations require unpaused Play and are discarded on Stop. Effect definitions
come from the content catalog. Policy-only effects use the first authored
ability effect spec when available; otherwise their content defaults apply.
Owner/source-dependent behaviors may require context that debug apply cannot
provide and can refuse application. Source is an opaque runtime identity, not
an actor name. Custom activation guards and behavior-specific costs may still
refuse an ability shown as ready; inspection does not invoke activation guards.

`sas::AbilitySystemComponent::BuildDebugSnapshot()` composes existing runtime
snapshots with copied attributes, counted tags and event history. The game
installs `sas::AbilitySystemDebugView` actor/player/content/period adapters and describes
game-specific activation gates. The engine and editor include no additional
game headers for the debugger. Snapshots own their data and contain no ASC or
actor pointers. `Editor Ability System` and `GAS Snapshot` profiler scopes expose
rendering and sampling costs separately.

The `VegaEditorGasDebugger` CTest runs `gas-debugger.vegascript`, verifies a
ready → cooldown → ready ability, applies the shipped Barrier buff and slow effect, checks
attribute/tag changes, inspects while paused, removes the effect and stops.
It writes a panel capture and before/after 300-frame profiles under
`build/VegaEditor/Saved`. Parser coverage is in `VegaEditorScript`;
`SpaceAbilitySystemDebugView` covers snapshot ownership, event bounds, session
clearing, counts and state precedence.
