# Card Naming System — Full Implementation Plan

> **Navigation**: [Documentation Index](DocsNewINDEX.md) | [Card_System](../03_Server_Side/Card_System.md) | [Inventory_System](../03_Server_Side/Inventory_System.md)

**Status**: IMPLEMENTED
**Date**: 2026-03-13
**Systems touched**: Database, Migration Script, Server (index.js), Client (CharacterData.h)

---

## 1. How RO Classic Card Naming Works

### 1.1 Core Concept — Client-Side Name Construction

The card naming system is **purely visual / client-side**. The server stores only:
- Base item ID (e.g., 1402 = Blade)
- `compounded_cards` JSONB array (e.g., `[4035, 4035, 4035, 4092]`)
- Refine level

The **client** constructs the display name at render time using two data sources:
1. **Card prefix/suffix text** — each card has a naming word (e.g., Hydra → "Bloody", Verit → "of Counter")
2. **Prefix vs suffix flag** — determines whether the text goes before or after the base item name

The server never stores or sends a pre-computed item name. The client builds:
```
[+RefineLevel] [Multiplier] [Prefix1] [Multiplier] [Prefix2...] BaseName [Multiplier] [Suffix1...] [TotalSlots]
```

### 1.2 Prefix vs Suffix — Per-Card, NOT Per-Equipment-Type

Each card has a **fixed** designation as either prefix or suffix. This is per-CARD, not per-equipment-slot. The same card always produces the same prefix or suffix regardless of what equipment it goes into.

**Convention:**
- **Prefix cards**: standalone word placed BEFORE item name — "Bloody", "Boned", "Cranial", "Immune", "Titan"
- **Suffix cards**: phrase starting with "of" placed AFTER item name — "of Counter", "of Hermes", "of Ghost", "of Warmth"

In the original RO client, this is determined by two files:
- `cardprefixnametable.txt` — maps every card ID to its naming text
- `cardpostfixnametable.txt` — lists which card IDs are suffixes (all others are prefixes)

### 1.3 Multiplier System for Duplicate Cards

When the **same card** is compounded multiple times, it does NOT repeat the word. Instead, a multiplier prefix is added:

| Count | Multiplier | Example (Hydra in weapon) |
|-------|-----------|---------------------------|
| 1× | *(none)* | `Bloody Blade [4]` |
| 2× | Double | `Double Bloody Blade [4]` |
| 3× | Triple | `Triple Bloody Blade [4]` |
| 4× | Quadruple | `Quadruple Bloody Blade [4]` |

The multiplier is applied **per unique card type**. Mixed cards each get their own multiplier:
- 2× Hydra + 2× Skeleton Worker = `Double Bloody Double Boned Blade [4]`
- 3× Hydra + 1× Skeleton Worker = `Triple Bloody Boned Blade [4]`

For **suffix cards**, the multiplier goes before the suffix text:
- 3× Verit (suffix "of Counter") = `Blade Triple of Counter [1]`

### 1.4 Card Ordering

Cards appear in the display name in **insertion order** (slot 0 first, slot 3 last). Unique cards are tracked by first appearance. If Hydra is in slot 0 and Skeleton Worker in slot 2, the name is "Bloody Boned" (not "Boned Bloody").

All prefixes are concatenated before the base name. All suffixes are concatenated after. Within each group, insertion order is preserved.

### 1.5 Slot Count Display — Always Shows TOTAL Slots

The bracket number `[N]` shows the **total number of slots** the item was manufactured with. It does NOT decrease as cards fill slots.

- `Blade [4]` — 4-slot, no cards
- `Quadruple Bloody Blade [4]` — 4-slot, ALL 4 slots filled — still shows `[4]`

Items with 0 slots show no brackets at all.

### 1.6 Full Name Assembly Order

```
{+RefineLevel} {Prefix1} {Prefix2} ... {BaseName} {Suffix1} {Suffix2} ... {[TotalSlots]}
```

Where each Prefix/Suffix has format: `{Multiplier}{Text}`

Examples:
```
+10 Triple Bloody Boned Blade [4]           — 3× Hydra + 1× Skel Worker
Double Bloody Double Boned Composite Bow [4] — 2× Hydra + 2× Skel Worker
Bloody Blade of Counter [4]                  — 1× Hydra (prefix) + 1× Verit (suffix)
+7 Cranial Buckler [1]                       — 1× Thara Frog
Immune Muffler [1]                           — 1× Raydric
Greaves of Hermes [1]                        — 1× Matyr
Holy Full Plate [1]                          — 1× Angeling
```

