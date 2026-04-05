# Document 3: Client Stats UI & Tooltip Updates

**Dependency**: Document 1 (Server Pipeline) and Document 2 (Formula Fixes) must be implemented first
**Reason**: The server must send the new fields (`hardMdef`, `weaponMATK`, `matkMin`, `matkMax`) before the client can parse and display them.

---

## Bugs Fixed

| # | Bug | Impact |
|---|-----|--------|
| 1 | MDEF hard component hardcoded to "0" in stats widget | Players can't see equipment MDEF |
| 2 | MATK displayed as single number, not min~max range | Missing RO Classic MATK range display |
| 3 | `hit_bonus, flee_bonus, critical_bonus, perfect_dodge_bonus` not parsed by client | Item tooltips can't show these bonuses |
| 4 | `FInventoryItem` missing bonus fields | No struct fields to store the bonus data |

---

## Files Modified

| File | Changes |
|------|---------|
| `client/SabriMMO/Source/SabriMMO/CharacterData.h` | Add 4 fields to `FInventoryItem`, 4 to `FShopItem`, update `ToInspectableItem()` |
| `client/SabriMMO/Source/SabriMMO/UI/CombatStatsSubsystem.h` | Add `HardMDEF`, `MATKMin`, `MATKMax`, `WeaponMATK` fields |
| `client/SabriMMO/Source/SabriMMO/UI/CombatStatsSubsystem.cpp` | Parse new fields in `HandlePlayerStats()` |
| `client/SabriMMO/Source/SabriMMO/UI/SCombatStatsWidget.cpp` | Fix MDEF display, fix MATK display |
| `client/SabriMMO/Source/SabriMMO/UI/InventorySubsystem.cpp` | Parse 4 new bonus fields in `ParseItemFromJson()` |
| `client/SabriMMO/Source/SabriMMO/UI/ItemTooltipBuilder.cpp` | Show HIT/FLEE/Crit/PD bonuses in tooltips |

**NOTE**: Changes to `.h` (header) files require an **editor restart** — Live Coding only works for `.cpp` changes.

---

## Implementation Steps

---

### Step 1: Add Fields to `FInventoryItem` in `CharacterData.h`

**FILE**: `client/SabriMMO/Source/SabriMMO/CharacterData.h`

**FIND** (lines 86-88):
```cpp
	int32 MaxHPBonus = 0;
	int32 MaxSPBonus = 0;
	int32 RequiredLevel = 1;
```

**REPLACE** with:
```cpp
	int32 MaxHPBonus = 0;
	int32 MaxSPBonus = 0;
	int32 HitBonus = 0;
	int32 FleeBonus = 0;
	int32 CriticalBonus = 0;
	int32 PerfectDodgeBonus = 0;
	int32 RequiredLevel = 1;
```

---

### Step 2: Add Fields to `FShopItem` in `CharacterData.h`

**FILE**: `client/SabriMMO/Source/SabriMMO/CharacterData.h`

**FIND** (lines 243-244):
```cpp
	int32 MaxHPBonus = 0;
	int32 MaxSPBonus = 0;
```

**REPLACE** with:
```cpp
	int32 MaxHPBonus = 0;
	int32 MaxSPBonus = 0;
	int32 HitBonus = 0;
	int32 FleeBonus = 0;
	int32 CriticalBonus = 0;
	int32 PerfectDodgeBonus = 0;
```

---

### Step 3: Update `ToInspectableItem()` in `FShopItem` (CharacterData.h)

**FILE**: `client/SabriMMO/Source/SabriMMO/CharacterData.h`

**FIND** (inside `FShopItem::ToInspectableItem()`, lines 289-290):
```cpp
		Item.MaxHPBonus = MaxHPBonus;
		Item.MaxSPBonus = MaxSPBonus;
```

**REPLACE** with:
```cpp
		Item.MaxHPBonus = MaxHPBonus;
		Item.MaxSPBonus = MaxSPBonus;
		Item.HitBonus = HitBonus;
		Item.FleeBonus = FleeBonus;
		Item.CriticalBonus = CriticalBonus;
		Item.PerfectDodgeBonus = PerfectDodgeBonus;
```

---

### Step 4: Add Fields to `CombatStatsSubsystem.h`

**FILE**: `client/SabriMMO/Source/SabriMMO/UI/CombatStatsSubsystem.h`

