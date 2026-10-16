add_library(${LIGHT_YEARS_ENGINE_TARGET_NAME} STATIC
	include/framework/ProjectSettings.h
	src/framework/ProjectSettings.cpp
	include/framework/ComponentIdentity.h
	src/framework/ComponentIdentity.cpp
	include/framework/reflection/Reflection.h
	include/framework/reflection/ReflectionMacros.h
	src/framework/reflection/Reflection.cpp
	src/framework/reflection/EngineReflection.cpp
	include/framework/scene/Scene.h
	src/framework/scene/Scene.cpp
	src/framework/scene/SceneSettings.cpp
	include/framework/scene/SceneSettings.h
	include/framework/scene/PrefabLibrary.h
	src/framework/scene/PrefabLibrary.cpp
	include/framework/scene/SceneEntityComponent.h
	src/framework/scene/SceneEntityComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/Application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/FramePacer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/FramePacer.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/EntryPoint.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/EntryPoint.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/Core.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/Core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/debug/Assert.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/debug/Assert.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/debug/Log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/debug/Log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/debug/Profiler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/debug/Profiler.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/engineConfigs/EngineStructs.h

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/World.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/WorldRenderPipeline.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/RenderViewport.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/World.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/RenderViewport.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/SimulationTime.h

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/camera/CameraManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/camera/CameraManager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/Actor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/Actor.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/ActorComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/ActorComponent.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/LightComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/LightComponent.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/SpriteComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/SpriteComponent.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/CollisionComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/CollisionComponent.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/BackGroundActor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/BackGroundActor.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/BackgroundLayer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/BackgroundLayer.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/Object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/Object.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/AssetManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/AssetManager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/MathUtility.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/MathUtility.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/PhysicsScene.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/PhysicsScene.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/Delegate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/Delegate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/SubscriptionSet.h

    ${CMAKE_CURRENT_SOURCE_DIR}/include/VFX/Particle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VFX/Particle.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/TimerManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/TimerManager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/AudioManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/AudioManager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/SoundComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/SoundComponent.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/ShaderManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/ShaderManager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/GameStage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/GameStage.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/WaitStage.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/WaitStage.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/HUD.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/HUD.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/Widget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/Widget.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/TextWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/TextWidget.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/TimerWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/TimerWidget.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/ValueGauge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/ValueGauge.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/ImageWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/ImageWidget.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/Button.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/Button.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/UILayout.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/UILayout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/UIViewModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/UIStyle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/UIStyle.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/Panel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/Panel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/widget/StackPanel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widget/StackPanel.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/JsonDocumentLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/JsonDocumentLoader.cpp
)

target_include_directories(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC
    nlohmann_json::nlohmann_json)

option(LY_WITH_PROFILING "Compile runtime-toggleable profiling (disable for shipping builds)" ON)
target_compile_definitions(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC
    LY_ENABLE_LOGGING=1
    $<$<CONFIG:Debug>:LY_ENABLE_ASSERTS=1>
    $<$<NOT:$<CONFIG:Debug>>:LY_ENABLE_ASSERTS=0>
    LY_ENABLE_PROFILING=$<BOOL:${LY_WITH_PROFILING}>
)

include(FetchContent)
set(SFML_LIB_NAME SFML)

FetchContent_Declare(
    ${SFML_LIB_NAME}
    URL https://github.com/SFML/SFML/archive/refs/tags/3.1.0.zip
)

set(CMAKE_CXX_FLAGS_DEBUG "${LY_DEBUG_FLAGS_RENDER}")
FetchContent_MakeAvailable(${SFML_LIB_NAME})
set(CMAKE_CXX_FLAGS_DEBUG "${LY_DEBUG_FLAGS_ORIGINAL}")

