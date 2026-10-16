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

set(VEGA_EDITOR_CORE_SOURCES
	src/EditorMain.cpp
	src/EditorApplication.cpp
	src/EditorScenes.cpp
	src/EditorWorldSettings.cpp
	src/EditorPrefabs.cpp
	src/EditorTheme.cpp
	src/EditorIcons.cpp
	src/EditorChrome.cpp
	src/EditorDiagnostics.cpp
	src/EditorGizmos.cpp
	src/GizmoMath.cpp
	src/EditorCamera.cpp
	src/EditorPanels.cpp
	src/EditorCommands.cpp
	src/EditorScript.cpp
	src/EditorScriptParser.cpp
	src/EditorViewport.cpp
	src/EditorSelection.cpp
	src/EditorContentPanels.cpp
	src/EditorContentAuthoring.cpp
	src/ContentWorkspace.cpp
	src/GameExporter.cpp
	src/JsonDocument.cpp
	src/JsonCommands.cpp
	include/editor/GameExporter.h
	include/editor/JsonDocument.h
	include/editor/JsonCommands.h
	include/editor/EditorApplication.h
	include/editor/EditorGasDebugger.h
	include/editor/EditorTheme.h
	include/editor/EditorIcons.h
	include/editor/GizmoMath.h
	include/editor/EditorCamera.h
	include/editor/EditorCommands.h
	include/editor/EditorScript.h
	include/editor/EditorViewport.h
	include/editor/EditorSelection.h
)
# Both assemblies compile the same editor sources. Optional tools are selected
# here so a project without them has no transitive game or GAS dependency.
add_library(VegaEditorCore STATIC ${VEGA_EDITOR_CORE_SOURCES})
set(VEGA_EDITOR_CORES VegaEditorCore)
if(VEGA_BUILD_LIGHTYEARS)
add_library(VegaEditorLightYearsCore STATIC ${VEGA_EDITOR_CORE_SOURCES}
	src/EditorGasDebugger.cpp src/SkyDocument.cpp src/EditorSkyEditor.cpp
	src/EditorSkyHandles.cpp src/EditorSkyScript.cpp)
target_compile_definitions(VegaEditorLightYearsCore PUBLIC VEGA_EDITOR_WITH_SKY VEGA_EDITOR_WITH_GAS)
target_link_libraries(VegaEditorLightYearsCore PUBLIC ${VEGA_GAME_MODULE_TARGET})
list(APPEND VEGA_EDITOR_CORES VegaEditorLightYearsCore)
endif()
foreach(core IN LISTS VEGA_EDITOR_CORES)
	target_include_directories(${core} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(${core} PUBLIC VegaScript ImGui-SFML::ImGui-SFML)
	target_compile_definitions(${core} PRIVATE
		VEGA_SOURCE_ROOT="${CMAKE_SOURCE_DIR}" VEGA_BUILD_ROOT="${CMAKE_BINARY_DIR}")
	ly_enable_warnings(${core})
endforeach()
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/VegaGameEditor.cmake)
if(VEGA_BUILD_LIGHTYEARS)
vega_add_game_editor(VegaEditor MODULE ${VEGA_GAME_MODULE_TARGET}
	RUNTIME ${VEGA_GAME_RUNTIME_TARGET} PROJECT "${VEGA_DEFAULT_PROJECT_FILE}"
	ENTRY "${VEGA_GAME_EDITOR_ENTRY}" CORE VegaEditorLightYearsCore)
