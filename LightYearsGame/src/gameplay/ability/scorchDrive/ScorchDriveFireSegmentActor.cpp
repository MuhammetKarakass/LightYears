#include "gameplay/ability/scorchDrive/ScorchDriveFireSegmentActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/scorchDrive/ScorchDriveContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ly
{
	namespace
	{
		const List<sas::AttributeId> ScorchDriveAttributeRoots{
			AbilityData::ScorchDrive::Actor::FireSegment::Root
		};

		const List<sas::AttributeId> ScorchDriveCommonAttributes{
			AreaAttributeIds::Width,
			AreaAttributeIds::Length
		};

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

		class ScorchDriveFireSegmentActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::ScorchDriveFireSegment;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return ScorchDriveAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return ScorchDriveCommonAttributes;
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
					AreaAttributeIds::Width,
					AreaAttributeIds::Length
				})
				{
					const sas::GameplayAttribute* attribute =
						sas::FindAttribute(definition.attributes, required);
					if (!attribute || attribute->baseValue <= 0.f)
					{
						return {
							false,
							"Scorch Drive fire segment requires positive area geometry."
						};
					}
				}
				if (!definition.presentationProfileId.IsValid() ||
					PresentationProfileRegistry<ScorchDrivePresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) == nullptr)
				{
					return {
						false,
						"Scorch Drive fire segment requires a valid typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const ScorchDrivePresentationProfile* profile =
					PresentationProfileRegistry<ScorchDrivePresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<ScorchDriveFireSegmentActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	ScorchDriveFireSegmentActor::ScorchDriveFireSegmentActor(
		World* world,
		Actor* owner,
		const ScorchDrivePresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
		, mOuter({ 1.f, 1.f })
		, mCore({ 1.f, 1.f })
	{
		SetRenderLayer(RenderLayer::GroundDecal);
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		mOuter.setFillColor(sf::Color::Transparent);
		mCore.setFillColor(sf::Color::Transparent);
	}

	void ScorchDriveFireSegmentActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mWidth = std::max(
			0.1f,
			sas::FindAttributeValue(attributes, AreaAttributeIds::Width, mWidth)
		);
		mLength = std::max(
			0.1f,
			sas::FindAttributeValue(attributes, AreaAttributeIds::Length, mLength)
		);
		mOuter.setSize({ mLength, mWidth });
		mCore.setSize({ mLength * 0.84f, mWidth * 0.42f });
		mOuter.setOrigin({ mLength * 0.5f, mWidth * 0.5f });
		mCore.setOrigin({ mLength * 0.42f, mWidth * 0.21f });
	}

	void ScorchDriveFireSegmentActor::ConfigureSegment(
		const sf::Vector2f& location,
		const sf::Vector2f& direction,
		float lifetime,
		const weak_ptr<ScorchDriveTrailCoordinatorActor>& coordinator
	)
	{
		mDirection = direction;
		if (GetVectorLength(mDirection) <= 0.001f)
		{
			mDirection = { 0.f, -1.f };
		}
		else
		{
			NormalizeVector(mDirection);
		}
		SetActorLocation(location);
		SetActorRotation(
			std::atan2(mDirection.y, mDirection.x) * 57.2957795131f + 90.f
		);
		SetLifeTime(std::max(0.f, lifetime));
		mCoordinator = coordinator;
	}

	void ScorchDriveFireSegmentActor::Tick(float deltaTime)
	{
		AbilityWorldActor::Tick(deltaTime);
		if (GetIsPendingDestroy())
		{
			return;
		}
		mVisualAge += std::max(0.f, deltaTime);
	}

	void ScorchDriveFireSegmentActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		const float lifetime = GetLifeTime();
		const float lifeProgress = lifetime > 0.f
			? std::clamp(GetAge() / lifetime, 0.f, 1.f)
			: 0.f;
		const float fade = std::pow(1.f - lifeProgress, std::max(
			0.1f,
			mPresentationProfile.visual.fadeExponent
		));
		const float pulse = 0.90f + 0.10f * std::sin(
			mVisualAge * mPresentationProfile.visual.pulseSpeed
		);
		const float alphaMultiplier = std::clamp(fade * pulse, 0.f, 1.f);

		mOuter.setPosition(GetActorLocation());
		mCore.setPosition(GetActorLocation());
		mOuter.setRotation(sf::degrees(GetActorRotation()));
		mCore.setRotation(sf::degrees(GetActorRotation()));
		mOuter.setFillColor(WithAlpha(
			mPresentationProfile.visual.outerColor,
			alphaMultiplier
		));
		mOuter.setOutlineColor(WithAlpha(
			mPresentationProfile.visual.edgeColor,
			alphaMultiplier
		));
		mOuter.setOutlineThickness(
			std::max(0.f, mPresentationProfile.visual.edgeThickness)
		);
		mCore.setFillColor(WithAlpha(
			mPresentationProfile.visual.coreColor,
			alphaMultiplier
		));
		window.draw(mOuter);
		window.draw(mCore);
	}

	bool RegisterScorchDriveFireSegmentActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<ScorchDriveFireSegmentActorTypeHandler>()
		);
	}
}
