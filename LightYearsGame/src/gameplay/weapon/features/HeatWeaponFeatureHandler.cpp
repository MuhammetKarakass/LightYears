#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "../internal/PrimaryWeaponBuiltIns.h"

#include "framework/Actor.h"
#include "attributes/AttributeMath.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr float BeamMaximumAttackSpeedHeatGainReduction = 0.35f;

		float GetBeamHeatGainMultiplier(
			const PrimaryWeaponExecutionContext& context,
			float currentHeat,
			float capacity
		)
		{
			if (context.definition.weaponType !=
				PrimaryWeaponType::BeamContinuous || capacity <= 0.f)
			{
				return 1.f;
			}

			const auto* combatant = dynamic_cast<const Combatant*>(&context.owner);
			if (!combatant)
			{
				return 1.f;
			}

			const float heatRatio = std::clamp(currentHeat / capacity, 0.f, 1.f);
			float highHeatWeight = 0.f;
			if (heatRatio > 0.5f && heatRatio < 0.75f)
			{
				highHeatWeight = (heatRatio - 0.5f) / 0.25f;
			}
			else if (heatRatio >= 0.75f)
			{
				highHeatWeight = 1.f;
			}

			const float attackSpeed = combatant->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
				OwnerAttributeIds::AttackSpeed
			);
			const float reduction = BeamMaximumAttackSpeedHeatGainReduction *
				sas::AttributeMath::SaturatingFraction(attackSpeed, sas::AttributeMath::PercentageRatingScale) *
				highHeatWeight;
			return std::clamp(1.f - reduction, 1.f - BeamMaximumAttackSpeedHeatGainReduction, 1.f);
		}

		float ResolveHeatGain(
			const PrimaryWeaponExecutionContext& context,
			float currentHeat,
			float capacity
		)
		{
			const float baseGain = std::max(0.f, sas::FindAttributeValue(
				context.attributes,
				PrimaryWeaponSchema::Feature::Heat::Gain,
				0.f
			));
			return baseGain * GetBeamHeatGainMultiplier(context, currentHeat, capacity);
		}

		float ApplyHeatGainCurve(
			const PrimaryWeaponDefinition& definition,
			float currentHeat,
			float capacity,
			float unscaledGain
		)
		{
			float result = std::clamp(currentHeat, 0.f, std::max(0.f, capacity));
			float remainingGain = std::max(0.f, unscaledGain);
			if (capacity <= 0.f || remainingGain <= 0.f)
			{
				return result;
			}
			if (definition.heatGainCurve.empty())
			{
				return std::min(capacity, result + remainingGain);
			}

			for (const HeatGainCurveSegmentDefinition& segment : definition.heatGainCurve)
			{
				const float segmentEnd =
					capacity * std::clamp(segment.endHeatPercentage, 0.f, 100.f) / 100.f;
				if (result >= segmentEnd)
				{
					continue;
				}
				if (segment.gainMultiplier <= 0.f)
				{
					return result;
				}

				const float segmentHeatRemaining = segmentEnd - result;
				const float scaledGain = remainingGain * segment.gainMultiplier;
				if (scaledGain < segmentHeatRemaining)
				{
					return result + scaledGain;
				}

				result = segmentEnd;
				remainingGain -= segmentHeatRemaining / segment.gainMultiplier;
				if (remainingGain <= 0.f || result >= capacity)
				{
					return result;
				}
			}

			return std::min(capacity, result);
		}

		class HeatWeaponFeatureHandler final : public PrimaryWeaponFeatureHandler
		{
		public:
			PrimaryWeaponFeatureType GetFeatureType() const override
			{
				return PrimaryWeaponFeatureType::Heat;
			}

			const List<sas::AttributeId>& GetAttributeRoots() const override
			{
				return PrimaryWeaponBuiltIns::HeatAttributeRoots();
			}

			const List<sas::AttributeId>& GetRuntimeValueKeys() const override
			{
				static const List<sas::AttributeId> keys{
					PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
				};
				return keys;
			}

			PrimaryWeaponValidationResult ValidateDefinition(
				const PrimaryWeaponDefinition& definition
			) const override
			{
				for (const sas::AttributeId& required : {
					PrimaryWeaponSchema::Feature::Heat::Gain,
					PrimaryWeaponSchema::Feature::Heat::Capacity,
					PrimaryWeaponSchema::Feature::Heat::Dissipation
				})
				{
					const PrimaryWeaponValidationResult result =
						PrimaryWeaponBuiltIns::RequireAttribute(
							definition,
							required,
							"Heat feature"
						);
					if (!result.isValid)
					{
						return result;
					}
				}
				const sas::GameplayAttribute* capacity =
					PrimaryWeaponBuiltIns::FindDefinitionAttribute(
						definition,
						PrimaryWeaponSchema::Feature::Heat::Capacity
					);
				if (!capacity || capacity->baseValue <= 0.f)
				{
					return { false, "Heat capacity must be greater than zero." };
				}
				if (!definition.heatGainCurve.empty())
				{
					float previousEndPercentage = 0.f;
					for (const HeatGainCurveSegmentDefinition& segment :
						definition.heatGainCurve)
					{
						if (segment.endHeatPercentage <= previousEndPercentage ||
							segment.endHeatPercentage > 100.f ||
							segment.gainMultiplier <= 0.f)
						{
							return {
								false,
								"Heat gain curve segments must have increasing end percentages within zero to one hundred and positive multipliers."
							};
						}
						previousEndPercentage = segment.endHeatPercentage;
					}
					if (std::abs(previousEndPercentage - 100.f) > 0.001f)
					{
						return {
							false,
							"Heat gain curve must end at one hundred percent heat."
						};
					}
				}

				if (definition.weaponType !=
					PrimaryWeaponType::BeamContinuous)
				{
					return { true, {} };
				}

				for (const sas::AttributeId& required : {
					PrimaryWeaponSchema::Feature::Heat::OverheatCooldown,
					PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat
				})
				{
					const PrimaryWeaponValidationResult result =
						PrimaryWeaponBuiltIns::RequireAttribute(
							definition,
							required,
							"Continuous beam heat feature"
						);
					if (!result.isValid)
					{
						return result;
					}
				}
				const sas::GameplayAttribute* cooldown =
					PrimaryWeaponBuiltIns::FindDefinitionAttribute(
						definition,
						PrimaryWeaponSchema::Feature::Heat::OverheatCooldown
					);
				const sas::GameplayAttribute* maximumDamageMultiplier =
					PrimaryWeaponBuiltIns::FindDefinitionAttribute(
						definition,
						PrimaryWeaponSchema::Feature::Heat::DamageMultiplierAtMaxHeat
					);
				return cooldown->baseValue > 0.f &&
					maximumDamageMultiplier->baseValue >= 1.f
					? PrimaryWeaponValidationResult{ true, {} }
					: PrimaryWeaponValidationResult{
						false,
						"Continuous beam heat cooldown must be positive and its maximum damage multiplier must be at least one."
					};
			}

			bool CanFire(
				const PrimaryWeaponExecutionContext& context,
				const PrimaryWeaponRuntimeState& state
			) const override
			{
				const float capacity = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Feature::Heat::Capacity,
					0.f
				));
				const float gain = ResolveHeatGain(
					context,
					state.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue),
					capacity
				);
				return state.GetFeatureValue(
					PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
				) + gain <= capacity;
			}

			void AfterFire(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponRuntimeState& state
			) const override
			{
				const float capacity = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Feature::Heat::Capacity,
					0.f
				));
				const float currentHeat = state.GetFeatureValue(
					PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
				);
				const float gain = ResolveHeatGain(context, currentHeat, capacity);
				state.SetFeatureValue(
					PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue,
					ApplyHeatGainCurve(
						context.definition,
						currentHeat,
						capacity,
						gain
					)
				);
			}

			void TickFire(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponRuntimeState& state,
				float deltaTime
			) const override
			{
				if (state.handler && !state.handler->UsesIntervalFire())
				{
					const float capacity = std::max(0.f, sas::FindAttributeValue(
						context.attributes,
						PrimaryWeaponSchema::Feature::Heat::Capacity,
						0.f
					));
					const float currentHeat = state.GetFeatureValue(
						PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
					);
					const float gain = ResolveHeatGain(context, currentHeat, capacity);
					const float current = ApplyHeatGainCurve(
						context.definition,
						currentHeat,
						capacity,
						gain * deltaTime
					);
					state.SetFeatureValue(
						PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue,
						current
					);
					if (capacity > 0.f && current >= capacity)
					{
						state.RequestCooldown(std::max(0.f, sas::FindAttributeValue(
							context.attributes,
							PrimaryWeaponSchema::Feature::Heat::OverheatCooldown,
							0.f
						)));
						state.SetFeatureValue(
							PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue,
							0.f
						);
					}
					return;
				}

				DissipateHeat(context, state, deltaTime);
			}

			void TickInactive(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponRuntimeState& state,
				float deltaTime
			) const override
			{
				DissipateHeat(context, state, deltaTime);
			}

		private:
			static void DissipateHeat(
				const PrimaryWeaponExecutionContext& context,
				PrimaryWeaponRuntimeState& state,
				float deltaTime
			)
			{
				const float dissipation = std::max(0.f, sas::FindAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Feature::Heat::Dissipation,
					0.f
				));
				state.SetFeatureValue(
					PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue,
					std::max(
						0.f,
						state.GetFeatureValue(
							PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue
						) - dissipation * deltaTime
					)
				);
			}
		};
	}

	namespace PrimaryWeaponBuiltIns
	{
		unique_ptr<PrimaryWeaponFeatureHandler> CreateHeatFeatureHandler()
		{
			return std::make_unique<HeatWeaponFeatureHandler>();
		}
	}
}
