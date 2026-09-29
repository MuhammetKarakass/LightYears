---
type: balance
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - gameplay-effect modifier application
  - MovementSlow sequential reduction usage
  - GasLiteCoreTests
source_files:
  - SpaceAbilitySystem/include/attributes/AttributeSystem.h
  - SpaceAbilitySystem/src/attributes/AttributeSystem.cpp
  - SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h
  - LightYearsGame/src/gameplay/ability/actions/AbilityActionAttributeResolver.cpp
  - LightYearsGame/tests/GasLiteCoreTests.cpp
symbols:
  - sas::AttributeModifierOperation
  - sas::AttributeModifier
  - sas::AttributeScalingRule
  - sas::CalculateModifiedAttributeValue
  - sas::AttributeSystem::ApplyBaseModifier
  - sas::AttributeSystem::GetSequentialReductionMultiplier
related:
  - "[[Attribute System]]"
  - "[[Attribute Resolution Flow]]"
  - "[[Gameplay Effect Application Flow]]"
  - "[[Derived Attributes]]"
---

# Modifier Operations

## Operation kataloğu

| Operation | `AttributeSystem` / helper | `ApplyBaseModifier` | `AttributeScalingRule` |
|---|---|---|---|
| `Add` | Add toplamına magnitude ekler | `base += magnitude` | `value += source × coefficient` |
| `Multiply` | Multiply çarpımını magnitude ile çarpar | `base *= magnitude` | `value *= 1 + source × coefficient` |
| `Override` | En yüksek priority magnitude sonucu değiştirir | `base = magnitude` | `value = source × coefficient` |

Bu üç bağlam aynı enum'u kullanır fakat özellikle **Multiply semantiği aynı değildir**: runtime modifier doğrudan multiplier (`1.2`) bekler; scaling rule coefficient (`0.2`) üzerinden `1 + source×coefficient` üretir.

## Örnek hesap

Bu değerler yalnız açıklama örneğidir; balance değeri değildir:

```text
Base = 10
Add = +5
Multiply = ×2
Override yok
Min = 0, Max = 100

1. 10 + 5 = 15
2. 15 × 2 = 30
3. Override olmadığı için 30 korunur
4. clamp(30, 0, 100) = 30
```

Aynı hesap `GasLiteCoreTests.cpp` içinde AttackPower için gerçek test girdileriyle de doğrulanır.

## Kalıcı ve geçici değişiklik

- `ApplyBaseModifier`: instant effect/progression gibi kalıcı base değişikliği; handle üretmez ve geri alınamaz.
- `AddModifier`: handle üretir; süreli/infinite effect removal sırasında aynı handle kaldırılır.
- `RemoveModifier`: reverse lookup ile doğru attribute'i bulur ve tekrar çözer.
- `CalculateModifiedAttributeValue`: storage kurmadan bir `GameplayAttribute` + modifier listesi çözer.

`AttributeModifier` kendi source actor/effect bilgisini taşımaz. Kaynak ilişkisi ancak modifier bir active effect'ten geldiyse `ActiveGameplayEffect::source/sourceScope` ve `appliedModifierHandles` üzerinden dolaylı kurulabilir. Aynı modifier değer olarak iki kez eklenebilir; her `AddModifier` ayrı handle üretir ve ikisi de çözümde yer alır.

## Override kuralı

En yüksek integer `priority` kazanır. Eşit priority için koşul `>=` olduğundan son dolaşılan modifier kazanır. Modifier storage unordered olduğundan eşit-priority çakışmasında deterministik insertion-order kontratı yoktur.

## Sequential reduction istisnası

Reduction kaynakları normal Add toplamı olarak ele alınmaz:

```text
reductionToMultiplier(r) = 1 - clamp(r, -0.95, 0.95)
result = max(minimumMultiplier, baseMultiplier × eachModifierMultiplier)
```

Örnek: iki `%10` slow/haste reduction kaynağı `0.9 × 0.9 = 0.81` multiplier üretir.

## Kritik gerçek kod

