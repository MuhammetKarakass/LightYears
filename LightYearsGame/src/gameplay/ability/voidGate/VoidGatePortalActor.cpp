#include "gameplay/ability/voidGate/VoidGatePortalActor.h"

#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/voidGate/VoidGateContracts.h"
#include "presentation/ability/PresentationProfileRegistry.h"
#include "framework/World.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> VoidGateCommonAttributes{
			CommonAttributeIds::Radius
		};

		sf::Color WithAlpha(const sf::Color& color, float alphaScale)
		{
			const float clampedAlpha = std::clamp(alphaScale, 0.f, 1.f);
			return sf::Color{
				color.r,
				color.g,
				color.b,
				static_cast<std::uint8_t>(
					static_cast<float>(color.a) * clampedAlpha
				)
			};
		}

		class VoidGatePortalActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::VoidGatePortal;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return VoidGateCommonAttributes;
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

				const sas::GameplayAttribute* radius = sas::FindAttribute(
					definition.attributes,
					CommonAttributeIds::Radius
				);
				if (!radius || radius->baseValue <= 0.f ||
					!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<VoidGatePresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return {
						false,
						"Void Gate portal requires a positive radius and typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const VoidGatePresentationProfile* profile =
					PresentationProfileRegistry<VoidGatePresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<VoidGatePortalActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	VoidGatePortalActor::VoidGatePortalActor(
		World* world,
		Actor* owner,
		const VoidGatePresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile(presentationProfile),
		mOuterRing(mPortalRadius, 64),
		mInnerRing(mPortalRadius * 0.88f, 64),
		mGlow(mPortalRadius * 0.96f, 64)
	{
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		SetRenderLayer(RenderLayer::GroundDecal);
		ConfigureGeometry();
	}

	void VoidGatePortalActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mPortalRadius = std::max(
			1.f,
			sas::FindAttributeValue(
				attributes,
				CommonAttributeIds::Radius,
				AbilityData::VoidGate::DefaultPortalRadius
			)
		);
		ConfigureGeometry();
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void VoidGatePortalActor::ConfigureGeometry()
	{
		mOuterRing.setRadius(mPortalRadius);
		mOuterRing.setOrigin({ mPortalRadius, mPortalRadius });
		mInnerRing.setRadius(mPortalRadius * 0.88f);
		mInnerRing.setOrigin({ mPortalRadius * 0.88f, mPortalRadius * 0.88f });
		mGlow.setRadius(mPortalRadius * 0.96f);
		mGlow.setOrigin({ mPortalRadius * 0.96f, mPortalRadius * 0.96f });
	}

	void VoidGatePortalActor::Tick(float deltaTime)
	{
		mVisualAge += std::max(0.f, deltaTime);
		AbilityWorldActor::Tick(deltaTime);
	}

	void VoidGatePortalActor::SetActivationAlpha(float alpha)
	{
		mActivationAlpha = std::clamp(alpha, 0.f, 1.f);
	}

	void VoidGatePortalActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		const VoidGatePortalVisualDefinition& visual =
			mPresentationProfile.portal;
		const float pulse = 1.f + visual.pulseAmount *
			std::sin(mVisualAge * visual.pulseSpeed);
		const sf::Vector2f location = GetActorLocation();
		mGlow.setPosition(location);
		mGlow.setFillColor(WithAlpha(visual.glowColor, mActivationAlpha));
		mGlow.setScale({ pulse, pulse });
		mOuterRing.setPosition(location);
		mOuterRing.setFillColor(sf::Color::Transparent);
		mOuterRing.setOutlineColor(WithAlpha(visual.outerColor, mActivationAlpha));
		mOuterRing.setOutlineThickness(visual.outerThickness);
		mInnerRing.setPosition(location);
		mInnerRing.setFillColor(WithAlpha(visual.innerColor, mActivationAlpha));
		mInnerRing.setOutlineColor(WithAlpha(visual.glowColor, mActivationAlpha));
		mInnerRing.setOutlineThickness(visual.innerThickness);

		sf::RenderStates additive;
		additive.blendMode = sf::BlendAdd;
		window.draw(mGlow, additive);
		window.draw(mInnerRing, additive);
		window.draw(mOuterRing, additive);
	}

	bool RegisterVoidGatePortalActorType()
	{
		static const bool registered = AbilityActorRegistry::RegisterHandler(
			std::make_unique<VoidGatePortalActorTypeHandler>()
		);
		return registered;
	}
}
