cmake_minimum_required(VERSION 3.31.6)

project(LightYears
    VERSION 1.0.0
    LANGUAGES CXX
)

include(CTest)

include(FetchContent)

set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

SET(CMAKE_CXX_STANDARD          17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS       OFF)

if(MSVC)
    # Ninja needs the exact /showIncludes prefix to track header dependencies.
    # On a Turkish Windows cl.exe prints "Not: eklenen dosya:" unless VSLANG=1033
    # is in its environment, and vcvars64.bat does not set it. A mismatch records
    # zero header dependencies and silently leaves stale objects after header
    # edits. Launch every compile through `cmake -E env VSLANG=1033` so the
    # English prefix below holds regardless of the caller's environment.
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "Note: including file:")
    set(CMAKE_C_CL_SHOWINCLUDES_PREFIX "Note: including file:")
    set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "Note: including file:")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_COMMAND};-E;env;VSLANG=1033")
    set(CMAKE_C_COMPILER_LAUNCHER "${CMAKE_COMMAND};-E;env;VSLANG=1033")
endif()

# Add optimization flags for Release builds
if(MSVC)
    # Enable optimizations for MSVC
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
else()
    # Enable optimizations for GCC/Clang
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
endif()

# Debug builds keep the game code fully unoptimized, but the rendering layers (SFML and LightYearsVisual) are pure
# per-frame number crunching and GL glue: at /Od /Ob0 /RTC1 they cost several milliseconds per frame and the game
# runs at 10-20 FPS. Compiling only those with /O2 keeps the Debug CRT, iterator checks and ABI of the rest unchanged
# (/RTC1 is incompatible with /O2, so it is dropped for them). Turn it off to step through SFML or the visual library.
option(LY_OPTIMIZE_RENDER_IN_DEBUG "Compile SFML and LightYearsVisual with /O2 in Debug builds (MSVC)" ON)
set(LY_DEBUG_FLAGS_ORIGINAL "${CMAKE_CXX_FLAGS_DEBUG}")
set(LY_DEBUG_FLAGS_RENDER "${CMAKE_CXX_FLAGS_DEBUG}")
if(MSVC AND LY_OPTIMIZE_RENDER_IN_DEBUG)
    string(REGEX REPLACE "/Od|/Ob[0-9]|/RTC[1csu]+" "" LY_DEBUG_FLAGS_RENDER "${CMAKE_CXX_FLAGS_DEBUG}")
    set(LY_DEBUG_FLAGS_RENDER "${LY_DEBUG_FLAGS_RENDER} /O2 /Ob2")
endif()

#creates a variable
set(LIGHT_YEARS_ENGINE_TARGET_NAME LightYearsEngine)
set(SPACE_ABILITY_SYSTEM_TARGET_NAME SpaceAbilitySystem)
set(LIGHT_YEARS_GAME_TARGET_NAME LightYearsGame)

# Raises the warning level on first-party targets only. Never call this on
# third-party targets (SFML, box2d, nlohmann_json); /WX is deliberately not set.
# CMP0092 (NEW since 3.15) keeps CMake from injecting /W3, so /W4 does not
# trigger D9025 override warnings.
function(ly_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

function(ly_enable_warnings_for_directory dir)
    get_property(ly_dir_targets DIRECTORY "${dir}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(ly_target IN LISTS ly_dir_targets)
        get_target_property(ly_target_type ${ly_target} TYPE)
        if(ly_target_type MATCHES "^(STATIC_LIBRARY|OBJECT_LIBRARY|EXECUTABLE)$")
            ly_enable_warnings(${ly_target})
        endif()
    endforeach()
endfunction()

option(VEGA_BUILD_LIGHTYEARS "Build the LightYears game and its editor integration" ON)
option(VEGA_BUILD_GAS "Build the optional ability system" ON)
if(VEGA_BUILD_LIGHTYEARS AND NOT VEGA_BUILD_GAS)
	message(FATAL_ERROR "LightYears requires VEGA_BUILD_GAS")
endif()
add_subdirectory(LightYearsEngine)
if(VEGA_BUILD_LIGHTYEARS)
	add_subdirectory(LightYearsVisual)
endif()
if(VEGA_BUILD_GAS)
	add_subdirectory(SpaceAbilitySystem)
endif()
add_subdirectory(VegaScript)
if(VEGA_BUILD_LIGHTYEARS)
	add_subdirectory(LightYearsGame)
endif()

option(VEGA_BUILD_EDITOR "Build the Vega editor" ON)
if(VEGA_BUILD_EDITOR)
	if(VEGA_BUILD_LIGHTYEARS)
		include(LightYearsGame/editor.cmake)
	endif()
	add_subdirectory(VegaEditor)
endif()

option(VEGA_BUILD_TEMPLATES "Build reusable game templates" ON)
if(VEGA_BUILD_TEMPLATES)
	add_subdirectory(Templates/Blank)
endif()

# First-party targets (LightYearsEngine, LightYearsVisual, SpaceAbilitySystem,
# LightYearsGameplayCore, LightYearsGame and their test executables). Each of
# these directories lists only its own targets, not the fetched dependencies.
set(ly_warning_dirs LightYearsEngine)
if(VEGA_BUILD_GAS)
	list(APPEND ly_warning_dirs SpaceAbilitySystem)
endif()
if(VEGA_BUILD_LIGHTYEARS)
	list(APPEND ly_warning_dirs LightYearsVisual LightYearsGame)
endif()
foreach(ly_warning_dir IN LISTS ly_warning_dirs)
    ly_enable_warnings_for_directory("${CMAKE_CURRENT_SOURCE_DIR}/${ly_warning_dir}")
endforeach()

if(BUILD_TESTING AND VEGA_BUILD_LIGHTYEARS)
	add_test(
		NAME LightYearsGasLiteCore
		COMMAND LightYearsGasLiteTests
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)
endif()