Dosya: `SpaceAbilitySystem/include/attributes/AttributeSystem.h`  
Sınıf veya namespace: `ly`  
Fonksiyon: `AttributeModifierOperation`  
Görevi: Üç ortak operation kimliğini tanımlar.

```cpp
	enum class AttributeModifierOperation
	{
		Add,
		Multiply,
		Override
	};
```

Dosya: `SpaceAbilitySystem/include/attributes/AttributeSystem.h`  
Sınıf veya namespace: `ly`  
Fonksiyon: `CalculateModifiedAttributeValue`  
Görevi: Normal modifier sonucunu hesaplar.

```cpp
		float value = (attribute.baseValue + additive) * multiplicative;
		if (hasOverride)
		{
			value = overrideValue;
		}
		return std::clamp(value, attribute.minValue, attribute.maxValue);
```

Dosya: `SpaceAbilitySystem/src/attributes/AttributeSystem.cpp`  
Sınıf veya namespace: `sas::AttributeSystem`  
Fonksiyon: `ApplyBaseModifier`  
Görevi: Instant operation'ı doğrudan base değere işler.

```cpp
		case AttributeModifierOperation::Multiply:
			attribute.baseValue *= modifier.magnitude;
			break;
		case AttributeModifierOperation::Override:
			attribute.baseValue = modifier.magnitude;
			break;
```

Dosya: `SpaceAbilitySystem/src/attributes/AttributeSystem.cpp`  
Sınıf veya namespace: `sas`  
Fonksiyon: `ApplyAttributeScalings`  
Görevi: Scaling rule Add semantiğini uygular.

```cpp
				case AttributeModifierOperation::Add:
					value += sourceValue * scaling.coefficient;
					break;
```

## Test durumu

- Add + Multiply sırası ve modifier removal test edilir.
- Instant base Add test edilir.
- Sequential reduction iki kaynakla test edilir.
- Override ve priority tie doğrudan test edilmez.
- Multiply için `0`, negatif veya NaN magnitude validasyonu core içinde doğrulanamadı.
- Debug/Release `exit 0` ve 2026-07-29 CTest 2/2 sonucu tarihsel kayıttır;
  bu docs-only denetiminde güncel build/test çalıştırılmadı.

## Operasyon ayrıntı matrisi

| Operation | Enum/değer | Uygulanma sırası | Formül | Stacking davranışı | Test durumu |
|---|---|---:|---|---|---|
| Additive flat | `Add` | 1 | `Σ magnitude` | Toplanır | Directly Tested |
| Multiplicative | `Multiply` | 2 | `Π magnitude` | Çarpılır | Directly Tested |
| Override | `Override` | 3 | En yüksek priority magnitude | Tek kazanan; eşit priority belirsiz sıra | Not Tested |
| Minimum/Maximum | Ayrı operation yok | 4 | `clamp(value,min,max)` | Attribute sınırı | Indirectly Tested |
| Sequential reduction | Özel getter | Ayrı yol | `Π(1-clamp(reduction))` | Çarpılır, minimum multiplier | Directly Tested |
| Source-based scaling | `AttributeScalingRule` + aynı enum | Definition resolution | `source × coefficient` varyantları | Rule sırasıyla uygulanır | Indirectly Tested |
| Tag conditional modifier | `ConditionalAttributeModifier` | Attachment domain | Koşul sağlanırsa modifier listesine katılır | Bu görevde ayrıntılı izlenmedi | Could Not Verify |

## Durum ayrımı

### Aktif ve koddan doğrulandı

- Tablodaki tüm operation semantikleri kodda uygulanmıştır.

### Tanımlı fakat aktif kullanımı doğrulanamadı

- `ConditionalAttributeModifier` attachment domain'inde tanımlıdır; tüm koşul/capability kombinasyonlarının runtime kullanımı bu görevde izlenmedi.

### Planlanan

- Yeni operation türü veya operation sırasını değiştiren onaylı plan doğrulanamadı.
- Override tie-break ve validation testleri öneridir; uygulanmış özellik olarak gösterilmez.

### Test-only

- Test extension attribute/modifier'ları production operation kataloğunu genişletmez.
