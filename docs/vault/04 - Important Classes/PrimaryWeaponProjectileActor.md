---
type: important-class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsGame/include/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h
  - LightYearsGame/src/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.cpp
  - LightYearsGame/include/gameplay/ability/actors/AbilityWorldActor.h
symbols:
  - ly::PrimaryWeaponProjectileActor
  - ly::ProjectileImpactBehavior
related:
  - "[[Projectile System]]"
  - "[[Primary Weapon Projectile Lifecycle]]"
---

# PrimaryWeaponProjectileActor

## 7 Eylül 2026 kaynak kontrolü

Constructor SetAbilityPhysicsEnabled(false); Move sweep ile hit'i sahiplenir. Bu sınıfın varlığı bütün weapon delivery türlerinin aynı fiziği kullandığı anlamına gelmez. [[Projectile System]].

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Rol

`ly::PrimaryWeaponProjectileActor`, standard ve shotgun primary weapon handler'larının doğurduğu fiziksel projectile actor'dır. `AbilityWorldActor`dan owner, damage payload/tags, lifetime, collision radius ve target yardımcılarını devralır; bunun üzerine projectile hareketi, range/pierce ve optional impact policy ekler.

## Public yüzey

Dosya: `LightYearsGame/include/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h`

```cpp
class PrimaryWeaponProjectileActor : public AbilityWorldActor
{
public:
    PrimaryWeaponProjectileActor(World* world, Actor* owner,
        const WeaponPresentationDefinition& presentation,
        const GameplayAttributeList& values);
    void SetLaunchVelocity(const sf::Vector2f& launchVelocity);
    void SetImpactBehavior(const shared_ptr<ProjectileImpactBehavior>& impactBehavior);
    void Tick(float deltaTime) override;
    void OnActorBeginOverlap(Actor* otherActor) override;
    void Destroy() override;
};
```

## Constructor'dan gelen runtime alanları

| Alan | Kaynak attribute / tanım | Kullanım |
|---|---|---|
| Damage | `CommonAttributeIds::Damage` | Direct impact damage temel değeri |
| Lifetime | Projectile delivery lifetime | Base actor ömrü |
| Range | `CommonAttributeIds::Range` | Travel-distance sonlandırması |
| Collision radius | `CommonAttributeIds::CollisionRadius` | Ability collision setup |
| Area radius | `CommonAttributeIds::AreaRadius` | Area-impact dalı |
| Pierce count | Projectile delivery pierce count | Overlap sonrası devam hakkı |
| Visual scale | Presentation definition | Sprite scale |

Actor ayrıca owner collision ayarından collision configuration alır ve projectile render layer'a yerleşir.

## Hareket kontratı

`SetLaunchVelocity`, açık launch velocity'yi kaydeder ve actor velocity'sini yazar. `Move`, launch velocity verilmemişse forward × configured speed fallback'i üretir; sonra velocity'yi yazar ve konumu `launchVelocity * deltaTime` kadar offset eder. Travel distance, launch velocity'nin uzunluğu ile birikir.

Bu sınıf acceleration veya homing state taşımaz. Rocket gibi ability projectile family'lerinin hareketinin aynı olduğu varsayılmamalıdır.

## Impact ve pierce

`OnActorBeginOverlap` içinde optional impact behavior, direct projectile damage yolundan önce çalışır. Behavior impact'i ele alırsa projectile hemen sonlanır. Aksi durumda area projectile alan damage uygular ve sonlanır; area olmayan projectile ise pierce sayısını tüketebilir.

`ApplyImpactDamage`, yalnız `IsValidAbilityTarget` için direct combat damage çağrısına iner. Area yolunda target listesi `AbilityWorldActor::ApplyCombatDamageInRadius` üzerinden çözülür.

## Cleanup kontratı

`Destroy`, pending destroy durumunda no-op'tur. `mImpactBehaviorCompleted`, optional `OnProjectileFinished` callback'inin birden çok destroy tetiklemesinde sadece bir kez çalışmasını sağlar. Son adımda `Actor::Destroy` çağrılır; actor'ın World container'dan fiilen kaldırılması World lifecycle tarafından yapılır.

## Sahiplik

- World projectile actor'ı sahiplenir.
- Owner pointer base `AbilityWorldActor` içinde non-owning tutulur.
- `ProjectileImpactBehavior` actor'da `shared_ptr` ile tutulur; shotgun volley group bu yolla bütün pellet'ler arasında paylaşılır.
- Damage payload/tags, constructor sonrasında actor üzerinde değer olarak saklanır.

## İlgili notlar

- Sistem: [[Projectile System]]
- Spawn–flight–impact akışı: [[Primary Weapon Projectile Lifecycle]]
- Weapon caller: [[Weapon System]], [[Primary Weapon Fire Lifecycle]]

## Kaynak doğrulaması

- Son doğrulanan commit: `b2e24c11d157c64b89bd1cf47c1390bf72784056`.
- Header ve implementation doğrudan okundu; test/build bu aşamada çalıştırılmadı.
