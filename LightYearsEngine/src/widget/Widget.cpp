#include "widget/Widget.h"

namespace ly
{
	Widget::Widget():
		mIsVisible{ true },
		mAnimState{ AnimState::Idle },
		mAlpha{ 1.0f },
		mAnimTimer{ 0.0f },
		mFadeInDuration{ 0.0f },
		mHoldDuration{ 0.0f },
		mFadeOutDuration{ 0.0f }
	{

	}

	void Widget::NativeDraw(sf::RenderWindow& windowRef)
	{
		if (mIsVisible)
		{
			Draw(windowRef);
		}
	}

	void Widget::NativeTick(float deltaTime)
	{
		UpdateAnimation(deltaTime);
		Tick(deltaTime);
	}

	void Widget::Tick(float deltaTime)
	{

	}

	void Widget::SetAlpha(float alpha)
	{
		mAlpha = alpha;
		if(mAlpha < 0.0f)
			mAlpha = 0.0f;
		else if (mAlpha > 1.0f)
			mAlpha = 1.0f;
		ApplyAlpha(mAlpha);
	}

	void Widget::StartFadeAnimation(float fadeInDuration, float holdDuration, float fadeOutDuration)
	{
		mFadeInDuration = fadeInDuration;
		mHoldDuration = holdDuration;
		mFadeOutDuration = fadeOutDuration;

		mAnimState = AnimState::FadingIn;
		mAnimTimer = 0.f;
		mAlpha = 0.f;
		ApplyAlpha(0.f);
	}

	void Widget::StopAnimation()
	{
		mAnimState = AnimState::Idle;
		mAnimTimer = 0.f;
	}

	void Widget::UpdateAnimation(float deltaTime)
	{
		switch (mAnimState)
		{
		case AnimState::FadingIn:
			mAnimTimer += deltaTime;

			if (mFadeInDuration > 0.f)
			{
				mAlpha = mAnimTimer / mFadeInDuration;
			}
			else
			{
				mAlpha = 1.f;
			}

			if (mAlpha >= 1.f)
			{
				mAlpha = 1.f;

				if (mHoldDuration > 0.f)
				{
					mAnimState = AnimState::Holding;
				}
				else if (mFadeOutDuration > 0.f)
				{
					mAnimState = AnimState::FadingOut;
				}
				else
				{
					mAnimState = AnimState::Idle;
				}

				mAnimTimer = 0.f;
			}

			ApplyAlpha(mAlpha);
			break;

		case AnimState::Holding:
			mAnimTimer += deltaTime;

			if (mAnimTimer >= mHoldDuration)
			{
				if (mFadeOutDuration > 0.f)
				{
					mAnimState = AnimState::FadingOut;
				}
				else
				{
					mAnimState = AnimState::Idle;
				}

				mAnimTimer = 0.f;
			}
			break;

		case AnimState::FadingOut:
			mAnimTimer += deltaTime;

			if (mFadeOutDuration > 0.f)
			{
				mAlpha = 1.f - (mAnimTimer / mFadeOutDuration);
			}
			else
			{
				mAlpha = 0.f;
			}

			if (mAlpha <= 0.f)
			{
				mAlpha = 0.f;
				mAnimState = AnimState::Idle;
			}

			ApplyAlpha(mAlpha);
			break;

		case AnimState::Idle:
		default:
			break;
		}
	}

	bool Widget::HandleEvent(const sf::Event& event)
	{
		return false;
	}

	void Widget::SetWidgetLocation(const sf::Vector2f& location)
	{
		mTransformable.setPosition(location);
		LocationUpdated(location);
	}

	void Widget::SetWidgetRotation(float rotation)
	{
		mTransformable.setRotation(sf::degrees(rotation));
		RotationUpdated(rotation);
	}

	void Widget::Draw(sf::RenderWindow& windowRef)
	{
	}

	void Widget::LocationUpdated(const sf::Vector2f& newLocation)
	{
	}

	void Widget::RotationUpdated(float newRotation)
	{
	}

	void Widget::ApplyAlpha(float alpha)
	{
	}

	sf::Vector2f Widget::GetCenterPosition() const
	{
		sf::FloatRect bound = GetBound();
		return sf::Vector2f{ 
			bound.position.x + bound.size.x / 2.0f, 
			bound.position.y + bound.size.y / 2.0f  
		};
	}

	// ? Center the origin based on widget's bounds
	void Widget::CenterOrigin()
	{
		sf::FloatRect bounds = GetBound();
		sf::Vector2f center{
			bounds.size.x / 2.0f,
			bounds.size.y / 2.0f
		};
		
		UpdateOrigin(center);
	}

	// ? Set origin using normalized coordinates (0.0-1.0)
	void Widget::SetOriginNormalized(float x, float y)
	{
		sf::FloatRect bounds = GetBound();
		sf::Vector2f origin{
			bounds.size.x * x,
			bounds.size.y * y
		};
		
		UpdateOrigin(origin);
	}
}