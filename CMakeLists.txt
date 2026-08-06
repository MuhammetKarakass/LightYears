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
    # Turkish MSVC localizes /showIncludes output as "Not: eklenen dosya:".
    # Ninja needs the exact prefix to track header dependencies such as GameplayConfig.h.
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "Not: eklenen dosya:")
    set(CMAKE_C_CL_SHOWINCLUDES_PREFIX "Not: eklenen dosya:")
    set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "Not: eklenen dosya:")
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

#creates a variable
set(LIGHT_YEARS_ENGINE_TARGET_NAME LightYearsEngine)
set(SPACE_ABILITY_SYSTEM_TARGET_NAME SpaceAbilitySystem)
set(LIGHT_YEARS_GAME_TARGET_NAME LightYearsGame)

add_subdirectory(LightYearsEngine)
add_subdirectory(SpaceAbilitySystem)
add_subdirectory(LightYearsGame)

if(BUILD_TESTING)
	add_test(
		NAME LightYearsGasLiteCore
		COMMAND LightYearsGasLiteTests
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)
endif()
