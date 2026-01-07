#include "framework/AssetManager.h"
#include "framework/MathUtility.h"
#include "framework/World.h"
#include "framework/BackgroundLayer.h"

namespace ly
{
	BackgroundLayer::BackgroundLayer(World* owningWorld, const sf::Vector2f& minVelocity, const sf::Vector2f& maxVelocity, float sizeMin, float sizeMax, int spriteCount, sf::Color colorTint) :
		Actor(owningWorld),
		mMinVelocity(minVelocity),
		mMaxVelocity(maxVelocity),
		mSizeMin(sizeMin),
		mSizeMax(sizeMax),
		mSpriteCount(spriteCount),
		mColorTint(colorTint),
		mSlowMotionScale(1.f),
		mPaused(false),
		mRandomVisibility(false),
		mUseDepthColor(false)
	{
	}

	BackgroundLayer::BackgroundLayer(World* owningWorld, const List<BackgroundLayerDefinition>& defs, const sf::Vector2f& minVelocity, const sf::Vector2f& maxVelocity, float sizeMin, float sizeMax, int spriteCount, sf::Color colorTint) :
		Actor(owningWorld),
		mMinVelocity(minVelocity),
		mMaxVelocity(maxVelocity),
		mSizeMin(sizeMin),
		mSizeMax(sizeMax),
		mSpriteCount(spriteCount),
		mColorTint(colorTint),
		mSlowMotionScale(1.f),
		mPaused(false),
		mRandomVisibility(true),
		mUseDepthColor(false),
		mDefinitions(defs)
	{
		InitializeSprites();
	}

	void BackgroundLayer::SetDefinitions(const List<BackgroundLayerDefinition>& defs)
	{
		mDefinitions = defs;
		InitializeSprites();
	}

	void BackgroundLayer::Tick(float deltaTime)
	{
		Actor::Tick(deltaTime);

		for (auto& element : mElements)
		{
			if (!element.sprite.has_value())
				continue;

			sf::Vector2f& velocity = element.velocity;

			if (IsSpriteOutScreen(element.sprite.value(), velocity))
			{
				SpawnRandomElement(element);
				if(element.sprite.has_value())
				{
					RandomSpritePosition(element.sprite.value(), false);
				}
			}

			element.sprite.value().setPosition(element.sprite.value().getPosition() + velocity * deltaTime * mSlowMotionScale);

			if(element.hasLight)
			{
				SetLightWorldPosition(element.lightTag, element.sprite.value().getPosition());
			}
		}
	}

	void BackgroundLayer::Render(sf::RenderWindow& window)
	{

		if (GetIsPendingDestroy())
			return;

		RenderLights(window);

		if (mUseDepthColor)
		{
			List<BackgroundElement*> sortedElements;
			for(auto& element : mElements)
			{
				if (element.sprite.has_value())
				{
					sortedElements.push_back(&element);
				}
			}

			std::sort(sortedElements.begin(), sortedElements.end(), [](const BackgroundElement* a, const BackgroundElement* b) {
				return std::abs(a->velocity.y) < std::abs(b->velocity.y);
				});

			for(const auto& element : sortedElements)
			{
				window.draw(element->sprite.value());
			}
		}
		else
		{
			for (const auto& element : mElements)
			{
				if (element.sprite.has_value())
				{
					window.draw(element.sprite.value());
				}
			}
		}
	}

	void BackgroundLayer::SetPaths(const List<std::string>& texturePaths)
	{
		List<BackgroundLayerDefinition> defs;

		for (const std::string& path : texturePaths)
		{
			defs.push_back(BackgroundLayerDefinition(path));
		}
		SetDefinitions(defs);
	}