**FIND** the derived combat stats block (the section with `StatusATK`, `WeaponATK`, etc.):
```cpp
    int32 StatusATK = 0;
    int32 WeaponATK = 0;
    int32 PassiveATK = 0;
    int32 StatusMATK = 0;
    int32 HIT = 0;
    int32 FLEE = 0;
    int32 Critical = 0;
    int32 PerfectDodge = 0;
    int32 SoftDEF = 0;
    int32 SoftMDEF = 0;
    int32 HardDEF = 0;
    int32 ASPD = 0;
```

**REPLACE** with:
```cpp
    int32 StatusATK = 0;
    int32 WeaponATK = 0;
    int32 PassiveATK = 0;
    int32 StatusMATK = 0;
    int32 MATKMin = 0;
    int32 MATKMax = 0;
    int32 WeaponMATK = 0;
    int32 HIT = 0;
    int32 FLEE = 0;
    int32 Critical = 0;
    int32 PerfectDodge = 0;
    int32 SoftDEF = 0;
    int32 SoftMDEF = 0;
    int32 HardDEF = 0;
    int32 HardMDEF = 0;
    int32 ASPD = 0;
```

---

### Step 5: Parse New Fields in `HandlePlayerStats()` (CombatStatsSubsystem.cpp)

**FILE**: `client/SabriMMO/Source/SabriMMO/UI/CombatStatsSubsystem.cpp`

**FIND** in `HandlePlayerStats()`, the block that parses from the `stats` object (around lines 93-108). After the existing `hardDef` parse line:

```cpp
    if (StatsObj->TryGetNumberField(TEXT("hardDef"), DVal)) HardDEF = (int32)DVal;
```

**ADD** after it:
```cpp
    if (StatsObj->TryGetNumberField(TEXT("hardMdef"), DVal)) HardMDEF = (int32)DVal;
    if (StatsObj->TryGetNumberField(TEXT("weaponMATK"), DVal)) WeaponMATK = (int32)DVal;
```

**FIND** in `HandlePlayerStats()`, the block that parses from the `derived` object (around lines 137-150). After the existing `softMDEF` parse line:

```cpp
    if (DerivedObj->TryGetNumberField(TEXT("softMDEF"), DVal)) SoftMDEF = (int32)DVal;
```

**ADD** after it:
```cpp
    if (DerivedObj->TryGetNumberField(TEXT("matkMin"), DVal)) MATKMin = (int32)DVal;
    if (DerivedObj->TryGetNumberField(TEXT("matkMax"), DVal)) MATKMax = (int32)DVal;
```

---

### Step 6: Fix MDEF Display in `SCombatStatsWidget.cpp`

**FILE**: `client/SabriMMO/Source/SabriMMO/UI/SCombatStatsWidget.cpp`

**FIND** the MDEF stat row (around line 157). It currently has a hardcoded "0":
```cpp
.Text_Lambda([Sub]() -> FText {
    if (!Sub) return FText::GetEmpty();
    return FText::FromString(FString::Printf(TEXT("0 + %d"), Sub->SoftMDEF));
})
```

**REPLACE** with:
```cpp
.Text_Lambda([Sub]() -> FText {
    if (!Sub) return FText::GetEmpty();
    return FText::FromString(FString::Printf(TEXT("%d + %d"), Sub->HardMDEF, Sub->SoftMDEF));
})
```

---

### Step 7: Fix MATK Display in `SCombatStatsWidget.cpp`

**FILE**: `client/SabriMMO/Source/SabriMMO/UI/SCombatStatsWidget.cpp`

**FIND** the MATK stat row (around line 135). It currently shows only `StatusMATK`:
```cpp
.Text_Lambda([Sub]() -> FText {
    if (!Sub) return FText::GetEmpty();
    return FText::FromString(FString::FromInt(Sub->StatusMATK));
})
```

**REPLACE** with:
```cpp
.Text_Lambda([Sub]() -> FText {
    if (!Sub) return FText::GetEmpty();
    if (Sub->MATKMin > 0 && Sub->MATKMax > 0 && Sub->MATKMin != Sub->MATKMax)
        return FText::FromString(FString::Printf(TEXT("%d ~ %d"), Sub->MATKMin, Sub->MATKMax));
    if (Sub->MATKMax > 0)
        return FText::FromString(FString::FromInt(Sub->MATKMax));
    return FText::FromString(FString::FromInt(Sub->StatusMATK));
})
```

This shows:
- `"45 ~ 72"` when min != max (normal case with INT > 1)
- `"72"` when min == max (edge case)
- Falls back to `StatusMATK` if the server hasn't sent matkMin/matkMax yet

---

### Step 8: Parse New Bonus Fields in `ParseItemFromJson()` (InventorySubsystem.cpp)

**FILE**: `client/SabriMMO/Source/SabriMMO/UI/InventorySubsystem.cpp`

