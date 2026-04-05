// SkillVFXData.cpp — Skill VFX config registry.
// Rebuilt every call (no static caching) so Live Coding patches take effect immediately.
//
// ===== HOW TO ADD A NEW SKILL VFX =====
//
// 1. Copy any existing block below (pick one with a similar template type)
// 2. Change the SkillId and comment
// 3. Set only the fields that differ from defaults — everything else is already sensible
// 4. Run PIE — check LogSkillVFX output for asset load warnings
//
// Template types:
//   BoltFromSky     — N bolts strike from above (Cold/Fire/Lightning Bolt)
//   Projectile      — Travels from attacker to target (Fire Ball, Soul Strike)
//   AoEImpact       — One-shot burst at location (Bash, Magnum Break)
//   GroundPersistent — Looping ground effect (Fire Wall, Safety Wall)
//   GroundAoERain   — Staggered strikes in AoE (Thunderstorm)
//   SelfBuff        — Aura on caster (Endure, Sight)
//   TargetDebuff    — Effect on target (Provoke, Frost Diver, Stone Curse)
//   HealFlash       — Quick heal visual (First Aid)
//
// Available Niagara assets (already in project):
//   /Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Buff
//   /Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1
//   /Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2
//   /Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1
//   /Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Area1
//   /Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Circle1
//   /Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Circle2
//   /Game/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Mist
//   /Game/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Solo_Impact
//   /Game/Mixed_Magic_VFX_Pack/VFX/NS_Lightning_Strike
//   /Game/Mixed_Magic_VFX_Pack/VFX/NS_Magma_Shot
//   /Game/Mixed_Magic_VFX_Pack/VFX/NS_Magic_Bubbles
//   /Game/Mixed_Magic_VFX_Pack/VFX/NS_Potion
//   /Game/Mixed_Magic_VFX_Pack/VFX/Sperate_VFX/NS_Magma_Shot_Impact
//   /Game/Mixed_Magic_VFX_Pack/VFX/Sperate_VFX/NS_Magma_Shot_Projectile
//   /Game/Vefects/Zap_VFX/VFX/Zap/Particles/NS_Zap_03_Yellow
//   /Game/Knife_light/VFX/NE_attack01
//
// Available Cascade assets (InfinityBlade — verify these exist in your project!):
//   /Game/InfinityBladeEffects/Effects/FX_Archive/P_Enrage_Base
//   /Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Elemental/ICE/P_Elemental_Ice_Proj
//   /Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Spider/ICE/P_Ice_Proj_charge_01
//   /Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Elemental/Fire/P_ElementalFire_Lg
//   /Game/InfinityBladeEffects/Effects/FX_Ambient/Fire/P_Env_Fire_Grate_01
//   /Game/InfinityBladeEffects/Effects/FX_Mobile/Misc/P_levelUp_Detail
//

#include "SkillVFXData.h"

// Helper: create a config with just the required fields, everything else defaults.
// Use named-field style: set C.Field = value for anything non-default.
static void AddConfig(TMap<int32, FSkillVFXConfig>& Configs, FSkillVFXConfig C)
{
	// Auto-derive casting circle color from element if not explicitly set
	if (C.bUseCastingCircle && C.CastingCircleColor == FLinearColor(0.3f, 0.8f, 1.0f, 1.0f))
	{
		C.CastingCircleColor = SkillVFXDataHelper::GetElementColor(C.Element);
	}
	// Auto-set casting circle radius from AoE radius
	if (C.bUseCastingCircle && C.CastingCircleRadius == 200.f && C.AoERadius > 0.f)
	{
		C.CastingCircleRadius = C.AoERadius;
	}
	Configs.Add(C.SkillId, C);
}

