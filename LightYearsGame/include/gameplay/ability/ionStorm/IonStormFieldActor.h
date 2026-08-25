#pragma once

#include "attributes/AttributeSystem.h"
#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/ionStorm/IonStormBoundary.h"
#include "presentation/ability/ionStorm/IonStormPresentationProfile.h"

#include <SFML/Graphics/VertexArray.hpp>

namespace ly
{
	class IonStormFieldActor final : public AbilityWorldActor
	{
	public:
		IonStormFieldActor(
			World* world,
			Actor* owner,
			const IonStormFieldPresentationProfile& presentationProfile
		);

		void Tick(float deltaTime) override;
		void Render(sf::RenderWindow& window) override;
		void Destroy() override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) override;

		float GetResolvedDuration() const { return mDuration; }
		float GetResolvedTickInterval() const { return mTickInterval; }
		int GetTickCount() const { return mTickCount; }
		const IonStormBoundary& GetBoundary() const { return mBoundary; }

	private:
		void ApplyDamageTick(Actor& owner);
		bool IsEligibleTarget(const Actor& actor) const;

		IonStormFieldPresentationProfile mPresentationProfile;
		std::weak_ptr<Actor> mOwnerReference;
		IonStormBoundary mBoundary;
		float mDuration = 4.f;
		float mTickInterval = 0.25f;
		float mTickAccumulator = 0.f;
		float mFieldAge = 0.f;
		int mTickCount = 0;
		int mMaximumTickCount = 16;
		float mVisualAge = 0.f;
	};

	bool RegisterIonStormFieldActorType();
}
