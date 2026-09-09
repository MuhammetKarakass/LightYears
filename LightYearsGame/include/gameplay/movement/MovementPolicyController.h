#pragma once

#include "gameplay/movement/MovementPolicyTypes.h"

#include <map>

namespace ly::movement
{
	// Resolves generic movement-rule overrides and the transition back to normal
	// movement. This class has no dependency on abilities, ships, tags, or
	// gameplay content, so it can be reused by any movement-owning actor.
	class MovementPolicyController final
	{
	public:
		bool SetPolicy(const MovementPolicyRequest& request);
		bool RemovePolicy(
			const MovementPolicySourceId& sourceId,
			MovementPolicyReleaseMode releaseMode,
			float currentSpeed,
			float normalizationDuration
		);
		void ClearPolicies();

		MovementPolicyResolution Resolve() const;
		float ResolveDampingRetention(float normalRetention) const;
		float ResolveSpeedCap(float baseMaxSpeed, float normalSpeedCapMultiplier) const;

		void Tick(float deltaTime);
		bool IsNormalizing() const;
		float GetNormalizationProgress() const;

	private:
		struct PolicyEntry
		{
			MovementPolicyRequest request;
		};

		struct NormalizationState
		{
			float initialSpeed = 0.f;
			float duration = 0.f;
			float elapsed = 0.f;
			bool restoreDamping = false;
			bool restoreSpeedCap = false;
		};

		const PolicyEntry* ResolveDampingPolicy() const;
		const PolicyEntry* ResolveSpeedCapPolicy() const;
		float ResolveSpeedCapFlatBonus() const;
		static bool HasHigherPriority(
			const PolicyEntry& candidate,
			const PolicyEntry* current
		);

		std::map<MovementPolicySourceId, PolicyEntry> mPolicies;
		std::optional<NormalizationState> mNormalization;
	};
}
