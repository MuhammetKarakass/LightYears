# Original integration assertions run against the game host and disposable game content.
add_executable(LightYearsSkyCaptureChecks SkyCaptureChecks.cpp)
target_link_libraries(LightYearsSkyCaptureChecks PRIVATE VegaSkyEditor VegaEditorCore)
target_compile_definitions(LightYearsSkyCaptureChecks PRIVATE
    VEGA_SKY_TEST_ROOT="${CMAKE_CURRENT_BINARY_DIR}/sky-document-test"
    VEGA_SKY_SOURCE="${CMAKE_SOURCE_DIR}/LightYearsVisual/assets")
foreach(runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
    CopyRuntimeLibToTarget(${runtime} LightYearsSkyCaptureChecks)
endforeach()
vega_enable_warnings_for_directory("${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME VegaEditorHotReload COMMAND ${CMAKE_COMMAND}
    -DSOURCE_ROOT=${CMAKE_SOURCE_DIR} -DTEST_ROOT=${CMAKE_BINARY_DIR}/hot-reload-editor-test
    -DEDITOR=$<TARGET_FILE:LightYearsEditor> -P ${CMAKE_CURRENT_SOURCE_DIR}/RunHotReload.cmake)
set_tests_properties(VegaEditorHotReload PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:LightYearsEditor> TIMEOUT 120)
set(VEGA_MULTI_SCENE "\"${CMAKE_CURRENT_BINARY_DIR}/multi-select-temp.scene.json\"")
set(VEGA_MULTI_CAPTURE "\"${CMAKE_CURRENT_BINARY_DIR}/multi-select-inspector.png\"")
set(VEGA_PREFABS_SCENE "\"${CMAKE_CURRENT_BINARY_DIR}/prefabs-temp.scene.json\"")
set(VEGA_PREFABS_CAPTURE "\"${CMAKE_CURRENT_BINARY_DIR}/prefab-inspector.png\"")
set(VEGA_SETTINGS_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/settings-source.scene.json")
set(VEGA_SETTINGS_SAVED "${CMAKE_CURRENT_BINARY_DIR}/settings-temp.scene.json")
set(VEGA_SETTINGS_BEFORE "${CMAKE_CURRENT_BINARY_DIR}/settings-before.png")
set(VEGA_SETTINGS_AFTER "${CMAKE_CURRENT_BINARY_DIR}/settings-after.png")
set(VEGA_EXPORT_TEST_DIR "${CMAKE_BINARY_DIR}/export-test/LightYears")
foreach(script IN ITEMS sky-editor gas-content gas-content-invalid gas-debugger multi-select lua-api scene-settings console-profiler prefabs content)
    configure_file(scripts/${script}.vegascript ${CMAKE_CURRENT_BINARY_DIR}/scripts/${script}.vegascript @ONLY)
endforeach()
configure_file(scripts/export.vegascript.in ${CMAKE_CURRENT_BINARY_DIR}/scripts/export.vegascript @ONLY)
foreach(test IN ITEMS SkyEditor GasContent MultiSelect LuaApi SceneSettings ConsoleProfiler Prefabs Export Content)
    configure_file(Run${test}Test.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/Run${test}Test.cmake @ONLY)
    add_test(NAME VegaEditor${test} COMMAND ${CMAKE_COMMAND}
        -DVEGA_EDITOR=$<TARGET_FILE:LightYearsEditor>
        -DVEGA_SKY_CHECK=$<TARGET_FILE:LightYearsSkyCaptureChecks>
        -DVEGA_CONTENT_LOADER=$<TARGET_FILE:LightYearsContentTests>
        -P ${CMAKE_CURRENT_BINARY_DIR}/Run${test}Test.cmake)
    set_tests_properties(VegaEditor${test} PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:LightYearsEditor> TIMEOUT 90)
endforeach()
set_tests_properties(VegaEditorSkyEditor VegaEditorExport PROPERTIES TIMEOUT 180)
set_tests_properties(VegaEditorGasContent PROPERTIES TIMEOUT 120)
set_tests_properties(VegaEditorContent PROPERTIES TIMEOUT 60)
file(COPY "${CMAKE_SOURCE_DIR}/LightYearsGame/assets/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/assets")
add_test(NAME VegaEditorGasDebugger COMMAND LightYearsEditor --asset-root ${CMAKE_CURRENT_BINARY_DIR}/assets
    --editor-script ${CMAKE_CURRENT_BINARY_DIR}/scripts/gas-debugger.vegascript)
set_tests_properties(VegaEditorGasDebugger PROPERTIES WORKING_DIRECTORY $<TARGET_FILE_DIR:LightYearsEditor> TIMEOUT 90)
