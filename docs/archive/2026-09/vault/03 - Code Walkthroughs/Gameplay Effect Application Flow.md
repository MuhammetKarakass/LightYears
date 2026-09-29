---
type: walkthrough
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - GameplayEffectSpec, content and validation
  - GameplayEffectSystem and producer integrations
  - GasLiteCoreTests
source_files:
  - SpaceAbilitySystem/include/effects/GameplayEffectSpec.h
  - SpaceAbilitySystem/include/effects/GameplayEffectRuntimeState.h
  - SpaceAbilitySystem/include/effects/GameplayEffectBindings.h
  - LightYearsGame/include/gameConfigs/combat/EffectStructs.h
  - SpaceAbilitySystem/include/AbilitySystemComponent.h
  - SpaceAbilitySystem/src/AbilitySystemComponent.cpp
  - SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h
  - LightYearsGame/tests/GasLiteCoreTests.cpp
symbols:
  - ly::MakeGameplayEffectSpec
  - sas::AbilitySystemComponent::CanApplyGameplayEffect
  - sas::AbilitySystemComponent::ApplyGameplayEffect
  - sas::GameplayEffectRuntimeSystem::Initialize
  - sas::GameplayEffectRuntimeSystem::ApplyGameplayEffectModifiers
  - sas::GameplayEffectRuntimeSystem::GrantGameplayEffectTags
related:
  - "[[Gameplay Effect System]]"
  - "[[Gameplay Effect Duration and Removal Flow]]"
  - "[[Gameplay Tag Definition and Lookup Flow]]"
  - "[[Attribute Resolution Flow]]"
---

# Gameplay Effect Application Flow

## Amaç

Definition veya resolved spec'in hedef `AbilitySystemComponent`'ına uygulanmasını; gate, instant, refresh, stack ve yeni active dallarıyla açıklar.

## Akış

```mermaid
sequenceDiagram
    participant P as Producer
    participant Spec as GameplayEffectSpec
    participant ES as AbilitySystemComponent
    participant Tags as OwnedTags
    participant Attr as AttributeSystem
    participant Hook as GameplayEffectBehavior
    P->>Spec: Make/copy and resolve
    P->>ES: ApplyEffect(spec, context)
    ES->>Tags: required all? blocked any?
    alt rejected
        ES-->>P: invalid handle
    else Instant
        ES->>Attr: ApplyBaseModifier(each)
        ES-->>P: new handle; applied then removed events
    else Refresh/Stack match
        ES->>ES: update existing ActiveGameplayEffect
        ES->>Attr: rebuild or add modifier handles
        ES->>Hook: Refresh / AddStack
        ES-->>P: existing handle
    else new active
        ES->>Hook: Initialize
        ES->>Attr: AddModifier(each)
        ES->>Tags: AddTag(each granted)
        ES-->>P: new active handle
    end
```

## Adım adım

1. Definition overload'ları `MakeGameplayEffectSpec` çağırır.
2. Source actor/context overload'ları kanonik `ApplyEffect(spec, context)` fonksiyonuna iner.
3. `CanApplyEffect`, SAS binding helper üzerinden required ve blocked owned tag'leri kontrol eder.
4. Her kabul edilen çağrı yeni handle ID ayırır; refresh/stack eşleşirse bu yeni ID dışarı dönmez.
5. Instant effect modifier'ları base'e kalıcı işler ve active list'e girmez.
6. Refresh/Stack yalnız policy `None` değilse aynı `effectId` üzerinde arama yapar; source-scoped effect ayrıca `sourceScope` eşitliği ister.
7. Eşleşme yoksa yeni `ActiveGameplayEffect` oluşur, behavior initialize edilir, modifier/tag/visual bağlanır.

`ValidateGameplayEffectDefinition` ve shipped catalog validation, content registration yolunda çalışır; `AbilitySystemComponent::ApplyGameplayEffect` içindeki gate, SAS runtime'ın required/blocked tag kontrolü ve duration/stack ön koşullarıdır. Bu ayrım nedeniyle doğrudan runtime'da elde oluşturulmuş invalid definition için core apply tam schema doğrulaması yapmaz.

## Kritik gerçek kod

Dosya: `SpaceAbilitySystem/src/AbilitySystemComponent.cpp`  
Sınıf veya namespace: `sas::AbilitySystemComponent`  
Fonksiyon: definition overload  
Görevi: Definition'ı spec'e kopyalar ve kanonik yola gönderir.

```cpp
	GameplayEffectHandle AbilitySystemComponent::ApplyGameplayEffect(
		const GameplayEffectDefinition& definition,
		const GameplayEffectSourceContext& context
	)
	{
		return mEffects.ApplyEffect(definition, context);
	}
```

Dosya: `SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h`  
Sınıf veya namespace: `sas::GameplayEffectRuntimeSystem`  
Fonksiyon: kanonik `ApplyEffect`  
Görevi: Tag gate başarısızsa invalid handle döndürür.

```cpp
		const GameplayEffectDefinition& definition = spec.definition;
		if (!CanApplyEffect(definition) || spec.maxStacks < 1)
		{
			return {};
		}
```

Dosya: `SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h`  
Sınıf veya namespace: `sas::GameplayEffectRuntimeSystem`  
Fonksiyon: `ApplyEffect` refresh eşleşmesi  
Görevi: Eski runtime sahipliklerini söküp yeni spec/context ile yeniden kurar.

```cpp
					RemoveGameplayEffectTags(stackingTarget->spec.definition, mOwnedTags);
					RemoveGameplayEffectModifiers(*stackingTarget, mAttributes);
					NotifyRemoving(*stackingTarget);
					stackingTarget->spec = spec;
					BindSource(*stackingTarget, context);
					Refresh(*stackingTarget);
```

Dosya: `SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h`  
Sınıf veya namespace: `sas::GameplayEffectRuntimeSystem`  
Fonksiyon: `ApplyGameplayEffectModifiers`  
Görevi: Attribute modifier handle'larını active effect'e bağlar.

```cpp
		ApplyGameplayEffectModifiers(
			effect.spec,
			effect,
			mAttributes
		);
```

## Test ve kaynak doğrulaması

- Catalog definition → bağımsız spec kopyaları doğrulanır.
- Invalid behavior/visual validation doğrulanır.
- Gravity Anomaly gerçek source-scoped refresh yolu ve MovementSlow entegrasyonu kapsar.
- Generic required/blocked gate ve tüm policy kombinasyonları ayrı küçük testlerde doğrulanamadı.
- Bu docs-only denetiminde build/test çalıştırılmadı; önceki Debug/Release
  executable exit `0` kaydı tarihsel ve dirty worktree ile yeniden doğrulanmadı.

## Okuma sırası

1. `GameplayEffectDefinition`
2. `GameplayEffectSpec` ve `MakeGameplayEffectSpec`
3. Application context / Active state
4. Dört public overload
5. Kanonik `ApplyEffect`
6. Modifier/tag helper'ları
