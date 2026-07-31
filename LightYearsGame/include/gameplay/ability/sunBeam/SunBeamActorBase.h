#pragma once

#include "attributes/AttributeSystem.h"

#include "gameplay/ability/actors/AbilityWorldActor.h"
#include "gameplay/ability/sunBeam/SunBeamVisual.h"

namespace ly
{
	class SunBeamActorBase : public AbilityWorldActor
	{
	public:
		SunBeamActorBase(
			World* world,
			Actor* owner,
			const SunBeamVisualDefinition& visualDefinition
		);

		void BeginPlay() final override;
		void Tick(float deltaTime) final override;
		void Render(sf::RenderWindow& window) final override;
		void ConfigureFromAttributes(const sas::GameplayAttributeList& attributes) final override;

	protected:
		virtual void OnSunBeamBeginPlay() {}
		virtual void ConfigureSunBeam(const sas::GameplayAttributeList& attributes) = 0;
		virtual void TickSunBeam(float deltaTime) = 0;
		virtual SunBeamVisualFrame BuildSunBeamVisualFrame() const = 0;

		void ApplyBeamDamageInRectangle(
			const sf::Vector2f& center,
			float length,
			float damageMultiplier = 1.f
		);
		float GetBeamWidth() const { return mBeamWidth; }
		float GetBeamLength() const { return mBeamLength; }
		const SunBeamVisualDefinition& GetSunBeamVisualDefinition() const { return mVisualDefinition; }

	private:
		SunBeamVisualDefinition mVisualDefinition;
		SunBeamVisual mVisual;
		float mBeamWidth = 64.f;
		float mBeamLength = 600.f;
		float mImpactRadius = 1.f;
	};
}
