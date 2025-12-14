#pragma once

#include "weapon/bulletShooter/MultiShooter.h"
#include "framework/Core.h"
#include "framework/MathUtility.h"


namespace ly
{
	namespace ShooterPresets
	{
		inline ShooterInitializer MakeBulletShooter(
			const std::string& texturePath,
			const sf::Vector2f& offset,
			float rotation = 0.f,
			float damage = 10.f,
			float speed = 600.f)
		{
			return [=](Actor* owner) -> unique_ptr<BulletShooter> {
				return std::make_unique<BulletShooter>(
					owner,
					texturePath,
					0.f,
					offset,
					rotation,
					damage,
					speed
				);
				};
		}


		inline List<ShooterInitializer> Dual(
			const std::string& texturePath,
			sf::Vector2f offset,
			float rotation = 0.f,
			float spacing = 20.f,
			float damage = 10.f,
			float speed = 600.f)
		{
			return {
				MakeBulletShooter(
					texturePath,
					offset + sf::Vector2f{-spacing / 2.f, 0.f},
					rotation,
					damage,
					speed),
				MakeBulletShooter(
					texturePath,
					offset + sf::Vector2f{spacing / 2.f, 0.f},
					rotation,
					damage,
					speed)
			};
		}

		inline List<ShooterInitializer> VShape(
			const std::string& texturePath,
			const sf::Vector2f& baseOffset,
			float angle = 30.f,
			float damage = 10.f,
			float speed = 600.f)
		{
			return {
				MakeBulletShooter(texturePath,
					baseOffset + sf::Vector2f{-10.f, -10.f}, -angle, damage, speed),
				MakeBulletShooter(texturePath,
					baseOffset, 0.f, damage, speed),
				MakeBulletShooter(texturePath,
					baseOffset + sf::Vector2f{10.f, -10.f}, angle, damage, speed)
			};
		}
		inline List<ShooterInitializer> XShape(
			const std::string& texturePath,
			float cooldownTime,
			const sf::Vector2f& baseOffset,
			float damage = 10.f,
			float speed = 450.f)
		{
			return {
				MakeBulletShooter(texturePath, baseOffset, -45.f, damage, speed),
				MakeBulletShooter(texturePath, baseOffset, 45.f, damage, speed),
				MakeBulletShooter(texturePath, baseOffset, -135.f, damage, speed),
				MakeBulletShooter(texturePath, baseOffset, 135.f, damage, speed)
			};
		}

		inline List<ShooterInitializer> PlusShape(
			const std::string& texturePath,
			float cooldownTime,
			const sf::Vector2f& baseOffset,
			float damage = 10.f,
			float speed = 450.f)
		{
			return {
				MakeBulletShooter(texturePath, baseOffset, 0.f, damage, speed),
				MakeBulletShooter(texturePath,  baseOffset, 90.f, damage, speed), 
				MakeBulletShooter(texturePath, baseOffset, 180.f, damage, speed),
				MakeBulletShooter(texturePath, baseOffset, -90.f, damage, speed) 
			};
		}


		inline List<ShooterInitializer> Circular(
			const std::string& texturePath,
			const sf::Vector2f& baseOffset = {0.f, -50.f},
			int bulletCount = 8,
			float damage = 7.5f,
			float radius = 0.f,
			float speed = 300.f,
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
				shooters.push_back(MakeBulletShooter(texturePath, offset, angle+90.f, damage, speed));
			}
			return shooters;
		}

		inline List<ShooterInitializer> Fan(
			const std::string& texturePath,
			const sf::Vector2f& baseOffset,
			int bulletCount = 5,
			float totalAngle = 90.f,
			float damage = 10.f,
			float speed = 600.f)
		{
			List<ShooterInitializer> shooters;
			float angleStep = totalAngle / (bulletCount - 1);
			float startAngle = -totalAngle / 2.f;

			for (int i = 0; i < bulletCount; ++i)
			{
				float angle = startAngle + (angleStep * i);
				shooters.push_back(
					MakeBulletShooter(texturePath, baseOffset, angle, damage, speed)
				);
			}

			return shooters;
		}

		inline List<ShooterInitializer> BossCenterPower(
			const std::string& sideTexturePath,
			const std::string& centerTexturePath,
			const sf::Vector2f& centerOffset,
			const sf::Vector2f& sideOffset,
			int bulletCount = 3,
			float centerDamage = 30.f,
			float sideDamage = 10.f,
			float centerSpeed = 900.f,
			float sideSpeed= 600.f)
		{
			return {
				MakeBulletShooter(sideTexturePath, 
					sideOffset + sf::Vector2f{-40.f, 0.f}, -15.f, sideDamage, sideSpeed),   // Zayýf
				MakeBulletShooter(centerTexturePath,
					centerOffset, 0.f, centerDamage, centerSpeed),                              // GÜÇLÜ!
				MakeBulletShooter(sideTexturePath,
					sideOffset + sf::Vector2f{40.f, 0.f}, 15.f, sideDamage, sideSpeed)      // Zayýf
			};
		}
	}
}