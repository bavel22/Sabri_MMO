# Audit: Ground Effects & Special Mechanics

## Summary

This audit compares deep research documents `34_Mount_Falcon_Trap_Performance.md` and `40_Ground_Effects_AoE.md` against the actual server implementation in `server/src/index.js`, `server/src/ro_ground_effects.js`, and `server/src/ro_damage_formulas.js`.

**Overall Assessment:** The implementation is largely correct and well-architected. The ground effect foundation system in `ro_ground_effects.js` is comprehensive but only partially used -- `index.js` has its own parallel ground effects registry that is the one actually in production. The mount, falcon, and performance systems are fully functional with accurate RO Classic formulas. Trap handling is solid but has several formula and cost discrepancies vs deep research.

**Key Findings:**
- 6 critical discrepancies (formula/cost mismatches)
- 5 moderate discrepancies (AoE, duration, behavior gaps)
- 8 missing features (visual, NPC, and edge-case systems)
- 2 internal contradictions within the deep research docs themselves

---

## Ground Effect System (categories, tick rates, overlap rules)

### Architecture

The project has **two** ground effect systems:

1. **`ro_ground_effects.js`** (foundation module) -- A fully-featured, well-documented ground effect infrastructure with typed categories (`EFFECT_CATEGORY`), `GroundEffect` typedef, `createGroundEffect()`, `processGroundEffectTick()`, `findEntitiesInEffect()`, immunity windows, per-caster limits, Land Protector blocking, Sage zone mutual exclusion, and trap trigger detection. This is the "clean" version.

2. **`index.js` inline system** (production) -- A simpler inline `activeGroundEffects` Map at line 568 with its own `createGroundEffect()` (line 571), `removeGroundEffect()` (line 595), and `getGroundEffectsAtPosition()` (line 599). This is what actually runs in production.

**Assessment:** The `index.js` inline system is used by all skill handlers. The `ro_ground_effects.js` module is only imported for `calculateTrapDamage()` (line 67). The full infrastructure in `ro_ground_effects.js` (tick loop, entity finder, immunity tracking, etc.) is **unused** -- all that logic is hand-coded per-skill in `index.js`.

**Deep Research vs Implementation:**

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Cell-based grid | Discrete cell occupancy | Circular radius (distance check) | Simplified -- functionally equivalent |
| CELL_SIZE | 50 UE units | 50 UE units (in `ro_ground_effects.js`) | MATCH |
| Categories | 8 types (damage/heal/buff/debuff/trap/obstacle/protector/contact) | String-based `type` + `category` fields per effect | Equivalent |
| Tick rates | Per-skill (450ms SG, 1000ms Sanctuary, etc.) | Per-skill via `setInterval` loops and onTick callbacks | MATCH |
| Expiration | Time-based, hit-based, wave-based, combined | Time-based (`expiresAt`), hit counters, wave counters | MATCH |
| Land Protector blocking | Blocks 16+ skill types | `LP_BLOCKED_TYPES` Set with 15 types (index.js line 573) | MATCH (missing `thunderstorm` in index.js but present in `ro_ground_effects.js`) |
| LP mutual destruction | LP vs LP destroys old one | Implemented at Sage skill handler | MATCH |
| Per-caster limits | Fire Wall 3, SW 1, Pneuma 1, etc. | Inline per-handler checks (e.g., trap max 3) | MATCH |
| Sage zone mutual exclusion | Only one of Volcano/Deluge/Gale per caster | Implemented in Sage handler (removes others on cast) | MATCH |
| NoReiteration/NoOverlap | rAthena UF_ flags | Not explicitly modeled -- handled via caster limits and immunity windows | Simplified but functional |

### Ground Effect Tick Loop

The trap tick runs at **200ms** (`setInterval` at line 31176). Other ground effects (Storm Gust, LoV, Meteor Storm, Sanctuary, etc.) use their own internal tick timing via `onTick` callbacks or inline handlers in the combat loop.

**Deep Research tick rates vs implementation:**

