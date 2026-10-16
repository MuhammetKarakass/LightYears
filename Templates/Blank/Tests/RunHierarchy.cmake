file(MAKE_DIRECTORY "${TEST_ROOT}")
file(COPY "${TEMPLATE_ROOT}/Assets/" DESTINATION "${TEST_ROOT}/assets")
file(REMOVE "${TEST_ROOT}/assets/prefabs/Hierarchy.prefab.json" "${TEST_ROOT}/assets/scenes/hierarchy.scene.json")
configure_file("${SCRIPT}" "${TEST_ROOT}/hierarchy.vegascript" COPYONLY)
execute_process(COMMAND "${EDITOR}" --project "${TEST_ROOT}/assets/vega.project.json"
	--asset-root "${TEST_ROOT}/assets" --editor-script "${TEST_ROOT}/hierarchy.vegascript"
	RESULT_VARIABLE result TIMEOUT 120)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Hierarchy editor script failed: ${result}; see ${TEST_ROOT}/hierarchy.vegascript.report.txt")
endif()
