#include "gameplay/ability/reclaimerProtocol/ReclaimerRepairKitActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/reclaimerProtocol/ReclaimerProtocolContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/HealthComponent.h"
#include "player/PlayerSpaceShip.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> RepairKitCommonAttributes{
			CommonAttributeIds::Duration,
			CollisionAttributeIds::Radius
		};

		const List<sas::AttributeId> RepairKitAttributeRoots{
			AbilityData::ReclaimerProtocol::Actor::RepairKit::Root
		};

		class ReclaimerRepairKitActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::ReclaimerRepairKit;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return RepairKitAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return RepairKitCommonAttributes;
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
					CommonAttributeIds::Duration,
					CollisionAttributeIds::Radius,
					AbilityData::ReclaimerProtocol::Actor::RepairKit::HealRatio
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
							"Reclaimer Repair Kit requires positive duration, collision radius, and HealRatio attributes."
						};
					}
				}

				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<ReclaimerProtocolPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return {
						false,
						"Reclaimer Repair Kit requires a registered typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const ReclaimerProtocolPresentationProfile* profile =
					PresentationProfileRegistry<ReclaimerProtocolPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<ReclaimerRepairKitActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	ReclaimerRepairKitActor::ReclaimerRepairKitActor(
		World* world,
		Actor* owner,
		const ReclaimerProtocolPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
	{
		SetRenderLayer(RenderLayer::WorldVfx);
		SetEnablePhysics(true);
		SetCollisionLayer(CollisionLayer::Powerup);
		SetCollisionMask(CollisionLayer::Player);
	}

	void ReclaimerRepairKitActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		SetEnablePhysics(true);
		SetCollisionLayer(CollisionLayer::Powerup);
		SetCollisionMask(CollisionLayer::Player);
		SetVelocity({});
	}

	void ReclaimerRepairKitActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mResolvedHealRatio = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::ReclaimerProtocol::Actor::RepairKit::HealRatio,
			mResolvedHealRatio
		));
		SetVelocity({});
		SetCollisionLayer(CollisionLayer::Powerup);
		SetCollisionMask(CollisionLayer::Player);
	}

	void ReclaimerRepairKitActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		if (mIsCollected)
		{
			mFlashTimeRemaining -= std::max(0.f, deltaTime);
			if (mFlashTimeRemaining <= 0.f)
			{
				Destroy();
			}
			return;
		}

		AbilityWorldActor::Tick(deltaTime);
	}

	void ReclaimerRepairKitActor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (GetIsPendingDestroy() || mIsCollected || !otherActor || otherActor->GetIsPendingDestroy())
		{
			return;
		}

		auto* playerShip = dynamic_cast<PlayerSpaceShip*>(otherActor);
		if (!playerShip)
		{
			return;
		}

		HealthComponent& healthComponent = playerShip->GetHealthComponent();

		const float maxHealth = healthComponent.GetMaxHealth();
		const float currentHealth = healthComponent.GetHealth();
		const float missing = maxHealth - currentHealth;

		if (missing <= 0.f)
		{
			// Player is at full health: retain kit in world
			return;
		}

		const float healAmount = std::min(maxHealth * mResolvedHealRatio, missing);
		if (healAmount <= 0.f)
		{
			return;
		}

		healthComponent.ChangeHealth(healAmount);

		mIsCollected = true;
		mFlashTimeRemaining = mPresentationProfile.flashDuration;
		SetEnablePhysics(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void ReclaimerRepairKitActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		const sf::Vector2f position = GetActorLocation();

		if (mIsCollected)
		{
			// Collection visual flash
			const float radius = mPresentationProfile.capsuleHeight * 0.8f;
			sf::CircleShape flash(radius);
			flash.setOrigin({ radius, radius });
			flash.setPosition(position);
			flash.setFillColor(mPresentationProfile.collectedFlashColor);
			window.draw(flash);
			return;
		}

		const float lifetimeRemaining = std::max(0.f, GetLifeTime() - GetAge());
		const float currentPulseSpeed = (lifetimeRemaining <= 2.f)
			? mPresentationProfile.fastPulseSpeed
			: mPresentationProfile.pulseSpeed;

		const float pulse = 1.f + 0.15f * std::sin(GetAge() * currentPulseSpeed);

		const float width = mPresentationProfile.capsuleWidth * pulse;
		const float height = mPresentationProfile.capsuleHeight * pulse;

		// Draw nanite / medical capsule rendering
		sf::RectangleShape body({ width, height - width });
		body.setOrigin({ width * 0.5f, (height - width) * 0.5f });
		body.setPosition(position);
		body.setFillColor(mPresentationProfile.capsuleColor);

		const float capRadius = width * 0.5f;
		sf::CircleShape topCap(capRadius);
		topCap.setOrigin({ capRadius, capRadius });
		topCap.setPosition({ position.x, position.y - (height - width) * 0.5f });
		topCap.setFillColor(mPresentationProfile.capsuleColor);

		sf::CircleShape bottomCap(capRadius);
		bottomCap.setOrigin({ capRadius, capRadius });
		bottomCap.setPosition({ position.x, position.y + (height - width) * 0.5f });
		bottomCap.setFillColor(mPresentationProfile.capsuleColor);

		// Core cross or nanite inner highlight
		sf::CircleShape innerGlow(capRadius * 0.6f);
		innerGlow.setOrigin({ capRadius * 0.6f, capRadius * 0.6f });
		innerGlow.setPosition(position);
		innerGlow.setFillColor(mPresentationProfile.innerColor);

		window.draw(body);
		window.draw(topCap);
		window.draw(bottomCap);
		window.draw(innerGlow);
	}

	bool RegisterReclaimerRepairKitActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<ReclaimerRepairKitActorTypeHandler>()
		);
	}
}