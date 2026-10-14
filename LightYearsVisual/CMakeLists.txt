option(LIGHTYEARS_BUILD_VISUAL_GALLERY "Build the LightYearsVisualGallery viewer" ON)

# Per-frame rendering code: optimized even in Debug builds (see LY_OPTIMIZE_RENDER_IN_DEBUG in the root CMakeLists).
set(CMAKE_CXX_FLAGS_DEBUG "${LY_DEBUG_FLAGS_RENDER}")

# Pure astrophysics math (blackbody colour, magnitudes, galaxy model). No SFML, so gameplay code and tests can use it.
add_library(LightYearsAstro STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/Astro.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Astro.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/GalaxyModel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/GalaxyModel.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/Relativity.h
)
target_include_directories(LightYearsAstro PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(LightYearsVisual STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/LuminanceTier.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/ReloadReport.h

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/ColorRamp.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ColorRamp.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/ColorScript.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ColorScript.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/PipelineSettings.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/PipelineSettings.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/ShaderLibrary.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ShaderLibrary.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/UniformWriter.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/UniformWriter.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/VisualAssets.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/VisualAssets.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/ShapeBrush.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ShapeBrush.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/Backdrop.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Backdrop.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/Galaxy.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Galaxy.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/Nebula.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Nebula.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/Anomalies.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Anomalies.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/NearField.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/NearField.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/AsteroidField.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/AsteroidField.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/DeepSky.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/DeepSky.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/OrbitSky.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/OrbitSky.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/VisualPipeline.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/VisualPipeline.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/ShipVisual.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ShipVisual.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/ShipRenderer.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ShipRenderer.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/src/FileUtil.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/FileUtil.cpp
)

target_include_directories(LightYearsVisual PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(LightYearsVisual PUBLIC
	LightYearsAstro
	sfml-graphics
	sfml-window
	sfml-system
	nlohmann_json::nlohmann_json
)

if(LIGHTYEARS_BUILD_VISUAL_GALLERY)
	add_executable(LightYearsVisualGallery
		${CMAKE_CURRENT_SOURCE_DIR}/gallery/main.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/gallery/GalleryScene.h
		${CMAKE_CURRENT_SOURCE_DIR}/gallery/GalleryScene.cpp
	)

	target_link_libraries(LightYearsVisualGallery PRIVATE LightYearsVisual)

	# The gallery reads the source assets directly so edited shaders, palettes and
	# pipeline.json hot-reload without a rebuild. --assets <dir> overrides this.
	target_compile_definitions(LightYearsVisualGallery PRIVATE
		LY_VISUAL_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
	)

	foreach(sfml_runtime_lib sfml-graphics sfml-window sfml-system)
		add_custom_command(TARGET LightYearsVisualGallery
			POST_BUILD
			COMMAND
			${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:${sfml_runtime_lib}>
			$<TARGET_FILE_DIR:LightYearsVisualGallery>
		)
	endforeach()
endif()

if(BUILD_TESTING)
	add_executable(LightYearsVisualAstroTests ${CMAKE_CURRENT_SOURCE_DIR}/tests/AstroGalaxyTests.cpp)
	target_link_libraries(LightYearsVisualAstroTests PRIVATE LightYearsAstro)
	add_test(NAME LightYearsVisualAstro COMMAND LightYearsVisualAstroTests)

	add_executable(LightYearsVisualRelativityTests ${CMAKE_CURRENT_SOURCE_DIR}/tests/RelativityTests.cpp)
	target_link_libraries(LightYearsVisualRelativityTests PRIVATE LightYearsAstro)
	add_test(NAME LightYearsVisualRelativity COMMAND LightYearsVisualRelativityTests)
endif()
