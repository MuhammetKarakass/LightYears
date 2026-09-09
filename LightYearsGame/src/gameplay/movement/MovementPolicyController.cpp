#include "gameplay/movement/MovementPolicyController.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ly::movement
{
	namespace
	{
		bool IsFinite(float value)
		{
			return std::isfinite(value);
		}

		float SmoothStep(float value)
		{
			const float clamped = std::clamp(value, 0.f, 1.f);
			return clamped * clamped * (3.f - 2.f * clamped);
		}

	}

	bool MovementPolicyController::SetPolicy(const MovementPolicyRequest& request)
	{
		if (request.sourceId.empty())
		{
			return false;
		}
		if (request.dampingRetentionOverride.has_value() &&
			(!IsFinite(*request.dampingRetentionOverride) ||
				*request.dampingRetentionOverride < 0.f ||
				*request.dampingRetentionOverride > 1.f))
		{
			return false;
		}
		if (request.speedCapMultiplierOverride.has_value() &&
			(!IsFinite(*request.speedCapMultiplierOverride) ||
				*request.speedCapMultiplierOverride < 0.f))
		{
			return false;
		}
		if (request.speedCapFlatBonus.has_value() &&
			!IsFinite(*request.speedCapFlatBonus))
		{
			return false;
		}
		if (request.speedCapDisabledOverride.value_or(false) &&
			request.speedCapMultiplierOverride.has_value())
		{
			return false;
		}

		mPolicies[request.sourceId] = PolicyEntry{ request };
		// A new policy owns the movement rules again, so a previous normalization
		// transition must not continue underneath it.
		mNormalization.reset();
		return true;
	}

	bool MovementPolicyController::HasHigherPriority(
		const PolicyEntry& candidate,
		const PolicyEntry* current
	)
	{
		if (!current)
		{
			return true;
		}
		if (candidate.request.priority != current->request.priority)
		{
			return candidate.request.priority > current->request.priority;
		}
		return candidate.request.sourceId < current->request.sourceId;
	}

	bool MovementPolicyController::RemovePolicy(
		const MovementPolicySourceId& sourceId,
		MovementPolicyReleaseMode releaseMode,
		float currentSpeed,
		float normalizationDuration
	)
	{
		const auto policyIt = mPolicies.find(sourceId);
		if (sourceId.empty() || policyIt == mPolicies.end())
		{
			return false;
		}
		const MovementPolicyRequest removedRequest = policyIt->second.request;
		mPolicies.erase(policyIt);

		if (mPolicies.empty() && releaseMode == MovementPolicyReleaseMode::Normalize &&
			IsFinite(currentSpeed) && currentSpeed > 0.f &&
			IsFinite(normalizationDuration) && normalizationDuration > 0.f &&
			(removedRequest.dampingRetentionOverride.has_value() ||
				removedRequest.speedCapFlatBonus.has_value() ||
				removedRequest.speedCapMultiplierOverride.has_value() ||
				removedRequest.speedCapDisabledOverride.value_or(false)))
		{
			mNormalization = NormalizationState{
				currentSpeed,
				normalizationDuration,
				0.f,
				removedRequest.dampingRetentionOverride.has_value(),
				removedRequest.speedCapFlatBonus.has_value() ||
				removedRequest.speedCapMultiplierOverride.has_value() ||
					removedRequest.speedCapDisabledOverride.value_or(false)
			};
		}
		else if (mPolicies.empty())
		{
			mNormalization.reset();
		}

		return true;
	}

	void MovementPolicyController::ClearPolicies()
	{
		mPolicies.clear();
		mNormalization.reset();
	}

	MovementPolicyResolution MovementPolicyController::Resolve() const
	{
		MovementPolicyResolution resolution;
		if (const PolicyEntry* dampingPolicy = ResolveDampingPolicy())
		{
			resolution.dampingRetentionOverride =
				dampingPolicy->request.dampingRetentionOverride;
		}
		if (const PolicyEntry* speedCapPolicy = ResolveSpeedCapPolicy())
		{
			resolution.speedCapMultiplierOverride =
				speedCapPolicy->request.speedCapMultiplierOverride;
			resolution.speedCapDisabledOverride =
				speedCapPolicy->request.speedCapDisabledOverride;
		}
		resolution.speedCapFlatBonus = ResolveSpeedCapFlatBonus();
		return resolution;
	}

	float MovementPolicyController::ResolveDampingRetention(float normalRetention) const
	{
		const float clampedNormalRetention = std::clamp(normalRetention, 0.f, 1.f);
		if (const PolicyEntry* policy = ResolveDampingPolicy())
		{
			return policy->request.dampingRetentionOverride.value_or(
				clampedNormalRetention
			);
		}
		if (!mNormalization.has_value() || !mNormalization->restoreDamping)
		{
			return clampedNormalRetention;
		}

		// Normalization starts at no damping and returns smoothly to the ship's
		// normal retention coefficient instead of cutting momentum on one frame.
		return clampedNormalRetention +
			(1.f - clampedNormalRetention) * (1.f - SmoothStep(GetNormalizationProgress()));
	}

	float MovementPolicyController::ResolveSpeedCap(
		float baseMaxSpeed,
		float normalSpeedCapMultiplier
	) const
	{
		const float safeBaseMaxSpeed = std::max(
			0.f,
			baseMaxSpeed + ResolveSpeedCapFlatBonus()
		);
		const float safeNormalMultiplier = std::max(0.f, normalSpeedCapMultiplier);
		if (const PolicyEntry* policy = ResolveSpeedCapPolicy())
		{
			if (policy->request.speedCapDisabledOverride.value_or(false))
			{
				return std::numeric_limits<float>::infinity();
			}
			// An override is intentionally relative to the resolved movement base,
			// not the caller's afterburner multiplier. This prevents accidental
			// double multiplication when a policy defines its own safety cap.
			if (policy->request.speedCapMultiplierOverride.has_value())
			{
				return safeBaseMaxSpeed * policy->request.speedCapMultiplierOverride.value();
			}
		}

		const float normalMaxSpeed = safeBaseMaxSpeed * safeNormalMultiplier;
		if (!mNormalization.has_value() || !mNormalization->restoreSpeedCap)
		{
			return normalMaxSpeed;
		}

		const float progress = SmoothStep(GetNormalizationProgress());
		return std::max(normalMaxSpeed,
			mNormalization->initialSpeed +
				(normalMaxSpeed - mNormalization->initialSpeed) * progress);
	}

	void MovementPolicyController::Tick(float deltaTime)
	{
		if (!mNormalization.has_value())
		{
			return;
		}

		mNormalization->elapsed += std::max(0.f, deltaTime);
		if (mNormalization->elapsed >= mNormalization->duration)
		{
			mNormalization.reset();
		}
	}

	bool MovementPolicyController::IsNormalizing() const
	{
		return mNormalization.has_value();
	}

	float MovementPolicyController::GetNormalizationProgress() const
	{
		if (!mNormalization.has_value() || mNormalization->duration <= 0.f)
		{
			return 1.f;
		}
		return std::clamp(
			mNormalization->elapsed / mNormalization->duration,
			0.f,
			1.f
		);
	}

	const MovementPolicyController::PolicyEntry*
	MovementPolicyController::ResolveDampingPolicy() const
	{
		const PolicyEntry* resolved = nullptr;
		for (const auto& [sourceId, entry] : mPolicies)
		{
			(void)sourceId;
			if (!entry.request.dampingRetentionOverride.has_value())
			{
				continue;
			}
			if (HasHigherPriority(entry, resolved))
			{
				resolved = &entry;
			}
		}
		return resolved;
	}

	const MovementPolicyController::PolicyEntry*
	MovementPolicyController::ResolveSpeedCapPolicy() const
	{
		const PolicyEntry* resolved = nullptr;
		for (const auto& [sourceId, entry] : mPolicies)
		{
			(void)sourceId;
			if (!entry.request.speedCapMultiplierOverride.has_value() &&
				!entry.request.speedCapDisabledOverride.has_value())
			{
				continue;
			}
			if (HasHigherPriority(entry, resolved))
			{
				resolved = &entry;
			}
		}
		return resolved;
	}

	float MovementPolicyController::ResolveSpeedCapFlatBonus() const
	{
		float total = 0.f;
		for (const auto& [sourceId, entry] : mPolicies)
		{
			(void)sourceId;
			total += entry.request.speedCapFlatBonus.value_or(0.f);
		}
		return total;
	}
}
