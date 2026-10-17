# Project collision channels

The engine retains eight `uint8_t` collision bits, now named `Layer0` through
`Layer7` in C++. Reflection initially names them `Layer 0` through `Layer 7`.
LightYears' symbolic names and combined masks moved to its own
`include/gameConfigs/CollisionLayers.h`; game call sites use those constants
without changing any bit values.

`ProjectSettings::collisionLayers` contains bit/name/optional-alias entries.
Parsing validates the complete list; malformed entries emit a warning and
discard the list, leaving neutral defaults. Application also validates typed
settings before changing descriptors. `ApplyCollisionLayers` registers engine
types in the supplied EngineContext and updates only that registry's collision
properties. Canonical labels and read-only aliases are separate, so aliases
never become Inspector choices or duplicate serialized flags.

The editor applies settings before module initialization and scene/prefab loads.
LightYears runtime applies them during module initialization before constructing
the first world. Blank runtime applies them before registering its module and
loading the startup scene. Exports already copy the authoring project file
unchanged and follow the same runtime initialization path.

The Inspector reads these descriptors directly. Scene/prefab decoding and Lua
reflected setters share `PropertyInfo::ParseFlags`, accepting canonical names,
aliases, neutral channel spellings and numeric string masks. Compatibility
spellings warn; numeric integer masks remain supported. Unknown names warn and
retain the property's prior value, with a diagnostic explaining alias/numeric
migration. Canonical saves retain bit order and existing LightYears spelling.

Blank's Player and Pickup channels retain bits 0 and 4. Its trigger sample now
uses Pickup; the project accepts the old Powerup spelling as a legacy alias.

## Files

- Engine: `include/framework/{Core,ProjectSettings}.h`,
  `include/framework/reflection/Reflection.h`,
  `src/framework/{CollisionComponent,ProjectSettings}.cpp`,
  `src/framework/reflection/Reflection.cpp`, `src/framework/scene/Scene.cpp`.
- Hosts/content: `VegaEditor/src/EditorApplication.cpp`,
  `LightYearsGame/src/gameFramework/LightYearsGameModule.cpp`,
  `Templates/Blank/Source/BlankMain.cpp`, both projects' `vega.project.json`,
  and Blank's `Assets/scenes/trigger.scene.json` and `Assets/scripts/trigger.lua`.
- Game vocabulary: the new `LightYearsGame/include/gameConfigs/CollisionLayers.h`
  and mechanical game include/source/test references to those constants.
- Scripting: `VegaScript/src/ScriptComponent.cpp`.
- Tests: new `LightYearsEngine/tests/CollisionLayersTests.cpp` and
  `tools/CheckCollisionLabels.cmake`, registered in engine CMake; extended
  `VegaEditor/tests/{MultiSelectionTests,GameExporterTests}.cpp`,
  `VegaScript/tests/{RichPropertiesTests,ScriptContextTests}.cpp`, and
  `LightYearsGame/tests/SceneSandboxTests.cpp`. Existing engine lifetime/spatial
  and script Lua API tests use neutral engine enum names with the same values.
- Documentation: `docs/project.md`, `docs/new-game.md`, this report.

## Coverage and golden

New tests are `VegaCollisionLayers` and `VegaNoGameCollisionLabels`. Coverage
includes custom/unspecified labels, aliases and numeric fallback warnings,
invalid configuration warnings/defaults, context isolation, prefab staging,
canonical scene bytes, Blank legacy bits, Inspector selection/edit descriptors,
Lua reflected setters with interleaved differently configured contexts, and
exported project labels. The source guard rejects each of the eight LightYears
collision string literals in production engine/editor/script include/src trees.

The LightYears sandbox test compares the saved file's actual bytes with the
authored file, including Windows line endings. It also checks a nonzero mask
containing all eight LightYears channels and its serialization round trip.
LightYears ships one scene and no prefab files. A generated LightYears prefab
containing all eight channels additionally checks byte-identical document
serialization after prefab realization. All generated files live under build.

`LightYearsEngine/tests/reflection-registry.golden.json` changes exactly 16
strings: eight labels on Collision Layer and eight on Collision Mask become
Layer 0 through Layer 7. No numeric value, property/type order, or other metadata
changes. The comparison against a dump from the pre-build executable is recorded
in `build/collision-golden-diff.json`. The saved pre-change dump has SHA-256
`4A58A386D9625DF82FFBAA957B60A9C4A83EB29B78939B62E7C4CD721984EFA7`,
matching the earlier validated golden. The interrupted run also reformatted
numbers; that formatting was restored. The final golden changes only those 16
label strings, preserving every other byte. Verification is recorded in
`build/collision-resume-golden-verification.json`; the final golden test passes.

## Final build and full suite

Resumed and validated in `D:\Vega`, Debug/MSVC/C++17, without git commands.
`cmd /c tools\vega-build.cmd` exited 0. The added LightYears prefab byte
regression passed in a focused sandbox run and in the full suite. After restoring
golden formatting, the focused reflection golden test also passed. The final
build includes the corrected Pickup comment in Blank's sample script.

Full command:

```text
ctest --test-dir build -j 8 --timeout 300 --output-on-failure --output-junit collision-tests.xml
```

| Run | Total | Passed | Known failures |
| --- | ---: | ---: | ---: |
| Saved pre-change review baseline | 167 | 143 | 24 |
| Final collision-layer suite | 169 | 145 | 24 |

CTest exited 8 in 175.09 seconds for exactly the documented 24 failures:
23 Failed and `LightYearsRareZoneRulesE2E` Timeout. That test retains its explicit
120-second timeout, which overrides the command's default. The baseline is
`build/review-tests.xml`; the known failure list remains the one in
[engine-context-validation.md](engine-context-validation.md).

Existing test status changes: **none**. Missing tests: **none**. Added tests:
`VegaCollisionLayers` Passed; `VegaNoGameCollisionLabels` Passed. All Inspector,
Lua, scene/prefab, export, reflection and Blank tests passed, including
`BlankSamplesLoad` and `BlankTemplateNoGameDeps`.

All 357 `LightYearsGame/assets` files retain their SHA-256 contents and last-write
timestamps across validation; no files were added or removed there. The runtime
and editor build copies of LightYears' project file match the authoring file
byte-for-byte, including all eight collision names. The exporter regression
additionally verifies byte-for-byte project copying and loading exported custom
labels into a separate context.

Evidence:

- `build/collision-resume-build.log`: final build, exit 0.
- `build/collision-tests.log`, `build/collision-tests.xml`: complete full run.
- `build/collision-test-summary.json`: all failures, new tests, empty status-change
  and missing-test lists compared with the 167-test baseline.
- `build/collision-golden-final.log`: final golden check, exit 0.
- `build/collision-resume-golden-verification.json`: exactly 16 label replacements,
  with every other golden byte preserved.
- `build/collision-resume-assets-verification.json`: 357 files, no changes.
- `build/e2e-artifacts/sandbox.scene.json` and
  `build/e2e-artifacts/CollisionLabels.prefab.json`: byte-round-trip fixtures.
