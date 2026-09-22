---
type: important-class
status: implemented
last_verified_commit: f83fe57b44777de4c31799b4e8bafac04a18700d
last_verified_date: 2026-09-20
verified_worktree_state: dirty
source_files:
  - SpaceAbilitySystem/include/abilities/AbilityBehavior.h
  - LightYearsGame/include/gameplay/ability/GameAbility.h
  - LightYearsGame/src/gameplay/ability/GameAbility.cpp
symbols:
  - sas::AbilityBehavior<GameAbilityDefinition, GameAbilityBehaviorContext>
  - ly::GameAbilityBehavior
  - ly::GameAbilityBehaviorContext
related:
  - "[[Ability Behavior System]]"
  - "[[Ability Behavior Registration and Dispatch Flow]]"
  - "[[AbilityInstance]]"
  - "[[AbilityExecutor]]"
---

# AbilityBehavior

## Rol

`ly::GameAbilityBehavior`, SAS'ın `sas::AbilityBehavior<GameAbilityDefinition, GameAbilityBehaviorContext>` tabanını game definition/context ile özelleştiren ability-family hook sınıfıdır. Ability'yi grant eden container değildir, cooldown/charge state'ini sahiplenmez ve action payload'larını kendisi dispatch etmek zorunda değildir.

## Context kontratı

Dosya: `LightYearsGame/include/gameplay/ability/GameAbility.h`  
Sembol: `ly::GameAbilityBehaviorContext`

```cpp
struct AbilityBehaviorContext
{
	LightYearsAbilitySystemComponent& abilitySystem;
	GameAbility& instance;
    Actor& owner;
	const GameAbilityDefinition& definition;
};
```

| Alan | Anlam |
|---|---|
| `abilitySystem` | Owner'a bağlı `LightYearsAbilitySystemComponent` servislerine erişim |
| `instance` | Çağrıyı yapan `GameAbility` runtime kaydı |
| `owner` | Ability sahibi `Actor` |
| `definition` | O çağrıda geçerli, salt-okunur `GameAbilityDefinition` |

Context referansları behavior tarafından sahiplenilmez. Geçerlilikleri çağrı süresiyle sınırlı kabul edilmelidir.

## Sanal hook'lar

| Hook | Çağrıldığı sınır | Base davranış |
|---|---|---|
| `Validate` | Definition/behavior uyumluluğu kontrol edilirken | `true` |
| `Activate` | `GameAbility` içeriği aktif yapılmadan ve action execution başlatılmadan önce | `true` |
| `OnInputPressed` | Aktif ability yeni press aldığında | `false` |
| `Tick` | Aktif execution tick'i sırasında | no-op |
| `End` | Aktif ability bir `AbilityEndReason` ile sonlandırılırken | no-op |

Base implementasyon:

```cpp
bool GameAbilityBehavior::Validate(const GameAbilityDefinition&, std::string*) const
{
    return true;
}

bool GameAbilityBehavior::Activate(GameAbilityBehaviorContext&)
{
    return true;
}
```

`Tick` ve `End` gövdeleri boştur. Bu varsayılanlar `Configured` behavior'ın yalnız action/executor ile çalışan bir ability definition için kullanılabilmesini sağlar.

## Sahiplik ve ömür

`GameAbility`, constructor'da behavior `unique_ptr`'ını devralır ve `mBehavior` üyesinde tutar. Registry bir singleton behavior döndürmez; `GameAbilityBehaviorRegistry` factory her create çağrısında yeni nesne üretir. Sanal destructor, derived family behavior'larının ability ile birlikte güvenli biçimde yok edilmesini sağlar.

```cpp
GameAbilityExecution mExecution;
unique_ptr<GameAbilityBehavior> mBehavior;
```

Behavior'ın ayrı bir owner container'ı veya açık unregister/destruction protokolü yoktur.

## Sorumluluk sınırı

- `GameAbilityBehavior`: family'ye özgü validation ve lifecycle tepkileri.
- `GameAbility` / `sas::GameplayAbilityInstance`: input, active state, cooldown, charge, süre ve generic lifecycle sahipliği.
- `GameAbilityActionExecutor`: definition action'larının phase/variant dispatch'i.
- `GameAbilityBehaviorRegistry`: behavior kimliğinden yeni behavior factory seçimi.

Bir derived behavior action listesiyle birlikte çalışabilir; ancak base kontrat, family'nin bütün işini behavior içinde veya bütün işini executor içinde yapmasını zorunlu kılmaz. Gerçek iş bölümü family bazında sonraki incelemede doğrulanmalıdır.

## Doğrulanmış derived sınıflar

`DashAbility`, `ShieldAbility`, `GravityAnomalyAbility`, `RocketAbility`,
`SunBeamAbility`, `InfernoSprayAbility`, `OverdriveCoreAbility`,
`NullPulseAbility`, `PhaseDriftAbility`, `ShieldHarvestAbility`,
`HullShockAbility`, `OrbitalDronesAbility`, `ExecutionDriveAbility` ve
`RelayPrismAbility`, `EchoProtocolAbility`, `MineLayerAbility`,
`RailBurstAbility`, `CrescentReaverAbility`, `EnergySpearAbility` ve
`ScorchDriveAbility`, game registration içindeki shipped factory hedefleridir.
Güncel runtime özeti: [[00 - Runtime Snapshot]].

## Dikkat edilmesi gerekenler

- `Activate` reddi aktivasyon state'ini ilerletmez; bu hook'un side effect üretmesi durumunda rollback kontratı base sınıfta tanımlı değildir.
- Base `Validate` bütün definition'ları kabul eder; family'ye özgü invariants derived override sorumluluğundadır.
- Context içindeki referansları uzun ömürlü pointer/reference olarak saklamak güvenli kabul edilmemelidir.
- Gravity-anomaly family mevcut dirty worktree'de görülmektedir.

## İlgili akışlar

- Genel sistem: [[Ability Behavior System]]
- Registration ve dispatch: [[Ability Behavior Registration and Dispatch Flow]]
- Instance sahipliği: [[AbilityInstance]] (not dosya adı; güncel sınıf `GameAbility` / `sas::GameplayAbilityInstance`)
- Action yürütme: [[Ability Execution System]], [[AbilityExecutor]] (not dosya adı; güncel sınıf `GameAbilityActionExecutor`)

## Kaynak doğrulaması

- Son doğrulanan commit: `f83fe57b44777de4c31799b4e8bafac04a18700d`.
- Kaynak sözleşme ve game adapter doğrudan okundu; lifecycle çağrıları `GameAbility` ve `sas::GameplayAbilityInstance` üzerinden doğrulandı.
- Test veya build çalıştırılmadı.
