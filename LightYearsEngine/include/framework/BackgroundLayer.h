#pragma once
#include "framework/Actor.h"
#include <optional>

namespace ly
{
	class BackgroundLayer : public Actor
	{
	public:
		BackgroundLayer(World* owningWorld,
			const List<std::string>& texturePaths = {},
			const sf::Vector2f& minVelocity = sf::Vector2f{ 0.f,30.f },
			const sf::Vector2f& maxVelocity = sf::Vector2f{ 0.f , 100.f },
			float sizeMin = 1.f,
			float sizeMax = 2.f,
			int spriteCount = 20,
			sf::Color colorTint = sf::Color{ 180, 180, 220, 245 }  // ✅ Referans renk (normal/orta)
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;

		void SetPaths(const List<std::string>& texturePaths);
		void SetColorTint(const sf::Color& colorTint);
		void SetSpriteCount(int spriteCount);
		void SetVelocityRange(const sf::Vector2f& minVelocity, const sf::Vector2f& maxVelocity);
		void SetSizeRange(float sizeMin, float sizeMax);
		void SetRandomVisibility(bool randomVisibility) { mRandomVisibility = randomVisibility; }
		bool GetRandomVisibility() const { return mRandomVisibility; }

		void SetPaused(bool paused);
		bool IsPaused() const { return mPaused; }

		void SetUseDepthColor(bool useDepth) { mUseDepthColor = useDepth; }
		bool GetUseDepthColor() const { return mUseDepthColor; }

	private:
		void InitializeSprites();
		std::optional<sf::Sprite> CreateRandomSprite();
		void RandomSpriteTransform(sf::Sprite& sprite, bool randomY = false);
		void RandomSpritePosition(sf::Sprite& sprite, bool randomY);
		void RandomSpriteRotation(sf::Sprite& sprite);
		void RandomSpriteSize(sf::Sprite& sprite);
		bool IsSpriteOutScreen(const sf::Sprite& sprite, sf::Vector2f& velocity) const;

		sf::Color CalculateDepthColor(float speed) const;

		std::shared_ptr<sf::Texture> GetRandomTexture() const;

		sf::Vector2f mMinVelocity;
		sf::Vector2f mMaxVelocity;
		float mSizeMin;
		float mSizeMax;
		int mSpriteCount;
		sf::Color mColorTint;

		List<std::shared_ptr<sf::Texture>> mTextures;
		List<sf::Sprite> mSprites;
		List<sf::Vector2f> mVelocities;

		float mSlowMotionScale;
		bool mPaused;
		bool mRandomVisibility;
		bool mUseDepthColor;
	};
}