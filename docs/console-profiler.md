# Console and Profiler

Open either dockable panel from **View**. Their visibility, docking and Clear on Play
option are saved in `vega_editor_layout_v5.ini`. Reset Layout restores docking.

The Console keeps the latest 5,000 records, dropping the oldest. Info (including
Trace/Debug), Warning and Error (including Fatal) filters show raw record counts.
Channel and text filters combine with those toggles. Message search uses ImGui's include/
exclude syntax. Collapse combines identical consecutive records, including source
location; hidden records still break a run. Select a row for full text and source,
or right-click it to copy. Auto-scroll follows new messages while already at the
bottom. Clear on Play is optional. The status bar shows recent warnings/errors for
six seconds and the current buffer's error count; click it to focus Console.

Logging subscriptions supplement the legacy SetSink callback, file and stdout.
Callbacks execute outside the logger mutex. Captured records own their source
strings; subscription callbacks retain shared buffer ownership during removal.
The UI shares immutable records and caches filtered row references by revision;
unchanged frames do not scan messages. Closed panels read constant-time cached
counts for the status bar without taking a row snapshot.
Lua `print(...)` and `log(...)` report as Info on Lua, with tab-separated arguments
and source line. Load/runtime errors retain Lua's path, line and traceback. Scene
and prefab warnings use Scene; editor command/status errors use Editor.

Profiler starts paused. **Record** captures the last 300 frames; **Pause** freezes
inspection. Click the frame graph to select a frame and pause. Guide lines mark
16.6 and 33.3 ms. The graph clips hitches at 100 ms; hover to see the actual duration.
Sort the scope table by name, calls, total/self/average/maximum
time. Counters include actors, bullets, particles and submitted/culled render items.
Frame duration includes the frame limiter; scope timings show actual phase work.
Self time excludes nested scopes on the same thread. Names must be stable literals.
Scopes finishing on worker threads join the frame active at completion; no call
tree or thread timeline is retained. Each frame permits 1,024 distinct name pointers.

Capture to file writes `build/Saved/profile.json` (the active build root), containing
versioned `vega.profile` JSON with frame times, scope statistics and counters.
Script captures may choose another path; relative paths resolve under build/Saved.
Asset directories are rejected. Export is synchronous and can hitch briefly for a
large Debug capture. This is a statistics snapshot, not Chrome trace JSON.
Draw-call counts are unavailable in the current renderer.

Profiling is compiled into Debug and Release engine/editor builds and costs one
relaxed atomic check per disabled scope. Configure `-DLY_WITH_PROFILING=OFF` for
shipping builds; exports continue to copy the selected game build as before.

Editor script commands:

```text
clear-console
expect-no-errors
expect-console Info "hello"
expect-console Warning "missing"
expect-console Error "bad.lua:2"
profiler on
wait 20
expect-profile-scope "World Tick"
profiler off
capture-profile "profile.json"
diagnostics on
```

`diagnostics on|off` opens/closes both panels for screenshot and cost comparisons.
Expect-console searches raw retained records, independent of UI filters; levels are
case sensitive. Expect-no-errors checks retained Error/Fatal records since Clear.
Expect-profile-scope searches completed frames in the retained window.
`VegaEditorConsoleProfiler` copies assets into the build tree, creates Lua fixtures
there, runs Play, validates JSON and writes `build/VegaEditor/Saved/console-profiler.png`.

## Editor audio

**View > Mute audio** silences play-mode output without stopping playback.
It defaults off and is saved per user in `%LOCALAPPDATA%/Vega/editor-settings.json`
(`$HOME/.config/Vega/editor-settings.json` on other platforms), outside project
files. `mute-audio true|false` sets the same preference in editor scripts;
`expect-mute-audio true|false` checks it. Scripts normally leave user settings
alone; `VEGA_EDITOR_SETTINGS` selects a disposable settings file for restart
tests. An environment, command-line or automation mute takes precedence over
the preference, so toggling it off cannot make an automated run audible.

