#include "gameplay/weapon/PrimaryWeaponRegistry.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "weapon/Bullet.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace ly
{
	namespace
	{
		using PrimaryWeaponHandlerMap = Dictionary<
			GameplayTag,
			unique_ptr<PrimaryWeaponHandler>,
			GameplayTagHash
		>;
		using PrimaryWeaponFeatureMap = Dictionary<
			GameplayTag,
			unique_ptr<PrimaryWeaponFeatureHandler>,
			GameplayTagHash
		>;

		PrimaryWeaponHandlerMap& GetHandlers()
		{
			static PrimaryWeaponHandlerMap handlers;
			return handlers;
		}

		PrimaryWeaponFeatureMap& GetFeatures()
		{
			static PrimaryWeaponFeatureMap features;
			return features;
		}

		const List<GameplayTag> EmptyTags{};
		const List<GameplayTag> ProjectileDeliveryAttributeRoots{
			PrimaryWeaponSchema::Projectile::Delivery::AttributeRoot
		};
		const List<GameplayTag> ShotgunAttributeRoots{
			PrimaryWeaponSchema::Projectile::Shotgun::AttributeRoot
		};
		const List<GameplayTag> HeatAttributeRoots{
			PrimaryWeaponSchema::Feature::Heat::AttributeRoot
		};

		bool HasExactTag(const List<GameplayTag>& tags, const GameplayTag& expectedTag)
		{
			return std::any_of(tags.begin(), tags.end(), [&](const GameplayTag& tag)
			{
				return tag.MatchesTagExact(expectedTag);
			});
		}

		bool MatchesAnyRoot(const GameplayTag& tag, const List<GameplayTag>& roots)
		{
			return std::any_of(roots.begin(), roots.end(), [&](const GameplayTag& root)
			{
				return tag.MatchesTag(root);
			});
		}

		bool IsCommonWeaponAttribute(const GameplayTag& attributeId)
		{
			static const std::array<GameplayTag, 6> supportedAttributes{
				CommonAttributeIds::Damage,
				CommonAttributeIds::FireRate,
				CommonAttributeIds::Interval,
				CommonAttributeIds::Range,
				CommonAttributeIds::CollisionRadius,
				CommonAttributeIds::AreaRadius
			};
			return std::any_of(supportedAttributes.begin(), supportedAttributes.end(), [&](const GameplayTag& supported)
			{
				return attributeId.MatchesTagExact(supported);
			});
		}

		const PrimaryWeaponHandler* FindHandler(const GameplayTag& weaponTypeTag);
		const PrimaryWeaponFeatureHandler* FindFeature(const GameplayTag& featureTag);

		const GameplayAttribute* FindDefinitionAttribute(
			const PrimaryWeaponDefinition& definition,
			const GameplayTag& attributeId
		)
		{
			return FindGameplayAttribute(definition.attributes, attributeId);
		}

		PrimaryWeaponValidationResult RequireAttribute(
			const PrimaryWeaponDefinition& definition,
			const GameplayTag& attributeId,
			const char* ownerName
		)
		{
			return FindDefinitionAttribute(definition, attributeId)
				? PrimaryWeaponValidationResult{ true, {} }
				: PrimaryWeaponValidationResult{
					false,
					std::string{ ownerName } + " requires attribute '" + attributeId.ToString() + "'."
				};
		}

		void FireProjectile(
			const PrimaryWeaponExecutionContext& context,
			const WeaponMuzzleDefinition& muzzle,
			float localRotationOffset
		)
		{
			if (!context.owner.GetWorld())
			{
				return;
			}

			weak_ptr<Bullet> bullet = context.owner.GetWorld()->SpawnActor<Bullet>(
				&context.owner,
				context.definition.presentationDefinition,
				context.attributes
			);
			if (auto spawnedBullet = bullet.lock())
			{
				spawnedBullet->SetDamageTags(context.damageTags);
				const sf::Vector2f location = context.owner.GetActorLocation() +
					context.owner.TransformLocalToWorld(muzzle.offset);
				spawnedBullet->SetActorLocation(location);
				spawnedBullet->SetActorRotation(
					context.owner.GetActorRotation() + muzzle.rotationOffset + localRotationOffset
				);
			}
		}

		void FireProjectileSet(
			const PrimaryWeaponExecutionContext& context,
			int projectileCount,
			float spreadAngle
		)
		{
			const int count = std::max(1, projectileCount);
			const float angleStep = count > 1 ? spreadAngle / static_cast<float>(count - 1) : 0.f;
			const float startAngle = count > 1 ? -spreadAngle * 0.5f : 0.f;
			const auto fireFromMuzzle = [&](const WeaponMuzzleDefinition& muzzle)
			{
				for (int index = 0; index < count; ++index)
				{
					FireProjectile(context, muzzle, startAngle + angleStep * static_cast<float>(index));
				}
			};

			if (context.definition.muzzleDefinitions.empty())
			{
				fireFromMuzzle(WeaponMuzzleDefinition{});
				return;
			}
			for (const WeaponMuzzleDefinition& muzzle : context.definition.muzzleDefinitions)
			{
				fireFromMuzzle(muzzle);
			}
		}

		class StandardProjectileWeaponHandler final : public PrimaryWeaponHandler
		{
		public:
			const GameplayTag& GetTypeTag() const override { return PrimaryWeaponSchema::Projectile::Standard::TypeId; }
			const List<GameplayTag>& GetOwnedAttributeRoots() const override { return ProjectileDeliveryAttributeRoots; }
			PrimaryWeaponValidationResult ValidateDefinition(const PrimaryWeaponDefinition& definition) const override
			{
				for (const GameplayTag& required : {
					CommonAttributeIds::Damage,
					PrimaryWeaponSchema::Projectile::Delivery::Speed,
					PrimaryWeaponSchema::Projectile::Delivery::Lifetime
				})
				{
					const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Projectile weapon");
					if (!result.isValid)
					{
						return result;
					}
				}
				return { true, {} };
			}
			void FireOnce(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState&) const override
			{
				FireProjectileSet(context, 1, 0.f);
			}
		};

		class ShotgunWeaponHandler final : public PrimaryWeaponHandler
		{
		public:
			const GameplayTag& GetTypeTag() const override { return PrimaryWeaponSchema::Projectile::Shotgun::TypeId; }
			const List<GameplayTag>& GetOwnedAttributeRoots() const override { return ShotgunAttributeRoots; }
			const List<GameplayTag>& GetInheritedAttributeRoots() const override { return ProjectileDeliveryAttributeRoots; }
			PrimaryWeaponValidationResult ValidateDefinition(const PrimaryWeaponDefinition& definition) const override
			{
				for (const GameplayTag& required : {
					CommonAttributeIds::Damage,
					PrimaryWeaponSchema::Projectile::Delivery::Speed,
					PrimaryWeaponSchema::Projectile::Delivery::Lifetime,
					PrimaryWeaponSchema::Projectile::Shotgun::PelletCount,
					PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle
				})
				{
					const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Shotgun weapon");
					if (!result.isValid)
					{
						return result;
					}
				}

				const float pelletCount = FindDefinitionAttribute(
					definition,
					PrimaryWeaponSchema::Projectile::Shotgun::PelletCount
				)->baseValue;
				if (pelletCount < 2.f || std::round(pelletCount) != pelletCount)
				{
					return { false, "Shotgun pellet count must be an integer of at least two." };
				}
				if (FindDefinitionAttribute(definition, PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle)->baseValue < 0.f)
				{
					return { false, "Shotgun spread angle cannot be negative." };
				}
				return { true, {} };
			}
			void FireOnce(const PrimaryWeaponExecutionContext& context, PrimaryWeaponTypeRuntimeState&) const override
			{
				const int pelletCount = std::max(
					1,
					static_cast<int>(std::round(FindGameplayAttributeValue(
						context.attributes,
						PrimaryWeaponSchema::Projectile::Shotgun::PelletCount,
						1.f
					)))
				);
				const float spreadAngle = std::max(0.f, FindGameplayAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Projectile::Shotgun::SpreadAngle,
					0.f
				));
				FireProjectileSet(context, pelletCount, spreadAngle);
			}
		};

		class HeatFeatureHandler final : public PrimaryWeaponFeatureHandler
		{
		public:
			const GameplayTag& GetFeatureTag() const override { return PrimaryWeaponSchema::Feature::Heat::FeatureId; }
			const List<GameplayTag>& GetAttributeRoots() const override { return HeatAttributeRoots; }
			PrimaryWeaponValidationResult ValidateDefinition(const PrimaryWeaponDefinition& definition) const override
			{
				for (const GameplayTag& required : {
					PrimaryWeaponSchema::Feature::Heat::Gain,
					PrimaryWeaponSchema::Feature::Heat::Capacity,
					PrimaryWeaponSchema::Feature::Heat::Dissipation
				})
				{
					const PrimaryWeaponValidationResult result = RequireAttribute(definition, required, "Heat feature");
					if (!result.isValid)
					{
						return result;
					}
				}
				const GameplayAttribute* capacity = FindDefinitionAttribute(
					definition,
					PrimaryWeaponSchema::Feature::Heat::Capacity
				);
				return capacity && capacity->baseValue > 0.f
					? PrimaryWeaponValidationResult{ true, {} }
					: PrimaryWeaponValidationResult{ false, "Heat capacity must be greater than zero." };
			}
			void BeginFire(const PrimaryWeaponExecutionContext&, PrimaryWeaponRuntimeState& state) const override
			{
				state.SetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue, 0.f);
			}
			bool CanFire(const PrimaryWeaponExecutionContext& context, const PrimaryWeaponRuntimeState& state) const override
			{
				const float gain = std::max(0.f, FindGameplayAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Feature::Heat::Gain,
					0.f
				));
				const float capacity = std::max(0.f, FindGameplayAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Feature::Heat::Capacity,
					0.f
				));
				return state.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue) + gain <= capacity;
			}
			void AfterFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state) const override
			{
				const float gain = std::max(0.f, FindGameplayAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Feature::Heat::Gain,
					0.f
				));
				const float capacity = std::max(0.f, FindGameplayAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Feature::Heat::Capacity,
					0.f
				));
				state.SetFeatureValue(
					PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue,
					std::min(capacity, state.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue) + gain)
				);
			}
			void TickFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state, float deltaTime) const override
			{
				const float dissipation = std::max(0.f, FindGameplayAttributeValue(
					context.attributes,
					PrimaryWeaponSchema::Feature::Heat::Dissipation,
					0.f
				));
				state.SetFeatureValue(
					PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue,
					std::max(0.f, state.GetFeatureValue(PrimaryWeaponSchema::Feature::Heat::CurrentRuntimeValue) - dissipation * deltaTime)
				);
			}
		};

		void EnsureBuiltInHandlers()
		{
			static const bool initialized = []
			{
				GetHandlers().emplace(PrimaryWeaponSchema::Projectile::Standard::TypeId, std::make_unique<StandardProjectileWeaponHandler>());
				GetHandlers().emplace(PrimaryWeaponSchema::Projectile::Shotgun::TypeId, std::make_unique<ShotgunWeaponHandler>());
				GetFeatures().emplace(PrimaryWeaponSchema::Feature::Heat::FeatureId, std::make_unique<HeatFeatureHandler>());
				return true;
			}();
			(void)initialized;
		}

		const PrimaryWeaponHandler* FindHandler(const GameplayTag& weaponTypeTag)
		{
			EnsureBuiltInHandlers();
			auto found = GetHandlers().find(weaponTypeTag);
			return found != GetHandlers().end() ? found->second.get() : nullptr;
		}

		const PrimaryWeaponFeatureHandler* FindFeature(const GameplayTag& featureTag)
		{
			EnsureBuiltInHandlers();
			auto found = GetFeatures().find(featureTag);
			return found != GetFeatures().end() ? found->second.get() : nullptr;
		}

		bool IsAllowedAttribute(
			const PrimaryWeaponHandler& handler,
			const List<const PrimaryWeaponFeatureHandler*>& features,
			const GameplayTag& attributeId
		)
		{
			if (IsCommonWeaponAttribute(attributeId) ||
				MatchesAnyRoot(attributeId, handler.GetOwnedAttributeRoots()) ||
				MatchesAnyRoot(attributeId, handler.GetInheritedAttributeRoots()))
			{
				return true;
			}
			return std::any_of(features.begin(), features.end(), [&](const PrimaryWeaponFeatureHandler* feature)
			{
				return feature && MatchesAnyRoot(attributeId, feature->GetAttributeRoots());
			});
		}

		PrimaryWeaponValidationResult ValidateAttributeId(
			const PrimaryWeaponHandler& handler,
			const List<const PrimaryWeaponFeatureHandler*>& features,
			const GameplayTag& attributeId,
			const char* usage
		)
		{
			return IsAllowedAttribute(handler, features, attributeId)
				? PrimaryWeaponValidationResult{ true, {} }
				: PrimaryWeaponValidationResult{
					false,
					std::string{ usage } + " is not consumed by the selected weapon type or feature."
				};
		}
	}

	float PrimaryWeaponRuntimeState::GetFeatureValue(const GameplayTag& key, float fallback) const
	{
		auto found = featureValues.find(key);
		return found != featureValues.end() ? found->second : fallback;
	}

	void PrimaryWeaponRuntimeState::SetFeatureValue(const GameplayTag& key, float value)
	{
		featureValues[key] = value;
	}

	const List<GameplayTag>& PrimaryWeaponHandler::GetInheritedAttributeRoots() const
	{
		return EmptyTags;
	}

	PrimaryWeaponValidationResult PrimaryWeaponHandler::ValidateDefinition(const PrimaryWeaponDefinition&) const
	{
		return { true, {} };
	}

	unique_ptr<PrimaryWeaponTypeRuntimeState> PrimaryWeaponHandler::CreateRuntimeState() const
	{
		return std::make_unique<PrimaryWeaponTypeRuntimeState>();
	}

	void PrimaryWeaponHandler::BeginFire(const PrimaryWeaponExecutionContext&, PrimaryWeaponTypeRuntimeState&) const
	{
	}

	void PrimaryWeaponHandler::TickFire(const PrimaryWeaponExecutionContext&, PrimaryWeaponTypeRuntimeState&, float) const
	{
	}

	void PrimaryWeaponHandler::EndFire(const PrimaryWeaponExecutionContext&, PrimaryWeaponTypeRuntimeState&) const
	{
	}

	PrimaryWeaponValidationResult PrimaryWeaponFeatureHandler::ValidateDefinition(const PrimaryWeaponDefinition&) const
	{
		return { true, {} };
	}

	void PrimaryWeaponFeatureHandler::BeginFire(const PrimaryWeaponExecutionContext&, PrimaryWeaponRuntimeState&) const
	{
	}

	bool PrimaryWeaponFeatureHandler::CanFire(const PrimaryWeaponExecutionContext&, const PrimaryWeaponRuntimeState&) const
	{
		return true;
	}

	void PrimaryWeaponFeatureHandler::AfterFire(const PrimaryWeaponExecutionContext&, PrimaryWeaponRuntimeState&) const
	{
	}

	void PrimaryWeaponFeatureHandler::TickFire(const PrimaryWeaponExecutionContext&, PrimaryWeaponRuntimeState&, float) const
	{
	}

	void PrimaryWeaponFeatureHandler::EndFire(const PrimaryWeaponExecutionContext&, PrimaryWeaponRuntimeState&) const
	{
	}

	bool PrimaryWeaponRegistry::RegisterHandler(unique_ptr<PrimaryWeaponHandler> handler)
	{
		EnsureBuiltInHandlers();
		if (!handler || !handler->GetTypeTag().IsValid())
		{
			return false;
		}
		return GetHandlers().emplace(handler->GetTypeTag(), std::move(handler)).second;
	}

	bool PrimaryWeaponRegistry::RegisterFeature(unique_ptr<PrimaryWeaponFeatureHandler> feature)
	{
		EnsureBuiltInHandlers();
		if (!feature || !feature->GetFeatureTag().IsValid())
		{
			return false;
		}
		return GetFeatures().emplace(feature->GetFeatureTag(), std::move(feature)).second;
	}

	PrimaryWeaponValidationResult PrimaryWeaponRegistry::ValidateDefinition(const PrimaryWeaponDefinition& definition)
	{
		const PrimaryWeaponHandler* handler = FindHandler(definition.weaponTypeTag);
		if (!handler)
		{
			return { false, "No primary weapon handler is registered for this weapon type." };
		}

		List<const PrimaryWeaponFeatureHandler*> features;
		List<GameplayTag> declaredFeatureTags;
		for (const GameplayTag& featureTag : definition.featureTags)
		{
			if (!featureTag.IsValid() || HasExactTag(declaredFeatureTags, featureTag))
			{
				return { false, "Primary weapon feature tags must be valid and unique." };
			}
			const PrimaryWeaponFeatureHandler* feature = FindFeature(featureTag);
			if (!feature)
			{
				return { false, "No primary weapon feature handler is registered for this feature." };
			}
			declaredFeatureTags.push_back(featureTag);
			features.push_back(feature);
		}

		List<GameplayTag> declaredAttributeIds;
		for (const GameplayAttribute& attribute : definition.attributes)
		{
			if (!attribute.id.IsValid() || HasExactTag(declaredAttributeIds, attribute.id))
			{
				return { false, "Primary weapon attributes must have valid, unique IDs." };
			}
			declaredAttributeIds.push_back(attribute.id);
			const PrimaryWeaponValidationResult result = ValidateAttributeId(*handler, features, attribute.id, "Weapon attribute");
			if (!result.isValid)
			{
				return result;
			}
		}

		for (const AttributeModifier& modifier : definition.attributeModifiers)
		{
			const PrimaryWeaponValidationResult result = ValidateAttributeId(*handler, features, modifier.attributeId, "Weapon modifier");
			if (!result.isValid)
			{
				return result;
			}
		}
		for (const PrimaryWeaponLevelStep& step : definition.levelProgression)
		{
			for (const AttributeModifier& modifier : step.modifiers)
			{
				const PrimaryWeaponValidationResult result = ValidateAttributeId(*handler, features, modifier.attributeId, "Weapon level modifier");
				if (!result.isValid)
				{
					return result;
				}
			}
		}
		for (const AttributeScalingRule& scaling : definition.scalingRules)
		{
			const PrimaryWeaponValidationResult result = ValidateAttributeId(*handler, features, scaling.targetAttributeId, "Weapon scaling target");
			if (!result.isValid)
			{
				return result;
			}
		}

		const PrimaryWeaponValidationResult handlerResult = handler->ValidateDefinition(definition);
		if (!handlerResult.isValid)
		{
			return handlerResult;
		}
		for (const PrimaryWeaponFeatureHandler* feature : features)
		{
			const PrimaryWeaponValidationResult featureResult = feature->ValidateDefinition(definition);
			if (!featureResult.isValid)
			{
				return featureResult;
			}
		}
		return { true, {} };
	}

	PrimaryWeaponValidationResult PrimaryWeaponRegistry::InitializeRuntime(
		const PrimaryWeaponDefinition& definition,
		PrimaryWeaponRuntimeState& state
	)
	{
		state.handler = FindHandler(definition.weaponTypeTag);
		if (!state.handler)
		{
			return { false, "No primary weapon handler is registered for this weapon type." };
		}
		state.features.clear();
		state.featureValues.clear();
		for (const GameplayTag& featureTag : definition.featureTags)
		{
			const PrimaryWeaponFeatureHandler* feature = FindFeature(featureTag);
			if (!feature)
			{
				return { false, "No primary weapon feature handler is registered for this feature." };
			}
			state.features.push_back(feature);
		}
		state.typeState = state.handler->CreateRuntimeState();
		state.isInitialized = state.typeState != nullptr;
		state.isFiring = false;
		return state.isInitialized
			? PrimaryWeaponValidationResult{ true, {} }
			: PrimaryWeaponValidationResult{ false, "Primary weapon runtime state could not be created." };
	}

	void PrimaryWeaponRegistry::BeginFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state)
	{
		if (!state.isInitialized || state.isFiring || !state.handler || !state.typeState)
		{
			return;
		}
		state.isFiring = true;
		state.handler->BeginFire(context, *state.typeState);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->BeginFire(context, state);
		}
	}

	bool PrimaryWeaponRegistry::UsesIntervalFire(const PrimaryWeaponRuntimeState& state)
	{
		return state.isInitialized && state.handler && state.handler->UsesIntervalFire();
	}

	bool PrimaryWeaponRegistry::FireOnce(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state)
	{
		if (!state.isInitialized || !state.isFiring || !state.handler || !state.typeState)
		{
			return false;
		}
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			if (!feature->CanFire(context, state))
			{
				return false;
			}
		}

		state.handler->FireOnce(context, *state.typeState);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->AfterFire(context, state);
		}
		return true;
	}

	void PrimaryWeaponRegistry::TickFire(
		const PrimaryWeaponExecutionContext& context,
		PrimaryWeaponRuntimeState& state,
		float deltaTime
	)
	{
		if (!state.isInitialized || !state.isFiring || !state.handler || !state.typeState)
		{
			return;
		}
		state.handler->TickFire(context, *state.typeState, deltaTime);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->TickFire(context, state, deltaTime);
		}
	}

	void PrimaryWeaponRegistry::EndFire(const PrimaryWeaponExecutionContext& context, PrimaryWeaponRuntimeState& state)
	{
		if (!state.isInitialized || !state.isFiring || !state.handler || !state.typeState)
		{
			return;
		}
		state.handler->EndFire(context, *state.typeState);
		for (const PrimaryWeaponFeatureHandler* feature : state.features)
		{
			feature->EndFire(context, state);
		}
		state.isFiring = false;
	}

	float PrimaryWeaponRegistry::BuildBaseFireInterval(const GameplayAttributeList& attributes, float actionInterval)
	{
		if (actionInterval > 0.f)
		{
			return actionInterval;
		}
		const float fireRate = std::max(0.01f, FindGameplayAttributeValue(attributes, CommonAttributeIds::FireRate, 1.f));
		return 1.f / fireRate;
	}
}
