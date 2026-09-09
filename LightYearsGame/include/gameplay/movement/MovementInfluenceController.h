#pragma once

#include "framework/Core.h"
#include "gameplay/movement/MovementInfluenceTypes.h"

#include <map>

namespace ly::movement
{
	// Per-ship state and deterministic resolution for gameplay-authored movement.
	// Abilities calculate their own spatial math; this controller owns stacking,
	// decay, source lifetime, and forced-movement precedence.
	class MovementInfluenceController final
	{
	public:
		void ApplyImpulse(const ImpulseRequest& request);
		void ClearImpulses();
		const List<sf::Vector2f>& AdvanceImpulses(float deltaTime);

		void SetAccelerationSource(const AccelerationSourceRequest& request);
		void RemoveSource(MovementInfluenceSourceId sourceId);
		void ClearAccelerationSources();
		sf::Vector2f ResolveAcceleration() const;

		void SetForcedMovement(const ForcedMovementRequest& request);
		void RemoveForcedMovement(MovementInfluenceSourceId sourceId);
		void ClearForcedMovements();
		bool HasForcedMovement() const;
		sf::Vector2f ResolveForcedVelocity() const;

	private:
		List<ImpulseRequest> mImpulses;
		List<sf::Vector2f> mResolvedImpulseOffsets;
		std::map<MovementInfluenceSourceId, sf::Vector2f>
			mAccelerationSources;
		std::map<MovementInfluenceSourceId, ForcedMovementRequest>
			mForcedMovements;
	};
}
