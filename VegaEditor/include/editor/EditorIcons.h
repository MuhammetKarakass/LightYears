#pragma once
#include <imgui.h>

namespace vega
{
	enum class IconKind { Play, Pause, Step, Camera, Move, Rotate, Folder, File, Json, Image, Audio, Script, Actor, Snap };
	void DrawEditorIcon(ImDrawList& draw, IconKind kind, ImVec2 min, ImVec2 size, ImU32 colour);
	bool IconButton(const char* id, IconKind kind, ImVec2 size, const char* tooltip, bool active = false);
}
