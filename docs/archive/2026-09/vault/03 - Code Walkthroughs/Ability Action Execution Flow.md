---
type: walkthrough
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
uncommitted_areas:
  - GameAbilityActionExecutor action dispatch and integration code
source_files:
  - LightYearsGame/src/gameplay/ability/GameAbility.cpp
  - LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp
  - LightYearsGame/include/gameplay/ability/GameAbilityActionExecutor.h
symbols:
  - ly::GameAbility::BeginExecution
  - ly::GameAbility::TickExecution
  - ly::GameAbility::EndExecution
  - ly::GameAbilityActionExecutor::BeginExecution
  - ly::GameAbilityActionExecutor::TickExecution
  - ly::GameAbilityActionExecutor::ExecuteAction
  - ly::GameAbilityActionExecutor::EndExecution
related:
  - "[[Ability Execution System]]"
  - "[[AbilityExecutor]]"
  - "[[Ability System]]"
---

# Ability Action Execution Flow

## Akış

```mermaid
sequenceDiagram
    participant AI as GameAbility
    participant EX as GameAbilityActionExecutor
    participant Act as ActiveAbilityAction
    participant GE as AbilitySystemComponent
    participant Actor as Actor / World actor registry

    AI->>EX: BeginExecution(mExecution, context)
    EX->>EX: Add OnActivate + WhileActive specs
    EX->>Act: ExecuteAction(OnActivate)
    loop each active frame while ability is active
        AI->>EX: TickExecution(mExecution, context, deltaTime)
        EX->>Act: TickAction(each WhileActive action)
        Act->>EX: ExecuteAction when interval/count permits
        alt ApplyEffectAction
            EX->>GE: ApplyEffect(effectSpec, owner)
        else ApplyImpulseAction
            EX->>Actor: MovementInfluenceService::ApplyImpulse(...)
        else SpawnActorAction
            EX->>Actor: AbilityActorRegistry::Spawn(...)
        end
    end
    AI->>EX: EndExecution(mExecution, context, reason)
    EX->>EX: close FireWeapon + execute OnEnd + clear actions
```

## Adım ayrıntıları

| Adım | Dosya / sembol | Okunan veri | Değişen runtime state | Sonraki sistem |
|---|---|---|---|---|
| Activation sonrası | `GameAbility::ActivateContent` | `mDefinition`, behavior | Lifecycle event ve içerik aktivasyonu | `BeginExecution` |
| Begin | `GameAbilityActionExecutor.cpp::BeginExecution` | Definition `actions`, phase | OnActivate/WhileActive `ActiveAbilityAction` kayıtları | `ExecuteAction` |
| Frame tick | `GameAbility::TickExecution` | `mDefinition`, instance | Context oluşturulur | `TickExecution` |
| WhileActive | `GameAbilityActionExecutor.cpp::TickAction` | interval, max count, runtime variant | SAS repeated scheduler state veya game-owned weapon state | `ExecuteAction` |
| Dispatch | `GameAbilityActionExecutor.cpp::ExecuteAction` | Variant payload, owner, target policy | Action-local runtime state olabilir | Effect, Actor, event, weapon veya actor registry |
| Ability end | `GameAbilityActionExecutor.cpp::EndExecution` | Active actions, end phase | Weapon state kapatılır; action listesi clear | OnEnd action dispatch |

## Kritik gerçek kod

Dosya: `LightYearsGame/src/gameplay/ability/GameAbility.cpp`  
Sınıf: `GameAbility`  
Fonksiyon: `TickExecution`  
Görev: Game ability context'ini oluşturup action executor'a her aktif frame taşır.

```cpp
	void GameAbility::TickExecution(float deltaTime)
	{
		AbilityExecutionContext context{
			&mAbilitySystem,
			&mDefinition,
			nullptr,
			this
		};
		GameAbilityActionExecutor::TickExecution(mExecution, context, deltaTime);
	}
```

Dosya: `LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp`  
Sınıf: `GameAbilityActionExecutor`  
Fonksiyon: `ExecuteAction`  
Görev: Variant payload'ını ziyaret ederek impulse action'ını movement service yoluna bağlar.

```cpp
			else if constexpr (std::is_same_v<T, ApplyImpulseAction>)
			{
				ExecuteApplyImpulseAction(actionData, owner);
			}
```

Dosya: `LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp`  
Sınıf: `GameAbilityActionExecutor`  
Fonksiyon: `EndExecution`  
Görev: FireWeapon lifecycle'ını kapatır, OnEnd action'larını çalıştırır ve execution lifecycle'ını sonlandırır.

```cpp
		sas::AbilityExecutionLifecycle::End(
			execution,
			actions,
			[&](ActiveAbilityAction& action)
			{
				FireWeaponActionRuntime::End(action, context);
			},
			[&](ActiveAbilityAction& action)
			{
				ExecuteAction(action, context);
			}
		);
```

## Completion ve cleanup

`GameAbilityActionExecutor` bir ability'yi kendisi bitirmez. `GameAbility::EndExecution`, executor end çağrısını yapar; ardından `mExecution.actions` listesini temizler. Active state/cooldown geçişi SAS runtime katmanında yönetilir. Action result queue veya persistent completion kaydı doğrulanmadı.

## Sonraki inceleme

Action payload'larının domain-side behavior'ı, behavior family'leri, weapon/projectile lifecycle'i ve presentation tarafı kapsam dışıdır.
