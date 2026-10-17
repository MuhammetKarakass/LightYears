# Wave 6, step 2: reflected connections and lists

Implemented GUID-backed ActorRef properties, homogeneous scalar/reference lists,
scene/prefab persistence and remapping, Inspector authoring, viewport overlays,
Lua conversion, script commands, the Blank patrol/target demo, and child type
subtitle alignment. Engine/editor code remains independent of LightYearsGame.

## API and persistence

```cpp
vega::ActorRef GetTarget() const { return mTarget; }
void SetTarget(const vega::ActorRef& value) { mTarget = value; }
const std::vector<sf::Vector2f>& GetPatrol() const { return mPatrol; }
void SetPatrol(const std::vector<sf::Vector2f>& value) { mPatrol = value; }

// In VEGA_REFLECT:
VEGA_ACTOR_REF("Target", GetTarget, SetTarget).RequiredType("BlankPlayer");
VEGA_LIST("Patrol", GetPatrol, SetPatrol).Speed(1.f);

// Runtime: weak cache, null-safe, no ownership of the referenced actor.
if (auto target = mTarget.Resolve(*GetWorld())) { /* use the live actor */ }
```

Lists support bool, int, float, Vec2, Color, string, enum and ActorRef. Enum lists
use `.Labels(...)`; reference lists support `.RequiredType(...)`. Full-list
validation precedes the setter. The maximum length is 4,096.

```json
{
  "Target": {"guid":"door-guid"},
  "Patrol": [[100,200],[300,200]],
  "Spawn List": [{"guid":"enemy-a"},{"guid":""}]
}
```

Null uses an empty GUID. Dangling GUIDs warn and survive saving. Forward
references resolve after all scene identities exist. Copy/duplicate remap links
within the copied set and preserve outside links, including list elements.
Prefab documents use `$prefab:` for their root and `$prefab:/child-id` for an
internal child. Placement resolves those addresses to scene GUIDs. Variants and
nested prefabs retain the same addressing. Lists override as complete values;
Apply/Revert never merges individual indices. Temporary staging GUIDs used by
undo/refresh are published together with property references and baselines.

```lua
local target = self.actor:get("Target") -- ActorHandle or nil
if target then target:set("Size", 0.5) end
self.actor:set("Target", nil)
self.actor:set("Patrol", {vec(100,200), vec(300,200)})
```

Lua list tables include `n`, their explicit length. Retain it for ActorRef lists
containing nil entries. Other lists require dense one-based keys. Colors use
`{r=...,g=...,b=...,a=...}`. Wrong-world handles and incompatible filtered targets
are refused. Lua integer conversion validates the original double, avoiding
float precision loss at int boundaries.

## Editor and automation

The Inspector offers a searchable, type-filtered dropdown, Pick in viewport,
Select target, and Add/Remove/Up/Down list rows. Mixed lists display one em dash.
Selected scene actors show dashed reference lines and draggable world-space Vec2
points. Structural operations are discrete undo commands; row/point drags merge
per gesture. Cached property values/overlays refresh by scene/selection revision;
no per-frame JSON serialization was introduced.

Script commands (Target is Actor/component on the selection, or actor name/GUID):

```text
set-ref Trigger "Target" Door
expect-ref Trigger "Target" Door
set-ref Trigger "Target" none
list-add Trigger "Patrol" 100 200
list-set Trigger "Patrol" 0 300 200
list-remove Trigger "Patrol" 0
expect-list-size Trigger "Patrol" 0
```

Indices are zero-based. Scalar syntax matches set-prop; reference elements use
a target name/GUID or none. Every mutation is one undo step.

BlankPlayer exposes Target and Patrol. Left Tile's shipped patrol script moves
through three points, then changes Right Tile's Size. The dedicated editor test
authors a patrol and connection, verifies playback/Stop, saves/reopens, creates
a connected prefab, places/copies/duplicates it, and checks each target after
reopening. Its assets are disposable build-tree copies.

## Files

Engine:

* `VegaEngine/CMakeLists.txt`
* `VegaEngine/include/framework/ActorRef.h`, `src/framework/ActorRef.cpp`
* `VegaEngine/include/framework/World.h`
* `VegaEngine/include/framework/reflection/Reflection.h`, `ReflectionMacros.h`
* `VegaEngine/src/framework/reflection/Reflection.cpp`
* `VegaEngine/include/framework/scene/Scene.h`, `PrefabLibrary.h`
* `VegaEngine/src/framework/scene/Scene.cpp`, `PrefabLibrary.cpp`
* `VegaEngine/tests/ReflectionRegistryGoldenTests.cpp`, `reflection-registry.golden.json`

