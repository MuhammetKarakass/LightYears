#pragma once

#include "weapon/bulletShooter/MultiShooter.h"
#include "framework/Core.h"
#include "framework/MathUtility.h"


namespace ly
{
	namespace ShooterPresets
	{
		inline ShooterInitializer MakeBulletShooter(
			const BulletDefinition& bulletDef,
			const sf::Vector2f& offset,
			float rotation = 0.f)
		{
			return [=](Actor* owner) -> unique_ptr<BulletShooter> {
				return std::make_unique<BulletShooter>(
					owner,
					bulletDef,
					0.f,
					offset,
					rotation
				);
				};
		}


		inline List<ShooterInitializer> Dual(
			const BulletDefinition& bulletDef,
			sf::Vector2f offset,
			float rotation = 0.f,
			float spacing = 20.f
		)
		{
			return {
				MakeBulletShooter(
					bulletDef,
					offset + sf::Vector2f{-spacing / 2.f, 0.f},
					rotation
				),
				MakeBulletShooter(
					bulletDef,
					offset + sf::Vector2f{spacing / 2.f, 0.f},
					rotation
				)
			};
		}

		inline List<ShooterInitializer> VShape(
			const BulletDefinition& bulletDef,
			const sf::Vector2f& baseOffset,
			float angle = 30.f)
		{
			return {
				MakeBulletShooter(bulletDef,
					baseOffset + sf::Vector2f{-10.f, -10.f}, -angle),
				MakeBulletShooter(bulletDef,
					baseOffset, 0.f),
				MakeBulletShooter(bulletDef,
					baseOffset + sf::Vector2f{10.f, -10.f}, angle)
			};
		}
		inline List<ShooterInitializer> XShape(
			const BulletDefinition& bulletDef,
			float cooldownTime,
			const sf::Vector2f& baseOffset,
			float damage = 10.f,
			float speed = 450.f)
		{
			return {
				MakeBulletShooter(bulletDef, baseOffset, -45.f),
				MakeBulletShooter(bulletDef, baseOffset, 45.f),
				MakeBulletShooter(bulletDef, baseOffset, -135.f),
				MakeBulletShooter(bulletDef, baseOffset, 135.f)
			};
		}

		inline List<ShooterInitializer> PlusShape(
			const BulletDefinition& bulletDef,
			float cooldownTime,
			const sf::Vector2f& baseOffset)
		{
			return {
				MakeBulletShooter(bulletDef, baseOffset, 0.f),
				MakeBulletShooter(bulletDef,  baseOffset, 90.f), 
				MakeBulletShooter(bulletDef, baseOffset, 180.f),
				MakeBulletShooter(bulletDef, baseOffset, -90.f) 
			};
		}


		inline List<ShooterInitializer> Circular(
			const BulletDefinition& bulletDef,
			const sf::Vector2f& baseOffset = {0.f, -50.f},
			int bulletCount = 8,
			float radius = 0.f,
			float startAngle = 0.f)
		{
			List<ShooterInitializer> shooters;
			float angleStep = 360.f / bulletCount;
			for (int i = 0; i <= bulletCount; ++i)
			{
				float angle = startAngle+ (i * angleStep);
				float radian = DegreesToRadians(angle);
				sf::Vector2f offset = baseOffset;

				if (radius > 0.f)
				{
					offset += sf::Vector2f{
						radius * std::cos(radian),
						-radius * std::sin(radian)
					};
				}

				// Each bullet shooter has NO cooldown - MultiShooter will handle timing
				shooters.push_back(MakeBulletShooter(bulletDef, offset, angle+90.f));
			}
			return shooters;
		}

		inline List<ShooterInitializer> Fan(
			const BulletDefinition& bulletDef,
			const sf::Vector2f& baseOffset,
			int bulletCount = 5,
			float totalAngle = 90.f)
		{
			List<ShooterInitializer> shooters;
			float angleStep = totalAngle / (bulletCount - 1);
			float startAngle = -totalAngle / 2.f;

			for (int i = 0; i < bulletCount; ++i)
			{
				float angle = startAngle + (angleStep * i);
				shooters.push_back(
					MakeBulletShooter(bulletDef, baseOffset, angle)
				);
			}

			return shooters;
		}

		inline List<ShooterInitializer> BossCenterPower(
			const BulletDefinition& centerBulletDef,
			const BulletDefinition& sideBulletDef,
			const sf::Vector2f& centerOffset,
			const sf::Vector2f& sideOffset)
		{
			return {
				MakeBulletShooter(sideBulletDef,
					sideOffset + sf::Vector2f{-40.f, 0.f}, -15.f),   // Zayýf
				MakeBulletShooter(centerBulletDef,
					centerOffset, 0.f),                              // GÜÇLÜ!
				MakeBulletShooter(sideBulletDef,
					sideOffset + sf::Vector2f{40.f, 0.f}, 15.f)      // Zayýf
			};
		}
	}
}