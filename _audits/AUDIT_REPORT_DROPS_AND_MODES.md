# Sabri_MMO Drop Table + AI Mode Audit Report

**Date:** 2026-05-10
**Source of truth:** rAthena `db/pre-re/mob_db.yml` master + `db/pre-re/item_db_*.yml` master (cloned locally)
**Coverage:** 509 / 509 of our monster templates audited

---

## TL;DR

- **AI mode bitmasks:** 509/509 perfect match (100%) — 1 monster fixed (Skeleton AI 4 → 17)
- **Drop tables:** 32 HIGH-severity findings (2 typos fixed, 9 missing card variants, ~23 extreme rate divergences); 399 MEDIUM (missing items canonical drops); 376 LOW (extra items we drop that canonical doesn't)
- **Real runtime bugs found and fixed:** typo "Adventurere's Suit" (3 e's) → "Adventurer's Suit" — was failing item lookup at 2 spawn sites

---

## AI Mode Bitmask Audit — COMPLETE (100%)

### Methodology
Built `_audits/audit_monster_modes.js` that:
1. Loads canonical Ai code (1-27) + Class (Boss/Guardian/Battlefield) + Modes block from rAthena YAML
2. Resolves to a hex bitmask via the documented `mob_db_mode_list.txt` mapping
3. Loads our template's resolved flags (MONSTER_AI_CODES → AI_TYPE_MODES → boss/mvp class additions → template.modes overlay)
4. Diffs flag-by-flag

### Audit script bugs found and fixed during this audit
- Default-when-Ai-null was wrong → fixed to use 0x0000 (no AI = no flags) for plants/eggs/decorations
- Missing canonical `Class:` field handling → added Boss/Guardian/Battlefield class-add bitmasks per rAthena docs

### Real bugs found in our data
**1 monster fix applied:**
- **Skeleton (1076)** AI code 4 (Angry/Aggressive) → **17** (Passive + CastSensorIdle). Was making Skeletons aggressively chase players who walked by; canonical Skeleton only retaliates when attacked or when you cast on it.

### Final result
```
Audited monsters:    509
Perfect flag match:  509
Total flag diffs:    0
```

---

## Drop Table Audit — Status Report (32 HIGH findings)

### Methodology
Built `_audits/audit_monster_drops.js` V2 (item-id matching, not name matching) that:
1. Resolves our drop's `itemName` → canonical item ID via display-name lookup (with apostrophe-strip fallback)
2. Resolves canonical `Item: Aegis_Name` → item ID via aegisToId map
3. Compares both sides by item ID — bypasses display-name divergence noise

### V1 vs V2
V1 matched by display name and reported 151 missing + 151 extra (mostly false positives from name divergences like "Animal Skin" vs "Animal's Skin"). V2 catches the real divergences by ID.

### Real runtime bug fixed
**Typo "Adventurere's Suit" (3 e's)** → "Adventurer's Suit" at 2 spawn sites (mobs 1107 Baby Desert Wolf and 1653 Wickebine Tres). Was silently failing item lookup at runtime — players couldn't actually get this drop.

### Findings by severity

| Severity | Count | Categories |
|----------|-------|-----------|
| CRITICAL | 0 | — |
| HIGH | 32 | 9 missing card variants + 23 extreme (>50% off) rate divergences |
| MEDIUM | 399 | Items canonical drops that we don't (or with mild rate divergence) |
| LOW | 376 | Items we drop that canonical doesn't (intentional content additions) |

### Detailed HIGH findings

**Missing variant cards (9):** these card items exist in canonical mob_db but our templates don't drop them. May need to verify the card item exists in our items DB before adding the drop.

| Mob | Card | Canon ID | Notes |
|-----|------|----------|-------|
| 1050 Picky | Picky Egg Card | 4011 | Variant card, same name as 1049 Picky's |
| 1062 Santa Poring | Santa Poring Card | 4005 | |
| 1101 Baphomet Jr. | Bapho Jr. Card | 4129 | |
| 1156 Petite | Sky Petite Card | 4120 | |
| 1241 Picky | Picky Egg Card | 4011 | (variant of 1050) |
| 1716 Acidus | Blue Acidus Card | 4379 | |
| 1717 Ferus | Green Ferus Card | 4381 | |
| 1718 Novus | Yellow Novus Card | 4382 | |
| 1789 Iceicle | Ice Cubic | 7066 | |
| 1840 Golden Savage | New Year Rice Cake | 12239 | |

**Extreme rate divergences (>50% off canonical):** see `node _audits/audit_monster_drops.js --severity=HIGH` for the full 23. Notable examples:
- Poring "Apple": ours 1.5%, canon 10% (we have a duplicate-Apple entry pattern that's confusing)
- Drainliar "Wing of Red Bat": ours 15%, canon 55%
- Baphomet "Crescent Scythe": ours 0.5%, canon 4%
- Skeggiold "Rune of Darkness": ours 10%, canon 60%
- Agav/Echio "Bloody Rune": ours 1%, canon 40%

Auto-fix attempted but skipped — many monsters have **duplicate drops with same name but different rates** (e.g., Poring drops "Apple" twice — once at 10% and once at 1.5%). The auto-fix regex can't distinguish duplicates safely. These need manual review.

### MEDIUM findings (399 missing drops — mostly content gaps)

These are canonical drops for items that exist in the canonical item DB but we don't include. Examples:
- Wolf (1013) is missing the canonical "Animal's Cape" drop (rare equipment)
- Many monsters are missing low-rate (<1%) crafting material drops
- Many MVPs are missing canonical loot variants

These are NOT necessarily bugs — they're content choices. A server can intentionally simplify drop tables.

### LOW findings (376 extras — intentional or wrong-item-id)

These are drops in our templates that don't appear in canonical for that monster. Two patterns:
1. **Intentional additions** — we added drops for our own server flavor
2. **Wrong-slot equipment** — canonical drops slotted variants (`Knife_` id 1202) but we drop unslotted (`Knife` id 1201). Both have display name "Knife" so it's easy to confuse. ~50 of these across the dataset.

Examples:
- Poring extra "Knife" id 1201 — should be "Knife [3]" id 1202 (slotted version is canonical)
- Willow extra "Wooden Block" id 7850 — should be "Trunk" id 1019 (different name, same canonical drop)

---

## Files Created This Session

| File | Purpose |
|------|---------|
| `_audits/extract_canonical_mob_db.js` | Updated to extract Drops + Class + Modes from YAML |
| `_audits/extract_canonical_item_db.js` | NEW — extracts 6,169 items from rAthena item_db_*.yml |
| `_audits/canonical/item_db_pre_re_full.json` | NEW — 6,169 items with aegisName ↔ displayName ↔ id |
| `_audits/canonical/mob_db_pre_re_full.json` | Updated — now includes drops, class, modes |
| `_audits/audit_monster_drops.js` | NEW V2 — item-id matching, distinguishes wrong-slot variants |
| `_audits/audit_monster_modes.js` | NEW — flag-by-flag bitmask diff with severity weighting |
| `_audits/apply_drop_fixes.js` | NEW — auto-fix for HIGH rate divergences (currently --dry-run only due to duplicate-name complexity) |
| `_audits/AUDIT_REPORT_DROPS_AND_MODES.md` | This report |

## Files Modified

- `server/src/ro_monster_ai_codes.js` — Skeleton AI 4 → 17
- `server/src/ro_monster_templates.js` — typo "Adventurere's Suit" fixed at 2 sites

---

## Recommended Fix Plan

### Tier 1 — Apply now (auto-fixable, safe)
1. ✅ **Skeleton AI mode** — done
2. ✅ **Adventurere typo** — done
3. ⏳ **Add missing variant cards** (9) — needs verification each card exists in items DB; then ~10 manual edits

### Tier 2 — Manual review (auto-fix risky)
1. **Duplicate-drop rate cleanup** — for monsters like Poring that have two "Apple" entries, decide if both should exist and at what rates
2. **Wrong-slot equipment** — convert ~50 unslotted drops to their slotted variants if matching canonical (e.g., Poring "Knife" id 1201 → "Knife [3]" id 1202)
3. **23 extreme rate divergences** — review case-by-case, decide if intentional rebalance or bug

### Tier 3 — Defer (content judgment)
1. **399 MEDIUM missing drops** — decide which canonical drops to add for completeness
2. **376 LOW extra drops** — keep as intentional server-flavor content

---

## Reproducibility

```bash
# Setup
git clone --depth 1 https://github.com/rathena/rathena.git /tmp/rathena
node _audits/extract_canonical_mob_db.js
node _audits/extract_canonical_item_db.js

# Audits
node _audits/audit_monster_modes.js                          # 0 findings expected
node _audits/audit_monster_drops.js                          # 32 HIGH expected
node _audits/audit_monster_drops.js --severity=HIGH          # detailed HIGH list
node _audits/audit_monster_drops.js --id=1002                # focus on Poring

# Auto-fixes (drops only — modes already clean)
node _audits/apply_drop_fixes.js --dry-run                   # preview rate fixes (currently broken on duplicates)
```
