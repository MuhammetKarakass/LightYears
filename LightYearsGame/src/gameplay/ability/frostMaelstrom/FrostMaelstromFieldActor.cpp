#include "gameplay/ability/frostMaelstrom/FrostMaelstromFieldActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/frostMaelstrom/FrostMaelstromContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "spaceShip/SpaceShip.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace ly
{
	namespace
	{
		// Keep a controllable outer band around the visible field. Without this
		// margin, targets standing on the outer edge can remain outside the query
		// radius and never receive the inward orbital acceleration.
		inline constexpr float ControlInfluenceRadiusMultiplier = 1.25f;

		const List<sas::AttributeId> FieldCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Duration
		};

		sf::Vector2f NormalizeOrDefault(const sf::Vector2f& value)
		{
			const float length = GetVectorLength(value);
			return length > 0.001f
				? value / length
				: sf::Vector2f{ 0.f, -1.f };
		}

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			return sf::Color{
				color.r,
				color.g,
				color.b,
				static_cast<std::uint8_t>(std::clamp(
					static_cast<float>(color.a) * std::max(0.f, multiplier),
					0.f,
					255.f
				))
			};
		}

		class FrostMaelstromFieldActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::FrostMaelstromField;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return FieldCommonAttributes;
			}

			AbilityActorValidationResult ValidateDefinition(
				const AbilityActorDefinition& definition
			) const override
			{
				const AbilityActorValidationResult baseResult =
					AbilityActorTypeHandler::ValidateDefinition(definition);
				if (!baseResult.isValid)
				{
					return baseResult;
				}

				for (const sas::AttributeId& required : {
					CommonAttributeIds::Damage,
					CommonAttributeIds::Duration
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute || !std::isfinite(attribute->baseValue) ||
						attribute->baseValue <= 0.f)
					{
						return {
							false,
							"Frost Maelstrom field requires positive damage and duration."
						};
					}
				}

				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<FrostMaelstromPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return {
						false,
						"Frost Maelstrom field requires a registered typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const FrostMaelstromPresentationProfile* profile =
					PresentationProfileRegistry<FrostMaelstromPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<FrostMaelstromFieldActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	FrostMaelstromFieldActor::FrostMaelstromFieldActor(
		World* world,
		Actor* owner,
		const FrostMaelstromPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
		, mOuterRing(1.f, 96)
		, mInnerRing(1.f, 96)
	{
		SetRenderLayer(RenderLayer::GroundDecal);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		ConfigureGeometry();
	}

	void FrostMaelstromFieldActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		mFieldDirection = NormalizeOrDefault(GetActorForwardDirection());
	}

	void FrostMaelstromFieldActor::ConfigureGeometry()
	{
		mOuterRing.setOrigin({ 1.f, 1.f });
		mInnerRing.setOrigin({ 1.f, 1.f });
		mOuterRing.setFillColor(sf::Color::Transparent);
		mInnerRing.setFillColor(sf::Color::Transparent);
	}

	void FrostMaelstromFieldActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mDuration = std::max(0.01f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Duration,
			mDuration
		));
		mTickDamage = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Damage,
			mTickDamage
		));
		SetLifeTime(mDuration);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void FrostMaelstromFieldActor::ConfigureFromAbilityValues(
		const sas::GameplayAttributeList& values
	)
	{
		const auto find = [&](const sas::AttributeId& id, float fallback)
		{
			return sas::FindAttributeValue(values, id, fallback);
		};

		mMinimumRadius = std::max(1.f, find(
			AbilityData::FrostMaelstrom::Attribute::MinimumRadius,
			AbilityData::FrostMaelstrom::DefaultMinimumRadius
		));
		mMaximumRadius = std::max(mMinimumRadius, find(
			AbilityData::FrostMaelstrom::Attribute::MaximumRadius,
			AbilityData::FrostMaelstrom::DefaultMaximumRadius
		));
		mMinimumSpeed = std::max(0.f, find(
			AbilityData::FrostMaelstrom::Attribute::MinimumMovementSpeed,
			AbilityData::FrostMaelstrom::DefaultMinimumMovementSpeed
		));
		mMaximumSpeed = std::max(mMinimumSpeed, find(
			AbilityData::FrostMaelstrom::Attribute::MaximumMovementSpeed,
			AbilityData::FrostMaelstrom::DefaultMaximumMovementSpeed
		));
		mTickInterval = std::max(0.001f, find(
			AbilityData::FrostMaelstrom::Attribute::TickInterval,
			AbilityData::FrostMaelstrom::DefaultTickInterval
		));
		mCryoStacksPerTick = std::clamp(find(
			AbilityData::FrostMaelstrom::Attribute::CryoStacksPerTick,
			AbilityData::FrostMaelstrom::DefaultCryoStacksPerTick
		), 1.f, 4.f);
		mOrbitalAngularSpeed = find(
			AbilityData::FrostMaelstrom::Attribute::OrbitalAngularSpeed,
			AbilityData::FrostMaelstrom::DefaultOrbitalAngularSpeed
		);
		mInwardForce = std::max(0.f, find(
			AbilityData::FrostMaelstrom::Attribute::InwardForce,
			AbilityData::FrostMaelstrom::DefaultInwardForce
		));
		mOrbitalRadiusRatio = std::clamp(find(
			AbilityData::FrostMaelstrom::Attribute::OrbitalRadiusRatio,
			AbilityData::FrostMaelstrom::DefaultOrbitalRadiusRatio
		), 0.1f, 1.f);
		mEnergyMaxReference = std::max(0.f, find(
			AbilityData::FrostMaelstrom::Attribute::EnergyMaxReference,
			AbilityData::FrostMaelstrom::DefaultEnergyMaxReference
		));
		mEnergyMaxDamageScale = std::max(0.f, find(
			AbilityData::FrostMaelstrom::Attribute::EnergyMaxDamageScale,
			AbilityData::FrostMaelstrom::DefaultEnergyMaxDamageScale
		));
		mEnergyMaxRadiusScale = std::max(0.f, find(
			AbilityData::FrostMaelstrom::Attribute::EnergyMaxRadiusScale,
			AbilityData::FrostMaelstrom::DefaultEnergyMaxRadiusScale
		));

		mTickDamage = std::max(0.f, find(
			AbilityData::FrostMaelstrom::Attribute::TickDamage,
			mTickDamage
		));
		float energyMax = 0.f;
		if (const Combatant* combatant = dynamic_cast<const Combatant*>(GetOwnerActor()))
		{
			energyMax = combatant->GetAbilitySystemComponent().GetAttributes()
				.GetCurrentValue(OwnerAttributeIds::EnergyMax);
		}
		const float energyBonus = std::max(0.f, energyMax - mEnergyMaxReference);
		const float radiusBonus = energyBonus * mEnergyMaxRadiusScale;
		mMinimumRadius = std::min(
			AbilityData::FrostMaelstrom::DefaultMaximumRadius,
			mMinimumRadius + radiusBonus
		);
		mMaximumRadius = std::min(
			AbilityData::FrostMaelstrom::DefaultMaximumRadius,
			mMaximumRadius + radiusBonus
		);
		mTickDamage += energyBonus * mEnergyMaxDamageScale;
		mCurrentRadius = mMinimumRadius;
		mCurrentSpeed = mMinimumSpeed;
		mFieldAge = 0.f;
		mTickAccumulator = 0.f;
		mVisualAge = 0.f;
	}

	void FrostMaelstromFieldActor::AdvanceField(float deltaTime)
	{
		const float progress = std::clamp(
			mFieldAge / std::max(0.01f, mDuration),
			0.f,
			1.f
		);
		mCurrentRadius = mMinimumRadius +
			(mMaximumRadius - mMinimumRadius) * progress;
		mCurrentSpeed = mMinimumSpeed +
			(mMaximumSpeed - mMinimumSpeed) * progress;
		AddActorLocationOffset(mFieldDirection * mCurrentSpeed * deltaTime);
	}

	bool FrostMaelstromFieldActor::ContainsTarget(const SpaceShip* target) const
	{
		return std::any_of(
			mControlledTargets.begin(),
			mControlledTargets.end(),
			[target](const ControlledTarget& controlled)
			{
				const shared_ptr<SpaceShip> current = controlled.target.lock();
				return current && current.get() == target;
			}
		);
	}

	void FrostMaelstromFieldActor::AddTarget(
		const shared_ptr<SpaceShip>& target
	)
	{
		if (!target || ContainsTarget(target.get()))
		{
			return;
		}
		target->GetAbilitySystemComponent().AddOwnedTag(
			AbilityData::FrostMaelstrom::State::Controlling
		);
		mControlledTargets.push_back(ControlledTarget{ target });
	}

	bool FrostMaelstromFieldActor::IsEligibleTarget(const Actor& target) const
	{
		const Combatant* combatant = dynamic_cast<const Combatant*>(&target);
		return combatant && !target.GetIsPendingDestroy() &&
			target.GetCollisionLayer() != CollisionLayer::None;
	}

	void FrostMaelstromFieldActor::RemoveControl(
		ControlledTarget& controlled
	)
	{
		if (const shared_ptr<SpaceShip> target = controlled.target.lock())
		{
			target->GetAbilitySystemComponent().RemoveOwnedTag(
				AbilityData::FrostMaelstrom::State::Controlling
			);
		}
	}

	void FrostMaelstromFieldActor::UpdateControlledTargets()
	{
		World* world = GetWorld();
		const Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return;
		}

		const float controlInfluenceRadius =
			mCurrentRadius * ControlInfluenceRadiusMultiplier;
		for (const shared_ptr<Actor>& candidate : targeting::FindOpposingCombatants(
			*world,
			*owner,
			GetActorLocation(),
			controlInfluenceRadius
		))
		{
			const shared_ptr<SpaceShip> ship = std::dynamic_pointer_cast<SpaceShip>(candidate);
			if (ship && IsEligibleTarget(*ship))
			{
				AddTarget(ship);
			}
		}

		for (auto controlled = mControlledTargets.begin();
			controlled != mControlledTargets.end();)
		{
			const shared_ptr<SpaceShip> target = controlled->target.lock();
			const float distance = target
				? GetVectorLength(target->GetActorLocation() - GetActorLocation())
				: 0.f;
			if (!target || target->GetIsPendingDestroy() ||
				target->GetHealthComponent().GetHealth() <= 0.f ||
				distance > controlInfluenceRadius)
			{
				RemoveControl(*controlled);
				controlled = mControlledTargets.erase(controlled);
				continue;
			}
			++controlled;
		}
	}

	sf::Vector2f FrostMaelstromFieldActor::ResolveControlAcceleration(
		const ControlledTarget& controlled
	) const
	{
		const shared_ptr<SpaceShip> target = controlled.target.lock();
		if (!target)
		{
			return {};
		}
		const sf::Vector2f delta = target->GetActorLocation() - GetActorLocation();
		const float distance = GetVectorLength(delta);
		const sf::Vector2f radial = distance > 0.001f
			? delta / distance
			: sf::Vector2f{ 0.f, 0.f };
		const sf::Vector2f tangent{
			-radial.y,
			radial.x
		};
		const float desiredRadius = std::clamp(
			mCurrentRadius * mOrbitalRadiusRatio,
			mCurrentRadius * 0.20f,
			mCurrentRadius
		);
		const float radiusError = std::clamp(
			(distance - desiredRadius) / std::max(1.f, desiredRadius),
			-1.f,
			1.f
		);
		const sf::Vector2f radialAcceleration = radial * (-mInwardForce * radiusError);
		const sf::Vector2f desiredVelocity =
			mFieldDirection * mCurrentSpeed +
			tangent * (mOrbitalAngularSpeed * desiredRadius);
		// Steer velocity through the shared movement component instead of setting
		// a forced position. Player thrust remains additive and, after the field
		// ends, its normal damping owns the resulting drift.
		sf::Vector2f velocityCorrection =
			(desiredVelocity - target->GetVelocity()) *
			ControlVelocityResponsePerSecond;

		// While a target is outside the desired orbit, the field's forward
		// movement can produce an outward radial component in the velocity
		// correction. That component used to cancel the inward force and could
		// throw targets away from the vortex. Keep tangential steering intact,
		// but never allow the outer-orbit correction to accelerate outward.
		if (radiusError > 0.f)
		{
			const float radialCorrection =
				velocityCorrection.x * radial.x +
				velocityCorrection.y * radial.y;
			if (radialCorrection > 0.f)
			{
				velocityCorrection -= radial * radialCorrection;
			}
		}

		return radialAcceleration + velocityCorrection;
	}

	void FrostMaelstromFieldActor::ApplyControlForces(float deltaTime)
	{
		for (ControlledTarget& controlled : mControlledTargets)
		{
			if (const shared_ptr<SpaceShip> target = controlled.target.lock())
			{
				target->GetMovementComponent().AddWorldAcceleration(
					ResolveControlAcceleration(controlled),
					std::max(0.f, deltaTime)
				);
			}
		}
	}

	void FrostMaelstromFieldActor::ApplyCryoTick()
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner || mTickDamage <= 0.f)
		{
			return;
		}

		DamagePayload payload = DamageTypeSystem::BuildPayload(
			GetDamageTags(),
			GetDamageAttributes()
		);
		payload.cryoBuildupPerHit = std::clamp(
			static_cast<int>(std::lround(mCryoStacksPerTick)),
			1,
			4
		);
		payload.cryoBuildupRequired = 4;
		payload.cryoBuildupDuration = 2.5f;
		payload.cryoSlowPercent = 0.25f;
		payload.cryoSlowDuration = 1.5f;
		for (const shared_ptr<Actor>& target : targeting::FindOpposingCombatants(
			*world,
			*owner,
			GetActorLocation(),
			mCurrentRadius
		))
		{
			if (!target || target->GetIsPendingDestroy() ||
				GetVectorLength(target->GetActorLocation() - GetActorLocation()) >
					mCurrentRadius)
			{
				continue;
			}
			ApplyCombatDamage(
				*target,
				mTickDamage,
				owner,
				GetDamageTags(),
				payload,
				GetSourceAbilityId(),
				GetSourceAbilityTags(),
				DamageDeliveryType::Area,
				this
			);
		}
	}

	void FrostMaelstromFieldActor::StopControl()
	{
		for (ControlledTarget& controlled : mControlledTargets)
		{
			RemoveControl(controlled);
		}
		mControlledTargets.clear();
	}

	void FrostMaelstromFieldActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		const float safeDeltaTime = std::max(0.f, deltaTime);
		mVisualAge += safeDeltaTime;
		mFieldAge += safeDeltaTime;
		AdvanceField(safeDeltaTime);
		UpdateControlledTargets();
		ApplyControlForces(safeDeltaTime);
		mTickAccumulator += safeDeltaTime;
		while (mTickAccumulator >= mTickInterval && mFieldAge <= mDuration + 0.001f)
		{
			mTickAccumulator -= mTickInterval;
			ApplyCryoTick();
		}
		AbilityWorldActor::Tick(safeDeltaTime);
	}

	void FrostMaelstromFieldActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		const float pulse = 0.88f + 0.12f * std::sin(
			mVisualAge * mPresentationProfile.pulseSpeed
		);
		const sf::Vector2f center = GetActorLocation();
		mOuterRing.setRadius(mCurrentRadius);
		mOuterRing.setOrigin({ mCurrentRadius, mCurrentRadius });
		mOuterRing.setPosition(center);
		mOuterRing.setOutlineThickness(mPresentationProfile.ringThickness);
		mOuterRing.setOutlineColor(WithAlpha(
			mPresentationProfile.outerColor,
			pulse
		));
		mInnerRing.setRadius(mCurrentRadius * 0.82f);
		mInnerRing.setOrigin({ mCurrentRadius * 0.82f, mCurrentRadius * 0.82f });
		mInnerRing.setPosition(center);
		mInnerRing.setOutlineThickness(mPresentationProfile.ringThickness * 0.65f);
		mInnerRing.setOutlineColor(WithAlpha(
			mPresentationProfile.innerColor,
			pulse
		));
		window.draw(mOuterRing);
		window.draw(mInnerRing);

		const int particleCount = std::max(0, mPresentationProfile.particleCount);
		for (int index = 0; index < particleCount; ++index)
		{
			const float normalized = static_cast<float>(index) /
				static_cast<float>(std::max(1, particleCount));
			const float angle = normalized * 6.28318530718f +
				mVisualAge * mPresentationProfile.visualRotationSpeed;
			const float radius = mCurrentRadius *
				(0.25f + 0.65f * std::fmod(normalized * 3.7f, 1.f));
			sf::CircleShape particle(mPresentationProfile.particleRadius, 8);
			particle.setOrigin({
				mPresentationProfile.particleRadius,
				mPresentationProfile.particleRadius
			});
			particle.setPosition(center + sf::Vector2f{
				std::cos(angle) * radius,
				std::sin(angle) * radius
			});
			particle.setFillColor(WithAlpha(
				mPresentationProfile.particleColor,
				pulse * (0.55f + 0.45f * std::sin(angle * 2.f + mVisualAge))
			));
			window.draw(particle);
		}
	}

	void FrostMaelstromFieldActor::Destroy()
	{
		StopControl();
		AbilityWorldActor::Destroy();
	}

	bool RegisterFrostMaelstromFieldActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<FrostMaelstromFieldActorTypeHandler>()
		);
		return registered;
	}
}
