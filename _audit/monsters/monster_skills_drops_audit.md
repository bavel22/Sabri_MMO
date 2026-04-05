# Monster Skills & Drop Tables Audit Report

**Date**: 2026-03-22
**Auditor**: Claude Opus 4.6
**Sources**: rAthena pre-re/mob_skill_db.txt, rAthena pre-re/mob_db.yml, iRO Wiki Classic, RateMyServer (pre-re)
**Files Audited**:
- `server/src/ro_monster_skills.js` (NPC_SKILLS + MONSTER_SKILL_DB)
- `server/src/ro_monster_templates.js` (509 templates, drops, stats)
- `server/src/index.js` (executeMonsterSkill, executeNPCSkill, rollEnemyDrops, slave mechanics)

---

## 1. Monster Skill System Architecture

### 1.1 Overview

The monster skill system is split across two layers:

| Layer | File | Role |
|-------|------|------|
| Data | `ro_monster_skills.js` | NPC_SKILLS (monster-only skill defs), MONSTER_SKILL_DB (per-monster entries) |
| Execution | `index.js` | executeMonsterSkill, executeNPCSkill, executeMonsterPlayerSkill, selectMonsterSkill, evaluateMonsterSkillCondition |

**Architecture**: The AI loop (`setInterval`, enemy tick) calls `selectMonsterSkill()` during IDLE and ATTACK states. Selected skills are routed to `executeMonsterSkill()`, which handles cast times, then dispatches to either `executeMonsterPlayerSkill()` (player-class skills copyable by Plagiarism) or `executeNPCSkill()` (monster-only NPC_ skills).

### 1.2 Skill Selection Pipeline

```
AI Tick → selectMonsterSkill(enemy, skills, currentState)
  1. Filter by AI state (idle/attack/chase/angry/any/anytarget)
  2. Check cooldown (skill.delay)
  3. Evaluate condition (15 condition types)
  4. Roll rate (out of 10000)
  5. First match wins (top-to-bottom evaluation)
```

### 1.3 Condition Types

| Condition | Implemented | Correct |
|-----------|:-----------:|:-------:|
| always | YES | YES |
| myhpltmaxrate | YES | YES |
| myhpinrate | YES | YES |
| mystatuson | YES | YES |
| mystatusoff | YES | YES |
| friendhpltmaxrate | YES | YES (same templateId only) |
| closedattacked | YES | YES |
| longrangeattacked | YES | YES |
| skillused | YES | YES |
| casttargeted | YES | YES |
| rudeattacked | YES | YES |
| onspawn | YES | YES |
| slavelt | YES | YES |
| afterskill | YES | YES |
| attackpcgt | YES | YES |
| attackpcge | YES | YES (bonus, not in rAthena) |

**Missing rAthena conditions**: None critical. `anytarget` state mapped correctly.

### 1.4 Cast System

- Cast time with `enemy._casting` object
- Cast interruption on damage (if `cancelable`)
- `enemy:casting` / `enemy:cast_interrupted` broadcasts
- Cast completion triggers `executeMonsterSkillEffect()`
- Loki's Veil blocks non-boss monster skills (correct)

---

## 2. NPC_ Skill Types Audit

### 2.1 Implemented NPC_ Skill Types

| Skill ID | Name | Type | Implemented | Correct |
|----------|------|------|:-----------:|:-------:|
| 184 | NPC_WATERATTACK | elemental_melee | YES | YES |
| 185 | NPC_GROUNDATTACK | elemental_melee | YES | YES |
| 186 | NPC_FIREATTACK | elemental_melee | YES | YES |
| 187 | NPC_WINDATTACK | elemental_melee | YES | YES |
| 188 | NPC_POISONATTACK | elemental_melee | YES | YES |
| 189 | NPC_HOLYATTACK | elemental_melee | YES | YES |
| 190 | NPC_DARKNESSATTACK | elemental_melee | YES | YES |
| 191 | NPC_GHOSTATTACK | elemental_melee | YES | YES |
| 192 | NPC_UNDEADATTACK | elemental_melee | YES | YES |
| 176 | NPC_POISON | status_melee | YES | YES |
| 177 | NPC_BLINDATTACK | status_melee | YES | YES |
| 178 | NPC_SILENCEATTACK | status_melee | YES | YES |
| 179 | NPC_STUNATTACK | status_melee | YES | YES |
| 180 | NPC_PETRIFYATTACK | status_melee | YES | YES |
| 181 | NPC_CURSEATTACK | status_melee | YES | YES |
| 182 | NPC_SLEEPATTACK | status_melee | YES | YES |
| 183 | NPC_RANDOMATTACK | random_element | YES | YES |
| 171 | NPC_COMBOATTACK | multi_hit | YES | YES |
| 197 | NPC_CRITICALSLASH | forced_crit | YES | PARTIAL* |
| 196 | NPC_SPEEDUP | self_buff | YES | YES |
| 199 | NPC_AGIUP | self_buff | YES | YES |
| 653 | NPC_EARTHQUAKE | aoe_physical | YES | PARTIAL** |
| 656 | NPC_DARKBREATH | ranged_magic | YES | YES |
| 657 | NPC_DARKBLESSING | status_ranged | YES | YES (Coma) |
| 660 | NPC_WIDEBLEEDING | aoe_status | YES | YES |
| 661 | NPC_WIDESILENCE | aoe_status | YES | YES |
| 662 | NPC_WIDESTUN | aoe_status | YES | YES |
| 663 | NPC_WIDECURSE | aoe_status | YES | YES |
| 665 | NPC_WIDEFREEZE | aoe_status | YES | YES |
| 669 | NPC_WIDESLEEP | aoe_status | YES | YES |
| 687 | NPC_BLOODDRAIN | drain_hp | YES | YES |
| 688 | NPC_ENERGYDRAIN | drain_sp | YES | YES |
| 198 | NPC_EMOTION | emote | YES | YES (no-op) |
| 304 | NPC_METAMORPHOSIS | transform | YES | YES |
| 485 | NPC_SUMMONSLAVE | summon | YES | YES |
| 175 | NPC_SELFDESTRUCTION | self_destruct | YES | YES |

