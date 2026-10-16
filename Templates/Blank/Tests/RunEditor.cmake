# Recreate the fixture on every run; never author in the template or game assets.
file(MAKE_DIRECTORY "${TEST_ROOT}")
file(COPY "${TEMPLATE_ROOT}/Assets/" DESTINATION "${TEST_ROOT}/assets")
# These are generated authoring documents, not template inputs. Reset them so
# repeated runs exercise creation rather than an existing prefab from a prior run.
file(REMOVE "${TEST_ROOT}/assets/prefabs/Player.prefab.json" "${TEST_ROOT}/assets/scenes/saved.scene.json")
get_filename_component(runtimeDirectory "${RUNTIME}" DIRECTORY)
file(READ "${TEST_ROOT}/assets/vega.project.json" project)
string(JSON project SET "${project}" editor gameBuildDir "\"${runtimeDirectory}\"")
string(JSON project SET "${project}" editor releaseGameBuildDir "\"${runtimeDirectory}/no-release\"")
if(CONFIG STREQUAL "Release")
	string(JSON project SET "${project}" editor releaseGameBuildDir "\"${runtimeDirectory}\"")
endif()
file(WRITE "${TEST_ROOT}/assets/vega.project.json" "${project}")
configure_file("${TEMPLATE_ROOT}/Tests/blank.vegascript.in" "${TEST_ROOT}/blank.vegascript" @ONLY)
execute_process(COMMAND "${EDITOR}" --project "${TEST_ROOT}/assets/vega.project.json"
	--asset-root "${TEST_ROOT}/assets" --editor-script "${TEST_ROOT}/blank.vegascript"
	RESULT_VARIABLE result TIMEOUT 120)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Blank editor script failed: ${result}")
endif()
file(REMOVE "${TEST_ROOT}/export/independent.png")
execute_process(COMMAND "${TEST_ROOT}/export/Blank.exe" --visual-capture "${TEST_ROOT}/export/independent.png"
	--capture-frames 12 WORKING_DIRECTORY "${TEST_ROOT}/export" RESULT_VARIABLE result TIMEOUT 45)
if(NOT result EQUAL 0 OR NOT EXISTS "${TEST_ROOT}/export/independent.png")
	message(FATAL_ERROR "Exported Blank failed: ${result}")
endif()
if(EXISTS "${TEST_ROOT}/export/visual")
	message(FATAL_ERROR "Blank export unexpectedly contains presentation assets")
endif()
