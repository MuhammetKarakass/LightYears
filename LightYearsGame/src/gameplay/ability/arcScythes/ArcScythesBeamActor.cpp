#include "gameplay/ability/arcScythes/ArcScythesBeamActor.h"

#include "attributes/AttributeSystem.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "gameplay/ability/arcScythes/ArcScythesContracts.h"
#include "gameplay/combat/Combatant.h"
#include "gameplay/combat/CombatRuntime.h"
#include "gameplay/portal/PortalTransferParticipant.h"
#include "gameplay/targeting/SweptGeometry.h"
#include "gameplay/time/PeriodicTickAccumulator.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>

namespace ly
{
	namespace
	{
		struct BeamSegment
		{
			sf::Vector2f start;
			sf::Vector2f end;
			sf::Vector2f direction;
		};

		const List<sas::AttributeId> ArcScythesCommonAttributes{
			CommonAttributeIds::Damage,
			CommonAttributeIds::Range
		};
		const List<sas::AttributeId> ArcScythesAttributeRoots{
			AbilityData::ArcScythes::Actor::Beam::Root,
			DamageAttributeIds::Root
		};

		bool HasPositiveAttribute(
			const AbilityActorDefinition& definition,
			const sas::AttributeId& id
		)
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes, id
			);
			return attribute && std::isfinite(attribute->baseValue) &&
				attribute->baseValue > 0.f;
		}

		bool HasNonNegativeAttribute(
			const AbilityActorDefinition& definition,
			const sas::AttributeId& id
		)
		{
			const sas::GameplayAttribute* attribute = sas::FindAttribute(
				definition.attributes, id
			);
			return attribute && std::isfinite(attribute->baseValue) &&
				attribute->baseValue >= 0.f;
		}

		sf::Color WithAlpha(const sf::Color& color, float alpha)
		{
			sf::Color result = color;
			result.a = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 1.f) * 255.f);
			return result;
		}

		void DrawBeamQuad(
			sf::RenderWindow& window,
			const sf::Vector2f& start,
			const sf::Vector2f& end,
			float halfThickness,
			const sf::Color& color
		)
		{
			const sf::Vector2f direction = end - start;
			const float length = GetVectorLength(direction);
			if (length <= 0.001f || halfThickness <= 0.f)
			{
				return;
			}
			const sf::Vector2f normal{
				-direction.y / length * halfThickness,
				direction.x / length * halfThickness
			};
			sf::VertexArray vertices(sf::PrimitiveType::TriangleStrip, 4);
			vertices[0] = sf::Vertex{ start + normal, color };
			vertices[1] = sf::Vertex{ start - normal, color };
			vertices[2] = sf::Vertex{ end + normal, color };
			vertices[3] = sf::Vertex{ end - normal, color };
			sf::RenderStates additive;
			additive.blendMode = sf::BlendAdd;
			window.draw(vertices, additive);
		}

		void DrawElectricArc(
			sf::RenderWindow& window,
			const BeamSegment& segment,
			float visualTime,
			const ArcScythesPresentationProfile& profile,
			float alpha
		)
		{
			constexpr std::size_t PointCount = 11;
			const sf::Vector2f normal{ -segment.direction.y, segment.direction.x };
			sf::VertexArray arc(sf::PrimitiveType::LineStrip, PointCount);
			for (std::size_t index = 0; index < PointCount; ++index)
			{
				const float progress = static_cast<float>(index) /
					static_cast<float>(PointCount - 1);
				const float offset = index == 0 || index == PointCount - 1
					? 0.f
					: std::sin(visualTime * profile.pulseSpeed + progress * 31.f) *
						profile.arcAmplitude;
				arc[index].position = segment.start +
					(segment.end - segment.start) * progress + normal * offset;
				arc[index].color = WithAlpha(profile.coreColor, alpha);
			}
			sf::RenderStates additive;
			additive.blendMode = sf::BlendAdd;
			window.draw(arc, additive);
		}

		class ArcScythesBeamActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override
			{
				return AbilityActorType::ArcScythesBeam;
			}

			const List<sas::AttributeId>& GetOwnedAttributeRoots() const override
			{
				return ArcScythesAttributeRoots;
			}

			const List<sas::AttributeId>& GetAllowedCommonAttributeIds() const override
			{
				return ArcScythesCommonAttributes;
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
				const bool validAttributes = definition.attributes.size() == 8 &&
					HasNonNegativeAttribute(definition, AbilityData::ArcScythes::Attribute::Damage) &&
					HasPositiveAttribute(definition, AbilityData::ArcScythes::Attribute::Range) &&
					HasPositiveAttribute(definition, AbilityData::ArcScythes::Attribute::CombatTickInterval) &&
					HasPositiveAttribute(definition, AbilityData::ArcScythes::Attribute::BeamHalfThickness) &&
					HasPositiveAttribute(definition, AbilityData::ArcScythes::Attribute::ElectricStacks) &&
					HasNonNegativeAttribute(definition, AbilityData::ArcScythes::Attribute::ElectricDamageTakenMultiplierPerStack) &&
					HasNonNegativeAttribute(definition, AbilityData::ArcScythes::Attribute::ElectricDuration) &&
					HasPositiveAttribute(definition, AbilityData::ArcScythes::Attribute::ElectricMaxStacks);
				if (!validAttributes || definition.lifeTime < 4.f ||
					!definition.presentationProfileId.IsValid() ||
					!PresentationProfileRegistry<ArcScythesPresentationProfile>::Find(
						definition.presentationProfileId.ToString()
					))
				{
					return {
						false,
						"Arc Scythes requires its eight beam and Electric payload attributes plus a registered typed presentation profile."
					};
				}
				return { true, {} };
			}

			weak_ptr<AbilityWorldActor> Spawn(
				const AbilityActorSpawnContext& context
			) const override
			{
				World* world = context.owner.GetWorld();
				const ArcScythesPresentationProfile* profile =
					PresentationProfileRegistry<ArcScythesPresentationProfile>::Find(
						context.definition.presentationProfileId.ToString()
					);
				return world && profile
					? world->SpawnActor<ArcScythesBeamActor>(&context.owner, *profile)
					: weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	ArcScythesBeamActor::ArcScythesBeamActor(
		World* world,
		Actor* owner,
		const ArcScythesPresentationProfile& presentationProfile
	)
		: AbilityWorldActor(world, owner)
		, mPresentationProfile(presentationProfile)
	{
		SetRenderLayer(RenderLayer::Projectile);
		// These are continuous query beams, not physical projectile bodies.
		SetAbilityPhysicsEnabled(false);
		SetCollisionLayer(CollisionLayer::None);
		SetCollisionMask(CollisionLayer::None);
	}

	void ArcScythesBeamActor::ConfigureFromAttributes(
		const sas::GameplayAttributeList& attributes
	)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);
		mRange = std::max(1.f, sas::FindAttributeValue(
			attributes, AbilityData::ArcScythes::Attribute::Range, 700.f
		));
		mCombatTickInterval = std::max(0.01f, sas::FindAttributeValue(
			attributes, AbilityData::ArcScythes::Attribute::CombatTickInterval, 0.25f
		));
		mBeamHalfThickness = std::max(1.f, sas::FindAttributeValue(
			attributes, AbilityData::ArcScythes::Attribute::BeamHalfThickness, 22.f
		));
	}

	void ArcScythesBeamActor::Tick(float deltaTime)
	{
		Actor* owner = GetOwnerActor();
		if (!owner || owner->GetIsPendingDestroy())
		{
			Destroy();
			return;
		}
		if (const auto* participant = dynamic_cast<const PortalTransferParticipant*>(owner);
			participant && participant->IsInPortalTransit())
		{
			SetRenderEnabled(false);
			return;
		}

		AbilityWorldActor::Tick(deltaTime);
		if (const auto* combatant = dynamic_cast<const Combatant*>(owner);
			!combatant || !combatant->GetCombatRuntime().GetAbilitySystemComponent()
				.GetOwnedTags().HasTag(AbilityData::ArcScythes::State::Active))
		{
			Destroy();
			return;
		}

		SetRenderEnabled(true);
		SetActorLocation(owner->GetActorLocation());
		mVisualTime += std::max(0.f, deltaTime);
		const int tickCount = time::ConsumePeriodicTicks(
			mCombatTickTimer, deltaTime, mCombatTickInterval
		);
		for (int tickIndex = 0; tickIndex < tickCount; ++tickIndex)
		{
			PerformCombatTick();
		}
	}

	void ArcScythesBeamActor::PerformCombatTick()
	{
		World* world = GetWorld();
		Actor* owner = GetOwnerActor();
		if (!world || !owner || GetDamage() <= 0.f)
		{
			return;
		}

		sf::Vector2f forward = owner->GetActorForwardDirection();
		if (GetVectorLength(forward) <= 0.001f)
		{
			forward = { 0.f, -1.f };
		}
		else
		{
			NormalizeVector(forward);
		}
		const sf::Vector2f side{ -forward.y, forward.x };
		const sf::Vector2f origin = owner->GetActorLocation();
		const std::array<BeamSegment, 2> beams{
			BeamSegment{ origin, origin + side * mRange, side },
			BeamSegment{ origin, origin - side * mRange, -side }
		};

		Set<Actor*> hitThisTick;
		for (const BeamSegment& beam : beams)
		{
			for (const weak_ptr<Actor>& candidate : world->GetActorsInBounds(
				targeting::swept::SegmentBounds(
					beam.start, beam.end, mBeamHalfThickness
				)
			))
			{
				const shared_ptr<Actor> target = candidate.lock();
				if (!target || target->GetIsPendingDestroy() ||
					hitThisTick.find(target.get()) != hitThisTick.end() ||
					!IsValidAbilityTarget(target.get()) ||
					!dynamic_cast<Combatant*>(target.get()) ||
					!targeting::swept::SegmentIntersectsExpandedBounds(
						beam.start,
						beam.end,
						target->GetActorGlobalBounds(),
						mBeamHalfThickness
					))
				{
					continue;
				}

				// The set spans both segments for this cadence only. A large collider
				// may overlap both sides, but it still receives one legitimate tick.
				hitThisTick.insert(target.get());
				ApplyCombatDamage(
					*target,
					GetDamage(),
					owner,
					GetDamageTags(),
					GetDamagePayload(),
					GetSourceAbilityId(),
					GetSourceAbilityTags(),
					DamageDeliveryType::Beam,
					this
				);
			}
		}
	}

	void ArcScythesBeamActor::Render(sf::RenderWindow& window)
	{
		if (!IsRenderEnabled() || GetIsPendingDestroy())
		{
			return;
		}

		Actor* owner = GetOwnerActor();
		if (!owner)
		{
			return;
		}
		sf::Vector2f forward = owner->GetActorForwardDirection();
		if (GetVectorLength(forward) <= 0.001f)
		{
			forward = { 0.f, -1.f };
		}
		else
		{
			NormalizeVector(forward);
		}
		const sf::Vector2f side{ -forward.y, forward.x };
		const sf::Vector2f origin = owner->GetActorLocation();
		const float alpha = 0.75f + 0.25f * std::sin(
			mVisualTime * mPresentationProfile.pulseSpeed
		);
		for (const sf::Vector2f& direction : { side, -side })
		{
			const BeamSegment visualBeam{
				origin + direction * std::min(mPresentationProfile.emitterOffset, mRange),
				origin + direction * mRange,
				direction
			};
			DrawBeamQuad(
				window,
				visualBeam.start,
				visualBeam.end,
				mPresentationProfile.outerHalfThickness,
				WithAlpha(mPresentationProfile.outerColor, alpha * 0.55f)
			);
			DrawBeamQuad(
				window,
				visualBeam.start,
				visualBeam.end,
				mPresentationProfile.coreHalfThickness,
				WithAlpha(mPresentationProfile.coreColor, alpha)
			);
			DrawElectricArc(window, visualBeam, mVisualTime, mPresentationProfile, alpha);
		}
	}

	bool RegisterArcScythesBeamActorType()
	{
		return AbilityActorRegistry::RegisterHandler(
			std::make_unique<ArcScythesBeamActorTypeHandler>()
		);
	}
}
