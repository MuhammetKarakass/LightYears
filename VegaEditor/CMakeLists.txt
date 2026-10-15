include(FetchContent)

FetchContent_Declare(
	imgui
	URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.6-docking.zip
	URL_HASH SHA256=1f8b6143c7f34bf22a5735dd15fe7dfe8ad074ef3a8486b03882a20ea7426c21
)
FetchContent_Declare(
	imgui-sfml
	URL https://github.com/SFML/imgui-sfml/archive/e8ee3667f1b1f731e5c9c32be4c544915291f987.zip
	URL_HASH SHA256=b04c23185a1428972dd97f811dad8bce6a87e24721b243420ea6868d10dfdaea
)

# Dear ImGui supplies sources without a CMake project; ImGui-SFML compiles them.
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR "${imgui_SOURCE_DIR}")
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "Use the existing SFML targets" FORCE)
FetchContent_MakeAvailable(imgui-sfml)

add_executable(VegaEditor
	src/EditorMain.cpp
	src/EditorApplication.cpp
	src/EditorPanels.cpp
	src/EditorCommands.cpp
	src/EditorScript.cpp
	src/EditorScriptParser.cpp
	src/EditorViewport.cpp
	src/EditorSelection.cpp
	include/editor/EditorApplication.h
	include/editor/EditorCommands.h
	include/editor/EditorScript.h
	include/editor/EditorViewport.h
	include/editor/EditorSelection.h
)
target_include_directories(VegaEditor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# GameApp carries LightYearsVisual, the whole-archive game library, engine definitions,
# and the generated game asset configuration, exactly as for LightYearsGame.
target_link_libraries(VegaEditor PRIVATE LightYearsGameApp ImGui-SFML::ImGui-SFML)
ly_enable_warnings(VegaEditor)

if(MSVC)
	target_link_options(VegaEditor PRIVATE /INCREMENTAL:NO)
endif()
if(WIN32)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		set_target_properties(VegaEditor PROPERTIES WIN32_EXECUTABLE TRUE)
	endif()
	target_link_libraries(VegaEditor PRIVATE sfml-main)
endif()

foreach(editor_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
	CopyRuntimeLibToTarget(${editor_runtime} VegaEditor)
endforeach()
get_target_property(editor_imgui_type ImGui-SFML TYPE)
if(editor_imgui_type STREQUAL "SHARED_LIBRARY")
	CopyRuntimeLibToTarget(ImGui-SFML VegaEditor)
endif()

# Run even when only content changes, matching the game's runtime synchronization.
add_custom_target(VegaEditorAssets ALL
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/LightYearsGame/assets
		$<TARGET_FILE_DIR:VegaEditor>/assets
	COMMENT "Synchronizing editor runtime assets..."
)
add_dependencies(VegaEditor VegaEditorAssets)
add_custom_command(TARGET VegaEditorAssets POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,true,copy_directory>
		${CMAKE_SOURCE_DIR}/LightYearsVisual/assets
		$<TARGET_FILE_DIR:VegaEditor>/visual
)

if(BUILD_TESTING)
	add_executable(VegaEditorViewportTests
		tests/EditorViewportTests.cpp
		src/EditorViewport.cpp
	)
	target_include_directories(VegaEditorViewportTests PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/include
		${CMAKE_SOURCE_DIR}/LightYearsEngine/include
	)
	# Only the mapping function runs; no application or OpenGL context is constructed.
	target_link_libraries(VegaEditorViewportTests PRIVATE SFML::Graphics)
	ly_enable_warnings(VegaEditorViewportTests)
	foreach(editor_test_runtime IN ITEMS sfml-graphics sfml-window sfml-system)
		CopyRuntimeLibToTarget(${editor_test_runtime} VegaEditorViewportTests)
	endforeach()
	add_test(NAME VegaEditorViewport COMMAND VegaEditorViewportTests)

	add_executable(VegaEditorSelectionTests
		tests/EditorSelectionTests.cpp
		src/EditorSelection.cpp
	)
	target_include_directories(VegaEditorSelectionTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorSelectionTests PRIVATE LightYearsEngine)
	ly_enable_warnings(VegaEditorSelectionTests)
	foreach(editor_selection_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyRuntimeLibToTarget(${editor_selection_runtime} VegaEditorSelectionTests)
	endforeach()
	add_test(NAME VegaEditorSelection COMMAND VegaEditorSelectionTests)

	add_executable(VegaEditorCommandTests
		tests/EditorCommandTests.cpp
		src/EditorCommands.cpp
		src/EditorSelection.cpp
	)
	target_include_directories(VegaEditorCommandTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorCommandTests PRIVATE LightYearsEngine)
	ly_enable_warnings(VegaEditorCommandTests)
	foreach(editor_command_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyRuntimeLibToTarget(${editor_command_runtime} VegaEditorCommandTests)
	endforeach()
	add_test(NAME VegaEditorCommand COMMAND VegaEditorCommandTests)

	add_executable(VegaEditorScriptTests
		tests/EditorScriptTests.cpp
		src/EditorScriptParser.cpp
	)
	target_include_directories(VegaEditorScriptTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorScriptTests PRIVATE SFML::System)
	ly_enable_warnings(VegaEditorScriptTests)
	CopyRuntimeLibToTarget(sfml-system VegaEditorScriptTests)
	add_test(NAME VegaEditorScript COMMAND VegaEditorScriptTests)

	# Run a copy in the build tree: the script writes <script>.report.txt next to itself, which must not land in
	# the source tree.
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/scripts/smoke.vegascript
		${CMAKE_CURRENT_BINARY_DIR}/scripts/smoke.vegascript COPYONLY)
	add_test(NAME VegaEditorSmoke COMMAND VegaEditor --editor-script
		${CMAKE_CURRENT_BINARY_DIR}/scripts/smoke.vegascript)
	set_tests_properties(VegaEditorSmoke PROPERTIES
		WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor>
		TIMEOUT 60
	)
endif()