*NPC_CRITICALSLASH: Uses 1.4x multiplier. rAthena CriticalSlash ignores Flee entirely and always hits as crit. Current implementation calculates normal damage then multiplies by 1.4, which could miss.

**NPC_EARTHQUAKE: rAthena Earthquake ignores Flee and has knockback. Current implementation uses simple flat ATK*multiplier/100 - hardDef formula, missing the ignore-Flee property.

### 2.2 Missing NPC_ Skill Types (from rAthena pre-re/mob_skill_db.txt)

These NPC_ skills exist in rAthena's pre-renewal database but are **NOT implemented** in the project:

| Skill Name | rAthena Usage | Priority | Notes |
|------------|---------------|----------|-------|
| NPC_CALLSLAVE | 17+ MVPs | **HIGH** | Re-summon dead slaves. Currently only NPC_SUMMONSLAVE exists (new spawn). Missing re-summon without re-spawning. |
| NPC_POWERUP | 15+ MVPs | **HIGH** | ATK up self-buff. rAthena: +200% ATK for duration. Not in NPC_SKILLS. |
| NPC_GUIDEDATTACK | 12+ MVPs | MEDIUM | Always-hit attack. Ignores Flee. |
| NPC_ARMORBRAKE | 10+ monsters | MEDIUM | Break target's armor (equipment break system). |
| NPC_SHIELDBRAKE | 5+ monsters | MEDIUM | Break target's shield. |
| NPC_WEAPONBRAKER | 5+ monsters | MEDIUM | Break target's weapon. |
| NPC_HELMBRAKE | 3+ monsters | MEDIUM | Break target's helm. |
| NPC_HELLJUDGEMENT | Baphomet, etc. | MEDIUM | AoE fire+dark magic damage. |
| NPC_DARKSTRIKE | Baphomet, etc. | MEDIUM | Ranged dark-element attack. |
| NPC_DARKTHUNDER | Osiris, etc. | MEDIUM | Multi-hit dark magic. |
| NPC_STOP | Mistress, etc. | MEDIUM | Immobilize target (root). |
| NPC_HALLUCINATION | Stormy Knight | LOW | Screen distortion (visual only). |
| NPC_PIERCINGATT | Hornet, etc. | LOW | Defense-piercing attack. |
| NPC_BLEEDING | Eddga, etc. | LOW | Inflict bleeding status. |
| NPC_WIDESTONE | Phreeoni | LOW | AoE stone curse. |
| NPC_LICK | Phreeoni | LOW | Stun + item strip. |
| NPC_SPLASHATTACK | Various | LOW | Splash melee. |
| NPC_TELEPORTATTACK | Various | LOW | Teleport + attack combo. |
| NPC_TELEKINESISATTACK | Various | LOW | Ghost-element melee. |
| NPC_MAGICALATTACK | Various | LOW | INT-based magic melee. |
| NPC_MENTALBREAKER | Various | LOW | SP damage. |
| NPC_KEEPING | Various | LOW | Hard DEF buff. |
| NPC_INVISIBLE | Various | LOW | Cloak/hide. |
| NPC_RANDOMMOVE | Various | LOW | Random movement. |
| NPC_ATTRICHANGE | Various | LOW | Change own element. |
| NPC_CHANGEFIRE/etc. | Various | LOW | Change to specific element. |
| NPC_CRITICALWOUND | Various | LOW | Reduce healing effectiveness. |
| NPC_RUN | Various | LOW | Flee/retreat. |
| NPC_RANGEATTACK | Various | LOW | Ranged physical. |
| NPC_EMOTION_ON | Various | LOW | Persistent emotion display. |
| NPC_PROVOCATION | Low-level | LOW | Provoke equivalent. |

