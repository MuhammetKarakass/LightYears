---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - GameAbilityActionExecutor action dispatch and integration code
source_files:
  - LightYearsGame/include/gameplay/ability/GameAbilityActionExecutor.h
  - LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp
  - SpaceAbilitySystem/include/abilities/AbilityExecution.h
symbols:
  - ly::GameAbilityActionExecutor
  - ly::GameAbilityActionExecutor::BeginExecution
  - ly::GameAbilityActionExecutor::TickExecution
  - ly::GameAbilityActionExecutor::EndExecution
  - ly::GameAbilityActionExecutor::ExecuteAction
related:
  - "[[Ability Execution System]]"
  - "[[Ability Action Execution Flow]]"
  - "[[AbilityInstance]]"
---

# GameAbilityActionExecutor

## Sorumluluk

`ly::GameAbilityActionExecutor`, stateless static execution dispatcher'ıdır. Caller-owned `GameAbilityExecution` içindeki action'ları phase'e göre başlatır, aktif frame'lerde tick eder ve end cleanup'ını yürütür.

## Sorumlu olmadığı işler

- Ability instance veya global execution koleksiyonu sahiplenmek.
- Cooldown/charge veya ability active state'i yönetmek.
- Behavior family kararlarını ya da weapon/projectile/presentation domain implementasyonlarını sahiplenmek.

## Sahiplik ve lifecycle

- Execution sahibi: `GameAbility::mExecution`.
- Executor sahibi: Yok; yalnız static method yüzeyi.
- Context: caller tarafından stack üzerinde oluşturulan `AbilityExecutionContext`.
- Begin: `GameAbility::BeginExecution`.
- Tick: `GameAbility::TickExecution`.
- End/cancel: `GameAbility::EndExecution`.

## Dispatch ve bağlantılar

`AbilityActionData` için `std::visit` + compile-time `if constexpr` dispatch kullanılır. Action runtime state'i `ActiveAbilityAction::runtimeState` variant'ında tutulur: normal repeated action accumulator/count, FireWeapon için typed runtime state. World/Actor tarafına spawn/movement service çağrıları, effect tarafına `ApplyGameplayEffect`, event tarafına `HandleGameplayEvent` geçişi vardır.

## Önemli kod

Dosya: `SpaceAbilitySystem/include/abilities/AbilityExecution.h` ve `LightYearsGame/include/gameplay/ability/GameAbilityActionExecutor.h`  
Sınıf: `AbilityExecution` / `ActiveAbilityAction`  
Fonksiyon: veri tanımı  
Görev: Caller-owned action listesi ile action-local runtime state'i ayırır.

```cpp
	struct ActiveAbilityAction
	{
		const AbilityActionSpec* spec = nullptr;
		AbilityActionRuntimeState runtimeState;
	};

	struct GameAbilityExecution
	{
		List<ActiveAbilityAction> actions;
	};
```

Dosya: `LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp`  
Sınıf: `GameAbilityActionExecutor`  
Fonksiyon: `TickExecution`  
Görev: Her active action'a aynı frame'de tick yönlendirir; generic serial queue yoktur.

```cpp
	void GameAbilityActionExecutor::TickExecution(GameAbilityExecution& execution, AbilityExecutionContext& context, float deltaTime)
	{
		for (ActiveAbilityAction& action : execution.actions)
		{
			TickAction(action, context, deltaTime);
		}
	}
```

Dosya: `LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp`  
Sınıf: `GameAbilityActionExecutor`  
Fonksiyon: `ExecuteAction`  
Görev: Spawn action'ını ability actor registry geçişine bağlar. Kesit, gerçek branch'in ilgili satırlarıdır.

```cpp
				weak_ptr<AbilityWorldActor> spawnedActor = AbilityActorRegistry::Spawn(
					AbilityActorSpawnContext{
						owner,
						*actorDefinition,
						values,
						ResolveAbilityActorTargetLocation(owner, actionData, context)
					}
				);
```

## Kod okuma sırası

1. `GameAbilityDefinition.h::AbilityActionSpec`
2. `SpaceAbilitySystem/include/abilities/AbilityExecution.h::AbilityExecution`
3. `GameAbilityActionExecutor.cpp::BeginExecution`
4. `GameAbilityActionExecutor.cpp::TickAction`
5. `GameAbilityActionExecutor.cpp::ExecuteAction`
6. `GameAbilityActionExecutor.cpp::EndExecution`

## Sonraki inceleme

AbilityBehavior family'leri ile SpawnActor/FireWeapon action'larının domain implementasyonları sonraki belgelerde ele alınmalıdır.
