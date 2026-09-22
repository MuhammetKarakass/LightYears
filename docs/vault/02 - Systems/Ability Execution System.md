---
type: system
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
uncommitted_areas:
  - GameAbilityActionExecutor action dispatch and integration code
source_files:
  - SpaceAbilitySystem/include/abilities/AbilityActionScheduler.h
  - SpaceAbilitySystem/src/abilities/AbilityActionScheduler.cpp
  - LightYearsGame/include/gameplay/ability/content/GameAbilityDefinition.h
  - SpaceAbilitySystem/include/abilities/AbilityExecution.h
  - LightYearsGame/include/gameplay/ability/GameAbilityActionExecutor.h
  - LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp
  - SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h
symbols:
  - ly::AbilityActionSpec
  - ly::GameAbilityExecution
  - ly::ActiveAbilityAction
  - ly::GameAbilityActionExecutor::BeginExecution
  - ly::GameAbilityActionExecutor::TickExecution
  - ly::GameAbilityActionExecutor::EndExecution
  - ly::GameAbilityActionExecutor::ExecuteAction
related:
  - "[[Ability System]]"
  - "[[Ability Action Execution Flow]]"
  - "[[AbilityExecutor]]"
  - "[[Weapon System]]"
  - "[[Primary Weapon Fire Lifecycle]]"
  - "[[PrimaryWeaponExecutionSystem]]"
  - "[[Gameplay Effect System]]"
---

# Ability Execution System

## Sistem sorumlulukları

| Katman | Doğrulanmış sorumluluk |
|---|---|
| `LightYearsAbilitySystemComponent` / `sas::AbilitySystemComponent` | Typed ability runtime'ını sahiplenir ve tick'i game ability instance'larına yönlendirir. |
| `GameAbility` | `mExecution` runtime kaydını sahiplenir; successful activation'da begin, aktifken tick, end/cancel'da cleanup çağırır. |
| `sas::AbilityActionScheduler` | Silah dışı repeated action interval/count ilerlemesini oyundan bağımsız yürütür. |
| `GameAbilityActionExecutor` | Phase action'larını seçer; on-activate/while-active/on-end payload dispatch ve game-owned cleanup yürütür. Static utility sınıftır; kendi active-execution container'ı yoktur. |
| `GameAbilityBehavior` / `sas::AbilityBehavior` | Ability-specific activate/tick/end hook'udur; executor action türlerinin sahibi değildir. Ayrıntı: [[Ability Behavior System]]. |
| `AbilityActionSpec` | Config action payload'ı, phase, interval ve maximum execution sayısını taşır. |

## Ana veri tipleri

| Veri tipi/sınıf | Dosya | Sorumluluk | Sahibi | Yaşam süresi |
|---|---|---|---|---|
| `AbilityActionSpec` | `GameAbilityDefinition.h` | Phase + variant payload + interval/max count | `GameAbilityDefinition` | Definition kopyasıyla |
| `GameAbilityExecution` | `SpaceAbilitySystem/include/abilities/AbilityExecution.h` + `GameAbilityActionExecutor.h` | Active action listesi | `GameAbility::mExecution` | Ability instance ömrü / end'e kadar |
| `sas::RepeatedAbilityActionState` | `AbilityActionScheduler.h` | Generic interval remaining + execution count | Action runtime variant | Execution süresi |
| `ActiveAbilityAction` | `GameAbilityActionExecutor.h` | Spec pointer + game/SAS runtime variant | `GameAbilityExecution` | Execution süresi |
| `AbilityExecutionContext` | `GameAbilityActionExecutor.h` | System, definition, event ve instance bağlamı | Begin/tick/end caller | Çağrı süresi |
| `GameAbilityActionExecutor` | `GameAbilityActionExecutor.h/.cpp` | Stateless phase/action dispatcher | Static methods | Program ömrü |

## Action tanımı ve dispatch yöntemi

Action payload'ı `AbilityActionData` variant'ı üzerinden taşınır; executor `std::visit` ve `if constexpr` ile tür bazlı dispatch yapar. İncelenen `ExecuteAction` branch'lerinde aşağıdaki action'lar doğrulandı.

| Action türü | Tanım tipi | Çalıştıran sembol | Bağlandığı sistem | Durum |
|---|---|---|---|---|
| Effect uygulama | `ApplyEffectAction` | `GameAbilityActionExecutor::ExecuteAction` | `GameplayEffectRuntimeSystem::ApplyEffect` | Active |
| Primary weapon fire | `FireWeaponAction` | `ExecuteAction`, `TickAction`, `EndExecution` | `PrimaryWeaponExecutionSystem` | Active; [[Weapon System]], [[Primary Weapon Fire Lifecycle]] |
| İmpulse movement | `ApplyImpulseAction` | `ExecuteAction` | `MovementInfluenceService::ApplyImpulse` | Active |
| Gameplay event yayını | `EmitGameplayEventAction` | `ExecuteAction` | `sas::AbilitySystemComponent::HandleGameplayEvent` | Active |
| Ability actor spawn | `SpawnActorAction` | `ExecuteAction` | `AbilityActorRegistry::Spawn` / World-bağlı actor | Active |
| Ayrı delay/wait | — | — | — | Not supported; WhileActive için interval accumulator vardır |
| Ayrı visual/presentation action | — | — | — | Not supported; spawn sonrası presentation ayrımı bu görevde incelenmedi |

