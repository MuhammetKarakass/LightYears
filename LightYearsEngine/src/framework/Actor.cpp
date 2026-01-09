#include "framework/Actor.h"
#include "framework/AssetManager.h"
#include "framework/World.h"
#include "framework/MathUtility.h"
#include <framework/PhysicsSystem.h>
#include "framework/PerfMonitor.h"

namespace ly
{
	Actor::Actor(World* OwningWorld, const std::string& TexturePath)
		: mOwningWorld{ OwningWorld },
		mBeganPlay{ false },
		mSprite{},
		mTexture{},
		mVelocity{},
		mPhysicsBodyId{},
		mPhysicsEnabled{ false },
		mCollisionLayer{ CollisionLayer::None },
		mCollisionMask{ CollisionLayer::None },
		mCanCollide{ false },
		mTickWhenPaused{ false }
	{
		SetTexture(TexturePath);
	}
	
	Actor::~Actor()
	{
	
	}
	
	void Actor::BeginPlayInternal()
	{
		if (!mBeganPlay)  
		{
			mBeganPlay = true;    
			BeginPlay();      
		}
	}
	
	void Actor::BeginPlay()
	{
		
	}

	void Actor::TickInternal(float deltaTime)
	{
		if (mBeganPlay && !GetIsPendingDestroy())
		{
			Tick(deltaTime);
		}
	}

	void Actor::Tick(float deltaTime)
	{
		for (auto& pair : mLightShaders)
		{
			LightData& light = pair.second;
			if (!light.isEnabled || !light.shouldStretch) continue;

			sf::Vector2f velocity = GetVelocity();

			sf::Vector2f forwardDir = GetActorForwardDirection();
			float forwardSpeed = (velocity.x * forwardDir.x) + (velocity.y * forwardDir.y);

			float targetStretch = 1.f + (forwardSpeed / 300.f);
			if (targetStretch > 4.0f) targetStretch = 4.0f;

			light.currentStretchFactor = Lerp(light.currentStretchFactor, targetStretch, deltaTime * 2.f);

			sf::Vector2f rightDir = GetActorRightDirection();
			float sideSpeed = (velocity.x * rightDir.x) + (velocity.y * rightDir.y);

			float targetRotation = sideSpeed * 0.1f;
			if (targetRotation > 30.f) targetRotation = 30.f;
			if (targetRotation < -30.f) targetRotation = -30.f;

			light.currentRotationOffset = Lerp(light.currentRotationOffset, targetRotation, deltaTime * 5.f);

			float flicker = 1.0f + ((rand() % 5) - 3) / 100.0f;
			light.currentStretchFactor *= flicker;
		}
	}
	
	void Actor::Destroy()
	{
		UnInitializePhysics();
		onActorDestroyed.Broadcast(this);
		Object::Destroy();
	}

	// =====================================================
	// LIGHT SYSTEM - GameplayTag Based (Auto-Indexed)
	// =====================================================

