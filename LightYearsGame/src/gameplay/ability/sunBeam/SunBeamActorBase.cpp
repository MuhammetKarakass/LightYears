#include "attributes/AttributeSystem.h"
#include "gameplay/attributes/AttributeIds.h"
#include "gameplay/ability/sunBeam/SunBeamActorBase.h"

#include "framework/World.h"
#include "gameConfigs/ability/offensive/SunBeamConfig.h"
#include "gameplay/combat/Combatant.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	SunBeamActorBase::SunBeamActorBase(
		World* world,
		Actor* owner,
		const SunBeamVisualDefinition& visualDefinition
	)
		: AbilityWorldActor(world, owner),
		mVisualDefinition(visualDefinition)
	{
		// Beams use explicit area queries; a sprite-free Box2D body would have
		// a zero-sized fixture and is neither needed nor valid.
		SetAbilityPhysicsEnabled(false);
		SetRenderLayer(RenderLayer::WorldVfx);
	}

	void SunBeamActorBase::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		OnSunBeamBeginPlay();
	}

	void SunBeamActorBase::Tick(float deltaTime)
	{
		AbilityWorldActor::Tick(deltaTime);

		if (!GetIsPendingDestroy())
		{
			TickSunBeam(deltaTime);
		}
	}

	void SunBeamActorBase::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}

		AbilityWorldActor::Render(window);
		const SunBeamVisualFrame frame = BuildSunBeamVisualFrame();
		mVisual.Draw(window, GetActorLocation(), frame, GetAge());
	}

	void SunBeamActorBase::ConfigureFromAttributes(const sas::GameplayAttributeList& attributes)
	{
		AbilityWorldActor::ConfigureFromAttributes(attributes);

		mBeamWidth = std::max(
			1.f,
			sas::FindGameplayAttributeValue(
				attributes,
				AbilityData::SunBeam::ActorSchema::Width,
				mBeamWidth
			)
		);

		mBeamLength = std::max(
			1.f,
			sas::FindGameplayAttributeValue(
				attributes,
				AbilityData::SunBeam::ActorSchema::Length,
				mBeamLength
			)
		);
		mImpactRadius = std::max(
			1.f,
			sas::FindGameplayAttributeValue(attributes, CommonAttributeIds::Radius, mImpactRadius)
		);

		mVisual.Configure(mVisualDefinition, mBeamWidth, mBeamLength, mImpactRadius);
		ConfigureSunBeam(attributes);
	}

	void SunBeamActorBase::ApplyBeamDamageInRectangle(
		const sf::Vector2f& center,
		float length,
		float damageMultiplier
	)
	{
		World* world = GetWorld();
		if (world == nullptr)
		{
			return;
		}

		const float halfWidth = mBeamWidth * 0.5f;
		const float halfLength = std::max(0.f, length) * 0.5f;
		const float damage = std::max(0.f, GetDamage() * std::max(0.f, damageMultiplier));
		if (damage <= 0.f)
		{
			return;
		}

		for (const weak_ptr<Actor>& actorWeak : world->GetActorsByType<Actor>())
		{
			const shared_ptr<Actor> target = actorWeak.lock();
			if (!target || !IsValidAbilityTarget(target.get()))
			{
				continue;
			}

			const sf::Vector2f delta = target->GetActorLocation() - center;
			if (std::abs(delta.x) > halfWidth || std::abs(delta.y) > halfLength)
			{
				continue;
			}

			ApplyCombatDamage(*target, damage, GetOwnerActor(), GetDamageTags(), GetDamagePayload());
		}
	}

}
