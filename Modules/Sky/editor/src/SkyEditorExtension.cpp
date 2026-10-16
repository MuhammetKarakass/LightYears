#include <editor/ScriptExecution.h>
#include <editor/EditorSkyEditor.h>
#include <imgui.h>

#include <editor/SkyScriptCommands.h>

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
		vega::RegisterSkyScriptCommands(extensions.commands, [self](vega::ScriptExecution& execution, const vega::EditorScriptCommand& command)
		{
			execution.success = self->Execute(command, execution.context, execution.detail);
		});
	}
}
