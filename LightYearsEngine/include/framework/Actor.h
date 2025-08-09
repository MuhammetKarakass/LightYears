#pragma once
#include "framework/Object.h"
#include "framework/Core.h"
#include <SFML/Graphics.hpp>
#include <optional>

namespace ly
{
	class World;
	class Actor : public Object
	{
	public:
        Actor(World* OwningWorld, const std::string& TexturePath = "");
		virtual ~Actor();

		void BeginPlayInternal();
		virtual void BeginPlay();
		void TickInternal(float deltaTime);
		virtual void Tick(float deltaTime);
		void Render(sf::RenderWindow& window);
		void SetTexture(const std::string& texturePath);
		void SetActorLocation(const sf::Vector2f& newLoc);
		void SetActorRotation(float newRotation);
		void AddActorLocationOffset(const sf::Vector2f& offset);
		void AddActorRotationOffset(float offset);
		void CenterPivot();
		sf::Vector2f GetActorLocation() const;
		float GetActorRotation() const;
		sf::Vector2f GetActorForwardDirection();
		sf::Vector2f GetActorRightDirection();


	private:
		World* mOwningWorld;
		bool mBeganPlay;
		std::optional<sf::Sprite> mSprite;
		shared_ptr<sf::Texture> mTexture;

	};
}