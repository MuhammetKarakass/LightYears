#include "framework/EngineContext.h"
#include "framework/EngineAssets.h"
#include "framework/LightComponent.h"
#include "framework/Actor.h"
#include "framework/SpriteComponent.h"
#include "framework/AssetManager.h"
#include "framework/MathUtility.h"
#include "framework/PerfMonitor.h"
#include "framework/reflection/Reflection.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	LightComponent::LightComponent(Actor& owner) : ActorComponent(owner)
	{
		mCanEverTick = true;
	}

	void LightComponent::TickComponent(float deltaTime)
	{
		Actor& owner = GetOwner();
		for (auto& pair : mLights)
		{
			LightData& light = pair.second;
			if (!light.isEnabled || !light.shouldStretch) continue;

			sf::Vector2f velocity = owner.GetVelocity();

			sf::Vector2f forwardDir = owner.GetActorForwardDirection();
			float forwardSpeed = (velocity.x * forwardDir.x) + (velocity.y * forwardDir.y);

			float targetStretch = 1.f + (forwardSpeed / 300.f);
			if (targetStretch > 4.0f) targetStretch = 4.0f;

			light.currentStretchFactor = Lerp(light.currentStretchFactor, targetStretch, deltaTime * 2.f);

			sf::Vector2f rightDir = owner.GetActorRightDirection();
			float sideSpeed = (velocity.x * rightDir.x) + (velocity.y * rightDir.y);

			float targetRotation = sideSpeed * 0.1f;
			if (targetRotation > 30.f) targetRotation = 30.f;
			if (targetRotation < -30.f) targetRotation = -30.f;

			light.currentRotationOffset = Lerp(light.currentRotationOffset, targetRotation, deltaTime * 5.f);

			float flicker = 1.0f + RandRange(-0.02f, 0.02f);
			light.currentStretchFactor *= flicker;
		}
	}

	int LightComponent::GetNextLightIndex(const GameplayTag& baseTag) const
	{
		int maxIndex = -1;

		for (const auto& pair : mLights)
		{
			if (pair.first.MatchesBase(baseTag))
			{
				const std::string& tagName = pair.first.ToString();
				size_t pos = tagName.rfind('_');
				if (pos != std::string::npos)
				{
					std::string suffix = tagName.substr(pos + 1);
					bool isNumber = !suffix.empty();
					for (char c : suffix)
					{
						if (!std::isdigit(static_cast<unsigned char>(c)))
						{
							isNumber = false;
							break;
						}
					}
					if (isNumber)
					{
						int index = std::stoi(suffix);
						if (index > maxIndex) maxIndex = index;
					}
				}
			}
		}

		return maxIndex + 1;
	}

	void LightComponent::SetLightEnabled(const GameplayTag& tag, bool enabled)
	{
		if (LightData* light = GetLightData(tag))
		{
			light->isEnabled = enabled;
		}
	}

	void LightComponent::SetLightColor(const GameplayTag& tag, const sf::Color& color)
	{
		if (LightData* light = GetLightData(tag))
		{
			light->color = color;
		}
	}

	void LightComponent::SetLightIntensity(const GameplayTag& tag, float intensity)
	{
		if (LightData* light = GetLightData(tag))
		{
			light->intensity = intensity;
		}
	}

	void LightComponent::SetLightSize(const GameplayTag& tag, const sf::Vector2f& size)
	{
		if (LightData* light = GetLightData(tag))
		{
			light->size = size;
		}
	}

	void LightComponent::SetLightOffset(const GameplayTag& tag, const sf::Vector2f& offset)
	{
		if (LightData* light = GetLightData(tag))
		{
			if (light->lightSpace == LightSpace::Local)
				light->offset = offset;
		}
	}

	void LightComponent::SetLightWorldPosition(const GameplayTag& tag, const sf::Vector2f& worldPosition)
	{
		if (LightData* light = GetLightData(tag))
		{
			if (light->lightSpace == LightSpace::World)
			{
				light->offset = worldPosition;
			}
		}
	}

	GameplayTag LightComponent::AddLight(const GameplayTag& tag, const std::string& lightPath, sf::Color color,
		float intensity, sf::Vector2f size, sf::Vector2f offset, bool shouldStretch,
		bool useComplexTrail, float taperAmount, float edgeSoftness, float shapeRoundness, LightSpace lightSpace)
	{
		GameplayTag finalTag = tag.WithIndex(GetNextLightIndex(tag));

		LightData lightData;
		lightData.tag = finalTag;
		lightData.lightSpace = lightSpace;
		// Definitions may be constructed before a game installs its defaults.
		const auto& shaderPath = lightPath.empty() ? GetOwner().GetEngineContext().Defaults().pointLightShader : lightPath;
		if (!shaderPath.empty()) lightData.shader = GetOwner().GetEngineContext().Assets().LoadShader(shaderPath);
		lightData.color = color;
		lightData.intensity = intensity;
		lightData.size = size;
		lightData.offset = offset;
		lightData.shouldStretch = shouldStretch;
		lightData.isEnabled = true;
		lightData.useComplexTrail = useComplexTrail;
		lightData.taperAmount = taperAmount;
		lightData.edgeSoftness = edgeSoftness;
		lightData.shapeRoundness = shapeRoundness;

		mLights[finalTag] = lightData;
		return finalTag;
	}

	GameplayTag LightComponent::AddLight(const GameplayTag& tag, const PointLightDefinition& def, const sf::Vector2f& offset, LightSpace lightSpace)
	{
		return AddLight(tag, def.shaderPath, def.color, def.intensity, def.size, offset,
			def.shouldStretch, def.complexTrail, def.taperAmount, def.edgeSoftness, def.shapeRoundness, lightSpace);
	}

	LightData* LightComponent::GetLightData(const GameplayTag& tag)
	{
		auto it = mLights.find(tag);
		if (it != mLights.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	bool LightComponent::HasLight(const GameplayTag& tag) const
	{
		return mLights.find(tag) != mLights.end();
	}

	void LightComponent::RemoveLight(const GameplayTag& tag)
	{
		mLights.erase(tag);
	}

	List<GameplayTag> LightComponent::GetLightsByBaseTag(const GameplayTag& baseTag) const
	{
		List<GameplayTag> result;
		for (const auto& pair : mLights)
		{
			if (pair.first.MatchesBase(baseTag))
			{
				result.push_back(pair.first);
			}
		}
		return result;
	}

	void LightComponent::SetAllLightsEnabled(const GameplayTag& baseTag, bool enabled)
	{
		for (auto& pair : mLights)
		{
			if (pair.first.MatchesBase(baseTag))
			{
				pair.second.isEnabled = enabled;
			}
		}
	}

	void LightComponent::SetAllLightsColor(const GameplayTag& baseTag, const sf::Color& color)
	{
		for (auto& pair : mLights)
		{
			if (pair.first.MatchesBase(baseTag))
			{
				pair.second.color = color;
			}
		}
	}

	void LightComponent::SetAllLightsIntensity(const GameplayTag& baseTag, float intensity)
	{
		for (auto& pair : mLights)
		{
			if (pair.first.MatchesBase(baseTag))
			{
				pair.second.intensity = intensity;
			}
		}
	}

	void LightComponent::Render(sf::RenderTarget& window)
	{
		if (!IsActive()) return;

		// Allow disabling lights for debugging GPU/driver stalls
		if (ly::perf::g_disableLights.load(std::memory_order_relaxed))
		{
			return;
		}

		// Early exit if no lights
		if (mLights.empty()) return;

		Actor& owner = GetOwner();
		const SpriteComponent* ownerSprite = owner.FindComponent<SpriteComponent>();
		for (auto& pair : mLights)
		{
			LightData& light = pair.second;

			if (!light.isEnabled || !light.shader) continue;

			sf::RenderStates lightStates;
			lightStates.shader = light.shader.get();
			lightStates.blendMode = sf::BlendAdd;

			sf::Vector2f finalSize = light.size;

			if (light.shouldStretch)
			{
				finalSize.y *= light.currentStretchFactor;
			}

			sf::RectangleShape lightShape(finalSize);
			if (light.lightSpace == LightSpace::Local)
			{
				lightShape.setOrigin({ finalSize.x / 2.f, 0.f });
			}
			if (light.lightSpace == LightSpace::World)
			{
				lightShape.setOrigin({ finalSize.x / 2.f, finalSize.y / 2.f });
			}

			if (light.lightSpace == LightSpace::World)
			{
				lightShape.setPosition(light.offset);
			}

			else
			{
				sf::Vector2f worldOffset = owner.TransformLocalToWorld(light.offset);
				lightShape.setPosition(worldOffset + owner.GetActorLocation());
			}

			float finalRotation = owner.GetActorRotation();

			if (light.shouldStretch)
			{
				finalRotation += light.currentRotationOffset;
			}

			if (light.lightSpace == LightSpace::Local)
			{
				lightShape.setRotation(sf::degrees(finalRotation));
			}

			// Use the actor's texture if available, otherwise use default texture
			if (ownerSprite && ownerSprite->HasSprite())
			{
				lightShape.setTexture(&ownerSprite->GetSprite()->getTexture());
			}
			else
			{
				lightShape.setTexture(GetOwner().GetEngineContext().Assets().GetDefaultTexture().get());
			}

			// Set shader uniforms
			light.shader->setUniform("lightColor", sf::Glsl::Vec3{ light.color.r / 255.f, light.color.g / 255.f, light.color.b / 255.f });
			light.shader->setUniform("lightIntensity", light.intensity);
			light.shader->setUniform("u_stretch", light.currentStretchFactor);
			light.shader->setUniform("u_is_trail", light.useComplexTrail ? 1.f : 0.f);
			light.shader->setUniform("u_taper", light.taperAmount);
			light.shader->setUniform("u_edge_softness", light.edgeSoftness);
			light.shader->setUniform("u_roundness", light.shapeRoundness);

			window.draw(lightShape, lightStates);
		}
	}

	void LightComponent::ExpandRenderBounds(std::optional<sf::FloatRect>& result) const
	{
		if (!IsActive()) return;

		const Actor& owner = GetOwner();
		for (const auto& [tag, light] : mLights)
		{
			(void)tag;
			if (!light.isEnabled || !light.shader)
			{
				continue;
			}

			sf::Vector2f size = light.size;
			if (light.shouldStretch)
			{
				size.y *= light.currentStretchFactor;
			}
			const float extent = 0.5f * std::sqrt(size.x * size.x + size.y * size.y);
			const sf::Vector2f center = light.lightSpace == LightSpace::World
				? light.offset
				: owner.TransformLocalToWorld(light.offset) + owner.GetActorLocation();
			const sf::FloatRect lightBounds{
				{ center.x - extent, center.y - extent },
				{ extent * 2.f, extent * 2.f }
			};

			if (!result)
			{
				result = lightBounds;
				continue;
			}

			const float left = std::min(result->position.x, lightBounds.position.x);
			const float top = std::min(result->position.y, lightBounds.position.y);
			const float right = std::max(
				result->position.x + result->size.x,
				lightBounds.position.x + lightBounds.size.x
			);
			const float bottom = std::max(
				result->position.y + result->size.y,
				lightBounds.position.y + lightBounds.size.y
			);
			*result = { { left, top }, { right - left, bottom - top } };
		}
	}

	// Light operations need a tag; Phase 1 reflects scalar object properties only.
	VEGA_REFLECT(LightComponent, ActorComponent)
	{
		VEGA_ADDABLE();
	}
}
