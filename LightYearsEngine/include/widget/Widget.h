#pragma once

#include <SFML/Graphics.hpp>
#include <type_traits>
#include <utility>
#include "framework/Object.h"
#include "widget/UILayout.h"

namespace ly
{
	// Children that have no layout keep absolute positioning and do not follow their parent;
	// give a child a UILayout (or place it in a StackPanel) to make it parent-relative.
	class Widget : public Object
	{
	public:
		virtual ~Widget();
		void NativeDraw(sf::RenderWindow& windowRef);
		void NativeTick(float deltaTime);
		virtual bool HandleEvent(const sf::Event& event);
		bool NativeHandleEvent(const sf::Event& event);
		void SetWidgetLocation(const sf::Vector2f& location);
		void SetWidgetRotation(float rotation);
		sf::Vector2f GetWidgetLocation() const { return mTransformable.getPosition(); }
		virtual sf::FloatRect GetBound() const = 0;
		sf::Vector2f GetCenterPosition() const;
		void SetVisibility(bool newVisibility);
		bool GetVisibility() const { return mIsVisible; }
		void SetAlpha(float alpha);
		float GetAlpha() const { return mAlpha; }
		float GetEffectiveAlpha() const;
		void StartFadeAnimation(float fadeInDuration, float holdDuration = 0.f, float fadeOutDuration = 0.f);
		void StopAnimation();
		bool IsAnimating() const { return mAnimState != AnimState::Idle; }
		bool IsExpired() const;
		void CenterOrigin();
		void SetOriginNormalized(float x, float y);
		void SetLifeTime(float lifeTime) { mLifeTime = lifeTime; }
		void SetTag(const std::string& tag) { mTag = tag; }
		const std::string& GetTag() const { return mTag; }
		void DestroyWidget() { mMarkedForRemoval = true; }

		template<typename T, typename... Args>
		weak_ptr<T> AddChild(Args&&... args)
		{
			auto child = std::make_shared<T>(std::forward<Args>(args)...);
			static_assert(std::is_base_of<Widget, T>::value, "T must derive from Widget");
			child->mParent = this;
			mChildren.push_back(child);
			child->ApplyEffectiveAlphaToTree();
			InvalidateLayout();
			return child;
		}
		void RemoveChild(const weak_ptr<Widget>& child);
		const List<shared_ptr<Widget>>& GetChildren() const { return mChildren; }
		Widget* GetParent() const { return mParent; }
		void SetLayout(const UILayout& layout);
		void ClearLayout();
		bool HasLayout() const { return mHasLayout; }
		const UILayout& GetLayout() const { return mLayout; }
		const UIRect& GetResolvedRect() const { return mResolvedRect; }
		void InvalidateLayout();
		void ResolveLayoutTree(const UIRect& parentRect, bool force);
		// Size the widget wants when its layout does not set one; containers query it.
		virtual sf::Vector2f GetIntrinsicSize() const;

	protected:
		Widget();
		virtual void UpdateOrigin(const sf::Vector2f& origin) = 0;
		virtual void ApplyAlpha(float alpha);
		virtual void Tick(float deltaTime);
		virtual void ArrangeChildren(const UIRect& selfRect, bool force);
		virtual void PlaceAt(const UIRect& rect);
		// Containers that compute child rects themselves (StackPanel) place a child through this
		// so the child's subtree is re-resolved whenever the rect it receives changes.
		void ArrangeChildAt(Widget& child, const UIRect& rect, bool force);
		float mLifeTime;
		float mCurrentTime;
		bool mMarkedForRemoval;
		std::string mTag;

	private:
		virtual void Draw(sf::RenderWindow& windowRef);
		virtual void LocationUpdated(const sf::Vector2f& newLocation);
		virtual void RotationUpdated(float newRotation);
		void UpdateAnimation(float deltaTime);
		void ApplyEffectiveAlphaToTree();
		void ApplyInheritedAlpha(float parentAlpha);
		sf::Transformable mTransformable;
		bool mIsVisible;
		enum class AnimState { Idle, FadingIn, Holding, FadingOut };
		AnimState mAnimState;
		float mAlpha;
		float mAnimTimer;
		float mFadeInDuration;
		float mHoldDuration;
		float mFadeOutDuration;
		Widget* mParent{ nullptr };
		List<shared_ptr<Widget>> mChildren;
		UILayout mLayout{};
		UIRect mResolvedRect{};
		bool mHasLayout{ false };
		bool mLayoutDirty{ true };
		bool mSubtreeDirty{ true };
	};
}
