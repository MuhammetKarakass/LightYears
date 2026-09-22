---
type: walkthrough
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - ability, damage, movement and presentation integrations
source_files:
  - LightYearsGame/src/gameplay/ability/GameAbility.cpp
  - LightYearsGame/src/gameplay/ability/actions/AbilityActionAttributeResolver.cpp
  - SpaceAbilitySystem/include/abilities/GameplayAbilityInstance.h
  - LightYearsGame/src/gameplay/weapon/PrimaryWeaponExecutionSystem.cpp
  - LightYearsGame/src/gameplay/progression/ShipProgression.cpp
symbols:
  - ly::AbilityActionAttributeResolver::ResolveAttributes
  - ly::GameAbility::RebuildDefinitionForLevel
  - ly::PrimaryWeaponExecutionSystem::EnsureRuntimeConfigured
  - ly::ShipProgression::RebuildLevelModifiers
related:
  - "[[Balance Atlas]]"
  - "[[Ability Skill Progression System]]"
  - "[[Primary Weapon Fire Lifecycle]]"
  - "[[Derived Attributes]]"
---

# Balance Data and Runtime Resolution Flow

Bu walkthrough, static config'in tek bir global balance runtime'ına yüklenmediğini gösterir. Ship, ability ve weapon katmanları kendi config'lerini kendi runtime'larında çözer; ortak attribute/rating kuralları onların girdilerini birleştirir.

```mermaid
sequenceDiagram
    participant Config as Ship / Ability / Weapon config
    participant Ship as ShipProgression
    participant Attr as AttributeSystem
    participant Ability as GameAbility
    participant Exec as AbilityActionAttributeResolver
    participant Weapon as PrimaryWeaponExecutionSystem

    Config->>Ship: progressionDefinition + level growth
    Ship->>Attr: AddModifier(total level bonuses)
    Config->>Ability: base definition + level steps
    Ability->>Ability: rebuild resolved definition for level
    Ability->>Weapon: refresh primary weapon runtime
    Config->>Weapon: base weapon + unlocked feature tags
    Ability->>Exec: action attribute requested
    Exec->>Attr: get resolved owner attribute
    Exec->>Exec: apply ordered scaling rules; clamp >= 0
```

## 1. Ship level girdisi

`ShipProgression::AddXP` level değiştiğinde mevcut modifier handle'larını kaldırır ve `RebuildLevelModifiers` ile toplam bonusu yeni `AttributeSystem`e yeniden ekler. Respawn yeni attribute runtime'ı oluşturduğunda `BindAttributes` aynı deterministik toplamı tekrar kurar. Bu nedenle level growth, her XP kazanımında art arda kalıcı modifier eklemek yerine mevcut level'dan hesaplanır.

## 2. Ability level girdisi

`GameAbility::RebuildDefinitionForLevel`, `mBaseDefinition`dan başlar; level step'lerinin modifier, upgrade ID, action ve trigger eklerini `mDefinition`a uygular. Primary-fire ability ise runtime configuration da yenilenir. Satın alma maliyeti ve restore state'i `Player`dadır: [[Ability Level Purchase and Respawn Restore Flow]].

## 3. Action değeri çözümlenmesi

`AbilityActionAttributeResolver::ResolveAttributes`, action attribute'unun base value'sunu önce ability scaling rule'larıyla, weapon definition verildiyse sonra weapon scaling rule'larıyla çözer. Her matching rule resolved owner attribute'unu okur:

- Add: `value += ownerAttribute × coefficient`
- Multiply: `value *= 1 + ownerAttribute × coefficient`
- Override: `value = ownerAttribute × coefficient`

Sonuç negatif olamaz. Bu fonksiyon yalnız action attribute çözümünün kuralıdır; `CombatRuntime`ın crit/armor/shield sıra ve etkilerini çözmez.

## 4. Weapon feature eşlemesi

`PrimaryWeaponExecutionSystem::ResolveRuntimeConfiguration`, önce weapon definition'ı validate eder, sonra base feature tag'lerini ekler. Ability level'dan gelen unlocked upgrade listesi varsa profile level step'lerindeki feature tag'leri exact tag eşleşmesiyle runtime'a ekler. Böylece “upgrade ID tanımlı” olması tek başına feature'ün aktif olduğu anlamına gelmez; uygun weapon profile ve registered handler da gerekir.

## Sınır

Bu akış numeric config değerlerin tasarımsal olarak dengeli olduğunu veya her combat sonucunun burada oluştuğunu kanıtlamaz. Damage reduction, crit, effect, shield ve health sırası için [[Combat and Damage System]]; merkezi eğriler için [[Derived Attributes]]; kaynak haritası için [[Balance Atlas]] kullanılır.

