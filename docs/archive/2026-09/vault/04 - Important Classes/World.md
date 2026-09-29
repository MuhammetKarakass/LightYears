---
type: class
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
source_files:
  - LightYearsEngine/include/framework/World.h
  - LightYearsEngine/src/framework/World.cpp
symbols:
  - ly::World
  - ly::World::SpawnActor
  - ly::World::BeginPlayInternal
  - ly::World::TickInternal
  - ly::World::CleanCycle
  - ly::World::Render
related:
  - "[[Actor and World System]]"
  - "[[Object Lifecycle]]"
  - "[[Actor]]"
  - "[[Application]]"
---

# World

## 7 Eylül 2026 kaynak kontrolü

World actor time domain, body-less spatial grid ve render-bounds culling sahibi oldu. SpawnActor her çağrıda yeni allocation yapar. TickInternal HUD/camera'yı actor domain dışında bırakır. [[Temporal Runtime]] ve [[Actor and World System]].

Bu ek kaynak incelemesidir; build/test çalıştırılmadı. Aşağıdaki eski örnekler tarihsel inceleme kapsamını taşır; bu güncelleme ile çelişen ayrıntılar güncel davranış kabul edilmemeli.


`ly::World`, actor’ların gerçek runtime container’ı ve güçlü sahibidir. Ayrıca stage, HUD, camera, pause, event dispatch, render traversal ve actor cleanup sorumluluklarını taşır.

## Sahiplik ve durum

Application’a raw pointer tutar; actor, stage ve HUD’ları `shared_ptr` ile sahiplenir. View target weak pointer’dır. Spawn sonucu caller’a weak pointer döner.

## Kritik gerçek kod

Dosya: `LightYearsEngine/include/framework/World.h`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: sınıf alanları  
Görevi: Runtime koleksiyonlarını tanımlamak.

```cpp
Application* mOwningApp;
bool mBeganPlay;
bool mIsPaused;

List<shared_ptr<Actor>> mActors;
List<shared_ptr<Actor>> mPendingActors;
List<shared_ptr<GameStage>> mGameStages;
List<shared_ptr<GameStage>>::iterator mCurrentStage;
shared_ptr<HUD> mHUD;
shared_ptr<HUD> mOverlayHUD;
```

Dosya: `LightYearsEngine/src/framework/World.cpp`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: `BeginPlayInternal`  
Görevi: World initialization hook ve stage başlangıcını bir kez çalıştırmak.

```cpp
if (!mBeganPlay)
{
	mBeganPlay = true;
	BeginPlay();  
	InitGameStages();
	BeginStages();
}
```

Dosya: `LightYearsEngine/src/framework/World.cpp`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: `TickInternal`  
Görevi: Pending actor promotion ve BeginPlay.

```cpp
List<shared_ptr<Actor>> actorsToSpawn;
actorsToSpawn.swap(mPendingActors);

for (const shared_ptr<Actor>& actor : actorsToSpawn)
{
	mActors.push_back(actor);
	actor->BeginPlayInternal();
	OnActorSpawned(actor.get());
}
```

Dosya: `LightYearsEngine/src/framework/World.cpp`  
Sınıf veya namespace: `ly::World`  
Fonksiyon: `Render`  
Görevi: Actor render’ını layer ve pending-destroy state ile filtrelemek.

```cpp
for (const std::shared_ptr<Actor>& actor : mActors)
{
	if (!actor->GetIsPendingDestroy() && actor->GetRenderLayer() == layer)
	{
		actor->Render(window);
	}
}
```

## Test ve doğrulama

### Mevcut testler

`GasLiteCoreTests.cpp`, `World` nesnelerini doğrudan kurar; `SpawnActor`, `TickInternal`, `GetActorsByType` ve weak expiration kullanır.

### Doğrudan test edilmeyen davranışlar

Stage sırası, pause branch’i, HUD initialization/event önceliği, render view restorasyonu ve destructor davranışı.

### Manuel doğrulama gereken noktalar

Camera/HUD çizimi ve gerçek level switch.

### Önerilen fakat henüz bulunmayan testler

- Minimal World ile actor/stage/world tick order.
- Pause ve `GetTickWhenPaused`.
- Render layer traversal ve view restore.

## Kod Okuma Sırası

1. `LightYearsEngine/include/framework/World.h`
2. `LightYearsEngine/src/framework/World.cpp`
3. `LightYearsEngine/include/gameplay/GameStage.h`
4. `LightYearsEngine/src/gameplay/GameStage.cpp`