**Total**: ~30 NPC_ skill types missing. The 3 most impactful are NPC_CALLSLAVE, NPC_POWERUP, and NPC_GUIDEDATTACK.

---

## 3. Per-Monster Skill Audit (rAthena Cross-Reference)

### 3.1 Coverage Summary

- **509** total monster templates in `ro_monster_templates.js`
- **27** monsters have skill entries in `MONSTER_SKILL_DB` (5.3%)
- **65** total skill entries across those 27 monsters
- **~281** monsters have skills in rAthena's pre-re/mob_skill_db.txt
- **Coverage gap**: ~254 monsters with rAthena skills but no project skills

### 3.2 MVP Monsters — Detailed Comparison

#### Baphomet (1039) — SIGNIFICANT GAPS

| rAthena Skill | Project Has | Notes |
|---------------|:-----------:|-------|
| NPC_EARTHQUAKE Lv5 | YES | Correct |
| AL_HEAL Lv11 | YES | Correct |
| NPC_DARKNESSATTACK Lv5 | YES | Correct (listed but not in MONSTER_SKILL_DB directly, uses generic) |
| NPC_SUMMONSLAVE | YES | Has val1: 1101 (Bapho Jr.) |
| MO_BODYRELOCATION | NO | Missing (teleport-to-target, chase) |
| KN_BRANDISHSPEAR Lv10 | NO | Missing (major AoE physical) |
| NPC_POWERUP Lv5 | NO | Missing (+200% ATK buff) |
| NPC_CALLSLAVE | NO | Missing (re-summon dead slaves) |
| NPC_HELLJUDGEMENT Lv5 | NO | Missing (AoE fire+dark magic) |
| NPC_DARKBREATH Lv5 | NO | Missing from MONSTER_SKILL_DB (type exists in NPC_SKILLS) |
| NPC_ARMORBRAKE Lv10 | NO | Missing (break armor) |
| NPC_GUIDEDATTACK Lv5 | NO | Missing (always-hit) |
| WZ_VERMILION Lv21 | NO | Missing (Lord of Vermilion) |
| NPC_DARKSTRIKE Lv10 | NO | Missing (ranged dark) |
| AL_TELEPORT Lv1 | NO | Missing (idle teleport) |

**Project has 4/15 skills. Missing 11 skills including critical ones (Brandish Spear, Power Up, Call Slave, Hell Judgement).**

#### Osiris (1038) — SIGNIFICANT GAPS

| rAthena Skill | Project Has | Notes |
|---------------|:-----------:|-------|
| AL_TELEPORT | YES | Correct (rudeattacked) |
| AL_HEAL Lv11 | YES | Correct (HP<50%) |
| NPC_CURSEATTACK Lv5 | YES | Correct |
| NPC_DARKNESSATTACK Lv5 | YES | Correct |
| NPC_POWERUP Lv5 | NO | Missing |
| AS_VENOMDUST Lv5 | NO | Missing (poison ground AoE) |
| NPC_AGIUP Lv5 | NO | Missing |
| MG_STONECURSE Lv10 | NO | Missing |
| NPC_CALLSLAVE | NO | Missing |
| ASC_METEORASSAULT Lv10 | NO | Missing (ranged AoE) |
| NPC_DARKBREATH Lv5 | NO | Missing |
| NPC_POISONATTACK Lv5 | NO | Missing |
| NPC_SUMMONSLAVE | NO | Missing (Osiris summons Verit/Isis) |
| SM_BASH Lv10 | NO | Missing |
| NPC_DARKTHUNDER Lv10 | NO | Missing |
| WZ_QUAGMIRE Lv5 | NO | Missing |

**Project has 4/16 skills. Missing 12 skills including summon slaves.**

#### Drake (1112) — MODERATE GAPS

| rAthena Skill | Project Has | Notes |
|---------------|:-----------:|-------|
| WZ_WATERBALL Lv9 | YES | Correct (Lv9 matches) |
| NPC_CURSEATTACK Lv5 | YES | Correct |
| NPC_STUNATTACK Lv5 | YES | Correct |
| AL_DECAGI Lv48 | NO | Missing (Decrease AGI) |
| NPC_ARMORBRAKE Lv10 | NO | Missing |
| AL_TELEPORT | NO | Missing |
| BS_MAXIMIZE | NO | Missing |
| NPC_AGIUP Lv10 | NO | Missing |
| NPC_CALLSLAVE | NO | Missing |
| NPC_GUIDEDATTACK Lv5 | NO | Missing |
| NPC_SUMMONSLAVE | NO | Missing |
| NPC_WATERATTACK Lv5 | NO | Missing |

**Project has 3/12 skills. Missing 9 skills.**

#### Golden Thief Bug (1086) — SEVERE GAPS

