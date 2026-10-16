# Preserve existing include and binary paths while exposing an engine module name.
add_subdirectory(${CMAKE_SOURCE_DIR}/SpaceAbilitySystem ${CMAKE_BINARY_DIR}/SpaceAbilitySystem)
add_library(VegaGAS ALIAS SpaceAbilitySystem)
if(BUILD_TESTING)
	add_executable(VegaGASContentSchemasTests tests/GasContentSchemasTests.cpp)
	target_link_libraries(VegaGASContentSchemasTests PRIVATE VegaGAS)
	add_test(NAME SpaceAbilitySystemContentSchemas COMMAND VegaGASContentSchemasTests)
endif()