| Skill | Research | Implementation | Match |
|-------|----------|---------------|-------|
| Storm Gust | ~450ms, 10 ticks | While-loop catch-up at 250ms tick (session note: 2026-03-20) | ADJUSTED -- 250ms is faster than 450ms |
| LoV | 1000ms, 4 waves | While-loop catch-up timing | MATCH (4 waves) |
| Meteor Storm | ~300ms between meteors | 300ms interval implemented | MATCH |
| Sanctuary | 1000ms | 1000ms heal tick | MATCH |
| Magnus Exorcismus | 3000ms immunity window | Implemented via immunity window tracking | MATCH |
| Fire Wall | ~25-40ms per hit | Contact-based, charges per segment | SIMPLIFIED -- single zone, not per-segment |
| Ice Wall | 1000ms (50 HP/sec decay) | 50 HP/sec passive decay | MATCH |
| Trap check | 200ms | 200ms setInterval (line 31176) | MATCH |
| Quagmire | Immediate on entry | Applied on entry/standing | MATCH |
| Performance | 1000ms | `PERFORMANCE_CONSTANTS.TICK_INTERVAL = 1000` | MATCH |

---

## Trap System (placement, damage formulas, interactions)

### Trap Placement

| Feature | Deep Research | Implementation | Status |
|---------|-------------|---------------|--------|
| Cell-based (1 cell) | Each trap occupies 1 cell | Uses ground effect with radius | MATCH |
| 2-cell min from entities | Cannot place within 2 cells of players/monsters/traps | **NOT enforced** -- traps can be placed anywhere | MISSING |
| Max active traps | Configurable, suggested 2-3 per type | Hard-coded max 3 total per hunter (line 17829-17833) | DIFFERENT -- research suggests per-type limits |
| Trap durability | 3,500 HP, destructible | **NOT implemented** -- traps cannot be attacked | MISSING |
| Trap visibility | Hidden from enemies in PvP | **NOT implemented** -- no visibility system | MISSING (PvP feature) |
| Trap arming delay | 1-2 second delay | **NOT implemented** -- immediate activation | MISSING |
| Trap item return on expiry | Trap item returned if trap expires unactivated | **NOT implemented** -- trap item consumed on placement | MISSING |
| Arrow Shower pushing | Arrow Shower pushes traps | **NOT implemented** | MISSING |
| WoE 4x duration | Trap duration quadrupled in WoE | **NOT implemented** (no WoE system yet) | FUTURE |

### Trap Item Costs

| Trap | Deep Research Cost | Implementation Cost | Match |
|------|-------------------|-------------------|-------|
| Skid Trap | 1 Trap | 1 Trap | MATCH |
| Land Mine | 1 Trap | 1 Trap | MATCH |
| Ankle Snare | 1 Trap | 1 Trap | MATCH |
| Sandman | 1 Trap | 1 Trap | MATCH |
| Flasher | 1 Trap | 1 Trap | MATCH |
| Freezing Trap | **2 Traps** | **1 Trap** | **DISCREPANCY** |
| Blast Mine | **2 Traps** | **1 Trap** | **DISCREPANCY** |
| Claymore Trap | 2 Traps | 2 Traps | MATCH |
| Shockwave Trap | 2 Traps | 2 Traps | MATCH |
| Talkie Box | 1 Trap | 1 Trap | MATCH |

**Critical:** Line 17818 only charges 2 for `claymore_trap` and `shockwave_trap`. Deep research (34_Mount, lines 393-394; 40_Ground, lines 65) says Freezing Trap and Blast Mine also cost 2 traps each. The `ro_ground_effects.js` comment at line 568 also says "Claymore Trap and Shockwave Trap cost 2". This is inconsistent -- the iRO Wiki and rAthena both confirm Freezing Trap and Blast Mine cost 2 Trap items.

### Trap Damage Formulas

