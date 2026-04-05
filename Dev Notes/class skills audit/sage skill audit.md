# Sage Skill Audit

claude --resume 65e95f94-ba6a-4a69-b5bb-d9a2c5517f08


  All 25 Fixes Applied

  Files Modified (4 files)

  ro_skill_data_2nd.js — 7 data fixes:
  - Hindsight duration: 60000+i*30000 → 120000+i*30000 (120s-390s)
  - Dispell cooldown: 2000 → 0
  - Magic Rod duration: 200+i*200 → 400+i*200
  - Land Protector range: 150 → 100 (2 cells)
  - Abracadabra cooldown: 3000 → 0
  - Earth Spike Sage ACD: 0 → 1000+i*200
  - Heaven's Drive Sage ACD: 0 → 1000

  ro_buff_system.js — 1 fix:
  - Deluge MaxHP: bonusMaxHp → bonusMaxHpPercent (percentage not flat)

  ro_damage_formulas.js — 3 additions to both damage functions:
  - Elemental damage boost (fire/water/wind) from Sage zones applied to final damage
  - Dragonology race resist reduces incoming Dragon damage
  - Dragonology magic ATK bonus (+2%/lv vs Dragons) in magical damage

  index.js — 12 fixes:
  1. Dragonology INT: [1,1,1,2,3] → [1,1,2,2,3] + added raceMATK.dragon
  2. Advanced Book ASPD: bookAspdBonus = abLv * 0.5
  3. getEffectiveStats(): added passiveRaceResist and passiveRaceMATK
  4. Volcano ATK: [10,15,20,25,30] → [10,20,30,40,50]
  5. Heaven's Drive MATK%: 125 → 100 (pre-renewal)
  6. Spell Breaker: Lv5 damage moved inside targetCast check + Magic Rod counter
  7. Hindsight spell pool: rewritten to pre-renewal (Napalm Beat at Lv1, no ES/TS/HD, per-level unlock)
  8. Dispell: UNDISPELLABLE expanded (13 new entries) + fallback formula fixed
  9. Endow expiry: weaponElement reverts to baseWeaponElement when endow expires
  10. Zone stat bonuses: element-restricted (ATK/MaxHP/FLEE only for matching element)
  11. LP vs LP: mutual destruction when overlapping
  12. checkMagicRodAbsorption(): utility function ready for PvP magic paths

---
> **Navigation**: [Documentation Index](DocsNewINDEX.md)
