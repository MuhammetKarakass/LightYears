#include "editor/GizmoMath.h"
#include <cmath>
#include <iostream>

int main()
{
	using namespace vega;
	int failures = 0;
	const auto check = [&](bool passed, const char* message)
	{
		if (!passed) { ++failures; std::cerr << message << '\n'; }
	};
	const sf::Vector2f origin{ 320.f, 240.f };
	check(HitMoveHandle(origin, origin) == GizmoHandle::Centre, "Centre hit");
	check(HitMoveHandle(origin + sf::Vector2f{ 70.f, 4.f }, origin) == GizmoHandle::X, "X arrow hit");
	check(HitMoveHandle(origin + sf::Vector2f{ -4.f, -80.f }, origin) == GizmoHandle::Y, "Y arrow hit");
	check(HitMoveHandle(origin + sf::Vector2f{ 40.f, 12.f }, origin) == GizmoHandle::None, "Axis tolerance");
	check(HitMoveHandle(origin + sf::Vector2f{ -40.f, 0.f }, origin) == GizmoHandle::None, "Axis direction");
	check(HitRotateHandle(origin + sf::Vector2f{ 64.f, 0.f }, origin) == GizmoHandle::Ring, "Ring hit");
	check(HitRotateHandle(origin, origin) == GizmoHandle::None, "Ring centre misses");
	check(HitRotateHandle(origin + sf::Vector2f{ 72.f, 0.f }, origin) == GizmoHandle::None, "Ring tolerance");
	check(ProjectGizmoDrag({ 20.f, -7.f }, GizmoHandle::X) == sf::Vector2f{ 20.f, 0.f }, "X constraint");
	check(ProjectGizmoDrag({ 20.f, -7.f }, GizmoHandle::Y) == sf::Vector2f{ 0.f, -7.f }, "Y constraint");
	check(ProjectGizmoDrag({ 20.f, -7.f }, GizmoHandle::Centre) == sf::Vector2f{ 20.f, -7.f }, "Free movement");
	check(ProjectGizmoDrag({ 20.f, -7.f }, GizmoHandle::None) == sf::Vector2f{}, "Inactive drag");
	check(SnapGizmoValue(23.f, 16.f) == 16.f && SnapGizmoValue(24.f, 16.f) == 32.f, "Grid boundaries");
	check(SnapGizmoValue(-24.f, 16.f) == -32.f, "Negative grid");
	check(SnapGizmoValue(23.f, 0.f) == 23.f, "Invalid increment");
	check(SnapGizmoValue(38.f, 15.f) == 45.f, "Angle snapping");
	check(SnapGizmoPosition({ 23.f, 7.f }, GizmoHandle::X, 16.f) == sf::Vector2f{ 16.f, 7.f }, "X snap preserves Y");
	check(SnapGizmoPosition({ 23.f, 7.f }, GizmoHandle::Y, 16.f) == sf::Vector2f{ 23.f, 0.f }, "Y snap preserves X");
	check(SnapGizmoPosition({ -23.f, 7.f }, GizmoHandle::Centre, 16.f) == sf::Vector2f{ -16.f, 0.f }, "Free snap");
	check(std::abs(GizmoAngle(origin + sf::Vector2f{ 0.f, 20.f }, origin) - 90.f) < .001f, "Screen angle");
	check(GizmoAngleDelta(179.f, -179.f) == 2.f, "Positive wrap");
	check(GizmoAngleDelta(-179.f, 179.f) == -2.f, "Negative wrap");
	check(GizmoAngleDelta(0.f, 45.f) + GizmoAngleDelta(45.f, 90.f) == 90.f, "Incremental rotation");
	return failures ? 1 : 0;
}
