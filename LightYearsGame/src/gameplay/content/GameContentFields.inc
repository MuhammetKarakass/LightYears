// Authored field vocabulary. Wildcards describe array rows; unknown keys remain opaque.
		if (schema.type == "ability") Add(schema, "/id", R"field("Ability.Movement.Dash.Basic")field");
		if (schema.type == "ability") Add(schema, "/cooldown", R"field(9)field");
		if (schema.type == "ability") Add(schema, "/duration", R"field(0.24)field");
		if (schema.type == "ability") Add(schema, "/maxCharges", R"field(1)field");
		if (schema.type == "ability") Add(schema, "/settings", R"field({})field");
		if (schema.type == "ability") Add(schema, "/settings/baseDistance", R"field(260)field");
		if (schema.type == "ability") Add(schema, "/settings/cameraZoomOutRatio", R"field(0.15)field");
		if (schema.type == "ability") Add(schema, "/progression", R"field({})field");
		if (schema.type == "ability") Add(schema, "/progression/scrapCost", R"field({})field");
		if (schema.type == "ability") Add(schema, "/progression/scrapCost/base", R"field(40)field");
		if (schema.type == "ability") Add(schema, "/progression/scrapCost/step", R"field(10)field");
		if (schema.type == "ability") Add(schema, "/progression/steps", R"field({})field");
		if (schema.type == "ability") Add(schema, "/progression/steps/attributeModifiers", R"field([])field");
		if (schema.type == "ability") Add(schema, "/progression/steps/attributeModifiers/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/progression/steps/attributeModifiers/*/attributeId", R"field("Ability.Movement.Dash.MoveSpeedScale")field");
		if (schema.type == "ability") Add(schema, "/progression/steps/attributeModifiers/*/operation", R"field("Add")field");
		if (schema.type == "ability") Add(schema, "/progression/steps/attributeModifiers/*/magnitude", R"field(0.1)field");
		if (schema.type == "ability") Add(schema, "/attributes", R"field([])field");
		if (schema.type == "ability") Add(schema, "/attributes/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/attributes/*/id", R"field("Ability.Movement.Dash.MoveSpeedScale")field");
		if (schema.type == "ability") Add(schema, "/attributes/*/baseValue", R"field(0.8)field");
		if (schema.type == "ability") Add(schema, "/attributes/*/minValue", R"field(0)field");
		if (schema.type == "ability") Add(schema, "/invocationOutputAttributes", R"field([])field");
		if (schema.type == "ability") Add(schema, "/invocationOutputAttributes/*", R"field("Ability.Movement.Dash.MoveSpeedScale")field");
		if (schema.type == "ability") Add(schema, "/effectSpecs", R"field([])field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/effectId", R"field("Effect.Barrier.Basic")field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/useAbilityDuration", R"field(true)field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/maxStacks", R"field(1)field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/attributes", R"field([])field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/attributes/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/attributes/*/id", R"field("Effect.BarrierCapacity")field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/attributes/*/baseValue", R"field(60)field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/attributes/*/minValue", R"field(0)field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/attributes/*/maxValue", R"field(1)field");
		if (schema.type == "ability") Add(schema, "/scalingRules", R"field([])field");
		if (schema.type == "ability") Add(schema, "/scalingRules/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/scalingRules/*/targetAttributeId", R"field("Effect.BarrierCapacity")field");
		if (schema.type == "ability") Add(schema, "/scalingRules/*/sourceAttributeId", R"field("Owner.MaxHealth")field");
		if (schema.type == "ability") Add(schema, "/scalingRules/*/operation", R"field("Add")field");
		if (schema.type == "ability") Add(schema, "/scalingRules/*/coefficient", R"field(0.2)field");
		if (schema.type == "ability") Add(schema, "/progression/steps/scalingRules", R"field([])field");
		if (schema.type == "ability") Add(schema, "/progression/steps/scalingRules/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/progression/steps/scalingRules/*/targetAttributeId", R"field("Effect.BarrierCapacity")field");
		if (schema.type == "ability") Add(schema, "/progression/steps/scalingRules/*/sourceAttributeId", R"field("Owner.MaxHealth")field");
		if (schema.type == "ability") Add(schema, "/progression/steps/scalingRules/*/operation", R"field("Add")field");
		if (schema.type == "ability") Add(schema, "/progression/steps/scalingRules/*/coefficient", R"field(0.01)field");
		if (schema.type == "ability") Add(schema, "/damageTags", R"field([])field");
		if (schema.type == "ability") Add(schema, "/damageTags/*", R"field("Damage.Type.Energy")field");
		if (schema.type == "ability") Add(schema, "/attributes/*/maxValue", R"field(1)field");
		if (schema.type == "ability") Add(schema, "/recordInAbilityHistory", R"field(false)field");
		if (schema.type == "ability") Add(schema, "/actors", R"field([])field");
		if (schema.type == "ability") Add(schema, "/actors/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/actors/*/id", R"field("Actor.Ability.SunBeam.Strike.Basic")field");
		if (schema.type == "ability") Add(schema, "/actors/*/lifeTime", R"field(0)field");
		if (schema.type == "ability") Add(schema, "/actors/*/spawnDistance", R"field(0)field");
		if (schema.type == "ability") Add(schema, "/actors/*/attributes", R"field([])field");
		if (schema.type == "ability") Add(schema, "/actors/*/attributes/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/actors/*/attributes/*/id", R"field("Common.Damage")field");
		if (schema.type == "ability") Add(schema, "/actors/*/attributes/*/baseValue", R"field(40)field");
		if (schema.type == "ability") Add(schema, "/actors/*/attributes/*/minValue", R"field(1)field");
		if (schema.type == "ability") Add(schema, "/actors/*/attributes/*/maxValue", R"field(4)field");
		if (schema.type == "ability") Add(schema, "/attributeProfiles", R"field([])field");
		if (schema.type == "ability") Add(schema, "/attributeProfiles/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/attributeProfiles/*/id", R"field("AttributeProfile.GravityAnomaly.Field.Basic")field");
		if (schema.type == "ability") Add(schema, "/attributeProfiles/*/attributes", R"field([])field");
		if (schema.type == "ability") Add(schema, "/attributeProfiles/*/attributes/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/attributeProfiles/*/attributes/*/id", R"field("Common.Duration")field");
		if (schema.type == "ability") Add(schema, "/attributeProfiles/*/attributes/*/baseValue", R"field(2.5)field");
		if (schema.type == "ability") Add(schema, "/attributeProfiles/*/attributes/*/minValue", R"field(0.01)field");
		if (schema.type == "ability") Add(schema, "/attributeProfiles/*/attributes/*/maxValue", R"field(0.95)field");
		if (schema.type == "ability") Add(schema, "/actors/*/attributeProfileIds", R"field([])field");
		if (schema.type == "ability") Add(schema, "/actors/*/attributeProfileIds/*", R"field("AttributeProfile.GravityAnomaly.Field.Basic")field");
		if (schema.type == "ability") Add(schema, "/settings/minCancelDuration", R"field(1)field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/modifiers", R"field([])field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/modifiers/*", R"field({})field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/modifiers/*/attributeId", R"field("Owner.MovementSlow")field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/modifiers/*/operation", R"field("Add")field");
		if (schema.type == "ability") Add(schema, "/effectSpecs/*/modifiers/*/magnitude", R"field(0.35)field");
		if (schema.type == "ability") Add(schema, "/settings/minActiveDuration", R"field(1)field");
		if (schema.type == "ability") Add(schema, "/settings/maximumDeliveryRange", R"field(450)field");
		if (schema.type == "ability") Add(schema, "/settings/deliverySpeed", R"field(280)field");
		if (schema.type == "ability") Add(schema, "/settings/formationDuration", R"field(0.5)field");
		if (schema.type == "ability") Add(schema, "/settings/barrierRadius", R"field(250)field");
		if (schema.type == "ability") Add(schema, "/settings/maxHealthReference", R"field(250)field");
		if (schema.type == "ability") Add(schema, "/settings/maxHealthPerStep", R"field(250)field");
		if (schema.type == "ability") Add(schema, "/settings/healRatioPerStep", R"field(0.01)field");
		if (schema.type == "ability") Add(schema, "/progression/steps/attributeModifiers/*/firstStep", R"field(2)field");
		if (schema.type == "ability") Add(schema, "/progression/steps/attributeModifiers/*/everySteps", R"field(2)field");
		if (schema.type == "effect") Add(schema, "/id", R"field("Effect.Barrier.Basic")field");
		if (schema.type == "effect") Add(schema, "/behaviorKey", R"field("EffectBehavior.Barrier")field");
		if (schema.type == "effect") Add(schema, "/durationPolicy", R"field("Duration")field");
		if (schema.type == "effect") Add(schema, "/stackingPolicy", R"field("RefreshDuration")field");
		if (schema.type == "effect") Add(schema, "/grantedTags", R"field([])field");
		if (schema.type == "effect") Add(schema, "/grantedTags/*", R"field("State.Effect.Defense.Barrier")field");
		if (schema.type == "effect") Add(schema, "/disposition", R"field("Beneficial")field");
		if (schema.type == "effect") Add(schema, "/category", R"field("Defense.Barrier")field");
		if (schema.type == "effect") Add(schema, "/activeVisualId", R"field("Visual.Effect.Shield.Basic")field");
		if (schema.type == "effect") Add(schema, "/sourceParameterized", R"field(true)field");
		if (schema.type == "effect") Add(schema, "/stackLifetimePolicy", R"field("DecayAfterDuration")field");
		if (schema.type == "effect") Add(schema, "/stackDecayInterval", R"field(1)field");
		if (schema.type == "effect") Add(schema, "/cleanseable", R"field(true)field");
		if (schema.type == "effect") Add(schema, "/applicationBlockedTags", R"field([])field");
		if (schema.type == "effect") Add(schema, "/applicationBlockedTags/*", R"field("State.Effect.Immunity.Movement.Slow")field");
		if (schema.type == "effect") Add(schema, "/immunityCategory", R"field("Movement.Slow")field");
		if (schema.type == "effect") Add(schema, "/sourceScopedApplication", R"field(true)field");
		if (schema.type == "effect") Add(schema, "/duration", R"field(5)field");
		if (schema.type == "effect") Add(schema, "/grantedImmunityCategory", R"field("Movement.Slow")field");
		if (schema.type == "module") Add(schema, "/id", R"field("Module.Defense.ShieldGenerator.Standard")field");
		if (schema.type == "module") Add(schema, "/displayName", R"field("Standard Shield Generator")field");
		if (schema.type == "module") Add(schema, "/description", R"field("Provides the shield; removing it leaves you unshielded.")field");
		if (schema.type == "module") Add(schema, "/family", R"field("Ability.Defense")field");
		if (schema.type == "module") Add(schema, "/systemCategory", R"field("Module.System.Shield")field");
		if (schema.type == "module") Add(schema, "/sizeVariants", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shape", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shape/*", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shape/*/*", R"field(0)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/condition", R"field("PrimaryShotOnly")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/damageMultiplier", R"field(1.3)field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/attributeModifiers", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/attributeModifiers/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/attributeModifiers/*/target", R"field("PrimaryWeapon")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/attributeModifiers/*/attributeId", R"field("PrimaryWeapon.Projectile.Delivery.PierceCount")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/attributeModifiers/*/operation", R"field("Add")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/attributeModifiers/*/magnitude", R"field(1)field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/attributeModifiers/*/affectedByZoneBonus", R"field(false)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/openingShotCount", R"field(3)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/abilityModifiers", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/abilityModifiers/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/abilityModifiers/*/attributeId", R"field("Common.Damage")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/abilityModifiers/*/operation", R"field("Multiply")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/abilityModifiers/*/magnitude", R"field(1.12)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/abilityModifiers/*/affectedByZoneBonus", R"field(false)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/trigger", R"field("AbilityActivated")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/requiredEventTags", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/requiredEventTags/*", R"field("Ability.Offense")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/durationSeconds", R"field(3)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/modifiers", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/modifiers/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/modifiers/*/target", R"field("Owner")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/modifiers/*/attributeId", R"field("Owner.AttackSpeed")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/modifiers/*/operation", R"field("Add")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/modifiers/*/magnitude", R"field(15)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/modifiers/*/affectedByZoneBonus", R"field(false)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/bonusDamageFraction", R"field(0.3)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/bonusDamageType", R"field("Damage.Type.Energy")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/bonusCriticalPolicy", R"field("Disabled")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/activeWhileTrigger", R"field("ShieldBroken")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/activeDurationSeconds", R"field(5)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/triggerBuffs/*/movementSpeedMultiplier", R"field(1.25)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/conditionValue", R"field(0.25)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/zoneEffects", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/zoneEffects/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/zoneEffects/*/zone", R"field("Module.Zone.Wings")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/zoneEffects/*/scope", R"field("FamilyOfZone")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/zoneEffects/*/kind", R"field("CooldownMultiplier")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/zoneEffects/*/seconds", R"field([0.25, 0.30, 0.35])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/monochromeCoreRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/monochromeCoreRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/monochromeCoreRules/*/pairMultiplier", R"field([1.25, 1.30, 1.35])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/monochromeCoreRules/*/additionalAbilityBonus", R"field(0.25)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/zoneEffects/*/multiplier", R"field(0.79)field");
		if (schema.type == "module") Add(schema, "/rarity", R"field("Uncommon")field");
		if (schema.type == "module") Add(schema, "/levelCount", R"field(3)field");
		if (schema.type == "module") Add(schema, "/sideFeatures", R"field([])field");
		if (schema.type == "module") Add(schema, "/sideFeatures/*", R"field("Module.SideFeature.GlassTop.ShieldReturn")field");
		if (schema.type == "module") Add(schema, "/sharedPools", R"field([])field");
		if (schema.type == "module") Add(schema, "/sharedPools/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sharedPools/*/poolId", R"field("Module.SidePool.Offense")field");
		if (schema.type == "module") Add(schema, "/sharedPools/*/allowedFeatureIds", R"field([])field");
		if (schema.type == "module") Add(schema, "/sharedPools/*/allowedFeatureIds/*", R"field("Module.SideFeature.Shared.O2")field");
		if (schema.type == "module") Add(schema, "/sizeLadder", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeLadder/*", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/attributeModifiers/*/magnitude/*", R"field(12)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions/*/trigger", R"field("KillConfirmed")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions/*/kind", R"field("RestoreShield")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions/*/amount", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions/*/amount/*", R"field(0.02)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions/*/abilityFamily", R"field("Ability.Utility")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules/*/source", R"field("MovementSpeed")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules/*/minimumSpeed", R"field(200)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules/*/speedStep", R"field(100)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules/*/attributeId", R"field("Owner.AttackSpeed")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules/*/amount", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules/*/amount/*", R"field(3.5)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/incoming", R"field(true)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/damageMultiplier/*", R"field(0.8)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/maxHealthDamageFraction", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/maxHealthDamageFraction/*", R"field(0.03)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldReflections", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldReflections/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldReflections/*/damageType", R"field("Damage.Type.Energy")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldReflections/*/fraction", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldReflections/*/fraction/*", R"field(0.35)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldReflections/*/durationSeconds", R"field(5)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldReflections/*/electricStacks", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/electricOverrides", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/electricOverrides/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/electricOverrides/*/damageTakenByStack", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/electricOverrides/*/damageTakenByStack/*", R"field(0.03)field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/electricOverrides/*/damageTakenByStack/*/*", R"field(0.19)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/cryoFreezes", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/cryoFreezes/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/cryoFreezes/*/threshold", R"field(4)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/cryoFreezes/*/durationSeconds", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/cryoFreezes/*/durationSeconds/*", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/cryoFreezes/*/cooldownSeconds", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/cryoFreezes/*/cooldownSeconds/*", R"field(8)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/cryoFreezes/*/normalTargetsOnly", R"field(true)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/ricochets", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/ricochets/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/ricochets/*/damageFraction", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/ricochets/*/damageFraction/*", R"field(0.25)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/ricochets/*/range", R"field(450)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos/*/damageType", R"field("Damage.Type.Kinetic")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos/*/damageFraction", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos/*/damageFraction/*", R"field(0.15)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos/*/projectileCount", R"field(3)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos/*/speed", R"field(600)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos/*/lifetime", R"field(3)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos/*/targetRange", R"field(1800)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/guidedSalvos/*/maxActive", R"field(64)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/damageType", R"field("Damage.Type.Kinetic")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/damage", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/damage/*", R"field(30)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/spacing", R"field(50)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/windowSeconds", R"field(5)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/fuseSeconds", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/radius", R"field(100)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/maxActive", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/maxActive/*", R"field(12)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/attackPowerCoefficient", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/trailMines/*/attackPowerCoefficient/*", R"field(0.3)field");
		if (schema.type == "module") Add(schema, "/conflictsWith", R"field([])field");
		if (schema.type == "module") Add(schema, "/conflictsWith/*", R"field("Module.Defense.ContinuousFieldGenerator.Rare")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/regenMode", R"field("SpeedScaled")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/speedMultiplierAtMax", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/speedMultiplierAtMax/*", R"field(0.5)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/minimumSpeed", R"field(200)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/continuousRateFraction", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/continuousRateFraction/*", R"field(0.2)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/breakRearmFraction", R"field(0.25)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/armorShare", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/armorShare/*", R"field(0.5)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/requiredEmptyAbilitySlots", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/perEmptyCellMultiplier", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/perEmptyCellMultiplier/*", R"field(0.015)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/movementRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/movementRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/movementRules/*/thrustMultiplier", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/movementRules/*/thrustMultiplier/*", R"field(0.7)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/movementRules/*/equalizeDirections", R"field(true)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/source", R"field("Distance")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/unitsPerStack", R"field(5000)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/damagePerStack", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/damagePerStack/*", R"field(0.01)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/moveSpeedPerStack", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/moveSpeedPerStack/*", R"field(0.01)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/thermalBursts", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/thermalBursts/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/thermalBursts/*/stackThreshold", R"field(4)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/thermalBursts/*/damageFraction", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/thermalBursts/*/damageFraction/*", R"field(1.3)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/kineticConsumes", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/kineticConsumes/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/kineticConsumes/*/stackThreshold", R"field(4)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/kineticConsumes/*/damageMultiplier", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/kineticConsumes/*/damageMultiplier/*", R"field(1.1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/damageRules/*/ratioSlope", R"field(0.05)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/sustainedSpeedRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/sustainedSpeedRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/sustainedSpeedRules/*/speedFraction", R"field(0.9)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/sustainedSpeedRules/*/holdSeconds", R"field(2)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/sustainedSpeedRules/*/primarySpeedMultiplier", R"field(2)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/sustainedSpeedRules/*/primaryPierceBonus", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/sustainedSpeedRules/*/primaryDamageMultiplier", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/sustainedSpeedRules/*/primaryDamageMultiplier/*", R"field(1.1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/killWeights", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/killWeights/normal", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/killWeights/elite", R"field(7.5)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/killWeights/miniboss", R"field(7.5)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/killWeights/boss", R"field(25)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/adjacentAbilityLimit", R"field(3)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*/trigger", R"field("KillConfirmed")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*/radius", R"field(350)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*/damage", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*/damage/*", R"field(60)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*/energyPowerCoefficient", R"field(0.4)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*/damageType", R"field("Damage.Type.Thermal")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*/igniteStacks", R"field(4)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*/chargeKills", R"field(10)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerBurstRules/*/chargeRadius", R"field(300)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/chargeRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/chargeRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/chargeRules/*/chargePerLevel", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/chargeRules/*/chargePerLevel/*", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions/*/requiredEventTags", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions/*/requiredEventTags/*", R"field("Ability.Defense")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/eventActions/*/durationSeconds", R"field(6)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/primaryStatusPrimers", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/primaryStatusPrimers/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/statusRules/primaryStatusPrimers/*/cooldownSeconds", R"field(1)field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/requiredFamily", R"field("Ability.Defense")field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/armorPerStack", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/armorCap", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/runCounterRules/*/armorCap/*", R"field(30)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/speedLances", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/speedLances/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/speedLances/*/speedDamageCoefficient", R"field(0.4)field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/speedLances/*/sampleSeconds", R"field(1)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/speedLances/*/projectileSpeed", R"field(1800)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/speedLances/*/range", R"field(900)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/annularBursts", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/annularBursts/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/annularBursts/*/innerRadius", R"field(600)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/annularBursts/*/outerRadius", R"field(900)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/annularBursts/*/baseDamage", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/annularBursts/*/baseDamage/*", R"field(50)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/annularBursts/*/energyPowerCoefficient", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/generatedRules/annularBursts/*/energyPowerCoefficient/*", R"field(0.4)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules/*/maximumAmount", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/ownerAttributeRules/*/maximumAmount/*", R"field(60)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRules/*/frontRearSplit", R"field(true)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldBatteryRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldBatteryRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldBatteryRules/*/reserveFraction", R"field([])field", true);
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldBatteryRules/*/reserveFraction/*", R"field(0.3)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRuptureRules", R"field([])field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRuptureRules/*", R"field({})field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRuptureRules/*/damageFraction", R"field(0.5)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRuptureRules/*/radius", R"field(150)field");
		if (schema.type == "module") Add(schema, "/sizeVariants/*/shieldRuptureRules/*/historySeconds", R"field(3)field");
		if (schema.type == "ship") Add(schema, "/id", R"field("Ship.Player.Fighter.Basic")field");
		if (schema.type == "ship") Add(schema, "/health", R"field(250)field");
		if (schema.type == "ship") Add(schema, "/speed", R"field([])field", true);
		if (schema.type == "ship") Add(schema, "/speed/*", R"field(350)field");
		if (schema.type == "ship") Add(schema, "/collisionDamage", R"field(25)field");
		if (schema.type == "ship") Add(schema, "/score", R"field(0)field");
		if (schema.type == "ship") Add(schema, "/shipXPReward", R"field(0)field");
		if (schema.type == "ship") Add(schema, "/primaryWeaponId", R"field("Weapon.Projectile.FighterRapidLaser.Basic")field");
		if (schema.type == "ship") Add(schema, "/schematic", R"field({})field");
		if (schema.type == "ship") Add(schema, "/schematic/legend", R"field({})field");
		if (schema.type == "ship") Add(schema, "/schematic/legend/N", R"field("Module.Zone.Nose")field");
		if (schema.type == "ship") Add(schema, "/schematic/legend/W", R"field("Module.Zone.Wings")field");
		if (schema.type == "ship") Add(schema, "/schematic/legend/H", R"field("Module.Zone.Hull")field");
		if (schema.type == "ship") Add(schema, "/schematic/legend/C", R"field("Module.Zone.Core")field");
		if (schema.type == "ship") Add(schema, "/schematic/legend/E", R"field("Module.Zone.Engine")field");
		if (schema.type == "ship") Add(schema, "/schematic/legend/S", R"field("Module.Zone.Sensor")field");
		if (schema.type == "ship") Add(schema, "/schematic/legend/B", R"field("Module.Zone.AbilityBay")field");
		if (schema.type == "ship") Add(schema, "/schematic/lockedLegend", R"field({})field");
		if (schema.type == "ship") Add(schema, "/schematic/lockedLegend/c", R"field("Module.Zone.Core")field");
		if (schema.type == "ship") Add(schema, "/schematic/lockedLegend/e", R"field("Module.Zone.Engine")field");
		if (schema.type == "ship") Add(schema, "/schematic/lockedLegend/h", R"field("Module.Zone.Hull")field");
		if (schema.type == "ship") Add(schema, "/schematic/lockedLegend/n", R"field("Module.Zone.Nose")field");
		if (schema.type == "ship") Add(schema, "/schematic/lockedLegend/s", R"field("Module.Zone.Sensor")field");
		if (schema.type == "ship") Add(schema, "/schematic/lockedLegend/w", R"field("Module.Zone.Wings")field");
		if (schema.type == "ship") Add(schema, "/schematic/rows", R"field([])field");
		if (schema.type == "ship") Add(schema, "/schematic/rows/*", R"field(".......nn.......")field");
		if (schema.type == "ship") Add(schema, "/schematic/defaultModules", R"field([])field");
		if (schema.type == "ship") Add(schema, "/schematic/defaultModules/*", R"field({})field");
		if (schema.type == "ship") Add(schema, "/schematic/defaultModules/*/moduleId", R"field("Module.Primary.PrimaryWeapon.Standard")field");
		if (schema.type == "ship") Add(schema, "/schematic/defaultModules/*/variant", R"field(0)field");
		if (schema.type == "ship") Add(schema, "/schematic/defaultModules/*/origin", R"field([])field");
		if (schema.type == "ship") Add(schema, "/schematic/defaultModules/*/origin/*", R"field(7)field");
		if (schema.type == "ship") Add(schema, "/schematic/defaultModules/*/rotation", R"field(0)field");
		if (schema.type == "ship") Add(schema, "/baseOwnerAttributes", R"field([])field");
		if (schema.type == "ship") Add(schema, "/baseOwnerAttributes/*", R"field({})field");
		if (schema.type == "ship") Add(schema, "/baseOwnerAttributes/*/attributeId", R"field("Owner.AttackPower")field");
		if (schema.type == "ship") Add(schema, "/baseOwnerAttributes/*/baseValue", R"field(39)field");
		if (schema.type == "ship") Add(schema, "/movement", R"field({})field");
		if (schema.type == "ship") Add(schema, "/movement/forwardThrust", R"field(650)field");
		if (schema.type == "ship") Add(schema, "/movement/reverseThrust", R"field(190)field");
		if (schema.type == "ship") Add(schema, "/movement/strafeThrust", R"field(270)field");
		if (schema.type == "ship") Add(schema, "/movement/angularTurnSpeed", R"field(400)field");
		if (schema.type == "ship") Add(schema, "/movement/angularTurnResponsiveness", R"field(5)field");
		if (schema.type == "ship") Add(schema, "/movement/linearDamping", R"field(0.7)field");
		if (schema.type == "ship") Add(schema, "/movement/maxSpeed", R"field(520)field");
		if (schema.type == "ship") Add(schema, "/movement/inputResponsiveness", R"field(12)field");
		if (schema.type == "ship") Add(schema, "/movement/mouseAimDeadZone", R"field(32)field");
		if (schema.type == "ship") Add(schema, "/movement/horizontalRatingToThrust", R"field(60)field");
		if (schema.type == "ship") Add(schema, "/movement/verticalRatingToThrust", R"field(60)field");
		if (schema.type == "ship") Add(schema, "/movement/ratingToMaxSpeed", R"field(20)field");
		if (schema.type == "ship") Add(schema, "/movement/movementRatingDiminishingScale", R"field(20)field");
		if (schema.type == "ship") Add(schema, "/energy", R"field({})field");
		if (schema.type == "ship") Add(schema, "/energy/baseEnergyPower", R"field(35)field");
		if (schema.type == "ship") Add(schema, "/energy/baseMaxShield", R"field(140)field");
		if (schema.type == "ship") Add(schema, "/energy/maxShieldPerLevel", R"field(20)field");
		if (schema.type == "ship") Add(schema, "/energy/shieldFullRechargeDuration", R"field(6)field");
		if (schema.type == "ship") Add(schema, "/energy/baseShieldRechargeDelay", R"field(6)field");
		if (schema.type == "ship") Add(schema, "/energy/baseAfterburnerCapacity", R"field(100)field");
		if (schema.type == "ship") Add(schema, "/energy/afterburnerCapacityPerLevel", R"field(3)field");
		if (schema.type == "ship") Add(schema, "/energy/afterburnerFullRechargeDuration", R"field(6)field");
		if (schema.type == "ship") Add(schema, "/energy/baseAfterburnerRechargeDelay", R"field(1.5)field");
		if (schema.type == "ship") Add(schema, "/energy/baseAfterburnerSpeedMultiplier", R"field(1.55)field");
		if (schema.type == "ship") Add(schema, "/energy/baseAfterburnerAccelerationMultiplier", R"field(1.8)field");
		if (schema.type == "ship") Add(schema, "/energy/baseAfterburnerEnergyDrainPerSecond", R"field(33)field");
		if (schema.type == "ship") Add(schema, "/energy/baseAfterburnerRampUpDuration", R"field(0.22)field");
		if (schema.type == "ship") Add(schema, "/energy/baseAfterburnerRampDownDuration", R"field(0.45)field");
		if (schema.type == "ship") Add(schema, "/energy/baseAfterburnerManeuverabilityMultiplier", R"field(0.9)field");
		if (schema.type == "ship") Add(schema, "/progression", R"field({})field");
		if (schema.type == "ship") Add(schema, "/progression/baseXP", R"field(6)field");
		if (schema.type == "ship") Add(schema, "/progression/xpExponent", R"field(0)field");
		if (schema.type == "ship") Add(schema, "/progression/xpPerLevel", R"field(1)field");
		if (schema.type == "ship") Add(schema, "/progression/naturalGrowth", R"field([])field");
		if (schema.type == "ship") Add(schema, "/progression/naturalGrowth/*", R"field({})field");
		if (schema.type == "ship") Add(schema, "/progression/naturalGrowth/*/attributeId", R"field("Owner.MaxHealth")field");
		if (schema.type == "ship") Add(schema, "/progression/naturalGrowth/*/perLevel", R"field(40)field");
		if (schema.type == "ship") Add(schema, "/texturePath", R"field("SpaceShooterRedux/PNG/playerShip2_orange.png")field");
		if (schema.type == "ship") Add(schema, "/engineMounts", R"field([])field");
		if (schema.type == "ship") Add(schema, "/engineMounts/*", R"field({})field");
		if (schema.type == "ship") Add(schema, "/engineMounts/*/offset", R"field([])field");
		if (schema.type == "ship") Add(schema, "/engineMounts/*/offset/*", R"field(-29)field");
		if (schema.type == "ship") Add(schema, "/rewards", R"field([])field");
		if (schema.type == "ship") Add(schema, "/rewards/*", R"field({})field");
		if (schema.type == "ship") Add(schema, "/rewards/*/type", R"field("Health")field");
		if (schema.type == "ship") Add(schema, "/rewards/*/weight", R"field(0.2)field");
		if (schema.type == "weapon") Add(schema, "/id", R"field("Weapon.Projectile.FighterRapidLaser.Basic")field");
		if (schema.type == "weapon") Add(schema, "/typeTag", R"field("PrimaryWeapon.Projectile.Standard")field");
		if (schema.type == "weapon") Add(schema, "/presentation", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/presentation/texturePath", R"field("SpaceShooterRedux/PNG/Lasers/laserBlue01.png")field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/shaderPath", R"field("SpaceShooterRedux/Shaders/point_light.frag")field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/color", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/color/*", R"field(75)field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/intensity", R"field(0.3)field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/size", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/size/*", R"field(18)field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/shouldStretch", R"field(false)field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/complexTrail", R"field(false)field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/taperAmount", R"field(0)field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/edgeSoftness", R"field(0.4)field");
		if (schema.type == "weapon") Add(schema, "/presentation/pointLight/shapeRoundness", R"field(0.7)field");
		if (schema.type == "weapon") Add(schema, "/presentation/lightOffset", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/presentation/lightOffset/*", R"field(0)field");
		if (schema.type == "weapon") Add(schema, "/presentation/visualScale", R"field(1)field");
		if (schema.type == "weapon") Add(schema, "/attributes", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/attributes/*", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/attributes/*/id", R"field("Common.Damage")field");
		if (schema.type == "weapon") Add(schema, "/attributes/*/baseValue", R"field(10)field");
		if (schema.type == "weapon") Add(schema, "/attributes/*/minValue", R"field(0)field");
		if (schema.type == "weapon") Add(schema, "/attributes/*/maxValue", R"field(48)field");
		if (schema.type == "weapon") Add(schema, "/muzzles", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/muzzles/*", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/muzzles/*/offset", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/muzzles/*/offset/*", R"field(0)field");
		if (schema.type == "weapon") Add(schema, "/muzzles/*/rotationOffset", R"field(0)field");
		if (schema.type == "weapon") Add(schema, "/automaticFire", R"field(true)field");
		if (schema.type == "weapon") Add(schema, "/progression", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/progression/scrapCost", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/progression/scrapCost/base", R"field(40)field");
		if (schema.type == "weapon") Add(schema, "/progression/scrapCost/step", R"field(5)field");
		if (schema.type == "weapon") Add(schema, "/progression/rules", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/firstLevel", R"field(2)field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/levelInterval", R"field(1)field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/attributeModifiers", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/attributeModifiers/*", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/attributeModifiers/*/attributeId", R"field("Common.Damage")field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/attributeModifiers/*/operation", R"field("Add")field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/attributeModifiers/*/magnitude", R"field(5)field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/scalingRules", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/scalingRules/*", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/scalingRules/*/targetAttributeId", R"field("PrimaryWeapon.Empowered.BonusDamage")field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/scalingRules/*/sourceAttributeId", R"field("Owner.Energy.Power")field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/scalingRules/*/operation", R"field("Add")field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/scalingRules/*/coefficient", R"field(0.03)field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/unlockedUpgradeIds", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/progression/rules/*/reward/unlockedFeatureTags", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/attributeModifiers", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/scalingRules", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/scalingRules/*", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/scalingRules/*/targetAttributeId", R"field("Common.Damage")field");
		if (schema.type == "weapon") Add(schema, "/scalingRules/*/sourceAttributeId", R"field("Owner.AttackPower")field");
		if (schema.type == "weapon") Add(schema, "/scalingRules/*/operation", R"field("Add")field");
		if (schema.type == "weapon") Add(schema, "/scalingRules/*/coefficient", R"field(0.41)field");
		if (schema.type == "weapon") Add(schema, "/featureTags", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/heatGainCurve", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/damageTags", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/damageTags/*", R"field("Damage.Type.Photonic")field");
		if (schema.type == "weapon") Add(schema, "/cadenceMode", R"field("OwnerAttackSpeedPercentage")field");
		if (schema.type == "weapon") Add(schema, "/damageRoundingPolicy", R"field("CeilFinalDamage")field");
		if (schema.type == "weapon") Add(schema, "/magazine", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/magazine/capacity", R"field(48)field");
		if (schema.type == "weapon") Add(schema, "/magazine/baseReloadTime", R"field(2)field");
		if (schema.type == "weapon") Add(schema, "/empoweredShot", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/empoweredShot/guaranteedCritical", R"field(true)field");
		if (schema.type == "weapon") Add(schema, "/empoweredShot/trigger", R"field("EverySuccessfulHits")field");
		if (schema.type == "weapon") Add(schema, "/empoweredShot/statusStacksOnlyWhenEmpowered", R"field(true)field");
		if (schema.type == "weapon") Add(schema, "/empoweredShot/damageTags", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/empoweredShot/damageTags/*", R"field("Damage.Type.Kinetic")field");
		if (schema.type == "weapon") Add(schema, "/empoweredShot/chargesPerCast", R"field(2)field");
		if (schema.type == "weapon") Add(schema, "/empoweredShot/chargedFireRate", R"field(4)field");
		if (schema.type == "weapon") Add(schema, "/featureTags/*", R"field("PrimaryWeapon.Feature.Heat")field");
		if (schema.type == "weapon") Add(schema, "/heatGainCurve/*", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/heatGainCurve/*/endHeatPercentage", R"field(50)field");
		if (schema.type == "weapon") Add(schema, "/heatGainCurve/*/gainMultiplier", R"field(1)field");
		if (schema.type == "weapon") Add(schema, "/heatDamageRegions", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/heatDamageRegions/*", R"field({})field");
		if (schema.type == "weapon") Add(schema, "/heatDamageRegions/*/upToHeatPercentage", R"field(35)field");
		if (schema.type == "weapon") Add(schema, "/heatDamageRegions/*/damageMultiplier", R"field(1)field");
		if (schema.type == "weapon") Add(schema, "/heatDamageRegions/*/damageTags", R"field([])field");
		if (schema.type == "weapon") Add(schema, "/heatDamageRegions/*/damageTags/*", R"field("Damage.Type.Energy")field");

		if (schema.type == "module")
		{
			for (const char* name : { "reviveRules", "healthThresholdRules", "cascadeRules", "fieldMasteryRules", "assaultVectorRules", "phaseSlipRules", "prismCoreRules", "bloodContractRules", "stormCapacitorRules", "clotArmorRules", "debtCollectionRules", "primaryMountRules", "familyStatRules" })
			{
				const std::string path = std::string{ "/sizeVariants/*/" } + name;
				Add(schema, path, R"field([])field");
				Add(schema, path + "/*", R"field({})field");
			}
			Add(schema, "/sizeVariants/*/stormCapacitorRules/*/primaryDamageFraction", R"field(0.5)field");
		}

		if (schema.type == "ability") Add(schema, "/mainEffectAttributes", R"field([])field");
		if (schema.type == "ability") Add(schema, "/mainEffectAttributes/*", R"field("Common.Damage")field");
		if (schema.type == "module")
		{
			for (const char* tablePath : {
				"/sizeVariants/*/abilityModifiers/*/magnitude",
				"/sizeVariants/*/assaultVectorRules/*/armorBonus",
				"/sizeVariants/*/assaultVectorRules/*/damageBonus",
				"/sizeVariants/*/attributeModifiers/*/magnitude",
				"/sizeVariants/*/bloodContractRules/*/emptyHealthMultiplier",
				"/sizeVariants/*/bloodContractRules/*/fullHealthMultiplier",
				"/sizeVariants/*/cascadeRules/*/stepBonus",
				"/sizeVariants/*/chargeRules/*/chargePerLevel",
				"/sizeVariants/*/conditionalMovementRules/*/multiplier",
				"/sizeVariants/*/damageRules/*/activeDurationSeconds",
				"/sizeVariants/*/damageRules/*/bonusDamageFraction",
				"/sizeVariants/*/damageRules/*/conditionValue",
				"/sizeVariants/*/damageRules/*/damageMultiplier",
				"/sizeVariants/*/damageRules/*/maxHealthDamageFraction",
				"/sizeVariants/*/damageRules/*/openingShotCount",
				"/sizeVariants/*/damageRules/*/perEmptyCellMultiplier",
				"/sizeVariants/*/eventActions/*/amount",
				"/sizeVariants/*/eventActions/*/chancePerLuck",
				"/sizeVariants/*/eventActions/*/targetCount",
				"/sizeVariants/*/familyStatRules/*/perModule",
				"/sizeVariants/*/fieldMasteryRules/*/usesPerLevelUp",
				"/sizeVariants/*/generatedRules/annularBursts/*/baseDamage",
				"/sizeVariants/*/generatedRules/annularBursts/*/energyPowerCoefficient",
				"/sizeVariants/*/generatedRules/delayedExplosions/*/damageFraction",
				"/sizeVariants/*/generatedRules/guidedSalvos/*/damageFraction",
				"/sizeVariants/*/generatedRules/killExplosions/*/maxHealthFraction",
				"/sizeVariants/*/generatedRules/ricochets/*/damageFraction",
				"/sizeVariants/*/generatedRules/speedLances/*/speedDamageCoefficient",
				"/sizeVariants/*/generatedRules/trailMines/*/attackPowerCoefficient",
				"/sizeVariants/*/generatedRules/trailMines/*/damage",
				"/sizeVariants/*/generatedRules/trailMines/*/maxActive",
				"/sizeVariants/*/healthThresholdRules/*/cooldownRecoveryMultiplier",
				"/sizeVariants/*/movementRules/*/thrustMultiplier",
				"/sizeVariants/*/monochromeCoreRules/*/pairMultiplier",
				"/sizeVariants/*/ownerAttributeRules/*/amount",
				"/sizeVariants/*/ownerAttributeRules/*/maximumAmount",
				"/sizeVariants/*/ownerBurstRules/*/damage",
				"/sizeVariants/*/ownerBurstRules/*/damageFractionOfMaxHealth",
				"/sizeVariants/*/phaseSlipRules/*/explosionRadius",
				"/sizeVariants/*/phaseSlipRules/*/primaryDamageMultiplier",
				"/sizeVariants/*/primaryMountRules/*/damageMultiplier",
				"/sizeVariants/*/prismCoreRules/*/statusDurationMultiplier",
				"/sizeVariants/*/reviveRules/*/charges",
				"/sizeVariants/*/reviveRules/*/healthFraction",
				"/sizeVariants/*/runCounterRules/*/armorCap",
				"/sizeVariants/*/runCounterRules/*/bossDamagePerKill",
				"/sizeVariants/*/runCounterRules/*/damagePerStack",
				"/sizeVariants/*/runCounterRules/*/eliteDamagePerKill",
				"/sizeVariants/*/runCounterRules/*/moveSpeedPerStack",
				"/sizeVariants/*/shieldBatteryRules/*/reserveFraction",
				"/sizeVariants/*/shieldReflections/*/fraction",
				"/sizeVariants/*/shieldRules/*/armorShare",
				"/sizeVariants/*/shieldRules/*/breakRearmFraction",
				"/sizeVariants/*/shieldRules/*/continuousRateFraction",
				"/sizeVariants/*/shieldRules/*/speedMultiplierAtMax",
				"/sizeVariants/*/shieldRuptureRules/*/damageFraction",
				"/sizeVariants/*/statusRules/cryoFreezes/*/cooldownSeconds",
				"/sizeVariants/*/statusRules/cryoFreezes/*/durationSeconds",
				"/sizeVariants/*/statusRules/durationBonuses/*/bonusSeconds",
				"/sizeVariants/*/statusRules/durationBonuses/*/durationMultiplier",
				"/sizeVariants/*/statusRules/electricOverrides/*/damageTakenByStack",
				"/sizeVariants/*/statusRules/kineticConsumes/*/damageMultiplier",
				"/sizeVariants/*/statusRules/primaryStatusPrimers/*/cooldownSeconds",
				"/sizeVariants/*/statusRules/thermalBursts/*/damageFraction",
				"/sizeVariants/*/statusRules/thermalCryoDurationBonuses/*/bonusSeconds",
				"/sizeVariants/*/sustainedSpeedRules/*/primaryDamageMultiplier",
				"/sizeVariants/*/triggerBuffs/*/durationSeconds",
				"/sizeVariants/*/triggerBuffs/*/modifiers/*/magnitude",
				"/sizeVariants/*/triggerBuffs/*/movementSpeedMultiplier",
				"/sizeVariants/*/zoneEffects/*/multiplier",
				"/sizeVariants/*/zoneEffects/*/seconds" })
			{
				const std::string path{ tablePath };
				Add(schema, path, R"field([1, 1, 1])field", true);
				Add(schema, path + "/*", R"field(1)field");
				Add(schema, path + "Growth", R"field({"type":"linear"})field");
				Add(schema, path + "Growth/type", R"field("linear")field");
				Add(schema, path + "Growth/ratio", R"field(1.05)field");
				Add(schema, path + "Growth/every", R"field(2)field");
				Add(schema, path + "Growth/amount", R"field(1)field");
			}
		}
