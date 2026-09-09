#include "gameplay/ability/strikeRun/StrikeRunBombardmentActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/strikeRun/StrikeRunContracts.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/targeting/CombatantTargetQuery.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace ly
{
	namespace
	{
		constexpr float DegreesPerRadian = 57.2957795131f;
		constexpr float DirectionEpsilon = 0.001f;
		constexpr int ShippedImpactCount = 5;

		const List<sas::AttributeId> BombardmentCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Radius,
			CommonAttributeIds::Range
		};

		const List<sas::AttributeId> BombardmentAttributeRoots{
			AbilityData::StrikeRun::Actor::Bombardment::Root
		};

		sf::Vector2f NormalizeOrDefault(
			const sf::Vector2f& value,
			const sf::Vector2f& fallback
		)
		{
			const float length = GetVectorLength(value);
			return std::isfinite(length) && length > DirectionEpsilon
				? value / length
				: fallback;
		}

		sf::Color WithAlpha(const sf::Color& color, float multiplier)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(color.a) * std::clamp(multiplier, 0.f, 1.f),
				0.f,
				255.f
			));
			return result;
		}

		float DirectionAngle(const sf::Vector2f& direction)
		{
			return std::atan2(direction.y, direction.x) * DegreesPerRadian;
		}

		class StrikeRunBombardmentActorTypeHandler final
			: public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::StrikeRunBombardment;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return BombardmentAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return BombardmentCommonAttributes;
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
					CommonAttributeIds::Radius,
					CommonAttributeIds::Range,
					AbilityData::StrikeRun::Attribute::ImpactCount,
					AbilityData::StrikeRun::Attribute::ImpactSpan,
					AbilityData::StrikeRun::Attribute::TargetingWindow,
					AbilityData::StrikeRun::Attribute::FinalTelegraphDuration,
					AbilityData::StrikeRun::Attribute::ImpactDelay
				})
				{
					const sas::GameplayAttribute* attribute = sas::FindAttribute(
						definition.attributes,
						required
					);
					if (!attribute || !std::isfinite(attribute->baseValue))
					{
						return {
							false,
							"Strike Run bombardment is missing required attribute '" +
								std::string{ required.GetName() } + "'."
						};
					}
				}

				const auto value = [&](const sas::AttributeId& id)
				{
					return sas::FindAttributeValue(definition.attributes, id, 0.f);
				};
				const float impactCount = value(AbilityData::StrikeRun::Attribute::ImpactCount);
				const float impactSpan = value(AbilityData::StrikeRun::Attribute::ImpactSpan);
				const float radius = value(CommonAttributeIds::Radius);
				const bool validProfile = definition.presentationProfileId.IsValid() &&
					PresentationProfileRegistry<StrikeRunPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					) != nullptr;
				if (std::lround(impactCount) != ShippedImpactCount ||
					impactCount < 1.f || impactSpan <= 0.f || radius <= 0.f ||
					value(CommonAttributeIds::Range) <= 0.f ||
					value(AbilityData::StrikeRun::Attribute::TargetingWindow) <= 0.f ||
					value(AbilityData::StrikeRun::Attribute::FinalTelegraphDuration) <= 0.f ||
					value(AbilityData::StrikeRun::Attribute::ImpactDelay) <= 0.f ||
					!validProfile)
				{
					return {
						false,
						"Strike Run bombardment requires five impacts, positive span/timing values, and a registered presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const StrikeRunPresentationProfile* profile =
					PresentationProfileRegistry<StrikeRunPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<StrikeRunBombardmentActor>(
						&context.owner,
						*profile
					)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	StrikeRunBombardmentActor::StrikeRunBombardmentActor(
		World* world,
		Actor* owner,
		const StrikeRunPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner),
		mPresentationProfile(presentationProfile),
		mStrikeLine({ 1.f, 1.f })
	{
		SetRenderLayer(RenderLayer::GroundDecal);
		// This actor is a fixed impact scheduler and visual delivery craft. It is
		// intentionally non-physical, so it cannot block ships or be reflected.
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
		mStrikeCraft.setPointCount(3);
		mStrikeCraft.setPoint(0, { 70.f, 0.f });
		mStrikeCraft.setPoint(1, { -35.f, 20.f });
		mStrikeCraft.setPoint(2, { -35.f, -20.f });
	}

	void StrikeRunBombardmentActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mExplosionRadius = std::max(0.f, sas::FindAttributeValue(
			attributes,
			CommonAttributeIds::Radius,
			mExplosionRadius
		));
		mImpactSpan = std::max(0.f, sas::FindAttributeValue(
			attributes,
			AbilityData::StrikeRun::Attribute::ImpactSpan,
			mImpactSpan
		));
		mImpactCount = std::clamp(static_cast<int>(std::lround(
			sas::FindAttributeValue(
				attributes,
				AbilityData::StrikeRun::Attribute::ImpactCount,
				static_cast<float>(mImpactCount)
			)
		)), 1, ShippedImpactCount);
		mFinalTelegraphDuration = std::max(0.01f, sas::FindAttributeValue(
			attributes,
			AbilityData::StrikeRun::Attribute::FinalTelegraphDuration,
			mFinalTelegraphDuration
		));
		mImpactDelay = std::max(0.01f, sas::FindAttributeValue(
			attributes,
			AbilityData::StrikeRun::Attribute::ImpactDelay,
			mImpactDelay
		));
		mDirection = NormalizeOrDefault(GetActorForwardDirection(), { 1.f, 0.f });
		mPhase = Phase::Preview;
		mPhaseElapsed = 0.f;
		mDetonatedImpactCount = 0;
		mConfirmed = false;
		mExplosionAges.fill(0.f);
		mImpactDetonated.fill(false);
		RebuildImpactLocations();
	}

	void StrikeRunBombardmentActor::SetPreviewDirection(
		const sf::Vector2f& direction
	)
	{
		if (mPhase != Phase::Preview)
		{
			return;
		}
		mDirection = NormalizeOrDefault(direction, mDirection);
		RebuildImpactLocations();
	}

	void StrikeRunBombardmentActor::Confirm()
	{
		if (mPhase != Phase::Preview || mConfirmed)
		{
			return;
		}
		mConfirmed = true;
		mPhase = Phase::Telegraph;
		mPhaseElapsed = 0.f;
	}

	void StrikeRunBombardmentActor::CancelPreview()
	{
		if (mPhase == Phase::Preview)
		{
			Destroy();
		}
	}

	void StrikeRunBombardmentActor::RebuildImpactLocations()
	{
		const sf::Vector2f center = GetActorLocation();
		for (int index = 0; index < ShippedImpactCount; ++index)
		{
			const float normalized = static_cast<float>(index) /
				static_cast<float>(ShippedImpactCount - 1);
			const float offset = mImpactSpan * normalized;
			mImpactLocations[static_cast<std::size_t>(index)] = center + mDirection * offset;
		}
	}

	void StrikeRunBombardmentActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy() || IsInPortalTransit())
		{
			return;
		}

		const float safeDeltaTime = std::max(0.f, deltaTime);
		for (int index = 0; index < mImpactCount; ++index)
		{
			if (mImpactDetonated[static_cast<std::size_t>(index)])
			{
				mExplosionAges[static_cast<std::size_t>(index)] += safeDeltaTime;
			}
		}

		float remainingDeltaTime = safeDeltaTime;
		if (mPhase == Phase::Telegraph)
		{
			const float telegraphRemaining = std::max(
				0.f,
				mFinalTelegraphDuration - mPhaseElapsed
			);
			if (remainingDeltaTime < telegraphRemaining)
			{
				mPhaseElapsed += remainingDeltaTime;
				remainingDeltaTime = 0.f;
			}
			else
			{
				remainingDeltaTime -= telegraphRemaining;
				mPhase = Phase::Impact;
				mPhaseElapsed = 0.f;
			}
		}

		if (mPhase == Phase::Impact)
		{
			mPhaseElapsed += remainingDeltaTime;
			while (mDetonatedImpactCount < mImpactCount &&
				mPhaseElapsed >= static_cast<float>(mDetonatedImpactCount) * mImpactDelay)
			{
				DetonateImpact(mDetonatedImpactCount);
			}

			const float lastImpactTime =
				static_cast<float>(std::max(0, mImpactCount - 1)) * mImpactDelay;
			if (mDetonatedImpactCount >= mImpactCount &&
				mPhaseElapsed >= lastImpactTime +
					mPresentationProfile.explosion.duration)
			{
				mPhase = Phase::Finished;
			}
		}

		if (mPhase == Phase::Finished)
		{
			Destroy();
			return;
		}

		AbilityWorldActor::Tick(safeDeltaTime);
	}

	void StrikeRunBombardmentActor::DetonateImpact(int index)
	{
		if (index < 0 || index >= mImpactCount ||
			mImpactDetonated[static_cast<std::size_t>(index)])
		{
			return;
		}

		const std::size_t impactIndex = static_cast<std::size_t>(index);
		mImpactDetonated[impactIndex] = true;
		mExplosionAges[impactIndex] = 0.f;
		++mDetonatedImpactCount;

		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner || owner->GetIsPendingDestroy())
		{
			return;
		}

		const sf::Vector2f impactLocation = mImpactLocations[impactIndex];
		for (const shared_ptr<Actor>& target : targeting::FindOpposingCombatants(
			*world,
			*owner,
			impactLocation,
			mExplosionRadius
		))
		{
			if (!target || target->GetIsPendingDestroy())
			{
				continue;
			}
			const sf::Vector2f delta = target->GetActorLocation() - impactLocation;
			if (delta.x * delta.x + delta.y * delta.y >
				mExplosionRadius * mExplosionRadius)
			{
				continue;
			}
			ApplyCombatDamage(
				*target,
				GetDamage(),
				owner,
				GetDamageTags(),
				GetDamagePayload(),
				GetSourceAbilityId(),
				GetSourceAbilityTags(),
				DamageDeliveryType::Area,
				this
			);
		}
	}

	void StrikeRunBombardmentActor::RenderTelegraph(
		sf::RenderWindow& window,
		float pulse
	)
	{
		const auto& visual = mPresentationProfile.telegraph;
		const sf::Vector2f center = GetActorLocation();
		const float angle = DirectionAngle(mDirection);
		const sf::Color lineColor = WithAlpha(
			visual.linePulseColor,
			0.65f + 0.35f * pulse
		);
		const sf::Color markerFill = WithAlpha(
			visual.markerFillColor,
			mPhase == Phase::Preview ? 0.80f : 1.f
		);
		mStrikeLine.setSize({ mImpactSpan, visual.lineThickness });
		mStrikeLine.setOrigin({ 0.f, visual.lineThickness * 0.5f });
		mStrikeLine.setPosition(center);
		mStrikeLine.setRotation(sf::degrees(angle));
		mStrikeLine.setFillColor(lineColor);
		window.draw(mStrikeLine, sf::RenderStates{ sf::BlendAdd });

		for (int index = 0; index < mImpactCount; ++index)
		{
			const std::size_t impactIndex = static_cast<std::size_t>(index);
			sf::CircleShape marker{ std::max(1.f, mExplosionRadius), 64 };
			marker.setOrigin({ mExplosionRadius, mExplosionRadius });
			marker.setPosition(mImpactLocations[impactIndex]);
			marker.setFillColor(markerFill);
			marker.setOutlineColor(WithAlpha(
				visual.markerOutlineColor,
				0.70f + 0.30f * pulse
			));
			marker.setOutlineThickness(visual.markerOutlineThickness);
			window.draw(marker, sf::RenderStates{ sf::BlendAdd });
		}
	}

	void StrikeRunBombardmentActor::RenderExplosions(
		sf::RenderWindow& window
	) const
	{
		const auto& visual = mPresentationProfile.explosion;
		const float duration = std::max(0.01f, visual.duration);
		for (int index = 0; index < mImpactCount; ++index)
		{
			const std::size_t impactIndex = static_cast<std::size_t>(index);
			if (!mImpactDetonated[impactIndex])
			{
				continue;
			}
			const float progress = std::clamp(
				mExplosionAges[impactIndex] / duration,
				0.f,
				1.f
			);
			const float eased = 1.f - (1.f - progress) * (1.f - progress);
			sf::CircleShape explosion{
				std::max(1.f, mExplosionRadius * (0.65f + 0.35f * eased)),
				64
			};
			const float explosionRadius = std::max(
				1.f,
				mExplosionRadius * (0.65f + 0.35f * eased)
			);
			explosion.setOrigin({ explosionRadius, explosionRadius });
			explosion.setPosition(mImpactLocations[impactIndex]);
			explosion.setFillColor(WithAlpha(
				visual.outerFillColor,
				1.f - progress
			));
			explosion.setOutlineColor(WithAlpha(
				visual.outerOutlineColor,
				1.f - progress * 0.55f
			));
			explosion.setOutlineThickness(visual.shockwaveThickness * 0.6f);
			window.draw(explosion, sf::RenderStates{ sf::BlendAdd });

			sf::CircleShape shockwave{
				std::max(1.f, mExplosionRadius * progress),
				64
			};
			const float shockwaveRadius = std::max(1.f, mExplosionRadius * progress);
			shockwave.setOrigin({ shockwaveRadius, shockwaveRadius });
			shockwave.setPosition(mImpactLocations[impactIndex]);
			shockwave.setFillColor(sf::Color::Transparent);
			shockwave.setOutlineColor(WithAlpha(
				visual.shockwaveColor,
				1.f - progress
			));
			shockwave.setOutlineThickness(visual.shockwaveThickness);
			window.draw(shockwave, sf::RenderStates{ sf::BlendAdd });
		}
	}

	void StrikeRunBombardmentActor::RenderStrikeCraft(
		sf::RenderWindow& window
	)
	{
		const std::size_t lastImpactIndex = static_cast<std::size_t>(
			std::max(0, mImpactCount - 1)
		);
		if (mPhase == Phase::Preview)
		{
			mStrikeCraft.setPosition(mImpactLocations[lastImpactIndex]);
		}
		else
		{
			const float travelDuration = std::max(
				0.20f,
				static_cast<float>(std::max(0, mImpactCount - 1)) * mImpactDelay + 0.20f
			);
			const float progress = std::clamp(
				(mPhase == Phase::Telegraph ? 0.f : mPhaseElapsed / travelDuration),
				0.f,
				1.f
			);
			mStrikeCraft.setPosition(
				mImpactLocations[0] +
					(mImpactLocations[lastImpactIndex] - mImpactLocations[0]) * progress
			);
		}
		mStrikeCraft.setRotation(sf::degrees(DirectionAngle(mDirection)));
		mStrikeCraft.setFillColor(mPresentationProfile.explosion.craftColor);
		window.draw(mStrikeCraft, sf::RenderStates{ sf::BlendAdd });
	}

	void StrikeRunBombardmentActor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		const float pulse = 0.5f + 0.5f * std::sin(
			mPhaseElapsed * mPresentationProfile.telegraph.pulseSpeed
		);
		if (mPhase == Phase::Preview || mPhase == Phase::Telegraph ||
			mPhase == Phase::Impact)
		{
			RenderTelegraph(window, pulse);
		}
		if (mPhase == Phase::Impact)
		{
			RenderExplosions(window);
		}
		if (mPhase == Phase::Preview || mPhase == Phase::Telegraph ||
			mPhase == Phase::Impact)
		{
			RenderStrikeCraft(window);
		}
	}

	bool RegisterStrikeRunBombardmentActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<StrikeRunBombardmentActorTypeHandler>()
		);
	}
}