---

## 2. Current State in Sabri_MMO

### 2.1 What Already Exists (Working)

| Component | Status | Details |
|-----------|--------|---------|
| DB columns `card_prefix`, `card_suffix` | Exist but ALL NULL | `items` table has the columns |
| Server reads prefix/suffix from DB | Working | `getPlayerInventory()` query includes them |
| Server sends prefix/suffix to client | Working | `compounded_card_details` payload includes them |
| Client `FCompoundedCardInfo` struct | Working | Has `CardPrefix` and `CardSuffix` fields |
| Client `GetDisplayName()` method | Partially working | Renders prefix/suffix but **lacks multiplier system** |
| All UI widgets use `GetDisplayName()` | Working | Tooltip, inspect, inventory, equipment, shop |

### 2.2 What's Missing

| Component | Status | What's Needed |
|-----------|--------|---------------|
| **Card prefix/suffix DATA** | Missing | 538 cards need prefix/suffix values populated in DB |
| **Postfix flag DATA** | Missing | Need to know which cards are suffix vs prefix (~113 suffix cards) |
| **Multiplier logic in `GetDisplayName()`** | Missing | No Double/Triple/Quadruple handling for duplicate cards |
| **Migration script parsing** | Missing | `generate_canonical_migration.js` doesn't parse prefix/suffix |
| **`is_postfix` column or equivalent** | Missing | DB has `card_prefix` and `card_suffix` but no flag |

### 2.3 Current `GetDisplayName()` Bug

The current implementation iterates `CompoundedCardDetails` and blindly appends every card's prefix/suffix. For 3× Hydra, this produces:
```
Bloody Bloody Bloody Blade [4]   ← WRONG (current behavior)
Triple Bloody Blade [4]          ← CORRECT (RO Classic behavior)
```

---

## 3. Data Source — rAthena Card Prefix/Suffix Table

The canonical source is `cardprefixnametable.txt` from the RO client data. This maps ~441 card IDs to their naming text. A separate `cardpostfixnametable.txt` lists ~113 card IDs that are suffixes.

### 3.1 Representative Prefix Cards (go BEFORE item name)

| Card ID | Card Name | Prefix Text |
|---------|-----------|-------------|
| 4001 | Poring Card | Lucky |
| 4005 | Chonchon Card | Evasion |
| 4009 | Willow Card | Wise |
| 4012 | Andre Card | Hurricane |
| 4013 | Drops Card | Dexterous |
| 4025 | Munak Card | Ancient |
| 4028 | Thara Frog Card | Cranial |
| 4035 | Hydra Card | Bloody |
| 4036 | Archer Skeleton Card | Kingbird |
| 4043 | Vadon Card | Flammable |
| 4044 | Drainliar Card | Saharic |
| 4047 | Ghostring Card | *(uses suffix: "of Ghost")* |
| 4054 | Angeling Card | Holy |
| 4058 | Andre Larva Card | *(check)* |
| 4060 | Desert Wolf Card | Gigantic |
| 4066 | Khalitzburg Card | Brutal |
| 4092 | Skeleton Worker Card | Boned |
| 4094 | Skel Archer Card | *(see table)* |
| 4104 | Bathory Card | Evil |
| 4119 | Argiope Card | Poisoning |
| 4126 | Minorous Card | Titan |
| 4133 | Raydric Card | Immune |
| 4137 | Drake Card | *(MVP, special)* |
| 4140 | Abysmal Knight Card | Liberation |

### 3.2 Representative Suffix Cards (go AFTER item name)

| Card ID | Card Name | Suffix Text |
|---------|-----------|-------------|
| 4031 | Sohee Card | of Ares |
| 4032 | Whisper Card | of Athena |
| 4041 | Sandman Card | of Sandman |
| 4046 | Vitata Card | of Cleric |
| 4050 | Matyr Card | of Hermes |
| 4056 | Marse Card | of Warmth |
| 4059 | Marc Card | of Cadi |
| 4074 | Giearth Card | of Gargantua |
| 4091 | Verit Card | of Counter |
| 4098 | Dokebi Card | of Zephyrus |
| 4099 | Pasana Card | of Ifrit |
| 4101 | Hode Card | of Gnome |
| 4113 | Ghostring Card | of Ghost |

