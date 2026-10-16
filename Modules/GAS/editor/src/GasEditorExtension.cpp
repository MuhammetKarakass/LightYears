#include <gas/GasEditorExtension.h>
#include <gas/GasContentSchemas.h>
#include <editor/EditorGasDebugger.h>
#include <editor/EditorExtensions.h>
#include <editor/EditorScript.h>
#include <imgui_internal.h>

namespace sas
{
	namespace
	{
		class GasEditorTool final : public vega::EditorModuleTool
		{
		public:
			vega::EditorGasDebugger debugger;
			void Initialize(vega::EditorContext&) override { debugger.InstallSettings(); }
			void Shutdown() override
			{
				AbilitySystemDebugView::recording = false;
				ImGui::RemoveSettingsHandler("VegaGasDebugger");
			}
			void PlayChanged(bool playing) override
			{
				debugger.Clear();
				if (playing) AbilitySystemDebugView::BeginPlay();
				else AbilitySystemDebugView::recording = false;
			}
			void ViewMenu() override
			{
				if (ImGui::MenuItem("Ability System", nullptr, &debugger.visible)) ImGui::MarkIniSettingsDirty();
			}
		};
	}
	void RegisterEditorExtension(vega::EditorExtensions& extensions)
	{
		RegisterContentSchemas();
		auto tool = std::make_shared<GasEditorTool>();
		extensions.AddTool(tool);
		extensions.inspectorWindows.push_back("Ability System");
		extensions.AddPanel([tool](vega::EditorContext& context, vega::CommandHistory&) { if (tool->debugger.visible) tool->debugger.Build(context); });
		for (const auto* name : {"gas-filter", "gas-select-player", "gas-reset-cooldowns", "gas-apply-effect", "gas-remove-effect", "gas-activate", "gas-panel", "expect-ability", "expect-effect", "expect-attribute", "expect-tag"})
			extensions.AddScriptCommand(name, vega::EditorExtensions::ParsedScript{[tool](vega::EditorContext& context, const vega::EditorScriptCommand& command, std::string& detail)
			{
				return tool->debugger.Execute(context, command, detail);
			}});
	}
}