| Trap | Deep Research Formula | Implementation | Match |
|------|----------------------|---------------|-------|
| Land Mine | `DEX*(3+BaseLv/100)*(1+INT/35)*Lv +/- 10% + TrapResearch*40` | `calculateTrapDamage('land_mine', lv, dex, int, eleMod)` = `lv*(dex+75)*(1+int/100)` | **DIFFERENT** |
| Blast Mine | `DEX*(2+BaseLv/100)*(1+INT/35)*Lv +/- 10% + TrapResearch*40` | `calculateTrapDamage('blast_mine', lv, dex, int, eleMod)` = `lv*(50+dex/2)*(1+int/100)` | **DIFFERENT** |
| Claymore | `DEX*(3+BaseLv/85)*(1.1+INT/35)*Lv +/- 10% + TrapResearch*40` | `calculateTrapDamage('claymore_trap', lv, dex, int, eleMod)` = `lv*(75+dex/2)*(1+int/100)` | **DIFFERENT** |
| Freezing Trap | `PlayerATK + TrapResearch*40` (ATK-based) | ATK-based: `(25+25*lv)% * ownerATK` with DEF reduction | DIFFERENT but both ATK-based |

**Analysis of Land Mine formula discrepancy:**
- Deep research (34_Mount, line 441): `DEX * (3 + BaseLv/100) * (1 + INT/35) * Lv`
- Deep research (40_Ground, line 558): `DEX * (3 + BaseLv/100) * (1 + INT/35) * Lv`
- `ro_ground_effects.js` (line 560): `lv * (dex + 75) * (1 + int/100)`
- The `ro_ground_effects.js` formula uses `INT/100` instead of `INT/35`, and `dex+75` instead of `DEX*(3+BaseLv/100)`. Both differ from deep research. The deep research formula includes BaseLv which is absent from the implementation.
- The Hunter skill audit MEMORY note (line for C2) references `(DEX/10+INT/2+SC*3+40)*2` which is the Blitz Beat formula, not trap formula.
- **No `TrapResearch` bonus** is applied in the implementation (the passive adds flat damage but it is not wired into `calculateTrapDamage`).

### Trap AoE Sizes

| Trap | Deep Research (34_Mount) | Deep Research (40_Ground) | Implementation | Status |
|------|-------------------------|--------------------------|---------------|--------|
| Land Mine | single target (line 455) | 1x1 | radius=25 (1x1) | MATCH |
| Ankle Snare | 3x3 trigger (line 479) | 1x1 trigger | radius=25 (1x1) | MATCH (1-cell trigger, affects 1 target) |
| Blast Mine | 3x3 (line 579) | 3x3 | radius=75 (3x3) | MATCH |
| Claymore | 5x5 (line 604) | 5x5 | radius=125 (5x5) | MATCH |
| Freezing Trap | 3x3 (line 555) | 3x3 | radius=75 (3x3) | MATCH |
| Sandman | **3x3** (line 515) | **5x5** (line 57) | radius=125 (**5x5**) | INTERNAL CONTRADICTION in deep research |
| Flasher | **3x3** (line 533) | **5x5** (line 58) | radius=75 (**3x3**) | INTERNAL CONTRADICTION in deep research |

**Note:** Document 34 says Sandman is 3x3 and Flasher is 3x3 in the trap type details. Document 40 says Sandman is 5x5 and Flasher is 5x5 in the overview table. iRO Wiki says Sandman is 5x5 splash and Flasher is 5x5 splash. The implementation has Sandman at 125 (5x5) matching iRO Wiki, but Flasher at 75 (3x3) which may be incorrect.

### Trap Duration Formulas

| Trap | Deep Research | Implementation | Match |
|------|-------------|---------------|-------|
| Land Mine | `240 - Lv*40`s (200/160/120/80/40) | `(6-lv)*40*1000`ms (200/160/120/80/40s) | MATCH |
| Ankle Snare | 250/200/150/100/50s | `(6-lv)*50*1000`ms (250/200/150/100/50s) | MATCH |
| Blast Mine | `30 - Lv*5`s (25/20/15/10/5) | `(30-5*lv)*1000`ms (25/20/15/10/5s) | MATCH |
| Claymore | 20/40/60/80/100s | `20*lv*1000`ms (20/40/60/80/100s) | MATCH |
| Freezing Trap | `180 - Lv*30`s (150/120/90/60/30) | `(6-lv)*30*1000`ms (150/120/90/60/30s) | MATCH |
| Shockwave Trap | `240 - Lv*40`s (200/160/120/80/40) | `(6-lv)*40*1000`ms (200/160/120/80/40s) | MATCH |
| Skid Trap | 300/240/180/120/60s | `(6-lv)*60*1000`ms (300/240/180/120/60s) | MATCH |
| Sandman | `180 - Lv*30`s (150/120/90/60/30) | `(6-lv)*30*1000`ms (150/120/90/60/30s) | MATCH |
| Flasher | `180 - Lv*30`s (150/120/90/60/30) | `(6-lv)*30*1000`ms (150/120/90/60/30s) | MATCH |

