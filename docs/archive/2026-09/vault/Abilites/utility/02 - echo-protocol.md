---
type: ability
name: Echo Protocol
category: utility
slot: 0
status: implemented
ability_id: Ability.Utility.EchoProtocol.Basic
behavior: EchoProtocol
features: [recorded-attack, echo-scaling, cooldown]
source: LightYearsGame/assets/content/data/abilities.json
---

# Echo Protocol

18 saniye cooldown'lı utility family’sidir. Son saldırı kaydı üzerinden echo
davranışı üretmek için 0.60 başlangıç güç oranı ve level başına +0.03 güç oranı
kullanır. AttackPower, MaxHealth, EnergyPower, AttackSpeed, Luck ve hareket için
ayrı scale katsayıları vardır. L2–L15 her seviyede cooldown'u 0.30 saniye azaltır.

Concrete replay/record akışının ayrıntılı behavior sınırı ilgili runtime sınıfı
ve test ile doğrulanmalıdır; bu not yalnız shipped content sözleşmesini kaydeder.
