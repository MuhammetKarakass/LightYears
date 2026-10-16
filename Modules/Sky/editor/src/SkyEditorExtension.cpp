#include <editor/EditorSkyEditor.h>
#include <imgui.h>

namespace vega
{
	void EditorSkyEditor::ViewMenu()
	{
		if (ImGui::MenuItem("Sky", nullptr, &visible) && visible) ImGui::SetWindowFocus("Sky###SkyEditor");
	}

	void EditorSkyEditor::Register(EditorExtensions& extensions, const std::shared_ptr<EditorSkyEditor>& self)
	{
		extensions.AddTool(self);
		extensions.inspectorWindows.push_back("Sky###SkyEditor");
		extensions.AddPanel([self](EditorContext& context, CommandHistory& history) { self->Build(context, history); });
		extensions.AddViewport([self](EditorViewportExtensionContext& context) { self->Handles(context.image, context.textureSize, context.hovered); });
		for (const auto* name : {"sky-open", "sky-set", "sky-drag", "sky-save", "expect-sky", "capture-sky"})
			extensions.AddScriptCommand(name, EditorExtensions::ParsedScript{[self](EditorContext& context, const EditorScriptCommand& command, std::string& detail)
			{
				return self->Execute(command, context, detail);
			}});
	}
}
