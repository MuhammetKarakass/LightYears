#pragma once
#include "framework/Actor.h"
#include <optional>

namespace ly
{
	struct BackgroundElement
	{
		std::shared_ptr<sf::Texture> texture;
		std::optional<sf::Sprite> sprite;

		sf::Vector2f velocity;

		bool hasLight = false;
		GameplayTag lightTag;
		float lightScaleRatio = 1.f;

	};

	class BackgroundLayer : public Actor
	{
	public:
		BackgroundLayer(World* owningWorld,
			const sf::Vector2f& minVelocity = sf::Vector2f{ 0.f,30.f },
			const sf::Vector2f& maxVelocity = sf::Vector2f{ 0.f , 100.f },
			float sizeMin = 1.f,
			float sizeMax = 2.f,
			int spriteCount = 20,
			sf::Color colorTint = sf::Color{ 180, 180, 220, 245 }
		);

		BackgroundLayer(World* owningWorld,
			const List<BackgroundLayerDefinition>& defs,
			const sf::Vector2f& minVelocity = sf::Vector2f{ 0.f,30.f },
			const sf::Vector2f& maxVelocity = sf::Vector2f{ 0.f , 100.f },
			float sizeMin = 1.f,
			float sizeMax = 2.f,
			int spriteCount = 20,
			sf::Color colorTint = sf::Color{ 180, 180, 220, 245 });

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
		void SetDefinitions(const List<BackgroundLayerDefinition>& defs);
		void RandomSpritePosition(sf::Sprite& sprite, bool randomY);
		bool IsSpriteOutScreen(const sf::Sprite& sprite, sf::Vector2f& velocity) const;
		const BackgroundLayerDefinition* GetRandomDefinition() const;
		void SpawnRandomElement(BackgroundElement& element);

		sf::Color CalculateDepthColor(float speed) const;


		sf::Vector2f mMinVelocity;
		sf::Vector2f mMaxVelocity;
		float mSizeMin;
		float mSizeMax;
		int mSpriteCount;
		sf::Color mColorTint;

		List<BackgroundElement> mElements;
		List<BackgroundLayerDefinition> mDefinitions;

		float mSlowMotionScale;
		bool mPaused;
		bool mRandomVisibility;
		bool mUseDepthColor;
	};
}