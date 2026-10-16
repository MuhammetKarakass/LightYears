get_filename_component(directory "${RUNTIME}" DIRECTORY)
# Remove only this test's output so a stale capture cannot hide a broken host.
file(REMOVE "${directory}/blank-capture.png")
execute_process(COMMAND "${RUNTIME}" --visual-capture "${directory}/blank-capture.png" --capture-frames 12
	WORKING_DIRECTORY "${directory}" RESULT_VARIABLE result TIMEOUT 45)
if(NOT result EQUAL 0 OR NOT EXISTS "${directory}/blank-capture.png")
	message(FATAL_ERROR "Blank runtime failed: ${result}")
endif()
