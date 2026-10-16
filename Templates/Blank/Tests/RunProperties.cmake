file(MAKE_DIRECTORY "${TEST_ROOT}")
file(COPY "${TEMPLATE_ROOT}/Assets/" DESTINATION "${TEST_ROOT}/assets")
file(REMOVE "${TEST_ROOT}/assets/prefabs/Connected.prefab.json" "${TEST_ROOT}/assets/scenes/properties.scene.json")
configure_file("${TEMPLATE_ROOT}/Tests/properties.vegascript" "${TEST_ROOT}/properties.vegascript" COPYONLY)
execute_process(COMMAND "${EDITOR}" --project "${TEST_ROOT}/assets/vega.project.json"
	--asset-root "${TEST_ROOT}/assets" --editor-script "${TEST_ROOT}/properties.vegascript"
	RESULT_VARIABLE result TIMEOUT 120)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "Rich properties script failed: ${result}; see ${TEST_ROOT}/properties.vegascript.report.txt")
endif()