**FIND** in `ParseItemFromJson()` (around lines 116-117):
```cpp
	if (Obj->TryGetNumberField(TEXT("max_hp_bonus"), Val)) Item.MaxHPBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("max_sp_bonus"), Val)) Item.MaxSPBonus = (int32)Val;
```

**ADD** after those lines:
```cpp
	if (Obj->TryGetNumberField(TEXT("hit_bonus"), Val)) Item.HitBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("flee_bonus"), Val)) Item.FleeBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("critical_bonus"), Val)) Item.CriticalBonus = (int32)Val;
	if (Obj->TryGetNumberField(TEXT("perfect_dodge_bonus"), Val)) Item.PerfectDodgeBonus = (int32)Val;
```

---

### Step 9: Show New Bonus Fields in Tooltips (ItemTooltipBuilder.cpp)

**FILE**: `client/SabriMMO/Source/SabriMMO/UI/ItemTooltipBuilder.cpp`

**FIND** the stat bonus section (around lines 125-129). Currently it shows ATK, MATK, DEF, MDEF, Weight:
```cpp
	if (Item.ATK > 0) AddStatLine(TEXT("ATK"), Item.ATK);
	if (Item.MATK > 0) AddStatLine(TEXT("MATK"), Item.MATK);
	if (Item.DEF > 0) AddStatLine(TEXT("DEF"), Item.DEF);
	if (Item.MDEF > 0) AddStatLine(TEXT("MDEF"), Item.MDEF);
	if (Item.Weight > 0) AddStatLine(TEXT("Weight"), Item.Weight);
```

**REPLACE** with (add stat bonuses between MDEF and Weight):
```cpp
	if (Item.ATK > 0) AddStatLine(TEXT("ATK"), Item.ATK);
	if (Item.MATK > 0) AddStatLine(TEXT("MATK"), Item.MATK);
	if (Item.DEF > 0) AddStatLine(TEXT("DEF"), Item.DEF);
	if (Item.MDEF > 0) AddStatLine(TEXT("MDEF"), Item.MDEF);
	if (Item.StrBonus != 0) AddStatLine(TEXT("STR"), Item.StrBonus);
	if (Item.AgiBonus != 0) AddStatLine(TEXT("AGI"), Item.AgiBonus);
	if (Item.VitBonus != 0) AddStatLine(TEXT("VIT"), Item.VitBonus);
	if (Item.IntBonus != 0) AddStatLine(TEXT("INT"), Item.IntBonus);
	if (Item.DexBonus != 0) AddStatLine(TEXT("DEX"), Item.DexBonus);
	if (Item.LukBonus != 0) AddStatLine(TEXT("LUK"), Item.LukBonus);
	if (Item.MaxHPBonus != 0) AddStatLine(TEXT("Max HP"), Item.MaxHPBonus);
	if (Item.MaxSPBonus != 0) AddStatLine(TEXT("Max SP"), Item.MaxSPBonus);
	if (Item.HitBonus != 0) AddStatLine(TEXT("HIT"), Item.HitBonus);
	if (Item.FleeBonus != 0) AddStatLine(TEXT("FLEE"), Item.FleeBonus);
	if (Item.CriticalBonus != 0) AddStatLine(TEXT("Critical"), Item.CriticalBonus);
	if (Item.PerfectDodgeBonus != 0) AddStatLine(TEXT("P.Dodge"), Item.PerfectDodgeBonus);
	if (Item.Weight > 0) AddStatLine(TEXT("Weight"), Item.Weight);
```

**Note**: Using `!= 0` instead of `> 0` for stat bonuses because some items have negative bonuses (e.g., Tao Gunka Card: MaxHP +100%, DEF -50). The `AddStatLine` lambda uses `%d` format which will show negative values with a minus sign.

---

### Step 10: Update `AddStatLine` to Show Sign for Bonuses (ItemTooltipBuilder.cpp)

The current `AddStatLine` lambda shows `"STR: 5"` but RO Classic convention shows bonuses with a sign: `"STR + 5"` or `"DEF - 50"`. This is optional but improves readability.

**FIND** (lines 114-123):
```cpp
	auto AddStatLine = [&Content](const FString& Label, int32 Value)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%s: %d"), *Label, Value)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(TooltipColors::TextPrimary))
		];
	};
```

