add_library(VegaSkyEditor STATIC
	src/SkyDocument.cpp src/EditorSkyEditor.cpp src/EditorSkyHandles.cpp
	src/EditorSkyScript.cpp src/SkyEditorExtension.cpp)
target_include_directories(VegaSkyEditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(VegaSkyEditor PUBLIC VegaEditorCore VegaSky)
ly_enable_warnings(VegaSkyEditor)
