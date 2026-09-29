---
type: system
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
uncommitted_areas:
  - weapon, ability and combat integrations
  - GasLiteCoreTests
source_files:
  - LightYearsGame/include/gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.h
  - LightYearsGame/include/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.h
  - LightYearsGame/src/gameplay/weapon/projectile/PrimaryWeaponProjectileSpawner.cpp
  - LightYearsGame/src/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.cpp
  - LightYearsGame/include/gameplay/weapon/impact/ProjectileImpactBehavior.h
  - LightYearsGame/include/gameplay/weapon/impact/ShotgunVolleyImpactGroup.h
  - LightYearsGame/include/gameplay/projectile/ProjectileSweep.h
symbols:
  - ly::PrimaryWeaponProjectileSpawner::FireSet
  - ly::PrimaryWeaponProjectileActor
  - ly::ProjectileImpactBehavior
  - ly::ShotgunVolleyImpactGroup
  - ly::projectile::FindSweptContacts
related:
  - "[[Weapon System]]"
  - "[[Primary Weapon Projectile Lifecycle]]"
  - "[[PrimaryWeaponProjectileActor]]"
  - "[[Combat and Damage System]]"
---

# Projectile System

## 7 Eylül 2026 kaynak kontrolü

PrimaryWeaponProjectileActor artık Box2D body açmaz (SetAbilityPhysicsEnabled(false)); hareket ve hit sahibi Move/ProjectileSweep yoludur. World sorgusu body-less grid + fizik broadphase birleşimidir. Bu nedenle aşağıdaki “fiziksel projectile” terimi Box2D body garantisi değildir. ProjectileGameplay temporal domain ve interception/portal kombinasyonlarının tamamı test edilmedi. Kaynak: LightYearsGame/src/gameplay/weapon/projectile/PrimaryWeaponProjectileActor.cpp.

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


## Kapsam ve ayrım

Light Years'ta tek bir evrensel projectile sınıfı bulunmadığı doğrulandı. Bu not, primary-weapon'ın fiziksel projectile yolunu açıklar: `PrimaryWeaponProjectileSpawner` ve `PrimaryWeaponProjectileActor`. Rocket, ability actor registry üzerinden doğan ayrı `RocketProjectileActor` family'sidir; electric arc, continuous beam ve expanding wave ise `PrimaryWeaponProjectileActor` üretmeyen farklı delivery türleridir.

Bu nedenle aşağıdaki not, tüm weapon family'lerinin aynı çarpışma veya damage davranışına sahip olduğunu iddia etmez.

## Ana bileşenler

| Bileşen | Sorumluluk | Sahiplik/ömür |
|---|---|---|
| `PrimaryWeaponHandler` | Standard/shotgun fire sırasında spawner'ı çağırır | Registry-owned built-in handler |
| `PrimaryWeaponProjectileSpawner::FireSet` | Muzzle ve pellet sayısına göre deferred World spawn istekleri kurar | Static namespace işlevi |
| `PrimaryWeaponProjectileActor` | Physical projectile hareketi, mesafe/lifetime, overlap ve cleanup | World actor; deferred destruction |
| `ProjectileImpactBehavior` | Optional projectile-grup veya custom impact politikası | Projectile'da `shared_ptr` |
| `ShotgunVolleyImpactGroup` | Target başına pellet sırasını izleyip progressive falloff'u impact anında uygular | Shotgun handler tarafından shared oluşturulur |
| `AbilityWorldActor` | Weak owner, source ability handle, damage payload/tags, lifetime, ability target ve radius yardımı | Projectile actor'ın base sınıfı |

## Spawn ve hareket

`FireSet`, her `WeaponMuzzleDefinition` için bir muzzle fire direction üretir; her pellet aynı muzzle için aynı carrier velocity'yi alır. Projectile sayısı en az birdir; spread açıları pellet'lere eşit adımlarla dağıtılır. Muzzle listesi boşsa varsayılan muzzle kullanılır.

Carrier velocity, `ProjectileMotion::ResolveCarrierVelocity` ile owner hızından hesaplanır:

| Bileşen | Uygulanan kural |
|---|---|
| İleri hız | Fire direction ile aynı yöndeyse `%75` miras alınır |
| Geri hız | Miras alınmaz (`0`) |
| Lateral hız | `%5` miras alınır |

Spawner actor'ı owner/world, presentation ve resolved weapon attributes ile spawn eder; damage tag'lerini, muzzle tabanlı transform'u, rotasyonu ve `projectile speed + carrier velocity` launch velocity'sini kurar.

## Actor runtime ve bitiş koşulları

