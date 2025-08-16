#include <framework/Actor.h>
#include <framework/Core.h>

namespace ly
{
	class World;
	class SpaceShip : public Actor
	{
	public:
		SpaceShip(World* owningWorld, std::string texturePath = "");
		virtual void Tick(float deltaTime) override;
		sf::Vector2f GetVelocity() const { return mVelocity; }
		void SetVelocity(const sf::Vector2f& velocity);
		sf::Vector2f GetVelocity() { return mVelocity; }

	private:
		sf::Vector2f mVelocity;
	};
}