# SFML defines SFML_DEBUG whenever NDEBUG is missing, and then calls glGetError after every single OpenGL call.
# That forces a driver round trip per call and made a Debug game run at ~10-20 FPS. Defining NDEBUG for the SFML
# libraries only keeps our own code fully Debug (CRT, iterator checks, asserts) and leaves the ABI unchanged.
foreach(ly_sfml_target IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio)
    if(TARGET ${ly_sfml_target})
        target_compile_definitions(${ly_sfml_target} PRIVATE $<$<CONFIG:Debug>:NDEBUG>)
    endif()
endforeach()

target_link_libraries(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC sfml-graphics)
target_link_libraries(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC sfml-audio)
target_link_libraries(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC sfml-system)
target_link_libraries(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC sfml-window)

set(BOX2D_LIB_NAME Box2D)
FetchContent_Declare(
    ${BOX2D_LIB_NAME}
    URL https://github.com/erincatto/box2d/archive/refs/tags/v3.1.1.zip
)

FetchContent_MakeAvailable(${BOX2D_LIB_NAME})
target_link_libraries(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC box2d)


function(CopyLibToTarget LIB_NAME TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME}
    POST_BUILD
    COMMAND 
        ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${LIB_NAME}>
        $<TARGET_FILE_DIR:${TARGET_NAME}>
    )
endfunction()
CopyLibtoTarget(sfml-graphics ${LIGHT_YEARS_ENGINE_TARGET_NAME})
CopyLibtoTarget(sfml-window ${LIGHT_YEARS_ENGINE_TARGET_NAME})
CopyLibtoTarget(sfml-system ${LIGHT_YEARS_ENGINE_TARGET_NAME})
CopyLibtoTarget(sfml-audio ${LIGHT_YEARS_ENGINE_TARGET_NAME})
CopyLibtoTarget(box2d ${LIGHT_YEARS_ENGINE_TARGET_NAME})