**REPLACE** with two lambdas — one for raw stats (ATK, DEF, Weight), one for bonuses:
```cpp
	auto AddStatLine = [&Content](const FString& Label, int32 Value)
	{
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%s: %d"), *Label, Value)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(TooltipColors::TextPrimary))
		];
	};

	auto AddBonusLine = [&Content](const FString& Label, int32 Value)
	{
		FString Sign = Value >= 0 ? TEXT("+ ") : TEXT("- ");
		int32 AbsVal = FMath::Abs(Value);
		Content->AddSlot().AutoHeight().Padding(4, 0)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%s %s%d"), *Label, *Sign, AbsVal)))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 7))
			.ColorAndOpacity(FSlateColor(Value >= 0 ? TooltipColors::TextPrimary : FLinearColor(0.9f, 0.3f, 0.3f, 1.f)))
		];
	};
```

Then update Step 9's bonus lines to use `AddBonusLine` instead of `AddStatLine`:
```cpp
	if (Item.ATK > 0) AddStatLine(TEXT("ATK"), Item.ATK);
	if (Item.MATK > 0) AddStatLine(TEXT("MATK"), Item.MATK);
	if (Item.DEF > 0) AddStatLine(TEXT("DEF"), Item.DEF);
	if (Item.MDEF > 0) AddStatLine(TEXT("MDEF"), Item.MDEF);
	if (Item.StrBonus != 0) AddBonusLine(TEXT("STR"), Item.StrBonus);
	if (Item.AgiBonus != 0) AddBonusLine(TEXT("AGI"), Item.AgiBonus);
	if (Item.VitBonus != 0) AddBonusLine(TEXT("VIT"), Item.VitBonus);
	if (Item.IntBonus != 0) AddBonusLine(TEXT("INT"), Item.IntBonus);
	if (Item.DexBonus != 0) AddBonusLine(TEXT("DEX"), Item.DexBonus);
	if (Item.LukBonus != 0) AddBonusLine(TEXT("LUK"), Item.LukBonus);
	if (Item.MaxHPBonus != 0) AddBonusLine(TEXT("Max HP"), Item.MaxHPBonus);
	if (Item.MaxSPBonus != 0) AddBonusLine(TEXT("Max SP"), Item.MaxSPBonus);
	if (Item.HitBonus != 0) AddBonusLine(TEXT("HIT"), Item.HitBonus);
	if (Item.FleeBonus != 0) AddBonusLine(TEXT("FLEE"), Item.FleeBonus);
	if (Item.CriticalBonus != 0) AddBonusLine(TEXT("Critical"), Item.CriticalBonus);
	if (Item.PerfectDodgeBonus != 0) AddBonusLine(TEXT("P.Dodge"), Item.PerfectDodgeBonus);
	if (Item.Weight > 0) AddStatLine(TEXT("Weight"), Item.Weight);
```

---

### Step 11: Update Shop Widget ParseShopItem (if applicable)

If `SShopWidget.cpp` has its own item parsing (separate from `ParseItemFromJson`), the 4 new bonus fields need to be parsed there too.

**CHECK**: `client/SabriMMO/Source/SabriMMO/UI/SShopWidget.cpp` — look for shop item JSON parsing. If it parses `FShopItem` structs, find where `max_hp_bonus` and `max_sp_bonus` are parsed, and add:
```cpp
if (ItemObj->TryGetNumberField(TEXT("hit_bonus"), DVal)) ShopItem.HitBonus = (int32)DVal;
if (ItemObj->TryGetNumberField(TEXT("flee_bonus"), DVal)) ShopItem.FleeBonus = (int32)DVal;
if (ItemObj->TryGetNumberField(TEXT("critical_bonus"), DVal)) ShopItem.CriticalBonus = (int32)DVal;
if (ItemObj->TryGetNumberField(TEXT("perfect_dodge_bonus"), DVal)) ShopItem.PerfectDodgeBonus = (int32)DVal;
```

**Note**: The shop:data server event uses a different query path. Verify the server's `shop:data` handler also selects these columns from the items table. If not, add them to the shop query. The server shop handler (around line 6738 in index.js) queries items for the shop — check if `hit_bonus, flee_bonus, critical_bonus, perfect_dodge_bonus` are included. If they aren't, add them:

In the server `shop:data` handler, find the items query and ensure these 4 columns are selected. This is needed for shop tooltips to show the bonuses.

---

## Header vs CPP Change Summary

| File | Type | Requires Editor Restart? |
|------|------|------------------------|
| `CharacterData.h` | Header | **YES** |
| `CombatStatsSubsystem.h` | Header | **YES** |
| `CombatStatsSubsystem.cpp` | Source | No (Live Coding OK) |
| `SCombatStatsWidget.cpp` | Source | No (Live Coding OK) |
| `InventorySubsystem.cpp` | Source | No (Live Coding OK) |
| `ItemTooltipBuilder.cpp` | Source | No (Live Coding OK) |