### Trap Status Effects vs Boss

| Trap | Deep Research Boss Behavior | Implementation | Match |
|------|----------------------------|---------------|-------|
| Ankle Snare | 1/5 duration for Boss | `snareDur / 5` for `bossProtocol` | MATCH |
| Land Mine | No stun, damage applies | `statusImmune` check before stun | MATCH |
| Sandman | No sleep for Boss | `statusImmune` check | MATCH |
| Flasher | No blind for Boss | `statusImmune` check | MATCH |
| Freezing Trap | No freeze, damage applies | `statusImmune` check before freeze | MATCH |
| Skid Trap | No pushback for Boss | `statusImmune` check before knockback | MATCH |

---

## Performance System (SP drain, movement, cancel conditions)

### Constants and Configuration

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Solo AoE | 7x7 (3.5 cell radius) | `SOLO_AOE_RADIUS = 175` (7x7) | MATCH |
| Ensemble AoE | 9x9 (4.5 cell radius) | `ENSEMBLE_AOE_RADIUS = 225` (9x9) | MATCH |
| Buff linger | 20 seconds after leaving | `EFFECT_LINGER_DURATION = 20000` | MATCH |
| Tick interval | Continuous | `TICK_INTERVAL = 1000` (1s) | MATCH |
| Caster excluded from own buff | Yes | Implemented (checked in tick loop) | MATCH |

### SP Drain Rates

| Skill | Deep Research | Implementation | Match |
|-------|-------------|---------------|-------|
| A Whistle | 1 SP / 5s | `a_whistle: 5000` | MATCH |
| Assassin Cross of Sunset | 1 SP / 3s | `assassin_cross_of_sunset: 3000` | MATCH |
| Poem of Bragi | 1 SP / 3s | `poem_of_bragi: 5000` | **DISCREPANCY** (5s vs 3s) |
| Apple of Idun | 1 SP / 3s | `apple_of_idun: 6000` | **DISCREPANCY** (6s vs 3s) |
| Humming | 1 SP / 5s | `humming: 5000` | MATCH |
| Please Don't Forget Me | 1 SP / 3s | `please_dont_forget_me: 10000` | **DISCREPANCY** (10s vs 3s) |
| Fortune's Kiss | 1 SP / 5s | `fortunes_kiss: 4000` | **DISCREPANCY** (4s vs 5s) |
| Service For You | 1 SP / 5s | `service_for_you: 5000` | MATCH |
| Ensemble skills | 1 SP / 3s each | 3000-5000ms (varies per ensemble) | PARTIALLY MATCHING |

**Analysis:** Several SP drain rates differ from the deep research. The deep research values come from iRO Wiki/RateMyServer guides. The implementation uses slower drain rates in several cases (Bragi 5s instead of 3s, Apple 6s instead of 3s, PDFM 10s instead of 3s). This makes performances less SP-costly in the current implementation. These may be intentional balance adjustments or inaccuracies.

### Movement Speed During Performance

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Formula | `BaseSpeed * (25 + 2.5 * MusicLessonsLv) / 100` | `(25 + 2.5 * lessonsLv) / 100` stored as multiplier | MATCH |
| Lv0 | 25% of normal | 25% | MATCH |
| Lv10 | 50% of normal | 50% | MATCH |

### Cancel Conditions