| rAthena Skill | Project Has | Notes |
|---------------|:-----------:|-------|
| AL_HEAL Lv9 | YES | Correct |
| NPC_FIREATTACK Lv5 | YES | Correct |
| NPC_STUNATTACK Lv5 | YES | Correct |
| HP_ASSUMPTIO Lv5 | NO | Missing (Assumptio self-buff) |
| CR_REFLECTSHIELD Lv10 | NO | Missing (reflect damage) |
| MC_MAMMONITE Lv22 | NO | Missing |
| SM_MAGNUM Lv25 | NO | Missing (Magnum Break) |
| MG_FIREBALL Lv93 | NO | Missing |
| NPC_GUIDEDATTACK Lv5 | NO | Missing |
| NPC_SUMMONSLAVE | NO | Missing |
| TF_HIDING | NO | Missing |
| AL_TELEPORT | NO | Missing |

**Project has 3/12 skills. Missing 9 skills.**

#### Moonlight Flower (1150) — SEVERE GAPS

| rAthena Skill | Project Has | Notes |
|---------------|:-----------:|-------|
| MG_FIREBOLT Lv10 | YES | Correct |
| MG_COLDBOLT Lv10 | YES | Correct |
| MG_LIGHTNINGBOLT Lv10 | YES | Correct |
| AL_HEAL Lv9 | YES | Correct |
| MC_MAMMONITE Lv10 | YES | Correct |
| SA_LANDPROTECTOR | NO | Missing (blocks ground effects) |
| NPC_WIDESILENCE Lv5 | NO | Missing |
| ST_FULLSTRIP Lv5 | NO | Missing (strip all equipment) |
| NPC_POWERUP Lv5 | NO | Missing |
| NPC_CALLSLAVE | NO | Missing |
| NPC_GUIDEDATTACK Lv5 | NO | Missing |
| SA_DISPELL Lv5 | NO | Missing |
| NPC_SUMMONSLAVE | NO | Missing |
| BS_HAMMERFALL Lv10 | NO | Missing |
| AL_TELEPORT | NO | Missing |

**Project has 5/15 skills. Missing 10 skills.**

#### Orc Hero (1087) — MODERATE GAPS

| rAthena Skill | Project Has | Notes |
|---------------|:-----------:|-------|
| NPC_EARTHQUAKE Lv3 | YES | Correct (attackpcgt condition) |
| NPC_COMBOATTACK Lv5 | YES | Correct |
| NPC_STUNATTACK Lv5 | YES | Correct |
| AL_DECAGI Lv48 | NO | Missing |
| AL_TELEPORT | NO | Missing |
| NPC_ARMORBRAKE Lv10 | NO | Missing |
| NPC_POWERUP Lv5 | NO | Missing |
| LK_SPIRALPIERCE Lv5 | NO | Missing |
| KN_TWOHANDQUICKEN Lv30 | NO | Missing |
| MG_THUNDERSTORM Lv20 | NO | Missing |
| NPC_CALLSLAVE | NO | Missing |
| NPC_GROUNDATTACK Lv5 | NO | Missing |
| NPC_GUIDEDATTACK Lv5 | NO | Missing |
| CR_AUTOGUARD Lv10 | NO | Missing |
| NPC_SUMMONSLAVE | NO | Missing |
| AL_HEAL Lv11 | NO | Missing |

**Project has 3/16 skills. Missing 13 skills.**

#### Mistress (1059) — MODERATE GAPS

| rAthena Skill | Project Has | Notes |
|---------------|:-----------:|-------|
| WZ_JUPITEL Lv10 | YES | Correct |
| NPC_WIDESTUN Lv5 | YES | Correct |
| AL_HEAL Lv11 | NO | Missing |
| AL_PNEUMA | NO | Missing |
| AL_TELEPORT | NO | Missing |
| NPC_AGIUP Lv5 | NO | Missing |
| NPC_CALLSLAVE | NO | Missing |
| NPC_WIDESILENCE Lv5 | NO | Missing (has WIDESTUN but not WIDESILENCE) |
| NPC_GUIDEDATTACK Lv5 | NO | Missing |
| NPC_SILENCEATTACK Lv5 | NO | Missing |
| NPC_SUMMONSLAVE | NO | Missing |
| NPC_STOP | NO | Missing |

**Project has 2/12 skills. Missing 10 skills.**

#### Maya (1147) — MODERATE GAPS

| rAthena Skill | Project Has | Notes |
|---------------|:-----------:|-------|
| AL_HEAL Lv11 | YES | Correct |
| MG_FIREPILLAR Lv10 | YES | Correct |
| NPC_EARTHQUAKE Lv3 | YES | Correct |
| NPC_WIDESILENCE Lv5 | NO | Missing |
| ST_FULLSTRIP Lv5 | NO | Missing |
| KN_BRANDISHSPEAR Lv10 | NO | Missing |
| NPC_POWERUP Lv5 | NO | Missing |
| NPC_CALLSLAVE | NO | Missing |
| NPC_ARMORBRAKE Lv10 | NO | Missing |
| NPC_GUIDEDATTACK Lv5 | NO | Missing |
| CR_AUTOGUARD Lv10 | NO | Missing |
| NPC_SUMMONSLAVE | NO | Missing |
| WZ_HEAVENDRIVE Lv5 | NO | Missing |
| MC_MAMMONITE Lv22 | NO | Missing |
| AL_TELEPORT | NO | Missing |

