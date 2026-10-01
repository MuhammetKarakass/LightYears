#include "widget/UILayout.h"
#include "widget/Widget.h"
#include "widget/StackPanel.h"
#include "widget/UIViewModel.h"
#include "widget/UIStyle.h"
#include "widget/HUD.h"
#include "framework/SubscriptionSet.h"
#include <cmath>
#include <iostream>

namespace
{
	class TestSubscriptionSource : public ly::Object
	{
	public:
		ly::Delegate<int> changed;
	};

	struct TestSubscriptionListener
	{
		ly::SubscriptionSet subscriptions;
		int calls{ 0 };
		ly::Delegate<int>* clearDuringCallback{ nullptr };

		void OnChanged(int)
		{
			++calls;
			if (clearDuringCallback != nullptr)
			{
				clearDuringCallback = nullptr;
				subscriptions.Clear();
			}
		}
	};

	class TestBoxWidget : public ly::Widget
	{
	public:
		TestBoxWidget(sf::Vector2f size = { 20.f, 10.f }) : mSize{ size } {}
		sf::FloatRect GetBound() const override { return { GetWidgetLocation(), mSize }; }
		int locationUpdates{ 0 };
		int eventCalls{ 0 };
		int tickCalls{ 0 };
		float appliedAlpha{ 1.f };
		std::function<void()> onTick;
		std::function<void()> onEvent;
		bool eventResult{ false };
		void SetSize(sf::Vector2f size) { mSize = size; InvalidateLayout(); }
	protected:
		void UpdateOrigin(const sf::Vector2f&) override {}
		void ApplyAlpha(float alpha) override { appliedAlpha = alpha; }
		void Tick(float) override { ++tickCalls; if (onTick) onTick(); }
	private:
		void LocationUpdated(const sf::Vector2f&) override { ++locationUpdates; }
		bool HandleEvent(const sf::Event&) override { ++eventCalls; if (onEvent) onEvent(); return eventResult; }
		sf::Vector2f mSize;
	};

	class TestHUD : public ly::HUD
	{
	public:
		using ly::HUD::AddToLayer;
		using ly::HUD::SetViewportSize;
		using ly::HUD::GetViewportSize;
		int manualDispatchCalls{ 0 };
		bool HandleEvent(const sf::Event& event) override
		{
			if (ly::HUD::HandleEvent(event)) return true;
			++manualDispatchCalls;
			return false;
		}
	};

	bool Check(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << message << '\n';
			return false;
		}
		return true;
	}

	bool Near(float actual, float expected)
	{
		return std::abs(actual - expected) < 0.001f;
	}
}

