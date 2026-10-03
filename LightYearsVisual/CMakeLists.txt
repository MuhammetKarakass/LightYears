option(LIGHTYEARS_BUILD_VISUAL_GALLERY "Build the LightYearsVisualGallery viewer" ON)

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

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/VisualAssets.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/VisualAssets.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/ShapeBrush.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/ShapeBrush.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/visual/Backdrop.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/Backdrop.cpp

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
