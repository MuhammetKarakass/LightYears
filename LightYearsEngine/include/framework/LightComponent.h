#pragma once
#include "framework/ActorComponent.h"
#include "framework/Core.h"
#include "engineConfigs/EngineStructs.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

namespace ly
{
	enum class LightSpace : uint8_t
	{
		Local = 0,
		World = 1
	};

	struct LightData
	{
		GameplayTag tag;
		LightSpace lightSpace;
		shared_ptr<sf::Shader> shader;
		sf::Color color;
		float intensity;
		sf::Vector2f size;
		sf::Vector2f offset;
		bool isEnabled;
		bool shouldStretch;
		bool useComplexTrail;
		float currentStretchFactor;
		float currentRotationOffset;
		float taperAmount;
		float edgeSoftness;
		float shapeRoundness;
		LightData() :
			tag{}, lightSpace(LightSpace::Local), color(sf::Color::White), intensity(1.f), size{ 100.f, 100.f },
			offset{ 0.f, 0.f }, isEnabled(true), shouldStretch(false), useComplexTrail{ false },
			currentStretchFactor(1.f), currentRotationOffset(0.f), taperAmount(0.f), edgeSoftness(1.f),
			shapeRoundness(1.f) {
		}
	};

	// GameplayTag-based, auto-indexed additive lights attached to an actor
	// (Tag_0, Tag_1, ...). Ticks to update the stretch/flicker of stretching
	// lights; Actor::Render draws it before the sprite and Actor::GetRenderBounds
	// unions its bounds. Use Actor::GetOrAddComponent<LightComponent>().
	class LightComponent : public ActorComponent
	{
	public:
		explicit LightComponent(Actor& owner);

		void TickComponent(float deltaTime) override;

		void Render(sf::RenderTarget& window);
		// Unions the bounds of every enabled light into `bounds` (which may start
		// empty or hold the owner's sprite bounds).
		void ExpandRenderBounds(std::optional<sf::FloatRect>& bounds) const;

		GameplayTag AddLight(const GameplayTag& tag, const std::string& lightPath, sf::Color color,
			float intensity, sf::Vector2f size, sf::Vector2f offset, bool shouldStretch,
			bool useComplexTrail, float taperAmount, float edgeSoftness, float shapeRoundness = 1.f, LightSpace lightSpace = LightSpace::Local);
		GameplayTag AddLight(const GameplayTag& tag, const PointLightDefinition& def, const sf::Vector2f& offset, LightSpace lightSpace = LightSpace::Local);

		LightData* GetLightData(const GameplayTag& tag);
		bool HasLight(const GameplayTag& tag) const;
		void RemoveLight(const GameplayTag& tag);
		List<GameplayTag> GetLightsByBaseTag(const GameplayTag& baseTag) const;

		void SetLightEnabled(const GameplayTag& tag, bool enabled);
		void SetLightColor(const GameplayTag& tag, const sf::Color& color);
		void SetLightIntensity(const GameplayTag& tag, float intensity);
		void SetLightSize(const GameplayTag& tag, const sf::Vector2f& size);
		void SetLightOffset(const GameplayTag& tag, const sf::Vector2f& offset);
		void SetLightWorldPosition(const GameplayTag& tag, const sf::Vector2f& worldPosition);

		void SetAllLightsEnabled(const GameplayTag& baseTag, bool enabled);
		void SetAllLightsColor(const GameplayTag& baseTag, const sf::Color& color);
		void SetAllLightsIntensity(const GameplayTag& baseTag, float intensity);

	private:
		int GetNextLightIndex(const GameplayTag& baseTag) const;

		Dictionary<GameplayTag, LightData, GameplayTagHash> mLights;
	};
}
