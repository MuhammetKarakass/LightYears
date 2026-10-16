# Autosave and crash recovery

Vega periodically snapshots dirty scenes, the open prefab, the Data Editor's JSON
document and all dirty Content Authoring catalogs. The default interval is two
minutes. Set **File > Project Settings > Autosave interval (seconds)**, or set
`editor.autosaveSeconds` in `vega.project.json`. Integers from 0 to 86400 are
accepted; 0 disables periodic capture while retaining manual recovery.

Snapshots are atomic writes beneath `<asset root>/.vega/Autosave/<session>/`.
Exports omit `.vega` directories, including recovery files, Trash and tooling.
They never replace authored files and do not clear dirty flags or undo history.
During Play, scene autosave captures the authored scene from before Play.
Untitled scenes use the recovery name `scenes/untitled.scene.json`. Scenes opened
outside the selected asset root cannot be autosaved; Console reports the error.
Capture runs only at the configured deadline, with no per-frame filesystem scan.

Each editor holds an OS session lock. A normal shutdown marks the session clean;
after a crash, the released lock and missing clean marker identify recoverable
files. A concurrently running editor is excluded. On startup, a recovery dialog
offers **Recover** per file with a JSON change count and up to eight changed JSON
pointers compared with the current disk file. Missing or unreadable originals
are identified explicitly. Damaged snapshots and escaping paths are skipped.

**Recover** opens an unsaved editor buffer. Save or discard any dirty target
buffer first, then review the recovered document and explicitly Save it. A
recovered content catalog uses Content Authoring when available; a removed
catalog can be reviewed in the Data Editor. JSON saves still refuse concurrent
disk changes. A recovered buffer is immediately copied into the new session,
so another crash before the next deadline still offers recovery.

Use **File > Recover Autosaves...** to browse retained snapshots, including those
from clean sessions and already recovered files. **Discard snapshot** removes
only the selected snapshot. Recovering acknowledges the old crash prompt without
deleting its snapshot. There is no automatic retention pruning yet; discard old
snapshots manually. Optional module tools with independent document stores (for
example Sky authoring in a separate presentation root) are not captured by the
core autosave service.

Editor automation supports:

```text
autosave-now
expect-autosave scenes/main.scene.json
expect-autosave data/unchanged.json false
simulate-unclean-exit
# On a subsequent launch with the same disposable asset root:
expect-recovery scenes/main.scene.json true
recover-autosave scenes/main.scene.json
```

`simulate-unclean-exit` leaves the session marker unclean on orderly script exit;
it does not terminate the process abruptly. Both expectation commands accept an
optional `true|false`, defaulting to true. Recovery chooses the newest matching
snapshot; the UI also lists each originating session. Scripts using source-tree
asset roots disable autosave metadata writes. Tests use build-tree project copies.

Coverage: `VegaEditorAutosaveStore` checks concurrent sessions, atomic replacement,
crash detection, diff summaries, damaged snapshots, path refusal, explicit save
and stale JSON conflicts. `VegaEditorAutosaveRecovery` launches BlankEditor three
times against a disposable Blank project, exercising periodic capture and scene,
prefab and JSON recovery. `VegaEditorContentWorkspace` covers invalid intermediate
catalog recovery and validation; `LightYearsProjectSettings` covers intervals.
