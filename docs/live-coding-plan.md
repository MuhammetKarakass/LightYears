# C++ Live Coding plan (design only, 2026-10-17)

Status: nothing implemented. Produced by a read-only Sol study; see .sol/PLAN.md for scheduling.

Implement this as **game DLL replacement plus edit-world reconstruction**. Keep the per-game editor executable from ROADMAP B3. Function patching and preserving live C++ objects would require substantially more machinery.

The existing boundary is useful, but unloading is currently unsafe: `UnregisterGameModule()` only nulls a pointer; reflection and extensions retain executable callbacks; LightYears installs GAS/Lua resolvers and uses process-wide registration guards. `LoadWorld(nullptr)` also cannot clear the current world because the deferred swap requires a non-null pending world.

**1. Binary boundary and ownership**

- Keep the entry point, `EditorApplication`, project/document state, Console, viewport, selection identities and reload controller in the editor executable. `EditorMain.cpp` currently just constructs the application; `LightYearsEditorMain.cpp` chooses a statically linked module. Replace that factory call with a loader.
- Add reloadable `BlankGameLive` and later `LightYearsGameLive` DLL targets. LightYears’ DLL must contain its module, actors, components, ability implementations, catalogs and editor adapter—the game code currently spread across `LightYearsGameplayCore`, `LightYearsGameLib`, `LightYearsGameApp` and `LightYearsEditorModule`. Static libraries can remain implementation inputs, linked into this DLL only.
- Introduce a **resident runtime DLL** containing engine and VegaScript implementation, including their service/global state. Optional GAS/Sky implementations must likewise have one resident owner. Never statically link independent copies into both host and game DLL. Export the editor API implementation used by game extensions through a small resident editor-support DLL; `VegaEditorCore` can otherwise remain static in the executable.
- Separate the engine’s executable entry-point object from the shared runtime. Update `vega_add_game_editor`: it currently directly links `${GAME_MODULE}` and depends on the standalone runtime target.

Use explicit export/import annotations. Export a small `extern "C"` query/create/destroy interface containing contract version, engine ABI fingerprint and build ID. It may return the existing C++ `GameModule` interface: this is a **same-build C++ ABI**, not a portable plugin ABI. Enforce identical MSVC toolset, architecture, configuration, `/MDd` or `/MD`, iterator-debug settings and dependency versions. Destroy the module through its DLL export; keep substantive initialization outside `DllMain`.

The host retains one `EngineContext` across reloads. Its compatibility binding and legacy fallback must live in the resident runtime; prefer explicit context registration. The opening “process-wide registry” statement in `reflection.md` is superseded by the context-owned implementation documented in `engine-context.md`.

Add generation-owned registration scopes for types, serializers, schemas, commands, resolvers and extension callbacks. Destroy a generation’s registrations **while its DLL remains loaded**. Remove RTTI/type-index keys, constructor inventories and descriptor caches too; re-register bases before derived types. Reusing existing descriptors is insufficient when properties disappear. Prevent game registration from silently modifying resident descriptors without reversible ownership.

Matching CRTs do not solve lifetime problems: `std::function` destructors, shared-pointer deleters/control blocks, vtables and RTTI can all point into the DLL. Release strong **and weak** references associated with its objects before unloading. SFML, ImGui and ImGui-SFML must each have one resident implementation/context; retain the window and UI across reloads. Release game render resources with the graphics context active.

**2. Edit-mode reload transaction**

Run replacement at a main-thread frame boundary, outside ticks, rendering, callbacks and script-command execution:

1. Build and validate the candidate artifact before disturbing the world.
2. Block authored mutations and transitions. Serialize the current scene—including unsaved edits—to host-owned JSON. Preserve scene path, dirty state, camera and selected GUIDs. Abort on serialization failure. Initially refuse reload while prefab isolation or an extension has unsaved documents.
3. Suspend content polling and game hooks. Shut down extension tools while their required worlds still exist; detach render pipelines and callbacks. Cancel timers/events/jobs and join game threads.
4. Add an explicit application operation that releases **both current and pending worlds**. Destroy complete worlds, including every game actor/component, prefab staging world and cached instance. Clear selection object references, drag state, Inspector caches and histories. Reset Lua state/resolvers that can retain game code.
5. Remove generation registrations and destroy the old module. Keep its DLL mapped temporarily for rollback, with no active objects or callbacks.
6. Create/configure/register/initialize the new module under the retained context; reconstruct the edit world from JSON. Restore selection by GUID, camera and dirty state; rebuild prefab caches and Lua tooling. Only then commit and release the old DLL.