Scripting:

* `VegaScript/CMakeLists.txt`
* `VegaScript/src/ScriptComponent.cpp`, `LuaTypes.cpp`
* `VegaScript/tests/RichPropertiesTests.cpp`

Editor:

* `VegaEditor/include/editor/EditorApplication.h`, `EditorScript.h`, `PropertyWidgets.h`
* `VegaEditor/src/EditorPanels.cpp`, `EditorGizmos.cpp`, `EditorCommands.cpp`, `EditorPrefabs.cpp`
* `VegaEditor/src/EditorScript.cpp`, `EditorScriptParser.cpp`
* `VegaEditor/tests/PropertyWidgetsTests.cpp`, `EditorScriptTests.cpp`, `MultiSelectionTests.cpp`
* `VegaEditor/tests/scripts/README.md`

Blank and documentation:

* `Templates/Blank/CMakeLists.txt`
* `Templates/Blank/Source/BlankGameModule.h`, `BlankGameModule.cpp`
* `Templates/Blank/Assets/scenes/main.scene.json`, `Assets/scripts/patrol.lua`
* `Templates/Blank/Tests/properties.vegascript`, `RunProperties.cmake`
* `docs/reflection.md`, `scene-authoring.md`, `prefabs.md`, `lua-api.md`, this report

## Golden schema

The dump deliberately adds two keys to each of the 66 existing property records:
`elementKind: null` and `requiredType: ""`. List records in future registries
will contain their actual element kind. Existing 20 type records, property
order, kind IDs, labels, ranges, speeds, categories, setter policies and other
data remain identical. The golden test passes. No existing game or engine
property was renamed or added; the new demo fields belong to Blank's registry.

## Verification, 2026-10-16

* `cmd /c tools\vega-build.cmd`: exit 0, including runtime asset synchronization.
  [Build log](../build/wave6-build-final.log).
* `ctest --test-dir build -j 8 --timeout 300 --output-on-failure`: nonzero for the known failures;
  **128/152 passed**, 24 baseline failures, **164.00 seconds**.
  Every required engine, Reflection, VegaScript, Scene, Project, Prefab,
  Diagnostics, SpaceAbilitySystem, module/dependency, Blank, Sky, Hierarchy and
  VegaEditor test passed. [Full test log](../build/wave6-ctest-final.log).
* Compared all 149 existing test statuses with
  `build/hierarchy-ctest-final.log`: **no existing status changed**. The failure
  set is identical: 23 pre-existing gameplay failures, including
  `LightYearsRareZoneRulesE2E`'s 120-second per-test timeout, plus the known
  sandbox failure `LightYearsAbilityBalance-second-E2E`.
  [Machine-readable comparison](../build/wave6-test-status.json).
* Added and passed: `LightYearsReflectionRichProperties`,
  `VegaScriptRichProperties`, and `VegaEditorRichProperties`.
* The focused six-test reflection/Lua/widget/parser/multi-selection run passed
  6/6. The editor integration run passed 2/2 after fixing staging GUID publication.
  New fixture-only singleton JSON initializer issues were corrected before the
  final full run.
* [Blank editor script report](../build/properties-editor-test/properties.vegascript.report.txt)
  verifies playback reaches [520,270], the target Size becomes 0.5, Stop restores
  the authored location, and connections survive save/reopen, prefab placement,
  copy/paste and duplicate. [Editor capture](../build/properties-editor-test/properties.png)
  was inspected; the child subtitle aligns with its node label.
* New test assets and generated prefabs/scenes remain under build. No tests added
  by this change write to LightYearsGame/assets. No git commands were run.

## Limits

Plain structs and nested lists are deferred. ActorRef is untyped in C++; the
optional required reflected type filter is property metadata. Viewport picking
and point handles operate in scene Edit mode; prefab isolation has the dropdown.
Vec2 lists use world coordinates. Explicit direct writes to public scene GUIDs
must invalidate the world's actor reference revision. Lua spawn's optional props
table retains its existing scalar conversion; use actor:set for rich values.