Dosya: `LightYearsGame/include/gameplay/ability/content/GameAbilityDefinition.h`  
Sınıf: `AbilityActionSpec`  
Fonksiyon: veri tanımı  
Görev: Definition içindeki action konfigürasyonunu taşır.

```cpp
	struct AbilityActionSpec
	{
		AbilityActionPhase phase = AbilityActionPhase::OnActivate;
		AbilityActionData action;
		float interval = 0.f;
		int maxExecutions = 1;
	};
```

## Execution lifecycle

1. `GameAbility::BeginExecution`, kendi `mExecution` kaydını temizler ve `BeginExecution` çağırır.
2. Begin, definition actions içinden `OnActivate` ve `WhileActive` phase'lerini active listeye ekler; OnActivate action'larını aynı çağrıda çalıştırır.
3. Active `GameAbility` her frame `GameAbility::TickExecution` üzerinden `TickExecution` çağırır.
4. Tick, active listedeki her WhileActive action'ı bağımsız olarak işler. Actions shared execution listesinde birlikte yaşar; biri tamamlanmadan diğerinin başlamasını engelleyen generic sıra/result state'i yoktur.
5. Normal repeated action için interval/count state ve due/record işlemleri SAS scheduler'dadır. `FireWeaponAction`, game-owned weapon lifecycle gerektirdiği için ayrı typed runtime state kullanır.
6. `GameAbility::EndExecution`, `EndExecution` çağırır: active weapon lifecycle kapatılır, OnEnd action'ları çalıştırılır ve action listesi `GameAbility` tarafından temizlenir.

Dosya: `LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp`  
Sınıf: `GameAbilityActionExecutor`  
Fonksiyon: `BeginExecution`  
Görev: Phase listelerini kurar ve OnActivate action'larını senkron dispatch eder.

```cpp
	void GameAbilityActionExecutor::BeginExecution(GameAbilityExecution& execution, AbilityExecutionContext& context)
	{
	sas::AbilityExecutionLifecycle::Begin(
		execution,
		context.definition->actions,
		[&](ActiveAbilityAction& action)
		{
			ExecuteAction(action, context);
		}
	);
	}
```

Dosya: `LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp`  
Sınıf: `GameAbilityActionExecutor`  
Fonksiyon: `TickAction`  
Görev: Normal WhileActive action için independent interval ve max-execution runtime state'ini ilerletir.

```cpp
		if (sas::AbilityActionScheduler::IsExecutionDue(
			*repeated,
			deltaTime,
			action.spec->maxExecutions
		))
		{
			ExecuteAction(action, context);
			const float nextInterval = AbilityActionAttributeResolver::ResolveEffectiveInterval(
				context,
				action.spec->interval
			);
			sas::AbilityActionScheduler::RecordExecution(*repeated, nextInterval);
		}
```

## Action geçiş noktaları

Dosya: `LightYearsGame/src/gameplay/ability/GameAbilityActionExecutor.cpp`  
Sınıf: `GameAbilityActionExecutor`  
Fonksiyon: `ExecuteAction`  
Görev: Variant dispatch ile effect action'ını target effect system'e yönlendirir. Kesit, gerçek branch'in ilgili satırlarıdır.

```cpp
			if (sas::AbilitySystemComponent* targetAbilitySystem = ResolveEffectTarget(
				*context.abilitySystem,
				actionData.targetPolicy,
				context
			))
			{
				targetAbilitySystem->ApplyGameplayEffect(effectSpec, &owner);
			}
```

`ApplyImpulseAction`, owner velocity'sine resolved direction × magnitude ekler. `SpawnActorAction`, world kontrolünden sonra `AbilityActorRegistry::Spawn` çağırır. Bu çağrıların target domain implementasyonları burada incelenmedi.

## Completion, cancel ve sınırlar

- Action dispatch fonksiyonları `void` döndürür; generic Success/Failure/Running result tipi doğrulanmadı.
- Lookup/target/lifecycle başarısızlıklarında action branch'i yerel `return` ile çıkar; generic execution failure state'i yoktur.
- `Cancel`, aktif game ability'de `GameAbility::EndExecution` yolunu çağırır; cleanup executor `EndExecution` üzerinden tamamlanır.
- Ability kaldırıldığında `RemoveAbility`, instance'ı cancel edip container'dan siler; active execution için normal end cleanup'ı bu yoldan gelir.
- Owner'ın normal `Clear` dışındaki doğrudan yok edilme senaryosu bu görevde yeniden denetlenmedi.
- Aynı anda birden çok granted ability active olabilir; her `GameAbility` kendi `mExecution` değerini tutar. Global executor queue yoktur.
- Executor cooldown/charge state'ini sahiplenmez; yalnız action interval veya weapon interval gibi execution-local state taşır.

## Kod okuma sırası

1. `GameAbilityDefinition.h::AbilityActionSpec` ve `sas::AbilityActionPhase`
2. `SpaceAbilitySystem/include/abilities/AbilityExecution.h::AbilityExecution` / `ActiveAbilityAction`
3. `GameAbilityActionExecutor.cpp::BeginExecution`
4. `GameAbilityActionExecutor.cpp::TickAction` ve `ExecuteAction`
5. `ExecuteAction` içindeki `ApplyEffectAction` veya `SpawnActorAction` branch'i
6. `GameAbilityActionExecutor.cpp::EndExecution` ve `GameAbility.cpp::EndExecution`

## Sonraki inceleme

- AbilityBehavior family implementasyonları.
- Projectile/actor family davranışları.
- Spawned actor sonrası presentation akışı.
