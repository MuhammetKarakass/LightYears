#include "gameplay/movement/MovementInfluenceController.h"

#include <framework/MathUtility.h>

#include <algorithm>
#include <cmath>

namespace ly::movement
{
	namespace
	{
		bool IsFinite(const sf::Vector2f& value)
		{
			return std::isfinite(value.x) && std::isfinite(value.y);
		}
	}

	void MovementInfluenceController::ApplyImpulse(const ImpulseRequest& request)
	{
		if (!IsFinite(request.velocity))
		{
			return;
		}
		mImpulses.push_back(ImpulseRequest{
			request.velocity,
			std::clamp(request.retentionPerSecond, 0.0001f, 0.9999f)
		});
	}

	void MovementInfluenceController::ClearImpulses()
	{
		mImpulses.clear();
		mResolvedImpulseOffsets.clear();
	}

	const List<sf::Vector2f>& MovementInfluenceController::AdvanceImpulses(
		float deltaTime
	)
	{
		mResolvedImpulseOffsets.clear();
		const float safeDeltaTime = std::max(0.f, deltaTime);
		if (safeDeltaTime <= 0.f)
		{
			return mResolvedImpulseOffsets;
		}

		for (auto impulse = mImpulses.begin(); impulse != mImpulses.end();)
		{
			if (GetVectorLength(impulse->velocity) <= 0.001f)
			{
				impulse = mImpulses.erase(impulse);
				continue;
			}
			mResolvedImpulseOffsets.push_back(impulse->velocity * safeDeltaTime);
			impulse->velocity *= std::pow(impulse->retentionPerSecond, safeDeltaTime);
			++impulse;
		}
		return mResolvedImpulseOffsets;
	}

	void MovementInfluenceController::SetAccelerationSource(
		const AccelerationSourceRequest& request
	)
	{
		if (request.sourceId == 0 || !IsFinite(request.acceleration))
		{
			return;
		}
		mAccelerationSources[request.sourceId] = request.acceleration;
	}

	void MovementInfluenceController::RemoveSource(MovementInfluenceSourceId sourceId)
	{
		mAccelerationSources.erase(sourceId);
		mForcedMovements.erase(sourceId);
	}

	void MovementInfluenceController::ClearAccelerationSources()
	{
		mAccelerationSources.clear();
	}

	sf::Vector2f MovementInfluenceController::ResolveAcceleration() const
	{
		sf::Vector2f result{};
		for (const auto& [sourceId, acceleration] : mAccelerationSources)
		{
			(void)sourceId;
			result += acceleration;
		}
		return result;
	}

	void MovementInfluenceController::SetForcedMovement(
		const ForcedMovementRequest& request
	)
	{
		if (request.sourceId == 0 || !IsFinite(request.velocity))
		{
			return;
		}
		mForcedMovements[request.sourceId] = request;
	}

	void MovementInfluenceController::RemoveForcedMovement(
		MovementInfluenceSourceId sourceId
	)
	{
		mForcedMovements.erase(sourceId);
	}

	void MovementInfluenceController::ClearForcedMovements()
	{
		mForcedMovements.clear();
	}

	bool MovementInfluenceController::HasForcedMovement() const
	{
		return !mForcedMovements.empty();
	}

	sf::Vector2f MovementInfluenceController::ResolveForcedVelocity() const
	{
		const ForcedMovementRequest* resolved = nullptr;
		for (const auto& [sourceId, request] : mForcedMovements)
		{
			if (!resolved || request.priority > resolved->priority ||
				(request.priority == resolved->priority && sourceId < resolved->sourceId))
			{
				resolved = &request;
			}
		}
		return resolved ? resolved->velocity : sf::Vector2f{};
	}
}