int main()
{
	{
		ly::UIRevision revision;
		ly::UIRevisionWatcher watcher;
		int health = 0;
		if (!Check(revision.Get() == 1 && watcher.Consume(revision) && !watcher.Consume(revision), "UIRevisionWatcher did not force only its first initial consume")) return 1;
		if (!Check(!ly::SetIfChanged(health, 0, revision) && revision.Get() == 1, "SetIfChanged bumped revision for an unchanged field")) return 1;
		if (!Check(ly::SetIfChanged(health, 10, revision) && revision.Get() == 2, "SetIfChanged did not bump revision for a changed field")) return 1;
		int shield = 0;
		if (!Check(ly::SetIfChanged(shield, 5, revision) && revision.Get() == 3, "A second changed field did not bump revision independently")) return 1;
		if (!Check(watcher.Consume(revision) && !watcher.Consume(revision), "Watcher did not coalesce multiple field changes into one consume")) return 1;
		watcher.Reset();
		if (!Check(watcher.Consume(revision) && !watcher.Consume(revision), "Watcher Reset did not force the next consume")) return 1;
	}
	{
		const ly::UIStyle defaults = ly::UIStyle::MakeDefault();
		ly::UIStyle custom = defaults;
		custom.colors[static_cast<std::size_t>(ly::UIColorRole::Accent)] = sf::Color{ 1, 2, 3 };
		ly::UIStyle::Set(custom);
		if (!Check(ly::UIStyle::Get().Color(ly::UIColorRole::Accent) == sf::Color{ 1, 2, 3 }, "UIStyle::Set did not update the active style")) return 1;
		ly::UIStyle::Set(defaults);
		if (!Check(ly::UIStyle::Get().Font(ly::UIFontRole::Body) == "SpaceShooterRedux/Bonus/kenvector_future.ttf" && ly::UIStyle::Get().TextSize(ly::UITextSize::Title) == 32 && ly::UIStyle::Get().Color(ly::UIColorRole::GaugeBackground) == sf::Color{ 128, 128, 128 }, "UIStyle defaults did not match the foundation contract")) return 1;
	}
	{
		auto source = std::make_shared<TestSubscriptionSource>();
		TestSubscriptionListener listener;
		listener.subscriptions.Bind(source, source->changed, &listener, &TestSubscriptionListener::OnChanged);
		if (!Check(listener.subscriptions.Count() == 1 && !listener.subscriptions.IsEmpty(), "SubscriptionSet did not record a guarded binding")) return 1;
		source->changed.Broadcast(1);
		if (!Check(listener.calls == 1, "SubscriptionSet binding did not deliver a callback")) return 1;
		listener.subscriptions.Clear();
		listener.subscriptions.Clear();
		source->changed.Broadcast(2);
		if (!Check(listener.calls == 1 && listener.subscriptions.IsEmpty(), "SubscriptionSet Clear was not idempotent or did not unbind")) return 1;
	}
	{
		auto source = std::make_shared<TestSubscriptionSource>();
		TestSubscriptionListener listener;
		listener.subscriptions.Bind(source, source->changed, &listener, &TestSubscriptionListener::OnChanged);
		std::weak_ptr<TestSubscriptionSource> sourceWeak = source;
		source.reset();
		listener.subscriptions.Clear();
		if (!Check(sourceWeak.expired() && listener.subscriptions.IsEmpty(), "Guarded Clear retained or accessed a destroyed source")) return 1;
	}
	{
		auto source = std::make_shared<TestSubscriptionSource>();
		TestSubscriptionListener listener;
		listener.clearDuringCallback = &source->changed;
		listener.subscriptions.Bind(source, source->changed, &listener, &TestSubscriptionListener::OnChanged);
		source->changed.Broadcast(1);
		source->changed.Broadcast(2);
		if (!Check(listener.calls == 1 && listener.subscriptions.IsEmpty(), "SubscriptionSet Clear during broadcast was unsafe or left a callback bound")) return 1;
	}
	{
		auto source = std::make_shared<TestSubscriptionSource>();
		TestSubscriptionListener listener;
		listener.subscriptions.Bind(source, source->changed, &listener, &TestSubscriptionListener::OnChanged);
		ly::SubscriptionSet movedTo{ std::move(listener.subscriptions) };
		if (!Check(listener.subscriptions.IsEmpty() && movedTo.Count() == 1, "SubscriptionSet move construction did not empty the source set")) return 1;
		movedTo = ly::SubscriptionSet{};
		source->changed.Broadcast(1);
		if (!Check(listener.calls == 0 && movedTo.IsEmpty(), "SubscriptionSet move assignment did not unbind replaced subscriptions")) return 1;
	}
	{
		auto source = std::make_shared<TestSubscriptionSource>();
		TestSubscriptionListener listener;
		{
			ly::SubscriptionSet subscriptions;
			subscriptions.Bind(source, source->changed, &listener, &TestSubscriptionListener::OnChanged);
		}
		source->changed.Broadcast(1);
		if (!Check(listener.calls == 0, "SubscriptionSet destructor did not unbind")) return 1;
	}
	{
		auto source = std::make_shared<TestSubscriptionSource>();
		TestSubscriptionListener listener;
		listener.subscriptions.BindUnguarded(source->changed, &listener, &TestSubscriptionListener::OnChanged);
		source->changed.Broadcast(1);
		listener.subscriptions.Clear();
		source->changed.Broadcast(2);
		if (!Check(listener.calls == 1, "SubscriptionSet unguarded binding did not unbind")) return 1;
	}

	TestHUD hud;
	hud.SetViewportSize({ 1280, 720 });
	auto anchored = hud.AddToLayer<TestBoxWidget>(ly::UILayer::Hud);
	anchored.lock()->SetLayout(ly::UILayout::Anchored(ly::UIAnchor::BottomRight, { -10.f, -10.f }, { 20.f, 10.f }));
	const sf::Event event{ sf::Event::Closed{} };
	anchored.lock()->eventResult = false;
	auto legacy = hud.AddWidget<TestBoxWidget>();
	legacy.lock()->eventResult = true;
	auto modal = hud.AddToLayer<TestBoxWidget>(ly::UILayer::Modal);
	modal.lock()->eventResult = false;
	hud.Tick(0.f);
	if (!Check(hud.GetViewportSize() == sf::Vector2u{ 1280, 720 } && Near(anchored.lock()->GetBound().position.x, 1250.f) && Near(anchored.lock()->GetBound().position.y, 700.f), "HUD did not resolve a layer child on the first tick")) return 1;
	hud.SetViewportSize({ 1920, 1080 });
	hud.Tick(0.f);
	if (!Check(Near(anchored.lock()->GetBound().position.x, 1890.f) && Near(anchored.lock()->GetBound().position.y, 1060.f), "HUD did not relayout after an explicit viewport update")) return 1;
	if (!Check(hud.HandleEvent(event) && modal.lock()->eventCalls == 1 && anchored.lock()->eventCalls == 0 && legacy.lock()->eventCalls == 0 && hud.manualDispatchCalls == 0, "Visible modal layer did not capture input before lower layers, legacy widgets, and override dispatch")) return 1;
	modal.lock()->SetVisibility(false);
	if (!Check(hud.HandleEvent(event) && legacy.lock()->eventCalls == 1, "Hidden modal child blocked legacy widget input")) return 1;
	modal.lock()->SetVisibility(true);
	modal.lock()->onEvent = [&]() { modal.lock()->DestroyWidget(); };
	if (!Check(hud.HandleEvent(event) && legacy.lock()->eventCalls == 1, "Modal callback destruction allowed same-event input to fall through")) return 1;
	modal.lock()->DestroyWidget();
	if (!Check(hud.HandleEvent(event) && legacy.lock()->eventCalls == 2, "Destroyed modal child blocked legacy widget input")) return 1;

	auto parentWidget = std::make_shared<TestBoxWidget>(sf::Vector2f{ 200.f, 100.f });
	parentWidget->SetWidgetLocation({ 30.f, 40.f });
	auto childWeak = parentWidget->AddChild<TestBoxWidget>();
	auto child = childWeak.lock();
	child->SetLayout(ly::UILayout::Anchored(ly::UIAnchor::Center));
	parentWidget->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	if (!Check(Near(child->GetBound().position.x, 120.f) && Near(child->GetBound().position.y, 85.f), "Child layout did not resolve against parent bounds")) return 1;
	parentWidget->SetWidgetLocation({ 50.f, 60.f });
	parentWidget->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, false);
	if (!Check(Near(child->GetBound().position.x, 140.f) && Near(child->GetBound().position.y, 105.f), "Parent movement did not invalidate descendant layout")) return 1;
	const int placements = child->locationUpdates;
	parentWidget->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, false);
	if (!Check(child->locationUpdates == placements, "Clean layout tree performed redundant placement")) return 1;

	auto first = parentWidget->AddChild<TestBoxWidget>();
	auto last = parentWidget->AddChild<TestBoxWidget>();
	first.lock()->eventResult = true;
	last.lock()->eventResult = true;
	if (!Check(parentWidget->NativeHandleEvent(event) && last.lock()->eventCalls == 1 && first.lock()->eventCalls == 0, "Events were not dispatched newest child first")) return 1;
	last.lock()->SetVisibility(false);
	if (!Check(parentWidget->NativeHandleEvent(event) && first.lock()->eventCalls == 1, "Invisible child handled an event")) return 1;
	auto mutationParent = std::make_shared<TestBoxWidget>();
	auto mutatingChild = mutationParent->AddChild<TestBoxWidget>().lock();
	ly::weak_ptr<TestBoxWidget> eventAddedChild;
	mutatingChild->onEvent = [&]() { mutatingChild->onEvent = {}; eventAddedChild = mutationParent->AddChild<TestBoxWidget>(); };
	if (!Check(!mutationParent->NativeHandleEvent(event) && eventAddedChild.lock() && eventAddedChild.lock()->eventCalls == 0 && mutationParent->eventCalls == 1, "Event dispatch included a child added during propagation")) return 1;
	mutationParent->NativeHandleEvent(event);
	if (!Check(eventAddedChild.lock()->eventCalls == 1, "Child added during event was not available on the next dispatch")) return 1;

	parentWidget->SetAlpha(.5f);
	child->SetAlpha(.5f);
	if (!Check(Near(child->GetEffectiveAlpha(), .25f) && Near(child->appliedAlpha, .25f), "Parent alpha was not inherited")) return 1;
	parentWidget->StartFadeAnimation(1.f);
	parentWidget->NativeTick(.5f);
	if (!Check(Near(child->appliedAlpha, .25f), "Animated parent alpha did not propagate")) return 1;
	auto fadedParent = std::make_shared<TestBoxWidget>();
	fadedParent->SetAlpha(.5f);
	auto fadedChild = fadedParent->AddChild<TestBoxWidget>().lock();
	if (!Check(Near(fadedChild->appliedAlpha, .5f), "New child did not inherit its parent's current alpha")) return 1;
	auto parentOwned = std::make_shared<TestBoxWidget>();
	auto externallyOwnedChild = parentOwned->AddChild<TestBoxWidget>().lock();
	externallyOwnedChild->SetAlpha(.5f);
	parentOwned->SetAlpha(.2f);
	parentOwned.reset();
	if (!Check(externallyOwnedChild->GetParent() == nullptr && Near(externallyOwnedChild->appliedAlpha, .5f), "Surviving child was not detached when parent was destroyed")) return 1;
	int addedChildTicks = 0;
	auto addingParent = std::make_shared<TestBoxWidget>();
	ly::weak_ptr<TestBoxWidget> addedDuringTick;
	addingParent->onTick = [&]() { addingParent->onTick = {}; addedDuringTick = addingParent->AddChild<TestBoxWidget>(); };
	addingParent->NativeTick(.01f);
	if (!Check(addedDuringTick.lock() && addedDuringTick.lock()->tickCalls == 0, "Child added during Tick was ticked in the same frame")) return 1;

	auto doomed = parentWidget->AddChild<TestBoxWidget>();
	doomed.lock()->onTick = [&]() { doomed.lock()->DestroyWidget(); };
	parentWidget->NativeTick(.01f);
	if (!Check(doomed.expired() && parentWidget->GetChildren().size() == 3, "Destroyed child was not removed safely at tick end")) return 1;
	child.reset();
	parentWidget.reset();
	mutationParent.reset();
	addingParent.reset();
	fadedParent.reset();
	if (!Check(doomed.expired(), "Child lifetime survived removed parent ownership")) return 1;

	auto row = std::make_shared<ly::StackPanel>(ly::UIOrientation::Horizontal, 5.f);
	row->SetPadding({ 10.f, 8.f });
	row->SetPanelSize({ 100.f, 50.f });
	row->SetLayout(ly::UILayout::Anchored(ly::UIAnchor::TopLeft, {}, { 100.f, 50.f }));
	row->SetCrossAlign(ly::UIAlign::Center);
	auto rowFirst = row->AddChild<TestBoxWidget>().lock();
	auto rowSecond = row->AddChild<TestBoxWidget>(sf::Vector2f{ 30.f, 20.f }).lock();
	row->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	if (!Check(Near(rowFirst->GetBound().position.x, 10.f) && Near(rowSecond->GetBound().position.x, 35.f) && Near(rowFirst->GetBound().position.y, 20.f), "Horizontal StackPanel spacing, padding, or center alignment failed")) return 1;
	rowSecond->SetVisibility(false);
	row->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, false);
	auto intrinsicRow = std::make_shared<ly::StackPanel>(ly::UIOrientation::Horizontal, 5.f);
	intrinsicRow->SetPadding({ 10.f, 8.f });
	intrinsicRow->AddChild<TestBoxWidget>();
	auto hiddenSizedChild = intrinsicRow->AddChild<TestBoxWidget>(sf::Vector2f{ 30.f, 20.f }).lock();
	intrinsicRow->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	hiddenSizedChild->SetVisibility(false);
	intrinsicRow->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, false);
	if (!Check(Near(intrinsicRow->GetResolvedRect().size.x, 40.f), "Hidden StackPanel child occupied space")) return 1;
	rowSecond->SetVisibility(true);
	rowFirst->SetSize({ 25.f, 12.f });
	row->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, false);
	if (!Check(Near(rowSecond->GetBound().position.x, 40.f), "StackPanel did not reflow after child intrinsic size changed")) return 1;
	auto partialRow = std::make_shared<ly::StackPanel>(ly::UIOrientation::Horizontal, 5.f);
	auto partialFirst = partialRow->AddChild<TestBoxWidget>(sf::Vector2f{ 25.f, 12.f }).lock();
	auto partialSecond = partialRow->AddChild<TestBoxWidget>(sf::Vector2f{ 30.f, 20.f }).lock();
	partialSecond->SetLayout(ly::UILayout::Anchored(ly::UIAnchor::TopLeft, {}, { 12.f, 0.f }));
	partialRow->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	if (!Check(Near(partialSecond->GetResolvedRect().size.x, 12.f) && Near(partialSecond->GetResolvedRect().size.y, 20.f) && Near(partialRow->GetResolvedRect().size.x, 42.f), "StackPanel did not apply a partial explicit child size per axis")) return 1;
	auto nested = std::make_shared<ly::StackPanel>(ly::UIOrientation::Vertical, 3.f);
	nested->AddChild<TestBoxWidget>(sf::Vector2f{ 12.f, 7.f });
	nested->AddChild<TestBoxWidget>(sf::Vector2f{ 20.f, 9.f });
	auto nestedRoot = std::make_shared<ly::StackPanel>(ly::UIOrientation::Horizontal);
	auto nestedChild = nestedRoot->AddChild<ly::StackPanel>(ly::UIOrientation::Vertical, 3.f).lock();
	nestedChild->AddChild<TestBoxWidget>(sf::Vector2f{ 12.f, 7.f });
	nestedChild->AddChild<TestBoxWidget>(sf::Vector2f{ 20.f, 9.f });
	nestedRoot->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	if (!Check(Near(nestedChild->GetResolvedRect().size.x, 20.f) && Near(nestedChild->GetResolvedRect().size.y, 19.f) && Near(nestedRoot->GetResolvedRect().size.x, 20.f), "Nested layoutless StackPanel intrinsic size was incorrect")) return 1;
	auto positionedPanel = std::make_shared<ly::Panel>(sf::Vector2f{ 40.f, 20.f });
	positionedPanel->SetWidgetLocation({ 100.f, 80.f });
	positionedPanel->CenterOrigin();
	positionedPanel->SetLayout(ly::UILayout::Anchored(ly::UIAnchor::TopLeft, { 10.f, 15.f }, { 40.f, 20.f }));
	positionedPanel->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	if (!Check(Near(positionedPanel->GetBound().position.x, 10.f) && Near(positionedPanel->GetBound().position.y, 15.f) && Near(positionedPanel->GetWidgetLocation().x, 30.f) && Near(positionedPanel->GetWidgetLocation().y, 25.f), "Panel layout position diverged from its physical bounds and origin")) return 1;
	const sf::Vector2f panelTransformPosition = positionedPanel->GetWidgetLocation();
	positionedPanel->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, false);
	if (!Check(positionedPanel->GetWidgetLocation() == panelTransformPosition, "Repeated Panel layout resolution drifted its transform")) return 1;
	positionedPanel->ClearLayout();
	positionedPanel->SetWidgetLocation(positionedPanel->GetWidgetLocation() + sf::Vector2f{ 5.f, 7.f });
	positionedPanel->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, false);
	if (!Check(Near(positionedPanel->GetBound().position.x, 15.f) && Near(positionedPanel->GetBound().position.y, 22.f), "Clearing Panel layout caused a physical position jump")) return 1;
	auto nestedOriginRoot = std::make_shared<ly::StackPanel>(ly::UIOrientation::Horizontal);
	auto nestedOriginPanel = nestedOriginRoot->AddChild<ly::StackPanel>(ly::UIOrientation::Vertical).lock();
	nestedOriginPanel->AddChild<TestBoxWidget>(sf::Vector2f{ 20.f, 10.f });
	nestedOriginRoot->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	nestedOriginPanel->CenterOrigin();
	nestedOriginRoot->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	const sf::Vector2f nestedOriginTransform = nestedOriginPanel->GetWidgetLocation();
	if (!Check(Near(nestedOriginPanel->GetBound().position.x, nestedOriginPanel->GetResolvedRect().position.x) && Near(nestedOriginPanel->GetBound().position.y, nestedOriginPanel->GetResolvedRect().position.y) && nestedOriginPanel->GetWidgetLocation() != nestedOriginPanel->GetBound().position, "Nested StackPanel placement diverged from physical bounds and origin")) return 1;
	nestedOriginRoot->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	if (!Check(nestedOriginPanel->GetWidgetLocation() == nestedOriginTransform, "Repeated nested StackPanel placement drifted its transform")) return 1;

	// Fixed row size: the row itself does not move or resize, so only the sibling shift
	// can carry the stacked Panel's anchored child along.
	auto shiftRow = std::make_shared<ly::StackPanel>(ly::UIOrientation::Horizontal);
	shiftRow->SetLayout(ly::UILayout::Anchored(ly::UIAnchor::TopLeft, {}, { 200.f, 20.f }));
	auto shiftSizer = shiftRow->AddChild<TestBoxWidget>(sf::Vector2f{ 20.f, 10.f }).lock();
	auto shiftPanel = shiftRow->AddChild<ly::Panel>(sf::Vector2f{ 40.f, 20.f }).lock();
	auto shiftAnchored = shiftPanel->AddChild<TestBoxWidget>(sf::Vector2f{ 10.f, 10.f }).lock();
	shiftAnchored->SetLayout(ly::UILayout::Anchored(ly::UIAnchor::Center));
	shiftRow->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	if (!Check(Near(shiftAnchored->GetBound().position.x, 35.f), "Anchored child inside a stacked Panel resolved incorrectly")) return 1;
	shiftSizer->SetSize({ 30.f, 10.f });
	shiftRow->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, false);
	if (!Check(Near(shiftPanel->GetBound().position.x, 30.f) && Near(shiftAnchored->GetBound().position.x, 45.f), "Anchored child stayed behind when its stacked Panel moved")) return 1;

	auto roundedRow = std::make_shared<ly::StackPanel>(ly::UIOrientation::Horizontal);
	roundedRow->SetCrossAlign(ly::UIAlign::Center);
	roundedRow->SetLayout(ly::UILayout::Anchored(ly::UIAnchor::TopLeft, {}, { 100.f, 51.f }));
	auto roundedChild = roundedRow->AddChild<TestBoxWidget>(sf::Vector2f{ 20.f, 10.f }).lock();
	roundedRow->ResolveLayoutTree({ { 0.f, 0.f }, { 400.f, 300.f } }, true);
	const float roundedY = roundedChild->GetBound().position.y;
	if (!Check(Near(roundedY, std::round(roundedY)) && Near(roundedY, 21.f), "StackPanel center alignment produced a fractional pixel position")) return 1;

	auto alphaRoot = std::make_shared<TestBoxWidget>();
	auto alphaMiddle = alphaRoot->AddChild<TestBoxWidget>().lock();
	auto alphaLeaf = alphaMiddle->AddChild<TestBoxWidget>().lock();
	alphaRoot->SetAlpha(.5f);
	alphaMiddle->SetAlpha(.5f);
	alphaLeaf->SetAlpha(.5f);
	if (!Check(Near(alphaLeaf->appliedAlpha, .125f), "Three-level alpha was not accumulated")) return 1;
	alphaRoot->SetAlpha(1.f);
	if (!Check(Near(alphaMiddle->appliedAlpha, .5f) && Near(alphaLeaf->appliedAlpha, .25f), "Root alpha change did not propagate through the subtree")) return 1;

	{
		TestSubscriptionListener bindListener;
		auto liveSource = std::make_shared<TestSubscriptionSource>();
		if (!Check(bindListener.subscriptions.Bind(liveSource, liveSource->changed, &bindListener, &TestSubscriptionListener::OnChanged), "Bind to a live source reported failure")) return 1;
		ly::weak_ptr<ly::Object> expiredWeak = std::make_shared<TestSubscriptionSource>();
		ly::Delegate<int> unusedDelegate;
		if (!Check(!bindListener.subscriptions.Bind(expiredWeak, unusedDelegate, &bindListener, &TestSubscriptionListener::OnChanged) && bindListener.subscriptions.Count() == 1, "Bind to an expired source reported success")) return 1;
		if (!Check(!bindListener.subscriptions.BindUnguarded(unusedDelegate, static_cast<TestSubscriptionListener*>(nullptr), &TestSubscriptionListener::OnChanged) && bindListener.subscriptions.Count() == 1, "Unguarded bind with a null listener reported success")) return 1;
	}

	const ly::UIRect parent{ { 0.f, 0.f }, { 1280.f, 720.f } };
	const sf::Vector2f box{ 100.f, 40.f };
	const ly::UIAnchor anchors[] = {
		ly::UIAnchor::TopLeft, ly::UIAnchor::Top, ly::UIAnchor::TopRight,
		ly::UIAnchor::Left, ly::UIAnchor::Center, ly::UIAnchor::Right,
		ly::UIAnchor::BottomLeft, ly::UIAnchor::Bottom, ly::UIAnchor::BottomRight
	};
	const sf::Vector2f expected[] = {
		{ 0.f, 0.f }, { 590.f, 0.f }, { 1180.f, 0.f },
		{ 0.f, 340.f }, { 590.f, 340.f }, { 1180.f, 340.f },
		{ 0.f, 680.f }, { 590.f, 680.f }, { 1180.f, 680.f }
	};
	for (std::size_t i = 0; i < 9; ++i)
	{
		const ly::UIRect rect = ly::ResolveLayout(ly::UILayout::Anchored(anchors[i], { 0.f, 0.f }, box), parent, { 0.f, 0.f });
		if (!Check(Near(rect.position.x, expected[i].x) && Near(rect.position.y, expected[i].y), "Anchor resolved to an unexpected position")) return 1;
	}

	const ly::UIRect intrinsic = ly::ResolveLayout(ly::UILayout::Anchored(ly::UIAnchor::Center), parent, { 80.f, 30.f });
	if (!Check(Near(intrinsic.position.x, 600.f) && Near(intrinsic.position.y, 345.f) && Near(intrinsic.size.x, 80.f) && Near(intrinsic.size.y, 30.f), "Zero point size did not use intrinsic size")) return 1;

	const ly::UIRect stretched = ly::ResolveLayout(ly::UILayout::Stretch(), parent, {});
	if (!Check(Near(stretched.position.x, 0.f) && Near(stretched.position.y, 0.f) && Near(stretched.size.x, 1280.f) && Near(stretched.size.y, 720.f), "Full stretch did not fill its parent")) return 1;
	const ly::UIRect inset = ly::ResolveLayout(ly::UILayout::Stretch({ 10.f, 20.f }, { 30.f, 40.f }), parent, {});
	if (!Check(Near(inset.position.x, 10.f) && Near(inset.position.y, 20.f) && Near(inset.size.x, 1240.f) && Near(inset.size.y, 660.f), "Stretch insets resolved incorrectly")) return 1;

	ly::UILayout mixed = ly::UILayout::Anchored(ly::UIAnchor::Center, { 0.f, 5.f }, { 0.f, 20.f });
	mixed.anchorMin.x = 0.f;
	mixed.anchorMax.x = 1.f;
	mixed.insetMin.x = 12.f;
	mixed.insetMax.x = 18.f;
	const ly::UIRect mixedRect = ly::ResolveLayout(mixed, parent, { 0.f, 50.f });
	if (!Check(Near(mixedRect.position.x, 12.f) && Near(mixedRect.size.x, 1250.f) && Near(mixedRect.position.y, 355.f) && Near(mixedRect.size.y, 20.f), "Mixed stretch and point axes resolved incorrectly")) return 1;

	const ly::UIRect offsetParent = ly::ResolveLayout(ly::UILayout::Anchored(ly::UIAnchor::BottomRight, { -5.f, -7.f }, box), { { 25.f, 35.f }, { 200.f, 100.f } }, {});
	if (!Check(Near(offsetParent.position.x, 120.f) && Near(offsetParent.position.y, 88.f), "Parent position was not included")) return 1;
	const ly::UIRect negative = ly::ResolveLayout(ly::UILayout::Stretch({ 90.f, 0.f }, { 20.f, 0.f }), { { 0.f, 0.f }, { 100.f, 50.f } }, {});
	if (!Check(Near(negative.size.x, 0.f), "Negative stretch extent was not clamped to zero")) return 1;
	const ly::UIRect rounded = ly::ResolveLayout(ly::UILayout::Anchored(ly::UIAnchor::TopLeft, { 1.5f, 2.5f }, { 10.f, 10.f }), parent, {});
	if (!Check(Near(rounded.position.x, 2.f) && Near(rounded.position.y, 3.f), "Half-pixel position did not round to the nearest pixel")) return 1;
	return 0;
}