	int Actor::GetNextLightIndex(const GameplayTag& baseTag) const
	{
		int maxIndex = -1;
		
		for (const auto& pair : mLightShaders)
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

	void Actor::SetLightEnabled(const GameplayTag& tag, bool enabled)
	{
		if (LightData* light = GetLightData(tag))
		{
			light->isEnabled = enabled;
		}
	}

	void Actor::SetLightColor(const GameplayTag& tag, const sf::Color& color)
	{
		if (LightData* light = GetLightData(tag))
		{
			light->color = color;
		}
	}

	void Actor::SetLightIntensity(const GameplayTag& tag, float intensity)
	{
		if (LightData* light = GetLightData(tag))
		{
			light->intensity = intensity;
		}
	}

	void Actor::SetLightSize(const GameplayTag& tag, const sf::Vector2f& size)
	{
		if (LightData* light = GetLightData(tag))
		{
			light->size = size;
		}
	}

	void Actor::SetLightOffset(const GameplayTag& tag, const sf::Vector2f& offset)
	{
		if (LightData* light = GetLightData(tag))
		{
			if (light->lightSpace == LightSpace::Local)
			light->offset = offset;
		}
	}

	void Actor::SetLightWorldPosition(const GameplayTag& tag, const sf::Vector2f& worldPosition)
	{
		if (LightData* light = GetLightData(tag))
		{
			if (light->lightSpace == LightSpace::World)
			{
				light->offset = worldPosition;
			}
		}
	}

	GameplayTag Actor::AddLight(const GameplayTag& tag, const std::string& lightPath, sf::Color color, 
		float intensity, sf::Vector2f size, sf::Vector2f offset, bool shouldStretch,
		bool useComplexTrail, float taperAmount, float edgeSoftness, float shapeRoundness, LightSpace lightSpace)
	{
		GameplayTag finalTag = tag.WithIndex(GetNextLightIndex(tag));
		
		LightData lightData;
		lightData.tag = finalTag;
		lightData.lightSpace = lightSpace;
		lightData.shader = AssetManager::GetAssetManager().LoadShader(lightPath);
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

		mLightShaders[finalTag] = lightData;		
		return finalTag;
	}

	GameplayTag Actor::AddLight(const GameplayTag& tag, const PointLightDefinition& def, const sf::Vector2f& offset, LightSpace lightSpace)
	{
		return AddLight(tag, def.shaderPath, def.color, def.intensity, def.size, offset,
			def.shouldStretch, def.complexTrail, def.taperAmount, def.edgeSoftness, def.shapeRoundness,lightSpace);
	}

	LightData* Actor::GetLightData(const GameplayTag& tag)
	{
		auto it = mLightShaders.find(tag);
		if (it != mLightShaders.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	bool Actor::HasLight(const GameplayTag& tag) const
	{
		return mLightShaders.find(tag) != mLightShaders.end();
	}

	void Actor::RemoveLight(const GameplayTag& tag)
	{
		mLightShaders.erase(tag);
	}

	List<GameplayTag> Actor::GetLightsByBaseTag(const GameplayTag& baseTag) const
	{
		List<GameplayTag> result;
		for (const auto& pair : mLightShaders)
		{
			if (pair.first.MatchesBase(baseTag))
			{
				result.push_back(pair.first);
			}
		}
		return result;
	}

	void Actor::SetAllLightsEnabled(const GameplayTag& baseTag, bool enabled)
	{
		for (auto& pair : mLightShaders)
		{
			if (pair.first.MatchesBase(baseTag))
			{
				pair.second.isEnabled = enabled;
			}
		}
	}

	void Actor::SetAllLightsColor(const GameplayTag& baseTag, const sf::Color& color)
	{
		for (auto& pair : mLightShaders)
		{
			if (pair.first.MatchesBase(baseTag))
			{
				pair.second.color = color;
			}
		}
	}

	void Actor::SetAllLightsIntensity(const GameplayTag& baseTag, float intensity)
	{
		for (auto& pair : mLightShaders)
		{
			if (pair.first.MatchesBase(baseTag))
			{
				pair.second.intensity = intensity;
			}
		}
	}

	void Actor::Render(sf::RenderWindow& window)
	{
		if (GetIsPendingDestroy()) return;
		RenderLights(window);

		if(mSprite.has_value())
			window.draw(mSprite.value());
	}

	void Actor::RenderLights(sf::RenderWindow& window)
	{
		// Allow disabling lights for debugging GPU/driver stalls
		if (ly::perf::g_disableLights.load(std::memory_order_relaxed))
		{
			return;
		}

		// Early exit if no lights
		if (mLightShaders.empty()) return;

		for (auto& pair : mLightShaders)
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
			if(light.lightSpace == LightSpace::Local)
			{
				lightShape.setOrigin({ finalSize.x /2.f,0.f });
			}
			if(light.lightSpace == LightSpace::World)
			{
				lightShape.setOrigin({ finalSize.x /2.f, finalSize.y /2.f });
			}

			if(light.lightSpace == LightSpace::World)
			{
				lightShape.setPosition(light.offset); 
			}

			else 
			{
				sf::Vector2f worldOffset = TransformLocalToWorld(light.offset);
				lightShape.setPosition(worldOffset + GetActorLocation());
			}

			float finalRotation = GetActorRotation();

			if (light.shouldStretch)
			{
				finalRotation += light.currentRotationOffset;
			}

			if (light.lightSpace == LightSpace::Local)
			{
				lightShape.setRotation(sf::degrees(finalRotation));
			}

			// Use texture if available, otherwise use default texture
			if (mTexture)
			{
				lightShape.setTexture(mTexture.get());
			}
			else
			{
				lightShape.setTexture(AssetManager::GetAssetManager().GetDefaultTexture().get());
			}

			// Set shader uniforms
			light.shader->setUniform("lightColor", sf::Glsl::Vec3{ light.color.r /255.f, light.color.g /255.f, light.color.b /255.f });
			light.shader->setUniform("lightIntensity", light.intensity);
			light.shader->setUniform("u_stretch", light.currentStretchFactor);
			light.shader->setUniform("u_is_trail", light.useComplexTrail ?1.f :0.f);
			light.shader->setUniform("u_taper", light.taperAmount);
			light.shader->setUniform("u_edge_softness", light.edgeSoftness);
			light.shader->setUniform("u_roundness", light.shapeRoundness);

			window.draw(lightShape, lightStates);
		}
	}
	
	bool Actor::IsActorOutOfWindow(float allowance) const
	{
		float windowWidth = GetWindowSize().x;   
		float windowHeight = GetWindowSize().y;  

		float width = GetActorGlobalBounds().size.x; 
		float height = GetActorGlobalBounds().size.y;  

		sf::Vector2f actorPosition = GetActorLocation();  

		if(actorPosition.x < -width-allowance-10.f)
			return true;

		if(actorPosition.x > windowWidth + width+allowance+10.f)
			return true;

		if (actorPosition.y < -height-allowance-10.f)
			return true;

		if (actorPosition.y > windowHeight + height+allowance+10.f)
			return true;

		return false;  
	}
	
	void Actor::SetEnablePhysics(bool enable)
	{
		mPhysicsEnabled = enable;

		if (mPhysicsEnabled)
		{
			InitializePhysics();    
		}
		else
		{
			UnInitializePhysics();  
		}
	}
	
	void Actor::InitializePhysics()
	{
		if (!mPhysicsBodyId)  
		{
			mPhysicsBodyId = PhysicsSystem::Get().AddListener(this);
		}
	}
	
	void Actor::UnInitializePhysics()
	{
		if(mPhysicsBodyId)
		{
			PhysicsSystem::Get().RemoveListener(*mPhysicsBodyId);
			mPhysicsBodyId.reset();  
		}
	}

	void Actor::UpdatePhysicsTransform()
	{
		if (mPhysicsBodyId)
		{
			float physicsRate = PhysicsSystem::Get().GetPhysicsRate();
			sf::Vector2f actorLocation = GetActorLocation();
			float actorRotation = GetActorRotation();
			
			b2Vec2 position{ actorLocation.x * physicsRate, actorLocation.y * physicsRate };
			b2Rot rotation = b2MakeRot(DegreesToRadians(actorRotation));
			
			b2Body_SetTransform(*mPhysicsBodyId, position, rotation);
		}
	}

	bool Actor::CanCollideWith(const Actor* other) const
	{
		if (other == nullptr) return false;
		return HasCollisionLayer(mCollisionMask, other->GetCollisionLayer());
	}

	void Actor::OnActorBeginOverlap(Actor* otherActor)
	{
		if (!otherActor) return;
		
		if (!CanCollideWith(otherActor) || !otherActor->CanCollideWith(this))
		{
			return; 
		}

		mCanCollide = (CanCollideWith(otherActor) || otherActor->CanCollideWith(this));
	}

	void Actor::OnActorEndOverlap(Actor* otherActor)
	{
		if (!otherActor) return;
		
		if (!CanCollideWith(otherActor) || !otherActor->CanCollideWith(this))
		{
			return; 
		}
	}

	void Actor::ApplyDamage(float amt)
	{
		
	}

	sf::FloatRect Actor::GetActorGlobalBounds() const
	{
		return mSprite.value().getGlobalBounds();
	}
	
	void Actor::SetTexture(const std::string& texturePath)
	{
		if (texturePath.empty()) return;
		mTexture = AssetManager::GetAssetManager().LoadTexture(texturePath);
		if (!mTexture) return;  

		mSprite.emplace(*mTexture);

		int width = mTexture->getSize().x;
		int height = mTexture->getSize().y;
		mSprite.value().setTextureRect(sf::IntRect{ sf::Vector2i{}, sf::Vector2i{width,height} });
		
		CenterPivot();  
	}

	sf::Vector2u Actor::GetWindowSize() const
	{
		return mOwningWorld->GetWindowSize();
	}

	void Actor::SetActorLocation(const sf::Vector2f& newLoc)
	{
		mSprite.value().setPosition(newLoc); 
		UpdatePhysicsTransform(); 
	}
	
	void Actor::SetActorRotation(float newRotation)
	{
		mSprite.value().setRotation(sf::degrees(newRotation));  
		UpdatePhysicsTransform();  
	}

	void Actor::SetTextureRepeated(bool repeated)
	{
		if (mTexture)
		{
			mTexture->setRepeated(repeated);
		}
	}
	
	void Actor::AddActorLocationOffset(const sf::Vector2f& offset)
	{
		SetActorLocation(GetActorLocation() + offset);  
	}
	
	void Actor::AddActorRotationOffset(float offset)
	{
		SetActorRotation(GetActorRotation() + offset);  
	}
	
	void Actor::CenterPivot()
	{
		sf::FloatRect rectBounds = mSprite.value().getGlobalBounds();  
		mSprite.value().setOrigin(sf::Vector2f{rectBounds.size.x/2.f, rectBounds.size.y/2.f});
	}
	
	sf::Vector2f Actor::GetActorLocation() const
	{
		if(mSprite.has_value())
		return mSprite.value().getPosition();  

		return sf::Vector2f{GetWindowSize().x/2.f, GetWindowSize().y/2.f};
	}
	
	float Actor::GetActorRotation() const
	{
		if(mSprite.has_value())
		return mSprite.value().getRotation().asDegrees(); 

		return 0.f;
	}
	
	sf::Vector2f Actor::GetActorForwardDirection() const
	{
		return RotationToVector(GetActorRotation() - 90.f); 
	}
	
	sf::Vector2f Actor::GetActorRightDirection() const
	{
		return RotationToVector(GetActorRotation());  
	}
	
	sf::Vector2f Actor::TransformLocalToWorld(const sf::Vector2f& localOffset) const
	{
		sf::Vector2f right = GetActorRightDirection();
		sf::Vector2f forward = GetActorForwardDirection();
		
		return localOffset.x * right + localOffset.y * forward;
	}
	
	void Actor::AddActorLocalLocationOffset(const sf::Vector2f& localOffset)
	{
		sf::Vector2f worldOffset = TransformLocalToWorld(localOffset);
		AddActorLocationOffset(worldOffset);
	}
	
	sf::Vector2f Actor::GetActorLocalLocation(const sf::Vector2f& worldLocation) const
	{
		sf::Vector2f deltaWorld = worldLocation - GetActorLocation();
		float rotation = GetActorRotation();
		float radians = DegreesToRadians(rotation - 90.f);
		
		float cosR = std::cos(-radians);
		float sinR = std::sin(-radians);
		
		return sf::Vector2f{
			deltaWorld.x * cosR - deltaWorld.y * sinR,
			deltaWorld.x * sinR + deltaWorld.y * cosR
		};
	}

	void Actor::SetCollisionRadius(float radius)
	{
		if (!mPhysicsBodyId) return;
		PhysicsSystem::Get().SetCollisionRadius(*mPhysicsBodyId, radius);
	}

	sf::Vector2f Actor::GetVelocity() const
	{
		return mVelocity;
	}

	void Actor::SetVelocity(const sf::Vector2f& velocity)
	{
		mVelocity = velocity;
	}
}