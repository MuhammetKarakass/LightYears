---
type: balance
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
uncommitted_areas:
  - owner and ship attribute integrations
  - GasLiteCoreTests
source_files:
  - SpaceAbilitySystem/include/attributes/AttributeMath.h
  - LightYearsGame/src/gameplay/combat/CombatRuntime.cpp
  - LightYearsGame/src/gameplay/ship/ShipRuntime.cpp
  - LightYearsGame/src/gameplay/ability/actions/AbilityActionAttributeResolver.cpp
  - LightYearsGame/tests/GasLiteCoreTests.cpp
symbols:
  - sas::AttributeMath::SaturatingFraction
  - sas::AttributeMath::GetAbilityHasteReduction
  - sas::AttributeMath::GetCriticalChance
  - sas::AttributeMath::GetCombatLuckFactor
  - sas::AttributeMath::GetLootLuckFactor
  - sas::AttributeMath::GetArmorDamageReduction
  - ly::ShipRuntime::RecalculateAttributes
related:
  - "[[Attribute Catalog]]"
  - "[[Attribute System]]"
  - "[[Attribute Change Notification Flow]]"
  - "[[Modifier Operations]]"
---

# Derived Attributes

## Uygulanan merkezi eğriler

Temel doygun fonksiyon:

```text
S(rating, scale) = clamp(1 - exp(-rating / scale), 0, 1)
```

| Çıktı | Formül | Scale | Durum |
|---|---|---:|---|
| Ability haste reduction | `S(haste, 100)` | 100 | Active |
| Ability cooldown multiplier | `1 - hasteReduction` | 100 | Active |
| Critical chance | `1 - (1-baseChance) × (1-S(critical,100))` | 100 | Active |
| Combat luck factor | `S(luck, 100)` | 100 | Active |
| Loot luck factor | `S(luck, 300)` | 300 | Active |
| Armor reduction | `max(0, armor) / (max(0, armor) + 100)` | 100 (hyperbolic) | Active |

Armor için hyperbolic diminishing-return eğrisi kullanılır (`100 Armor = %50 DR`). Hard cap yoktur, sonlu zırhta %100'e ulaşmaz, asimptotik yaklaşır.

## Uygulanan ship türetmeleri

| Türetilmiş değer | Kaynak attribute'lar | Gerçek formül | Hesaplayan sembol | Güncellenme zamanı | Kullanıcı sistemler | Durum |
|---|---|---|---|---|---|---|
| Owner HealthRegen | MaxHealth | `max(0, MaxHealth) / 1200` | `ShipRuntime::RecalculateAttributes` | Initialization; MaxHealth/EnergyPower change callback | `SpaceShip` regeneration | Active |
| Ship MaxShield | EnergyPower + ship config | `max(0, baseMaxShield + (EnergyPower × 2) × ShieldAffinity)` | `ShipRuntime::RecalculateAttributes` | Initialization; EnergyPower callback | `ShieldComponent` | Active |
| AfterburnerCapacity | EnergyPower + ship config | `max(0, baseCapacity + (EnergyPower × 2) × AfterburnerAffinity)` | `ShipRuntime::RecalculateAttributes` | Initialization; EnergyPower callback | `EnergyComponent` | Active |
| AfterburnerRegen | AfterburnerCapacity + config | `capacity / max(0.001, fullRechargeDuration)` | `ShipRuntime::RecalculateAttributes` | Aynı recalculation | `EnergyComponent` | Active |
| Ship ShieldRegen | MaxShield + config | `maxShield / max(0.001, fullRechargeDuration)` | `ShipRuntime::RecalculateAttributes` | Aynı recalculation | `SpaceShip` regeneration | Active |
| Ability cooldown multiplier | AbilityHaste | `1 - S(AbilityHaste,100)` | `AttributeMath::GetAbilityCooldownMultiplier` | Cooldown çözümünde | `GameAbility::ResolveCooldownDuration` | Active |
| Critical chance | CriticalChance rating | `1-(1-base)×(1-S(rating,100))` | `CombatRuntime::GetCriticalChance` | Okuma/hasar çözümünde | Combat/damage | Active |
| Armor reduction | Armor | `max(0, Armor) / (max(0, Armor) + 100)` | `CombatRuntime::ProcessIncomingDamage` | Incoming damage | Combat/damage | Active |

AttackPower→damage, AttackSpeed→fire/heat ve movement→dash ilişkileri `AttributeScalingRule`/domain resolver'larıyla uygulanır. Vardırlar, fakat ability/weapon/movement ayrıntıları bu görevde genişletilmemiştir.

## Scaling rule bağlantısı