Validate every serialized actor/component type and writable property against the candidate schema. Initially reject missing types, incompatible property kinds and refused setters; newly added properties use constructor defaults. Do not silently accept a loader fallback that drops authored data.

Clear scene, prefab and extension undo/redo history on successful replacement and report that policy. Existing GUID-based commands help across ordinary Play/Stop, but do not establish schema compatibility across code generations.

For recoverable initialization/load failures, tear down candidate registrations and reconstruct using the retained previous DLL and snapshot. This requires repeatable setup/shutdown, including replacing LightYears’ `static bool registered`. Access violations and corrupted native state cannot be reliably rolled back; retain a recovery snapshot.

**Play policy:** builds may finish during Play, but installation waits until Stop restores the edit snapshot. Paused Play still counts as Play. No running abilities, physics state, timers or coroutine migration.

**3. Build and diagnostics**

Add project/build metadata for build tree, configuration, game DLL target and artifact manifest. `EditorProject.cpp` currently resolves project/export paths; it does not select a dynamic module.

A manual **Compile & Reload** action launches:

`cmake --build <build-tree> --config <configuration> --target <GameLiveTarget>`

Use a hidden child process with asynchronously drained stdout/stderr, cancellation and one build per tree. Marshal output to Console on the main thread. Compilation failure leaves the current generation untouched.

Always load a uniquely named shadow copy, never the linker output. Give generations private DLL/PDB locations and configure matching linker PDB paths; copying a DLL alone does not redirect embedded PDB information. Account for compiler PDB/object locks under debugging. Publish artifacts only after successful linking. Reject runtime/editor/dependency ABI changes with “restart required”; the live target must not overwrite resident binaries.

**4. Stages and acceptance**

- **Foundation:** resident binary ownership, exports, ABI checks, generation registration teardown and explicit world release. No reload UI until unload invariants work.
- **First shippable slice:** Blank only, manual edit-mode replacement, one reflected actor/component and a callable function; unchanged schema, no custom panels or worker threads. Include the build action and failure diagnostics.
- **Schema stage:** additive fields/defaults, strict incompatibility refusal, prefab handling and repeated reload stress.
- **LightYears stage:** audit catalog/ability factories, retained definition generations, PlayerManager, GAS debugger callbacks, Lua player resolver, render pipeline, timers and Sky/GAS tools. Add scoped shutdown before enabling reload.
- **Convenience stage:** optional debounced source watching and shorter incremental builds.

Add proposed core editor-script verbs: `compile-game`, `reload-game`, `wait-game-reload <timeout-seconds>`, `expect-game-generation <id>`, `expect-game-reload <result>` and `expect-history-empty`. Keep the command registry object stable because `EditorScript` retains a reference; remove only generation-owned commands.

A disposable fixture supplies two known DLL generations. Using existing `create`, `set-prop`, `call-function`, `expect-prop`, `expect-selected`, `expect-component` and `expect-console`, verify changed behavior, preserved GUID/property/selection, cleared history, compile failure, ABI rejection, schema refusal and repeated unloading.

Today `--editor-script` still constructs SFML/ImGui and a window. **True headless acceptance requires a new runner** using the same reload controller, command registry, explicit EngineContext and worlds without UI initialization. Retain a separate unattended GUI test for graphics and extension teardown.

**5. Risks and review allocation**

The dominant risk is an untracked callback or shared/weak control block surviving unload. Registration ownership and shutdown audits are prerequisites; counters help detect violations but cannot prove arbitrary game code safe.

Do not attempt instruction patching, vtable replacement, arbitrary binary compatibility, engine hot reload, automatic schema migration or seamless Play continuation. Native constructors and global initialization remain capable of crashing the editor.

A cheaper worker model suits bounded CMake wiring, project fields, Console plumbing, command declarations, documentation and deterministic fixtures **after contracts are fixed**. Require careful review for ABI/export ownership, registration replacement, teardown ordering, shared-pointer lifetimes, rollback and LightYears globals. Those decisions determine whether reload is safe.

Read-only inspection only; no files, git commands or builds were run.