---
type: system
status: implemented
last_verified_commit: b2e24c11d157c64b89bd1cf47c1390bf72784056
verified_worktree_state: dirty
source_files:
  - LightYearsEngine/include/framework/MathUtility.h
  - LightYearsEngine/src/framework/MathUtility.cpp
symbols:
  - ly::RandRange
  - ly::NormalizeVector
  - ly::ClampVectorLength
  - ly::LerpFloat
  - ly::RotationToVector
related:
  - "[[Physics System]]"
  - "[[CameraManager]]"
  - "[[Balance Atlas]]"
---

# Math Utility

`MathUtility` header/source çifti engine genelinde kullanılan küçük vektör, interpolation, açı ve rastgelelik yardımcılarını sağlar. Stateful bir manager değildir.

| Grup | Davranış |
|---|---|
| Açı / yön | `DegreesToRadians`, `RadiansToDegrees`, `RotationToVector` |
| Vektör | length, in-place normalize/scale, length clamp |
| Interpolation | float/color/vector `Lerp`; float alpha `0..1` aralığına sınırlandırılır |
| Rastgelelik | `RandRange<T>` her thread için `mt19937` ve `random_device` seed kullanır; integer/real dağıtımı tipe göre seçilir |

`NormalizeVector` sıfır uzunlukta vektörü sıfır bırakır. `ClampVectorLength` negatif max değeri `0` sayar ve çok kısa vektör için bölme yapmaz. Random generator için seed override, deterministic replay veya test injection API'si doğrulanmadı.

