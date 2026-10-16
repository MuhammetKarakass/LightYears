file(MAKE_DIRECTORY "${TEST_ROOT}")
set(assets "${TEST_ROOT}/assets")
file(REMOVE_RECURSE "${assets}")
file(COPY "${TEMPLATE_ROOT}/Assets/" DESTINATION "${assets}")
file(MAKE_DIRECTORY "${assets}/prefabs" "${assets}/moved" "${assets}/data")
file(WRITE "${assets}/data/paths.json" "{\"texture\":\"textures/tile.png\",\"nested\":{\"script\":\"scripts/spin.lua\"}}\n")
file(WRITE "${assets}/data/unused.json" "{\"value\":1}\n")
file(WRITE "${TEST_ROOT}/moves.vegascript" [=[
select-name Player
create-prefab prefabs/Player.prefab.json
save-scene scenes/main.scene.json
asset-rename scripts/spin.lua rotate.lua
wait 3
expect-asset scripts/spin.lua false
expect-asset scripts/rotate.lua true
open-json data/paths.json
expect-json /nested/script "scripts/rotate.lua"
undo
wait 3
expect-asset scripts/spin.lua true
expect-json /nested/script "scripts/spin.lua"
redo
wait 3
expect-asset scripts/rotate.lua true
asset-move scripts moved/scripts
wait 3
expect-asset scripts false
expect-asset moved/scripts/rotate.lua true
expect-json /nested/script "moved/scripts/rotate.lua"
asset-rename textures art
wait 3
expect-asset art/tile.png true
expect-asset textures false
expect-json /texture "art/tile.png"
asset-move scenes moved/scenes
wait 3
expect-project-startup moved/scenes/main.scene.json
expect-asset scenes/main.scene.json false
expect-asset moved/scenes/main.scene.json true
open-prefab prefabs/Player.prefab.json
expect-prop Actor Size 1.5
close-prefab
asset-delete data/unused.json confirm
expect-asset data/unused.json false
undo
expect-asset data/unused.json true
redo
expect-asset data/unused.json false
play-mode
wait 20
expect-no-errors
stop
wait 3
expect-scene-dirty false
quit
]=])
execute_process(COMMAND "${EDITOR}" --project "${assets}/vega.project.json" --asset-root "${assets}"
	--editor-script "${TEST_ROOT}/moves.vegascript" RESULT_VARIABLE result TIMEOUT 90)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Asset move script failed: ${result}; see ${TEST_ROOT}/moves.vegascript.report.txt")
endif()
file(READ "${assets}/prefabs/Player.prefab.json" prefab)
string(FIND "${prefab}" "moved/scripts/rotate.lua" found)
if(found EQUAL -1)
	message(FATAL_ERROR "Prefab script reference did not follow folder moves")
endif()
