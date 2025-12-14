#pragma once 

#include <SFML/Graphics.hpp>
#include "framework/Object.h"

namespace ly
{
	class Widget : public Object
	{
	public:
		void NativeDraw(sf::RenderWindow& windowRef);
		void NativeTick(float deltaTime);

		virtual bool HandleEvent(const sf::Event& event);
		void SetWidgetLocation(const sf::Vector2f& location);
		void SetWidgetRotation(float rotation);
		sf::Vector2f GetWidgetLocation() const { return mTransformable.getPosition(); };

		virtual sf::FloatRect GetBound() const=0;
		sf::Vector2f GetCenterPosition() const;

		void SetVisibility(bool newVisibility) { mIsVisible = newVisibility; };
		bool GetVisibility() const { return mIsVisible; };

		void SetAlpha(float alpha);
		float GetAlpha() const { return mAlpha; };

		void StartFadeAnimation(float fadeInDuration, float holdDuration = 0.f, float fadeOutDuration = 0.f);
		void StopAnimation();
		bool IsAnimating() const { return mAnimState !=AnimState::Idle; }

		bool IsExpired() const;

		void CenterOrigin();
		void SetOriginNormalized(float x, float y);

		void SetLifeTime(float lifeTime) { mLifeTime = lifeTime; };
		void SetTag(const std::string& tag) { mTag = tag; };
		const std::string& GetTag() const { return mTag; };
		void DestroyWidget() { mMarkedForRemoval = true; };

	protected:
		Widget();
		
		virtual void UpdateOrigin(const sf::Vector2f& origin) = 0;

		virtual void ApplyAlpha(float alpha);

		virtual void Tick(float deltaTime);

		float mLifeTime;
		float mCurrentTime;
		bool mMarkedForRemoval;
		std::string mTag;

	private:
		virtual void Draw(sf::RenderWindow &windowRef);
		virtual void LocationUpdated(const sf::Vector2f& newLocation);
		virtual void RotationUpdated(float newRotation);

		void UpdateAnimation(float deltaTime);

		sf::Transformable mTransformable;
		bool mIsVisible;

		enum class AnimState
		{
			Idle,
			FadingIn,
			Holding,
			FadingOut
		};

		AnimState mAnimState;
		float mAlpha;
		float mAnimTimer;
		float mFadeInDuration;
		float mHoldDuration;
		float mFadeOutDuration;
	};

}