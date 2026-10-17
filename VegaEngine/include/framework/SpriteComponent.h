#pragma once
#include "framework/ActorComponent.h"
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

namespace vega
{
	// Textured sprite attached to an actor. The transform stays authoritative on
	// Actor: Actor::SetActorLocation/Rotation push the new values into this
	// component (SyncLocation/SyncRotation), so GetSprite() always reports the
	// actor's current position and rotation to gameplay code that tints, scales or
	// crops it. Actor(World*, texturePath) adds one when a path is given; add one
	// to a texture-less actor with GetOrAddComponent<SpriteComponent>() and
	// SetTexture(). The component can exist without a sprite (empty path or failed
	// texture load), so callers still check HasSprite()/GetSprite().
	class SpriteComponent : public ActorComponent
	{
		VEGA_CLASS(SpriteComponent)
	public:
		explicit SpriteComponent(Actor& owner, const std::string& texturePath = "");

		// Loads the texture and rebuilds the sprite at the owner's current transform
		// with the full texture rect and a centered pivot. An empty path or a failed
		// load leaves the current sprite untouched and returns false.
		bool SetTexture(const std::string& texturePath);
		const std::string& GetTexturePath() const { return mTexturePath; }
		sf::Vector2f GetScale() const { return mSprite ? mSprite->getScale() : mScale; }
		void SetScale(sf::Vector2f scale);
		void SetTextureRepeated(bool repeated);
		// Moves the origin to the middle of the sprite's current global bounds.
		void CenterPivot();
		// Resets the tint to opaque white (visible) or fully transparent (hidden).
		void SetVisibility(bool visible);

		bool HasSprite() const { return mSprite.has_value(); }
		std::optional<sf::Sprite>& GetSprite() { return mSprite; }
		const std::optional<sf::Sprite>& GetSprite() const { return mSprite; }
		// Global bounds of the sprite; zero-size rect at the origin when there is none.
		sf::FloatRect GetGlobalBounds() const;

		void Render(sf::RenderTarget& window) const;

		// Called by Actor when its transform changes.
		void SyncLocation();
		void SyncRotation();

	private:
		shared_ptr<sf::Texture> mTexture;
		std::optional<sf::Sprite> mSprite;
		std::string mTexturePath;
		sf::Vector2f mScale{ 1.f, 1.f };
	};
}
