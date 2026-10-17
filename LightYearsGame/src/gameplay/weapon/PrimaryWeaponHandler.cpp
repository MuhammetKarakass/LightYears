#include "gameplay/weapon/PrimaryWeaponHandler.h"
#include "gameplay/attributes/AttributeIds.h"

#include <algorithm>

namespace vega
{
	float PrimaryWeaponRuntimeState::GetFeatureValue(
		const sas::AttributeId& key,
		float fallback
	) const
	{
		auto found = featureValues.find(key);
		return found != featureValues.end() ? found->second : fallback;
	}

	void PrimaryWeaponRuntimeState::SetFeatureValue(
		const sas::AttributeId& key,
		float value
	)
	{
		featureValues[key] = value;
	}

	void PrimaryWeaponRuntimeState::RequestCooldown(float duration)
	{
		requestedCooldown = std::max(requestedCooldown, std::max(0.f, duration));
	}

	float PrimaryWeaponRuntimeState::ConsumeRequestedCooldown()
	{
		const float cooldown = requestedCooldown;
		requestedCooldown = 0.f;
		return cooldown;
	}

	float ResolvePrimaryWeaponShotDamage(const PrimaryWeaponExecutionContext& context)
	{
		float damage = sas::FindAttributeValue(context.attributes, CommonAttributeIds::Damage, 0.f);
		if (context.shotMetadata.isEmpowered && context.definition.empoweredShot.has_value())
		{
			damage += sas::FindAttributeValue(
				context.attributes,
				PrimaryWeaponSchema::Empowered::BonusDamage,
				0.f
			);
		}
		return std::max(0.f, damage * context.shotMetadata.shotDamageMultiplier);
	}

	unique_ptr<PrimaryWeaponTypeRuntimeState> PrimaryWeaponHandler::CreateRuntimeState() const
	{
		return std::make_unique<PrimaryWeaponTypeRuntimeState>();
	}

	bool PrimaryWeaponHandler::UsesIntervalFire() const
	{
		const PrimaryWeaponTypeValidationContract* contract =
			PrimaryWeaponValidationContractRegistry::FindType(GetType());
		return contract && contract->usesIntervalFire;
	}

	void PrimaryWeaponHandler::BeginFire(
		const PrimaryWeaponExecutionContext&,
		PrimaryWeaponTypeRuntimeState&
	) const
	{
	}

	void PrimaryWeaponHandler::TickFire(
		const PrimaryWeaponExecutionContext&,
		PrimaryWeaponTypeRuntimeState&,
		float
	) const
	{
	}

	void PrimaryWeaponHandler::EndFire(
		const PrimaryWeaponExecutionContext&,
		PrimaryWeaponTypeRuntimeState&
	) const
	{
	}

	const List<sas::AttributeId>& PrimaryWeaponFeatureHandler::GetRuntimeValueKeys() const
	{
		static const List<sas::AttributeId> emptyKeys{};
		return emptyKeys;
	}

	void PrimaryWeaponFeatureHandler::BeginFire(
		const PrimaryWeaponExecutionContext&,
		PrimaryWeaponRuntimeState&
	) const
	{
	}

	bool PrimaryWeaponFeatureHandler::CanFire(
		const PrimaryWeaponExecutionContext&,
		const PrimaryWeaponRuntimeState&
	) const
	{
		return true;
	}

	void PrimaryWeaponFeatureHandler::AfterFire(
		const PrimaryWeaponExecutionContext&,
		PrimaryWeaponRuntimeState&
	) const
	{
	}

	void PrimaryWeaponFeatureHandler::TickFire(
		const PrimaryWeaponExecutionContext&,
		PrimaryWeaponRuntimeState&,
		float
	) const
	{
	}

	void PrimaryWeaponFeatureHandler::TickInactive(
		const PrimaryWeaponExecutionContext&,
		PrimaryWeaponRuntimeState&,
		float
	) const
	{
	}

	void PrimaryWeaponFeatureHandler::EndFire(
		const PrimaryWeaponExecutionContext&,
		PrimaryWeaponRuntimeState&
	) const
	{
	}
}