| Condition | Deep Research | Implementation | Match |
|-----------|-------------|---------------|-------|
| Adaptation (5s lockout) | Cannot use within first 5s | Checks `perfElapsed` (line 19239) | MATCH |
| Weapon swap | Cancels performance | `cancelPerformance` on weapon change (line 23212) | MATCH |
| Dispel | Cancels performance | `cancelPerformance` on Dispel (line 15348) | MATCH |
| Heavy damage (>25% MaxHP) | Cancels performance | `damage > player.maxHealth * 0.25` (line 1693) | MATCH |
| Silence | Does NOT cancel solo | Not in CC check list (line 27147) | MATCH |
| SP depletion | Cancels | `player.mana <= 0` check (line 27175) | MATCH |
| New performance | Cancels old | `cancelPerformance(..., 'new_performance')` (line 19001) | MATCH |
| Death | Cancels | `cancelPerformance(..., 'death')` (line 2050) | MATCH |
| Encore | Half SP, clears memory | Implemented with `lastPerformanceSkillId` (line 19297) | MATCH |
| Frost Joker/Scream/Pang Voice blocked during performance | BLOCKED | Checked via `AllowWhenPerforming` whitelist | MATCH |

### Song Overlap

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Two Bard songs overlap | Dissonance Lv1 | Implemented -- overlap detection in tick loop (line 27207-27228) | MATCH |
| Two Dancer dances overlap | Ugly Dance Lv1 | Implied in overlap system | MATCH |
| Bard + Dancer overlap | No conflict | `performanceType` check skips different types (line 27218) | MATCH |

---

## Ensemble System (midpoint, requirements)

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Required: Bard + Dancer | Same party, adjacent | Party check, class check, adjacency check | MATCH |
| Midpoint calc | `avg(positions)` | `centerX = (player.lastX + partner.lastX) / 2` (line 19104) | MATCH |
| AoE | 9x9 stationary | `ENSEMBLE_AOE_RADIUS = 225`, stationary | MATCH |
| Skill level | `min(BardLv, DancerLv)` | `Math.min(learnedLevel, partnerLevel)` (line 19091) | MATCH |
| Movement locked | Cannot move | `ensembleState` prevents movement | MATCH |
| Musical Strike/Slinging Arrow | Usable during ensemble | Checked in performance-blocked skill list | MATCH |
| SP drain | Both performers drain | Both drain at `drainInterval` | MATCH |
| Either SP depleted | Ends ensemble | Checked in ensemble tick | MATCH |
| Aftermath debuff | 10s debuff on both | `ensemble_aftermath` buff (10000ms, noSkills + moveSpeedReduction 50%) | MATCH |
| Party immunity (Lullaby) | Caster's party immune | `casterPartyId` filter (line 1365) | MATCH |

### Ensemble Skills

| Skill | Deep Research | Implementation | Match |
|-------|-------------|---------------|-------|
| Lullaby | Sleep enemies, party immune | `sleep_aoe` type, 6s tick, party immunity | MATCH |
| Mr. Kim A Rich Man | +EXP 20-60% | `exp_bonus` type, `ensemble_mr_kim` buff | MATCH |
| Eternal Chaos | DEF to 0 | `vit_def_zero` type, `_ensembleVitDefZero` flag | MATCH |
| Drum on Battlefield | +ATK, +DEF | `atk_def_buff` type, `ensemble_drum` buff | MATCH |
| Ring of Nibelungen | Lv4 weapon ATK bonus | `lv4_weapon_atk` type, `ensemble_nibelungen` buff | MATCH |
| Loki's Veil | Prevent skill use | `skill_block` type | MATCH |
| Into the Abyss | No gemstone costs | `no_catalysts` type, `ensemble_into_abyss` buff | MATCH |
| Inv. Siegfried | Element + status resist | `element_status_resist` type, `ensemble_siegfried` buff | MATCH |

---

