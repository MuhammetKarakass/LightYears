#include "editor/EditorIcons.h"
#include <algorithm>

namespace vega
{
	void DrawEditorIcon(ImDrawList& d, IconKind kind, ImVec2 min, ImVec2 size, ImU32 c)
	{
		const float s = std::min(size.x, size.y);
		const ImVec2 o{ min.x + (size.x - s) * 0.5f, min.y + (size.y - s) * 0.5f };
		const auto p = [&](float x, float y) { return ImVec2{ o.x + x * s, o.y + y * s }; };
		const auto line = [&](float x, float y, float a, float b) { d.AddLine(p(x, y), p(a, b), c, 1.8f); };
		switch (kind)
		{
		case IconKind::Play: d.AddTriangleFilled(p(.32f, .18f), p(.82f, .5f), p(.32f, .82f), c); break;
		case IconKind::Pause:
			d.AddRectFilled(p(.26f, .2f), p(.42f, .8f), c, 1.f);
			d.AddRectFilled(p(.58f, .2f), p(.74f, .8f), c, 1.f); break;
		case IconKind::Step:
			d.AddTriangleFilled(p(.2f, .22f), p(.65f, .5f), p(.2f, .78f), c);
			line(.78f, .2f, .78f, .8f); break;
		case IconKind::Camera:
			d.AddRect(p(.12f, .3f), p(.88f, .8f), c, 3.f, 0, 1.8f);
			d.AddCircle(p(.5f, .55f), s * .16f, c, 16, 1.8f); line(.25f, .3f, .35f, .18f); line(.35f, .18f, .58f, .18f); break;
		case IconKind::Move:
			line(.2f, .5f, .8f, .5f); line(.5f, .2f, .5f, .8f);
			line(.2f, .5f, .34f, .36f); line(.2f, .5f, .34f, .64f);
			line(.8f, .5f, .66f, .36f); line(.8f, .5f, .66f, .64f);
			line(.5f, .2f, .36f, .34f); line(.5f, .2f, .64f, .34f); break;
		case IconKind::Rotate:
			d.PathArcTo(p(.5f, .5f), s * .31f, .4f, 5.3f, 24); d.PathStroke(c, 0, 1.8f);
			d.AddTriangleFilled(p(.65f, .10f), p(.86f, .30f), p(.59f, .33f), c); break;
		case IconKind::Folder:
			d.AddRectFilled(p(.12f, .27f), p(.48f, .43f), c, 2.f);
			d.AddRect(p(.12f, .36f), p(.88f, .80f), c, 3.f, 0, 2.f); break;
		case IconKind::Image:
			d.AddRect(p(.12f, .16f), p(.88f, .84f), c, 2.f, 0, 1.8f);
			d.AddCircleFilled(p(.66f, .34f), s * .07f, c);
			line(.16f, .74f, .4f, .48f); line(.4f, .48f, .62f, .72f); line(.62f, .72f, .76f, .58f); break;
		case IconKind::Audio:
			line(.4f, .25f, .4f, .73f); line(.4f, .25f, .78f, .17f); line(.78f, .17f, .78f, .65f);
			d.AddCircleFilled(p(.28f, .75f), s * .12f, c); d.AddCircleFilled(p(.66f, .67f), s * .12f, c); break;
		case IconKind::Json:
		case IconKind::Script:
			line(.35f, .23f, .15f, .5f); line(.15f, .5f, .35f, .77f);
			line(.65f, .23f, .85f, .5f); line(.85f, .5f, .65f, .77f);
			if (kind == IconKind::Script) line(.57f, .2f, .43f, .8f);
			else { d.AddCircleFilled(p(.5f, .4f), s * .04f, c); d.AddCircleFilled(p(.5f, .6f), s * .04f, c); } break;
		case IconKind::Actor:
			d.AddQuad(p(.5f, .12f), p(.86f, .5f), p(.5f, .88f), p(.14f, .5f), c, 1.8f); break;
		case IconKind::Prefab:
			d.AddQuad(p(.5f, .12f), p(.84f, .3f), p(.5f, .5f), p(.16f, .3f), c, 1.8f);
			line(.16f, .3f, .16f, .7f); line(.84f, .3f, .84f, .7f);
			line(.16f, .7f, .5f, .9f); line(.84f, .7f, .5f, .9f); line(.5f, .5f, .5f, .9f); break;
		case IconKind::Snap:
			for (int i = 0; i < 3; ++i) { const float v = .2f + i * .3f; line(v, .15f, v, .85f); line(.15f, v, .85f, v); } break;
		default:
			d.AddRect(p(.23f, .12f), p(.78f, .88f), c, 2.f, 0, 1.8f);
			line(.34f, .42f, .65f, .42f); line(.34f, .57f, .65f, .57f); line(.34f, .72f, .56f, .72f); break;
		}
	}

	bool IconButton(const char* id, IconKind kind, ImVec2 size, const char* tooltip, bool active)
	{
		if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ .13f, .39f, .42f, 1.f });
		const bool clicked = ImGui::Button(id, size);
		if (active) ImGui::PopStyleColor();
		const auto min = ImGui::GetItemRectMin();
		DrawEditorIcon(*ImGui::GetWindowDrawList(), kind, { min.x + 6.f, min.y + 6.f },
			{ size.x - 12.f, size.y - 12.f }, ImGui::GetColorU32(active ? ImGuiCol_CheckMark : ImGuiCol_Text));
		if (tooltip) ImGui::SetItemTooltip("%s", tooltip);
		return clicked;
	}
}
