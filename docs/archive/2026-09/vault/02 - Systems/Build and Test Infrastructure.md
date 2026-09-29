---
type: system
verified_on: 2026-09-20
verification: source-review-only
verified_worktree_state: dirty
---

# Build and Test Infrastructure

## Kaynakta tanımlı altyapı

Kök `CMakeLists.txt`: C++17, minimum CMake 3.31.6, CTest/BUILD_TESTING. Engine CMake SFML 3.1.0 ve Box2D 3.1.1; kök JSON 3.12.0 indirir. Bunlar depodaki sabitlemelerdir, en yeni sürüm iddiası değildir. LightYearsEngine, SpaceAbilitySystem, LightYearsGameplayCore statik hedefleri ve LightYearsGame executable mevcut.

| CTest kaydı | Test kaynağı |
|---|---|
| LightYearsEngineLifetime | LightYearsEngine/tests/EngineLifetimeTests.cpp |
| LightYearsGasLiteCore | LightYearsGame/tests/GasLiteCoreTests.cpp |
| LightYearsContent | LightYearsGame/tests/ContentLoaderTests.cpp |
| LightYearsAutoTargeting | LightYearsGame/tests/AutoTargetingTests.cpp |
| LightYearsGameplayTagSchema | LightYearsGame/tests/GameplayTagSchemaTests.cpp |
| LightYearsPrimaryWeapon | LightYearsGame/tests/PrimaryWeaponMagazineTests.cpp |
| LightYearsMovementInfluence | LightYearsGame/tests/MovementInfluenceTests.cpp |
| LightYearsMovementPolicy | LightYearsGame/tests/MovementPolicyTests.cpp |

Kaynak CMake dosyalarında 8 CTest kaydı vardır. MovementPolicy testinde cap kaldırma/normalleşme assertion'ları örneklendi; testlerin tamamı ayrıntılı denetlenmedi. Test dosyası varlığı, aile bazında lifecycle/presentation kabul testlerinin tamam olduğu anlamına gelmez.

## Bu denetimin sonucu

Build/test çalıştırılmadı; yalnız dokümantasyon düzenlendi. Var olan build klasörleri ve önceki başarı kayıtları güncel kaynakla eşleştiği doğrulanmadan kullanılmadı. Güncel temiz configure/build sonrası CTest ve arena/LevelOne smoke testi gereklidir. Test sonucuyla birlikte kullanılan build dizini, config, HEAD ve dirty kaynak durumu kaydedilmeli. CI ve sanitizer pipeline'larının uçtan uca çalışması incelenmedi.