## Mount System (speed, weight, ASPD)

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| `/mount` command | Toggle mount | `player.isMounted = !player.isMounted` (line 7893) | MATCH |
| Speed bonus | ~+25% (equiv. to Inc AGI) | `1.36` multiplier (~+36%) (line 4203) | **DISCREPANCY** (36% vs 25%) |
| Weight bonus | +1,000 | `maxW += 1000` (line 4750) | MATCH |
| ASPD penalty base | 50% of normal | `mountMultiplier = 0.5 + cavalierMasteryLv * 0.1` | MATCH |
| Cavalier Mastery recovery | +10% per level, Lv5 = full | `0.5 + lv * 0.1`, `min(1, multiplier)` | MATCH |
| Spear vs Medium | 100% when mounted | `isMounted` passed to damage formula | MATCH |
| Spear Mastery | +5 ATK/lv mounted (vs +4) | `perLevel = player.isMounted ? 5 : 4` (line 786) | MATCH |
| Brandish Spear requires mount | Yes | `if (!player.isMounted)` check (line 13298) | MATCH |
| Mount type visual | Peco Peco (Knight) vs Grand Peco (Crusader) | `mountType` distinguishes by `jobClass` (line 7902) | MATCH |
| Class restriction | Knight/LK + Crusader/Paladin | Riding skill check (passive 708) | MATCH |
| CC check | Cannot mount while stunned/frozen/stoned | `isFrozen || isStoned || isStunned` check (line 7887) | MATCH |
| Stats update after toggle | ASPD, speed | `buildFullStatsPayload` sent after toggle (line 7911) | MATCH |
| Mount persistence | Saved through map changes | `isMounted` in player state (line 5657), **not persisted to DB** | **GAP** |
| Dismount overweight handling | Inventory may exceed limit | No explicit overweight check on dismount | **GAP** |
| Indoor restrictions | None in pre-renewal | None implemented (correct) | MATCH |

**Speed discrepancy analysis:** Deep research says approximately +25% (equivalent to Increase AGI). The implementation uses 1.36x (~+36%). This was noted in the gap analysis of document 34 (line 1011) as "slightly higher than reference (+25% / Increase AGI)". The session note for 2026-03-20 records this as an intentional deviation for gameplay feel.

---

## Falcon System (Blitz Beat, Auto Blitz)

### Blitz Beat (Manual Cast)

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Requires falcon | `hasFalcon` check | `if (!player.hasFalcon)` (line 18008) | MATCH |
| Hit count | = skill level (1-5) | `numHits = learnedLevel` (line 18027) | MATCH |
| Damage type | MISC (ignores DEF/MDEF) | Direct damage application, no DEF check | MATCH |
| Element | Neutral | `'neutral'` (line 18029) | MATCH |
| 3x3 splash | Full damage to each target | Splash loop at line 18048, full damage per target | MATCH |

**Blitz Beat formula comparison:**

| Source | Formula |
|--------|---------|
| Deep Research | `DamagePerHit = 80 + (SteelCrow*6) + INT + Floor(DEX/5)` |
| Deep Research (simplified) | `80 + SteelCrow*6 + INT + DEX/5` |
| Implementation (line 18026) | `(Floor(DEX/10) + Floor(INT/2) + SteelCrow*3 + 40) * 2` |
| Expanded impl. | `DEX/5 + INT + SteelCrow*6 + 80` (after multiplying each term by 2) |

**Analysis:** The formulas are **equivalent**. `(DEX/10 + INT/2 + SteelCrow*3 + 40) * 2` = `DEX/5 + INT + SteelCrow*6 + 80`. The deep research and implementation agree after expansion.

### Auto Blitz Beat

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Trigger chance | `Floor(LUK/3)` % | `Math.floor(abLuk / 3)` (line 25122/25666) | MATCH |
| Trigger on bow only | Normal bow attacks only | `attacker.weaponType === 'bow'` (line 25118/25662) | MATCH |
| Triggers on miss | Yes | Implemented in miss path (line 25118) | MATCH |
| Triggers after kill | Yes | Implemented after damage (line 25659-25661) | MATCH |
| No SP cost | Yes | No SP deduction in auto path | MATCH |
| Hit count | `min(BB_Lv, Floor((JobLv+9)/10))` | `Math.min(abBlitzLv, Math.min(5, Math.floor((jobLv + 9) / 10)))` | MATCH |
| Damage split | Split by target count | `abTotalBase / abTargets.length` (line 25686) | MATCH |
| Manual: no split | Full damage to each | Full damage at line 18030-18031 | MATCH |

### Steel Crow

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Bonus | +6 per level per hit | `steelCrowLv * 3 + 40) * 2` = `SteelCrow*6 + 80` | MATCH (included in formula) |

### Detect Skill

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Reveals hidden enemies | 3x3 area | Implemented: 7x7 radius (175 UE) at line 17985 | **DISCREPANCY** (7x7 vs 3x3) |
| Requires falcon | Yes | `if (!player.hasFalcon)` (line 17951) | MATCH |