**Recommended workflow**: Make all header changes first, restart editor, then make all .cpp changes and use Live Coding.

---

## Testing Checklist

### Stats Widget (F8 Panel)
- [ ] MDEF row shows `"X + Y"` where X = equipment MDEF (HardMDEF), Y = formula MDEF (SoftMDEF)
- [ ] MDEF shows `"0 + Y"` when no equipment has MDEF (same as before but now dynamic)
- [ ] Equip item with MDEF bonus → X increases
- [ ] MATK row shows `"45 ~ 72"` format when INT > 1
- [ ] Equip weapon with MATK → range increases (both min and max go up)
- [ ] Character with INT 1 → MATK shows `"1"` or `"1 ~ 1"` (single value)

### Item Tooltips
- [ ] Hover over weapon → shows ATK, MATK (if > 0), DEF, MDEF, stat bonuses, Weight
- [ ] Item with STR +5 → tooltip shows `"STR + 5"` in normal color
- [ ] Item with DEF -50 → tooltip shows `"DEF - 50"` in red color
- [ ] Item with HIT bonus → tooltip shows `"HIT + X"`
- [ ] Item with FLEE bonus → tooltip shows `"FLEE + X"`
- [ ] Item with Critical bonus → tooltip shows `"Critical + X"`
- [ ] Item with Perfect Dodge bonus → tooltip shows `"P.Dodge + X"`
- [ ] Items without these bonuses → no extra lines shown (clean tooltip)

### Inventory Display
- [ ] `ParseItemFromJson` correctly reads `hit_bonus`, `flee_bonus`, `critical_bonus`, `perfect_dodge_bonus`
- [ ] Items in inventory grid show these bonuses in their tooltips
- [ ] Equipment panel shows these bonuses in tooltips for equipped items

### Shop Display
- [ ] Shop items show stat bonuses in tooltips
- [ ] Right-click inspect on shop items shows bonuses

### Regression
- [ ] Existing tooltip content (name, type, description, ATK, DEF, Weight, Required Level) still displays correctly
- [ ] F8 stats panel still shows all existing stats correctly (ATK, HIT, Critical, ASPD, DEF, FLEE, P.Dodge)
- [ ] Drag and drop from inventory/equipment/skill tree to hotbar still works
- [ ] Item inspect window still works (right-click)
- [ ] 2 PIE instances: each player sees own stats, tooltips are correct

### Visual
- [ ] MATK range text fits within the stat row width (no truncation)
- [ ] MDEF text fits within the stat row width
- [ ] Bonus lines in tooltips don't cause tooltip to become too wide (200px width constraint from SBox)
- [ ] Negative bonus values appear in red (FLinearColor(0.9f, 0.3f, 0.3f, 1.f))
- [ ] Positive bonus values appear in normal text color (TooltipColors::TextPrimary)

---

## Complete Data Flow (All 3 Documents)

```
DB items table has: atk, def, matk, mdef, str_bonus...luk_bonus, max_hp_bonus, max_sp_bonus,
                    hit_bonus, flee_bonus, critical_bonus, perfect_dodge_bonus, element, two_handed
    ↓ [Doc 1: Server reads ALL columns]
Server tracks: equipmentBonuses.{str..luk, maxHp, maxSp, hit, flee, critical, perfectDodge}
               player.hardDef, player.hardMdef, player.weaponMATK
               player.weaponElement, player.armorElement
    ↓ [Doc 1: getEffectiveStats passes all fields]
effectiveStats: { str..luk (merged), bonusPerfectDodge, weaponMATK, jobClass, weaponType, hardMdef, ... }
    ↓ [Doc 2: calculateDerivedStats uses class-aware formulas]
derived: { statusATK, statusMATK, matkMin, matkMax, hit, flee, critical, perfectDodge,
           softDEF, softMDEF, aspd, maxHP, maxSP }
    ↓ [Doc 2: buildFullStatsPayload applies buff multipliers]
player:stats payload: { stats: { hardDef, hardMdef, weaponMATK, ... },
                        derived: { matkMin, matkMax, softMDEF(+buffMDEF), ... } }
    ↓ [Doc 3: Client parses new fields]
CombatStatsSubsystem: HardMDEF, MATKMin, MATKMax, WeaponMATK
    ↓ [Doc 3: Widget displays correctly]
MDEF row: "5 + 38"  (was "0 + 7")
MATK row: "45 ~ 72"  (was "7")
Tooltips: "STR + 5", "HIT + 10", "FLEE + 5", "P.Dodge + 3"
```