	void BackgroundLayer::SetColorTint(const sf::Color& colorTint)
	{
		mColorTint = colorTint;

		for (auto& element : mElements)
		{
			if (element.sprite.has_value() && element.sprite.value().getColor().a > 0)
			{
				if (mUseDepthColor)
				{
					float speed = std::abs(element.velocity.y);
					element.sprite.value().setColor(CalculateDepthColor(speed));
				}
				else
				{
					element.sprite.value().setColor(mColorTint);
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

		for (auto& element : mElements)
		{
			float velX = RandRange(mMinVelocity.x, mMaxVelocity.x);
			float velY = RandRange(mMinVelocity.y, mMaxVelocity.y);
			element.velocity = sf::Vector2f{ velX, velY };

			if (mUseDepthColor && element.sprite.has_value() && element.sprite.value().getColor().a > 0)
			{
				float speed = std::abs(velY);
				element.sprite.value().setColor(CalculateDepthColor(speed));
			}
		}
	}

	void BackgroundLayer::SetSizeRange(float sizeMin, float sizeMax)
	{
		mSizeMin = sizeMin;
		mSizeMax = sizeMax;
		for (auto& element : mElements)
		{
			if (element.sprite.has_value())
			{
				float scale = RandRange(mSizeMin, mSizeMax);
				element.sprite.value().setScale({ scale, scale });
			}

			if(element.hasLight && element.sprite.has_value())
			{
				sf::FloatRect globalBounds = element.sprite.value().getGlobalBounds();
				sf::Vector2f newLightSize = sf::Vector2f{ globalBounds.size.x, globalBounds.size.y } *element.lightScaleRatio;
				SetLightSize(element.lightTag, newLightSize);
			}

			if(mUseDepthColor && element.sprite.has_value() && element.sprite.value().getColor().a > 0)
			{
				float speed = std::abs(element.velocity.y);
				element.sprite.value().setColor(CalculateDepthColor(speed));
			}
		}
	}

	void BackgroundLayer::InitializeSprites()
	{
		for (auto& element : mElements)
		{
			if (element.hasLight)
			{
				RemoveLight(element.lightTag);
				element.hasLight = false;
			}
		}

		mElements.clear();

		if (mDefinitions.empty())
			return;

		mElements.resize(mSpriteCount);

		for (auto& element : mElements)
		{
			SpawnRandomElement(element);
			if (element.sprite)
			{
				RandomSpritePosition(*element.sprite, true);
				if (element.hasLight)
				{
					SetLightWorldPosition(element.lightTag, element.sprite->getPosition());
				}
			}
		}
	}

	sf::Color BackgroundLayer::CalculateDepthColor(float speed) const
	{
		float minSpeed = std::abs(mMinVelocity.y);
		float maxSpeed = std::abs(mMaxVelocity.y);

		float normalizedSpeed = (speed - minSpeed) / (maxSpeed - minSpeed);
		normalizedSpeed = std::clamp(normalizedSpeed, 0.f, 1.f);

		sf::Color farColor{ 120, 130, 220, 230 };
		sf::Color nearColor = mColorTint;

		return LerpColor(farColor, nearColor, normalizedSpeed);
	}


	void BackgroundLayer::RandomSpritePosition(sf::Sprite& sprite, bool randomY)
	{
		auto windowsSize = GetWorld()->GetWindowSize();
		auto bounds = sprite.getGlobalBounds();

		float posX = RandRange(0.f, (float)windowsSize.x);
		float posY = randomY ? RandRange(0.f, (float)windowsSize.y) : -(bounds.size.y * 0.5f) - 20.f;

		sprite.setPosition(sf::Vector2f{ posX, posY });
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

		if (velocity.x > 0.f)
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

	const BackgroundLayerDefinition* BackgroundLayer::GetRandomDefinition() const
	{
		if(mDefinitions.empty())
			return nullptr;

		int randIndex = RandRange(0, (int)mDefinitions.size() - 1);
		return &mDefinitions[randIndex];
	}

	void BackgroundLayer::SpawnRandomElement(BackgroundElement& element)
	{
		if(element.hasLight)
		{
			RemoveLight(element.lightTag);
			element.hasLight = false;
		}

		const BackgroundLayerDefinition* def = GetRandomDefinition();
		if(!def)
			return;

		if (def->texturePath.empty())
		{
			LOG("BackgroundLayer::SpawnRandomElement - Empty texture path in definition!");
			return;
		}

		element.texture = AssetManager::GetAssetManager().LoadTexture(def->texturePath);
		
		if(element.texture == nullptr)
		{
			LOG("BackgroundLayer::SpawnRandomElement - Failed to load texture: %s", def->texturePath.c_str());
			return;
		}

		element.sprite.emplace(*element.texture);
		element.sprite.value().setTextureRect(sf::IntRect{ sf::Vector2i{0,0}, (sf::Vector2i)element.texture->getSize() });
		sf::FloatRect bound = element.sprite.value().getLocalBounds();
		element.sprite.value().setOrigin(sf::Vector2f{ bound.size.x / 2.f, bound.size.y / 2.f });

		element.sprite.value().setRotation(sf::degrees(RandRange(0.f, 360.f)));

		float scale = RandRange(mSizeMin, mSizeMax);
		element.sprite.value().setScale({ scale, scale });
		float velX = RandRange(mMinVelocity.x, mMaxVelocity.x);
		float velY = RandRange(mMinVelocity.y, mMaxVelocity.y);
		element.velocity = sf::Vector2f{ velX, velY };

		if (mUseDepthColor)
		{
			float speed = std::abs(velY);
			element.sprite.value().setColor(CalculateDepthColor(speed));
		}
		else
		{
			element.sprite.value().setColor(mColorTint);
		}
		if(mRandomVisibility)
		{
			float randVisibility = RandRange(0.f, 1.f);
			if (randVisibility < 0.66f)
			{
				element.sprite.value().setColor(sf::Color::Transparent);
			}
		}
		if (element.hasLight)
		{
			RemoveLight(element.lightTag);
			element.hasLight = false;
		}
		if (def->hasLight && element.sprite.value().getColor().a > 0)
		{
			sf::FloatRect globalBounds = element.sprite.value().getGlobalBounds();

			sf::Vector2f scaledSize = { globalBounds.size.x, globalBounds.size.y };
			sf::Vector2f finalLightSize = scaledSize * def->lightScaleRatio;

			element.lightScaleRatio = def->lightScaleRatio;

			sf::Vector2f spritePos = element.sprite.value().getPosition();
	
			LOG("Sprite scale set to: %f,%f", element.sprite.value().getGlobalBounds().size.x, element.sprite.value().getGlobalBounds().size.y);
			LOG("light size: x=%f, y=%f", finalLightSize.x, finalLightSize.y);

			element.lightTag = AddLight(
				GameTags::Environment::Planet,
				def->lightDef.shaderPath,
				def->lightDef.color,
				def->lightDef.intensity,
				finalLightSize, 
				sf::Vector2f{ spritePos },
				def->lightDef.shouldStretch,
				def->lightDef.complexTrail,
				def->lightDef.taperAmount,
				def->lightDef.edgeSoftness,
				def->lightDef.shapeRoundness,
				LightSpace::World
			);
		element.hasLight = true;
		}
	}



	void BackgroundLayer::SetPaused(bool paused)
	{
		if (paused == mPaused)
			return;

		mPaused = paused;
		mSlowMotionScale = paused ? 0.3f : 1.f;
	}
}