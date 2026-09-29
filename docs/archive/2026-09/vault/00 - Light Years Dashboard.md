---
type: dashboard
verified_on: 2026-09-20
verification: source-review-only
verified_worktree_state: dirty
---

# Light Years Dashboard

> Güncel değerlendirme: bu çalışma ağacı karşılaştırması · Tarihsel kaynak snapshot: [[00 - Runtime Snapshot]]

Light Years savaş mekanikleri bakımından kapsamlı bir prototip; güncel başlangıç ateş eden sabit hedeflerle ArenaTestLevel. Ana döngünün kod bağlantıları mevcut, ancak restart, alternatif bölüm HUD'su ve scrap ekonomisinde entegrasyon açıkları var. Bu denetimde build/test veya oyun çalıştırılmadı.

## Güncel durum

- Güncel runtime slot binding'i `LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp` sahibidir; bu dashboard complete loadout tablosunu kopyalamaz.
- HEAD `f83fe57b44777de4c31799b4e8bafac04a18700d`; mevcut kullanıcı değişiklikleri dahil dirty çalışma ağacı incelendi.
- C++17; CMake minimum 3.31.6. Depo bağımlılıkları SFML 3.1.0, Box2D 3.1.1, nlohmann/json 3.12.0.
- CMake'de sekiz test kaydı var; geçtikleri burada varsayılmıyor. [[Build and Test Infrastructure]].
- İlk iş: arena restart ve Player yaşam süresi doğrulaması. [[Technical Debt and Roadmap]].

## Ana navigasyon

- [[Project Overview]] · [[Directory Map]] · [[System Index]] · [[Source File Index]] · [[Vault Note Inventory]]
- [[Current Implementation]] · [[2026-09-07 Project Status Review]] · [[Technical Debt and Roadmap]]
- [[Game Loop]] · [[Actor and World System]] · [[Ownership and Lifetime]] · [[Frame Update Flow]]
- [[Engine Runtime Services]] · [[Physics System]] · [[Engine Performance and Memory]] · [[Audio System]]
- [[Movement]] · [[Shield and Energy]] · [[Temporal Runtime]] · [[Game Flow and UI]]
- [[Gameplay Tag System]] · [[Attribute System]] · [[Gameplay Effect System]]
- [[Ability System]] · [[Ability Execution System]] · [[Ability Behavior System]] · [[Ability Content Inventory]]
- [[Weapon System]] · [[Projectile System]] · [[Combat and Damage System]] · [[Shield System]]
- [[Ship Progression System]] · [[Ability Skill Progression System]] · [[Save and Load]]
- [[Balance Atlas]] · [[Existing Documentation Index]] · [[yetenekler-dashboard]]

## Öğrenme yolları

1. Runtime: [[Application Startup Flow]] → [[Frame Update Flow]] → [[Actor Spawn and Destruction Flow]] → [[Timer, Delegate and Camera Frame Flow]].
2. Savaş: [[Ability Grant and Activation Flow]] → [[Ability Action Execution Flow]] → [[Primary Weapon Fire Lifecycle]] → [[Primary Weapon Projectile Lifecycle]] → [[Damage and Shield Resolution Flow]].
3. İlerleme: [[Ship Progression and Respawn Flow]] → [[Ability Level Purchase and Respawn Restore Flow]]. Satın alma API'si mevcut, oyuncu etkileşimi henüz bağlı değil.

Eski notlardaki commit/test sonuçları kendi tarihleriyle sınırlıdır. Tasarım notları ve evolve tabloları runtime uygulama envanteri değildir.

## Belge takibi

Vault Markdown notları `.gitignore` içindeki `*.md` nedeniyle normal Git diff'inde görünmez. Bu güncelleme yerel dosyalardadır; ignore kuralı değiştirilmedi. Son vault commit tarihi not güncellemesi diye yorumlanmamalı.