`AttributeScalingRule`, başka bir attribute'in resolved değerini ability/weapon-local bir hedef değere uygular:

```text
Add:      value += source × coefficient
Multiply: value *= 1 + source × coefficient
Override: value  = source × coefficient
```

Bu, owner `AttributeSystem::Recalculate` modifier sırasından ayrı bir definition-resolution katmanıdır.

## Kritik gerçek kod

Dosya: `SpaceAbilitySystem/include/attributes/AttributeMath.h`  
Sınıf veya namespace: `sas::AttributeMath`  
Fonksiyon: `SaturatingFraction`  
Görevi: Merkezi doygun rating eğrisini uygular.

```cpp
	inline float SaturatingFraction(float rating, float scale)
	{
		if (rating <= 0.f || scale <= 0.f)
		{
			return 0.f;
		}
		return std::clamp(1.f - std::exp(-rating / scale), 0.f, 1.f);
	}
```

Dosya: `SpaceAbilitySystem/include/attributes/AttributeMath.h`  
Sınıf veya namespace: `sas::AttributeMath`  
Fonksiyon: `GetCriticalChance`  
Görevi: Base chance ile rating katkısını kalan olasılık üzerinde birleştirir.

```cpp
	inline float GetCriticalChance(float criticalRating, float baseCriticalChance = 0.f)
	{
		const float baseChance = std::clamp(baseCriticalChance, 0.f, 1.f);
		return 1.f - (1.f - baseChance) * (1.f - SaturatingFraction(criticalRating, PercentageRatingScale));
	}
```

Dosya: `LightYearsGame/src/gameplay/ship/ShipRuntime.cpp`  
Sınıf veya namespace: `ly::ShipRuntime`  
Fonksiyon: `RecalculateAttributes`  
Görevi: EnergyPower'ten shield ve afterburner kapasitesini türetir.

```cpp
		const float reactorBudget = energyPower * 2.f;
		const float maxShield = std::max(0.f,
			mEnergyAttributes.baseMaxShield + reactorBudget * mEnergyAttributes.shieldAffinity
		);
		const float afterburnerCapacity = std::max(0.f,
			mEnergyAttributes.baseAfterburnerCapacity +
				reactorBudget * mEnergyAttributes.afterburnerAffinity
		);
```

Dosya: `LightYearsGame/src/gameplay/ability/actions/AbilityActionAttributeResolver.cpp`  
Sınıf veya namespace: `ly::AbilityActionAttributeResolver`  
Fonksiyon: `ResolveAttributes`  
Görevi: Scaling-rule operation semantiğini definition-local değere uygular.

```cpp
				case AttributeModifierOperation::Multiply:
					value *= 1.f + attributeValue * scaling.coefficient;
					break;
				case AttributeModifierOperation::Override:
					value = attributeValue * scaling.coefficient;
					break;
```

## Durum ayrımı

### Aktif ve koddan doğrulandı

Yukarıdaki rating, cooldown, armor ve ship-derived formüller production sembolleri ve test/integration kullanımlarıyla doğrulandı.

### Tanımlı fakat aktif kullanımı doğrulanamadı

Feature-local scaling rule'larının tüm katalog girdileri bu dar görevde caller seviyesine kadar izlenmedi; tabloda yalnız doğrudan doğrulanan merkezi örnekler Active sayıldı.

### Planlanan

- `BALANCE_AND_ROADMAP_NOTEBOOK.md` boş ArmorScale, Shield multiplier ve Crit multiplier deney satırları içerir; yeni hedef değer belirtilmediği için **plan değil şablon** olarak değerlendirilmiştir.
- Yeni core derived-stat formülü veya onaylanmış değişiklik doğrulanamadı.
- Movement rating için ayrı diminishing eğri vardır, fakat bu GAS-Lite merkezi `AttributeMath` kapsamı dışındaki movement domain davranışıdır.

### Test-only

Test helper'larında ölçüm amaçlı resolved değerler vardır; production derived attribute kataloğuna eklenmemiştir.

## Testler ve riskler

- Armor 100 → 0.5 (50 Armor ≈ 0.3333, 200 Armor ≈ 0.6667).
- Haste/crit/luck 500 → 1'den küçük.
- Combat luck 20 > combat luck 10.
- Ship derived capacity/regen ve owner değişim propagation'ı entegrasyon testleriyle kapsanır.
- Scale sabitleri değişirse combat'ın geniş bölümü etkilenir; golden-value kapsamı sınırlıdır.
- Bu docs-only denetiminde build/test çalıştırılmadı; önceki Debug/Release
  exit `0` kaydı tarihsel ve dirty worktree ile yeniden doğrulanmadı.
