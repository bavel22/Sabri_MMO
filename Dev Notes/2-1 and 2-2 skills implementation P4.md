# 2-1 and 2-2 skills implementation Phase 4

2-1 and 2-2 skills implementation Phase 4

claude --resume 306eb5b5-c8e2-4a9f-af9e-c8a928fdbae4

  Phase 4 — Performance classes (shared system):
Do these one by one in order.
  - 4A: Performance system core (shared Bard/Dancer)
  - 4B: All Bard songs + Dancer dances

   ---

     ## PHASE 4: Performance Classes

     ---

     ### Prompt 4A: Performance System Core (Shared Bard/Dancer)

     **Dependencies:** Phase 0A, 0B (ground effects)

     ```
     Load skills: /sabrimmo-skills, /full-stack, /realtime, /sabrimmo-buff

     Read BOTH research documents:
     - docsNew/05_Development/Bard_Class_Research.md (Performance System Architecture section)
     - docsNew/05_Development/Dancer_Class_Research.md (Performance System Architecture section)
     Read server/src/index.js

     Implement the PERFORMANCE SYSTEM — shared infrastructure for both Bard and Dancer:

     PERFORMANCE STATE:
     1. Add to player object:
        - player.performanceState = null (or { skillId, skillLevel, startedAt, spDrainInterval })
        - player.lastPerformanceSkillId = null (for Encore)
        - player.performanceType = null ('song' for Bard, 'dance' for Dancer)

     2. Performance activation flow:
        a. Player uses song/dance skill
        b. Validate: instrument equipped (Bard) or whip equipped (Dancer)
        c. Validate: not already performing another song/dance
        d. Validate: SP sufficient for initial cost + drain
        e. Set performanceState
        f. Create ground effect (7x7 AoE centered on caster) with type: 'song'
        g. Broadcast `performance:start` event to zone

     3. Performance tick (integrate into server tick loop or ground effect tick):
        a. Every SP drain interval: drain SP from performer
        b. If performer runs out of SP: cancel performance
        c. Every effect tick: apply buff/debuff to entities in 7x7 AoE
        d. Ground effect follows caster (moves with them, unlike static ground effects)
        e. Movement speed reduction while performing: base 50%, improved by Music/Dancing Lessons

     4. Performance cancellation:
        a. Use Adaptation to Circumstances skill
        b. Swap to dagger (weapon change)
        c. Run out of SP
        d. Get hit by certain CC effects (Stun, Freeze, etc.)
        e. Die
        f. On cancel: remove ground effect, clear performanceState
        g. Broadcast `performance:stop` event

     5. Buff linger:
        - When ally leaves song/dance AoE, buff persists for 20 seconds
        - After 20s, buff fades
        - Implementation: when entity exits AoE, set a linger timer on their buff

     6. Stacking rules:
        - Only ONE song/dance can affect a player at a time
        - If player enters area of two songs, most recently entered takes priority
        - Same performer cannot stack multiple songs (only one active at a time)

     7. Socket.io events:
        - `performance:start` { characterId, skillId, position, radius }
        - `performance:stop` { characterId }
        - `performance:effect` { characterId, skillId, affectedIds, buffType } (per tick)

     COMMON SKILLS (both Bard and Dancer have these):
     8. **Adaptation to Circumstances** — Cancel current performance
        - Simple: clear performanceState, remove ground effect
        - 1 SP cost

     9. **Encore** — Replay last song/dance at half SP
        - Uses lastPerformanceSkillId
        - Casts same skill but with 50% SP cost
        - 10s cooldown from research doc

     Do NOT implement individual songs/dances yet — just the performance infrastructure.
     ```

     ---

     ### Prompt 4B: Bard + Dancer — All Songs and Dances

     **Dependencies:** Prompt 4A (performance system core)

     ```
     Load skills: /sabrimmo-skills, /sabrimmo-combat, /full-stack, /realtime, /sabrimmo-buff, /sabrimmo-debuff

     Read BOTH research documents:
     - docsNew/05_Development/Bard_Class_Research.md
     - docsNew/05_Development/Dancer_Class_Research.md
     Read server/src/index.js for performance system from Prompt 4A

     Implement ALL individual Bard and Dancer skills:

     BARD SONGS (use performance system, apply buffs to allies in AoE):
     1. **A Whistle** — FLEE + Perfect Dodge buff
        - FLEE = SkillLv + AGI/10 + MusicLessons*0.5
        - PD = floor((SkillLv+1)/2) + LUK/30 + MusicLessons*0.2

     2. **Assassin Cross of Sunset** — ASPD buff
        - ASPD = MusicLessons/2 + AGI/20 + SkillLv + 5
        - Integrate with ASPD calculation

     3. **A Poem of Bragi** — Cast time + after-cast delay reduction
        - VCT reduction = SkillLv*3 + DEX/10 + MusicLessons
        - ACD reduction = (Lv<10 ? Lv*3 : 50) + INT/5 + MusicLessons*2
        - Hook into cast time and afterCastDelay calculations

     4. **Apple of Idun** — MaxHP buff + HP regen
        - MaxHP% = 5 + SkillLv*2 + VIT/10 + MusicLessons/2
        - HP per tick = 30 + SkillLv*5 + VIT/2 + MusicLessons*5

     5. **Humming** — HIT buff (shared with Dancer)
        - HIT = SkillLv*2 + DEX/10 + MusicLessons

     6. **Fortune's Kiss** — CRIT buff (shared with Dancer)
        - CRIT = SkillLv + LUK/10 + MusicLessons

     7. **Service for You** — SP reduction (shared with Dancer)
        - MaxSP% = 15 + SkillLv + INT/10
        - SP cost reduction = 20 + SkillLv*3 + INT/10 + ceil(MusicLessons/2)

     8. **Dissonance** — Damage to enemies in AoE
        - Damage per 3s tick: 30 + 10*SkillLv + MusicLessons*3

     BARD OFFENSIVE:
     9. **Musical Strike** — Instrument attack
        - 150-250% ATK per level, requires instrument
        - Usable during performance (does not cancel song)

     10. **Frost Joker** — AoE freeze chance
         - (15 + 5*SkillLv)% freeze chance
         - Screen-wide (or large AoE), affects everyone INCLUDING allies
         - Cancels current performance when used

     DANCER DANCES (use performance system, apply debuffs to enemies in AoE):
     11. **Please Don't Forget Me** — ASPD + move speed debuff
         - ASPD reduction = (SkillLv*3) + DEX/10 + DancingLessons
         - Move speed reduction = (SkillLv*2) + AGI/10 + ceil(DancingLessons/2)
         - Debuff applied to enemies in AoE

     12. **Ugly Dance** — SP drain on enemies
         - SP drain per tick from research doc formula

     13. **Scream / Dazzler** — AoE stun chance on enemies
         - Stun chance per level from research doc

     14. **Moonlit Water Mill** — Block self-buff songs (if in research doc)

     DANCER OFFENSIVE:
     15. **Slinging Arrow / Throw Arrow** — Whip attack
         - ATK% per level, requires whip
         - Usable during performance

     16. **Charming Wink** — Provoke/taunt (quest skill)
         - Forces target to attack Dancer

     PASSIVES:
     17. **Music Lessons (Bard)** — +3 ATK per level with instrument, improves song effects
         - Add to getPassiveSkillBonuses()

     18. **Dancing Lessons (Dancer)** — +3 ATK per level with whip, improves dance effects
         - Add to getPassiveSkillBonuses()

     ENSEMBLE SKILLS (defer until party system exists):
     19. Mark these as TODO in code comments:
         - Lullaby, Mr. Kim A Rich Man, Eternal Chaos, Drum on Battlefield
         - Ring of Nibelungen, Loki's Veil, Into the Abyss, Invulnerable Siegfried
         - Require: 1 Bard + 1 Dancer in same party, adjacent cells
         - 9x9 AoE (larger than solo), average both performers' skill levels
         - Leave stub handlers with TODO comments referencing research docs
     ```


 Audit Fixes Applied

  Critical (6):

  1. Apple of Idun heal counter — moved _healCounter increment outside ally loop so healing fires every 6s regardless of
   party size
  2. Dissonance tick rate — added _effectTickCounter, damage now fires every 3 ticks (3s) instead of every 1s
  3. Assassin Cross ASPD formula — changed skillLevel + 5 to 10 + skillLevel (canonical: 10 + lv + AGI/20 + ML/2)
  4. Pang Voice baseChance — 5000 → 100 (guaranteed confusion)
  5. Charming Wink baseChance — 7000 → 70 (70% success rate)
  6. Ugly Dance — confirmed correct for PvE (monsters have no SP). PvP drain deferred.

  Moderate (5):

  7. PDFM prerequisite — Dance Lessons Lv3 → Ugly Dance Lv3 (skillId 1525)
  8. Moonlit Water Mill SP drain — flat 1 → 4 * skillLevel per tick via spDrainAmount field
  9. Frost Joker + Scream — added player loop (affects ALL on screen including self/party)
  10. Encore — stores lastPerformanceLevel in startPerformance(), used in Encore handler
  11. Dance buff naming — renamed song_humming/fortune_kiss/service_for_you → dance_humming/fortune_kiss/service_for_you
   in BUFF_MAP, BUFF_TYPES, and getBuffModifiers

  Minor (5):

  12. Adaptation 5-second window — blocks cancel in first/last 5 seconds of performance
  13. Adaptation/Encore ACD — all 4 skills (1503/1504/1523/1524) now have afterCastDelay: 300
  14. Charming Wink castTime/ACD — castTime: 1000, afterCastDelay: 1000
  15. Charming Wink race restriction — only affects Demi-Human, Angel, Demon races
  16-17. Comment fixes — corrected freeze/stun chance range descriptions



---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