if(BUILD_TESTING)
	add_executable(LightYearsReflectionMacroTests tests/ReflectionMacroTests.cpp)
	target_link_libraries(LightYearsReflectionMacroTests PRIVATE ${LIGHT_YEARS_ENGINE_TARGET_NAME})
	foreach(runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyLibToTarget(${runtime} LightYearsReflectionMacroTests)
	endforeach()
	add_test(NAME LightYearsReflectionMacros COMMAND LightYearsReflectionMacroTests)
	add_executable(LightYearsDiagnosticsTests tests/DiagnosticsTests.cpp)
	target_link_libraries(LightYearsDiagnosticsTests PRIVATE ${LIGHT_YEARS_ENGINE_TARGET_NAME})
	add_test(NAME LightYearsEngineDiagnostics COMMAND LightYearsDiagnosticsTests)
	# Compile the diagnostics implementation under NDEBUG too, without rebuilding the game in Release.
	add_executable(LightYearsDiagnosticsNDebugTests tests/DiagnosticsTests.cpp src/framework/debug/Log.cpp src/framework/debug/Profiler.cpp)
	target_include_directories(LightYearsDiagnosticsNDebugTests PRIVATE include)
	target_link_libraries(LightYearsDiagnosticsNDebugTests PRIVATE nlohmann_json::nlohmann_json)
	target_compile_definitions(LightYearsDiagnosticsNDebugTests PRIVATE NDEBUG LY_ENABLE_PROFILING=1)
	file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/diagnostics-ndebug)
	add_test(NAME LightYearsEngineDiagnosticsNDebug COMMAND LightYearsDiagnosticsNDebugTests)
	set_tests_properties(LightYearsEngineDiagnosticsNDebug PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/diagnostics-ndebug)
	add_executable(LightYearsPrefabLibraryTests tests/PrefabLibraryTests.cpp)
	target_link_libraries(LightYearsPrefabLibraryTests PRIVATE ${LIGHT_YEARS_ENGINE_TARGET_NAME})
	target_compile_definitions(LightYearsPrefabLibraryTests PRIVATE PREFAB_TEST_ROOT="${CMAKE_CURRENT_BINARY_DIR}/prefab-test-assets")
	foreach(prefab_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyLibToTarget(${prefab_runtime} LightYearsPrefabLibraryTests)
	endforeach()
	add_test(NAME LightYearsPrefabLibrary COMMAND LightYearsPrefabLibraryTests)
	add_executable(LightYearsProjectSettingsTests tests/ProjectSettingsTests.cpp)
	target_link_libraries(LightYearsProjectSettingsTests PRIVATE ${LIGHT_YEARS_ENGINE_TARGET_NAME})
	target_compile_definitions(LightYearsProjectSettingsTests PRIVATE PROJECT_TEST_ROOT="${CMAKE_CURRENT_BINARY_DIR}/project-test")
	add_test(NAME LightYearsProjectSettings COMMAND LightYearsProjectSettingsTests)
	add_executable(LightYearsSceneSerializerTests tests/SceneSerializerTests.cpp)
	target_link_libraries(LightYearsSceneSerializerTests PRIVATE ${LIGHT_YEARS_ENGINE_TARGET_NAME})
	foreach(scene_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyLibToTarget(${scene_runtime} LightYearsSceneSerializerTests)
	endforeach()
	add_test(NAME LightYearsSceneSerializer COMMAND LightYearsSceneSerializerTests)
	add_executable(LightYearsReflectionTests tests/ReflectionTests.cpp)
	target_link_libraries(LightYearsReflectionTests PRIVATE ${LIGHT_YEARS_ENGINE_TARGET_NAME})
	if(MSVC)
		# Reflection template instantiations change COMDATs between builds; avoid stale incremental link state.
		target_link_options(LightYearsReflectionTests PRIVATE /INCREMENTAL:NO /DEBUG:FASTLINK)
	endif()
	foreach(reflection_runtime IN ITEMS sfml-graphics sfml-window sfml-system sfml-audio box2d)
		CopyLibToTarget(${reflection_runtime} LightYearsReflectionTests)
	endforeach()
	add_test(NAME LightYearsReflection COMMAND LightYearsReflectionTests)
    add_executable(LightYearsGameplayTagTests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/GameplayTagTests.cpp
    )
    target_link_libraries(LightYearsGameplayTagTests PRIVATE
        ${LIGHT_YEARS_ENGINE_TARGET_NAME}
    )
    add_test(NAME LightYearsGameplayTag COMMAND LightYearsGameplayTagTests)

    add_executable(LightYearsEngineLifetimeTests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/EngineLifetimeTests.cpp
    )
    target_link_libraries(LightYearsEngineLifetimeTests PRIVATE
        ${LIGHT_YEARS_ENGINE_TARGET_NAME}
    )
    add_test(NAME LightYearsEngineLifetime COMMAND LightYearsEngineLifetimeTests)

    add_executable(LightYearsWorldSpatialQueryTests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/WorldSpatialQueryTests.cpp
    )
    target_link_libraries(LightYearsWorldSpatialQueryTests PRIVATE
        ${LIGHT_YEARS_ENGINE_TARGET_NAME}
    )
    add_test(NAME LightYearsWorldSpatialQuery COMMAND LightYearsWorldSpatialQueryTests)

    add_executable(LightYearsRenderViewportTests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/RenderViewportTests.cpp
    )
    target_link_libraries(LightYearsRenderViewportTests PRIVATE
        ${LIGHT_YEARS_ENGINE_TARGET_NAME}
    )
    add_test(NAME LightYearsRenderViewport COMMAND LightYearsRenderViewportTests)

    add_executable(LightYearsUIFoundationTests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UIFoundationTests.cpp
    )
    target_link_libraries(LightYearsUIFoundationTests PRIVATE
        ${LIGHT_YEARS_ENGINE_TARGET_NAME}
    )
    add_test(NAME LightYearsUIFoundation COMMAND LightYearsUIFoundationTests)
endif()
