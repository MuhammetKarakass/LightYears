#include "gameplay/ability/infernoSpray/InfernoSprayActor.h"

#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameConfigs/ability/offensive/InfernoSprayConfig.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/damage/DamageTypeSystem.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		const List<GameplayTag> InfernoSprayCommonAttributes{
			AbilityData::InfernoSpray::ActorSchema::Range,
			AbilityData::InfernoSpray::ActorSchema::ConeAngle,
			AbilityData::InfernoSpray::ActorSchema::CombatTickInterval,
			AbilityData::InfernoSpray::ActorSchema::BaseDPS
		};

		const List<GameplayTag> InfernoSprayAttributeRoots{
			AbilityData::InfernoSpray::ActorSchema::AttributeRoot,
			DamageAttributeIds::AttributeRoot
		};

		class InfernoSprayActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			const GameplayTag& GetActorTypeTag() const override
			{
				return AbilityData::InfernoSpray::ActorSchema::TypeId;
			}

			const List<GameplayTag>& GetOwnedAttributeRoots() const override
			{
				return InfernoSprayAttributeRoots;
			}

			const List<GameplayTag>& GetAllowedCommonAttributeIds() const override
			{
				return InfernoSprayCommonAttributes;
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
				if (definition.presentationProfileId.empty() ||
					PresentationProfileRegistry<InfernoSprayPresentationProfile>::Find(
						definition.presentationProfileId
					) == nullptr)
				{
					return { false, "Inferno Spray actor requires a valid typed presentation profile." };
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const InfernoSprayPresentationProfile* profile =
					PresentationProfileRegistry<InfernoSprayPresentationProfile>::Find(
						context.definition.presentationProfileId
					);
				return world && profile
					? world->SpawnActor<InfernoSprayActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	InfernoSprayActor::InfernoSprayActor(
		World* world,
		Actor* owner,
		const InfernoSprayPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
	{
		SetRenderLayer(RenderLayer::Projectile);
		SetDamageTags({ DamageTypeSchema::Thermal });
		ConfigureCollisionFromOwner();
	}

	void InfernoSprayActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		SetActorLocation(GetMuzzleLocation());
	}

	void InfernoSprayActor::ConfigureFromAttributes(const sas::GameplayAttributeList& attributes)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mRange = std::max(10.f, sas::FindGameplayAttributeValue(
			attributes,
			AbilityData::InfernoSpray::ActorSchema::Range,
			0.f
		));
		mConeAngleDegrees = std::clamp(sas::FindGameplayAttributeValue(
			attributes,
			AbilityData::InfernoSpray::ActorSchema::ConeAngle,
			0.f
		), 1.f, 180.f);
		mCombatTickInterval = std::max(0.05f, sas::FindGameplayAttributeValue(
			attributes,
			AbilityData::InfernoSpray::ActorSchema::CombatTickInterval,
			0.f
		));
		mBaseDPS = std::max(0.f, sas::FindGameplayAttributeValue(
			attributes,
			AbilityData::InfernoSpray::ActorSchema::BaseDPS,
			0.f
		));
	}

	void InfernoSprayActor::Tick(float deltaTime)
	{
		AbilityWorldActor::Tick(deltaTime);

		Actor* owner = GetOwnerActor();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}

		if (auto* combatant = dynamic_cast<Combatant*>(owner))
		{
			if (!combatant->GetCombatRuntime().GetAbilitySystemComponent().GetOwnedTags().HasTag(AbilityData::InfernoSpray::StateTag))
			{
				Destroy();
				return;
			}
		}

		SetActorLocation(GetMuzzleLocation());

		mVisualTime += deltaTime;
		mCombatTickTimer += deltaTime;
		while (mCombatTickTimer >= mCombatTickInterval && mCombatTickInterval > 0.f)
		{
			mCombatTickTimer -= mCombatTickInterval;
			PerformCombatTick();
		}
	}

	void InfernoSprayActor::Destroy()
	{
		AbilityWorldActor::Destroy();
	}

	sf::Vector2f InfernoSprayActor::GetMuzzleLocation() const
	{
		if (Actor* owner = GetOwnerActor())
		{
			return owner->GetActorLocation() + owner->GetActorForwardDirection() * 15.f;
		}
		return GetActorLocation();
	}

	sf::Vector2f InfernoSprayActor::GetAimDirection() const
	{
		if (Actor* owner = GetOwnerActor())
		{
			return owner->GetActorForwardDirection();
		}
		return GetActorForwardDirection();
	}

	void InfernoSprayActor::PerformCombatTick()
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner)
		{
			return;
		}

		const sf::Vector2f muzzlePos = GetMuzzleLocation();
		const sf::Vector2f aimDir = GetAimDirection();
		const float halfAngleRad = (mConeAngleDegrees * 0.5f) * (3.14159265f / 180.f);
		const float cosHalfAngle = std::cos(halfAngleRad);

		float attackPower = 0.f;
		if (auto* combatantOwner = dynamic_cast<Combatant*>(owner))
		{
			const sas::AttributeSystem& attributes = combatantOwner->GetCombatRuntime().GetAbilitySystemComponent().GetAttributes();
			if (attributes.HasAttribute(OwnerAttributeIds::AttackPower))
			{
				attackPower = std::max(0.f, attributes.GetCurrentValue(OwnerAttributeIds::AttackPower));
			}
		}

		const float resolvedDPS = mBaseDPS + attackPower * 0.75f;
		const float damagePerTick = resolvedDPS * mCombatTickInterval;

		for (const weak_ptr<Actor>& actorWeak : world->GetActorsByType<Actor>())
		{
			shared_ptr<Actor> target = actorWeak.lock();
			if (!target || !IsValidAbilityTarget(target.get()))
			{
				continue;
			}

			const sf::Vector2f toTarget = target->GetActorLocation() - muzzlePos;
			const float dist = GetVectorLength(toTarget);
			if (dist > mRange)
			{
				continue;
			}

			const sf::Vector2f dirToTarget = (dist > 0.001f) ? (toTarget / dist) : aimDir;
			const float dot = aimDir.x * dirToTarget.x + aimDir.y * dirToTarget.y;

			if (dot >= cosHalfAngle)
			{
				if (auto* combatantTarget = dynamic_cast<Combatant*>(target.get()))
				{
					DamageContext context;
					context.source = owner;
					context.target = target.get();
					context.originalDamage = damagePerTick;
					context.remainingDamage = damagePerTick;
					context.damageTags = GetDamageTags().empty()
						? List<GameplayTag>{ GameplayTag{ "Damage.Thermal" } }
						: GetDamageTags();
					context.payload = GetDamagePayload();

					combatantTarget->ReceiveDamage(context);
				}
			}
		}
	}

	void InfernoSprayActor::Render(sf::RenderWindow& window)
	{
		AbilityWorldActor::Render(window);

		const sf::Vector2f muzzlePos = GetMuzzleLocation();
		const sf::Vector2f aimDir = GetAimDirection();
		const float aimAngleRad = std::atan2(aimDir.y, aimDir.x);
		const float halfAngleRad = (mConeAngleDegrees * 0.5f) * (3.14159265f / 180.f);

		const int numSegments = 24;
		sf::VertexArray outerFan(sf::PrimitiveType::TriangleFan, numSegments + 2);
		sf::VertexArray coreFan(sf::PrimitiveType::TriangleFan, numSegments + 2);

		// Boundary arc/edge line for clear range indicator
		sf::VertexArray boundaryOutline(sf::PrimitiveType::LineStrip, numSegments + 3);

		const auto& visualDef = mPresentationProfile.visual;
		// Micro-alpha pulse (subtle color pulse, 100% stable size)
		const float alphaPulse = 0.96f + 0.04f * std::sin(mVisualTime * visualDef.pulseSpeed);
		const float currentRange = mRange;

		outerFan[0].position = muzzlePos;
		sf::Color centerOuterColor = visualDef.outerFlameColor;
		centerOuterColor.a = static_cast<std::uint8_t>(centerOuterColor.a * alphaPulse);
		outerFan[0].color = centerOuterColor;

		coreFan[0].position = muzzlePos;
		sf::Color centerCoreColor = visualDef.coreFlameColor;
		centerCoreColor.a = static_cast<std::uint8_t>(centerCoreColor.a * alphaPulse);
		coreFan[0].color = centerCoreColor;

		boundaryOutline[0].position = muzzlePos;
		sf::Color boundaryColor = sf::Color{ 255, 150, 40, static_cast<std::uint8_t>(220 * alphaPulse) };
		boundaryOutline[0].color = boundaryColor;

		for (int i = 0; i <= numSegments; ++i)
		{
			const float alpha = static_cast<float>(i) / static_cast<float>(numSegments);
			const float currentAngle = aimAngleRad - halfAngleRad + (alpha * mConeAngleDegrees * (3.14159265f / 180.f));
			const sf::Vector2f segDir{ std::cos(currentAngle), std::sin(currentAngle) };
			const sf::Vector2f edgePos = muzzlePos + segDir * currentRange;

			sf::Color segOuterColor = visualDef.outerFlameColor;
			segOuterColor.a = static_cast<std::uint8_t>(segOuterColor.a * 0.5f * alphaPulse);
			outerFan[i + 1].position = edgePos;
			outerFan[i + 1].color = segOuterColor;

			sf::Color segCoreColor = visualDef.coreFlameColor;
			segCoreColor.a = static_cast<std::uint8_t>(segCoreColor.a * 0.7f * alphaPulse);
			coreFan[i + 1].position = muzzlePos + segDir * (currentRange * visualDef.coreWidthRatio);
			coreFan[i + 1].color = segCoreColor;

			boundaryOutline[i + 1].position = edgePos;
			boundaryOutline[i + 1].color = boundaryColor;
		}

		boundaryOutline[numSegments + 2].position = muzzlePos;
		boundaryOutline[numSegments + 2].color = boundaryColor;

		window.draw(outerFan);
		window.draw(coreFan);
		window.draw(boundaryOutline);
	}

	bool RegisterInfernoSprayActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<InfernoSprayActorTypeHandler>()
		);
		return registered;
	}
}
