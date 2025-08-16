add_executable(${LIGHT_YEARS_GAME_TARGET_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/include/gameFramework/GameApplication.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gameFramework/GameApplication.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/spaceShip/SpaceShip.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spaceShip/SpaceShip.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/player/PlayerSpaceShip.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerSpaceShip.cpp

)

target_include_directories(${LIGHT_YEARS_GAME_TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${LIGHT_YEARS_GAME_TARGET_NAME} PUBLIC ${LIGHT_YEARS_ENGINE_TARGET_NAME})


function(CopyLibDirToTarget LIB_NAME TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME}
    POST_BUILD
    COMMAND 
    ${CMAKE_COMMAND} -E copy_directory
    $<TARGET_FILE_DIR:${LIB_NAME}>
     $<TARGET_FILE_DIR:${TARGET_NAME}>
    )
endfunction()
CopyLibDirToTarget(${LIGHT_YEARS_ENGINE_TARGET_NAME} ${LIGHT_YEARS_GAME_TARGET_NAME})

set(RESOURCE_FOLDER_NAME "assets")
set(RESOURCE_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_FOLDER_NAME}")

# Configure the Config.h file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/Config.h" 
    @ONLY
)

add_custom_command(
    TARGET ${LIGHT_YEARS_GAME_TARGET_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${RESOURCE_SRC_DIR}
    $<TARGET_FILE_DIR:${LIGHT_YEARS_GAME_TARGET_NAME}>/${RESOURCE_FOLDER_NAME}
)

configure_file(
    "Config.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Config.h" ESCAPE_QUOTES
)