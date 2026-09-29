# Ability davranış denetimi — 2026-09-29

Kapsam: katalogdaki 55 yetenek (`docs/ABILITY_SCALING_CATALOG.md`), `abilities.json`,
C++ runtime (`src/gameplay/ability/<family>`, actor/effect behavior) ve fallback
config'ler. Salt okunur denetim; altı Sonnet alt ajanı (1–9, 10–18, 19–28, 29–37,
38–46, 47–55). Build ve test yapılmadı; bulgular kod okumasına dayanır.
Kesinlik: **kesin** = kod açıkça farklı; **olası** = yol izlendi, çalıştırılmadı.

Genel sonuç: 55 yeteneğin tamamında JSON temel değerleri, cooldown/süre/charge ve
seviye başı artışlar katalogla eşleşiyor; hiçbir seviye adımında Cooldown modifier
yok. Farklar runtime davranışında, hasar tipinde ve fallback config'lerde.

## A. Runtime davranış uyumsuzlukları (katalog ≠ oyun)

| # | Yetenek | Katalog | Kod | Kesinlik | Kaynak |
|---|---|---|---|---|---|
| 31 | ShieldHarvest | Focus sırasında hareket serbest (`MovementAllowed=true`) | `ApplyFocusActionLocks` hareket girdisini de kilitliyor | kesin | `ShieldHarvestAbility.cpp:267`, `FocusActionLocks.cpp:20` |
| 32 | DirectionalBarrier | ~120° (±60°) ön yay | `Dot > 0` → 180° yarım küre | kesin | `DirectionalBarrierEffectBehavior.cpp:22-35` |
| 32 | DirectionalBarrier | Aktifken ana silah ve diğer yetenekler kilitli | Kilit tag'i eklenmiyor | olası | `DirectionalBarrierAbility.cpp:~200`, `effects.json:145-154` |
| 32 | DirectionalBarrier | Önden gelen hasar tamamen engellenir | Yalnız Projectile teslimatı engelleniyor; beam/contact geçiyor | olası | `DirectionalBarrierEffectBehavior.cpp:49` |
| 37 | CrystalBarricade | Yalnız ilk güçlendirilmiş sekme çarpan alır | Kayıt duvar örneği başına; ikinci duvarda tekrar çarpan | olası | `CrystalBarricadeActor.cpp:275-285` |
| 39 | IroncladProtocol | ShotDamage/FireRate yalnız yetenek formülü | Silah şablonunun `scalingRules`'u (AP×0.25, AS×1.0) da ekleniyor; AS=100'de ~162/s | olası | `IroncladProtocolAbility.cpp:150-177`, `weapons.json:556-604`, `AbilityActionAttributeResolver.cpp:88-95` |
| 40 | TemporalConvergence | Tetiklenince alan kapanır | Alan ve yavaşlatma süre sonuna kadar kalıyor | kesin | `TemporalConvergenceFieldActor.cpp:~470-520` |
| 42 | ReclaimerProtocol | İyileştirme miktarı spawn anında sabitlenir | Oran sabit, miktar alım anındaki MaxHealth ile | olası | `ReclaimerProtocolAbility.cpp:~165-175`, `ReclaimerRepairKitActor.cpp:~150` |
| 48 | GravityAnomaly | Elite/boss çekimi control-response ile azalır | `ResolveControlResponse` çağrılmıyor | olası | `GravityAnomalyEffectBehavior.cpp:~44-54` |
| 48 | GravityAnomaly | Normal hedef tam `PullSpeed` alır | `(1 - dist/r)^2` düşüşü; kenarda sıfır | olası | `GravityAnomalyEffectBehavior.cpp:~39-46` |
| 48 | GravityAnomaly | Alan dışında kalıcı yavaşlatma yok | `InsideEffectDuration=2.0` s | olası | `abilities.json` (~826) |
| 49 | NullPulse | Düşman mermileri temizlenir | Takım filtresi yok; oyuncu mermileri de siliniyor | olası | `NullPulseTargetQuery.cpp:~38-56` |
| 50 | FrostMaelstrom | Tek alan yarıçapı 300→600 | Çekim alanı yarıçapın 1.25 katı | olası | `FrostMaelstromFieldActor.cpp:28` |

## B. Hasar tipi / katalog–içerik tutarsızlıkları (karar gerekli)

| # | Yetenek | Fark | Kaynak |
|---|---|---|---|
| 26 | Blastback | Katalog Photonic; kod/JSON Thermal (Ignite ile tutarlı) | `BlastbackConfig.h:70`, `BlastbackAbility.cpp:139-140` |
| 26 | Blastback | Stun hedef MaxHealth ile ölçekleniyor; katalogda sabit 1 s / 0.5 s | `BlastbackAbility.cpp:474-486` |
| 20, 21 | WingSentinels, CombatSentry | Katalog Photonic; damageTags yok (sayısal etki yok) | `WingSentinelProjectileActor.cpp:117`, `CombatSentryProjectileActor.cpp:212` |
| 37 | CrystalBarricade | Katalog Photonic; contact hasarı tipsiz | `CrystalBarricadeActor.cpp:243-252` |
| 13 | AegisReaver | JSON'da katalogda olmayan ShieldStealRatio +0.01/lv (base 0.1); katalog "ayrıca dengelenecek" diyor | `abilities.json` AegisReaver steps |
| 15 | IonStorm | Katalog Radius=335; kod 250 çekirdek + 250–335 düzensiz sınır (CURRENT_IMPLEMENTATION_CATALOG ile çelişki) | `IonStormFieldActor.cpp:~291-300` |

## C. Katalogda yazmayan ek davranışlar (zararsız, belgelenebilir)

InfernoSpray %35 öz-yavaşlatma; GlacialPressure itme/çarpışma stun'ları ve MaxHealth
itme ölçeği; FrozenThrong Luck kesir rastgele ek spawn; Cryostasis kalkan gecikmesi
temizliği ve manuel iptal; ClosedCircuit yeni bariyerin aktifleşince eskisini
değiştirmesi; VoidGate portal yerleşiminde menzil sınırı yok.

## D. Eski fallback config'ler (oyunu etkilemez)

JSON parser fallback sayılarını devralmaz (`AbilityDefinitionJsonParser.cpp:451`);
oyun JSON'dan yükler. Eski/eksik: ShieldGraft, EnergySpear, TemporalConvergence
(BaseShield 120, EP step eksik), PhaseDrift, GravityAnomaly, NullPulse, CryoBola;
stub (tasarım gereği): RailBurst, CrescentReaver, AegisReaver, Dash, SunBeam, Rocket,
InfernoSpray, OverdriveCore, MineLayer. Bazıları fallback kullanılırsa doğrulayıcıdan
geçmez.

## Uyumlu bulunanlar

1–12, 14, 16–25, 27–30, 33–36, 38, 41, 43–47, 51–55 (B/C notları hariç).
