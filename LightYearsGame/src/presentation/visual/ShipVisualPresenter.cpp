#include "presentation/visual/ShipVisualPresenter.h"

#include "framework/Actor.h"
#include "framework/World.h"
#include "gameplay/ability/GameAbility.h"
#include "gameplay/ability/LightYearsAbilitySystemComponent.h"
#include "player/Player.h"
#include "player/PlayerManager.h"
#include "player/PlayerSpaceShip.h"

#include <algorithm>
#include <cmath>

namespace ly
{
	namespace
	{
		// Same speed scale the old engine glow used: full thrust look at 500 units per second.
		constexpr float ThrustFullSpeed = 500.f;
		constexpr float HullHitSeconds = 0.22f;
		constexpr float ShieldHitSeconds = 0.5f;
		constexpr float ActivationBurstSeconds = 0.45f;
		constexpr float GoldenAngle = 2.399963f;

		// Ability ids read "Ability.<Category>.<Family>...": the second token picks the port color.
		visual::AbilityCategory CategoryOf(const std::string& abilityId)
		{
			const std::size_t first = abilityId.find('.');
			if (first == std::string::npos)
			{
				return visual::AbilityCategory::Utility;
			}
			const std::size_t second = abilityId.find('.', first + 1);
			const std::string category = abilityId.substr(first + 1, second == std::string::npos ? std::string::npos : second - first - 1);
			if (category == "Primary") return visual::AbilityCategory::Primary;
			if (category == "Offense") return visual::AbilityCategory::Offense;
			if (category == "Defense") return visual::AbilityCategory::Defense;
			if (category == "Movement") return visual::AbilityCategory::Movement;
			return visual::AbilityCategory::Utility;
		}

		float Fraction(float value, float maximum)
		{
			return maximum > 0.f ? std::clamp(value / maximum, 0.f, 1.f) : 0.f;
		}
	}

	bool ShipVisualPresenter::HidesActor(const Actor& actor) const
	{
		return mHiddenShipId != 0 && actor.GetUniqueID() == mHiddenShipId;
	}

	void ShipVisualPresenter::Track(PlayerSpaceShip& ship, float deltaSeconds)
	{
		const float health = ship.GetHealthComponent().GetHealth();
		const float shield = ship.GetShieldComponent().GetShield();
		if (mTrackedShipId != ship.GetUniqueID())
		{
			mTrackedShipId = ship.GetUniqueID();
			mHullHit = 0.f;
			mShieldHit = 0.f;
			mShieldHits = 0;
			mPorts = {};
		}
		else
		{
			if (health < mLastHealth - 0.01f)
			{
				mHullHit = 1.f;
			}
			if (shield < mLastShield - 0.01f)
			{
				mShieldHit = 1.f;
				++mShieldHits;
			}
		}
		mLastHealth = health;
		mLastShield = shield;
		mHullHit = std::max(0.f, mHullHit - deltaSeconds / HullHitSeconds);
		mShieldHit = std::max(0.f, mShieldHit - deltaSeconds / ShieldHitSeconds);
		for (PortTracker& port : mPorts)
		{
			port.burst = std::max(0.f, port.burst - deltaSeconds / ActivationBurstSeconds);
		}
	}