endif()
if(BUILD_TESTING AND VEGA_BUILD_LIGHTYEARS)
	set(VEGA_PROJECT_TEST_ROOT "${CMAKE_CURRENT_BINARY_DIR}/project-path-tests")
	add_executable(VegaEditorProjectTests tests/EditorProjectTests.cpp src/EditorProject.cpp)
	target_include_directories(VegaEditorProjectTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorProjectTests PRIVATE ${LIGHT_YEARS_ENGINE_TARGET_NAME})
	target_compile_definitions(VegaEditorProjectTests PRIVATE
		VEGA_PROJECT_TEST_ROOT="${VEGA_PROJECT_TEST_ROOT}"
		VEGA_DEFAULT_PROJECT_FILE="${VEGA_PROJECT_TEST_ROOT}/default/vega.project.json"
		VEGA_SOURCE_ROOT="${VEGA_PROJECT_TEST_ROOT}" VEGA_BUILD_ROOT="${VEGA_PROJECT_TEST_ROOT}/build")
	ly_enable_warnings(VegaEditorProjectTests)
	add_test(NAME VegaEditorProjectDefaults COMMAND VegaEditorProjectTests)
	add_test(NAME VegaEditorProjectOverrides COMMAND VegaEditorProjectTests
		--project ${VEGA_PROJECT_TEST_ROOT}/selected/project/vega.project.json --asset-root ${VEGA_PROJECT_TEST_ROOT}/authoring)
	add_executable(VegaEditorGameModuleTests tests/GameModuleTests.cpp src/EditorScriptParser.cpp
		src/EditorCommands.cpp src/EditorCamera.cpp src/EditorSelection.cpp)
	target_include_directories(VegaEditorGameModuleTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorGameModuleTests PRIVATE VegaScript)
	ly_enable_warnings(VegaEditorGameModuleTests)
	foreach(module_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyRuntimeLibToTarget(${module_runtime} VegaEditorGameModuleTests)
	endforeach()
	add_test(NAME VegaEditorGameModule COMMAND VegaEditorGameModuleTests)
	add_test(NAME VegaEditorNoGameIncludes COMMAND ${CMAKE_COMMAND}
		-DSOURCE_ROOT=${CMAKE_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/NoGameIncludes.cmake)
	add_executable(VegaEditorSkyDocumentTests tests/SkyDocumentTests.cpp src/SkyDocument.cpp src/JsonDocument.cpp src/JsonCommands.cpp
		src/EditorCommands.cpp src/EditorCamera.cpp src/EditorSelection.cpp)
	target_include_directories(VegaEditorSkyDocumentTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorSkyDocumentTests PRIVATE VegaScript ${VEGA_EDITOR_VISUAL_TARGET})
	target_compile_definitions(VegaEditorSkyDocumentTests PRIVATE
		VEGA_SKY_TEST_ROOT="${CMAKE_CURRENT_BINARY_DIR}/sky-document-test"
		VEGA_SKY_SOURCE="${VEGA_EDITOR_VISUAL_ASSETS}")
	ly_enable_warnings(VegaEditorSkyDocumentTests)
	foreach(sky_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyRuntimeLibToTarget(${sky_runtime} VegaEditorSkyDocumentTests)
	endforeach()
	add_test(NAME VegaEditorSkyDocument COMMAND VegaEditorSkyDocumentTests)
	configure_file(tests/scripts/sky-editor.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/sky-editor.vegascript @ONLY)
	configure_file(tests/RunSkyEditorTest.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RunSkyEditorTest.cmake @ONLY)
	add_test(NAME VegaEditorSkyEditor COMMAND ${CMAKE_COMMAND}
		-DVEGA_EDITOR=$<TARGET_FILE:VegaEditor> -DVEGA_SKY_CHECK=$<TARGET_FILE:VegaEditorSkyDocumentTests>
		-P ${CMAKE_CURRENT_BINARY_DIR}/RunSkyEditorTest.cmake)
	set_tests_properties(VegaEditorSkyEditor PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 180)
	add_executable(VegaEditorContentWorkspaceTests tests/ContentWorkspaceTests.cpp
		src/ContentWorkspace.cpp src/JsonDocument.cpp src/EditorCommands.cpp src/EditorCamera.cpp src/EditorSelection.cpp)
	target_include_directories(VegaEditorContentWorkspaceTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorContentWorkspaceTests PRIVATE VegaScript)
	target_compile_definitions(VegaEditorContentWorkspaceTests PRIVATE VEGA_CONTENT_TEST_ROOT="${CMAKE_CURRENT_BINARY_DIR}/content-workspace-tests")
	ly_enable_warnings(VegaEditorContentWorkspaceTests)
	foreach(content_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyRuntimeLibToTarget(${content_runtime} VegaEditorContentWorkspaceTests)
	endforeach()
	add_test(NAME VegaEditorContentWorkspace COMMAND VegaEditorContentWorkspaceTests)
	configure_file(tests/scripts/gas-content.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/gas-content.vegascript COPYONLY)
	configure_file(tests/scripts/gas-content-invalid.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/gas-content-invalid.vegascript COPYONLY)
	configure_file(tests/RunGasContentTest.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RunGasContentTest.cmake @ONLY)
	add_test(NAME VegaEditorGasContent COMMAND ${CMAKE_COMMAND}
		-DVEGA_EDITOR=$<TARGET_FILE:VegaEditor> -DVEGA_CONTENT_LOADER=$<TARGET_FILE:${VEGA_CONTENT_LOADER_TARGET}>
		-P ${CMAKE_CURRENT_BINARY_DIR}/RunGasContentTest.cmake)
	set_tests_properties(VegaEditorGasContent PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 120)
	configure_file(tests/scripts/gas-debugger.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/gas-debugger.vegascript @ONLY)
	add_test(NAME VegaEditorGasDebugger COMMAND VegaEditor --asset-root ${CMAKE_CURRENT_BINARY_DIR}/assets --editor-script ${CMAKE_CURRENT_BINARY_DIR}/scripts/gas-debugger.vegascript)
	set_tests_properties(VegaEditorGasDebugger PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 90)
	set(VEGA_MULTI_SCENE "\"${CMAKE_CURRENT_BINARY_DIR}/multi-select-temp.scene.json\"")
	set(VEGA_MULTI_CAPTURE "\"${CMAKE_CURRENT_BINARY_DIR}/multi-select-inspector.png\"")
	configure_file(tests/scripts/multi-select.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/multi-select.vegascript @ONLY)
	configure_file(tests/RunMultiSelectTest.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RunMultiSelectTest.cmake @ONLY)
	add_test(NAME VegaEditorMultiSelect COMMAND ${CMAKE_COMMAND}
		-DVEGA_EDITOR=$<TARGET_FILE:VegaEditor> -P ${CMAKE_CURRENT_BINARY_DIR}/RunMultiSelectTest.cmake)
	set_tests_properties(VegaEditorMultiSelect PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 90)
	add_executable(VegaEditorMultiSelectionTests tests/MultiSelectionTests.cpp
		src/EditorCommands.cpp src/EditorCamera.cpp src/EditorSelection.cpp)
	target_include_directories(VegaEditorMultiSelectionTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorMultiSelectionTests PRIVATE VegaScript)
	ly_enable_warnings(VegaEditorMultiSelectionTests)
	foreach(multi_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyRuntimeLibToTarget(${multi_runtime} VegaEditorMultiSelectionTests)
	endforeach()
	add_test(NAME VegaEditorMultiSelectionCommands COMMAND VegaEditorMultiSelectionTests)
	configure_file(tests/scripts/lua-api.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/lua-api.vegascript COPYONLY)
	configure_file(tests/RunLuaApiTest.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RunLuaApiTest.cmake @ONLY)
	add_test(NAME VegaEditorLuaApi COMMAND ${CMAKE_COMMAND}
		-DVEGA_EDITOR=$<TARGET_FILE:VegaEditor> -P ${CMAKE_CURRENT_BINARY_DIR}/RunLuaApiTest.cmake)
	set_tests_properties(VegaEditorLuaApi PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 90)
	set(VEGA_SETTINGS_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/settings-source.scene.json")
	set(VEGA_SETTINGS_SAVED "${CMAKE_CURRENT_BINARY_DIR}/settings-temp.scene.json")
	set(VEGA_SETTINGS_BEFORE "${CMAKE_CURRENT_BINARY_DIR}/settings-before.png")
	set(VEGA_SETTINGS_AFTER "${CMAKE_CURRENT_BINARY_DIR}/settings-after.png")
	configure_file(tests/scripts/scene-settings.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/scene-settings.vegascript @ONLY)
	configure_file(tests/RunSceneSettingsTest.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RunSceneSettingsTest.cmake @ONLY)
	add_test(NAME VegaEditorSceneSettings COMMAND ${CMAKE_COMMAND}
		-DVEGA_EDITOR=$<TARGET_FILE:VegaEditor> -P ${CMAKE_CURRENT_BINARY_DIR}/RunSceneSettingsTest.cmake)
	set_tests_properties(VegaEditorSceneSettings PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 90)
	configure_file(tests/scripts/console-profiler.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/console-profiler.vegascript @ONLY)
	configure_file(tests/RunConsoleProfilerTest.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RunConsoleProfilerTest.cmake @ONLY)
	add_test(NAME VegaEditorConsoleProfiler COMMAND ${CMAKE_COMMAND}
		-DVEGA_EDITOR=$<TARGET_FILE:VegaEditor> -P ${CMAKE_CURRENT_BINARY_DIR}/RunConsoleProfilerTest.cmake)
	set_tests_properties(VegaEditorConsoleProfiler PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 90)
	set(VEGA_PREFABS_SCENE "\"${CMAKE_CURRENT_BINARY_DIR}/prefabs-temp.scene.json\"")
	set(VEGA_PREFABS_CAPTURE "\"${CMAKE_CURRENT_BINARY_DIR}/prefab-inspector.png\"")
	configure_file(tests/scripts/prefabs.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/prefabs.vegascript @ONLY)
	configure_file(tests/RunPrefabsTest.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RunPrefabsTest.cmake @ONLY)
	add_test(NAME VegaEditorPrefabs COMMAND ${CMAKE_COMMAND}
		-DVEGA_EDITOR=$<TARGET_FILE:VegaEditor> -P ${CMAKE_CURRENT_BINARY_DIR}/RunPrefabsTest.cmake)
	set_tests_properties(VegaEditorPrefabs PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 90)
	add_executable(VegaEditorPropertyWidgetsTests tests/PropertyWidgetsTests.cpp)
	target_include_directories(VegaEditorPropertyWidgetsTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorPropertyWidgetsTests PRIVATE ImGui-SFML::ImGui-SFML)
	ly_enable_warnings(VegaEditorPropertyWidgetsTests)
	foreach(widget_runtime IN ITEMS sfml-graphics sfml-window sfml-system)
		CopyRuntimeLibToTarget(${widget_runtime} VegaEditorPropertyWidgetsTests)
	endforeach()
	if(editor_imgui_type STREQUAL "SHARED_LIBRARY")
		CopyRuntimeLibToTarget(ImGui-SFML VegaEditorPropertyWidgetsTests)
	endif()
	add_test(NAME VegaEditorPropertyWidgets COMMAND VegaEditorPropertyWidgetsTests)
	configure_file(tests/scripts/repro-texture.vegascript
		${CMAKE_CURRENT_BINARY_DIR}/scripts/repro-texture.vegascript COPYONLY)
	add_test(NAME VegaEditorPropertyError COMMAND VegaEditor --editor-script
		${CMAKE_CURRENT_BINARY_DIR}/scripts/repro-texture.vegascript)
	set_tests_properties(VegaEditorPropertyError PROPERTIES
		WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 60)
	set(VEGA_COMPONENTS_SCENE "\"${CMAKE_CURRENT_BINARY_DIR}/components-temp.scene.json\"")
	configure_file(tests/scripts/components.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/components.vegascript @ONLY)
	add_test(NAME VegaEditorComponents COMMAND VegaEditor --editor-script ${CMAKE_CURRENT_BINARY_DIR}/scripts/components.vegascript)
	set_tests_properties(VegaEditorComponents PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 60)
	set(VEGA_AUTHORING_SCENE "\"${CMAKE_CURRENT_BINARY_DIR}/authoring-temp.scene.json\"")
	configure_file(tests/scripts/authoring.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/authoring.vegascript @ONLY)
	add_test(NAME VegaEditorAuthoring COMMAND VegaEditor --editor-script ${CMAKE_CURRENT_BINARY_DIR}/scripts/authoring.vegascript)
	set_tests_properties(VegaEditorAuthoring PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 60)
	add_executable(VegaEditorGizmoMathTests tests/GizmoMathTests.cpp src/GizmoMath.cpp)
	target_include_directories(VegaEditorGizmoMathTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorGizmoMathTests PRIVATE SFML::System)
	ly_enable_warnings(VegaEditorGizmoMathTests)
	CopyRuntimeLibToTarget(sfml-system VegaEditorGizmoMathTests)
	add_test(NAME VegaEditorGizmoMath COMMAND VegaEditorGizmoMathTests)
	configure_file(tests/scripts/ui.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/ui.vegascript COPYONLY)
	add_test(NAME VegaEditorUI COMMAND VegaEditor --editor-script ${CMAKE_CURRENT_BINARY_DIR}/scripts/ui.vegascript)
	set_tests_properties(VegaEditorUI PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 60)
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
		src/EditorCamera.cpp
		src/EditorSelection.cpp
	)
	target_include_directories(VegaEditorCommandTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorCommandTests PRIVATE VegaScript)
	target_compile_definitions(VegaEditorCommandTests PRIVATE VEGA_COMMAND_SCRIPT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/command-script-assets")
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
	target_link_libraries(VegaEditorScriptTests PRIVATE SFML::System nlohmann_json::nlohmann_json)
	ly_enable_warnings(VegaEditorScriptTests)
	CopyRuntimeLibToTarget(sfml-system VegaEditorScriptTests)
	add_test(NAME VegaEditorScript COMMAND VegaEditorScriptTests)

	add_executable(VegaEditorCameraTests tests/EditorCameraTests.cpp src/EditorCamera.cpp)
	target_include_directories(VegaEditorCameraTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorCameraTests PRIVATE SFML::Graphics)
	ly_enable_warnings(VegaEditorCameraTests)
	foreach(camera_test_runtime IN ITEMS sfml-graphics sfml-window sfml-system)
		CopyRuntimeLibToTarget(${camera_test_runtime} VegaEditorCameraTests)
	endforeach()
	add_test(NAME VegaEditorCameraMath COMMAND VegaEditorCameraTests)

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
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tests/scripts/camera.vegascript
		${CMAKE_CURRENT_BINARY_DIR}/scripts/camera.vegascript COPYONLY)
	add_test(NAME VegaEditorCamera COMMAND VegaEditor --editor-script
		${CMAKE_CURRENT_BINARY_DIR}/scripts/camera.vegascript)
	set_tests_properties(VegaEditorCamera PROPERTIES
		WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor>
		TIMEOUT 60
	)
	add_executable(VegaEditorJsonDocumentTests
		tests/JsonDocumentTests.cpp src/JsonDocument.cpp src/JsonCommands.cpp
		src/EditorCommands.cpp src/EditorCamera.cpp src/EditorSelection.cpp)
	target_include_directories(VegaEditorJsonDocumentTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_link_libraries(VegaEditorJsonDocumentTests PRIVATE VegaScript nlohmann_json::nlohmann_json)
	target_compile_definitions(VegaEditorJsonDocumentTests PRIVATE
		VEGA_TEST_DATA_DIR="${VEGA_EDITOR_ASSET_ROOT}/content/data"
		VEGA_TEST_TEMP_DIR="${CMAKE_CURRENT_BINARY_DIR}/json-test-assets")
	ly_enable_warnings(VegaEditorJsonDocumentTests)
	foreach(json_test_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyRuntimeLibToTarget(${json_test_runtime} VegaEditorJsonDocumentTests)
	endforeach()
	add_test(NAME VegaEditorJsonDocument COMMAND VegaEditorJsonDocumentTests)

	# Pure export logic: fake game and source trees under the build tree, no window and no game process.
	configure_file(tests/scripts/scripting.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/scripting.vegascript COPYONLY)
	add_test(NAME VegaEditorScripting COMMAND VegaEditor --editor-script
		${CMAKE_CURRENT_BINARY_DIR}/scripts/scripting.vegascript)
	set_tests_properties(VegaEditorScripting PROPERTIES
		WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 60)
	add_executable(VegaEditorGameExporterTests tests/GameExporterTests.cpp src/GameExporter.cpp)
	target_link_libraries(VegaEditorGameExporterTests PRIVATE ${LIGHT_YEARS_ENGINE_TARGET_NAME})
	target_include_directories(VegaEditorGameExporterTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
	target_compile_definitions(VegaEditorGameExporterTests PRIVATE
		VEGA_EXPORT_TEST_TEMP_DIR="${CMAKE_CURRENT_BINARY_DIR}/game-export-test")
	ly_enable_warnings(VegaEditorGameExporterTests)
	add_test(NAME VegaEditorGameExporter COMMAND VegaEditorGameExporterTests)

	# The script is generated so the export target is absolute. It runs the real game in the build tree.
	set(VEGA_EXPORT_TEST_DIR "${CMAKE_BINARY_DIR}/export-test/${VEGA_EXPORT_NAME}")
	configure_file(tests/scripts/export.vegascript.in
		${CMAKE_CURRENT_BINARY_DIR}/scripts/export.vegascript @ONLY)
	configure_file(tests/RunExportTest.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RunExportTest.cmake @ONLY)
	add_test(NAME VegaEditorExport COMMAND ${CMAKE_COMMAND}
		-DVEGA_EDITOR=$<TARGET_FILE:VegaEditor> -P ${CMAKE_CURRENT_BINARY_DIR}/RunExportTest.cmake)
	set_tests_properties(VegaEditorExport PROPERTIES
		WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor>
		TIMEOUT 180
	)
	configure_file(tests/scripts/content.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/content.vegascript COPYONLY)
	configure_file(tests/RunContentTest.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/RunContentTest.cmake @ONLY)
	add_test(NAME VegaEditorContent COMMAND ${CMAKE_COMMAND}
		-DVEGA_EDITOR=$<TARGET_FILE:VegaEditor> -P ${CMAKE_CURRENT_BINARY_DIR}/RunContentTest.cmake)
	set_tests_properties(VegaEditorContent PROPERTIES
		WORKING_DIRECTORY $<TARGET_FILE_DIR:VegaEditor> TIMEOUT 60)
endif()