### Falcon Rental

| Feature | Deep Research | Implementation | Match |
|---------|-------------|---------------|-------|
| Rental NPC (2,500z) | Hunter Guild NPC | **NOT implemented** -- falcon auto-set on Falconry Mastery learn (line 8255) | DIFFERENT |
| Falcon Pipes (10,000z) | Consumable item | **NOT implemented** | MISSING |
| `hasFalcon` flag | Saved state | `hasFalcon` set on skill learn and player join (line 5696) | Functional (auto-grant) |

---

## Critical Discrepancies

### D1: Trap Item Costs -- Freezing Trap and Blast Mine Should Cost 2 Traps
- **Location:** `server/src/index.js` line 17818
- **Current:** `(skill.name === 'claymore_trap' || skill.name === 'shockwave_trap') ? 2 : 1`
- **Should be:** `(skill.name === 'claymore_trap' || skill.name === 'shockwave_trap' || skill.name === 'freezing_trap' || skill.name === 'blast_mine') ? 2 : 1`
- **Impact:** Freezing Trap and Blast Mine cost half the intended trap items, making them too cheap.

### D2: Trap Damage Formulas Differ from Deep Research
- **Location:** `server/src/ro_ground_effects.js` lines 556-576
- **Land Mine:** Implementation uses `lv*(dex+75)*(1+int/100)`. Research says `DEX*(3+BaseLv/100)*(1+INT/35)*Lv`. The key differences: `INT/100` vs `INT/35` (INT scaling is ~3.5x weaker in implementation), and `dex+75` vs `DEX*(3+BaseLv/100)` (BaseLv scaling is missing). Both sources are rAthena-based but clearly different formulas.
- **Blast Mine:** Implementation uses `lv*(50+dex/2)*(1+int/100)`. Research says `DEX*(2+BaseLv/100)*(1+INT/35)*Lv`.
- **Claymore:** Implementation uses `lv*(75+dex/2)*(1+int/100)`. Research says `DEX*(3+BaseLv/85)*(1.1+INT/35)*Lv`. Claymore should use the enhanced formula (`BaseLv/85` and `1.1`).
- **Impact:** Trap damage scaling differs significantly at high INT and high BaseLv. The implementation produces lower damage at INT 105+ and higher BaseLv.
- **Note:** No `TrapResearch` bonus (+40/lv) is added to trap damage in the implementation.

### D3: Flasher AoE May Be Wrong
- **Location:** `server/src/index.js` line 17888 (`trapRadius = 75` = 3x3)
- **Deep Research (40_Ground, line 58):** Flasher is 5x5 splash.
- **Deep Research (34_Mount, line 533):** Flasher is 3x3 (contradicts doc 40).
- **iRO Wiki:** Flasher is 5x5 splash area.
- **Impact:** If iRO Wiki is correct, Flasher should be radius=125 (5x5), not 75 (3x3).

### D4: Detect Skill AoE Too Large
- **Location:** `server/src/index.js` line 17985 (`detectRadius = 175` = 7x7)
- **Deep Research:** 3x3 area around target cell
- **Impact:** Detect reveals hidden players in an area 5.4x larger than intended.

### D5: SP Drain Rates Differ for 3 Performance Skills
- **Location:** `server/src/index.js` lines 940-951
- Poem of Bragi: 5000ms (impl) vs 3000ms (research) -- 67% slower drain
- Apple of Idun: 6000ms (impl) vs 3000ms (research) -- 100% slower drain
- Please Don't Forget Me: 10000ms (impl) vs 3000ms (research) -- 233% slower drain
- **Impact:** These performances are significantly cheaper to maintain than in RO Classic, affecting balance. May be intentional for gameplay.

### D6: Mount Speed 36% vs 25%
- **Location:** `server/src/index.js` line 4203 (`1.36` divisor)
- **Deep Research:** ~+25% (equivalent to Increase AGI)
- **Implementation:** ~+36%
- **Impact:** Mounted knights/crusaders move ~9% faster than intended. Previously noted as intentional.

---

## Missing Features