	void ShipVisualPresenter::Draw(const World& world, const sf::View& worldView, visual::VisualAssets& assets, visual::VisualPipeline& pipeline, float timeSeconds)
	{
		const float deltaSeconds = std::clamp(timeSeconds - mLastTime, 0.f, 0.1f);
		mLastTime = timeSeconds;
		mHiddenShipId = 0;

		const sf::Vector2f viewSize = worldView.getSize();
		Player* player = PlayerManager::GetPlayerManager().GetPlayer();
		const shared_ptr<PlayerSpaceShip> ship = player ? player->GetCurrentSpaceShip().lock() : nullptr;
		if (viewSize.y <= 0.f || !ship || ship->GetIsPendingDestroy() || ship->GetWorld() != &world)
		{
			mTrackedShipId = 0;
			return;
		}
		// The ship content id picks the definition through the "bindings" of the visual data; no definition, no change.
		const visual::ShipVisualDefinition* definition = assets.Ships().FindByBinding(PlayerManager::GetPlayerManager().GetSelectedShipId());
		if (!definition)
		{
			return;
		}
		mHiddenShipId = ship->GetUniqueID();
		Track(*ship, deltaSeconds);
		// Portal transit hides the actor through its own render flag; the procedural ship follows it.
		if (!ship->IsRenderEnabled())
		{
			return;
		}
		if (!mRenderer)
		{
			mRenderer = std::make_unique<visual::ShipRenderer>(assets);
		}

		const sf::Vector2f output(pipeline.OutputSize());
		const float pixelsPerUnit = output.y / viewSize.y;
		const sf::Vector2f origin = worldView.getCenter() - viewSize * 0.5f;

		// Fit the design to the sprite's footprint by area, so the visual covers about what the hitbox covers.
		sf::Vector2f spriteSize = definition->size;
		if (const std::optional<sf::Sprite>& sprite = ship->GetSprite())
		{
			spriteSize = sprite->getLocalBounds().size.componentWiseMul(sprite->getScale());
		}
		const float fit = std::sqrt(std::max(1.f, std::abs(spriteSize.x * spriteSize.y)) / (definition->size.x * definition->size.y));

		visual::ShipVisualState state;
		state.position = (ship->GetActorLocation() - origin) * pixelsPerUnit;
		state.rotationDegrees = ship->GetActorRotation();
		state.scale = fit * pixelsPerUnit;
		state.hullFraction = mHullOverride ? std::clamp(*mHullOverride, 0.f, 1.f) : Fraction(mLastHealth, ship->GetHealthComponent().GetMaxHealth());
		state.shieldFraction = Fraction(mLastShield, ship->GetShieldComponent().GetMaxShield());
		state.hitFlash = mHullHit;
		state.shieldHit = mShieldHit;
		state.shieldHitAngle = std::fmod(static_cast<float>(mShieldHits) * GoldenAngle, 6.2831853f);
		state.thrust = std::clamp(std::sqrt(ship->GetVelocity().x * ship->GetVelocity().x + ship->GetVelocity().y * ship->GetVelocity().y) / ThrustFullSpeed, 0.f, 1.f);
		state.afterburner = ship->IsAfterburning();
		state.ghost = ship->IsInvulnerable();
		state.time = timeSeconds;
		state.seed = ship->GetUniqueID();

		// Ability slots Q, E, F, R are Ability1..Ability4, the same mapping the HUD ability bar reads.
		LightYearsAbilitySystemComponent& abilitySystem = ship->GetAbilitySystemComponent();
		for (int index = 0; index < visual::ShipPortCount; ++index)
		{
			const sas::AbilitySlot slot = static_cast<sas::AbilitySlot>(static_cast<int>(sas::AbilitySlot::Ability1) + index);
			PortTracker& tracker = mPorts[static_cast<std::size_t>(index)];
			GameAbility* ability = abilitySystem.GetAbility(slot);
			if (!ability)
			{
				tracker = {};
				continue;
			}
			const std::string& abilityId = ability->GetDefinition().abilityId;
			const bool active = ability->IsActive();
			const bool onCooldown = ability->IsOnCooldown();
			// A new activation: the ability became active, or went straight to cooldown without an active phase.
			const bool sameAbility = tracker.abilityId == abilityId;
			if (sameAbility && ((active && !tracker.active) || (!active && onCooldown && !tracker.onCooldown && !tracker.active)))
			{
				tracker.burst = 1.f;
			}
			tracker.abilityId = abilityId;
			tracker.active = active;
			tracker.onCooldown = onCooldown;

			visual::ShipPortState& port = state.ports[static_cast<std::size_t>(index)];
			port.equipped = true;
			port.category = CategoryOf(abilityId);
			port.isActive = active;
			port.isReady = !active && !onCooldown;
			const float duration = ability->GetCooldownDuration();
			port.cooldownFraction = onCooldown && duration > 0.f ? std::clamp(1.f - ability->GetCooldownRemaining() / duration, 0.f, 1.f) : 1.f;
			port.justActivated = tracker.burst;
		}

		mRenderer->Draw(pipeline, *definition, state);
	}
}
