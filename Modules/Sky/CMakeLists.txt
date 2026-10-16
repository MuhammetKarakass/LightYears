# The module has no engine, game, scripting or ability-system dependency.
set(CMAKE_CXX_FLAGS_DEBUG "${LY_DEBUG_FLAGS_RENDER}")
add_library(VegaSkyAstro STATIC src/Astro.cpp src/GalaxyModel.cpp)
target_include_directories(VegaSkyAstro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(VegaSky STATIC
	src/ColorRamp.cpp src/ColorScript.cpp src/PipelineSettings.cpp
	src/ShaderLibrary.cpp src/UniformWriter.cpp src/VisualAssets.cpp src/FileUtil.cpp
	src/ShapeBrush.cpp src/Backdrop.cpp src/Galaxy.cpp src/Nebula.cpp
	src/Anomalies.cpp src/NearField.cpp src/AsteroidField.cpp src/DeepSky.cpp
	src/OrbitSky.cpp src/VisualPipeline.cpp)
target_include_directories(VegaSky PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(VegaSky PUBLIC VegaSkyAstro SFML::Graphics nlohmann_json::nlohmann_json)
ly_enable_warnings(VegaSky)
ly_enable_warnings(VegaSkyAstro)

# Copy only reusable renderer assets. Each game owns its authored sky documents
# and can replace this neutral palette with its own art direction.
function(vega_stage_sky_assets target destination)
	add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_SOURCE_DIR}/Modules/Sky/assets" "${destination}")
endfunction()

if(BUILD_TESTING)
	add_executable(VegaSkyAssetsTests tests/SkyAssetsTests.cpp)
	target_link_libraries(VegaSkyAssetsTests PRIVATE VegaSky)
	target_compile_definitions(VegaSkyAssetsTests PRIVATE
		VEGA_SKY_ASSETS_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/assets"
		VEGA_SKY_ASSETS_TEST_ROOT="${CMAKE_CURRENT_BINARY_DIR}/asset-tests")
	add_test(NAME VegaSkyAssets COMMAND VegaSkyAssetsTests)
endif()
