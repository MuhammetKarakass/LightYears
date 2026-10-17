if(NOT "$ENV{VEGA_MUTE_AUDIO}" STREQUAL "1")
	message(FATAL_ERROR "Editor script runner did not inherit mute")
endif()
file(MAKE_DIRECTORY "${TEST_ROOT}")
file(COPY "${TEMPLATE_ROOT}/Assets/" DESTINATION "${TEST_ROOT}/assets")
file(REMOVE "${TEST_ROOT}/settings.json")
set(ENV{VEGA_EDITOR_SETTINGS} "${TEST_ROOT}/settings.json")
set(scripts
	"expect-mute-audio false\nmute-audio true\nexpect-mute-audio true\nquit\n"
	"expect-mute-audio true\nmute-audio false\nexpect-mute-audio false\nquit\n"
	"expect-mute-audio false\nquit\n")
set(index 0)
foreach(script IN LISTS scripts)
	math(EXPR index "${index} + 1")
	set(path "${TEST_ROOT}/mute-${index}.vegascript")
	file(WRITE "${path}" "${script}")
	execute_process(COMMAND "${EDITOR}" --project "${TEST_ROOT}/assets/vega.project.json"
		--asset-root "${TEST_ROOT}/assets" --editor-script "${path}"
		RESULT_VARIABLE result TIMEOUT 40)
	if(NOT result EQUAL 0)
		message(FATAL_ERROR "Audio preference restart ${index} failed: ${result}")
	endif()
endforeach()
