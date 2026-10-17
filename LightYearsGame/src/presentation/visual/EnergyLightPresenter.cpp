#include "gameConfigs/CollisionLayers.h"
#include "presentation/visual/EnergyLightPresenter.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		constexpr int MaxProjectileGlows = 64;
		constexpr float ViewMargin = 160.f;

		float Length(sf::Vector2f v)
		{
			return std::sqrt(v.x * v.x + v.y * v.y);
		}

		struct Frame
		{
			// world position of output pixel (0, 0)
			sf::Vector2f origin;
			// output pixels per world unit
			float scale = 1.f;

			sf::Vector2f ToPixels(sf::Vector2f worldPosition) const { return (worldPosition - origin) * scale; }
		};

		visual::TrailParams MakeTrail(const Frame& frame, sf::Vector2f head, sf::Vector2f direction, float worldLength, int points)
		{
			visual::TrailParams trail;
			for (int i = 0; i < points; ++i)
			{
				float t = static_cast<float>(i) / static_cast<float>(points - 1);
				trail.points.push_back(frame.ToPixels(head + direction * (worldLength * t)));
			}
			return trail;
		}

		void SubmitProjectile(const Frame& frame, const Actor& projectile, sf::Color color, float intensity, visual::LuminanceTier tier, visual::VisualPipeline& pipeline)
		{
			using namespace visual;
			sf::Vector2f velocity = projectile.GetVelocity();
			const float speed = Length(velocity);
			const sf::Vector2f direction = speed > 1.f ? velocity / speed : projectile.GetActorForwardDirection();
			const sf::Vector2f head = projectile.GetActorLocation();
			const float radius = std::max(projectile.GetPhysicsCollisionRadius(), 4.f);

			TrailParams trail = MakeTrail(frame, head, -direction, std::clamp(speed * 0.04f, radius * 1.5f, 70.f), 5);
			trail.headWidth = radius * 1.4f * frame.scale;
			trail.tailWidth = radius * 0.2f * frame.scale;
			trail.color = color;
			trail.intensity = intensity;
			trail.tier = tier;
			pipeline.SubmitTrail(trail);

			GlowParams glow;
			glow.position = frame.ToPixels(head);
			glow.coreRadius = std::clamp(radius * 0.6f * frame.scale, 2.f, 8.f);
			glow.haloRadius = std::clamp(radius * 3.5f * frame.scale, 12.f, 44.f);
			glow.color = color;
			glow.intensity = intensity;
			glow.tier = tier;
			pipeline.SubmitGlow(glow);
		}
	}

	void EnergyLightPresenter::Submit(const World& world, const sf::View& worldView, visual::VisualAssets& assets, visual::VisualPipeline& pipeline) const
	{
		using namespace visual;
		const sf::Vector2f viewSize = worldView.getSize();
		if (viewSize.y <= 0.f)
		{
			return;
		}
		const sf::Vector2f output(pipeline.OutputSize());
		// worldView is the pixel-snapped view, so glows line up with the sprites drawn through the same view.
		const Frame frame{ worldView.getCenter() - viewSize * 0.5f, output.y / viewSize.y };
		const ColorScript& script = assets.Script();
		const sf::Color playerShot = script.Role("player").Sample(0.85f);
		const sf::Color enemyShot = script.Role("enemy").Sample(0.8f);

		const sf::Vector2f margin(ViewMargin, ViewMargin);
		const sf::FloatRect bounds(worldView.getCenter() - viewSize * 0.5f - margin, viewSize + margin * 2.f);
		int projectileGlows = 0;
		world.ForEachActorInBounds(bounds, [&](Actor& actor)
		{
			if (actor.GetIsPendingDestroy())
			{
				return;
			}
			const CollisionLayer layer = actor.GetCollisionLayer();
			// The player ship's engine and port lights belong to ShipVisualPresenter, not to this scan.
			if (layer == GameCollision::Player)
			{
				return;
			}
			if ((layer != GameCollision::PlayerBullet && layer != GameCollision::EnemyBullet) || projectileGlows >= MaxProjectileGlows)
			{
				return;
			}
			const AbilityWorldActor* projectile = dynamic_cast<const AbilityWorldActor*>(&actor);
			if (!projectile || !projectile->IsProjectileActor())
			{
				return;
			}
			++projectileGlows;
			// Player shots stay in the Player tier so the player always reads their own fire; a lower intensity
			// keeps them dimmer than the engine, the hottest light on screen. Enemy shots are capped one tier below.
			if (layer == GameCollision::PlayerBullet)
			{
				SubmitProjectile(frame, actor, playerShot, 0.7f, LuminanceTier::Player, pipeline);
			}
			else
			{
				SubmitProjectile(frame, actor, enemyShot, 0.9f, LuminanceTier::EnemyProjectile, pipeline);
			}
		});
	}
}
