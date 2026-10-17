# The pre-split CMake referenced this ignored fixture, absent from the supplied clone/staging.
# Reject invalid content before the game world starts, without changing shipped assets.
set(root "${TEST_ROOT}/assets")
file(MAKE_DIRECTORY "${root}/content/data")
file(COPY "${SOURCE_ROOT}/LightYearsGame/assets/content/data/" DESTINATION "${root}/content/data")
file(WRITE "${root}/content/data/abilities.json" "{ invalid json\n")
execute_process(COMMAND "${RUNTIME}" --asset-root "${root}" --visual-capture "${TEST_ROOT}/unexpected.png" --capture-frames 2
    WORKING_DIRECTORY "${TEST_ROOT}" RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 45)
file(WRITE "${TEST_ROOT}/output.txt" "${output}\n${errors}")
if(NOT result STREQUAL "1")
    message(FATAL_ERROR "Invalid shipped content must exit 1, got ${result}: ${output}\n${errors}")
endif()
file(READ "${TEST_ROOT}/LightYears.log" log)
if(NOT log MATCHES "Failed to register shipped game content; application will not start the game world")
    message(FATAL_ERROR "Bootstrap rejection diagnostic missing")
endif()
if(log MATCHES "SceneLevel: scene-loaded")
    message(FATAL_ERROR "Game world started after content bootstrap failure")
endif()
