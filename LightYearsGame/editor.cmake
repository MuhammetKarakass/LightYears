# Game host, content and integrations consume only public engine targets.
add_library(LightYearsEditorModule STATIC "${CMAKE_CURRENT_LIST_DIR}/src/editor/LightYearsEditorModule.cpp")
target_link_libraries(LightYearsEditorModule PUBLIC LightYearsGameApp VegaEditorCore VegaSkyEditor VegaGASEditor)
vega_add_game_editor(LightYearsEditor MODULE LightYearsEditorModule
    RUNTIME LightYearsGame PROJECT "${CMAKE_CURRENT_LIST_DIR}/assets/vega.project.json"
    ENTRY "${CMAKE_CURRENT_LIST_DIR}/app/LightYearsEditorMain.cpp"
    SOURCE_ROOT "${CMAKE_SOURCE_DIR}" BUILD_ROOT "${CMAKE_BINARY_DIR}")
# Packaged builds ship the engine's Sky shaders next to the editor; LightYearsVisual keeps only overrides.
vega_stage_sky_shaders(LightYearsEditor)
if(BUILD_TESTING)
    add_subdirectory(editor-tests)
endif()
