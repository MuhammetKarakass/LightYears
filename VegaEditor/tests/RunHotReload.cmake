set(assets "${TEST_ROOT}/assets")
file(REMOVE_RECURSE "${assets}")
file(MAKE_DIRECTORY "${assets}/content/data" "${assets}/scenes")
file(COPY "${SOURCE_ROOT}/LightYearsGame/assets/content/data/" DESTINATION "${assets}/content/data")
file(WRITE "${assets}/scenes/empty.scene.json" [=[{"format":"vega.scene","version":1,"settings":{},"entities":[]}]=])
file(WRITE "${TEST_ROOT}/reload.vegascript" [=[
play-mode
wait 5
content-open ability Ability.Movement.Dash.Basic
content-set /cooldown 10
content-save
wait-seconds 1
expect-console Info "Reloaded content:"
expect-console Info "abilities.json (future spawns/grants)"
content-open ship Ship.Player.Fighter.Basic
content-set /health 260
content-save
wait-seconds 1
expect-console Info "ships.json (future spawns/grants)"
expect-no-errors
stop
wait 3
quit
]=])
execute_process(COMMAND "${EDITOR}" --asset-root "${assets}" --editor-scene "${assets}/scenes/empty.scene.json"
	--editor-script "${TEST_ROOT}/reload.vegascript" RESULT_VARIABLE result TIMEOUT 90)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Hot reload script failed: ${result}; see ${TEST_ROOT}/reload.vegascript.report.txt")
endif()
