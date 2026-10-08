#include "gameplay/module/generated/ModuleDelayedExplosionActor.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include <algorithm>

namespace ly
{
	ModuleDelayedExplosionActor::ModuleDelayedExplosionActor(World* world, const weak_ptr<Actor>& source, const ModuleExplosionPresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, float damage, float radius, float delay, const List<GameplayTag>& tags) : Actor{ world }, mSource{ source }, mProfile{ profile }, mLease{ lease }, mDamage{ damage }, mRadius{ radius }, mRemaining{ delay }, mTags{ tags }
	{
		SetEnablePhysics(false);
		SetRenderLayer(RenderLayer::WorldVfx);
	}
	void ModuleDelayedExplosionActor::Tick(float deltaTime)
	{
		const auto source = mSource.lock();
		const auto lease = mLease.lock();
		if (!source || source->GetIsPendingDestroy() || !lease || !lease->active) { Destroy(); return; }
		mRemaining -= std::max(0.f, deltaTime);
		if (mRemaining > 0.f) return;
		if (mExploded) { Destroy(); return; }
		mExploded = true;
		mRemaining = mProfile.duration;
		ApplyModuleGeneratedAreaDamage(*this, *source, lease, GetActorLocation(), mRadius, mDamage, mTags);
	}
	void ModuleDelayedExplosionActor::Render(sf::RenderTarget& window)
	{
		sf::CircleShape ring{ mRadius };
		ring.setOrigin({ mRadius, mRadius });
		ring.setPosition(GetActorLocation());
		ring.setFillColor(sf::Color::Transparent);
		ring.setOutlineColor(mProfile.ring);
		ring.setOutlineThickness(mExploded ? mProfile.thickness : mProfile.thickness * 0.5f);
		window.draw(ring);
	}
}
