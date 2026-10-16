add_library(BlankGameModule STATIC Source/BlankGameModule.cpp)
target_include_directories(BlankGameModule PUBLIC Source)
target_link_libraries(BlankGameModule PUBLIC VegaScript)
ly_enable_warnings(BlankGameModule)
add_executable(BlankGame Source/BlankMain.cpp)
target_link_libraries(BlankGame PRIVATE BlankGameModule)
ly_enable_warnings(BlankGame)
if(WIN32)
	target_link_libraries(BlankGame PRIVATE sfml-main)
endif()
foreach(runtime sfml-graphics sfml-window sfml-system sfml-audio box2d)
	CopyRuntimeLibToTarget(${runtime} BlankGame)
endforeach()
add_custom_target(BlankGameAssets ALL COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:BlankGame>/assets")
add_dependencies(BlankGame BlankGameAssets)
if(VEGA_BUILD_EDITOR)
	vega_add_game_editor(BlankEditor MODULE BlankGameModule RUNTIME BlankGame
		PROJECT "${CMAKE_CURRENT_SOURCE_DIR}/Assets/vega.project.json"
		ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/Source/BlankEditorMain.cpp")
endif()

if(BUILD_TESTING)
	include(Tests/Dependencies.cmake)
	vega_record_dependencies(BlankGame)
	if(VEGA_BUILD_EDITOR)
		vega_record_dependencies(BlankEditor)
	endif()
	get_property(graph GLOBAL PROPERTY BLANK_DEPENDENCY_GRAPH)
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/dependencies.txt" "${graph}")
	add_test(NAME BlankTemplateNoGameDeps COMMAND ${CMAKE_COMMAND}
		-DTEMPLATE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
		-DGRAPH=${CMAKE_CURRENT_BINARY_DIR}/dependencies.txt
		-P ${CMAKE_CURRENT_SOURCE_DIR}/Tests/NoGameDeps.cmake)
	add_test(NAME BlankGameRuns COMMAND ${CMAKE_COMMAND}
		-DRUNTIME=$<TARGET_FILE:BlankGame> -P ${CMAKE_CURRENT_SOURCE_DIR}/Tests/RunGame.cmake)
	set_tests_properties(BlankGameRuns PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:BlankGame> TIMEOUT 60)
	if(VEGA_BUILD_EDITOR)
		add_test(NAME VegaEditorHierarchy COMMAND ${CMAKE_COMMAND}
			-DTEMPLATE_ROOT=${CMAKE_CURRENT_SOURCE_DIR} -DTEST_ROOT=${CMAKE_BINARY_DIR}/hierarchy-editor-test
			-DSCRIPT=${CMAKE_SOURCE_DIR}/VegaEditor/tests/scripts/hierarchy.vegascript
			-DEDITOR=$<TARGET_FILE:BlankEditor> -P ${CMAKE_CURRENT_SOURCE_DIR}/Tests/RunHierarchy.cmake)
		set_tests_properties(VegaEditorHierarchy PROPERTIES WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/VegaEditor" TIMEOUT 180)
		add_test(NAME BlankEditorScript COMMAND ${CMAKE_COMMAND}
			-DTEMPLATE_ROOT=${CMAKE_CURRENT_SOURCE_DIR} -DTEST_ROOT=${CMAKE_BINARY_DIR}/blank-editor-test
			-DRUNTIME=$<TARGET_FILE:BlankGame> -DEDITOR=$<TARGET_FILE:BlankEditor>
			-DCONFIG=$<CONFIG>
			-P ${CMAKE_CURRENT_SOURCE_DIR}/Tests/RunEditor.cmake)
		set_tests_properties(BlankEditorScript PROPERTIES WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/VegaEditor" TIMEOUT 180)
	endif()
endif()
