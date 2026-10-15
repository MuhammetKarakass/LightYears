#include "framework/SpriteComponent.h"
#include "framework/Actor.h"
#include "framework/AssetManager.h"

namespace ly
{
	SpriteComponent::SpriteComponent(Actor& owner, const std::string& texturePath) : ActorComponent{ owner }
	{
		SetTexture(texturePath);
	}

	bool SpriteComponent::SetTexture(const std::string& texturePath)
	{
		if (texturePath.empty()) return false;
		auto texture = AssetManager::GetAssetManager().LoadTexture(texturePath);
		if (!texture) return false;
		mTexture = std::move(texture);

		mSprite.emplace(*mTexture);
		mTexturePath = texturePath;
		SyncLocation();
		SyncRotation();

		int width = mTexture->getSize().x;
		int height = mTexture->getSize().y;
		mSprite->setTextureRect(sf::IntRect{ sf::Vector2i{}, sf::Vector2i{width,height} });

		CenterPivot();
		mSprite->setScale(mScale);
		return true;
	}

	void SpriteComponent::SetTextureRepeated(bool repeated)
	{
		if (mTexture)
		{
			mTexture->setRepeated(repeated);
		}
	}

	void SpriteComponent::SetScale(sf::Vector2f scale)
	{
		mScale = scale;
		if (mSprite) mSprite->setScale(scale);
	}

	void SpriteComponent::CenterPivot()
	{
		if (!mSprite)
		{
			return;
		}

		const sf::FloatRect rectBounds = mSprite->getGlobalBounds();
		mSprite->setOrigin(sf::Vector2f{ rectBounds.size.x / 2.f, rectBounds.size.y / 2.f });
	}

	void SpriteComponent::SetVisibility(bool visible)
	{
		if (mSprite) mSprite->setColor(visible ? sf::Color::White : sf::Color::Transparent);
	}

	sf::FloatRect SpriteComponent::GetGlobalBounds() const
	{
		return mSprite ? mSprite->getGlobalBounds() : sf::FloatRect{};
	}

	void SpriteComponent::Render(sf::RenderTarget& window) const
	{
		if (mSprite.has_value())
			window.draw(mSprite.value());
	}

	void SpriteComponent::SyncLocation()
	{
		if (mSprite)
		{
			mSprite->setPosition(GetOwner().GetActorLocation());
		}
	}

	void SpriteComponent::SyncRotation()
	{
		if (mSprite)
		{
			mSprite->setRotation(sf::degrees(GetOwner().GetActorRotation()));
		}
	}
}