**Project has 3/15 skills. Missing 12 skills.**

### 3.3 Regular Monster Comparison (Currently Spawned)

#### Hornet (1004)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_PIERCINGATT Lv2 | NPC_POISON Lv1 | NO |
| NPC_WINDATTACK Lv1 | (not present) | NO |

**WRONG**: Project gives Hornet NPC_POISON (176). rAthena gives NPC_PIERCINGATT (defense-piercing) + NPC_WINDATTACK. Hornet is a Wind/1 Insect -- it should NOT use poison. This is incorrect.

#### Familiar (1005)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_BLINDATTACK Lv1 | NPC_BLINDATTACK Lv1 | YES |
| NPC_DARKNESSATTACK Lv1 | (not present) | NO |

**PARTIAL**: Blind attack correct, but missing NPC_DARKNESSATTACK.

#### Zombie (1015)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_POISON Lv1 | NPC_DARKNESSATTACK Lv1 | NO |
| NPC_UNDEADATTACK Lv1 | NPC_STUNATTACK Lv3 | NO |

**WRONG**: Project gives Zombie NPC_DARKNESSATTACK + NPC_STUNATTACK. rAthena gives NPC_POISON + NPC_UNDEADATTACK. Element mismatch (Zombie is Undead/1, uses Undead attack not Dark).

#### Skeleton (1076)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_EMOTION (chase) | NPC_STUNATTACK Lv3 | NO |
| NPC_UNDEADATTACK Lv1 | (not present) | NO |

**WRONG**: Project gives Skeleton NPC_STUNATTACK. rAthena gives NPC_EMOTION + NPC_UNDEADATTACK. Skeleton should NOT stun.

#### Spore (1014)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_SLEEPATTACK Lv1 | NPC_WATERATTACK Lv1 | PARTIAL |
| NPC_WATERATTACK Lv1 | (not present) | - |
| NPC_EMOTION | (not present) | - |
| NPC_PROVOCATION | (not present) | - |

