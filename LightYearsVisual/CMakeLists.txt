option(LIGHTYEARS_BUILD_VISUAL_GALLERY "Build the LightYearsVisualGallery viewer" ON)

# Per-frame rendering code: optimized even in Debug builds (see VEGA_OPTIMIZE_RENDER_IN_DEBUG in the root CMakeLists).
set(CMAKE_CXX_FLAGS_DEBUG "${VEGA_DEBUG_FLAGS_RENDER}")

# Legacy game-art target and paths remain available to other worktrees.
add_library(LightYearsAstro INTERFACE)
target_include_directories(LightYearsAstro INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(LightYearsAstro INTERFACE VegaSkyAstro)
add_library(LightYearsVisual STATIC src/ShipVisual.cpp src/ShipRenderer.cpp src/VisualAssets.cpp src/FileUtil.cpp)
target_include_directories(LightYearsVisual PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(LightYearsVisual PUBLIC VegaSky)
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
		VEGA_VISUAL_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
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
