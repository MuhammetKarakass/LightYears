#include "gameplay/ability/closedCircuit/ClosedCircuitFieldActor.h"

#include "framework/MathUtility.h"
#include "framework/World.h"
#include "gameConfigs/ability/AbilityActorStructs.h"
#include "gameplay/ability/actors/AbilityActorRegistry.h"
#include "presentation/ability/PresentationProfileRegistry.h"

#include <SFML/Graphics/CircleShape.hpp>
#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace ly
{
	namespace
	{
		std::unordered_map<unsigned int, weak_ptr<ClosedCircuitFieldActor>>& ActiveFields()
		{
			static std::unordered_map<unsigned int, weak_ptr<ClosedCircuitFieldActor>> fields;
			return fields;
		}

		class ClosedCircuitFieldActorTypeHandler final : public AbilityActorTypeHandler
		{
		public:
			AbilityActorType GetActorType() const override { return AbilityActorType::ClosedCircuitDelivery; }
			AbilityActorValidationResult ValidateDefinition(const AbilityActorDefinition& definition) const override
			{
				const AbilityActorValidationResult base = AbilityActorTypeHandler::ValidateDefinition(definition);
				if (!base.isValid) return base;
				return definition.presentationProfileId.IsValid() && PresentationProfileRegistry<ClosedCircuitPresentationProfile>::Find(definition.presentationProfileId.ToString())
					? AbilityActorValidationResult{ true, {} }
					: AbilityActorValidationResult{ false, "Closed Circuit delivery requires its typed presentation profile." };
			}
			weak_ptr<AbilityWorldActor> Spawn(const AbilityActorSpawnContext& context) const override
			{
				const auto* profile = PresentationProfileRegistry<ClosedCircuitPresentationProfile>::Find(context.definition.presentationProfileId.ToString());
				return context.owner.GetWorld() && profile ? context.owner.GetWorld()->SpawnActor<ClosedCircuitFieldActor>(&context.owner, *profile) : weak_ptr<AbilityWorldActor>{};
			}
		};
	}

	ClosedCircuitFieldActor::ClosedCircuitFieldActor(World* world, Actor* owner, const ClosedCircuitPresentationProfile& profile)
		: AbilityWorldActor(world, owner), mProfile(profile)
	{
		SetRenderLayer(RenderLayer::World);
		SetAbilityPhysicsEnabled(false);
	}

	void ClosedCircuitFieldActor::ConfigureDelivery(const sf::Vector2f& target, float speed, float formationDuration, float barrierRadius, float barrierHealth)
	{
		mTarget = target;
		mDeliverySpeed = std::max(1.f, speed);
		mFormationDuration = std::max(0.01f, formationDuration);
		mBarrierRadius = std::max(1.f, barrierRadius);
		mRemainingHealth = std::max(0.f, barrierHealth);
		mMaximumHealth = mRemainingHealth;
		mPhase = Phase::Delivery;
		mPhaseElapsed = 0.f;
	}

	void ClosedCircuitFieldActor::Tick(float deltaTime)
	{
		const float safeDelta = std::max(0.f, deltaTime);
		if (mPhase == Phase::Delivery)
		{
			const sf::Vector2f delta = mTarget - GetActorLocation();
			const float distance = GetVectorLength(delta);
			const float step = mDeliverySpeed * safeDelta;
			if (distance <= step || distance <= 0.001f)
			{
				SetActorLocation(mTarget); mPhase = Phase::Forming; mPhaseElapsed = 0.f;
			}
			else SetActorLocation(GetActorLocation() + delta / distance * step);
		}
		else if (mPhase == Phase::Forming)
		{
			mPhaseElapsed += safeDelta;
			if (mPhaseElapsed >= mFormationDuration) ActivateField();
		}
		AbilityWorldActor::Tick(safeDelta);
	}

	void ClosedCircuitFieldActor::ActivateField()
	{
		mPhase = Phase::Active;
		const shared_ptr<Actor> owner = LockOwnerActor();
		if (!owner) return;
		weak_ptr<ClosedCircuitFieldActor>& current = ActiveFields()[owner->GetUniqueID()];
		if (const shared_ptr<ClosedCircuitFieldActor> previous = current.lock(); previous && previous.get() != this) previous->Destroy();
		const shared_ptr<Object> object = GetWeakPtr().lock();
		current = object ? std::dynamic_pointer_cast<ClosedCircuitFieldActor>(object) : weak_ptr<ClosedCircuitFieldActor>{};
	}

	bool ClosedCircuitFieldActor::TryInterceptProjectile(AbilityWorldActor& projectile, const sf::Vector2f& previousLocation)
	{
		if (GetIsPendingDestroy() || projectile.GetIsPendingDestroy() ||
			projectile.GetWorld() != GetWorld() || mPhase != Phase::Active ||
			mRemainingHealth <= 0.f ||
			projectile.GetCollisionLayer() != CollisionLayer::EnemyBullet)
		{
			return false;
		}
		const sf::Vector2f center = GetActorLocation();
		const sf::Vector2f end = projectile.GetActorLocation();
		const sf::Vector2f startOffset = previousLocation - center;
		const sf::Vector2f endOffset = end - center;
		const float radiusSquared = mBarrierRadius * mBarrierRadius;
		if (startOffset.x * startOffset.x + startOffset.y * startOffset.y <= radiusSquared || endOffset.x * endOffset.x + endOffset.y * endOffset.y > radiusSquared) return false;
		const sf::Vector2f direction = end - previousLocation;
		const float a = direction.x * direction.x + direction.y * direction.y;
		const float b = 2.f * (startOffset.x * direction.x + startOffset.y * direction.y);
		const float c = startOffset.x * startOffset.x + startOffset.y * startOffset.y - radiusSquared;
		const float discriminant = b * b - 4.f * a * c;
		if (a <= 0.001f || discriminant < 0.f) return false;
		const float fraction = std::clamp((-b - std::sqrt(discriminant)) / (2.f * a), 0.f, 1.f);
		projectile.SetActorLocation(previousLocation + direction * fraction);
		mRemainingHealth = std::max(0.f, mRemainingHealth - std::max(0.f, projectile.GetDamage()));
		if (mRemainingHealth <= 0.f) Destroy();
		return true;
	}

	void ClosedCircuitFieldActor::Render(sf::RenderWindow& window)
	{
		if (mPhase == Phase::Delivery)
		{
			sf::CircleShape projectile(mProfile.deliveryRadius);
			projectile.setOrigin({ mProfile.deliveryRadius, mProfile.deliveryRadius });
			projectile.setPosition(GetActorLocation()); projectile.setFillColor(mProfile.deliveryColor); window.draw(projectile); return;
		}
		const float formation = mPhase == Phase::Forming ? std::clamp(mPhaseElapsed / mFormationDuration, 0.f, 1.f) : 1.f;
		const float radius = mBarrierRadius * formation;
		const float healthFraction = mMaximumHealth > 0.f ? mRemainingHealth / mMaximumHealth : 0.f;
		const sf::Color color = mPhase == Phase::Forming ? mProfile.formationColor : (healthFraction < 0.30f ? mProfile.lowHealthColor : mProfile.fieldColor);
		sf::CircleShape barrier(radius, 64);
		barrier.setOrigin({ radius, radius }); barrier.setPosition(GetActorLocation());
		barrier.setFillColor(sf::Color{ color.r, color.g, color.b, 18 });
		barrier.setOutlineColor(color); barrier.setOutlineThickness(mProfile.fieldWidth); window.draw(barrier);
	}

	void ClosedCircuitFieldActor::Destroy()
	{
		// Derived projectile/field destruction can be requested by more than one
		// path in the same frame. Cleanup must be idempotent before touching the
		// active-field registry or any owner state.
		if (GetIsPendingDestroy())
		{
			return;
		}

		if (const shared_ptr<Actor> owner = LockOwnerActor())
		{
			auto found = ActiveFields().find(owner->GetUniqueID());
			if (found != ActiveFields().end())
			{
				const shared_ptr<ClosedCircuitFieldActor> active = found->second.lock();
				if (!active || active.get() == this) ActiveFields().erase(found);
			}
		}
		AbilityWorldActor::Destroy();
	}

	bool RegisterClosedCircuitFieldActorType()
	{
		return AbilityActorRegistry::RegisterHandler(std::make_unique<ClosedCircuitFieldActorTypeHandler>());
	}
}
