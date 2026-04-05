# VFX Asset Reference for Sabri_MMO

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Skill_VFX_Implementation_Plan](Skill_VFX_Implementation_Plan.md) | [Skills_VFX_System_Audit](../06_Audit/Skills_VFX_System_Audit.md)

> Generated 2026-03-05. Comprehensive inventory of all available VFX assets across disk locations,
> mapped to Ragnarok Online skills for the Sabri_MMO project.

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Pack Inventory](#2-pack-inventory)
   - [FreeNiagara: NiagaraExamples](#21-niagaraexamples-epics-official-pack)
   - [FreeNiagara: Free_Magic](#22-free_magic)
   - [FreeNiagara: Mixed_Magic_VFX_Pack](#23-mixed_magic_vfx_pack)
   - [FreeNiagara: Vefects/Zap_VFX](#24-vefectszap_vfx)
   - [FreeNiagara: _SplineVFX](#25-_splinevfx)
   - [FreeNiagara: Knife_light](#26-knife_light)
   - [FreeNiagara: M5VFXVOL2](#27-m5vfxvol2)
   - [FreeNiagara: SmokePack](#28-smokepack)
   - [FreeNiagara: DrapEffet](#29-drapeffet)
   - [FreeNiagara: FreeNiagaraPack](#210-freeniagarapack)
   - [FreeNiagara: A_Surface_Footstep](#211-a_surface_footstep)
   - [FreeNiagara: LightVortexShader](#212-lightvortexshader)
   - [InfinityBlade Effects](#213-infinityblade-effects)
   - [Starter Content / Game Assets](#214-starter-content--game-assets)
3. [Skill to VFX Mapping](#3-skill-to-vfx-mapping)
   - [Currently Implemented Skills](#31-currently-implemented-skills-18)
   - [Future Skills: Archer](#32-archer)
   - [Future Skills: Acolyte](#33-acolyte)
   - [Future Skills: Thief](#34-thief)
   - [Future Skills: Merchant](#35-merchant)
   - [Future Skills: Knight](#36-knight)
   - [Future Skills: Wizard](#37-wizard)
   - [Future Skills: Hunter](#38-hunter)
   - [Future Skills: Priest](#39-priest)
   - [Future Skills: Assassin](#310-assassin)
   - [Future Skills: Blacksmith](#311-blacksmith)
   - [Future Skills: Crusader](#312-crusader)
   - [Future Skills: Monk](#313-monk)
   - [Future Skills: Sage](#314-sage)
   - [Future Skills: Alchemist](#315-alchemist)
4. [Missing VFX / Custom Work Needed](#4-missing-vfx--custom-work-needed)
5. [Migration Status Summary](#5-migration-status-summary)

---

## 1. Executive Summary

| Pack | Location | Total VFX Assets | Type | In SabriMMO? | RO Relevance |
|------|----------|----------------:|------|:------------:|:------------:|
| NiagaraExamples | FreeNiagara | 76 | Niagara (NS_/NE_) | YES (partial) | HIGH -- buffs, debuffs, teleport, impacts, explosions |
| Free_Magic | FreeNiagara | 17 | Niagara (NS_) | YES (full) | CRITICAL -- magic circles, projectiles, area effects |
| Mixed_Magic_VFX_Pack | FreeNiagara | 29 | Niagara (NS_) + 1 Cascade | YES (full) | CRITICAL -- lightning, ice, dark, fire, potions |
| Vefects/Zap_VFX | FreeNiagara | 7 | Niagara (NS_) | YES (full) | HIGH -- colored lightning bolts |
| _SplineVFX | FreeNiagara | 10 | Niagara (NS_) | YES (full) | HIGH -- spline trails: fire, frost, lightning, holy |
| Knife_light | FreeNiagara | 5 | Niagara (NE_) | YES (full) | MEDIUM -- melee slash effects |
| M5VFXVOL2 | FreeNiagara | 113 (NFire_ prefix) | Niagara + Cascade | NO | HIGH -- 100+ fire variations, explosions |
| SmokePack | FreeNiagara | 42 | Niagara (NS_) | NO | LOW -- environment smoke only |
| DrapEffet | FreeNiagara | 7 | Niagara (NE_) | NO | LOW -- water drops |
| FreeNiagaraPack | FreeNiagara | 5 | Niagara (NS_) | NO | MEDIUM -- wormhole, atom, star effects |
| A_Surface_Footstep | FreeNiagara | 50+ (PEN_/PSN_ prefix) | Niagara | NO | LOW -- surface footstep FX |
| LightVortexShader | FreeNiagara | 0 (material only) | Material | NO | MEDIUM -- 59 vortex material instances |
| InfinityBlade Effects | Game Assets | 838 | Cascade (P_) | NO | HIGH -- heal, buff, shield, ice, lightning, aura, combat |
| Starter Content | Game Assets | 375 | Cascade (P_) | NO | LOW -- mostly weapon/environment, some useful |

**Grand Total: ~1,574 VFX assets across all locations**
**Already in SabriMMO: 127 assets**

---

## 2. Pack Inventory

### 2.1 NiagaraExamples (Epic's Official Pack)

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/NiagaraExamples/`
**In SabriMMO**: YES -- `Content/NiagaraExamples/` (partial migration -- FX_Misc, FX_Footstep, FX_PickUp, FX_NDC, FX_SkeletalMesh, FX_Ribbons, FX_Smoke NOT migrated)

#### FX_Explosions (6 systems + 7 emitters)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Dirt_Explosion | NS | YES | AoE/Explosion | Magnum Break (105), Earth Spike (804) |
| NS_Dirt_Explosion_Medium | NS | YES | AoE/Explosion | Magnum Break (105) |
| NS_Dirt_Explosion_Small | NS | YES | AoE/Explosion | Bash impact (103) |
| NS_Explosion | NS | YES | AoE/Explosion | Meteor Storm (802), Fire Ball explosion (207) |
| NS_Explosion_Medium | NS | YES | AoE/Explosion | Fire Ball explosion (207) |
| NS_Explosion_Small | NS | YES | AoE/Explosion | Generic impact |
| NE_Core | NE | YES | Emitter | Sub-emitter for explosions |
| NE_Debris | NE | YES | Emitter | Sub-emitter for explosions |
| NE_DustExplosion | NE | YES | Emitter | Sub-emitter for dirt effects |
| NE_Explosion | NE | YES | Emitter | Sub-emitter for fire effects |
| NE_GroundDust | NE | YES | Emitter | Sub-emitter for ground effects |
| NE_PostProcess | NE | YES | Emitter | Post-process bloom for blasts |
| NE_SparkDebris | NE | YES | Emitter | Sub-emitter for sparks |

#### FX_Player (7 systems)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Player_Buff_Looping | NS | YES | Buff/Aura | Endure (106), Blessing (402), Inc AGI (403), Angelus (406) |
| NS_Player_DeBuff_Looping | NS | YES | Debuff | Provoke (104), Dec AGI (404), Quagmire (806) |
| NS_Player_Electricity_Looping | NS | YES | Buff/Aura | Two-Hand Quicken (705), Adrenaline Rush (1200) |
| NS_Player_Teleport_In | NS | YES | Portal/Teleport | Warp Portal arrival |
| NS_Player_Teleport_Out | NS | YES | Portal/Teleport | Warp Portal departure, Fly Wing |
| NS_Weapon_Buff_Looping | NS | YES | Buff/Aura | Power Thrust (1202), weapon element buffs |
| NS_Weapon_DeBuff_Looping | NS | YES | Debuff | Envenom weapon glow (504) |

#### FX_Sparks (3 systems + 2 emitters)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Spark_Burst | NS | YES | Impact | Generic melee hit spark |
| NS_Spark_Continuous | NS | YES | Impact | Continuous spark (forging) |
| NS_Spark_Impact_Looping | NS | YES | Impact | Looping impact for sustained effects |
| NE_SecondarySparks | NE | YES | Emitter | Sub-emitter |
| NE_Sparks | NE | YES | Emitter | Sub-emitter |

#### FX_Weapons (9 systems + 10 emitters)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Impact_Concrete | NS | YES | Impact | Physical attack hit |
| NS_Impact_Glass | NS | YES | Impact | Crystal/ice shatter |
| NS_Impact_Metal | NS | YES | Impact | Shield Charge hit (1304) |
| NS_Impact_Wood | NS | YES | Impact | Arrow hit |
| NS_BulletTracer | NS | YES | Projectile | Arrow Shower trail (304) |
| NS_RocketTrail | NS | YES | Projectile | Fire Ball trail (207) |
| NS_SimpleRibbonTrail | NS | YES | Trail | Generic ribbon trail |
| NS_MuzzleFlash | NS | YES | Impact | Generic flash |
| NS_SubUV_Utilities | NS | YES | Utility | Material helper |
| NE_Impact + 5 variants | NE | YES | Emitter | Sub-emitters |
| NE_BulletShells + 3 MF variants | NE | YES | Emitter | Sub-emitters |

#### FX_Markers (2 systems)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Marker_Location | NS | YES | UI/Ground | Target location indicator |
| NS_Marker_Target | NS | YES | UI/Ground | Enemy target indicator |

#### FX_Misc (NOT migrated -- 8 systems)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Boundary | NS | NO | Utility | AoE indicator ring |
| NS_Boundary_Box | NS | NO | Utility | AoE indicator box |
| NS_Boundary_Cylinder | NS | NO | Utility | AoE indicator cylinder (Thunderstorm area) |
| NS_Boundary_Sphere | NS | NO | Utility | AoE indicator sphere (Pneuma dome) |
| NS_Bubble_Burst | NS | NO | Impact | Napalm Beat bubbles (203) |
| NS_Fire | NS | NO | Fire | Fire Wall segments (209), Sight orbiting fire (205) |
| NS_FireworkBurst | NS | NO | AoE/Explosion | Grand Cross burst (1303) |
| NS_HitDissolve | NS | NO | Death | Enemy death dissolve |

#### FX_Footstep, FX_PickUp, FX_NDC, FX_SkeletalMesh, FX_Ribbons, FX_Smoke (NOT migrated -- 20 systems)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Footstep_Bubbles/Fire/Gravel/LW | NS | NO | Footstep | Character footstep effects |
| NS_Pickup_Idle/Spawn/Success/Timeout | NS | NO | Pickup | Item drop glow/pickup |
| NS_NDC_Footsteps/Bubbles/Fire/Impacts | NS | NO | NDC | NDC footstep variants |
| NS_TeslaCoil | NS | NO | Lightning | Lightning Bolt (202), Jupitel Thunder (800) |
| NE_Arc | NE | NO | Lightning | Lightning arc sub-emitter |
| NS_Chimney_Smoke | NS | NO | Environment | Environment smoke |
| NS_Smoke_Plume | NS | NO | Environment | Environment smoke |
| NE_Smoke | NE | NO | Emitter | Sub-emitter |
| NS_Dino_Tri_Loop/Tris_Color_Burst | NS | NO | Utility | Death dissolve into triangles |

#### Utilities (NOT migrated)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_TestSprite | NS | YES | Utility | Sprite generation test |
| NS_ExplosionRoil + 4 baked variants | NS | YES | Utility | Pre-baked explosion atlas |
| NS_SmokePuffLight + 4 baked variants | NS | YES | Utility | Pre-baked smoke atlas |

---

### 2.2 Free_Magic

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/Free_Magic/VFX_Niagara/`
**In SabriMMO**: YES -- `Content/Free_Magic/VFX_Niagara/` (full migration)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Free_Magic_Area1 | NS | YES | AoE/Ground | Thunderstorm (212), Lord of Vermilion (801), Magnus Exorcismus (1005) |
| NS_Free_Magic_Area2 | NS | YES | AoE/Ground | Meteor Storm (802), Heaven's Drive (805), Sanctuary (1000) |
| NS_Free_Magic_Attack1 | NS | YES | Impact | Bash impact (103), generic melee skill hit |
| NS_Free_Magic_Attack2 | NS | YES | Impact | Sonic Blow hit (1101) |
| NS_Free_Magic_Attack_Line | NS | YES | Projectile/Line | Grimtooth line (1102) |
| NS_Free_Magic_Aura | NS | YES | Buff/Aura | Endure (106), Angelus (406), Magnificat (1002) |
| NS_Free_Magic_Buff | NS | YES | Buff/Aura | Blessing (402), Inc AGI (403), Power Thrust (1202) |
| NS_Free_Magic_Circle1 | NS | YES | Casting Circle | Casting circle (used for cast bar) |
| NS_Free_Magic_Circle2 | NS | YES | Casting Circle | Alternate casting circle |
| NS_Free_Magic_Hit1 | NS | YES | Impact | Cold Bolt hit (200), Frost Diver hit (208) |
| NS_Free_Magic_Hit2 | NS | YES | Impact | Fire Bolt hit (201), Fire Ball hit (207) |
| NS_Free_Magic_Projectile1 | NS | YES | Projectile | Soul Strike orb (210), Frost Diver spike (208) |
| NS_Free_Magic_Projectile1_Base | NS | YES | Projectile | Base projectile system |
| NS_Free_Magic_Projectile2 | NS | YES | Projectile | Fire Ball (207) |
| NS_Free_Magic_Projectile2_Base | NS | YES | Projectile | Base projectile system |
| NS_Free_Magic_Slash | NS | YES | Melee | Bash (103), Pierce (701) |
| NS_Free_Magic_Slash2 | NS | YES | Melee | Bowling Bash (707), Brandish Spear (703) |

---

### 2.3 Mixed_Magic_VFX_Pack

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/Mixed_Magic_VFX_Pack/VFX/`
**In SabriMMO**: YES -- `Content/Mixed_Magic_VFX_Pack/VFX/` (full migration)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Big_Hammer | NS | YES | Melee/AoE | Mammonite (603), Occult Impaction (1602) |
| NS_Big_Hammer_Impact | NS | YES | Impact | Mammonite hit (603) |
| NS_Big_Sword | NS | YES | Melee | Bash (103), Pierce (701) |
| NS_Crystal_Torrent | NS | YES | AoE/Ice | Storm Gust (803), Frost Diver chain (208) |
| NS_Crystal_Torrent_Owner_Cast | NS | YES | Casting | Ice casting animation |
| NS_Dark_Mist | NS | YES | AoE/Dark | Venom Dust (1105), Quagmire (806) |
| NS_Dark_Owner_Cast_Spell | NS | YES | Casting | Dark spell casting |
| NS_Dark_Solo_Impact | NS | YES | Impact/Dark | Napalm Beat hit (203), Soul Strike hit (210) |
| NS_Dark_Solo_Projectile | NS | YES | Projectile/Dark | Soul Strike orb (210) |
| NS_Dark_Stone | NS | YES | AoE/Earth | Earth Spike (804), Stone Curse ground (206) |
| NS_Dark_Stone_Impact | NS | YES | Impact/Earth | Earth Spike hit (804) |
| NS_Gelmir_Fury | NS | YES | AoE/Fire | Magnum Break (105), Grand Cross (1303) |
| NS_Gelmir_Wizard_Impact | NS | YES | Impact/Fire | Fire Ball explosion (207), Meteor hit (802) |
| NS_Ice_Mist | NS | YES | AoE/Ice | Frost Diver freeze (208), Ice Wall (808) |
| NS_Lightning_Owner_Cast | NS | YES | Casting | Lightning casting circle |
| NS_Lightning_Strike | NS | YES | Lightning | Lightning Bolt (202), Jupitel Thunder (800) |
| NS_Magic_Big_Bubbles_Explosion | NS | YES | AoE/Ghost | Napalm Beat AoE (203) |
| NS_Magic_Big_Bubbles_Owner_Cast | NS | YES | Casting | Ghost casting |
| NS_Magic_Bubbles | NS | YES | Projectile/Ghost | Soul Strike projectile (210) |
| NS_Magic_Bubbles_Owner_Cast_Spell | NS | YES | Casting | Ghost skill casting |
| NS_Magma_Shot | NS | YES | Projectile/Fire | Fire Ball (207), Fire Bolt hit (201) |
| NS_Magma_Shot_Owner_Cast_Spell | NS | YES | Casting | Fire casting |
| NS_Magma_Shot_Impact | NS | YES | Impact/Fire | Fire Ball explosion (207) |
| NS_Magma_Shot_Projectile | NS | YES | Projectile/Fire | Fire Ball projectile (207) |
| NS_Potion | NS | YES | Healing | First Aid (2), Health Potion use |
| NS_Shattering_Crystal | NS | YES | AoE/Ice | Cold Bolt shards (200), Storm Gust (803) |
| NS_Shattering_Crystal_Owner_Cast | NS | YES | Casting | Ice casting |
| P_Big_Sword_Trail | P | YES | Trail/Melee | Bowling Bash trail (707), weapon trail |
| NS_Dark_Solo_Projectile | NS | YES | Projectile | Soul Strike (210) |

---

### 2.4 Vefects/Zap_VFX

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/Vefects/Zap_VFX/VFX/Zap/Particles/`
**In SabriMMO**: YES -- `Content/Vefects/Zap_VFX/VFX/Zap/Particles/` (full migration)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Zap_01_Red | NS | YES | Lightning/Fire | Fire Bolt (201) |
| NS_Zap_02_Blue | NS | YES | Lightning/Ice | Cold Bolt (200), Frost Diver (208) |
| NS_Zap_03_Yellow | NS | YES | Lightning | Lightning Bolt (202), Thunderstorm (212) |
| NS_Zap_04_Green | NS | YES | Lightning/Poison | Envenom (504) |
| NS_Zap_05_Purple | NS | YES | Lightning/Ghost | Soul Strike (210), Napalm Beat (203) |
| NS_Zap_06_White | NS | YES | Lightning/Holy | Holy Cross (1302), Turn Undead (1006) |
| NS_Zap_07_Black | NS | YES | Lightning/Dark | Grimtooth (1102) |

---

### 2.5 _SplineVFX

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/_SplineVFX/NS/`
**In SabriMMO**: YES -- `Content/_SplineVFX/NS/` (full migration)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NS_Spline_ElectricLightning | NS | YES | Spline/Lightning | Jupitel Thunder beam (800), Lightning Bolt trail (202) |
| NS_Spline_EnergyLoop | NS | YES | Spline/Energy | Kyrie Eleison ring (1001) |
| NS_Spline_Fire | NS | YES | Spline/Fire | Fire Wall segments (209), Fire Ball trail (207) |
| NS_Spline_Frost | NS | YES | Spline/Ice | Frost Diver trail (208), Storm Gust rim (803) |
| NS_Spline_Holy | NS | YES | Spline/Holy | Sanctuary border (1000), Magnus Exorcismus (1005) |
| NS_Spline_Magic | NS | YES | Spline/Magic | Casting circle rim, generic magic trail |
| NS_Spline_MiasmaBoil | NS | YES | Spline/Dark | Venom Dust cloud (1105), Quagmire rim (806) |
| NS_Spline_ParticleFollow | NS | YES | Spline/Utility | Particle following spline path |
| NS_Spline_WaterSplash | NS | YES | Spline/Water | Cold Bolt splash trail (200) |
| NS_Spline_Wind | NS | YES | Spline/Wind | Increase AGI wind (403), Arrow Shower wind (304) |

---

### 2.6 Knife_light

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/Knife_light/VFX/`
**In SabriMMO**: YES -- `Content/Knife_light/VFX/` (full migration)

| Asset | Type | In SabriMMO | Category | Potential RO Match |
|-------|------|:-----------:|----------|-------------------|
| NE_attack01 | NE | YES | Melee/Slash | Bash (103), auto-attack |
| NE_attack02 | NE | YES | Melee/Slash | Pierce (701), Sonic Blow hit (1101) |
| NE_attack03 | NE | YES | Melee/Slash | Bowling Bash (707), Brandish Spear (703) |
| NE_attack04 | NE | YES | Melee/Slash | Double Strafe (303), dual swing |
| NE_attack05 | NE | YES | Melee/Slash | Finger Offensive (1604) |

---

### 2.7 M5VFXVOL2

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/M5VFXVOL2/`
**In SabriMMO**: NO
**Note**: Uses `NFire_` prefix (Niagara) and `Fire_` prefix (Cascade), NOT standard `NS_`/`P_` naming.

#### Niagara Systems (NFire_ prefix) -- 62 systems

| Subfolder | Assets | Description | Potential RO Match |
|-----------|-------:|-------------|-------------------|
| Niagara/Fire/ | 19 (NFire_00 to NFire_15 + 3 ground) | Looping fire variants, different sizes | Fire Wall (209), Sight (205) |
| Niagara/Fire_for_BP/ | 20 (NFire_BP_00 to _15 + ground + candle + torch) | Blueprint-ready fires | Fire Wall (209), env torches |
| Niagara/Fire_for_Dir/ | 19 (NFire_Dir_00 to _15 + ground) | Directional fires | Fire Ball trail (207) |
| Niagara/Explosion/ | 3 (NFire_Exp_00, _01, _03) | Fire explosions | Fire Ball explosion (207), Magnum Break (105) |

#### Cascade Particles (Fire_ prefix) -- 51 systems

| Subfolder | Assets | Description | Potential RO Match |
|-----------|-------:|-------------|-------------------|
| Particles/Fire/ | 22 (Fire_00 to _15 + ground + debris) | Legacy fire particles | Same as Niagara equivalents |
| Particles/Fire_for_BP/ | 20 | Blueprint-ready Cascade fires | Same as Niagara equivalents |
| Particles/Fire_for_Dir/ | 19 | Directional Cascade fires | Same as Niagara equivalents |
| Particles/Explosion/ | 3 | Legacy explosions | Same as Niagara equivalents |

---

### 2.8 SmokePack

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/SmokePack/Niagara/`
**In SabriMMO**: NO

| Asset | Type | Category | Potential RO Match |
|-------|------|----------|-------------------|
| NS_EnvironmentSmokes_01 to _29 | NS | Environment/Smoke | Hiding (503) smoke, Cloaking (1103) fade |
| NS_Plume01 to NS_Plume08 | NS | Environment/Plume | Smoke bomb visual, environment dressing |
| NS_Fogcard | NS | Environment/Fog | Fog cards for zones |
| NS_ImpactSmokes_01, _02 | NS | Impact/Smoke | Magnum Break smoke (105) |
| NS_Smoke | NS | Smoke | Generic smoke |
| NS_VehicularSmokes_01, _02 | NS | Vehicle/Smoke | Not relevant |

---

### 2.9 DrapEffet

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/DrapEffet/VFX/`
**In SabriMMO**: NO

| Asset | Type | Category | Potential RO Match |
|-------|------|----------|-------------------|
| NE_drop_effects01 to NE_drop_effects07 | NE | Water/Drops | Aqua Benedicta, Cold Bolt splash (200), environment water |

---

### 2.10 FreeNiagaraPack

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/FreeNiagaraPack/Effects/`
**In SabriMMO**: NO

| Asset | Type | Category | Potential RO Match |
|-------|------|----------|-------------------|
| NS_ActiveAtom | NS | Orbiting/Buff | Sight orbiting particles (205), Ruwach (408) |
| NS_EyeColor | NS | Eye/Aura | Character buff glow |
| NS_GridFigure | NS | Grid/Utility | Targeting grid overlay |
| NS_StarTrack_Medium | NS | Orbiting/Trail | Asura Strike charge (1605) |
| NS_Worm-Hole | NS | Portal | Warp Portal effect, Dispell vortex (1403) |

---

### 2.11 A_Surface_Footstep

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/A_Surface_Footstep/`
**In SabriMMO**: NO
**Note**: Uses `PEN_` (emitter) and `PSN_` (system) prefix naming. 50+ assets.

Key systems by surface type:

| System | Surfaces Covered | Potential RO Match |
|--------|-----------------|-------------------|
| PSN_Dirt_Surface | Dirt footsteps | Character movement FX |
| PSN_Grass_Surface | Grass footsteps | Character movement FX |
| PSN_Ice_Surface | Ice footsteps | Frost Diver ground (208), Storm Gust area (803) |
| PSN_Sand_Surface | Sand footsteps | Desert zones |
| PSN_SnowHeavy/Light_Surface | Snow footsteps | Ice/snow zones |
| PSN_Sparks_Surface | Sparky footsteps | Lightning zone floor |
| PSN_WaterHeavy/Light_Surface | Water footsteps | Water zone floor |
| PSN_SpecialAbility_Surface | Special burst | Skill activation ground burst |

---

### 2.12 LightVortexShader

**Source**: `C:/Unreal Projects/AssetsHolder/FreeNiagara/FreeNiagra/Content/LightVortexShader/`
**In SabriMMO**: NO
**Note**: Material-based vortex effects only (no particle systems). 59 material instances (MI_Light_1 to MI_Light_59) + 1 master material (M_LightVortexShader).

| Asset Type | Count | Potential RO Match |
|-----------|------:|-------------------|
| M_LightVortexShader (master) | 1 | Portal/vortex base material |
| MI_Light_1 to MI_Light_59 | 59 | Warp Portal ground decal, Safety Wall pillar, Sanctuary glow |

---

### 2.13 InfinityBlade Effects

**Source**: `C:/Game assets/InfinityBlade/InfinityBlade/Content/InfinityBladeEffects/Effects/`
**In SabriMMO**: NO (838 total assets)
**Note**: All Cascade (P_ prefix). Would need Niagara conversion for modern pipeline, BUT Cascade still works in UE5.

#### FX_Ability (27 assets) -- HIGHEST relevance

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| **Heal/** | | |
| P_Heal_Aura | Healing/Aura | Heal (400), Sanctuary (1000) |
| P_Heal_LongDistance_Start | Healing/Cast | Heal (400) cast on ally -- caster VFX |
| P_Heal_LongDistance_Target | Healing/Target | Heal (400) -- target VFX |
| P_Heal_Pickup | Healing/Pickup | First Aid (2), potion use |
| P_Heal_Potion | Healing/Item | Potion use |
| P_Heal_Shrine_Start | Healing/Ground | Sanctuary ground (1000) |
| P_Heal_Startup_mobile | Healing/Self | First Aid start (2) |
| P_Heal_Target | Healing/Target | Heal target flash (400) |
| P_HealthPotion_Pickup_01 | Healing/Item | Health Potion pickup |
| P_Health_Pickup_01 | Healing/Item | Item pickup glow |
| P_Health_Player_Buff_Ping_01 | Buff/Ping | Heal proc, regen tick |
| P_TerminusHeal | Healing/Big | Sanctuary full heal (1000) |
| **Buff/** | | |
| P_Buff_Char_SpeedUp_01 | Buff/Speed | Increase AGI (403), Adrenaline Rush (1200) |
| P_Buff_Char_TerraDrain_01 | Buff/Drain | Occult Impaction drain (1602) |
| **Defense/** | | |
| P_Defender_Activate | Shield/Activation | Kyrie Eleison activate (1001) |
| P_EnergyBlast | Blast/Defense | Asura Strike (1605) |
| P_EnergyBlast_Impact | Impact/Defense | Asura Strike impact (1605) |
| P_Shield_Spawn | Shield/Spawn | Kyrie Eleison shield appear (1001) |
| P_Shield_Sphere_Buff | Shield/Buff | Kyrie Eleison sphere (1001), Safety Wall (211) |
| **Armor/** | | |
| P_Reduced_Melee_Shield_01 | Shield/Buff | Angelus (406) armor boost |
| **Stun/** | | |
| P_Stun_Stars_Base | Status/Stun | Bash stun (103), Stone Curse (206) |
| **Summon/** | | |
| P_Summon_Child/Parent/Portal etc. (6) | Summon | Not directly RO-relevant (no summons yet) |

#### FX_Combat_Base (36 assets) -- HIGH relevance

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| **Impact/** | | |
| P_ImpactSpark | Impact/Generic | Auto-attack hit |
| P_Impact_Enemy_Fire_Strong | Impact/Fire | Fire Bolt hit (201), Fire Ball hit (207) |
| P_Impact_Enemy_IB | Impact/IB | Generic strong hit |
| P_Impact_Enemy_Ice_Strong | Impact/Ice | Cold Bolt hit (200), Frost Diver hit (208) |
| P_Impact_Enemy_Strong | Impact/Generic | Bash hit (103) |
| P_Impact_Player_Fire_Strong | Impact/Fire | Enemy fire attack on player |
| P_Impact_Player_Ice_Strong | Impact/Ice | Enemy ice attack on player |
| P_Impact_Player_Strong | Impact/Generic | Enemy strong hit on player |
| **Resurrection/** | | |
| P_Resurrection | Resurrection | Resurrection (Priest) |
| P_Resurrection_02 | Resurrection | Alternate resurrection |
| **WeaponCombo/** | | |
| P_ComboFinish_Tap_01/02 | Combo/Finish | Bowling Bash final hit (707) |
| P_Combo_Finish_Fire_01 | Combo/Fire | Magnum Break finale (105) |
| P_Combo_Finish_Ice_01 | Combo/Ice | Storm Gust finale (803) |
| P_Combo_Finish_Untyped_01 | Combo/Generic | Sonic Blow finale (1101) |
| P_IB_OneShotAOE | AoE | Asura Strike AoE (1605) |
| P_OneShotChargeUp | Charge | Asura Strike charge (1605) |
| **WeaponShimmer/** | | |
| P_WeaponShimmer_Fire_Blade | Weapon/Fire | Fire weapon enchant |
| P_WeaponShimmer_Fire_Blunt | Weapon/Fire | Fire weapon enchant (blunt) |
| P_WeaponShimmer_Ice_Blunt | Weapon/Ice | Frost weapon enchant |
| P_WeaponShimmer_Poison_Blunt | Weapon/Poison | Envenom weapon (504) |
| P_Weaponshimmer_Lightning_blunt_01 | Weapon/Lightning | Lightning weapon enchant |
| **death/** | | |
| P_Death_BlastMark | Death | Enemy death ground mark |
| P_Impact_Gib_Fire/Ice/Poison + LG variants | Death/Gib | Element death effects |

#### FX_Combat_Ice (4 assets)

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_Ice_Freeze_Motion | Ice/Freeze | Frost Diver freeze (208) |
| P_Impact_Ice | Ice/Impact | Cold Bolt hit (200) |
| P_Player_Frozen_Exit_01 | Ice/Thaw | Frost Diver thaw (208) |
| P_IceTrapFrozen | Ice/Trap | Ice Wall (808), Frost Diver ground (208) |

#### FX_Combat_Lightning (2 assets)

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_DOT_Lightning_01 | Lightning/DoT | Jupitel Thunder (800), Lord of Vermilion (801) |
| P_LineToPoint_Proj_Lightning_00 | Lightning/Projectile | Jupitel Thunder beam (800) |

#### FX_Skill_Aura (22 assets) -- HIGH relevance

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_AuraCircle_Default_Base_01 | Aura/Ground | Casting circle, Sanctuary base (1000) |
| P_AuraCircle_Default_Expansion_01 | Aura/Expand | Magnum Break ring expand (105) |
| P_AuraCircle_Default_IronSkin_01 | Aura/Defense | Endure (106), Angelus (406) |
| P_AuraCircle_Default_StartUp_01 | Aura/Start | Generic aura startup |
| P_AuraCircle_Ice_Base/StartUp/Vortex_01 | Aura/Ice | Storm Gust ground (803) |
| P_AuraCircle_Poison_Base/StartUp/Vortex_01 | Aura/Poison | Venom Dust area (1105), Quagmire (806) |
| P_Aura_Default_Ending | Aura/End | Buff expire |
| P_Aura_Default_Upheaval_01 | Aura/Burst | Grand Cross burst (1303), Magnus Exorcismus (1005) |
| P_Aura_Default_Upheaval_Charge_01 | Aura/Charge | Grand Cross charge (1303) |
| P_Aura_Ice_Ending/Shatter/Upheaval/Charge_01 | Aura/Ice | Ice skill variants |
| P_Aura_Poison_Ending/Shatter/Upheaval/Charge_01 | Aura/Poison | Poison skill variants |

#### FX_Skill_Leap (18 assets)

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_Skill_Leap_Base_Impact/Launch/Trail etc. | Leap/Generic | Bowling Bash leap (707) |
| P_Skill_Leap_Fire_* (7 variants) | Leap/Fire | Magnum Break leap component |
| P_Skill_Leap_Poison_* (5 variants) | Leap/Poison | Envenom leap attack |

#### FX_Skill_RockBurst (24 assets) -- HIGH relevance for earth skills

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_RBurst_Default_Burst_Area/Stalag/SinkH_01 | AoE/Earth | Earth Spike (804), Heaven's Drive (805) |
| P_RBurst_Default_Proj_01 | Projectile/Earth | Earth Spike projectile (804) |
| P_RBurst_Fire_Aoe/Burst/Eruption/Proj_01 | AoE/Fire | Meteor Storm (802), Fire Pillar |
| P_RBurst_Lightning_Burst/Eruption/Proj/Pull_01 | AoE/Lightning | Lord of Vermilion (801), Thunderstorm (212) |
| P_Health_Absorb_01 | Drain | Occult Impaction (1602), Soul Drain |

#### FX_Skill_TeleCharge (23 assets)

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_Skill_Telecharge_Base_Proj/Impact_01 | Projectile/Generic | Generic ranged attack projectile |
| P_Skill_Telecharge_Ice_Proj/Impact_01/02/03 | Projectile/Ice | Cold Bolt shards (200), Frost Diver (208) |
| P_Skill_Telecharge_Shock_Proj/Impact_01/02/03 | Projectile/Lightning | Lightning Bolt (202), Jupitel Thunder (800) |
| P_Skill_telecharge_Base/Ice/Shock_Charge_01 | Charge | Cast bar charge-up |

#### FX_Skill_WeaponThrow (17 assets)

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_Skill_Throw_Base_* | Throw/Generic | Shield Charge (1304) |
| P_Skill_Throw_Fire_* | Throw/Fire | Demonstration (1802) |
| P_Skill_Throw_Ice_* | Throw/Ice | Cold Bolt throw variant (200) |

#### FX_Skill_Whirlwind (16 assets)

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_Whirlwind_Default_Area/Start/WeaponTrail_01 | Whirlwind/Generic | Bowling Bash spin (707), Brandish Spear (703) |
| P_Whirlwind_Fire | Whirlwind/Fire | Grand Cross (1303) fire variant |
| P_Whirlwind_Lightning_* (5 variants) | Whirlwind/Lightning | Lord of Vermilion (801) |
| P_Whirlwind_Poison_* (5 variants) | Whirlwind/Poison | Venom Dust spread (1105) |

#### FX_Monsters (218 assets)

Monster-specific effects. Key usable ones:

| Asset Pattern | Category | Potential RO Match |
|--------------|----------|-------------------|
| P_Golem_* (various) | Monster/Earth | Earth element monster attacks |
| P_OceanBoss_* (various) | Monster/Water | Water element effects |
| P_TreantBoss_* (various) | Monster/Nature | Poison/nature effects |
| P_DarkKnight_* (various) | Monster/Dark | Dark/shadow attacks |
| P_IceKnight_* (various) | Monster/Ice | Ice element attacks |

#### FX_Ambient (95 assets)

Environment effects. Key usable ones:

| Asset Pattern | Category | Potential RO Match |
|--------------|----------|-------------------|
| P_Fire_Torch_01/Blue/Red | Fire/Torch | Fire Wall segment (209), zone decoration |
| P_Fire_GroundFire_Blue_mobile | Fire/Ground | Blue ground fire |
| P_SmokePuff | Smoke | Hiding appear/disappear (503), Cloaking (1103) |
| P_Mist_mobile/purple_mobile | Fog | Dark zone atmosphere, Quagmire overlay (806) |
| P_SnowFall_01 | Snow | Ice zone environment |
| P_HealthShrine_lg | Shrine/Heal | Sanctuary pillar (1000) |

#### FX_Treasure (19 assets)

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_Treasure_Chest_Open/Close etc. | Treasure | Chest open VFX |
| P_Treasure_Spawn/Glow | Spawn | Item spawn glow |

#### Remaining categories

| Category | Assets | Relevance |
|----------|-------:|-----------|
| FX_Mobile | 266 | LOW -- mobile-optimized duplicates |
| FX_Cines | 17 | LOW -- cinematic effects |
| FX_Forging | 7 | MEDIUM -- crafting VFX for Blacksmith |
| FX_Gates | 6 | LOW -- gate mechanics |
| FX_Breakables | 8 | LOW -- destructible objects |
| FX_Archive | 12 | LOW -- legacy/archived |
| FX_Meshes | 1 | LOW -- mesh reference |

---

### 2.14 Starter Content / Game Assets

**Source**: `C:/Game assets/Content/Content/`
**In SabriMMO**: NO (375 total assets)
**Note**: Mostly FPS weapon effects (Unreal Tournament heritage). Limited RO skill relevance.

#### StarterContent/Particles (6 assets)

| Asset | Type | Category | Potential RO Match |
|-------|------|----------|-------------------|
| P_Ambient_Dust | P | Environment | Zone atmosphere |
| P_Explosion | P | Explosion | Generic explosion |
| P_Fire | P | Fire | Fire Wall (209) |
| P_Smoke | P | Smoke | Generic smoke |
| P_Sparks | P | Sparks | Impact sparks |
| P_Steam_Lit | P | Steam | Environment steam |

#### Potentially Useful from RestrictedAssets

| Asset | Category | Potential RO Match |
|-------|----------|-------------------|
| P_Player_Spawn | Spawn | Character login spawn VFX |
| P_Player_Teleport | Teleport | Warp Portal (Fly Wing) teleport VFX |
| P_WarpStart/End/Flash/Streak | Warp | Warp Portal effects |
| P_Elec_Arc_OffBot | Lightning | Lightning Bolt (202), Jupitel Thunder (800) |
| P_LightningRifle_projectile | Lightning/Proj | Lightning projectile |
| P_Lightning_Beam_03_1P | Lightning/Beam | Jupitel Thunder beam (800) |
| P_Lightning_Trail/Trail2 | Lightning/Trail | Lightning skill trails |
| P_Blizzard | Snow/Blizzard | Storm Gust (803) |
| P_Blood_explosion_01/02 | Blood | Critical hit, monster death |
| P_WP_ShockRifle_Ball/Explo | Energy/Ball | Soul Strike orb (210) |
| P_BrazierFire_01/02 | Fire/Torch | Fire Wall segment (209) |

---

## 3. Skill to VFX Mapping

### 3.1 Currently Implemented Skills (18)

| ID | Skill Name | VFX Template | Best Primary VFX | Best Secondary VFX | Status |
|----|-----------|-------------|------------------|-------------------|--------|
| 2 | First Aid | HealFlash | NS_Potion (Mixed Magic) | P_Heal_Pickup (IB) | IN SABRIMMO |
| 103 | Bash | AoEImpact | NS_Free_Magic_Attack1 (Free Magic) | NE_attack01 (Knife_light) | IN SABRIMMO |
| 104 | Provoke | TargetDebuff | NS_Player_DeBuff_Looping (NiagaraEx) | NS_Zap_01_Red (Zap) | IN SABRIMMO |
| 105 | Magnum Break | AoEImpact | NS_Gelmir_Fury (Mixed Magic) | NS_Explosion (NiagaraEx) | IN SABRIMMO |
| 106 | Endure | SelfBuff | NS_Player_Buff_Looping (NiagaraEx) | NS_Free_Magic_Aura (Free Magic) | IN SABRIMMO |
| 200 | Cold Bolt | BoltFromSky | NS_Shattering_Crystal (Mixed Magic) | NS_Zap_02_Blue (Zap) | IN SABRIMMO |
| 201 | Fire Bolt | BoltFromSky | NS_Magma_Shot (Mixed Magic) | NS_Zap_01_Red (Zap) | IN SABRIMMO |
| 202 | Lightning Bolt | BoltFromSky | NS_Lightning_Strike (Mixed Magic) | NS_Zap_03_Yellow (Zap) | IN SABRIMMO |
| 203 | Napalm Beat | AoEImpact | NS_Magic_Big_Bubbles_Explosion (Mixed) | NS_Dark_Solo_Impact (Mixed) | IN SABRIMMO |
| 205 | Sight | SelfBuff | NS_Free_Magic_Aura (Free Magic) | NS_Fire (NiagaraEx, NOT migrated) | IN SABRIMMO |
| 206 | Stone Curse | TargetDebuff | NS_Dark_Stone (Mixed Magic) | NS_Dark_Stone_Impact (Mixed) | IN SABRIMMO |
| 207 | Fire Ball | Projectile | NS_Magma_Shot_Projectile (Mixed) | NS_Gelmir_Wizard_Impact (Mixed) | IN SABRIMMO |
| 208 | Frost Diver | Projectile | NS_Crystal_Torrent (Mixed Magic) | NS_Ice_Mist (Mixed Magic) | IN SABRIMMO |
| 209 | Fire Wall | GroundPersistent | NS_Spline_Fire (_SplineVFX) | NFire_00-15 (M5VFX, NOT migrated) | IN SABRIMMO |
| 210 | Soul Strike | Projectile | NS_Dark_Solo_Projectile (Mixed Magic) | NS_Magic_Bubbles (Mixed Magic) | IN SABRIMMO |
| 211 | Safety Wall | GroundPersistent | NS_Free_Magic_Circle1 (Free Magic) | P_Shield_Sphere_Buff (IB, NOT migrated) | IN SABRIMMO |
| 212 | Thunderstorm | GroundAoERain | NS_Lightning_Strike (Mixed Magic) | NS_Zap_03_Yellow (Zap) | IN SABRIMMO |
| -- | Warp Portal | WarpPortal | NS_Player_Teleport_In/Out (NiagaraEx) | NS_Worm-Hole (FreeNiagara, NOT migrated) | IN SABRIMMO |
| -- | Casting Circle | CastingCircle | NS_Free_Magic_Circle1/2 (Free Magic) | NS_Lightning_Owner_Cast (Mixed) | IN SABRIMMO |

### 3.2 Archer

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 303 | Double Strafe | NS_Free_Magic_Projectile1 (Free Magic) | P_Skill_Telecharge_Base_Proj_01 (IB) | Partial -- need arrow mesh |
| 304 | Arrow Shower | NS_Free_Magic_Area1 (Free Magic) | P_RBurst_Default_Burst_Area_01 (IB) | YES -- rain of arrows AoE |

### 3.3 Acolyte

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 400 | Heal | NS_Potion (Mixed Magic) | P_Heal_Target + P_Heal_LongDistance_Target (IB) | Partial -- green upward sparkle |
| 402 | Blessing | NS_Player_Buff_Looping (NiagaraEx) | P_Buff_Char_SpeedUp_01 (IB) | NO |
| 403 | Increase AGI | NS_Player_Buff_Looping (NiagaraEx) | P_Buff_Char_SpeedUp_01 (IB) | Partial -- wind swirl |
| 404 | Decrease AGI | NS_Player_DeBuff_Looping (NiagaraEx) | P_AuraCircle_Poison_Vortex_01 (IB) | NO |
| 406 | Angelus | NS_Free_Magic_Aura (Free Magic) | P_Shield_Sphere_Buff + P_Reduced_Melee_Shield_01 (IB) | NO |
| 408 | Ruwach | NS_Free_Magic_Area1 (Free Magic) | P_AuraCircle_Default_Expansion_01 (IB) | Partial -- blue detection ring |
| 411 | Pneuma | NS_Free_Magic_Circle1 (Free Magic) | P_Shield_Sphere_Buff (IB) | Partial -- transparent dome |

### 3.4 Thief

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 504 | Envenom | NS_Zap_04_Green (Zap) | P_WeaponShimmer_Poison_Blunt (IB) | NO |
| 503 | Hiding | NS_Player_Teleport_Out (NiagaraEx) | NS_EnvironmentSmokes_01 (SmokePack) | Partial -- fade out + smoke |

### 3.5 Merchant

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 603 | Mammonite | NS_Big_Hammer + NS_Big_Hammer_Impact (Mixed) | P_Impact_Enemy_Strong (IB) | NO -- gold coin shower customization |

### 3.6 Knight

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 701 | Pierce | NS_Free_Magic_Slash (Free Magic) | P_Impact_Enemy_Strong (IB) | Partial -- thrust line effect |
| 703 | Brandish Spear | NS_Free_Magic_Slash2 (Free Magic) | P_Whirlwind_Default_Area_01 (IB) | Partial -- wide arc slash |
| 707 | Bowling Bash | P_Big_Sword_Trail (Mixed Magic) | P_ComboFinish_Tap_01 + P_Skill_Leap_Base_Impact (IB) | Partial -- knockback wave |
| 705 | Two-Hand Quicken | NS_Player_Electricity_Looping (NiagaraEx) | P_Buff_Char_SpeedUp_01 (IB) | NO |

### 3.7 Wizard

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 800 | Jupitel Thunder | NS_Lightning_Strike + NS_Spline_ElectricLightning | P_DOT_Lightning_01 + P_LineToPoint_Proj_Lightning_00 (IB) | Partial -- multi-hit beam |
| 801 | Lord of Vermilion | NS_Free_Magic_Area1 (Free Magic) | P_RBurst_Lightning_Eruption_01 + P_Whirlwind_Lightning_Typhoon_01 (IB) | YES -- massive lightning AoE |
| 802 | Meteor Storm | NS_Gelmir_Wizard_Impact (Mixed) | P_RBurst_Fire_Eruption_01 (IB) | YES -- falling meteors |
| 803 | Storm Gust | NS_Crystal_Torrent + NS_Ice_Mist (Mixed) | P_AuraCircle_Ice_Vortex_01 + P_Blizzard (IB/Starter) | Partial -- ice tornado |
| 804 | Earth Spike | NS_Dark_Stone + NS_Dark_Stone_Impact (Mixed) | P_RBurst_Default_Burst_Stalag_01 (IB) | NO |
| 805 | Heaven's Drive | NS_Free_Magic_Area2 (Free Magic) | P_RBurst_Default_Burst_Area_01 (IB) | Partial -- ground eruption wave |
| 806 | Quagmire | NS_Dark_Mist (Mixed Magic) | P_AuraCircle_Poison_Base_01 (IB) | Partial -- slow swamp circle |
| 808 | Ice Wall | NS_Ice_Mist (Mixed Magic) | P_IceTrapFrozen (IB) | YES -- ice block wall segments |

### 3.8 Hunter

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 900 | Blitz Beat | NS_Free_Magic_Attack2 (Free Magic) | P_Skill_Throw_Base_Proj_Hammer (IB) | YES -- falcon dive attack |
| 903 | Ankle Snare | NS_Spline_EnergyLoop (_SplineVFX) | P_IceTrapFrozen (IB, retextured) | Partial -- trap on ground |
| 904 | Land Mine | NS_Explosion_Small (NiagaraEx) | P_RBurst_Fire_Aoe_01 (IB) | Partial -- mine trigger explosion |
| 907 | Claymore Trap | NS_Explosion (NiagaraEx) | P_RBurst_Fire_Eruption_01 (IB) | Partial -- large mine explosion |

### 3.9 Priest

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 1000 | Sanctuary | NS_Free_Magic_Circle1 + NS_Spline_Holy | P_Heal_Aura + P_Heal_Shrine_Start + P_HealthShrine_lg (IB) | Partial -- healing ground circle |
| 1001 | Kyrie Eleison | NS_Spline_EnergyLoop (_SplineVFX) | P_Shield_Sphere_Buff + P_Defender_Activate + P_Shield_Spawn (IB) | Partial -- translucent barrier sphere |
| 1002 | Magnificat | NS_Free_Magic_Aura (Free Magic) | P_AuraCircle_Default_Base_01 (IB) | NO |
| 1005 | Magnus Exorcismus | NS_Free_Magic_Area1 + NS_Spline_Holy | P_Aura_Default_Upheaval_01 + P_AuraCircle_Default_Expansion_01 (IB) | YES -- cross-shaped holy AoE |
| 1006 | Turn Undead | NS_Zap_06_White (Zap) | P_EnergyBlast (IB) | Partial -- holy blast toward target |

### 3.10 Assassin

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 1101 | Sonic Blow | NE_attack02 + NE_attack03 (Knife_light) | P_Combo_Finish_Untyped_01 + P_Whirlwind_Default_WeaponTrail_01 (IB) | Partial -- rapid 8-hit slash |
| 1102 | Grimtooth | NS_Free_Magic_Attack_Line (Free Magic) | NS_Zap_07_Black (Zap) | Partial -- ground-travel dark wave |
| 1103 | Cloaking | NS_Player_Teleport_Out (NiagaraEx) | NS_EnvironmentSmokes_01 (SmokePack) + P_SmokePuff (IB) | Partial -- gradual fade + shadow |
| 1105 | Venom Dust | NS_Dark_Mist (Mixed Magic) | P_AuraCircle_Poison_Base/Vortex_01 (IB) | Partial -- poison cloud AoE |

### 3.11 Blacksmith

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 1200 | Adrenaline Rush | NS_Player_Electricity_Looping (NiagaraEx) | P_Buff_Char_SpeedUp_01 (IB) | NO |
| 1202 | Power Thrust | NS_Weapon_Buff_Looping (NiagaraEx) | P_WeaponShimmer_Fire_Blade (IB) | NO |

### 3.12 Crusader

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 1302 | Holy Cross | NS_Zap_06_White (Zap) + NS_Free_Magic_Slash (Free Magic) | P_Combo_Finish_Fire_01 (IB, retint white) | YES -- cross-shaped holy slash |
| 1303 | Grand Cross | NS_Gelmir_Fury (Mixed Magic) | P_Whirlwind_Fire + P_Aura_Default_Upheaval_01 (IB) | YES -- massive cross-shaped holy AoE |
| 1304 | Shield Charge | NS_Free_Magic_Attack1 (Free Magic) | P_Skill_Leap_Base_Impact (IB) | Partial -- shield bash forward |

### 3.13 Monk

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 1602 | Occult Impaction | NS_Big_Hammer + NS_Big_Hammer_Impact (Mixed) | P_Health_Absorb_01 (IB) | Partial -- spirit fist strike |
| 1604 | Finger Offensive | NE_attack05 (Knife_light) | P_Skill_Telecharge_Base_Proj_01 (IB) | YES -- rapid spirit ball projectiles |
| 1605 | Asura Strike | NS_Gelmir_Fury (Mixed Magic) | P_EnergyBlast + P_EnergyBlast_Impact + P_OneShotChargeUp + P_IB_OneShotAOE (IB) | YES -- massive single-hit energy explosion |

### 3.14 Sage

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 1403 | Dispell | NS_Player_DeBuff_Looping (NiagaraEx) | NS_Worm-Hole (FreeNiagara) + P_Aura_Default_Ending (IB) | Partial -- buff strip vortex |

### 3.15 Alchemist

| ID | Skill | Best Primary VFX (In SabriMMO) | Best Unmigrated VFX | Custom Needed? |
|----|-------|-------------------------------|---------------------|:--------------:|
| 1801 | Acid Terror | NS_Dark_Solo_Projectile (Mixed Magic, retint green) | P_Skill_Telecharge_Ice_Proj_01 (IB, retint) | YES -- acid bottle throw + splash |
| 1802 | Demonstration | NS_Explosion (NiagaraEx) | P_Skill_Throw_Fire_Field + P_RBurst_Fire_Aoe_01 (IB) | Partial -- bottle throw + fire AoE |

---

## 4. Missing VFX / Custom Work Needed

### Critical Custom VFX (no good match available)

| Priority | Skill | What's Missing | Suggested Approach |
|:--------:|-------|---------------|-------------------|
| HIGH | Meteor Storm (802) | Falling meteor chunks from sky | Custom NS: rocks falling + fire trail + impact explosions. Use NS_Gelmir_Wizard_Impact for impacts. |
| HIGH | Lord of Vermilion (801) | Massive sustained lightning AoE | Custom NS: combine NS_Lightning_Strike rain pattern with P_RBurst_Lightning variants. |
| HIGH | Grand Cross (1303) | Cross-shaped holy ground AoE | Custom NS: cross mesh emitter + holy particles from NS_Spline_Holy + NS_Zap_06_White. |
| HIGH | Magnus Exorcismus (1005) | Cross-shaped holy persistent AoE | Custom NS: large cross pattern on ground, holy light pillars. Adapt from Grand Cross. |
| HIGH | Asura Strike (1605) | Massive single-hit energy explosion | Custom NS: charge-up from P_OneShotChargeUp + massive burst. Combine NS_Gelmir_Fury + NS_Explosion. |
| MEDIUM | Holy Cross (1302) | Cross-shaped single-target slash | Custom NS: cross slash trail using NE_attack variants + white/gold NS_Zap. |
| MEDIUM | Finger Offensive (1604) | Rapid spirit ball projectile chain | Custom NS: multiple small orbs rapid-fire. Adapt NS_Free_Magic_Projectile1. |
| MEDIUM | Ice Wall (808) | Row of ice block segments | Custom NS: ice pillar mesh spawning in a line. Use NS_Ice_Mist for ambient. |
| MEDIUM | Blitz Beat (900) | Falcon dive-attack | Custom NS: bird silhouette diving + impact. No bird mesh available. |
| MEDIUM | Acid Terror (1801) | Acid bottle throw + corrosion | Custom NS: bottle projectile + green splash. Retint NS_Dark_Solo_Projectile. |
| LOW | Arrow Shower (304) | Rain of arrows in AoE | Custom NS: arrow mesh rain in circle. Need arrow static mesh. |

### VFX Needing Customization/Retinting Only

| Skill | Base Asset | Modification Needed |
|-------|-----------|-------------------|
| Storm Gust (803) | NS_Crystal_Torrent + NS_Ice_Mist | Scale up + add vortex motion |
| Earth Spike (804) | NS_Dark_Stone | Retint brown/earth, add upward spike motion |
| Quagmire (806) | NS_Dark_Mist | Retint murky green/brown, flatten to ground level |
| Hiding (503) | NS_Player_Teleport_Out | Slow down, add smoke puff |
| Cloaking (1103) | NS_Player_Teleport_Out | Gradual fade, dark mist |
| Venom Dust (1105) | NS_Dark_Mist | Retint green/purple, add poison bubbles |
| Ankle Snare (903) | NS_Spline_EnergyLoop | Scale down, add ground chain/net pattern |
| Sanctuary (1000) | NS_Free_Magic_Circle1 + NS_Spline_Holy | Combine for healing ground circle + sparkle |
| Kyrie Eleison (1001) | NS_Spline_EnergyLoop | Create translucent sphere shell |

---

## 5. Migration Status Summary

### Already Migrated to SabriMMO (6 packs, 127 assets)

| Pack | Path in SabriMMO | Asset Count |
|------|-----------------|------------:|
| Free_Magic | Content/Free_Magic/ | 17 |
| Mixed_Magic_VFX_Pack | Content/Mixed_Magic_VFX_Pack/ | 29 |
| NiagaraExamples (partial) | Content/NiagaraExamples/ | 62 |
| Vefects/Zap_VFX | Content/Vefects/ | 7 |
| _SplineVFX | Content/_SplineVFX/ | 10 |
| Knife_light | Content/Knife_light/ | 5 |

### Not Yet Migrated -- Recommended for Migration

| Priority | Pack | Why | Asset Count |
|:--------:|------|-----|------------:|
| HIGH | NiagaraExamples/FX_Misc | NS_Fire, NS_TeslaCoil, NS_Boundary, NS_HitDissolve | 8+2 |
| HIGH | FreeNiagaraPack | NS_Worm-Hole for portals, NS_ActiveAtom for orbiting | 5 |
| HIGH | InfinityBlade FX_Ability | P_Heal_*, P_Shield_*, P_Stun_* -- critical for Acolyte/Priest | 27 |
| HIGH | InfinityBlade FX_Skill_Aura | P_AuraCircle_* -- critical for buff/debuff ground circles | 22 |
| MEDIUM | InfinityBlade FX_Combat_Base | P_Impact_*, P_Resurrection -- good impact VFX | 36 |
| MEDIUM | InfinityBlade FX_Combat_Ice | P_Ice_Freeze_Motion -- needed for Frost Diver | 4 |
| MEDIUM | InfinityBlade FX_Combat_Lightning | P_DOT_Lightning_01 -- Jupitel Thunder | 2 |
| MEDIUM | InfinityBlade FX_Skill_RockBurst | P_RBurst_* -- earth/fire/lightning eruptions | 24 |
| MEDIUM | M5VFXVOL2 (select fires) | NFire_00, NFire_Exp_00 -- Fire Wall and explosions | ~10 |
| LOW | SmokePack | NS_EnvironmentSmokes -- Hiding/Cloaking smoke | 5-10 |
| LOW | InfinityBlade FX_Skill_TeleCharge | P_Skill_Telecharge_* -- projectile variants | 23 |
| LOW | A_Surface_Footstep | PSN_* -- character movement VFX | ~15 |
| LOW | LightVortexShader | MI_Light_* -- portal/vortex materials | ~10 |
| LOW | DrapEffet | NE_drop_effects* -- water zones | 7 |

### Migration Notes

- **InfinityBlade assets are Cascade (P_)**: They work in UE5 but are deprecated. For long-term maintainability, convert to Niagara after initial prototyping. Cascade is still rendered by UE5.7.
- **M5VFXVOL2 uses non-standard naming (NFire_)**: Will need to reference by full path rather than standard NS_ lookup.
- **A_Surface_Footstep uses PEN_/PSN_ naming**: Niagara-based but non-standard prefix.
- **LightVortexShader is material-only**: Apply to mesh planes for portal/vortex ground effects. No particle system needed.

---

*End of VFX Asset Reference*
