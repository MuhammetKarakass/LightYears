#include "gameplay/module/generated/ModuleMineActor.h"
#include "gameplay/module/generated/ModuleGeneratedDamage.h"
#include "gameConfigs/combat/DamageTypeConfig.h"
#include <algorithm>

namespace ly
{
	ModuleMineActor::ModuleMineActor(World* world, const weak_ptr<Actor>& source, const ModuleMinePresentationProfile& profile, const weak_ptr<ModuleGeneratedLease>& lease, float damage, float radius, float fuse, int cryoStacks, const GameplayTag& damageType) : Actor{ world }, mSource{ source }, mProfile{ profile }, mLease{ lease }, mDamage{ damage }, mRadius{ radius }, mFuse{ fuse }, mRemaining{ fuse }, mCryoStacks{ cryoStacks }, mDamageType{ damageType }
	{
		SetEnablePhysics(false);
		SetRenderLayer(RenderLayer::GroundDecal);
	}
	void ModuleMineActor::Tick(float deltaTime)
	{
		const auto source = mSource.lock();
		const auto lease = mLease.lock();
		if (!source || source->GetIsPendingDestroy() || !lease || !lease->active) { Destroy(); return; }
		mRemaining -= std::max(0.f, deltaTime);
		if (mRemaining > 0.f) return;
		if (mExploded) { Destroy(); return; }
		mExploded = true;
		mRemaining = mProfile.explosionDuration;
		ApplyModuleGeneratedAreaDamage(*this, *source, lease, GetActorLocation(), mRadius, mDamage, { mDamageType }, mCryoStacks, DamageSourceScalingPolicy::Normal);
	}
	void ModuleMineActor::Render(sf::RenderTarget& window)
	{
		const float radius = mExploded ? mRadius : mProfile.bodyRadius;
		sf::CircleShape body{ radius };
		body.setOrigin({ radius, radius });
		body.setPosition(GetActorLocation());
		body.setFillColor(mExploded ? sf::Color::Transparent : mProfile.body);
		body.setOutlineColor(mProfile.fuseRing);
		body.setOutlineThickness(mProfile.ringThickness);
		window.draw(body);
		if (!mExploded)
		{
			const float ringRadius = mRadius * std::clamp(1.f - mRemaining / mFuse, 0.f, 1.f);
			sf::CircleShape ring{ ringRadius };
			ring.setOrigin({ ringRadius, ringRadius });
			ring.setPosition(GetActorLocation());
			ring.setFillColor(sf::Color::Transparent);
			ring.setOutlineColor(mProfile.fuseRing);
			ring.setOutlineThickness(mProfile.ringThickness);
			window.draw(ring);
		}
	}
}
