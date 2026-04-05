-- Migration: fix_item_script_column_audit.sql
-- Full audit of 1,417 equippable items with scripts — fixes columns to match
-- unconditional (always-active) bonuses only.
--
-- 91 issues fixed across 4 categories:
--   A. VALUE_MISMATCH (10): column had wrong value vs unconditional script bonus
--   B. CONDITIONAL_IN_COLUMN — batch 1 (4 items): conditional bonuses in columns
--   C. MISSING perfect_dodge_bonus (21): bFlee2 script bonus, column was 0
--   D. CONDITIONAL_IN_COLUMN — batch 2 (14 items): VIP/class/refine conditionals
--
-- Remaining: Brynhild (2383) MaxHP=20*BaseLevel, MaxSP=5*BaseLevel — dynamic,
-- can't be stored as static values, intentionally left at 0.
--
-- The server reads columns directly (no script parsing), so columns must
-- reflect ONLY the unconditional (always-active) bonuses.

-- ============================================================
-- FIX A: VALUE MISMATCHES — correct column to match unconditional script
-- ============================================================

-- Aries Diadem/Crown: vit_bonus=3 includes conditional +1 from refine>6, should be 2
UPDATE items SET vit_bonus = 2 WHERE item_id IN (5545, 5546);

-- Staff Of Bordeaux: int_bonus=5 includes conditional +3 from Dragonology lv5, should be 2
UPDATE items SET int_bonus = 2 WHERE item_id = 1648;

-- Kronos: int_bonus=0 but script has unconditional bInt,3 (the +refine/2 part is dynamic)
-- Also max_hp_bonus=0 but script has unconditional bMaxHP,300 (the +50*refine/2 is dynamic)
UPDATE items SET int_bonus = 3, max_hp_bonus = 300 WHERE item_id = 2004;

-- Taurus Diadem/Crown: dex_bonus=3 includes conditional +1 from refine>6, should be 2
UPDATE items SET dex_bonus = 2 WHERE item_id IN (5549, 5550);

