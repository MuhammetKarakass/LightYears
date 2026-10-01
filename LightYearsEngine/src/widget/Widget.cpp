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
		mFadeOutDuration{ 0.0f },
		mLifeTime{-1},
		mCurrentTime{ 0.0f },
		mMarkedForRemoval{ false },
		mTag{}
	{

	}

	Widget::~Widget()
	{
		for (const auto& child : mChildren)
			if (child && child->mParent == this)
			{
				child->mParent = nullptr;
				child->ApplyEffectiveAlphaToTree();
			}
		if (mParent)
			mParent->InvalidateLayout();
	}

	void Widget::NativeDraw(sf::RenderWindow& windowRef)
	{
		if (mIsVisible)
		{
			Draw(windowRef);
			// Index loop: a draw override must not add children, but if one does the
			// vector may reallocate and a range-for iterator would dangle.
			for (std::size_t i = 0; i < mChildren.size(); ++i)
				if (mChildren[i] && !mChildren[i]->IsExpired()) mChildren[i]->NativeDraw(windowRef);
		}
	}

	void Widget::NativeTick(float deltaTime)
	{
		const std::size_t count = mChildren.size();
		UpdateAnimation(deltaTime);
		if (mLifeTime > 0)
		{
			mCurrentTime += deltaTime;
		}
		Tick(deltaTime);
		for (std::size_t i = 0; i < count && i < mChildren.size(); ++i)
			if (mChildren[i] && !mChildren[i]->IsExpired()) mChildren[i]->NativeTick(deltaTime);
		for (auto it = mChildren.begin(); it != mChildren.end();)
		{
			if (!*it || (*it)->IsExpired())
			{
				if (*it && (*it)->mParent == this)
				{
					(*it)->mParent = nullptr;
					(*it)->ApplyEffectiveAlphaToTree();
				}
				it = mChildren.erase(it);
				InvalidateLayout();
			}
			else ++it;
		}
	}

	void Widget::Tick(float deltaTime)
	{

	}

	bool Widget::IsExpired() const
	{
		if(mMarkedForRemoval)
			return true;
		if(mLifeTime > 0 && mCurrentTime >= mLifeTime)
			return true;
		return false;
	}

	void Widget::SetAlpha(float alpha)
	{
		mAlpha = alpha;
		if(mAlpha < 0.0f)
			mAlpha = 0.0f;
		else if (mAlpha > 1.0f)
			mAlpha = 1.0f;
		ApplyEffectiveAlphaToTree();
	}

	void Widget::StartFadeAnimation(float fadeInDuration, float holdDuration, float fadeOutDuration)
	{
		mFadeInDuration = fadeInDuration;
		mHoldDuration = holdDuration;
		mFadeOutDuration = fadeOutDuration;

		mAnimState = AnimState::FadingIn;
		mAnimTimer = 0.f;
		mAlpha = 0.f;
		ApplyEffectiveAlphaToTree();
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

			ApplyEffectiveAlphaToTree();
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

			ApplyEffectiveAlphaToTree();
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
		InvalidateLayout();
	}

	void Widget::SetVisibility(bool visible)
	{
		if (mIsVisible == visible) return;
		mIsVisible = visible;
		if (mParent) mParent->InvalidateLayout();
	}

	float Widget::GetEffectiveAlpha() const
	{
		return mAlpha * (mParent ? mParent->GetEffectiveAlpha() : 1.f);
	}

	void Widget::ApplyEffectiveAlphaToTree()
	{
		ApplyInheritedAlpha(mParent ? mParent->GetEffectiveAlpha() : 1.f);
	}

	void Widget::ApplyInheritedAlpha(float parentAlpha)
	{
		// Pass the accumulated alpha down so a subtree update stays linear in its size.
		const float effectiveAlpha = mAlpha * parentAlpha;
		ApplyAlpha(effectiveAlpha);
		for (const auto& child : mChildren)
			if (child) child->ApplyInheritedAlpha(effectiveAlpha);
	}

	void Widget::RemoveChild(const weak_ptr<Widget>& child)
	{
		if (auto locked = child.lock(); locked && locked->mParent == this) locked->DestroyWidget();
	}

	void Widget::SetLayout(const UILayout& layout)
	{
		mLayout = layout;
		mHasLayout = true;
		InvalidateLayout();
	}

	void Widget::ClearLayout()
	{
		mHasLayout = false;
		InvalidateLayout();
	}

	void Widget::InvalidateLayout()
	{
		mLayoutDirty = true;
		for (Widget* parent = mParent; parent; parent = parent->mParent) parent->mSubtreeDirty = true;
	}

	sf::Vector2f Widget::GetIntrinsicSize() const
	{
		return GetBound().size;
	}

	void Widget::PlaceAt(const UIRect& rect)
	{
		const sf::FloatRect bound = GetBound();
		SetWidgetLocation(GetWidgetLocation() + (rect.position - bound.position));
	}

	void Widget::ArrangeChildren(const UIRect& selfRect, bool force)
	{
		for (std::size_t i = 0; i < mChildren.size(); ++i)
			if (mChildren[i] && !mChildren[i]->IsExpired()) mChildren[i]->ResolveLayoutTree(selfRect, force);
	}

	void Widget::ArrangeChildAt(Widget& child, const UIRect& rect, bool force)
	{
		const bool changed = child.mResolvedRect.position != rect.position || child.mResolvedRect.size != rect.size;
		child.PlaceAt(rect);
		child.mResolvedRect = rect;
		// A moved or resized child must re-resolve its whole subtree; its anchored
		// descendants are not dirty themselves and would otherwise stay behind.
		child.ArrangeChildren(rect, force || changed);
		child.mLayoutDirty = false;
		child.mSubtreeDirty = false;
	}

	void Widget::ResolveLayoutTree(const UIRect& parentRect, bool force)
	{
		if (!force && !mLayoutDirty && !mSubtreeDirty) return;
		const UIRect previousRect = mResolvedRect;
		UIRect resolved;
		if (mHasLayout) resolved = ResolveLayout(mLayout, parentRect, GetIntrinsicSize());
		else
		{
			const sf::FloatRect bound = GetBound();
			resolved = { bound.position, GetIntrinsicSize() };
		}
		const bool moved = previousRect.position != resolved.position || previousRect.size != resolved.size;
		if (mHasLayout || previousRect.size != resolved.size) PlaceAt(resolved);
		mResolvedRect = resolved;
		ArrangeChildren(mResolvedRect, force || moved);
		mLayoutDirty = false;
		mSubtreeDirty = false;
	}

	bool Widget::NativeHandleEvent(const sf::Event& event)
	{
		if (!mIsVisible || IsExpired()) return false;
		// Handlers may add or remove children. Removal is deferred to Tick, and children
		// added during dispatch sit past the start index, so walking down by index is safe
		// without copying the child list for every widget on every event.
		for (std::size_t i = mChildren.size(); i > 0; --i)
		{
			if (i > mChildren.size()) continue;
			const shared_ptr<Widget> child = mChildren[i - 1];
			if (child && child->mParent == this && !child->IsExpired() && child->NativeHandleEvent(event)) return true;
		}
		return HandleEvent(event);
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
