#include "attributes/AttributeSystem.h"
#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include <algorithm>
#include <cmath>

namespace vega
{
	namespace
	{
		// Owns Whiteout timing for Cryo-style wave weapons. Waves award meter
		// points through the shared PrimaryWeaponFrostMeterState as they hit; this
		// feature advances Whiteout and its lockout and tells the execution system
		// how fast and how empowered the next wave is.
		class FrostMeterWeaponFeatureHandler final : public PrimaryWeaponFeatureHandler
		{
		public:
			PrimaryWeaponFeatureType GetFeatureType() const override
			{
				return PrimaryWeaponFeatureType::FrostMeter;
			}

			void BeginFire(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponRuntimeState& state
			) const override
			{
				Sync(context, state);
			}

			void AfterFire(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponRuntimeState& state
			) const override
			{
				if (state.frostMeter && state.frostMeter->IsWhiteout())
				{
					// Whiteout alternates empowered and normal waves.
					state.frostMeter->nextWhiteoutShotEmpowered = !state.frostMeter->nextWhiteoutShotEmpowered;
				}
				Sync(context, state);
			}

			void TickFire(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponRuntimeState& state,
				float deltaTime
			) const override
			{
				Advance(state, deltaTime);
				Sync(context, state);
			}

			void TickInactive(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponRuntimeState& state,
				float deltaTime
			) const override
			{
				Advance(state, deltaTime);
				Sync(context, state);
			}

		private:
			static void Advance(PrimaryWeaponRuntimeState& state, float deltaTime)
			{
				PrimaryWeaponFrostMeterState* frost = state.frostMeter.get();
				if (!frost || deltaTime <= 0.f)
				{
					return;
				}
				if (frost->IsWhiteout())
				{
					frost->whiteoutRemaining = std::max(0.f, frost->whiteoutRemaining - deltaTime);
					if (!frost->IsWhiteout())
					{
						frost->meter = 0;
						frost->nextWhiteoutShotEmpowered = false;
						frost->lockoutRemaining = frost->lockoutDuration;
					}
					return;
				}
				frost->lockoutRemaining = std::max(0.f, frost->lockoutRemaining - deltaTime);
			}

			static void Sync(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state)
			{
				PrimaryWeaponFrostMeterState* frost = state.frostMeter.get();
				if (!frost)
				{
					return;
				}
				const auto find = [&](const sas::AttributeId& id, float fallback)
				{
					return sas::FindAttributeValue(context.attributes, id, fallback);
				};
				frost->maxMeter = std::max(1, static_cast<int>(std::round(
					find(PrimaryWeaponSchema::Feature::FrostMeter::Max, 4.f))));
				frost->whiteoutDuration = std::max(0.f, find(PrimaryWeaponSchema::Feature::FrostMeter::WhiteoutDuration, 4.f));
				frost->lockoutDuration = std::max(0.f, find(PrimaryWeaponSchema::Feature::FrostMeter::LockoutDuration, 3.f));
				const float speedUp = std::max(0.01f, find(PrimaryWeaponSchema::Feature::FrostMeter::WhiteoutFireRateMultiplier, 1.f));
				state.featureFireIntervalScale = frost->IsWhiteout() ? 1.f / speedUp : 1.f;
				state.featureRequestsEmpoweredShot = frost->IsWhiteout() && frost->nextWhiteoutShotEmpowered;
			}
		};
	}

	namespace PrimaryWeaponBuiltIns
	{
		unique_ptr<PrimaryWeaponFeatureHandler> CreateFrostMeterFeatureHandler()
		{
			return std::make_unique<FrostMeterWeaponFeatureHandler>();
		}
	}
}
