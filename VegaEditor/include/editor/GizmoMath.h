#pragma once

#include <SFML/System/Vector2.hpp>

namespace vega
{
	enum class EditorTool { Move, Rotate };
	enum class GizmoHandle { None, X, Y, Centre, Ring };

	struct EditorViewSettings
	{
		EditorTool tool = EditorTool::Move;
		bool snap = false;
		bool grid = true;
		bool colliders = false;
		bool contentGrid = true;
		bool requestContent = false;
		float gridSize = 16.f;
	};

	// All hit tests use screen pixels so zoom never changes the interaction target.
	GizmoHandle HitMoveHandle(sf::Vector2f mouse, sf::Vector2f origin);
	GizmoHandle HitRotateHandle(sf::Vector2f mouse, sf::Vector2f origin);
	sf::Vector2f ProjectGizmoDrag(sf::Vector2f delta, GizmoHandle handle);
	float SnapGizmoValue(float value, float increment);
	sf::Vector2f SnapGizmoPosition(sf::Vector2f value, GizmoHandle handle, float increment);
	float GizmoAngle(sf::Vector2f point, sf::Vector2f origin);
	float GizmoAngleDelta(float previous, float current);
}
