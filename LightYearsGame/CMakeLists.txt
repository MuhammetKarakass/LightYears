add_executable(${LIGHT_YEARS_GAME_TARGET_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/include/gameFramework/GameApplication.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameFramework/GameApplication.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/spaceship/Spaceship.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/spaceship/Spaceship.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/player/PlayerSpaceship.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerSpaceship.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/weapon/Shooter.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/weapon/Shooter.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/weapon/BulletShooter.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/weapon/BulletShooter.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/weapon/ThreeWayShooter.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/weapon/ThreeWayShooter.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/weapon/FrontalWiper.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/weapon/FrontalWiper.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/weapon/Bullet.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/weapon/Bullet.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/gameplay/HealthComponent.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/HealthComponent.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/VFX/Explosion.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/VFX/Explosion.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/EnemySpaceShip.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/EnemySpaceShip.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/Vanguard.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/Vanguard.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/LevelOne.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/LevelOne.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/MainMenuLevel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/MainMenuLevel.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/level/GameLevel.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/level/GameLevel.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/VanguardStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/VanguardStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/TwinBlade.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/TwinBlade.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/TwinBladeStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/TwinBladeStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/Hexagon.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/Hexagon.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/HexagonStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/HexagonStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/UFO.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/UFO.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/UFOStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/UFOStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/enemy/ChaosStage.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/enemy/ChaosStage.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/player/Reward.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/Reward.cpp
	
	${CMAKE_CURRENT_SOURCE_DIR}/include/player/Player.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/Player.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/player/PlayerManager.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/player/PlayerManager.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/widget/GameHUD.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/widget/GameHUD.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/widget/MainMenuHUD.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/widget/MainMenuHUD.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/include/widget/PauseMenuHUD.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/widget/PauseMenuHUD.cpp
)

target_include_directories(${LIGHT_YEARS_GAME_TARGET_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

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

add_custom_command(
	TARGET ${LIGHT_YEARS_GAME_TARGET_NAME}
	POST_BUILD
	COMMAND
	${CMAKE_COMMAND} -E copy_directory
	${RESOURCE_SRC_DIR}
	$<TARGET_FILE_DIR:${LIGHT_YEARS_GAME_TARGET_NAME}>/${RESOURCE_FOLDER_NAME}
)

configure_file(
    "config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/config.h" ESCAPE_QUOTES
)

target_include_directories(${LIGHT_YEARS_GAME_TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)