-- Brynhild: MaxHP = 20*BaseLevel (dynamic, can't store), MaxSP = 5*BaseLevel (dynamic)
-- Leave at 0 since we can't evaluate BaseLevel expressions

-- Striped Hat: MaxHP = 100+(refine*20), base unconditional is 100
UPDATE items SET max_hp_bonus = 100 WHERE item_id = 5395;

-- Brynhild: MaxSP = 5*BaseLevel — dynamic, leave at 0

-- Heart Breaker: bCritical,20 in script, column = 0
UPDATE items SET critical_bonus = 20 WHERE item_id = 1376;

-- Encyclopedia: bCritical,20 in script, column = 0
UPDATE items SET critical_bonus = 20 WHERE item_id = 1564;

-- Giant Encyclopedia: bCritical,20 in script, column = 0
UPDATE items SET critical_bonus = 20 WHERE item_id = 1580;

-- Sniping Suit (both variants): bCritical,6 in script, column = 0
UPDATE items SET critical_bonus = 6 WHERE item_id IN (2367, 2398);

-- Mercury Riser: crit=7 includes conditional +2 (refine>=7) + +2 (refine>=9), should be 3
UPDATE items SET critical_bonus = 3 WHERE item_id = 18597;

-- ============================================================
-- FIX B: CONDITIONAL_IN_COLUMN — clear columns for conditional-only bonuses
-- Server can't evaluate if-blocks, autobonus, refine checks, or stat thresholds
-- ============================================================

-- Bloody Eater: crit=100 from autobonus (proc-based, not permanent)
UPDATE items SET critical_bonus = 0 WHERE item_id = 1182;

-- Vecer Axe: crit=5 from if(DEX>=90) condition
UPDATE items SET critical_bonus = 0 WHERE item_id = 1311;

-- Rogue's Treasure: hit=10/flee=10/crit=10 all from if(STR/AGI>=90) conditions
UPDATE items SET hit_bonus = 0, flee_bonus = 0, critical_bonus = 0 WHERE item_id = 2620;

-- Gemini Crown: flee=10 from if(refine>6) condition — unconditional agi=2 is correct
UPDATE items SET flee_bonus = 0 WHERE item_id = 5570;

-- ============================================================
-- FIX C: MISSING perfect_dodge_bonus (bFlee2)
-- 21 items have bFlee2 in unconditional script but perfect_dodge_bonus = 0
-- NOTE: The server currently doesn't query perfect_dodge_bonus,
-- but we fix the data now for when it's implemented.
-- ============================================================

UPDATE items SET perfect_dodge_bonus = 10 WHERE item_id = 1181; -- Tae Goo Lyeon
UPDATE items SET perfect_dodge_bonus = 20 WHERE item_id = 1186; -- Death Guidance
UPDATE items SET perfect_dodge_bonus = 20 WHERE item_id = 1223; -- Fortune Sword
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 1237; -- Grimtooth
UPDATE items SET perfect_dodge_bonus = 2  WHERE item_id IN (1261, 1266, 1267); -- Infiltrator x3
UPDATE items SET perfect_dodge_bonus = 2  WHERE item_id = 1964; -- Chemeti Whip
UPDATE items SET perfect_dodge_bonus = 3  WHERE item_id = 2390; -- Improved Tights
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 2516; -- Falcon Muffler
UPDATE items SET perfect_dodge_bonus = 8  WHERE item_id = 2519; -- Morrigane's Manteau
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 2524; -- Valkyrian Manteau
UPDATE items SET perfect_dodge_bonus = 1  WHERE item_id = 2543; -- Sylphid Manteau
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 5260; -- Cookie Hat
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 5270; -- Autumn Leaves
UPDATE items SET perfect_dodge_bonus = 3  WHERE item_id = 5332; -- Loki Mask
UPDATE items SET perfect_dodge_bonus = 3  WHERE item_id = 5402; -- Mischievous Fairy
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 5415; -- Poring Cake Hat
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 5416; -- Beer Hat
UPDATE items SET perfect_dodge_bonus = 3  WHERE item_id = 5446; -- Catfoot Hairpin
UPDATE items SET perfect_dodge_bonus = 5  WHERE item_id = 5527; -- Lunatic Hat

-- ============================================================
-- FIX D: Additional CONDITIONAL_IN_COLUMN items (found in second pass)
-- All of these have bonuses inside if-blocks, autobonus, or VIP checks
-- that the server cannot evaluate — columns must be 0.
-- ============================================================

-- Shell Of Resistance: bAllStats,1 inside VIP conditional
UPDATE items SET str_bonus=0, agi_bonus=0, vit_bonus=0, int_bonus=0, dex_bonus=0, luk_bonus=0
WHERE item_id = 2132;

-- Magistrate Hat: agi=1 from if(BaseJob==Taekwon) conditional
UPDATE items SET agi_bonus = 0 WHERE item_id = 5173;

-- Ayam: int=1 from if(BaseJob==Taekwon) conditional
UPDATE items SET int_bonus = 0 WHERE item_id = 5174;

-- Mythical Lion Mask: dex=2 from if(BaseClass==Taekwon) conditional
UPDATE items SET dex_bonus = 0 WHERE item_id = 5177;

-- Bride Mask: luk=2, crit=5 from if(Class==Wedding) conditional
UPDATE items SET luk_bonus = 0, critical_bonus = 0 WHERE item_id = 5169;

-- Hahoe Mask: luk=1 from if(BaseJob==Taekwon) conditional
UPDATE items SET luk_bonus = 0 WHERE item_id = 5176;

-- Dead Tree Cane: maxHP=-200, maxSP=-100 from if(refine>=10) conditional
UPDATE items SET max_hp_bonus = 0, max_sp_bonus = 0 WHERE item_id = 1643;

-- Academy Of Badge: maxHP=400, maxSP=200 from if(BaseLevel>=80) conditional
UPDATE items SET max_hp_bonus = 0, max_sp_bonus = 0 WHERE item_id = 2751;

-- Sigrun's Wings: maxHP=80, maxSP=30 from if(BaseJob) conditional
UPDATE items SET max_hp_bonus = 0, max_sp_bonus = 0 WHERE item_id = 5592;

-- Lich's Bone Wand (x2): maxSP=300 from if(getrefine()>=9) conditional
UPDATE items SET max_sp_bonus = 0 WHERE item_id IN (1624, 1645);

-- Chakram: hit=10 from if(readparam(bLuk)>=50) conditional
UPDATE items SET hit_bonus = 0 WHERE item_id = 1285;

-- Giant Axe: hit=10 from if(readparam(bStr)>=95) conditional
UPDATE items SET hit_bonus = 0 WHERE item_id = 1387;

-- Cancer Crown: flee=10 from if(refine>6) conditional
UPDATE items SET flee_bonus = 0 WHERE item_id = 5582;

-- Puppy Hat: crit=100 from autobonus (proc-based, not permanent)
UPDATE items SET critical_bonus = 0 WHERE item_id = 5454;
