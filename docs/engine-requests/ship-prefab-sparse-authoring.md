# Engine request: retain sparse catalog-backed type prefab authoring

Date: 2026-10-18. Read-only engine inspected: Vega `7ea0f2c`.

## Problem and game boundary

LightYears' Fighter prefab deliberately contains only `base.type=PlayerSpaceShip`
and `properties.Ship=Ship.Player.Fighter.Basic`. Ship numbers remain canonical in
`ships.json`. The game can safely place/spawn and save sparse scene instances
using the existing engine API, and rejects authored component snapshots.

The engine editor cannot currently Apply or Save this type-based prefab without
copying constructor values into the asset:

- `D:/Vega/VegaEditor/src/EditorPrefabs.cpp`, `ApplyResolved`: starts with the
  realized `updated` entity and only diffs against a parent for `base.prefab`.
- The same file, `EditorApplication::CapturePrefab`: starts with
  `SceneSerializer::SaveSubtree(actor, true)` and only diffs for `base.prefab`.
- `D:/Vega/VegaEngine/src/framework/scene/Scene.cpp`, `SaveEntity`: expanded
  serialization includes writable actor properties and constructor components.

Thus even applying `Render Enabled` or saving the scratch Inspector writes health,
movement, shield/energy and presentation snapshots into the Fighter prefab. A
game-side reflected factory cannot prevent those writes. Globally hiding shared
component reflection or replacing the editor's save machinery would be broader
and less safe than fixing the engine owner. No engine files were modified.

## Requested behavior / proposed API

Provide a reusable way to author a type prefab as a sparse delta against a freshly
constructed property-aware base. Apply, Apply All, scratch Save Prefab, and Create
Prefab from Class should share this rule. A construction selector must be passed
to the base factory before taking component defaults, and retained as authored
construction data even when equal to the default selector. For catalog-owned
properties, either expose a serialization/override policy or preserve only the
authored keys plus the explicit property being applied. Do not put LightYears
IDs, catalogs, or ship classes into Vega.

One possible interface is a per-type scene construction policy that identifies
construction-property keys and optionally rejects authored overrides; the editor
can then construct the correct base and store its sparse delta. Exact API design
belongs to a Vega task under Vega's rules, not this game change.

Acceptance: applying Render Enabled or saving the Fighter scratch Inspector
retains Ship and the intentional visibility override, with no numeric component
snapshots. Reopen, Revert and refresh preserve catalog values; changing a
construction selector reconstructs or reports its refusal clearly. Catalog reload
must invalidate constructor realization caches or reconstruct the base freshly.

## Game-side reproduction

`VegaEditorFighterPrefab` operates on a disposable asset copy. It places Fighter,
executes Revert/Apply, saves/reopens the scene and saves the scratch prefab. Its
postcheck records that the engine expanded the copied document and the game
validator refuses it. This characterizes a safe rejection, not usable sparse
Apply/Save authoring or completion of the engine request. Shipped source assets
remain minimal. See `docs/SHIP_PREFABS.md` for the supported pilot boundary.
