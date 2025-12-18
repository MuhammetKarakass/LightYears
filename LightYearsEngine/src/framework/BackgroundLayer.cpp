#include "framework/BackgroundLayer.h"
#include "framework/AssetManager.h"
#include "framework/MathUtility.h"
#include "framework/World.h"

namespace ly
{
	BackgroundLayer::BackgroundLayer(World* owningWorld, const List<std::string>& texturePaths, const sf::Vector2f& minVelocity, const sf::Vector2f& maxVelocity, float sizeMin, float sizeMax, int spriteCount, sf::Color colorTint) :
		Actor(owningWorld),
		mMinVelocity(minVelocity),
		mMaxVelocity(maxVelocity),
		mSizeMin(sizeMin),
		mSizeMax(sizeMax),
		mSpriteCount(spriteCount),
		mColorTint(colorTint),  // Referans renk (180, 180, 200)
		mSlowMotionScale(1.f),
		mVelocities{},
		mPaused(false),
		mRandomVisibility(false),
		mUseDepthColor(false)
	{
		SetPaths(texturePaths);
		SetEnablePhysics(false);
		SetTickWhenPaused(true);
	}

	void BackgroundLayer::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);

		for (size_t i = 0; i < mSprites.size(); ++i)
		{
			sf::Sprite& sprite = mSprites[i];
			sf::Vector2f& velocity = mVelocities[i];

			sprite.setPosition(sprite.getPosition() + velocity * deltaTime * mSlowMotionScale);

			if (IsSpriteOutScreen(sprite , velocity))
			{
				auto newSprite = CreateRandomSprite();

				if (newSprite)
				{
					// ✅ 1. Önce yeni sprite'ı listeye ekle
					mSprites[i] = *newSprite;

					// ✅ 2. Transform uygula
					RandomSpriteTransform(mSprites[i], false);

					// ✅ 3. Yeni velocity
					float velX = RandRange(mMinVelocity.x, mMaxVelocity.x);
					float velY = RandRange(mMinVelocity.y, mMaxVelocity.y);
					mVelocities[i] = sf::Vector2f{ velX, velY };

					// ✅ 4. Renk ataması (random visibility durumuna göre)
					if (mRandomVisibility)
					{
						float randVisibility = RandRange(0.f, 1.f);

						if (randVisibility < 0.33f)
						{
							mSprites[i].setColor(sf::Color::Transparent);
						}
						else
						{
							if (mUseDepthColor)
							{
								float speed = std::abs(velY);
								mSprites[i].setColor(CalculateDepthColor(speed));
							}
							else
							{
								mSprites[i].setColor(mColorTint);
							}
						}
					}
					else
					{
						// ✅ Random visibility kapalı: HER ZAMAN görünür
						if (mUseDepthColor)
						{
							float speed = std::abs(velY);
							mSprites[i].setColor(CalculateDepthColor(speed));
						}
						else
						{
							mSprites[i].setColor(mColorTint);
						}
					}
				}
			}
		}
	}

	void BackgroundLayer::Render(sf::RenderWindow& window)
	{

		if(GetIsPendingDestroy())
			return;

		if (mUseDepthColor)
		{
			List<std::pair<size_t, float>> spriteOrder;

			for(size_t i = 0; i < mSprites.size(); ++i)
			{
				float speed = std::abs(mVelocities[i].y);
				spriteOrder.push_back({ i, speed });
			}
			std::sort(spriteOrder.begin(), spriteOrder.end(), [](const auto& a, const auto& b) {
				return a.second < b.second;
			});

			for (const auto& [index, speed] : spriteOrder)
			{
				window.draw(mSprites[index]);
			}
		}
		else
		{
			for (sf::Sprite& sprite : mSprites)
			{
				window.draw(sprite);
			}
		}
	}

	void BackgroundLayer::SetPaths(const List<std::string>& texturePaths)
	{
		for (const std::string& path : texturePaths)
		{
			auto texture = AssetManager::GetAssetManager().LoadTexture(path);
			if (texture)
			{
				mTextures.push_back(texture);
			}
		}
		InitializeSprites();
	}

	void BackgroundLayer::SetColorTint(const sf::Color& colorTint)
	{
		mColorTint = colorTint;

		for (size_t i = 0; i < mSprites.size(); ++i)
		{
			if (mSprites[i].getColor().a > 0)
			{
				if (mUseDepthColor)
				{
					float speed = std::abs(mVelocities[i].y);
					mSprites[i].setColor(CalculateDepthColor(speed));
				}
				else
				{
					mSprites[i].setColor(mColorTint);
				}
			}
		}
	}

	void BackgroundLayer::SetSpriteCount(int spriteCount)
	{
		mSpriteCount = spriteCount;
		InitializeSprites();
	}

	void BackgroundLayer::SetVelocityRange(const sf::Vector2f& minVelocity, const sf::Vector2f& maxVelocity)
	{
		mMinVelocity = minVelocity;
		mMaxVelocity = maxVelocity;

		for (size_t i = 0; i < mSprites.size(); i++)
		{
			float velX = RandRange(mMinVelocity.x, mMaxVelocity.x);
			float velY = RandRange(mMinVelocity.y, mMaxVelocity.y);
			mVelocities[i] = sf::Vector2f{ velX, velY };

			if (mUseDepthColor && mSprites[i].getColor().a > 0)
			{
				float speed = std::abs(velY);
				mSprites[i].setColor(CalculateDepthColor(speed));
			}
		}
	}

	void BackgroundLayer::SetSizeRange(float sizeMin, float sizeMax)
	{
		mSizeMin = sizeMin;
		mSizeMax = sizeMax;
		for (size_t i = 0; i < mSprites.size(); i++)
		{
			RandomSpriteSize(mSprites[i]);
		}
	}

	void BackgroundLayer::InitializeSprites()
	{
		if (mTextures.empty())
			return;

		mSprites.clear();
		mVelocities.clear();

		for (int i = 0; i < mSpriteCount; i++)
		{
			auto newSprite = CreateRandomSprite();

			if (newSprite)
			{
				RandomSpriteTransform(*newSprite, true);

				float velX = RandRange(mMinVelocity.x, mMaxVelocity.x);
				float velY = RandRange(mMinVelocity.y, mMaxVelocity.y);
				mVelocities.push_back(sf::Vector2f{ velX, velY });

				if (mRandomVisibility)
				{
					float randVisibility = RandRange(0.f, 1.f);

					if (randVisibility < 0.33f)
					{
						newSprite->setColor(sf::Color::Transparent);
					}
					else
					{
						if (mUseDepthColor)
						{
							float speed = std::abs(velY);
							newSprite->setColor(CalculateDepthColor(speed));
						}
						else
						{
							newSprite->setColor(mColorTint);
						}
					}
				}
				else
				{
					if (mUseDepthColor)
					{
						float speed = std::abs(velY);
						newSprite->setColor(CalculateDepthColor(speed));
					}
					else
					{
						newSprite->setColor(mColorTint);
					}
				}

				mSprites.push_back(*newSprite);
			}
		}
	}

	sf::Color BackgroundLayer::CalculateDepthColor(float speed) const
	{
		// ✅ Velocity range'ini normalize et
		float minSpeed = std::abs(mMinVelocity.y);
		float maxSpeed = std::abs(mMaxVelocity.y);

		float normalizedSpeed = (speed - minSpeed) / (maxSpeed - minSpeed);
		normalizedSpeed = std::clamp(normalizedSpeed, 0.f, 1.f);

		sf::Color farColor{ 120, 130, 220, 230 };
		sf::Color nearColor = mColorTint;

		// ✅ Linear interpolation
		return LerpColor(farColor, nearColor, normalizedSpeed);
	}

	std::optional<sf::Sprite> BackgroundLayer::CreateRandomSprite()
	{
		auto texture = GetRandomTexture();

		if (texture)
		{
			sf::Sprite sprite(*texture);

			sprite.setTextureRect(sf::IntRect{ sf::Vector2i{0,0}, (sf::Vector2i)texture->getSize() });
			sf::FloatRect bound = sprite.getGlobalBounds();
			sprite.setOrigin(sf::Vector2f{ bound.size.x / 2.f, bound.size.y / 2.f });

			return sprite;
		}

		return std::nullopt;
	}

	void BackgroundLayer::RandomSpriteTransform(sf::Sprite& sprite, bool randomY)
	{
		RandomSpriteSize(sprite);
		RandomSpritePosition(sprite, randomY);
		RandomSpriteRotation(sprite);
	}

	void BackgroundLayer::RandomSpritePosition(sf::Sprite& sprite, bool randomY)
	{
		auto windowsSize = GetWorld()->GetWindowSize();
		auto bounds = sprite.getGlobalBounds();

		float posX = RandRange(0.f, (float)windowsSize.x);
		float posY = randomY ? RandRange(0.f, (float)windowsSize.y) : -(bounds.size.y * 0.5f) - 20.f;

		sprite.setPosition(sf::Vector2f{ posX, posY });
	}

	void BackgroundLayer::RandomSpriteRotation(sf::Sprite& sprite)
	{
		sprite.setRotation(sf::degrees(RandRange(0.f, 360.f)));
	}

	void BackgroundLayer::RandomSpriteSize(sf::Sprite& sprite)
	{
		float scale = RandRange(mSizeMin, mSizeMax);
		sprite.setScale({scale, scale});
	}

	bool BackgroundLayer::IsSpriteOutScreen(const sf::Sprite& sprite, sf::Vector2f& velocity) const
	{
		auto windowSize = GetWorld()->GetWindowSize();
		auto bounds = sprite.getGlobalBounds();
		auto position = sprite.getPosition();

		if (velocity.y > 0.f)
		{
			if (position.y > windowSize.y + bounds.size.y)
				return true;
		}

		else if (velocity.y < 0.f)
		{
			if (position.y < -bounds.size.y)
				return true;
		}

		if(velocity.x > 0.f)
		{
			if (position.x > windowSize.x + bounds.size.x)
				return true;
		}
		else if (velocity.x < 0.f)
		{
			if (position.x < -bounds.size.x)
				return true;
		}

		return false;
	}

	std::shared_ptr<sf::Texture> BackgroundLayer::GetRandomTexture() const
	{
		if (mTextures.empty()) return nullptr;

		int randIndex = RandRange(0, (int)mTextures.size() - 1);
		return mTextures[randIndex];
	}

	void BackgroundLayer::SetPaused(bool paused)
	{
		if (paused == mPaused)
			return;

		mPaused = paused;
		mSlowMotionScale = paused ? 0.3f : 1.f;
	}
}