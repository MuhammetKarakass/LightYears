#include "gameplay/HealthComponent.h"
#include "gameplay/ShieldComponent.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
	using namespace ly;
	constexpr float Tolerance = 0.001f;

	struct ResourceChangeRecorder
	{
		int count = 0;
		float amount = 0.f;
		float current = 0.f;
		float maximum = 0.f;

		void Record(float changedAmount, float currentValue, float maxValue)
		{
			++count;
			amount = changedAmount;
			current = currentValue;
			maximum = maxValue;
		}

		void Reset()
		{
			count = 0;
			amount = 0.f;
			current = 0.f;
			maximum = 0.f;
		}
	};

	bool NearlyEqual(float actual, float expected)
	{
		return std::abs(actual - expected) <= Tolerance;
	}

	bool Expect(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << "FAILED: " << message << '\n';
			return false;
		}
		return true;
	}

	bool TestTemporalRecallOverhealthAndCryostasisRegeneration()
	{
		ResourceChangeRecorder changes;
		ly::HealthComponent health{ 100.f, 100.f };
		health.onHealthChanged.BindAction(&changes, &ResourceChangeRecorder::Record);

		const float granted = health.GrantTemporaryOverhealth(
			"Ability.Utility.TemporalRecall.Basic",
			40.f,
			2.f,
			10.f
		);
		if (!Expect(NearlyEqual(granted, 40.f) && NearlyEqual(health.GetHealth(), 140.f),
			"Temporal Recall temporary overhealth grant was not applied") ||
			!Expect(changes.count == 1 && NearlyEqual(changes.amount, 40.f),
			"Overhealth grant did not emit one accurate health notification"))
		{
			return false;
		}

		changes.Reset();
		const float cryostasisRegenPerSecond = 8.f + 100.f * 0.04f;
		health.Regenerate(cryostasisRegenPerSecond * 0.25f);
		if (!Expect(NearlyEqual(health.GetHealth(), 140.f),
			"Cryostasis regeneration changed health while temporary overhealth was active") ||
			!Expect(changes.count == 0,
			"A no-op heal while over maximum emitted a health notification"))
		{
			return false;
		}

		health.ChangeHealth(-45.f);
		if (!Expect(NearlyEqual(health.GetHealth(), 95.f),
			"Damage did not consume temporary overhealth before normal health"))
		{
			return false;
		}
		changes.Reset();
		health.Regenerate(cryostasisRegenPerSecond * 0.25f);
		health.Regenerate(cryostasisRegenPerSecond * 0.25f);
		return Expect(NearlyEqual(health.GetHealth(), 100.f),
			"Cryostasis regeneration exceeded normal maximum health") &&
			Expect(changes.count == 2 && NearlyEqual(changes.amount, 2.f) &&
				NearlyEqual(changes.current, 100.f) && NearlyEqual(changes.maximum, 100.f),
			"Cryostasis healing notifications did not report the capped resource value");
	}

	bool TestShieldHealingAndDamage()
	{
		ResourceChangeRecorder changes;
		ly::ShieldComponent shield{ 100.f, 100.f, 3.f };
		shield.onShieldChanged.BindAction(&changes, &ResourceChangeRecorder::Record);
		const float granted = shield.GrantTemporaryOvershield(
			"Ability.Utility.TemporalRecall.Basic",
			40.f,
			2.f,
			10.f
		);
		if (!Expect(NearlyEqual(granted, 40.f) && NearlyEqual(shield.GetShield(), 140.f),
			"Temporary overshield grant was not applied"))
		{
			return false;
		}

		changes.Reset();
		shield.ChangeShield(25.f);
		if (!Expect(NearlyEqual(shield.GetShield(), 140.f) && changes.count == 0,
			"Ordinary shield restoration erased or increased temporary overshield"))
		{
			return false;
		}

		const float absorbed = shield.AbsorbDamage(45.f, 1.f, 0.f);
		if (!Expect(NearlyEqual(absorbed, 45.f) && NearlyEqual(shield.GetShield(), 95.f),
			"Shield damage did not consume overshield before normal shield"))
		{
			return false;
		}
		shield.ChangeShield(20.f);
		return Expect(NearlyEqual(shield.GetShield(), 100.f),
			"Ordinary shield restoration exceeded normal maximum") &&
			Expect(changes.count == 2 && NearlyEqual(changes.current, 100.f) &&
				NearlyEqual(changes.maximum, 100.f),
			"Shield changes did not emit only accurate value notifications");
	}

	bool TestMaximumReductionPreservesAbsoluteExcess()
	{
		ResourceChangeRecorder healthChanges;
		ly::HealthComponent health{ 100.f, 100.f };
		health.onHealthChanged.BindAction(&healthChanges, &ResourceChangeRecorder::Record);
		health.GrantTemporaryOverhealth("Recall", 40.f, 0.f, 10.f);
		healthChanges.Reset();
		health.SetMaxHealth(80.f);
		if (!Expect(NearlyEqual(health.GetHealth(), 120.f) && NearlyEqual(health.GetMaxHealth(), 80.f),
			"Health maximum reduction did not preserve the 40-point absolute excess") ||
			!Expect(healthChanges.count == 1 && NearlyEqual(healthChanges.amount, -20.f) &&
				NearlyEqual(healthChanges.current, 120.f) && NearlyEqual(healthChanges.maximum, 80.f),
			"Health maximum reduction notification did not report the new values"))
		{
			return false;
		}
		health.TickTemporaryOverhealths(1.f);
		health.ChangeHealth(-25.f);
		if (!Expect(NearlyEqual(health.GetHealth(), 85.f),
			"Health decay or damage did not reconcile retained overhealth"))
		{
			return false;
		}

		ResourceChangeRecorder shieldChanges;
		ly::ShieldComponent shield{ 100.f, 100.f, 3.f };
		shield.onShieldChanged.BindAction(&shieldChanges, &ResourceChangeRecorder::Record);
		shield.GrantTemporaryOvershield("Recall", 40.f, 0.f, 10.f);
		shieldChanges.Reset();
		shield.SetMaxShield(80.f);
		if (!Expect(NearlyEqual(shield.GetShield(), 120.f) && NearlyEqual(shield.GetMaxShield(), 80.f),
			"Shield maximum reduction did not preserve the 40-point absolute excess") ||
			!Expect(shieldChanges.count == 1 && NearlyEqual(shieldChanges.amount, -20.f) &&
				NearlyEqual(shieldChanges.current, 120.f) && NearlyEqual(shieldChanges.maximum, 80.f),
			"Shield maximum reduction notification did not report the new values"))
		{
			return false;
		}
		shield.TickTemporaryOvershields(1.f);
		shield.AbsorbDamage(25.f, 1.f, 0.f);
		return Expect(NearlyEqual(shield.GetShield(), 85.f),
			"Shield decay or damage did not reconcile retained overshield");
	}

	bool TestMaximumIncreaseAndPercentPreservation()
	{
		ResourceChangeRecorder healthChanges;
		ly::HealthComponent health{ 100.f, 100.f };
		health.onHealthChanged.BindAction(&healthChanges, &ResourceChangeRecorder::Record);
		health.GrantTemporaryOverhealth("Recall", 40.f, 0.f, 10.f);
		healthChanges.Reset();
		health.SetMaxHealth(120.f);
		if (!Expect(NearlyEqual(health.GetHealth(), 160.f),
			"Increasing maximum health did not preserve absolute excess") ||
			!Expect(healthChanges.count == 1 && NearlyEqual(healthChanges.amount, 20.f) &&
				NearlyEqual(healthChanges.maximum, 120.f),
			"Health HUD notification omitted the cap and total change"))
		{
			return false;
		}
		health.TickTemporaryOverhealths(1.f);
		if (!Expect(NearlyEqual(health.GetHealth(), 150.f),
			"Health ledger did not decay the preserved excess after cap growth"))
		{
			return false;
		}

		ly::HealthComponent absorbedHealth{ 100.f, 100.f };
		absorbedHealth.GrantTemporaryOverhealth("Recall", 40.f, 0.f, 10.f);
		absorbedHealth.SetMaxHealth(150.f);
		absorbedHealth.TickTemporaryOverhealths(1.f);
		if (!Expect(NearlyEqual(absorbedHealth.GetHealth(), 180.f),
			"Large maximum increase lost the preserved overhealth ledger"))
		{
			return false;
		}

		ly::HealthComponent scaledHealth{ 100.f, 100.f };
		scaledHealth.GrantTemporaryOverhealth("Recall", 40.f, 0.f, 10.f);
		scaledHealth.SetMaxHealth(80.f, true);
		if (!Expect(NearlyEqual(scaledHealth.GetHealth(), 120.f),
			"preserveHealthPercent scaled the temporary excess instead of only normal health"))
		{
			return false;
		}

		ResourceChangeRecorder shieldChanges;
		ly::ShieldComponent shield{ 100.f, 100.f, 3.f };
		shield.onShieldChanged.BindAction(&shieldChanges, &ResourceChangeRecorder::Record);
		shield.GrantTemporaryOvershield("Recall", 40.f, 0.f, 10.f);
		shieldChanges.Reset();
		shield.SetMaxShield(120.f);
		if (!Expect(NearlyEqual(shield.GetShield(), 160.f),
			"Increasing maximum shield did not preserve absolute excess") ||
			!Expect(shieldChanges.count == 1 && NearlyEqual(shieldChanges.amount, 20.f) &&
				NearlyEqual(shieldChanges.maximum, 120.f),
			"Shield HUD notification omitted the cap and total change"))
		{
			return false;
		}
		shield.TickTemporaryOvershields(1.f);
		if (!Expect(NearlyEqual(shield.GetShield(), 150.f),
			"Shield ledger did not decay the preserved excess after cap growth"))
		{
			return false;
		}

		ly::ShieldComponent absorbedShield{ 100.f, 100.f, 3.f };
		absorbedShield.GrantTemporaryOvershield("Recall", 40.f, 0.f, 10.f);
		absorbedShield.SetMaxShield(150.f);
		absorbedShield.TickTemporaryOvershields(1.f);
		if (!Expect(NearlyEqual(absorbedShield.GetShield(), 180.f),
			"Large maximum increase lost the preserved overshield ledger"))
		{
			return false;
		}

		ly::ShieldComponent scaledShield{ 100.f, 100.f, 3.f };
		scaledShield.GrantTemporaryOvershield("Recall", 40.f, 0.f, 10.f);
		scaledShield.SetMaxShield(80.f, true);
		return Expect(NearlyEqual(scaledShield.GetShield(), 120.f),
			"preserveShieldPercent scaled the temporary excess instead of only normal shield");
	}

	bool TestMultipleTemporarySourcesExpireIndependently()
	{
		ly::HealthComponent health{ 100.f, 100.f };
		health.GrantTemporaryOverhealth("Recall.A", 20.f, 1.f, 10.f);
		health.GrantTemporaryOverhealth("Recall.B", 30.f, 2.f, 20.f);
		health.ChangeHealth(-10.f);
		health.TickTemporaryOverhealths(1.5f);
		if (!Expect(NearlyEqual(health.GetHealth(), 135.f),
			"The first health source did not decay after its hold expired"))
		{
			return false;
		}
		health.TickTemporaryOverhealths(0.5f);
		health.TickTemporaryOverhealths(0.25f);
		if (!Expect(NearlyEqual(health.GetHealth(), 125.f),
			"The second health source did not retain its independent hold duration"))
		{
			return false;
		}
		health.TickTemporaryOverhealths(1.25f);
		if (!Expect(NearlyEqual(health.GetHealth(), 100.f),
			"Health did not return to normal maximum when both temporary sources expired"))
		{
			return false;
		}

		ly::ShieldComponent shield{ 100.f, 100.f, 3.f };
		shield.GrantTemporaryOvershield("Recall.A", 20.f, 1.f, 10.f);
		shield.GrantTemporaryOvershield("Recall.B", 30.f, 2.f, 20.f);
		shield.ChangeShield(-10.f);
		shield.TickTemporaryOvershields(1.5f);
		if (!Expect(NearlyEqual(shield.GetShield(), 135.f),
			"The first shield source did not decay after its hold expired"))
		{
			return false;
		}
		shield.TickTemporaryOvershields(0.5f);
		shield.TickTemporaryOvershields(0.25f);
		if (!Expect(NearlyEqual(shield.GetShield(), 125.f),
			"The second shield source did not retain its independent hold duration"))
		{
			return false;
		}
		shield.TickTemporaryOvershields(1.25f);
		return Expect(NearlyEqual(shield.GetShield(), 100.f),
			"Shield did not return to normal maximum when both temporary sources expired");
	}

	bool TestMaximumInputPolicy()
	{
		ly::HealthComponent health{ 100.f, 100.f };
		health.SetMaxHealth(0.f);
		health.SetMaxHealth(-10.f);
		if (!Expect(NearlyEqual(health.GetMaxHealth(), 100.f) && NearlyEqual(health.GetHealth(), 100.f),
			"Zero or negative maximum health no longer leaves the resource unchanged"))
		{
			return false;
		}
		health.SetMaxHealth(std::numeric_limits<float>::quiet_NaN());
		if (!Expect(std::isnan(health.GetMaxHealth()) && NearlyEqual(health.GetHealth(), 100.f),
			"NaN maximum health behavior changed"))
		{
			return false;
		}

		ly::ShieldComponent shield{ 100.f, 100.f, 3.f };
		shield.SetMaxShield(-10.f);
		return Expect(NearlyEqual(shield.GetMaxShield(), 0.f) && NearlyEqual(shield.GetShield(), 0.f),
			"Negative maximum shield no longer clamps to zero");
	}
}

int main()
{
	if (!TestTemporalRecallOverhealthAndCryostasisRegeneration() ||
		!TestShieldHealingAndDamage() ||
		!TestMaximumReductionPreservesAbsoluteExcess() ||
		!TestMaximumIncreaseAndPercentPreservation() ||
		!TestMultipleTemporarySourcesExpireIndependently() ||
		!TestMaximumInputPolicy())
	{
		return 1;
	}

	std::cout << "Temporary overcap tests passed.\n";
	return 0;
}
