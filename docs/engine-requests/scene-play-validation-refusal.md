# Engine request: transactional editor Play refusal

Status: required for complete ArenaTest stage 3 acceptance. Vega 378f9aa is
read-only in this game task; no engine change is made here.

## What and why

The game must reject invalid scene gameplay configuration before arena/player/
encounter initialization, report the exact field, and leave the editor in Edit
so the user can repair the asset. `LevelLayout::Parse` now validates all ArenaTest
configuration before a play world is returned. It throws an actionable diagnostic
on invalid input; an edit world still loads repairable gameplay data.

The current editor transition does not handle that factory refusal transactionally:

* `D:/Vega/VegaEditor/src/EditorScenes.cpp`, `SetPlayMode` (lines 165–186), sets
  `mContext.playMode` before `LoadSceneWorld` and has no exception handler.
* `LoadSceneWorld` (lines 95–108) sets `transitioning=true` before invoking
  `mModule->CreateLevel` inside `LoadWorld`.
* `D:/Vega/VegaEditor/src/EditorApplication.cpp`, sceneAction (lines 212–223),
  delegates Play directly to SetPlayMode without catching factory errors.
* `GameModule::CreateLevel` returns a world; no existing optional capability
  exposes a pre-Play validation/refusal result. Editor tool PlayChanged is a void
  notification, not a veto contract.

Consequently, game-side validation prevents a gameplay world from being created,
but cannot guarantee a usable Edit session following invalid Play from the UI.
Do not catch errors and return an empty/fallback world, weaken validation, or
intercept private editor state to hide this failure.

## Proposed engine behavior/API

No new public interface is necessary if the host treats a CreateLevel exception
as an ordinary transition refusal. Construct the candidate world first, before
changing mode/selection/camera/transition flags or queuing the world replacement.
Catch std::exception, publish its message in sceneError and Console, return false,
and retain the active edit world, selection, settings, camera, undo history and
dirty state. Only commit those flags and callbacks after candidate construction
succeeds. Apply equivalent exception safety to Open/Stop.

An alternative explicit optional `ISceneValidator::ValidateScene(json, error)`
capability is acceptable if the same host transaction guarantees remain; game
rules and schema validation must stay game-side.

## Acceptance

Use a game module whose CreateLevel deliberately refuses Play with a named field
diagnostic. UI Play and script play-mode must both refuse without process exit or
stuck transitioning state. Assert Edit remains active, the same world and selection
survive, undo still works, correction then Play succeeds, and Stop restores Edit.
In LightYears, open arena-test.scene.json, set Entries[0].count=0, attempt Play,
require the count diagnostic, repair to 2 and Play again. No player, enemy, music
or gameplay actor lifecycle may start on the refused attempt.

Nested `Waves[].entries[]` reflection is a separate optional engine improvement.
Stage 3 currently uses two flat reflected lists supported by Vega v1; this request
does not require expanding the reflection value model.
