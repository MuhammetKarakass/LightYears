set(assets "${TEST_ROOT}/assets")
file(REMOVE_RECURSE "${assets}")
file(COPY "${TEMPLATE_ROOT}/Assets/" DESTINATION "${assets}")
file(MAKE_DIRECTORY "${assets}/prefabs")
file(COPY "${TEMPLATE_ROOT}/Tests/silence.wav" DESTINATION "${assets}")
file(WRITE "${assets}/prefabs/Base.prefab.json" [=[{"format":"vega.prefab","version":1,"base":{"type":"BlankPlayer","properties":{"Size":1}}}]=])
file(WRITE "${assets}/prefabs/Spawn.prefab.json" [=[{"format":"vega.prefab","version":1,"base":{"prefab":"prefabs/Base.prefab.json"}}]=])
file(WRITE "${assets}/scripts/spawn.lua" [=[
function on_start(self)
 local sound=audio.play('silence.wav',0,1.1)
 assert(sound:is_playing()); sound:stop(); assert(not sound:is_playing())
 music.play('silence.wav',0)
 music.fade('silence.wav',0.1,0)
 log('Lua audio ready')
 timer.after(2, function() world.spawn('prefabs/Spawn.prefab.json',400,200) end)
end
]=])
file(WRITE "${TEST_ROOT}/daily.vegascript" [=[
set-setting bounds [100,100,500,400]
expect-setting bounds [100,100,500,400]
undo
expect-setting bounds null
redo
select-name Player
move 900 20
add-script scripts/spawn.lua
save-scene scenes/main.scene.json
capture ../daily-use-test/bounds.png
play-mode
wait 5
select-name Player
expect-location 600 100
open-json prefabs/Base.prefab.json
json-set /base/properties/Size 2
json-save
wait-seconds 2.5
expect-console Info "Lua audio ready"
expect-console Info "Content change dispatched: prefabs/Base.prefab.json"
select-name prefabs/Spawn.prefab.json
expect-prop Actor Size 2
stop
wait 3
expect-setting bounds [100,100,500,400]
select-name Player
expect-location 900 20
expect-no-errors
quit
]=])
execute_process(COMMAND "${EDITOR}" --project "${assets}/vega.project.json" --asset-root "${assets}"
	--editor-script "${TEST_ROOT}/daily.vegascript" RESULT_VARIABLE result TIMEOUT 90)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Daily use script failed: ${result}; see ${TEST_ROOT}/daily.vegascript.report.txt")
endif()
