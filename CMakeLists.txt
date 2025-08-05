cmake_minimum_required(VERSION 4.1.0)

project(LightYears
    VERSION 1.0.0
    LANGUAGES CXX
)

SET(CMAKE_CXX_STANDARD          17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS       OFF)

#creates a variable
set(LIGHT_YEARS_GAME_TARGET_NAME LightYearsGame)

add_subdirectory(LightYears)