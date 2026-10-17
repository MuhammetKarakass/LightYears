#include "editor/EditorApplication.h"
#include "editor/EditorIcons.h"
#include "editor/PropertyWidgets.h"
#include "editor/EditorPrefabs.h"
#include <framework/World.h>
#include <framework/Actor.h>
#include <framework/scene/SceneEntityComponent.h>
#include <script/ScriptComponent.h>
#include <script/ScriptRuntime.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui-SFML.h>
#include <algorithm>
#include <cctype>
#include <typeinfo>
#include <typeindex>
#include <string_view>

namespace vega
{
	void EditorApplication::BuildStatsPanel()
	{
		if (ImGui::Begin("Stats", &mShowStats))
		{
			ImGui::Text("%.0f FPS", ImGui::GetIO().Framerate);
			ImGui::SameLine();
			ImGui::TextDisabled(" / %.2f ms", 1000.f / std::max(1.f, ImGui::GetIO().Framerate));
			ImGui::Separator();
			const vega::World* world = GetCurrentWorld();
			std::string name = world ? typeid(*world).name() : "None";
			if (const auto scope = name.rfind("::"); scope != std::string::npos) name.erase(0, scope + 2);
			ImGui::TextWrapped("World: %s", name.c_str());
			if (world)
			{
				std::size_t live = 0;
				for (const auto& weak : world->GetActorsByType<vega::Actor>())
					if (const auto actor = weak.lock(); actor && !actor->GetIsPendingDestroy()) ++live;
				ImGui::Text("Live actors: %zu", live);
				ImGui::TextDisabled("%s camera", world->GetViewOverride() ? "Editor" : "Game");
			}
		}
		ImGui::End();
	}
}
