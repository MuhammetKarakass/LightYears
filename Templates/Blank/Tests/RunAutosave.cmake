file(MAKE_DIRECTORY "${TEST_ROOT}")
# Each run starts a fresh disposable project; crash markers must not leak between runs.
set(assets "${TEST_ROOT}/assets")
file(REMOVE_RECURSE "${assets}")
file(COPY "${TEMPLATE_ROOT}/Assets/" DESTINATION "${assets}")
file(MAKE_DIRECTORY "${assets}/prefabs" "${assets}/data")
file(READ "${assets}/vega.project.json" project)
string(JSON project SET "${project}" editor autosaveSeconds 1)
file(WRITE "${assets}/vega.project.json" "${project}\n")
file(WRITE "${assets}/data/test.json" "{\"value\":1}\n")
file(WRITE "${TEST_ROOT}/capture.vegascript" [=[
select-name Player
create-prefab prefabs/Test.prefab.json
save-scene scenes/main.scene.json
move 123 234
open-json data/test.json
json-set /value 7
open-prefab prefabs/Test.prefab.json
set-prop Actor Rotation 21
wait-seconds 1.4
expect-autosave scenes/main.scene.json
expect-autosave prefabs/Test.prefab.json
expect-autosave data/test.json
autosave-now
simulate-unclean-exit
quit
]=])
file(WRITE "${TEST_ROOT}/recover.vegascript" [=[
expect-recovery scenes/main.scene.json
expect-recovery prefabs/Test.prefab.json
expect-recovery data/test.json
capture ../autosave-editor-test/recovery.png
recover-autosave scenes/main.scene.json
wait 3
select-name Player
expect-location 123 234
expect-scene-dirty true
save-scene scenes/recovered.scene.json
recover-autosave prefabs/Test.prefab.json
expect-prop Actor Rotation 21
save-prefab
close-prefab
recover-autosave data/test.json
expect-json /value 7
expect-json-dirty true
json-save
expect-json-dirty false
expect-no-errors
quit
]=])
file(WRITE "${TEST_ROOT}/clean.vegascript" [=[
expect-recovery scenes/main.scene.json false
expect-recovery prefabs/Test.prefab.json false
expect-recovery data/test.json false
expect-autosave scenes/main.scene.json
expect-autosave prefabs/Test.prefab.json
expect-autosave data/test.json
quit
]=])
execute_process(COMMAND "${EDITOR}" --project "${assets}/vega.project.json" --asset-root "${assets}"
	--editor-script "${TEST_ROOT}/capture.vegascript" RESULT_VARIABLE result TIMEOUT 60)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Autosave capture failed: ${result}; see ${TEST_ROOT}/capture.vegascript.report.txt")
endif()
file(READ "${assets}/data/test.json" original)
if(NOT original STREQUAL "{\"value\":1}\n")
	message(FATAL_ERROR "Autosave replaced the original JSON")
endif()
file(READ "${assets}/prefabs/Test.prefab.json" prefab)
string(JSON rotation GET "${prefab}" properties Rotation)
if(NOT rotation EQUAL 0)
	message(FATAL_ERROR "Autosave replaced the original prefab")
endif()
file(READ "${assets}/scenes/main.scene.json" scene)
string(FIND "${scene}" "123" changed)
if(NOT changed EQUAL -1)
	message(FATAL_ERROR "Autosave replaced the original scene")
endif()
foreach(phase recover clean)
	execute_process(COMMAND "${EDITOR}" --project "${assets}/vega.project.json" --asset-root "${assets}"
		--editor-script "${TEST_ROOT}/${phase}.vegascript" RESULT_VARIABLE result TIMEOUT 60)
	if(NOT result EQUAL 0)
		message(FATAL_ERROR "Autosave ${phase} failed: ${result}; see ${TEST_ROOT}/${phase}.vegascript.report.txt")
	endif()
endforeach()
