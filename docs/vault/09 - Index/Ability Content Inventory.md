---
type: index
verified_on: 2026-09-20
verification: registration-inventory
---

# Ability Content Inventory

`LightYearsGame/assets/content/data/abilities.json` içinde 55 ability kaydı ve
`LightYearsGame/src/gameConfigs/ability/AbilityCatalog.cpp` içinde bunlara karşılık
gelen builtin definition envanteri vardır. Bu bir kayıt envanteridir; aktif slotta
olma veya her davranışın oyun testinden geçme kanıtı değildir. JSON
materialization/validation `GameContentBootstrap::Register` yolundadır. JSON
JSON content toplamları 55 ability, 19 effect, 10 weapon, 4 ship (1 player +
3 enemy) ve 1 attachment'tır; C++ fallback attachment tanımları ayrıca
`AttachmentConfig.h` içinde 7 kayıttır. Güncel ID ve numeric content sahibi
[Current Implementation Catalog](../../CURRENT_IMPLEMENTATION_CATALOG.md)'dur.
Runtime slot binding'i `LightYearsGame/src/gameplay/ability/loadout/DefaultAbilityLoadout.cpp`
dosyasının sahibidir; [[00 - Runtime Snapshot]] tarihsel bir snapshot'tır.
Sayısal tablo burada kopyalanmaz.

| C++ definition | Denetim kapsamı |
|---|---|
| `Shield_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `DirectionalBarrier_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `SunBeam_Strike_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `Dash_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `GravityAnomaly_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `Rocket_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `InfernoSpray_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `OverdriveCore_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `NullPulse_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `PhaseDrift_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `ShieldHarvest_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `Cryostasis_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `HullShock_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `GlacialPressure_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `OrbitalDrones_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `ExecutionDrive_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `RelayPrism_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `EchoProtocol_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `RailBurst_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `CrescentReaver_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `AegisReaver_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `MineLayer_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `EnergySpear_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `ScorchDrive_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `InertialWake_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `IonStorm_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `ChainLightning_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `StormMark_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `VoidGate_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `FrostMaelstrom_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `CryoBola_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `FrozenThrong_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `CombatSentry_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `NanoPlague_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `AstralSurge_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `WingSentinels_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `ReturnProtocol_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `CrystalBarricade_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `SolarBombardment_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `StrikeRun_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `Blastback_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `SeismicCharge_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `ArcScythes_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `IroncladProtocol_Basic` | Katalog + aktivasyon/end kaynak okuması; oyun testi yok |
| `ClosedCircuit_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `VectorSync_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `ZeroDrag_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `TemporalConvergence_Basic` | Katalog kaydı; tüm runtime fazları incelenmedi |
| `TemporalRecall_Basic` | Katalog + seçilmiş behavior fonksiyonları; tam lifecycle incelenmedi |
| `TimeSlip_Basic` | Katalog + aktivasyon/end kaynak okuması; oyun testi yok |
| `FoldspaceArena_Basic` | Katalog + seçilmiş behavior fonksiyonları; tam lifecycle incelenmedi |

## Tasarım ile ayrım

`Abilites/` notlarındaki hedef kategori sayıları, evolve seviyeleri ve fikirler bu katalogdan bağımsızdır. `presentation/ability/<family>/` typed profil kontratı uygulanacak mimari sınırdır; ayrı evolve seçim sistemi kanıtı değildir.
