#include "editor/EditorSkyEditor.h"
#include <imgui.h>
#include <cmath>
#include <algorithm>

namespace vega
{
	namespace
	{
		constexpr float Rad = 0.01745329252f;
		float Length(sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); }
		float Dot(sf::Vector2f a, sf::Vector2f b) { return a.x * b.x + a.y * b.y; }
	}

	bool EditorSkyEditor::Drag(const std::string& handle, sf::Vector2f delta, std::uint64_t group)
	{
		if (!mDocument || !SkyDocument::IsHandle(handle)) return false;
		const auto number = [&](const std::string& path) { return mDocument->Value(path).get<float>(); };
		const auto setNumber = [&](const std::string& path, float value, float snap)
		{
			if (mSnap) value = std::round(value / snap) * snap;
			const auto* field = ly::visual::FindSkyField(path);
			if (field->min) value = std::max(value, *field->min);
			if (field->max) value = std::min(value, *field->max);
			return mDocument->Set(path, value, group);
		};
		const auto move = [&](const std::string& path, bool mirror, sf::Vector2f fallback)
		{
			const auto effective = mDocument->Effective();
			const auto* authored = mDocument->Authored(path);
			Json at = mDocument->Value(path);
			if (!authored && path == "planet.at" && !effective.value("planet", Json::object()).contains("at")) at = {mirror ? 1.f - fallback.x : fallback.x, fallback.y};
			float x = at[0].get<float>() + delta.x * (mirror ? -1.f : 1.f);
			float y = at[1].get<float>() + delta.y;
			if (mSnap) { x = std::round(x * 100.f) / 100.f; y = std::round(y * 100.f) / 100.f; }
			return mDocument->Set(path, Json{x, y}, group);
		};
		if (handle == "planet")
		{
			if (mDocument->Effective().contains("orbit")) { mDocument->error = "Orbit planet position is fixed by its framing; use orbit handles"; return false; }
			return move("planet.at", mGeometry.mirrored, mGeometry.planetAt);
		}
		if (handle == "planet.scale") return setNumber("planet.scale", number("planet.scale") + delta.y / std::max(.01f, mPipeline->EditorAssets().Settings().planet.radius * mGeometry.planetBaseScale), .05f);
		if (handle == "band.angle")
		{
			const float before = mDocument->Authored("band.angleDeg") ? number("band.angleDeg") : mGeometry.band.angleDeg * (mGeometry.mirrored ? -1.f : 1.f);
			return setNumber("band.angleDeg", before + delta.x * 360.f * (mGeometry.mirrored ? -1.f : 1.f), 15.f);
		}
		if (handle == "band.offset")
		{
			const float angle = mGeometry.band.angleDeg * Rad;
			const sf::Vector2f normal{-std::sin(angle), std::cos(angle)};
			const float before = mDocument->Authored("band.offset") ? number("band.offset") : mGeometry.band.offset;
			return setNumber("band.offset", before + Dot({delta.x * mOutput.x / mOutput.y, delta.y}, normal), .01f);
		}
		if (handle.find("orbit.") == 0)
		{
			if (!mDocument->Effective().contains("orbit")) { mDocument->error = "No orbit at this stop"; return false; }
			if (handle == "orbit.disk") return setNumber("orbit.diskRadius", number("orbit.diskRadius") + delta.y, .01f);
			if (handle == "orbit.angle") return setNumber("orbit.horizonAngleDeg", number("orbit.horizonAngleDeg") + delta.x * 360.f, 15.f);
			if (handle == "orbit.radius") return setNumber("orbit.radius", number("orbit.radius") + delta.x * 10.f, .1f);
			const float angle = number("orbit.horizonAngleDeg") * Rad;
			return setNumber("orbit.coverage", number("orbit.coverage") + Dot({delta.x * mOutput.x / mOutput.y, delta.y}, {std::cos(angle), std::sin(angle)}), .01f);
		}
		const auto dot = handle.find('.', 7);
		const auto index = handle.substr(7, dot == std::string::npos ? dot : dot - 7);
		const std::string path = "objects." + index;
		if (dot == std::string::npos) return move(path + ".at", mGeometry.mirrored && mDocument->Value("mirror").get<bool>(), {});
		if (handle.substr(dot) == ".scale") return setNumber(path + ".size", number(path + ".size") + delta.y, .01f);
		return setNumber(path + ".angleDeg", number(path + ".angleDeg") + delta.x * 360.f * (mGeometry.mirrored && mDocument->Value("mirror").get<bool>() ? -1.f : 1.f), 15.f);
	}

	void EditorSkyEditor::Handles(const sf::FloatRect& image, sf::Vector2u textureSize, bool hovered)
	{
		if (!Active() || mCompare || image.size.x <= 0.f || image.size.y <= 0.f) { mDragHandle.clear(); return; }
		mGeometry = mPipeline->EditorBackdrop().EditorGeometry(mPipeline->EditorAssets(), textureSize);
		CacheRows();
		const auto origin = ImGui::GetMainViewport()->Pos;
		const sf::Vector2f top{image.position.x + origin.x, image.position.y + origin.y};
		const auto screen = [&](sf::Vector2f p) { return sf::Vector2f{top.x + p.x * image.size.x, top.y + p.y * image.size.y}; };
		const auto im = [](sf::Vector2f p) { return ImVec2{p.x, p.y}; };
		const auto mousePos = ImGui::GetMousePos();
		const sf::Vector2f mouse{mousePos.x, mousePos.y};
		auto* draw = ImGui::GetWindowDrawList();
		draw->PushClipRect(im(top), im(top + image.size), true);
		std::string hover;
		float best = 12.f;
		const auto point = [&](const std::string& handle, sf::Vector2f p, const char* label)
		{
			const float distance = Length(mouse - p);
			if (hovered && distance < best) { best = distance; hover = handle; }
			const auto color = mDragHandle == handle ? IM_COL32(255, 230, 128, 255) : IM_COL32(102, 222, 228, 240);
			draw->AddCircleFilled(im(p), 5.f, color);
			draw->AddCircle(im(p), 8.f, IM_COL32(12, 20, 28, 230), 20, 2.f);
			draw->AddText(im(p + sf::Vector2f{11.f, 7.f}), color, label);
		};
		const auto circle = [&](const std::string& prefix, sf::Vector2f p, float radius, bool rotate)
		{
			draw->AddCircle(im(p), radius, IM_COL32(102, 222, 228, 170), 96, 1.5f);
			const float distance = std::abs(Length(mouse - p) - radius);
			if (hovered && distance < 5.f && distance < best) { best = distance; hover = prefix + ".scale"; }
			point(prefix, p, prefix.c_str());
			point(prefix + ".scale", p + sf::Vector2f{0.f, radius}, "scale");
			if (rotate)
			{
				const auto index = prefix.substr(7);
				float angle = mDocument->Value("objects." + index + ".angleDeg").get<float>();
				if (mGeometry.mirrored && mDocument->Value("mirror").get<bool>()) angle = -angle;
				point(prefix + ".rotate", p + sf::Vector2f{std::cos(angle * Rad), std::sin(angle * Rad)} * (radius + 18.f), "rotate");
			}
		};
		if (mEffective.contains("orbit"))
		{
			ly::visual::OrbitSettings orbit;
			if (const auto* stop = mPipeline->EditorAssets().DeepSky().Find(mPipeline->Route().Stops()[mStopIndex].id); stop && stop->orbit) orbit = *stop->orbit;
			const auto view = ly::visual::OrbitSky::MakeView(orbit, image.size, 0.f, {});
			ly::visual::PlanetParams planet; ly::visual::OrbitSky::PlacePlanet(planet, orbit, view, {});
			const sf::Vector2f center = top + planet.center;
			if (orbit.framing == ly::visual::OrbitFraming::Disk)
			{
				draw->AddCircle(im(center), planet.radius, IM_COL32(102, 222, 228, 190), 100);
				point("orbit.disk", center + sf::Vector2f{0.f, planet.radius}, "disk radius");
			}
			else
			{
				const sf::Vector2f apex = center - view.flow * planet.radius;
				const auto across = view.Across();
				draw->AddLine(im(apex - across * image.size.x), im(apex + across * image.size.x), IM_COL32(102, 222, 228, 180), 2.f);
				point("orbit.horizon", apex, "coverage");
				point("orbit.radius", apex + across * image.size.y * .22f, "curvature");
			}
			point("orbit.angle", screen({.5f, .5f}) + sf::Vector2f{std::cos(orbit.horizonAngleDeg * Rad), std::sin(orbit.horizonAngleDeg * Rad)} * image.size.y * .35f, "orbit direction");
		}
		else
		{
			if (mGeometry.hasPlanet) circle("planet", screen(mGeometry.planetAt), mGeometry.planetRadius * image.size.y, false);
			if (mEffective.contains("objects")) for (std::size_t i = 0; i < mEffective["objects"].size(); ++i)
			{
				const std::string path = "objects." + std::to_string(i);
				const auto at = mDocument->Value(path + ".at");
				sf::Vector2f p{at[0].get<float>(), at[1].get<float>()};
				if (mGeometry.mirrored && mDocument->Value("mirror").get<bool>()) p.x = 1.f - p.x;
				circle("object." + std::to_string(i), screen(p - mGeometry.bandDrift), mDocument->Value(path + ".size").get<float>() * image.size.y, true);
			}
			const float angle = mGeometry.band.angleDeg * Rad;
			const sf::Vector2f direction{std::cos(angle), std::sin(angle)}, normal{-direction.y, direction.x};
			const auto center = screen(sf::Vector2f{.5f, .5f} - mGeometry.bandDrift) + normal * (mGeometry.band.offset * image.size.y);
			draw->AddLine(im(center - direction * image.size.x), im(center + direction * image.size.x), IM_COL32(240, 188, 108, 180), 2.f);
			point("band.offset", center, "band offset");
			point("band.angle", center + direction * (image.size.y * .32f), "band angle");
		}
		if (!hover.empty()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
		if (hovered && !hover.empty() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			mDragHandle = hover; mDragStart = mDragPrevious = mouse; mDragTotal = {}; mGroup = mDocument->history.BeginMergeGroup();
		}
		if (!mDragHandle.empty())
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				sf::Vector2f total{(mouse.x - mDragStart.x) / image.size.x, (mouse.y - mDragStart.y) / image.size.y};
				if (mSnap) { total.x = std::round(total.x * 100.f) / 100.f; total.y = std::round(total.y * 100.f) / 100.f; }
				sf::Vector2f delta = total - mDragTotal;
				const bool rotate = mDragHandle.find("rotate") != std::string::npos || mDragHandle == "band.angle" || mDragHandle == "orbit.angle";
				const bool scale = mDragHandle.find(".scale") != std::string::npos || mDragHandle == "orbit.disk";
				if (scale)
				{
					sf::Vector2f center = screen(mGeometry.planetAt);
					if (mDragHandle == "orbit.disk") center = screen({.5f, .5f});
					else if (mDragHandle.find("object.") == 0)
					{
						const auto end = mDragHandle.find('.', 7); const auto at = mDocument->Value("objects." + mDragHandle.substr(7, end - 7) + ".at");
						sf::Vector2f p{at[0].get<float>(), at[1].get<float>()}; if (mGeometry.mirrored && mDocument->Value("mirror").get<bool>()) p.x = 1.f - p.x;
						center = screen(p - mGeometry.bandDrift);
					}
					float radial = (Length(mouse - center) - Length(mDragPrevious - center)) / image.size.y;
					if (mSnap && std::abs(radial) < .01f) radial = 0.f;
					delta = {0.f, radial};
					if (radial != 0.f) mDragPrevious = mouse;
				}
				if (rotate)
				{
					sf::Vector2f center = screen({.5f, .5f});
					if (mDragHandle == "band.angle") center = screen(sf::Vector2f{.5f, .5f} - mGeometry.bandDrift) + sf::Vector2f{-std::sin(mGeometry.band.angleDeg * Rad), std::cos(mGeometry.band.angleDeg * Rad)} * (mGeometry.band.offset * image.size.y);
					else if (mDragHandle.find("object.") == 0)
					{
						const auto end = mDragHandle.find('.', 7); const auto at = mDocument->Value("objects." + mDragHandle.substr(7, end - 7) + ".at");
						sf::Vector2f p{at[0].get<float>(), at[1].get<float>()}; if (mGeometry.mirrored && mDocument->Value("mirror").get<bool>()) p.x = 1.f - p.x;
						center = screen(p - mGeometry.bandDrift);
					}
					const auto a = mDragPrevious - center, b = mouse - center;
					float degrees = std::remainder((std::atan2(b.y, b.x) - std::atan2(a.y, a.x)) / Rad, 360.f);
					if (mSnap && std::abs(degrees) < 15.f) degrees = 0.f;
					delta = {degrees / 360.f, 0.f};
					if (degrees != 0.f) mDragPrevious = mouse;
				}
				if (delta != sf::Vector2f{}) Drag(mDragHandle, delta, mGroup);
				mDragTotal = total;
			}
			else mDragHandle.clear();
		}
		draw->PopClipRect();
	}
}