### 3.3 Data Population Strategy

We have two options:

**Option A: SQL migration with hardcoded data** — Create a migration file with UPDATE statements for all 538 cards. Data sourced from `cardprefixnametable.txt` and `cardpostfixnametable.txt`.

**Option B: Update the generator script** — Add prefix/suffix parsing to `generate_canonical_migration.js` so it's included in future regenerations. This requires having the source data files.

**Recommended: Option A first (immediate), Option B later (sustainable).** The source `.txt` files aren't in our repo, and rAthena YAML doesn't contain prefix/suffix data (it's client-side data, not server-side). We'll need a one-time SQL data migration.

---

## 4. Implementation Plan

### Phase 1: Database — Populate Card Prefix/Suffix Data

**Files modified:**
- `database/migrations/populate_card_naming.sql` (NEW)

**Tasks:**
- [ ] 1.1 Create migration SQL file with UPDATE statements for all cards
- [ ] 1.2 Each card gets: `card_prefix = 'Text'` (for prefix cards) OR `card_suffix = 'of Text'` (for suffix cards)
- [ ] 1.3 Convention: prefix cards have `card_prefix` populated and `card_suffix = NULL`; suffix cards have `card_suffix` populated and `card_prefix = NULL`
- [ ] 1.4 Source data from `cardprefixnametable.txt` + `cardpostfixnametable.txt` (eAthena/roBrowser data)
- [ ] 1.5 Run migration on local DB
- [ ] 1.6 Verify with: `SELECT item_id, name, card_prefix, card_suffix FROM items WHERE item_type = 'card' AND (card_prefix IS NOT NULL OR card_suffix IS NOT NULL) ORDER BY item_id;`

**Data format in DB:**
```sql
-- Prefix cards: text goes BEFORE item name
UPDATE items SET card_prefix = 'Bloody', card_suffix = NULL WHERE item_id = 4035;  -- Hydra
UPDATE items SET card_prefix = 'Boned', card_suffix = NULL WHERE item_id = 4092;   -- Skeleton Worker
UPDATE items SET card_prefix = 'Cranial', card_suffix = NULL WHERE item_id = 4028; -- Thara Frog

-- Suffix cards: text goes AFTER item name (starts with "of")
UPDATE items SET card_prefix = NULL, card_suffix = 'of Counter' WHERE item_id = 4091; -- Verit
UPDATE items SET card_prefix = NULL, card_suffix = 'of Hermes' WHERE item_id = 4050;  -- Matyr
UPDATE items SET card_prefix = NULL, card_suffix = 'of Ghost' WHERE item_id = 4113;   -- Ghostring
```

**Why this DB design works:**
- A card is a PREFIX if `card_prefix IS NOT NULL`
- A card is a SUFFIX if `card_suffix IS NOT NULL`
- No card has BOTH (mutually exclusive in RO Classic)
- No extra `is_postfix` column needed — NULL check is sufficient

### Phase 2: Server — Verify Payload (No Changes Expected)

**Files checked:**
- `server/src/index.js`

**Tasks:**
- [ ] 2.1 Verify `getPlayerInventory()` query includes `i.card_prefix, i.card_suffix` in SELECT
- [ ] 2.2 Verify `compounded_card_details` in inventory payload includes `card_prefix` and `card_suffix`
- [ ] 2.3 Verify `card:result` response includes prefix/suffix data
- [ ] 2.4 Test with a compounded item: confirm payload arrives at client with correct prefix/suffix

**Expected: No server code changes needed.** The server already reads and sends these fields.

### Phase 3: Client — Fix `GetDisplayName()` with Multiplier System

**Files modified:**
- `client/SabriMMO/Source/SabriMMO/CharacterData.h`

**Tasks:**
- [ ] 3.1 Rewrite `GetDisplayName()` to implement the RO Classic algorithm:

```
Algorithm:
1. Build a map of unique card -> {prefix/suffix text, count, first_seen_index}
2. Iterate CompoundedCardDetails in slot order (0 to N)
3. For each card, if new unique card: record its prefix/suffix and first_seen_index
4. If already seen: increment count
5. Build prefix string: for each unique PREFIX card (in first_seen order):
   - Prepend multiplier word if count > 1 (Double/Triple/Quadruple)
   - Append card prefix text + space
6. Build suffix string: for each unique SUFFIX card (in first_seen order):
   - Prepend space + multiplier word if count > 1
   - Append card suffix text
7. Assemble: [+Refine] [Prefixes] [BaseName] [Suffixes] [Slots]
```

- [ ] 3.2 Handle edge cases:
  - Card with both prefix AND suffix NULL (skip naming, just the effect)
  - Items with 0 cards (no prefix/suffix, just base name)
  - Mix of prefix and suffix cards
  - All 4 slots same card (Quadruple)
  - Empty slots in middle (CompoundedCardDetails may have invalid entries for empty slots)

**New `GetDisplayName()` implementation:**

```cpp
FString GetDisplayName() const
{
    FString Result;

    // 1. Refine prefix
    if (RefineLevel > 0)
        Result = FString::Printf(TEXT("+%d "), RefineLevel);

    // 2. Count unique cards and track insertion order
    static const TCHAR* Multipliers[] = { TEXT(""), TEXT("Double "), TEXT("Triple "), TEXT("Quadruple ") };

    struct FCardNaming
    {
        FString PrefixText;   // Non-empty if prefix card
        FString SuffixText;   // Non-empty if suffix card
        int32 Count = 0;      // 0-indexed: 0=first, 1=second, ...
        int32 FirstSeenOrder = 0;
    };

    TMap<int32, FCardNaming> UniqueCards;   // CardItemId -> naming data
    TArray<int32> InsertionOrder;           // Unique card IDs in slot order
    int32 OrderCounter = 0;

    for (const FCompoundedCardInfo& Card : CompoundedCardDetails)
    {
        if (!Card.IsValid()) continue;

        if (FCardNaming* Existing = UniqueCards.Find(Card.ItemId))
        {
            Existing->Count++;
        }
        else
        {
            FCardNaming NewEntry;
            NewEntry.PrefixText = Card.CardPrefix;
            NewEntry.SuffixText = Card.CardSuffix;
            NewEntry.Count = 0;  // 0 = first occurrence
            NewEntry.FirstSeenOrder = OrderCounter++;
            UniqueCards.Add(Card.ItemId, NewEntry);
            InsertionOrder.Add(Card.ItemId);
        }
    }

    // 3. Build prefix string (cards with non-empty CardPrefix, in insertion order)
    for (int32 CardId : InsertionOrder)
    {
        const FCardNaming& CN = UniqueCards[CardId];
        if (CN.PrefixText.IsEmpty()) continue;
        int32 MultIdx = FMath::Clamp(CN.Count, 0, 3);
        Result += Multipliers[MultIdx];
        Result += CN.PrefixText;
        Result += TEXT(" ");
    }

    // 4. Base name
    Result += Name;

    // 5. Build suffix string (cards with non-empty CardSuffix, in insertion order)
    for (int32 CardId : InsertionOrder)
    {
        const FCardNaming& CN = UniqueCards[CardId];
        if (CN.SuffixText.IsEmpty()) continue;
        int32 MultIdx = FMath::Clamp(CN.Count, 0, 3);
        Result += TEXT(" ");
        Result += Multipliers[MultIdx];
        Result += CN.SuffixText;
    }

    // 6. Slot count (total slots, NOT remaining empty)
    if (Slots > 0)
        Result += FString::Printf(TEXT(" [%d]"), Slots);

    return Result;
}
```

### Phase 4: Verification & Testing

**Tasks:**
- [ ] 4.1 Verify in-game with test items:
  - 1× Hydra in Blade → `Bloody Blade [4]`
  - 2× Hydra in Blade → `Double Bloody Blade [4]`
  - 3× Hydra + 1× Skel Worker → `Triple Bloody Boned Blade [4]`
  - 4× Hydra → `Quadruple Bloody Blade [4]`
  - 1× Matyr in Shoes → `Shoes of Hermes [1]`
  - 1× Hydra + 1× Verit → `Bloody Blade of Counter [4]`
  - +7, 2× Hydra + 2× Skel Worker → `+7 Double Bloody Double Boned Blade [4]`
  - 0-slot item with no cards → `Sword` (no brackets)
  - Unslotted item → just base name
- [ ] 4.2 Check all UI surfaces render correctly:
  - Inventory grid tooltip
  - Equipment panel tooltip
  - Item inspect popup title
  - Shop widget (buy/sell)
  - Hotbar tooltip
  - Card compound popup equipment list
- [ ] 4.3 Verify card compound flow still works end-to-end:
  - Compound a card → name updates immediately in inventory
  - Compound second same card → name shows "Double"

---

## 5. Complete Card Prefix/Suffix Reference Table

This is the data that needs to go into the migration SQL. Sourced from `cardprefixnametable.txt` and `cardpostfixnametable.txt` (eAthena/roBrowserLegacy).

### 5.1 Prefix Cards (text goes BEFORE item name)

| Card ID | Card Name | Prefix |
|---------|-----------|--------|
| 4001 | Poring Card | Lucky |
| 4002 | Fabre Card | Vital |
| 4003 | Pupa Card | Staunch |
| 4004 | Condor Card | *(check)* |
| 4005 | Chonchon Card | Evasion |
| 4006 | Thief Bug Egg Card | Warding |
| 4007 | Familiar Card | *(check)* |
| 4008 | Rocker Card | *(check)* |
| 4009 | Willow Card | Wise |
| 4010 | Picky Card | *(check)* |
| 4011 | Thief Bug Card | *(check)* |
| 4012 | Andre Card | Hurricane |
| 4013 | Drops Card | Dexterous |
| 4015 | Savage Babe Card | *(check)* |
| 4016 | Andre Larva Card | *(check)* |
| 4017 | Roda Frog Card | *(check)* |
| 4018 | Lunatic Card | *(check)* |
| 4019 | Pecopeco Egg Card | *(check)* |
| 4020 | Steel Chonchon Card | *(check)* |
| 4021 | Skeleton Card | *(check)* |
| 4025 | Munak Card | Ancient |
| 4026 | Poison Spore Card | *(check)* |
| 4027 | Plankton Card | *(check)* |
| 4028 | Thara Frog Card | Cranial |
| 4029 | Wormtail Card | *(check)* |
| 4033 | Horn Card | Heavy |
| 4034 | Carat Card | *(check)* |
| 4035 | Hydra Card | Bloody |
| 4040 | Muka Card | *(check)* |
| 4043 | Vadon Card | Flammable |
| 4044 | Drainliar Card | Saharic |
| 4047 | Ghostring Card | *(suffix — see below)* |
| 4048 | Picky Egg Card | *(check)* |
| 4049 | Scorpion Card | *(check)* |
| 4052 | Elder Willow Card | Erudite |
| 4053 | Myst Card | *(check)* |
| 4054 | Angeling Card | Holy |
| 4055 | Mandragora Card | Windy |
| 4060 | Desert Wolf Card | Gigantic |
| 4064 | Zerom Card | Nimble |
| 4066 | Khalitzburg Card | Brutal |
| 4077 | Phen Card | Under a Cast |
| 4092 | Skeleton Worker Card | Boned |
| 4104 | Bathory Card | Evil |
| 4114 | Argiope Card | Poisoning |
| 4119 | *(check)* | *(check)* |
| 4126 | Minorous Card | Titan |
| 4133 | Raydric Card | Immune |
| 4140 | Abysmal Knight Card | Liberation |

### 5.2 Suffix Cards (text goes AFTER item name)

| Card ID | Card Name | Suffix |
|---------|-----------|--------|
| 4014 | Thief Bug Card | of Champion |
| 4022 | Spore Card | of Spore |
| 4023 | Desert Wolf Puppy Card | of Desert |
| 4031 | Sohee Card | of Ares |
| 4032 | Whisper Card | of Athena |
| 4038 | Magnolia Card | of Health |
| 4039 | Poring Card | *(wait — 4039 != 4001; check which card)* |
| 4041 | Sandman Card | of Sandman |
| 4042 | Savage Babe Card | of He-Man |
| 4046 | Vitata Card | of Cleric |
| 4050 | Matyr Card | of Hermes |
| 4051 | Marina Card | of Flash |
| 4056 | Marse Card | of Warmth |
| 4059 | Marc Card | of Cadi |
| 4070 | Bongun Card | of Witch |
| 4074 | Giearth Card | of Gargantua |
| 4091 | Verit Card | of Counter |
| 4098 | Dokebi Card | of Zephyrus |
| 4099 | Pasana Card | of Ifrit |
| 4101 | Hode Card | of Gnome |
| 4113 | Ghostring Card | of Ghost |

> **NOTE**: The full list of ~441 cards with prefix/suffix text + ~113 postfix flags needs to be sourced from the actual `cardprefixnametable.txt` data file during implementation. The tables above are representative samples. The implementation task will web-fetch the complete data.

---

## 6. Architecture Summary

### What Changes

```
┌─────────────────────────────────────────────────────┐
│ DATABASE (items table)                              │
│ card_prefix: "Bloody" | NULL                        │  ← Populate via migration
│ card_suffix: NULL | "of Counter"                    │  ← Populate via migration
└───────────────┬─────────────────────────────────────┘
                │ Already reads & sends these
                ▼
┌─────────────────────────────────────────────────────┐
│ SERVER (index.js)                                   │
│ compounded_card_details[].card_prefix/card_suffix   │  ← No changes needed
└───────────────┬─────────────────────────────────────┘
                │ Already receives these
                ▼
┌─────────────────────────────────────────────────────┐
│ CLIENT (CharacterData.h)                            │
│ FCompoundedCardInfo.CardPrefix / CardSuffix         │  ← Already has fields
│ GetDisplayName()                                    │  ← FIX: Add multiplier logic
└───────────────┬─────────────────────────────────────┘
                │ Already calls GetDisplayName()
                ▼
┌─────────────────────────────────────────────────────┐
│ ALL UI WIDGETS                                      │
│ Inventory, Equipment, Inspect, Shop, Hotbar, Tooltip│  ← No changes needed
└─────────────────────────────────────────────────────┘
```

### What Does NOT Change
- Server `index.js` — already reads and sends card_prefix/card_suffix
- `FCompoundedCardInfo` struct — already has CardPrefix/CardSuffix fields
- `FInventoryItem` struct — already has CardPrefix/CardSuffix fields
- All widget code — already uses `GetDisplayName()`
- `ItemTooltipBuilder` — already uses `GetDisplayName()`
- `SItemInspectWidget` — already uses `GetDisplayName()`
- `SCardCompoundPopup` — equipment names use `GetDisplayName()`
- Card compound server handler — already sends prefix/suffix
- `ro_card_effects.js` — combat effects, unrelated to naming

### What Changes
1. **Database migration** — populate `card_prefix`/`card_suffix` for all 538 cards
2. **`GetDisplayName()` in CharacterData.h** — rewrite with multiplier counting algorithm

**That's it. Two changes.**

---

## 7. Answer: Visual Only or Actual Item Change?

**The card naming is 100% visual / client-side display only.**

- The database stores `compounded_cards JSONB` (array of card item IDs)
- The server sends the raw card data + card prefix/suffix text
- The client computes the display name at render time
- The item's `name` field in the DB is NEVER modified
- If you remove all cards (hypothetically), the name reverts to the base item name

This matches the original RO Classic design: the server has no concept of "item display name with cards". It just stores base item + card IDs. The client does all the cosmetic name assembly.

---

## 8. Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| Incomplete prefix/suffix data (some cards missing) | Medium | Web-fetch complete `cardprefixnametable.txt` during implementation |
| Prefix/suffix text mismatch with rAthena IDs | Low | Cross-reference with divine-pride.net and iRO wiki |
| GetDisplayName() regression breaks existing tooltips | Low | All widgets already call this method; just need correct output |
| Performance of counting algorithm | None | Max 4 cards per item; negligible overhead |
| Empty CompoundedCardDetails for uncarded items | None | Algorithm handles empty array gracefully |

---

## 9. Implementation Effort Estimate

| Phase | Effort | Complexity |
|-------|--------|-----------|
| Phase 1: DB migration (card data) | Medium | Data gathering + SQL generation |
| Phase 2: Server verification | Trivial | Just confirm existing code |
| Phase 3: Client GetDisplayName() fix | Small | ~40 lines of C++ |
| Phase 4: Testing | Medium | Need test items with various card combos |
| **Total** | **~2-3 hours** | Low complexity, mostly data population |
