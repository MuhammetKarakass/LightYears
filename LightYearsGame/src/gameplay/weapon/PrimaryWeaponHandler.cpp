#include "gameplay/weapon/PrimaryWeaponHandler.h"

#include <algorithm>

namespace ly
{
	namespace
	{
		const List<GameplayTag> EmptyTags{};
		const List<sas::AttributeId> EmptyAttributeRoots{};
	}

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

	const List<sas::AttributeId>& PrimaryWeaponHandler::GetInheritedAttributeRoots() const
	{
		return EmptyAttributeRoots;
	}

	PrimaryWeaponValidationResult PrimaryWeaponHandler::ValidateDefinition(
		const PrimaryWeaponDefinition&
	) const
	{
		return { true, {} };
	}

	unique_ptr<PrimaryWeaponTypeRuntimeState>
		PrimaryWeaponHandler::CreateRuntimeState() const
	{
		return std::make_unique<PrimaryWeaponTypeRuntimeState>();
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

	PrimaryWeaponValidationResult PrimaryWeaponFeatureHandler::ValidateDefinition(
		const PrimaryWeaponDefinition&
	) const
	{
		return { true, {} };
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
