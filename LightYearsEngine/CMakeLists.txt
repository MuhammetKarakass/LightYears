add_library(${LIGHT_YEARS_ENGINE_TARGET_NAME} STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/Application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/Application.cpp

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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/World.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/SimulationTime.h

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/camera/CameraManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/camera/CameraManager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/Actor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/Actor.cpp

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

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/PhysicsSystem.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/PhysicsSystem.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/Delegate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/Delegate.cpp

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

    ${CMAKE_CURRENT_SOURCE_DIR}/include/framework/JsonDocumentLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/framework/JsonDocumentLoader.cpp
)

target_include_directories(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC
    nlohmann_json::nlohmann_json)

target_compile_definitions(${LIGHT_YEARS_ENGINE_TARGET_NAME} PUBLIC
    LY_ENABLE_LOGGING=1
    $<$<CONFIG:Debug>:LY_ENABLE_ASSERTS=1>
    $<$<NOT:$<CONFIG:Debug>>:LY_ENABLE_ASSERTS=0>
    $<$<CONFIG:Debug>:LY_ENABLE_PROFILING=1>
    $<$<NOT:$<CONFIG:Debug>>:LY_ENABLE_PROFILING=0>
)

include(FetchContent)
set(SFML_LIB_NAME SFML)

FetchContent_Declare(
    ${SFML_LIB_NAME}
    URL https://github.com/SFML/SFML/archive/refs/tags/3.1.0.zip
)

FetchContent_MakeAvailable(${SFML_LIB_NAME})

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
    add_executable(LightYearsEngineLifetimeTests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/EngineLifetimeTests.cpp
    )
    target_link_libraries(LightYearsEngineLifetimeTests PRIVATE
        ${LIGHT_YEARS_ENGINE_TARGET_NAME}
    )
    add_test(NAME LightYearsEngineLifetime COMMAND LightYearsEngineLifetimeTests)
endif()
