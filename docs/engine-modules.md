# Optional engine modules

Engine modules are opt-in CMake libraries with runtime APIs and optional editor
extensions. They are linked at build time; there is no dynamic plugin loader.
The engine and `VegaEditorCore` do not link either module. `docs/modules.md`
continues to describe LightYears equipment, which is a different concept.

| Module | Runtime | Editor | Build switch |
| --- | --- | --- | --- |
| Sky | `VegaSky`, pure-math `VegaSkyAstro` | `VegaSkyEditor` | `VEGA_BUILD_SKY` |
| GAS | `VegaGAS` (alias of `SpaceAbilitySystem`) | `VegaGASEditor` | `VEGA_BUILD_GAS` |

Both switches default ON in this repository because LightYears uses both.
`VEGA_BUILD_LIGHTYEARS=OFF` permits either switch to be OFF independently.
Blank does not link either module even when their targets are available.

## Layout and compatibility

`Modules/Sky/include/sky`, `src`, `assets` and `editor` contain the reusable Sky
renderer, shaders, neutral sample palette, asset example and authoring tool.
Its API is `vega::sky`; `SkyAsset` and `DeepSkyCatalog::Find(id)` use opaque sky
IDs. The old `LightYearsVisual/include/visual` headers forward existing callers
to this API. `LightYearsAstro` is a compatibility interface target.

`LightYearsVisual` now compiles only ship art and its asset loader. Its
`ly::visual::VisualAssets` extends the generic loader with `Ships()`. Ship JSON,
ship palettes and LightYears' authored `deepsky.json` stay at their original
paths. Existing shader paths remain byte-identical compatibility copies;
`EngineModulesNoGameDependencies` checks them against the module shaders.
This preserves authoring, hot reload and other branches' paths without changing
the shipped look. Update both copies deliberately when changing a shared shader.

`Modules/GAS` supplies the module target, generic content descriptors and editor
extension. Existing runtime source and include paths under `SpaceAbilitySystem`
remain intact to avoid disrupting gameplay branches. Its public namespace is
still `sas`. Engine directories and the engine's `ly` namespace are unchanged.

## Enable a module in a game

Link runtime services explicitly:

```cmake
target_link_libraries(MyGameModule PUBLIC VegaSky VegaGAS)
# The editor adapter can use either extension independently.
target_link_libraries(MyEditorModule PUBLIC VegaSkyEditor VegaGASEditor)
vega_add_game_editor(MyEditor MODULE MyEditorModule RUNTIME MyGame
    PROJECT "${CMAKE_CURRENT_SOURCE_DIR}/Assets/vega.project.json"
    ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/Source/MyEditorMain.cpp")
```

Implement `IEditorProvider::RegisterEditorExtensions` in the editor adapter. GAS uses
`sas::RegisterEditorExtension(extensions)` from `gas/GasEditorExtension.h`.
It registers its panel, View menu, layout settings, Play recording lifecycle and
script commands. The game installs actor/player/content resolvers through
`AbilitySystemDebugView`; the module never assumes a player ship or game catalog.

Editor hosts construct `vega::EditorExtensions{ context }`, where the explicit
`ly::EngineContext` outlives the extensions. The GAS extension uses
`extensions.Engine()` to register schemas in that host, independently of the
ambient compatibility binding.

`sas::RegisterContentSchemas(context)` also works in runtime/tool hosts. It registers
generic ability/effect forms only when that type has no descriptor. Games supply
their production-loader validation, templates, ID/tag/attribute vocabulary and
additional fields. `sas::MakeContentField` supplies common GAS field metadata.
LightYears extends it with ship/module/weapon references, game fields and its
existing validators. Its authored catalogs and runtime behavior are unchanged.

For Sky, implement `vega::sky::SkyEditorHost` from `sky/SkyEditorHost.h` with:

- the module's `VisualAssets`, document path and a stable list of sky IDs;
- preview selection, optional scene-setting commands, and scene selection;
- viewport geometry/palette and deterministic off-screen rendering.

Register a shared `vega::EditorSkyEditor` using its `Register(extensions, self)`
method. The extension owns its document/history and registers the panel, View
menu, viewport handles and `sky-*`, `expect-sky`, `capture-sky` commands.
`LightYearsGame/src/editor/LightYearsEditorModule.cpp` is the complete adapter
example: it maps route indices to the legacy sky IDs and uses the unchanged
game-side `ProceduralBackdrop` for preview composition. A route-free game can
instead select a sky asset path from its scene settings and render with
`VisualPipeline`, `DeepSkyLayer`, `GalaxyLayer`, `OrbitSky` and the drawing APIs.
No route type or ship code is required by the module or its editor extension.

`EditorModuleTool` supplies initialize/shutdown, pre-render update, Play changes,
View menu, active-input arbitration, optional history and unsaved-exit handling.
Register panels and handles through `EditorExtensions`, as other game extensions
do. Settings handlers must be removed during Shutdown, before ImGui is destroyed.
The host keeps extensions alive until after worlds are finished.

## Sky assets

An individual file such as `skies/aurora.sky.json` contains:

```json
{
  "id": "aurora",
  "sky": { "band": { "width": 0.25 }, "objects": [{ "kind": "fog" }] }
}
```

A collection contains `{"presets": {...}, "skies": {"aurora": {...}}}`.
Entries may be a preset name or an object with `"use": "preset-name"` and
overrides. `presets` is optional. The legacy `stops` collection key remains
accepted, with exactly the same seeded parsing and array inheritance rules.
IDs belong to assets; LightYears alone interprets its legacy IDs as route stops.
Unknown keys and original text remain preserved by the editor document.

Use `DeepSkyCatalog::Load(path, error)` directly, or
`VisualAssets::Load(root, error, "skies/aurora.sky.json")` for settings, palettes,
shaders and hot reload. The optional third argument defaults to `deepsky.json`.
The same selected file is restored after ending an editor preview.
`SkyDocument(root, relativeFile)` edits either envelope without normalizing its
saved bytes. Its default path remains `deepsky.json` for existing hosts.

`vega_stage_sky_assets(target, destination)` stages the reusable shaders and
neutral sample assets. A game supplies its own palettes, settings and sky art;
it does not need LightYears' ship palettes or authored collection. Blank keeps
Sky OFF at the target-link level; copy the example asset and add the runtime and
editor links/adapters above to enable it. There is no template-specific option.

## Independence checks

Configure with `VEGA_BUILD_LIGHTYEARS=OFF`, `VEGA_BUILD_SKY=OFF` and
`VEGA_BUILD_GAS=OFF` to build Blank and the editor without optional modules.
Enable either module independently to build its runtime/editor targets and
module tests. `VegaEditorNoGameIncludes` has no exceptions. The core contains
neither game visual headers nor Sky/GAS tool implementations. The script parser
uses the same command registry as the extensions. Each module registers its
schemas and handlers together, retaining the existing validated grammar. A verb
is unknown when its module is absent; `help` lists only installed commands.