`PrimaryWeaponProjectileActor`, construct aşamasında damage, damage attributes, lifetime, collision radius, collision setup, range, area radius, pierce count ve visual scale'i resolved attributes'tan alır. `Tick`, launch velocity yönünde konumu günceller ve travel distance'i biriktirir. Pozitif `Range` aşılırsa actor destroy edilir; base `AbilityWorldActor` lifetime'ı da ayrı ömür sınırıdır.

Ortak `ProjectileSweep`, her hareket karesi başlangıç-bitiş segmenti için
`World::GetActorsInBounds` broadphase adaylarını alır, `SweptGeometry` ile
expanded actor bounds kesişimini test eder ve temasları segment sırasına göre
döndürür. Standard primary projectile, Rocket ve Overdrive bu resolver'ı
kullanır; yüksek hızlı projectile küçük hedefi iki frame konumu arasında
atlayamaz. Standard projectile aynı target ID'sine yalnız bir kez impact
uygular; pierce farklı hedeflerde devam eder.

## Overlap, impact ve cleanup

Overlap'ta actor collision etkinse sıralama şöyledir:

1. Optional `ProjectileImpactBehavior`, geçerli ability target için `HandleImpact` çağrısı alır. `true` dönerse projectile doğrudan damage yoluna girmeden destroy edilir.
2. Aksi halde projectile direct target damage veya area damage yoluna gider.
3. Area radius pozitifse impact sonrası actor yok edilir.
4. Area yoksa remaining pierce pozitif olduğunda bir azalır ve projectile yaşamaya devam eder; aksi halde destroy edilir.

`Destroy`, pending-destroy ise tekrar işlem yapmaz. Optional impact behavior'a `OnProjectileFinished` yalnız bir kez iletilir; bullet performans sayacı düşürülür ve actor deferred destruction'a girer.

## Shotgun impact koordinasyonu

Shotgun handler, per-additional-hit damage reduction pozitif olduğunda `ShotgunVolleyImpactGroup` oluşturup bütün pellet'lere aynı `shared_ptr<ProjectileImpactBehavior>` nesnesini verir. Spawner her başarılı spawn girişiminde `OnProjectileSpawned`, projectile bitişinde ise `OnProjectileFinished` çağrısını sağlar; spawn başarısızlığında spawner completion'ı doğrudan bildirir.

Group, her target için kaç pellet'in daha önce vurduğunu tutar. İlk pellet tam
hasar verir; sonraki pellet'ler `damageReductionPerAdditionalHit` kadar
progressive azalır ve minimum multiplier altında kalmaz. Hasar çarpışma anında
uygulanır; son/uzak pellet bütün volley hasarını geciktirmez. Projectile finish
bildirimi yalnız yaşam döngüsü sayacını kapatır.

## Diğer delivery sınırları

| Delivery/family | Doğrulanan durum | Bu notla ilişkisi |
|---|---|---|
| Standard projectile | `PrimaryWeaponProjectileActor` üretir | Ana kapsam |
| Shotgun | Aynı actor üzerinden çoklu pellet üretir; optional volley policy kullanır | Ana kapsam |
| Rocket ability | `RocketProjectileActor : AbilityWorldActor`; ortak swept contact discovery kullanır | Ayrı ability actor family; damage/presentation detayları dışı |
| Electric arc | Projectile actor üretmeden arc visual/target zinciri kullanır | Detay dışı |
| Continuous beam | Projectile actor üretmeden beam delivery kullanır | Detay dışı |
| Expanding wave | Ayrı wave actor kullanır | Detay dışı |

## Relay Prism projectile dönüşümü

`RelayPrismActor`, normal projectile physics veya impact actor'ü değildir;
MouseWorld'de duran fizik dışı capture volume'dür. Yalnız
`AbilityWorldActor::CanBeCapturedByRelay()` kabul eden ability projectile'lerini
yakalar. Kaynağı destroy eder, snapshot'ından clone üretir ve `ProjectileRelayLineage`
ile aynı volume'un tekrar yakalamasını engeller. Clone'lar kaynağın normal
çarpışma yoluna geri dönebilir; ancak Prism capture döngüsü oluşturamaz.

## Sınırlar ve belirsizlikler

- `AbilityWorldActor` collision filtering ve `IsValidAbilityTarget`ın tüm collision-layer matrisi bu aşamada incelenmedi.
- Projectile'nin direct/area damage sonuçları, type modifier'ları ve shield etkileşimi sonraki combat aşamasının konusudur.
- Rocket, arc, beam ve wave için burada ortak projectile davranışı varsayılmamalıdır.
- Test kaynağında primary projectile hitch-catch-up, high-speed sweep,
  dual-muzzle ve anlık shotgun progressive-falloff senaryoları bulunur.

## Kaynak doğrulaması

- İncelenen durum: dirty worktree; belge mevcut çalışma ağacını açıklar.
- Directly read: primary projectile spawner/actor, impact behavior ve shotgun volley group.
