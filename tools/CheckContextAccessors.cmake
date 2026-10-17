# Compatibility definitions are allowed; callers must select their owning context.
set(shims
	"VegaEngine/src/framework/reflection/Reflection.cpp|TypeRegistry|Get"
	"VegaEngine/src/framework/scene/PrefabLibrary.cpp|PrefabLibrary|Get"
	"VegaEngine/src/framework/AssetManager.cpp|AssetManager|GetAssetManager"
	"VegaEngine/src/framework/AudioManager.cpp|AudioManager|GetAudioManager"
	"VegaEngine/src/framework/EngineContext.cpp|EngineAssets|Get"
	"VegaEngine/src/framework/scene/SceneSettings.cpp|SceneSettingsSchema|Get"
	"VegaEngine/src/framework/EngineContext.cpp|ContentSchemaRegistry|Get"
	"VegaScript/src/ScriptRuntime.cpp|ScriptRuntime|Get")
set(space "[ \t\r\n]*")
set(accessor "((TypeRegistry|PrefabLibrary|EngineAssets|SceneSettingsSchema|ContentSchemaRegistry|ScriptRuntime)${space}::${space}Get|AssetManager${space}::${space}GetAssetManager|AudioManager${space}::${space}GetAudioManager)${space}\\(${space}\\)")
foreach(module VegaEngine VegaEditor VegaScript Modules/GAS Modules/GAS/editor SpaceAbilitySystem)
	file(GLOB_RECURSE sources
		"${SOURCE_ROOT}/${module}/include/*.h" "${SOURCE_ROOT}/${module}/include/*.hpp"
		"${SOURCE_ROOT}/${module}/src/*.h" "${SOURCE_ROOT}/${module}/src/*.hpp"
		"${SOURCE_ROOT}/${module}/src/*.cpp" "${SOURCE_ROOT}/${module}/src/*.cc"
		"${SOURCE_ROOT}/${module}/src/*.cxx")
	foreach(source IN LISTS sources)
		file(READ "${source}" content)
		# Strip comments so documentation of the compatibility API is harmless.
		string(REGEX REPLACE "/\\*([^*]|\\*+[^*/])*\\*+/|//[^\n]*" "" content "${content}")
		file(RELATIVE_PATH relative "${SOURCE_ROOT}" "${source}")
		foreach(shim IN LISTS shims)
			string(REPLACE "|" ";" parts "${shim}")
			list(GET parts 0 file)
			list(GET parts 1 service)
			list(GET parts 2 method)
			if(relative STREQUAL file)
				# Remove only the definition signature, never its body or other calls in this file.
				string(REGEX REPLACE "${service}${space}&${space}([A-Za-z_][A-Za-z_0-9]*::)*${service}${space}::${space}${method}${space}\\(${space}\\)${space}\\{" "{" content "${content}")
			endif()
		endforeach()
		if(content MATCHES "${accessor}")
			message(FATAL_ERROR "Legacy context accessor ${CMAKE_MATCH_0} in ${relative}; use the owning EngineContext")
		endif()
	endforeach()
endforeach()