### M1: Trap Placement Restrictions (2-cell minimum from entities)
Not enforced. Traps can be placed directly under enemies or other traps. This enables trap stacking exploits.

### M2: Trap Durability (3,500 HP, destructible)
Traps cannot be attacked or destroyed by damage. No counterplay against placed traps except Land Protector.

### M3: Trap Arming Delay (1-2 seconds)
Traps activate instantly on placement. Should have a brief arming delay before they can trigger.

### M4: Trap Item Return on Expiry
When traps expire without being triggered, the Trap item is consumed and lost. RO Classic returns the item to the caster.

### M5: Arrow Shower Trap Pushing
Arrow Shower does not interact with traps. In RO Classic, Arrow Shower pushes traps in the knockback direction, enabling the core Hunter tactic of pushing traps under monsters.

### M6: Mount Persistence Through Map Changes
`isMounted` flag exists on the player state object but is not saved to the database. On disconnect/reconnect or zone transition, mount state may be lost.

### M7: Dismount Overweight Handling
When dismounting, the +1000 weight bonus is removed but there is no check for whether the player's inventory now exceeds their (reduced) max weight. This could leave the player in an inconsistent state.

### M8: Falcon Rental NPC / Falcon Pipes
Falcon is auto-granted on learning Falconry Mastery (line 8255). No NPC rental or consumable Falcon Pipes item. This simplification is acceptable for early development but differs from RO Classic's economy gate (2,500z rental or 10,000z Falcon Pipe).

---

## Recommended Fixes

### Priority 1 (Formula/Cost Accuracy)

1. **Fix Freezing Trap and Blast Mine trap item cost to 2** -- Single-line fix at line 17818. Add `|| skill.name === 'freezing_trap' || skill.name === 'blast_mine'` to the cost=2 condition.

2. **Fix trap damage formulas** -- Update `calculateTrapDamage()` in `ro_ground_effects.js` to match the deep research formulas that include `BaseLv` scaling and `INT/35` instead of `INT/100`. Add `TrapResearch` bonus (+40/lv) parameter. This requires passing BaseLv and TrapResearchLv to the function.

3. **Fix Flasher AoE** -- Change `trapRadius` from 75 to 125 at line 17888 (if iRO Wiki 5x5 is authoritative).

4. **Fix Detect skill AoE** -- Change `detectRadius` from 175 (7x7) to 75 (3x3) at line 17985.

### Priority 2 (Gameplay Impact)

5. **Add trap placement restrictions** -- Enforce 2-cell minimum distance from entities before placing traps.

6. **Add trap item return on expiry** -- When a trap expires at line 31181, return the Trap item(s) to the caster's inventory.

7. **Review SP drain rates** -- Decide whether the current slower drain rates are intentional balance adjustments or should match the deep research. Document the decision.

8. **Add mount DB persistence** -- Save `isMounted` to the characters table so mount state survives reconnection.

### Priority 3 (Feature Completeness)

9. **Add trap durability** -- Give traps 3,500 HP and allow them to be targeted/attacked.

10. **Add Arrow Shower trap pushing** -- When Arrow Shower hits a cell with a trap, push the trap in the knockback direction.

11. **Add trap arming delay** -- 1-2 second delay between placement and activation.

12. **Add dismount overweight check** -- On dismount, check if weight exceeds new limit and apply overweight penalties.

13. **Consolidate ground effect systems** -- The `ro_ground_effects.js` foundation module is comprehensive but only used for `calculateTrapDamage()`. Either migrate `index.js` inline effects to use the foundation module, or remove the unused module to reduce confusion.

### Priority 4 (Future/Visual)

14. **Mount visual sprite** -- Client-side mounted animation and model swap (requires art).

15. **Falcon visual** -- Shoulder perch sprite and attack animation (requires art).

16. **Falcon rental NPC** -- Add NPC with 2,500z cost and Falcon Pipe consumable item.

17. **Trap visibility system** -- Hidden traps in PvP/WoE (future feature).

---

*Audit generated by comparing `34_Mount_Falcon_Trap_Performance.md` and `40_Ground_Effects_AoE.md` against server source code (`index.js`, `ro_ground_effects.js`, `ro_damage_formulas.js`). All line numbers reference the codebase as of 2026-03-22.*