static TMap<int32, FSkillVFXConfig> BuildSkillVFXConfigs()
{
	TMap<int32, FSkillVFXConfig> Configs;

	// =====================================================================
	//  NOVICE
	// =====================================================================

	// First Aid (2) — Green heal sparkle on self
	{
		FSkillVFXConfig C;
		C.SkillId    = 2;
		C.Template   = ESkillVFXTemplate::HealFlash;
		C.PrimaryColor = FLinearColor(0.2f, 0.9f, 0.3f, 1.f);  // green
		C.Element    = TEXT("neutral");
		C.Scale      = 200.0f;  // NS_Potion baseline is tiny
		C.VFXOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Potion.NS_Potion");
		AddConfig(Configs, C);
	}

	// =====================================================================
	//  SWORDSMAN
	// =====================================================================

	// Bash (103) — Melee slash impact at enemy
	{
		FSkillVFXConfig C;
		C.SkillId    = 103;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 1.f, 0.8f, 1.f);  // warm white
		C.Element    = TEXT("neutral");
		C.Scale      = 1.5f;
		C.VFXOverridePath = TEXT("/Game/Knife_light/VFX/NE_attack01.NE_attack01");
		AddConfig(Configs, C);
	}

	// Provoke (104) — Red anger effect on enemy, persists 3s
	{
		FSkillVFXConfig C;
		C.SkillId    = 104;
		C.Template   = ESkillVFXTemplate::TargetDebuff;
		C.PrimaryColor = FLinearColor(1.f, 0.1f, 0.1f, 1.f);  // red
		C.Element    = TEXT("neutral");
		C.Scale      = 3.0f;
		C.CascadeLifetime = 3.0f;
		C.VFXOverridePath = TEXT("/Game/InfinityBladeEffects/Effects/FX_Archive/P_Enrage_Base.P_Enrage_Base");
		C.bIsCascade = true;
		AddConfig(Configs, C);
	}

	// Magnum Break (105) — Fire AoE burst centered on caster
	{
		FSkillVFXConfig C;
		C.SkillId    = 105;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.3f, 0.0f, 1.f);  // orange/fire
		C.Element    = TEXT("fire");
		C.AoERadius  = 300.f;  // matches server AOE_RADIUS = 300
		C.bSelfCentered = true;
		C.VFXOverridePath = TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Buff.NS_Free_Magic_Buff");
		AddConfig(Configs, C);
	}

	// Endure (106) — Brief golden aura on self
	{
		FSkillVFXConfig C;
		C.SkillId    = 106;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 0.9f, 0.3f, 1.f);  // gold
		C.Element    = TEXT("neutral");
		C.Scale      = 2.0f;
		C.CascadeLifetime = 1.5f;
		C.VFXOverridePath = TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Spider/ICE/P_Ice_Proj_charge_01.P_Ice_Proj_charge_01");
		C.bIsCascade = true;
		AddConfig(Configs, C);
	}

	// =====================================================================
	//  MAGE
	// =====================================================================

	// Cold Bolt (200) — Ice bolts from sky, with casting circle
	{
		FSkillVFXConfig C;
		C.SkillId    = 200;
		C.Template   = ESkillVFXTemplate::BoltFromSky;
		C.PrimaryColor = FLinearColor(0.3f, 0.8f, 1.f, 1.f);  // cyan/ice
		C.Element    = TEXT("water");
		C.bUseCastingCircle = true;
		C.BoltSpawnHeight = 500.f;
		C.CascadeLifetime = 0.5f;  // bolt linger time
		C.VFXOverridePath = TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Elemental/ICE/P_Elemental_Ice_Proj.P_Elemental_Ice_Proj");
		C.bIsCascade = true;
		AddConfig(Configs, C);
	}

	// Fire Bolt (201) — Fire bolts from sky, with casting circle
	{
		FSkillVFXConfig C;
		C.SkillId    = 201;
		C.Template   = ESkillVFXTemplate::BoltFromSky;
		C.PrimaryColor = FLinearColor(1.f, 0.3f, 0.0f, 1.f);  // orange/fire
		C.Element    = TEXT("fire");
		C.bUseCastingCircle = true;
		C.BoltSpawnHeight = 500.f;
		C.Scale      = 3.0f;  // NS_Magma_Shot_Projectile baseline is tiny
		C.VFXOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/Sperate_VFX/NS_Magma_Shot_Projectile.NS_Magma_Shot_Projectile");
		AddConfig(Configs, C);
	}

	// Lightning Bolt (202) — Electric bolts from sky, with casting circle
	{
		FSkillVFXConfig C;
		C.SkillId    = 202;
		C.Template   = ESkillVFXTemplate::BoltFromSky;
		C.PrimaryColor = FLinearColor(1.f, 1.f, 0.5f, 1.f);  // yellow
		C.Element    = TEXT("wind");
		C.bUseCastingCircle = true;
		C.BoltSpawnHeight = 100.f;  // shorter drop for lightning feel
		C.CascadeLifetime = 1.0f;   // linger at impact
		C.VFXOverridePath = TEXT("/Game/Vefects/Zap_VFX/VFX/Zap/Particles/NS_Zap_03_Yellow.NS_Zap_03_Yellow");
		AddConfig(Configs, C);
	}

	// Napalm Beat (203) — Ghost AoE impact on enemy
	{
		FSkillVFXConfig C;
		C.SkillId    = 203;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.6f, 0.2f, 0.8f, 1.f);  // purple/ghost
		C.Element    = TEXT("ghost");
		C.AoERadius  = 300.f;
		C.VFXOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Solo_Impact.NS_Dark_Solo_Impact");
		AddConfig(Configs, C);
	}

	// Sight (205) — Fire aura around caster (reveals hidden)
	{
		FSkillVFXConfig C;
		C.SkillId    = 205;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 0.4f, 0.1f, 1.f);  // fire orange
		C.Element    = TEXT("fire");
		C.Scale      = 1.5f;
		C.CascadeLifetime = 2.0f;
		C.VFXOverridePath = TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Elemental/Fire/P_ElementalFire_Lg.P_ElementalFire_Lg");
		C.bIsCascade = true;
		AddConfig(Configs, C);
	}

	// Stone Curse (206) — Dark mist on target, persists until removed
	{
		FSkillVFXConfig C;
		C.SkillId    = 206;
		C.Template   = ESkillVFXTemplate::TargetDebuff;
		C.PrimaryColor = FLinearColor(0.4f, 0.4f, 0.4f, 1.f);  // grey/stone
		C.Element    = TEXT("earth");
		C.bUseCastingCircle = true;
		C.bLooping   = true;
		C.Scale      = 0.5f;  // enemy-sized
		C.VFXOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Mist.NS_Dark_Mist");
		AddConfig(Configs, C);
	}

	// Fire Ball (207) — Single projectile to primary target, then AoE explosion
	{
		FSkillVFXConfig C;
		C.SkillId    = 207;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(1.f, 0.3f, 0.0f, 1.f);  // fire
		C.Element    = TEXT("fire");
		C.bUseCastingCircle = true;
		C.AoERadius  = 500.f;
		C.ProjectileSpeed = 1500.f;
		C.bSingleProjectile = true;  // only 1 projectile even with AoE multi-damage
		C.VFXOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Magma_Shot.NS_Magma_Shot");
		C.ImpactOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/Sperate_VFX/NS_Magma_Shot_Impact.NS_Magma_Shot_Impact");
		AddConfig(Configs, C);
	}

	// Frost Diver (208) — Ice effect on target, persists for freeze duration
	// Uses Cascade because NS_Ice_Mist uses world-space simulation and ignores component scale.
	{
		FSkillVFXConfig C;
		C.SkillId    = 208;
		C.Template   = ESkillVFXTemplate::TargetDebuff;
		C.PrimaryColor = FLinearColor(0.3f, 0.8f, 1.f, 1.f);  // cyan/ice
		C.Element    = TEXT("water");
		C.bUseCastingCircle = true;
		C.bLooping   = true;
		C.Scale      = 1.5f;
		C.CascadeLifetime = 5.0f;
		C.VFXOverridePath = TEXT("/Game/InfinityBladeEffects/Effects/FX_Monsters/FX_Monster_Spider/ICE/P_Ice_Proj_charge_01.P_Ice_Proj_charge_01");
		C.bIsCascade = true;
		AddConfig(Configs, C);
	}

	// Fire Wall (209) — Persistent looping fire on ground
	{
		FSkillVFXConfig C;
		C.SkillId    = 209;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(1.f, 0.3f, 0.0f, 1.f);  // fire
		C.Element    = TEXT("fire");
		C.bUseCastingCircle = true;
		C.AoERadius  = 150.f;
		C.bLooping   = true;
		C.Scale      = 1.5f;
		C.VFXOverridePath = TEXT("/Game/InfinityBladeEffects/Effects/FX_Ambient/Fire/P_Env_Fire_Grate_01.P_Env_Fire_Grate_01");
		C.bIsCascade = true;
		AddConfig(Configs, C);
	}

	// Soul Strike (210) — Ghost projectiles from player to enemy, one per hit
	{
		FSkillVFXConfig C;
		C.SkillId    = 210;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(0.6f, 0.3f, 0.9f, 1.f);  // purple/ghost
		C.Element    = TEXT("ghost");
		C.ProjectileSpeed = 1500.f;
		C.VFXOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Magic_Bubbles.NS_Magic_Bubbles");
		AddConfig(Configs, C);
	}

	// Safety Wall (211) — Protective circle on ground, looping
	{
		FSkillVFXConfig C;
		C.SkillId    = 211;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(1.f, 0.4f, 0.6f, 1.f);  // pink
		C.Element    = TEXT("neutral");
		C.bUseCastingCircle = true;
		C.AoERadius  = 100.f;
		C.bLooping   = true;
		C.CascadeLifetime = 60.f;
		C.VFXOverridePath = TEXT("/Game/InfinityBladeEffects/Effects/FX_Mobile/Misc/P_levelUp_Detail.P_levelUp_Detail");
		C.bIsCascade = true;
		AddConfig(Configs, C);
	}

	// Thunderstorm (212) — Lightning strikes raining in AoE
	{
		FSkillVFXConfig C;
		C.SkillId    = 212;
		C.Template   = ESkillVFXTemplate::GroundAoERain;
		C.PrimaryColor = FLinearColor(1.f, 1.f, 0.5f, 1.f);  // yellow/wind
		C.Element    = TEXT("wind");
		C.bUseCastingCircle = true;
		C.AoERadius  = 500.f;
		C.VFXOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Lightning_Strike.NS_Lightning_Strike");
		AddConfig(Configs, C);
	}

	// Heal (400) — Green heal sparkle
	{
		FSkillVFXConfig C;
		C.SkillId    = 400;
		C.Template   = ESkillVFXTemplate::HealFlash;
		C.PrimaryColor = FLinearColor(0.2f, 0.9f, 0.3f, 1.f);
		C.Scale      = 250.f;
		C.VFXOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Potion.NS_Potion");
		AddConfig(Configs, C);
	}

	// Blessing (402) — Holy golden buff
	{
		FSkillVFXConfig C;
		C.SkillId    = 402;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 0.85f, 0.3f, 1.f);
		C.Element    = TEXT("holy");
		C.Duration   = 2.f;
		AddConfig(Configs, C);
	}

	// Increase AGI (403) — Blue speed buff
	{
		FSkillVFXConfig C;
		C.SkillId    = 403;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.3f, 0.7f, 1.f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Decrease AGI (404) — Purple debuff
	{
		FSkillVFXConfig C;
		C.SkillId    = 404;
		C.Template   = ESkillVFXTemplate::TargetDebuff;
		C.PrimaryColor = FLinearColor(0.3f, 0.2f, 0.6f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Angelus (406) — Holy white buff
	{
		FSkillVFXConfig C;
		C.SkillId    = 406;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 1.f, 0.9f, 1.f);
		C.Element    = TEXT("holy");
		C.Duration   = 2.f;
		AddConfig(Configs, C);
	}

	// Ruwach (408) — Holy reveal glow
	{
		FSkillVFXConfig C;
		C.SkillId    = 408;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 1.f, 0.8f, 1.f);
		C.Element    = TEXT("holy");
		C.Duration   = 1.f;
		AddConfig(Configs, C);
	}

	// Double Strafe (303) — Two arrow projectiles
	{
		FSkillVFXConfig C;
		C.SkillId    = 303;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(0.8f, 0.6f, 0.3f, 1.f);
		C.ProjectileSpeed = 3000.f;
		C.BoltCount  = 2;
		C.BoltInterval = 0.2f;
		AddConfig(Configs, C);
	}

	// Arrow Shower (304) — Arrow rain AoE (5x5 cells = 125 UE radius)
	{
		FSkillVFXConfig C;
		C.SkillId    = 304;
		C.Template   = ESkillVFXTemplate::GroundAoERain;
		C.PrimaryColor = FLinearColor(0.7f, 0.5f, 0.2f, 1.f);
		C.AoERadius  = 125.f;
		C.bUseCastingCircle = true;
		AddConfig(Configs, C);
	}

	// Arrow Repel (306) — Strong arrow knockback
	{
		FSkillVFXConfig C;
		C.SkillId    = 306;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(0.9f, 0.7f, 0.3f, 1.f);
		C.ProjectileSpeed = 2500.f;
		AddConfig(Configs, C);
	}

	// Envenom (504) — Poison attack impact
	{
		FSkillVFXConfig C;
		C.SkillId    = 504;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.5f, 0.1f, 0.6f, 1.f);
		C.Element    = TEXT("poison");
		AddConfig(Configs, C);
	}

	// Sand Attack (506) — Earth impact
	{
		FSkillVFXConfig C;
		C.SkillId    = 506;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.7f, 0.6f, 0.3f, 1.f);
		C.Element    = TEXT("earth");
		AddConfig(Configs, C);
	}

	// Mammonite (603) — Golden strike
	{
		FSkillVFXConfig C;
		C.SkillId    = 603;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.85f, 0.f, 1.f);
		C.Scale      = 1.5f;
		AddConfig(Configs, C);
	}

	// Cart Revolution (608) — AoE impact
	{
		FSkillVFXConfig C;
		C.SkillId    = 608;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.6f, 0.4f, 0.2f, 1.f);
		C.AoERadius  = 150.f;
		AddConfig(Configs, C);
	}

	// Energy Coat (213) — Blue mana shield
	{
		FSkillVFXConfig C;
		C.SkillId    = 213;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.3f, 0.5f, 0.9f, 1.f);
		C.Duration   = 2.f;
		AddConfig(Configs, C);
	}

	// Loud Exclamation (609) — Orange STR buff
	{
		FSkillVFXConfig C;
		C.SkillId    = 609;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 0.5f, 0.f, 1.f);
		C.Duration   = 1.f;
		AddConfig(Configs, C);
	}

	// Improve Concentration (302) — Yellow focus buff
	{
		FSkillVFXConfig C;
		C.SkillId    = 302;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.9f, 0.9f, 0.5f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// ================================================================
	// SAGE SKILLS (Phase 2C)
	// ================================================================

	// Endow Blaze (1408) — Fire weapon endow
	{
		FSkillVFXConfig C;
		C.SkillId    = 1408;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 0.4f, 0.1f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Endow Tsunami (1409) — Water weapon endow
	{
		FSkillVFXConfig C;
		C.SkillId    = 1409;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.3f, 0.5f, 0.9f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Endow Tornado (1410) — Wind weapon endow
	{
		FSkillVFXConfig C;
		C.SkillId    = 1410;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.6f, 0.9f, 0.4f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Endow Quake (1411) — Earth weapon endow
	{
		FSkillVFXConfig C;
		C.SkillId    = 1411;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.6f, 0.45f, 0.2f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Volcano (1412) — Fire ground zone
	{
		FSkillVFXConfig C;
		C.SkillId    = 1412;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(1.f, 0.35f, 0.1f, 1.f);
		C.AoERadius  = 350.f;
		C.Duration   = 10.f;
		AddConfig(Configs, C);
	}

	// Deluge (1413) — Water ground zone
	{
		FSkillVFXConfig C;
		C.SkillId    = 1413;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(0.3f, 0.5f, 0.9f, 1.f);
		C.AoERadius  = 350.f;
		C.Duration   = 10.f;
		AddConfig(Configs, C);
	}

	// Violent Gale (1414) — Wind ground zone
	{
		FSkillVFXConfig C;
		C.SkillId    = 1414;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(0.5f, 0.8f, 0.4f, 1.f);
		C.AoERadius  = 350.f;
		C.Duration   = 10.f;
		AddConfig(Configs, C);
	}

	// Land Protector (1415) — Neutral nullification zone
	{
		FSkillVFXConfig C;
		C.SkillId    = 1415;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(0.8f, 0.8f, 0.6f, 1.f);
		C.AoERadius  = 350.f;
		C.Duration   = 10.f;
		AddConfig(Configs, C);
	}

	// Dispell (1403) — White dispel flash
	{
		FSkillVFXConfig C;
		C.SkillId    = 1403;
		C.Template   = ESkillVFXTemplate::TargetDebuff;
		C.PrimaryColor = FLinearColor(0.9f, 0.9f, 1.f, 1.f);
		AddConfig(Configs, C);
	}

	// Magic Rod (1404) — Blue shield aura
	{
		FSkillVFXConfig C;
		C.SkillId    = 1404;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.4f, 0.6f, 1.f, 1.f);
		C.Duration   = 0.5f;
		AddConfig(Configs, C);
	}

	// Spell Breaker (1406) — Purple interrupt bolt
	{
		FSkillVFXConfig C;
		C.SkillId    = 1406;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(0.7f, 0.3f, 0.9f, 1.f);
		C.ProjectileSpeed = 2000.f;
		AddConfig(Configs, C);
	}

	// Hindsight (1402) — Orange autocast aura
	{
		FSkillVFXConfig C;
		C.SkillId    = 1402;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 0.7f, 0.3f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// ================================================================
	// WIZARD SKILLS (Phase 2B)
	// ================================================================

	// Jupitel Thunder (800) — Wind bolt projectile
	{
		FSkillVFXConfig C;
		C.SkillId    = 800;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(0.6f, 0.8f, 1.f, 1.f);
		C.ProjectileSpeed = 1200.f;
		AddConfig(Configs, C);
	}

	// Lord of Vermilion (801) — Massive wind AoE
	{
		FSkillVFXConfig C;
		C.SkillId    = 801;
		C.Template   = ESkillVFXTemplate::GroundAoERain;
		C.PrimaryColor = FLinearColor(0.7f, 0.7f, 1.f, 1.f);
		C.AoERadius  = 450.f;
		C.Scale      = 2.5f;
		AddConfig(Configs, C);
	}

	// Meteor Storm (802) — Fire meteor rain
	{
		FSkillVFXConfig C;
		C.SkillId    = 802;
		C.Template   = ESkillVFXTemplate::GroundAoERain;
		C.PrimaryColor = FLinearColor(1.f, 0.4f, 0.1f, 1.f);
		C.AoERadius  = 350.f;
		C.Scale      = 2.f;
		AddConfig(Configs, C);
	}

	// Storm Gust (803) — Water blizzard AoE
	{
		FSkillVFXConfig C;
		C.SkillId    = 803;
		C.Template   = ESkillVFXTemplate::GroundAoERain;
		C.PrimaryColor = FLinearColor(0.5f, 0.7f, 1.f, 1.f);
		C.AoERadius  = 350.f;
		C.Scale      = 2.f;
		AddConfig(Configs, C);
	}

	// Earth Spike (804) — Earth bolt impact
	{
		FSkillVFXConfig C;
		C.SkillId    = 804;
		C.Template   = ESkillVFXTemplate::BoltFromSky;
		C.PrimaryColor = FLinearColor(0.6f, 0.45f, 0.2f, 1.f);
		AddConfig(Configs, C);
	}

	// Heaven's Drive (805) — Earth AoE spikes
	{
		FSkillVFXConfig C;
		C.SkillId    = 805;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.6f, 0.45f, 0.2f, 1.f);
		C.AoERadius  = 250.f;
		AddConfig(Configs, C);
	}

	// Quagmire (806) — Earth debuff zone
	{
		FSkillVFXConfig C;
		C.SkillId    = 806;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(0.3f, 0.25f, 0.1f, 1.f);
		C.AoERadius  = 250.f;
		C.Duration   = 5.f;
		AddConfig(Configs, C);
	}

	// Water Ball (807) — Water bolt multi-hit
	{
		FSkillVFXConfig C;
		C.SkillId    = 807;
		C.Template   = ESkillVFXTemplate::BoltFromSky;
		C.PrimaryColor = FLinearColor(0.3f, 0.5f, 0.9f, 1.f);
		AddConfig(Configs, C);
	}

	// Ice Wall (808) — Ice barrier
	{
		FSkillVFXConfig C;
		C.SkillId    = 808;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(0.6f, 0.85f, 1.f, 1.f);
		C.AoERadius  = 250.f;
		C.Duration   = 10.f;
		AddConfig(Configs, C);
	}

	// Sight Rasher (809) — Fire AoE explosion
	{
		FSkillVFXConfig C;
		C.SkillId    = 809;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.5f, 0.15f, 1.f);
		C.AoERadius  = 350.f;
		C.Scale      = 1.5f;
		AddConfig(Configs, C);
	}

	// Fire Pillar (810) — Fire ground trap
	{
		FSkillVFXConfig C;
		C.SkillId    = 810;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(1.f, 0.3f, 0.1f, 1.f);
		C.AoERadius  = 100.f;
		C.Duration   = 3.f;
		AddConfig(Configs, C);
	}

	// Frost Nova (811) — Water freeze AoE
	{
		FSkillVFXConfig C;
		C.SkillId    = 811;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.5f, 0.7f, 1.f, 1.f);
		C.AoERadius  = 250.f;
		AddConfig(Configs, C);
	}

	// Sight Blaster (813) — Fire reactive buff
	{
		FSkillVFXConfig C;
		C.SkillId    = 813;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 0.6f, 0.2f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Earth Spike Sage (1417) — reuse Earth Spike VFX
	{
		FSkillVFXConfig C;
		C.SkillId    = 1417;
		C.Template   = ESkillVFXTemplate::BoltFromSky;
		C.PrimaryColor = FLinearColor(0.6f, 0.45f, 0.2f, 1.f);
		AddConfig(Configs, C);
	}

	// Heaven's Drive Sage (1418) — reuse Heaven's Drive VFX
	{
		FSkillVFXConfig C;
		C.SkillId    = 1418;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.6f, 0.45f, 0.2f, 1.f);
		C.AoERadius  = 250.f;
		AddConfig(Configs, C);
	}

	// ================================================================
	// CRUSADER SKILLS (Phase 2A)
	// ================================================================

	// Holy Cross (1302) — Holy impact, golden
	{
		FSkillVFXConfig C;
		C.SkillId    = 1302;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.9f, 0.4f, 1.f);
		C.Scale      = 1.2f;
		AddConfig(Configs, C);
	}

	// Grand Cross (1303) — Holy cross AoE, bright gold
	{
		FSkillVFXConfig C;
		C.SkillId    = 1303;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.95f, 0.6f, 1.f);
		C.AoERadius  = 200.f;
		C.Scale      = 2.f;
		AddConfig(Configs, C);
	}

	// Shield Charge (1304) — Silver impact
	{
		FSkillVFXConfig C;
		C.SkillId    = 1304;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.8f, 0.8f, 0.9f, 1.f);
		AddConfig(Configs, C);
	}

	// Shield Boomerang (1305) — Silver projectile
	{
		FSkillVFXConfig C;
		C.SkillId    = 1305;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(0.7f, 0.75f, 0.85f, 1.f);
		C.ProjectileSpeed = 1500.f;
		AddConfig(Configs, C);
	}

	// Auto Guard (1301) — Blue shield buff
	{
		FSkillVFXConfig C;
		C.SkillId    = 1301;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.4f, 0.6f, 0.9f, 1.f);
		C.Duration   = 1.f;
		AddConfig(Configs, C);
	}

	// Reflect Shield (1307) — White shield aura
	{
		FSkillVFXConfig C;
		C.SkillId    = 1307;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.9f, 0.9f, 1.f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Defender (1309) — Blue defensive aura
	{
		FSkillVFXConfig C;
		C.SkillId    = 1309;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.3f, 0.5f, 0.8f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Devotion (1306) — Golden link flash on target
	{
		FSkillVFXConfig C;
		C.SkillId    = 1306;
		C.Template   = ESkillVFXTemplate::HealFlash;
		C.PrimaryColor = FLinearColor(1.f, 0.85f, 0.3f, 1.f);
		AddConfig(Configs, C);
	}

	// Providence (1308) — Holy resistance buff, soft gold
	{
		FSkillVFXConfig C;
		C.SkillId    = 1308;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.9f, 0.85f, 0.5f, 1.f);
		C.Duration   = 1.f;
		AddConfig(Configs, C);
	}

	// Spear Quicken (1310) — Blue speed aura
	{
		FSkillVFXConfig C;
		C.SkillId    = 1310;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.4f, 0.7f, 1.f, 1.f);
		C.Duration   = 1.f;
		AddConfig(Configs, C);
	}

	// Heal Crusader (1311) — Green heal flash
	{
		FSkillVFXConfig C;
		C.SkillId    = 1311;
		C.Template   = ESkillVFXTemplate::HealFlash;
		C.PrimaryColor = FLinearColor(0.3f, 1.f, 0.4f, 1.f);
		AddConfig(Configs, C);
	}

	// Cure Crusader (1312) — White cure flash
	{
		FSkillVFXConfig C;
		C.SkillId    = 1312;
		C.Template   = ESkillVFXTemplate::HealFlash;
		C.PrimaryColor = FLinearColor(0.9f, 0.95f, 1.f, 1.f);
		AddConfig(Configs, C);
	}

	// =====================================================================
	// HUNTER SKILLS (900-917) — Phase 3C
	// =====================================================================

	// Blitz Beat (900) — Gold falcon strike
	{
		FSkillVFXConfig C;
		C.SkillId    = 900;
		C.Template   = ESkillVFXTemplate::BoltFromSky;
		C.PrimaryColor = FLinearColor(1.f, 0.85f, 0.4f, 1.f);
		C.BoltSpawnHeight = 400.f;
		AddConfig(Configs, C);
	}

	// Land Mine (904) — Brown earth explosion
	{
		FSkillVFXConfig C;
		C.SkillId    = 904;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.6f, 0.45f, 0.2f, 1.f);
		AddConfig(Configs, C);
	}

	// Blast Mine (912) — Cyan-white wind explosion
	{
		FSkillVFXConfig C;
		C.SkillId    = 912;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.5f, 0.9f, 1.f, 1.f);
		C.AoERadius  = 75.f;
		AddConfig(Configs, C);
	}

	// Claymore Trap (907) — Orange-red fire explosion
	{
		FSkillVFXConfig C;
		C.SkillId    = 907;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.5f, 0.1f, 1.f);
		C.AoERadius  = 125.f;
		AddConfig(Configs, C);
	}

	// Freezing Trap (911) — Blue-white ice burst
	{
		FSkillVFXConfig C;
		C.SkillId    = 911;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(0.4f, 0.7f, 1.f, 1.f);
		C.AoERadius  = 75.f;
		AddConfig(Configs, C);
	}

	// Phantasmic Arrow (917) — White ghost projectile
	{
		FSkillVFXConfig C;
		C.SkillId    = 917;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(0.9f, 0.9f, 1.f, 1.f);
		C.ProjectileSpeed = 2500.f;
		AddConfig(Configs, C);
	}

	// =====================================================================
	// MONK SKILLS (1600-1615) — Phase 3A
	// =====================================================================

	// Summon Spirit Sphere (1601) — Golden holy sphere aura
	{
		FSkillVFXConfig C;
		C.SkillId    = 1601;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 0.85f, 0.3f, 1.f);
		C.Duration   = 0.8f;
		AddConfig(Configs, C);
	}

	// Investigate (1602) — White melee impact
	{
		FSkillVFXConfig C;
		C.SkillId    = 1602;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 1.f, 0.9f, 1.f);
		AddConfig(Configs, C);
	}

	// Finger Offensive (1604) — Golden ranged projectile
	{
		FSkillVFXConfig C;
		C.SkillId    = 1604;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(1.f, 0.85f, 0.3f, 1.f);
		C.ProjectileSpeed = 2000.f;
		AddConfig(Configs, C);
	}

	// Asura Strike (1605) — Massive white-gold burst
	{
		FSkillVFXConfig C;
		C.SkillId    = 1605;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.95f, 0.7f, 1.f);
		C.AoERadius  = 150.f;
		AddConfig(Configs, C);
	}

	// Critical Explosion / Fury (1611) — Red-gold self buff aura
	{
		FSkillVFXConfig C;
		C.SkillId    = 1611;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(1.f, 0.4f, 0.1f, 1.f);
		C.Duration   = 1.2f;
		AddConfig(Configs, C);
	}

	// Steel Body (1612) — Silver metallic self buff
	{
		FSkillVFXConfig C;
		C.SkillId    = 1612;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.75f, 0.75f, 0.8f, 1.f);
		C.Duration   = 1.5f;
		AddConfig(Configs, C);
	}

	// Ki Explosion (1615) — Orange AoE burst
	{
		FSkillVFXConfig C;
		C.SkillId    = 1615;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.6f, 0.2f, 1.f);
		C.AoERadius  = 150.f;
		AddConfig(Configs, C);
	}

	// Absorb Spirit Sphere (1607) — Blue-white absorb aura
	{
		FSkillVFXConfig C;
		C.SkillId    = 1607;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.5f, 0.7f, 1.f, 1.f);
		C.Duration   = 0.8f;
		AddConfig(Configs, C);
	}

	// Triple Attack (1603) — Yellow-gold melee impact (passive proc)
	{
		FSkillVFXConfig C;
		C.SkillId    = 1603;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.9f, 0.4f, 1.f);
		AddConfig(Configs, C);
	}

	// Chain Combo (1610) — Orange rapid melee impacts
	{
		FSkillVFXConfig C;
		C.SkillId    = 1610;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.65f, 0.15f, 1.f);
		AddConfig(Configs, C);
	}

	// Combo Finish (1613) — Red-orange burst with AoE
	{
		FSkillVFXConfig C;
		C.SkillId    = 1613;
		C.Template   = ESkillVFXTemplate::AoEImpact;
		C.PrimaryColor = FLinearColor(1.f, 0.4f, 0.1f, 1.f);
		C.AoERadius  = 250.f;
		AddConfig(Configs, C);
	}

	// =====================================================================
	//  ALCHEMIST
	// =====================================================================

	// Acid Terror (1801) — Green acid projectile at enemy
	{
		FSkillVFXConfig C;
		C.SkillId    = 1801;
		C.Template   = ESkillVFXTemplate::Projectile;
		C.PrimaryColor = FLinearColor(0.3f, 0.9f, 0.1f, 1.f);  // acid green
		C.Element    = TEXT("neutral");
		C.ProjectileSpeed = 1800.f;
		C.VFXOverridePath = TEXT("/Game/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1.NS_Free_Magic_Projectile1");
		AddConfig(Configs, C);
	}

	// Demonstration (1802) — Persistent fire ground zone
	{
		FSkillVFXConfig C;
		C.SkillId    = 1802;
		C.Template   = ESkillVFXTemplate::GroundPersistent;
		C.PrimaryColor = FLinearColor(1.f, 0.4f, 0.1f, 1.f);  // fire orange
		C.Element    = TEXT("fire");
		C.AoERadius  = 150.f;
		C.bLooping   = true;
		C.bSelfCentered = false;
		AddConfig(Configs, C);
	}

	// Potion Pitcher (1806) — Heal flash on target
	{
		FSkillVFXConfig C;
		C.SkillId    = 1806;
		C.Template   = ESkillVFXTemplate::HealFlash;
		C.PrimaryColor = FLinearColor(0.2f, 0.9f, 0.3f, 1.f);  // green heal
		C.Element    = TEXT("neutral");
		C.Scale      = 200.0f;
		C.VFXOverridePath = TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Potion.NS_Potion");
		AddConfig(Configs, C);
	}

	// Chemical Protection Helm (1808) — Buff aura on self
	{
		FSkillVFXConfig C;
		C.SkillId    = 1808;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.8f, 0.8f, 0.2f, 1.f);  // golden yellow
		C.Element    = TEXT("neutral");
		AddConfig(Configs, C);
	}

	// Chemical Protection Shield (1809)
	{
		FSkillVFXConfig C;
		C.SkillId    = 1809;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.8f, 0.8f, 0.2f, 1.f);
		C.Element    = TEXT("neutral");
		AddConfig(Configs, C);
	}

	// Chemical Protection Armor (1810)
	{
		FSkillVFXConfig C;
		C.SkillId    = 1810;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.8f, 0.8f, 0.2f, 1.f);
		C.Element    = TEXT("neutral");
		AddConfig(Configs, C);
	}

	// Chemical Protection Weapon (1811)
	{
		FSkillVFXConfig C;
		C.SkillId    = 1811;
		C.Template   = ESkillVFXTemplate::SelfBuff;
		C.PrimaryColor = FLinearColor(0.8f, 0.8f, 0.2f, 1.f);
		C.Element    = TEXT("neutral");
		AddConfig(Configs, C);
	}

	return Configs;
}

const FSkillVFXConfig& SkillVFXDataHelper::GetSkillVFXConfig(int32 SkillId)
{
	// Rebuild every call so Live Coding patches pick up changes immediately.
	// This only runs on skill events (not per-frame), so cost is negligible.
	static TMap<int32, FSkillVFXConfig> Configs = BuildSkillVFXConfigs();
	Configs = BuildSkillVFXConfigs();

	static FSkillVFXConfig EmptyConfig;
	if (const FSkillVFXConfig* Found = Configs.Find(SkillId))
	{
		return *Found;
	}
	return EmptyConfig;
}