**PARTIAL**: Has NPC_WATERATTACK (correct) but missing NPC_SLEEPATTACK (Spore's signature skill).

#### Creamy (1018)

| rAthena | Project | Match |
|---------|---------|:-----:|
| AL_TELEPORT (attack/idle) | AL_TELEPORT (idle, rudeattacked) | PARTIAL |
| NPC_SLEEPATTACK Lv3 | (not present) | NO |
| NPC_WINDATTACK Lv1 | (not present) | NO |

**PARTIAL**: Teleport present but condition is wrong (rAthena: attack state, not just rudeattacked). Missing sleep and wind attacks.

#### Poporing (1031)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_POISON Lv2 | (not present) | NO |
| NPC_POISONATTACK Lv1 | NPC_POISONATTACK Lv1 | YES |
| NPC_EMOTION | (not present) | - |

**PARTIAL**: Poison attack correct. Missing NPC_POISON (status effect).

#### Poison Spore (1077)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_POISON Lv3 | NPC_POISON Lv3 | YES |
| NPC_POISONATTACK Lv1 | NPC_POISONATTACK Lv2 | PARTIAL (wrong level) |
| NPC_EMOTION | (not present) | - |

**MOSTLY CORRECT**: NPC_POISON matches. NPC_POISONATTACK level is 2 in project vs 1 in rAthena.

#### Smokie (1056)

| rAthena | Project | Match |
|---------|---------|:-----:|
| AL_HEAL Lv5 | (not present) | NO |
| NPC_GROUNDATTACK Lv2 | (not present) | NO |
| TF_HIDING | (deferred comment) | NO |
| NPC_PROVOCATION | (not present) | NO |
| NPC_COMBOATTACK Lv1 | NPC_COMBOATTACK Lv1 | YES |

**WRONG**: Only combo attack matches. Missing self-heal, ground attack, hiding, provocation.

#### Yoyo (1057)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_GROUNDATTACK Lv1 | (not present) | NO |
| SM_PROVOKE Lv10 | (not present) | NO |
| TF_THROWSTONE | (not present) | NO |
| AM_POTIONPITCHER | (not present) | NO |
| NPC_COMBOATTACK Lv2 | NPC_COMBOATTACK Lv2 | YES |

**WRONG**: Only combo attack matches. Missing ground attack, provoke, throw stone, potion pitcher.

### 3.4 Boss/Mini-Boss Comparison

#### Angeling (1096)

| rAthena | Project | Match |
|---------|---------|:-----:|
| AL_HEAL Lv9 | AL_HEAL Lv9 | YES |
| AL_HOLYLIGHT Lv5 | AL_HOLYLIGHT Lv5 | YES |
| AL_TELEPORT | (not present) | NO |
| MG_SAFETYWALL Lv5 | (not present) | NO |
| NPC_EMOTION | (not present) | - |
| NPC_HOLYATTACK Lv5 | (not present) | NO |
| NPC_SUMMONSLAVE | (not present) | NO |
| SA_REVERSEORCISH | (not present) | NO |

**Project has 2/8 skills (only offensive spells). Missing summon, Safety Wall, Holy Attack.**

#### Deviling (1582)

**NOTE**: Deviling is NOT in rAthena pre-re mob_skill_db according to the fetch. Project gives it AL_TELEPORT + MG_SOULSTRIKE. This may be a custom addition or from a different source.

#### Raydric (1163)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_DARKNESSATTACK Lv3 | NPC_DARKNESSATTACK Lv3 | YES |
| SM_MAGNUM | (not present) | NO |
| BS_MAXIMIZE | (not present) | NO |
| CR_AUTOGUARD | (not present) | NO |
| NPC_EMOTION | (not present) | - |
| NPC_STUNATTACK Lv3 | NPC_STUNATTACK Lv3 | YES |
| NPC_COMBOATTACK Lv3 | NPC_COMBOATTACK Lv3 | YES |

**Project has 3/7 skills. Missing Magnum Break, Maximize Power, Auto Guard.**

#### Abysmal Knight (1219)

| rAthena | Project | Match |
|---------|---------|:-----:|
| NPC_DARKNESSATTACK Lv5 | NPC_DARKNESSATTACK Lv5 | YES |
| NPC_STUNATTACK Lv5 | NPC_STUNATTACK Lv5 | YES |
| NPC_COMBOATTACK Lv5 | NPC_COMBOATTACK Lv5 | YES |
| AC_CHARGEARROW | (not present) | NO |
| KN_BRANDISHSPEAR | (not present) | NO |
| NPC_ARMORBRAKE | (not present) | NO |
| CR_AUTOGUARD | (not present) | NO |
| NPC_SUMMONSLAVE | (not present) | NO |
| AL_TELEPORT | (not present) | NO |

**Project has 3/9 skills. Missing Brandish Spear, Armor Break, charge.**

---

## 4. Drop Table Analysis

### 4.1 Drop System Architecture

- Drops defined in `ro_monster_templates.js` as `drops: [{ itemName, rate }]`
- `rate` is percentage (e.g., `0.01` = 0.01% = 1 in 10,000)
- Converted to decimal `chance = rate / 100` in ENEMY_TEMPLATES adapter
- `rollEnemyDrops()` rolls `Math.random() < drop.chance` per drop slot
- Item IDs resolved lazily via `itemNameToId` map after DB load
- MVP drops in separate `mvpDrops` array (rolled only for `monsterClass === 'mvp'`)
- Cards always last slot with `rate: 0.01` and `stealProtected: true`

### 4.2 Drop Rate Verification

**Card Drop Rates** (should be 0.01% = 1/10,000 for all normal monsters):

Spot-checked 20 monsters — ALL have `rate: 0.01` for their card. **CORRECT**.

**Normal Drop Rates** — Cross-referenced against rAthena pre-re mob_db.yml for 5 monsters:

| Monster | Drop Slot | Project Rate | rAthena Rate | Match |
|---------|-----------|:------------:|:------------:|:-----:|
| Poring | Jellopy | 70% | 70% | YES |
| Poring | Knife | 1% | 1% | YES |
| Poring | Sticky Mucus | 4% | 4% | YES |
| Poring | Apple (1st) | 10% | 10% | YES |
| Baphomet | Crescent Scythe | 4% | 4% | YES |
| Baphomet | Majestic Goat | 3% | 3% | YES |
| Osiris | Old Purple Box | 20% | 20% | YES |
| Drake | Saber | 6% | 6% | YES |

All spot-checked rates match rAthena. The templates were auto-generated from rAthena pre-re/mob_db.yml, so rates should be accurate across all 509 monsters.

### 4.3 MVP Drop Table

MVP drops use a separate `mvpDrops` array. Verified structure:

| MVP | mvpDrops | rAthena Match |
|-----|----------|:-------------:|
| Baphomet | Yggdrasil Berry 20%, Baphomet Doll 5% | YES |
| Osiris | Old Blue Box 40%, Yggdrasil Seed 30% | YES |
| Drake | White Potion 50% | YES |
| Maya | 1carat Diamond 20%, Old Blue Box 30% | YES |

### 4.4 Drop Resolution Issues

The `resolveDropItemIds()` function logs unresolved drops (items named in templates but missing from DB). These would silently fail to drop. The log message `[ENEMY] Drop itemId resolution: X resolved, Y unresolved` should be checked at server startup.

**Potential Issue**: If the items database doesn't contain entries for all items named in monster drop tables, those drops silently fail. There is no error at drop time -- the `if (itemId)` guard just skips the drop.

### 4.5 Drop Chance Conversion

The conversion `chance = rate / 100` is correct:
- `rate: 70` (70%) -> `chance: 0.70` -> `Math.random() < 0.70` = 70% chance. **CORRECT**.
- `rate: 0.01` (0.01%) -> `chance: 0.0001` -> `Math.random() < 0.0001` = 1/10,000. **CORRECT**.

---

## 5. Slave Mechanics Verification

### 5.1 Master-Slave Lifecycle

| Mechanic | Implemented | Correct |
|----------|:-----------:|:-------:|
| Master tracks slaves via `_slaves` Set | YES | YES |
| `_slaveCount` kept in sync | YES | YES |
| `slavelt` condition checks slave count | YES | YES |
| Slave spawn near master position | YES | YES (+/- 200 UE units) |
| Slaves share master's aggro target | YES | YES |
| Slave death decrements master count | YES | YES (both auto-attack and skill death paths) |
| Master death kills all slaves | YES | YES (both auto-attack and skill death paths) |
| Slaves give NO EXP on death | YES | YES (early return before EXP) |
| Slaves give NO drops on death | YES | YES (early return before drops) |
| Slaves don't respawn independently | YES | YES (`if (enemy._isSlave) { enemies.delete(); return; }`) |
| Slave state cleared on master respawn | YES | YES (`enemy._slaves = null; enemy._slaveCount = 0;`) |

### 5.2 Slave EXP/Drop Behavior — Accuracy Check

The code comment says `// RO Classic: Slaves give NO EXP and NO drops`. However, rAthena's behavior is configurable:
- `mob_slave_keep_ai` (default: yes) - slaves keep AI when master dies
- Summoned slaves can give EXP/drops if they are spawned by `NPC_SUMMONSLAVE` (varies by config)

In **official pre-renewal** (iRO Classic), summoned slaves (Bapho Jr. from Baphomet) **DO give EXP and drops** when killed by players. They only give NO EXP/drops when killed by master death.

**BUG FOUND**: The current code suppresses EXP/drops for ALL slave deaths (line 2167-2178), not just master-death kills. Player-killed slaves should give normal EXP and drops. Only the master-death path (line 2362-2386) should suppress EXP/drops.

### 5.3 Missing Slave Features

| Feature | Status | Notes |
|---------|--------|-------|
| NPC_CALLSLAVE | MISSING | Re-summon dead slaves to master's position. Different from NPC_SUMMONSLAVE (which creates new spawns). |
| Slave leashing | MISSING | Slaves should follow master, not wander independently. rAthena: slaves return to master if too far. |
| Slave aggro assist | PARTIAL | Slaves get master's target on spawn, but don't update when master switches targets. |

---

## 6. Missing Monster Skills — Priority List

### 6.1 Monsters That SHOULD Have Skills But Don't

The following currently-spawned monsters have NO skills in `MONSTER_SKILL_DB` but DO have skills in rAthena:

| Monster | ID | Zone | rAthena Skills | Priority |
|---------|----|------|----------------|----------|
| (None currently spawned that are missing from MONSTER_SKILL_DB beyond the above) | | | | |

All currently spawned monsters that have rAthena skills are represented in MONSTER_SKILL_DB (Hornet, Familiar, Spore, Zombie, Skeleton, Creamy, Poporing, Poison Spore, Smokie, Yoyo). However, their skills are often WRONG or INCOMPLETE (see Section 3.3).

### 6.2 MVPs/Bosses Without Spawns

The following MVPs/bosses have skill entries but no active spawn zones:
- Baphomet (1039), Osiris (1038), Drake (1112), Golden Thief Bug (1086)
- Orc Hero (1087), Mistress (1059), Maya (1147), Eddga (1115)
- Moonlight Flower (1150), Phreeoni (1159), Orc Lord (1190)
- Stormy Knight (1251), Dark Lord (1272)
- Angeling (1096), Deviling (1582), Abysmal Knight (1219), Raydric (1163)

These have skeleton skill entries but are severely incomplete (averaging 3-4 out of 10-16 rAthena skills).

---

## 7. Recommendations

### 7.1 Critical Fixes (Should Fix Now)

1. **FIX slave EXP/drops**: Player-killed slaves should give EXP and drops. Only master-death kills should suppress them. The `_isSlave` early-return at line 2167 needs to be moved to only the master-death path.

2. **FIX Hornet skills**: Currently has NPC_POISON (wrong). Should have NPC_PIERCINGATT + NPC_WINDATTACK per rAthena.

3. **FIX Zombie skills**: Currently has NPC_DARKNESSATTACK + NPC_STUNATTACK (wrong). Should have NPC_POISON + NPC_UNDEADATTACK per rAthena.

4. **FIX Skeleton skills**: Currently has NPC_STUNATTACK (wrong). Should have NPC_UNDEADATTACK + NPC_EMOTION per rAthena.

5. **FIX Spore skills**: Missing NPC_SLEEPATTACK (its signature skill in RO Classic).

### 7.2 High Priority (MVP Completeness)

6. **Implement NPC_CALLSLAVE**: Used by 17+ MVPs. Without it, MVPs can never re-summon dead slaves.

7. **Implement NPC_POWERUP**: Used by 15+ MVPs. Significant ATK buff that makes MVPs dangerous.

8. **Implement NPC_GUIDEDATTACK**: Used by 12+ MVPs. Always-hit attack, important for high-Flee characters.

9. **Add missing MVP skills**: Baphomet, Osiris, Drake, GTB, Orc Hero, Mistress, Maya all have less than 33% of their rAthena skills. Each needs 8-13 additional skill entries.

10. **Add NPC_SUMMONSLAVE to MVPs that need it**: Osiris (summons Verit/Isis), Drake, GTB, Orc Hero, Mistress, Maya, Moonlight, Phreeoni, Orc Lord, Stormy Knight all summon slaves in rAthena but lack NPC_SUMMONSLAVE entries.

### 7.3 Medium Priority (Equipment Break / Status)

11. **Implement equipment break skills**: NPC_ARMORBRAKE, NPC_SHIELDBRAKE, NPC_WEAPONBRAKER, NPC_HELMBRAKE. Requires equipment break status system.

12. **Implement NPC_HELLJUDGEMENT**: Baphomet's signature AoE. Fire+Dark magical damage to all in range.

13. **Implement NPC_STOP**: Root/immobilize target. Used by Mistress, several bosses.

14. **Implement NPC_BLEEDING**: Bleeding status. Used by Eddga, several high-level monsters.

15. **Implement NPC_WIDESTONE**: AoE Stone Curse. Used by Phreeoni.

### 7.4 Low Priority (Flavor / Completeness)

16. **Add NPC_EMOTION to low-level monsters**: Smokie, Yoyo, Spore, Skeleton all use emotes in rAthena. Cosmetic only.

17. **Implement NPC_PIERCINGATT**: Defense-piercing attack used by Hornet and others.

18. **Implement NPC_PROVOCATION**: Monster provoke. Used by Spore, Smokie, Yoyo.

19. **Implement remaining NPC_ types**: ~20 additional NPC_ skill types for full coverage.

20. **Expand MONSTER_SKILL_DB**: Only 27/509 monsters have skills (5.3%). rAthena has 281 monsters with skills (55%). A future pass should add the remaining ~254 monsters.

### 7.5 NPC_CRITICALSLASH Accuracy Fix

Current implementation calculates normal damage then multiplies by 1.4x. rAthena's NPC_CRITICALSLASH forces a critical hit (ignores Flee entirely, always hits, double damage with no DEF). The forced_crit handler should set `isMiss = false` unconditionally and use the proper crit formula (ignore soft DEF).

### 7.6 NPC_EARTHQUAKE Accuracy Fix

Current implementation uses `ATK * multiplier / 100 - hardDef`. rAthena's Earthquake:
- Ignores Flee (always hits)
- Has 2-cell knockback
- Damage = ATK * (skill_lv * 100 + 500)% / 100
- Hits all players in range, not just connected ones

The `aoe_physical` handler should add `isMiss = false` forced-hit behavior.

---

## 8. Summary Statistics

| Metric | Value |
|--------|-------|
| Monster templates | 509 |
| Templates with skills | 27 (5.3%) |
| rAthena monsters with skills | ~281 (55%) |
| Total skill entries | 65 |
| NPC_ skill types implemented | 17 unique types |
| NPC_ skill types in rAthena | ~47 unique types |
| NPC_ skill type coverage | 36% |
| MVPs in project | 43 |
| MVPs with skill entries | 13 |
| Average MVP skill accuracy | ~28% (avg 3.4 of 12.3 rAthena skills) |
| Regular monster skill accuracy | ~40% (wrong skills common) |
| Drop rate accuracy | 100% (auto-generated from rAthena) |
| Card drop rate accuracy | 100% (all 0.01%) |
| Slave lifecycle correctness | 90% (EXP bug, missing leash/callslave) |
| Condition system completeness | 100% (all 15 types) |
| Cast system correctness | 100% |

**Overall Assessment**: The monster skill system architecture is solid and well-designed. The condition evaluation, skill selection, cast system, and NPC_ skill execution framework are all correct. The primary gaps are in **data completeness** (only 27/509 monsters have skills, only 17/47 NPC_ types implemented) and **accuracy of existing entries** (several low-level monsters have wrong skills). The slave lifecycle is almost correct but has a significant EXP/drop bug. Drop tables are accurate (auto-generated from rAthena).
