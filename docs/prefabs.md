# Prefabs

Prefabs are reusable entity assets under the active asset root. Logic remains in
attached Lua scripts; prefabs introduce no visual scripting system.

`prefabs/BigRock.prefab.json`:

```json
{
	"format": "vega.prefab",
	"version": 1,
	"name": "BigRock",
	"base": { "type": "Asteroid" },
	"properties": { "Velocity": [0, 100] },
	"components": [{ "type": "HealthComponent", "properties": { "Max Health": 300 } }]
}
```

Prefab documents use the scene entity's properties, components, removed, and
scripts fields. They have no entity GUID, entity name, or Location. The document
name is an asset display name. A base contains exactly one spawnable type or
asset-relative prefab path. Paths cannot escape the asset root.

A variant, `prefabs/HugeRock.prefab.json`:

```json
{
	"format": "vega.prefab",
	"version": 1,
	"name": "HugeRock",
	"base": { "prefab": "prefabs/BigRock.prefab.json" },
	"components": [{ "type": "HealthComponent", "properties": { "Max Health": 500 } }]
}
```

A scene instance:

```json
{
	"guid": "a0000000000000000000000000000008",
	"name": "Rock near gate",
	"prefab": "prefabs/HugeRock.prefab.json",
	"properties": { "Location": [800, 400], "Velocity": [30, 80] }
}
```

Resolution applies constructor defaults, parent documents, the prefab document,
and instance overrides in that order. The existing scene factories, reflection
validation, component policies, and script serializers then load the resulting
entity. Canonical paths detect cycles; chains are limited to 64 documents.
Missing files, invalid documents, and unknown bases return warnings. Unresolved
scene instances spawn nothing and retain their original JSON for the next save.

## Overrides and composition

`SceneEntityComponent` retains the prefab path and the realized load baseline.
Saving compares current writable properties with that baseline, omitting equal
values. GUID, entity name, and Location always belong to the instance. Values
equal to the baseline are inherited, even if explicitly authored earlier.

Components match by reflected type and zero-based occurrence, as in ordinary
scenes. Changed properties are sparse. Empty preceding entries of the same type
retain occurrence addressing. Missing occurrences produce `removed`; additional
occurrences carry `added`. Removals consume the first surviving occurrence of a
type, and property differences are calculated against the remaining occurrences.
Required and non-removable component rules still apply.

Omitted scripts inherit the complete list. An explicit scripts array defines its
membership and order, with properties inherited by script path and occurrence.
Saving a changed list includes its paths and only changed script properties.
An empty list removes every script. A null script property in an override clears
the inherited authored override, allowing the Lua declaration default to apply.
Null is consumed during resolution and is never passed to a script setter.

`ly::PrefabLibrary` loads and caches documents by path, checking modification time
and file size on access. `Reload()` explicitly clears the cache. `Resolve()`
returns the complete normalized entity and warnings; `Merge()`, `Diff()`, and
`Document()` support authoring tools. `Get()` provides the scene serializer's
library; `SetRoot()` selects its asset root. AssetManager root changes update it.
No prefab resolution starts gameplay or ticks a scratch world.

## Editor workflow

* Right-click a Hierarchy entity and choose **Create Prefab...**. The name defaults
  to the entity name. The editor atomically writes `prefabs/<Name>.prefab.json`
  and converts the entity to an instance. Undo restores the plain entity; the
  asset file remains. Existing files are protected from accidental creation overwrite.
* Prefab instances have blue Hierarchy rows. Content shows a cube icon for prefab
  assets. Drag an asset onto the viewport to place it, or use Hierarchy's
  **Create > Prefabs** menu to place it at the camera centre. Placement is undoable.
* Right-click a prefab asset and choose **Create Variant...** to create a document
  with a parent link.
* Double-click a prefab asset or click **open** in the instance's Inspector
  header. Inspector enters **Prefab** mode on a temporary instance in an unticked
  scratch world. Property, component, and script editing use that instance's
  separate history. **Save Prefab** atomically writes the asset and refreshes scene
  instances, preserving each instance's overrides, GUID, and selection.
* **Back to Scene**, replacing the prefab, and scene replacement/exit prompt to
  Save / Discard / Cancel when the prefab has unsaved changes.
* Instance property overrides are coloured blue. Right-click a property for
  **Revert to Prefab** or **Apply to Prefab**. Section context menus offer
  **Revert all** / **Apply all** for that section; the prefab header offers them
  for the complete entity, including composition and scripts. Location remains
  instance-owned. Apply writes immediately and refreshes instances. Scene changes
  are undoable; undo does not roll back the prefab file.

Applying a property or section on an instance-added component also applies that
component's membership. Reverting it when the prefab has no matching occurrence
removes the added component. Section operations enter history as a single edit.

External file changes invalidate the document cache on its next access. Use
`reload-prefabs` in automation to refresh live scene instances. Prefabs have no
nested entity hierarchy or entity-reference remapping in this version. The
Inspector uses the existing scalar/reflection and Lua property editing controls.

## Verification

`LightYearsPrefabLibrary` covers resolution, variants, repeated component
overrides and composition, sparse scene round trips, script lists and properties,
missing-data preservation, invalid bases, cycles, depth limits, path validation,
and cache invalidation. `VegaEditorScript` covers command parsing.
`VegaEditorPrefabs` runs `prefabs.vegascript` against a fresh build-tree copy of
the assets, including undo/redo, Apply/Revert, variants, scene save/reopen, and
scratch Inspector edits with instance refresh. It never writes source assets.

Verified 2026-10-15: `cmd /c tools\vega-build.cmd` exits 0. The final required
engine, Project, Scene, VegaScript, and VegaEditor group passes 30/30 in 68.05
seconds. The full suite passes 88/112 in 306.72 seconds; its 24 failure names
exactly match `build/scene-baseline-ctest.log`, including the existing RareZone
timeout and AbilityBalance sandbox failure. Logs are
`build/prefab-build-final.log`, `build/prefab-required-ctest-final.log`, and
`build/prefab-ctest-final.log`. The inspected UI capture is
`build/VegaEditor/prefab-inspector.png`.

## Implementation files

* Engine: `LightYearsEngine/include/framework/scene/PrefabLibrary.h` and
  `src/framework/scene/PrefabLibrary.cpp`; scene `Scene.h` / `Scene.cpp`;
  `SceneEntityComponent.h`; framework `World.h`, `AssetManager.h` /
  `AssetManager.cpp`; `tests/PrefabLibraryTests.cpp`; `CMakeLists.txt`.
* Editor: `VegaEditor/include/editor/EditorPrefabs.h` and
  `src/EditorPrefabs.cpp`; `EditorApplication.h` / `.cpp`, `EditorCommands.h` /
  `.cpp`, `EditorIcons.h` / `.cpp`, `EditorScript.h` / `.cpp`;
  `src/EditorScenes.cpp`, `src/EditorPanels.cpp`, `src/EditorContentPanels.cpp`,
  `src/EditorScriptParser.cpp`; `CMakeLists.txt`.
* Automation: `VegaEditor/tests/EditorCommandTests.cpp`, `EditorScriptTests.cpp`,
  `RunPrefabsTest.cmake.in`, `scripts/prefabs.vegascript`, and `scripts/README.md`.
* Documentation: this file and `docs/scene-authoring.md`.
