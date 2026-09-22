---
type: walkthrough
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - GameplayEffectSystem lifecycle and behavior hooks
  - area and movement effect integrations
  - GasLiteCoreTests
source_files:
  - LightYearsGame/include/gameConfigs/combat/EffectStructs.h
  - SpaceAbilitySystem/include/effects/ActiveGameplayEffect.h
  - SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h
  - SpaceAbilitySystem/include/AbilitySystemComponent.h
  - SpaceAbilitySystem/src/AbilitySystemComponent.cpp
  - LightYearsGame/src/gameplay/combat/CombatRuntime.cpp
  - LightYearsGame/tests/GasLiteCoreTests.cpp
symbols:
  - ly::CombatRuntime::Tick
  - sas::AbilitySystemComponent::Tick
  - sas::GameplayEffectRuntimeSystem::Tick
  - sas::GameplayEffectRuntimeSystem::RefreshEffectDuration
  - sas::GameplayEffectRuntimeSystem::RemoveEffect
  - sas::GameplayEffectRuntimeSystem::RemoveEffectAt
  - sas::GameplayEffectRuntimeSystem::Clear
related:
  - "[[Gameplay Effect System]]"
  - "[[Gameplay Effect Application Flow]]"
  - "[[Frame Update Flow]]"
  - "[[Ownership and Lifetime]]"
---

# Gameplay Effect Duration and Removal Flow

## Amaç

Aktif effect'in frame tick, explicit refresh, behavior-driven removal, duration expiry, manual remove ve runtime clear yollarını tek cleanup noktasına kadar izler.

## Akış

```mermaid
flowchart TD
    Combat["CombatRuntime::Tick"] --> Tick["AbilitySystemComponent::Tick"]
    Tick --> Behavior["GameplayEffectBehavior::Tick"]
    Behavior --> BehaviorRemove{"removeEffect?"}
    BehaviorRemove -- Evet --> RemoveAt["RemoveEffectAt(index)"]
    BehaviorRemove -- Hayır --> Duration{"Policy == Duration?"}
    Duration -- Evet --> Dec["remainingDuration -= deltaTime"]
    Dec --> Expired{"<= 0?"}
    Expired -- Evet --> RemoveAt
    Expired -- Hayır --> Sync["notify + synchronize"]
    Duration -- Hayır --> Sync
    Manual["RemoveEffect(handle)"] --> RemoveAt
    Clear["Clear()"] --> RemoveAt
    RemoveAt --> Tags["RemoveGrantedTags"]
    Tags --> Modifiers["RemoveModifiers"]
    Modifiers --> Visual["DestroyVisual"]
    Visual --> Erase["erase active entry + delegates"]
```

## Policy davranışı

| Policy | Tick davranışı | Normal bitiş |
|---|---|---|
| `Instant` | Active list'e girmez | Apply çağrısında applied/removed |
| `Duration` | Sayaç her tick azalır | `remainingDuration <= 0` |
| `Infinite` | Sayaç azalmaz | Behavior, manual remove veya clear |

`RefreshEffectDuration`, yalnız Duration policy için remaining ve total duration değerlerini spec duration'a resetler; modifier/tag/visual yeniden kurulmaz.

## Kritik gerçek kod

Dosya: `LightYearsGame/src/gameplay/combat/CombatRuntime.cpp`  
Sınıf veya namespace: `ly::CombatRuntime`  
Fonksiyon: `Tick`  
Görevi: Effect'leri aynı frame içinde ability'lerden önce günceller.

```cpp
	void CombatRuntime::Tick(float deltaTime)
	{
		mAbilitySystemComponent.Tick(deltaTime);
	}
```

Dosya: `SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h`  
Sınıf veya namespace: `sas::GameplayEffectRuntimeSystem`  
Fonksiyon: `Tick`  
Görevi: Duration sayacını azaltır ve expiry'de ortak removal yoluna girer.

```cpp
			if (effect.spec.definition.durationPolicy == GameplayEffectDurationPolicy::Duration)
			{
				if (effect.TickDuration(deltaTime))
				{
					RemoveEffectAt(i);
					continue;
				}
```

Dosya: `SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h`  
Sınıf veya namespace: `sas::GameplayEffectRuntimeSystem`  
Fonksiyon: `RefreshEffectDuration`  
Görevi: Duration state'i yıkmadan süreyi spec değerine resetler.

```cpp
			effect.RefreshDuration(effect.spec.duration);
			SynchronizeVisual(effect);
			onEffectChanged.Broadcast(effect.handle);
			onEffectsChanged.Broadcast();
			return true;
```

Dosya: `SpaceAbilitySystem/include/effects/GameplayEffectRuntimeSystem.h`  
Sınıf veya namespace: `sas::GameplayEffectRuntimeSystem`  
Fonksiyon: `Clear`  
Görevi: Storage'ı doğrudan silmek yerine her effect'i normal cleanup'tan geçirir.

```cpp
	void GameplayEffectRuntimeSystem::Clear()
	{
		for (const GameplayEffectHandle handle : GetHandles())
		{
			RemoveEffect(handle);
		}
	}
```

## Test ve kaynak doğrulaması

- Barrier behavior remove talebi ve runtime cleanup bağlantısı test edilir.
- Infinite barrier tick boyunca kalır ve runtime attributes değişir.
- Gravity Anomaly inside effect'i içeride refresh edilir; çıkış sonrası iki saniyelik tail sonunda kaldırılır.
- Manual `RemoveEffect` hareket değerini geri yükleyen integration testinde kullanılır.
- Bu docs-only denetiminde build/test çalıştırılmadı; önceki Debug/Release
  exit `0` kaydı tarihsel.

## Kapsam boşlukları

- Sıfır/negatif duration'ın ilk tick'e kadar kısa süreli active state yaratması izole test edilmemiştir.
- `RefreshEffectDuration` çağrısının modifier/tag handle sayısını değiştirmediğini doğrudan assertion ile doğrulayan test bulunamadı.
- Delegate reentrancy ve remove sırasında callback'in listeyi değiştirmesi doğrulanamadı.

## Okuma sırası

1. Duration/stack enum'ları
2. `ActiveGameplayEffect`
3. `CombatRuntime::Tick`
4. `GameplayEffectRuntimeSystem::Tick`
5. `RefreshEffectDuration`
6. `RemoveEffectAt` ve `Clear`
