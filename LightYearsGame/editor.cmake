# LightYears selects its module, optional editor tools and integration fixtures.
# Additional games use vega_add_game_editor with the plain VegaEditorCore.
add_library(LightYearsEditorModule STATIC "${CMAKE_CURRENT_LIST_DIR}/src/editor/LightYearsEditorModule.cpp")
target_include_directories(LightYearsEditorModule PRIVATE "${CMAKE_SOURCE_DIR}/VegaEditor/include")
target_link_libraries(LightYearsEditorModule PUBLIC LightYearsGameApp VegaSkyEditor VegaGASEditor)
vega_enable_warnings(LightYearsEditorModule)
set(VEGA_GAME_MODULE_TARGET LightYearsEditorModule)
set(VEGA_GAME_RUNTIME_TARGET LightYearsGame)
set(VEGA_GAME_EDITOR_ENTRY "${CMAKE_SOURCE_DIR}/VegaEditor/app/LightYearsEditorMain.cpp")
set(VEGA_DEFAULT_PROJECT_FILE "${CMAKE_CURRENT_LIST_DIR}/assets/vega.project.json")
set(VEGA_EDITOR_VISUAL_TARGET LightYearsVisual)
set(VEGA_CONTENT_LOADER_TARGET LightYearsContentTests)

# Resolve the paths the same way as EditorProject: relative to the project file.
file(READ "${VEGA_DEFAULT_PROJECT_FILE}" VEGA_PROJECT_JSON)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${VEGA_DEFAULT_PROJECT_FILE}")
get_filename_component(VEGA_PROJECT_DIRECTORY "${VEGA_DEFAULT_PROJECT_FILE}" DIRECTORY)
string(JSON VEGA_EDITOR_ASSET_ROOT GET "${VEGA_PROJECT_JSON}" editor assetRoot)
cmake_path(ABSOLUTE_PATH VEGA_EDITOR_ASSET_ROOT BASE_DIRECTORY "${VEGA_PROJECT_DIRECTORY}" NORMALIZE)
string(JSON VEGA_EDITOR_VISUAL_ASSETS GET "${VEGA_PROJECT_JSON}" editor visualAssets)
cmake_path(ABSOLUTE_PATH VEGA_EDITOR_VISUAL_ASSETS BASE_DIRECTORY "${VEGA_PROJECT_DIRECTORY}" NORMALIZE)
string(JSON VEGA_EXPORT_NAME GET "${VEGA_PROJECT_JSON}" name)
