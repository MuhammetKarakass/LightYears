#include "gameplay/combat/summon/SummonedCombatantActor.h"

#include "attributes/AttributeMath.h"
#include "gameplay/attributes/AttributeIds.h"

#include <algorithm>

namespace ly
{
	SummonedCombatantActor::SummonedCombatantActor(World* world, Actor* owner)
		: AbilityWorldActor(world, owner)
		, mHealthComponent(1.f, 1.f)
		, mCombatRuntime(*this)
	{
		mCombatRuntime.InitializeOwnerAttributes(1.f);
	}

	SummonedCombatantActor::~SummonedCombatantActor()
	{
		mCombatRuntime.Clear();
	}

	void SummonedCombatantActor::BeginPlay()
	{
		AbilityWorldActor::BeginPlay();
		ConfigureFriendlySummonCollision();
		mHealthComponent.onHealthEmpty.BindAction(
			GetWeakPtr(),
			&SummonedCombatantActor::OnHealthEmpty
		);
		SynchronizeDynamicOwnerCombatAttributes();
	}

	void SummonedCombatantActor::Tick(float deltaTime)
	{
		if (GetIsPendingDestroy())
		{
			return;
		}
		SynchronizeDynamicOwnerCombatAttributes();
		mCombatRuntime.Tick(std::max(0.f, deltaTime));
		AbilityWorldActor::Tick(std::max(0.f, deltaTime));
	}

	void SummonedCombatantActor::ApplyDamage(float amount)
	{
		DamageContext context;
		context.target = this;
		context.originalDamage = amount;
		context.remainingDamage = amount;
		ReceiveDamage(context);
	}

	void SummonedCombatantActor::ReceiveDamage(DamageContext context)
	{
		if (context.remainingDamage <= 0.f || mCombatRuntime.BlocksIncomingDamage())
		{
			return;
		}

		context.target = this;
		mCombatRuntime.ProcessIncomingDamage(context);
		if (context.remainingDamage > 0.f)
		{
			const float healthBeforeDamage = mHealthComponent.GetHealth();
			mHealthComponent.ChangeHealth(-context.remainingDamage);
			context.appliedDamage = std::max(
				0.f,
				healthBeforeDamage - mHealthComponent.GetHealth()
			);
			context.targetWasKilled = healthBeforeDamage > 0.f &&
				mHealthComponent.GetHealth() <= 0.f;
		}
		mCombatRuntime.NotifyDamageResolved(context);
	}

	void SummonedCombatantActor::ConfigureCombatant(
		const CombatantConfiguration& configuration
	)
	{
		mConfiguration = configuration;
		mConfiguration.maxHealthSnapshot = std::max(
			1.f,
			mConfiguration.maxHealthSnapshot
		);
		mConfiguration.baseArmor = std::max(0.f, mConfiguration.baseArmor);
		mConfiguration.ownerArmorScale = std::max(0.f, mConfiguration.ownerArmorScale);
		mConfiguration.ownerCriticalChanceScale = std::max(
			0.f,
			mConfiguration.ownerCriticalChanceScale
		);

		mHealthComponent.SetInitialHealth(
			mConfiguration.maxHealthSnapshot,
			mConfiguration.maxHealthSnapshot
		);
		mCombatRuntime.GetAbilitySystemComponent().GetAttributes().SetBaseValue(
			OwnerAttributeIds::MaxHealth,
			mConfiguration.maxHealthSnapshot
		);
		ConfigureFriendlySummonCollision();
		SynchronizeDynamicOwnerCombatAttributes();
	}

	float SummonedCombatantActor::GetOwnerCombatAttribute(
		const sas::AttributeId& attributeId
	) const
	{
		const Actor* owner = GetOwnerActor();
		const auto* ownerCombatant = owner
			? dynamic_cast<const Combatant*>(owner)
			: nullptr;
		return ownerCombatant
			? ownerCombatant->GetAbilitySystemComponent().GetAttributes().GetCurrentValue(
				attributeId
			)
			: 0.f;
	}

	float SummonedCombatantActor::GetOwnerAttackSpeedMultiplier() const
	{
		const float rating = std::max(
			0.f,
			GetOwnerCombatAttribute(OwnerAttributeIds::AttackSpeed)
		);
		return 1.f + rating / sas::AttributeMath::PercentageRatingScale;
	}

	void SummonedCombatantActor::ConfigureFriendlySummonCollision()
	{
		SetCollisionLayer(CollisionLayer::FriendlySummon);
		SetCollisionMask(CollisionLayer::Enemy | CollisionLayer::EnemyBullet);
	}

	void SummonedCombatantActor::SynchronizeDynamicOwnerCombatAttributes()
	{
		sas::AttributeSystem& attributes =
			mCombatRuntime.GetAbilitySystemComponent().GetAttributes();
		const float ownerArmor = std::max(
			0.f,
			GetOwnerCombatAttribute(OwnerAttributeIds::Armor)
		);
		const float ownerCriticalChance = std::max(
			0.f,
			GetOwnerCombatAttribute(OwnerAttributeIds::CriticalChance)
		);
		attributes.SetBaseValue(
			OwnerAttributeIds::Armor,
			mConfiguration.baseArmor + ownerArmor * mConfiguration.ownerArmorScale
		);
		attributes.SetBaseValue(
			OwnerAttributeIds::CriticalChance,
			ownerCriticalChance * mConfiguration.ownerCriticalChanceScale
		);
	}

	void SummonedCombatantActor::OnHealthEmpty()
	{
		mCombatRuntime.Clear();
		Destroy();
	}
}
