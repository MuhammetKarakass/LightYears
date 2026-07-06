#include "level/ArenaBoundaryIndicator.h"


namespace ly
{
	ArenaBoundaryIndicator::ArenaBoundaryIndicator(World* owningWorld, const ArenaDefinition& arenaDefinition):
		Actor{ owningWorld },
		mArenaDefinition{ arenaDefinition },
		mWarningActive{ false }
	{
	}

	void ArenaBoundaryIndicator::Render(sf::RenderWindow& window)
	{
		if(GetIsPendingDestroy() || !mArenaDefinition.boundaryVisual.enabled)
			return;

		switch (mArenaDefinition.boundaryVisual.visualType)
		{
		case ArenaBoundaryVisualType::DebugRectangle:
		default:
			RenderDebugRectangle(window);
			break;
		}
	}
	void ArenaBoundaryIndicator::SetWarningActive(bool active)
	{
		mWarningActive = active;
	}
	void ArenaBoundaryIndicator::RenderDebugRectangle(sf::RenderWindow& window)
	{
		const ArenaBoundaryVisualDefinition& visualDef = mArenaDefinition.boundaryVisual;

		sf::RectangleShape boundaryShape;
		boundaryShape.setPosition(mArenaDefinition.legalBounds.position);
		boundaryShape.setSize(mArenaDefinition.legalBounds.size);
		boundaryShape.setFillColor(sf::Color::Transparent);
		boundaryShape.setOutlineColor(mWarningActive ? visualDef.warningColor : visualDef.outlineColor);
		boundaryShape.setOutlineThickness(visualDef.outlineThickness);

		window.draw(boundaryShape);
	}
}