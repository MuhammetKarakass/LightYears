#include "enemy/UFO.h"
#include "gameplay/combat/Combatant.h"
#include <framework/World.h>
#include <cmath> 
#include <algorithm> 
#include <framework/MathUtility.h>

namespace ly
{
	UFO::UFO(World* owningWorld, const ShipDefinition& shipDef, const sf::Vector2f& velocity, float rotationSpeed) :
		EnemySpaceShip{ owningWorld, shipDef },
		mRotationSpeed{ rotationSpeed }
	{
		SetVelocity(velocity);
		SetActorRotation(180.f);
		SetScoreAmt(shipDef.scoreAmt);

		float visualRadius = std::min(GetActorGlobalBounds().size.x, GetActorGlobalBounds().size.y) / 2.f;
		float collisionRadius = visualRadius * 0.4f;
		SetCollisionRadius(collisionRadius);

		mAttachedLightTags.push_back(AddLight(GameTags::Ship::Engine_Main, shipDef.engineMounts[0].pointLightDef, shipDef.engineMounts[0].offset));
	}

	UFO::~UFO()
	{
	}

	void UFO::Tick(float deltaTime)
	{
		EnemySpaceShip::Tick(deltaTime);
		Shoot();
		AddActorRotationOffset(deltaTime * mRotationSpeed);

		CheckBounce();
	}

	void UFO::SetupCollisionLayers()
	{
		SetCollisionLayer(CollisionLayer::Enemy);
		SetCollisionMask(CollisionLayer::Player | CollisionLayer::PlayerBullet | CollisionLayer::Enemy);
	}

	void UFO::OnActorBeginOverlap(Actor* otherActor)
	{
		SpaceShip::OnActorBeginOverlap(otherActor);

		if (!otherActor || !GetCanCollide()) return;

		UFO* otherUFO = dynamic_cast<UFO*>(otherActor);
		if (otherUFO)
		{
		
			sf::Vector2f thisPos = GetActorLocation();
			sf::Vector2f otherPos = otherUFO->GetActorLocation();

			sf::Vector2f collisionNormal = thisPos - otherPos;

			if (collisionNormal.x == 0.f && collisionNormal.y == 0.f)
			{
				collisionNormal = sf::Vector2f(RandRange(-1.f, 1.f), RandRange(-1.f, 1.f));
			}

			NormalizeVector(collisionNormal);

			sf::Vector2f v1 = GetVelocity();
			sf::Vector2f v2 = otherUFO->GetVelocity();
			sf::Vector2f relativeVelocity = v1 - v2;

			float velocityAlongNormal = relativeVelocity.x * collisionNormal.x + relativeVelocity.y * collisionNormal.y;

			if (velocityAlongNormal > 0.f)
			{
				return;
			}
			float restitution = 0.8f;

			float impulseMagnitude = -(1.f + restitution) * velocityAlongNormal / 2.f;
			sf::Vector2f impulse = collisionNormal * impulseMagnitude;

			SetVelocity(v1 + impulse);
			otherUFO->SetVelocity(v2 - impulse);

			return;
		}

			ApplyCombatDamage(*otherActor, GetCollisionDamage(), this);
	}

	void UFO::CheckBounce()
	{
		auto windowSize = GetWorld()->GetWindowSize();

		sf::Vector2f currentPos = GetActorLocation();
		sf::Vector2f currentVelocity = GetVelocity();


		float spriteWidth = GetActorGlobalBounds().size.x - 100.f;

		bool bounced = false;

		if (currentPos.x - spriteWidth / 2.f <= 0.f && currentVelocity.x < 0.f)
		{
			currentVelocity.x = -currentVelocity.x;
			bounced = true;

			currentPos.x = spriteWidth / 2.f;
		}
		else if (currentPos.x + spriteWidth / 2.f >= windowSize.x && currentVelocity.x > 0.f)
		{
			currentVelocity.x = -currentVelocity.x;
			bounced = true;

			currentPos.x = windowSize.x - spriteWidth / 2.f;
		}

		if (bounced)
		{
			SetVelocity(currentVelocity);
			SetActorLocation(currentPos);
		}
	}

	void UFO::Shoot()
	{
		GetAbilitySystemComponent().SetAbilitySlotInput(sas::AbilitySlot::PrimaryFire, true);
	}
}


