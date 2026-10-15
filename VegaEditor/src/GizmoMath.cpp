#include "editor/GizmoMath.h"
#include <cmath>

namespace vega
{
	GizmoHandle HitMoveHandle(sf::Vector2f mouse, sf::Vector2f origin)
	{
		const auto d = mouse - origin;
		if (std::abs(d.x) <= 9.f && std::abs(d.y) <= 9.f) return GizmoHandle::Centre;
		if (d.x >= 13.f && d.x <= 86.f && std::abs(d.y) <= 8.f) return GizmoHandle::X;
		if (d.y <= -13.f && d.y >= -86.f && std::abs(d.x) <= 8.f) return GizmoHandle::Y;
		return GizmoHandle::None;
	}

	GizmoHandle HitRotateHandle(sf::Vector2f mouse, sf::Vector2f origin)
	{
		const auto d = mouse - origin;
		return std::abs(std::hypot(d.x, d.y) - 64.f) <= 7.f ? GizmoHandle::Ring : GizmoHandle::None;
	}

	sf::Vector2f ProjectGizmoDrag(sf::Vector2f delta, GizmoHandle handle)
	{
		if (handle == GizmoHandle::X) return { delta.x, 0.f };
		if (handle == GizmoHandle::Y) return { 0.f, delta.y };
		return handle == GizmoHandle::Centre ? delta : sf::Vector2f{};
	}

	float SnapGizmoValue(float value, float increment)
	{
		return std::isfinite(increment) && increment > 0.f ? std::round(value / increment) * increment : value;
	}

	sf::Vector2f SnapGizmoPosition(sf::Vector2f value, GizmoHandle handle, float increment)
	{
		if (handle == GizmoHandle::X || handle == GizmoHandle::Centre) value.x = SnapGizmoValue(value.x, increment);
		if (handle == GizmoHandle::Y || handle == GizmoHandle::Centre) value.y = SnapGizmoValue(value.y, increment);
		return value;
	}

	float GizmoAngle(sf::Vector2f point, sf::Vector2f origin)
	{
		const auto d = point - origin;
		return std::atan2(d.y, d.x) * 180.f / 3.141592654f;
	}

	float GizmoAngleDelta(float previous, float current)
	{
		return std::remainder(current - previous, 360.f);
	}
}
